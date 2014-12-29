#include <apis/composer/composer.h>
#include <application/rpc/rpc.h>
#include <core/core.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>
#include <syslog.h>
#include <assert.h>
#include <stdio.h>

#include "bcm_host.h"
#include <GLES/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

struct stak_composer_element_s{
    DISPMANX_RESOURCE_HANDLE_T resource_handle;
    DISPMANX_ELEMENT_HANDLE_T element_handle;
    VC_RECT_T rect;
    EGL_DISPMANX_WINDOW_T nativewindow;
};

struct stak_composer_state_s{
    DISPMANX_DISPLAY_HANDLE_T display;
    DISPMANX_RESOURCE_HANDLE_T resource;
    struct stak_composer_element_s* elements;
}*stak_composer_state;


typedef enum {
    STAK_CANVAS_OFFSCREEN = 0x01,
    STAK_CANVAS_ONSCREEN = 0x02
}stak_composer_flags;

// function prototype for update thread
void* rpc_respond(void* arg);

// max and min helper macros
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

// global state information
volatile int terminate; // set to true to kill the update thread.
const int stak_composer_num_elements = 32;
const int stak_composer_final_element = 0;
const int stak_composer_internal_fb_width = 512;
const int stak_composer_internal_fb_height = 512;

pthread_t thread_rpc_respond;
typedef int (*stak_rpc_call)( struct stak_rpc_msgbuf* );
struct stak_rpc_call_responder {
    uint32_t call;
    stak_rpc_call fptr;
}; 

// we'll need an rpc message buffer
struct stak_rpc_msgbuf* message_buffer;

int create_gl_layer( struct stak_rpc_msgbuf* msg ) {
    int width = stak_rpc_buffer_get_int(msg, 0);
    int height = stak_rpc_buffer_get_int(msg, 1);
    int opengl_width = width;
    int opengl_height = height;
    int flags = STAK_CANVAS_ONSCREEN;
    uint32_t vc_image_ptr = 0;
    struct stak_composer_element_s* element = calloc(1, sizeof(struct stak_composer_element_s));
    
    element->resource_handle = vc_dispmanx_resource_create(VC_IMAGE_RGB565, opengl_width, opengl_height, &vc_image_ptr);
    if (!element->resource_handle)
    {
        printf("composerd: Unable to create video core resource\n");
        return 0;
    }
    printf("composerd: created gl layer %i\n", element->resource_handle);

    
    printf("composerd: opened display %i\n", stak_composer_state->display);


    uint32_t screen_width = opengl_width;
    uint32_t screen_height = opengl_height;
    VC_RECT_T screen_rect;
    graphics_get_display_size(stak_composer_state->display , &screen_width, &screen_height);

    vc_dispmanx_rect_set( &screen_rect, 0, 0, opengl_width, opengl_height );
    vc_dispmanx_rect_set( &element->rect, 0, 0, opengl_width << 16, opengl_height << 16 );

    VC_DISPMANX_ALPHA_T alpha = { DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 255, 0 };
    DISPMANX_UPDATE_HANDLE_T update;

    update = vc_dispmanx_update_start( 0 );
    element->element_handle = vc_dispmanx_element_add(
        update,                         // update
        stak_composer_state->display,   // display
        1,                              // layer
        &screen_rect,                   // dst rect
        element->resource_handle,       // src
        &element->rect,                 // src rect
        DISPMANX_PROTECTION_NONE,       // protection
        &alpha,                         // alpha
        0,                              // clamp
        DISPMANX_NO_ROTATE              // transform
    );
    vc_dispmanx_update_submit_sync( update );
    if( element->element_handle == -1 ) {
        printf("Error adding dispmanx element\n");
    }

    assert(element->element_handle != 0);

    element->nativewindow.element = element->element_handle;
    element->nativewindow.width = opengl_width;
    element->nativewindow.height = opengl_height;


    int element_id = 0;
    for( ; element_id < stak_composer_num_elements; element_id++ ) {
        struct stak_composer_element_s* dst_element = &stak_composer_state->elements[element_id];
        if( dst_element->resource_handle == 0 ) {
            memcpy( dst_element, element, sizeof(struct stak_composer_element_s) );
            break;
        }
    }

    long message[] = {
        RPC_COMPOSER_CREATE_GL_CONTEXT_RESPONSE,
        4,
        element->nativewindow.element,
        element->nativewindow.width,
        element->nativewindow.height,
        element_id
    };
    printf("Element: %i\n", element->nativewindow.element);
    printf("Width: %i\n", element->nativewindow.width);
    printf("Height: %i\n", element->nativewindow.height);
    printf("Layer: %i\n", element_id);

    stak_rpc_message_send((struct stak_rpc_msgbuf *)&message, 4);
    free(element);
    return 0;
}
int destroy_gl_layer( uint32_t element_id ) {
    if( ( element_id < 0 ) ||
        ( element_id > stak_composer_num_elements ) )
        return -1;

    DISPMANX_UPDATE_HANDLE_T update;
    struct stak_composer_element_s* element = &stak_composer_state->elements[element_id];
    if( element->resource_handle != 0 ) {
        update = vc_dispmanx_update_start( 0 );
        vc_dispmanx_element_remove(update, element->element_handle);
        element->element_handle = 0;
        vc_dispmanx_resource_delete(element->resource_handle);
        element->resource_handle = 0;
        vc_dispmanx_update_submit_sync( update );
    }
    return 0;
}


int init() {

    bcm_host_init();
    
    uint32_t vc_image_ptr = 0;
    DISPMANX_UPDATE_HANDLE_T update;
    
    stak_composer_state = calloc(1, sizeof(struct stak_composer_state_s));
    stak_composer_state->elements = calloc(stak_composer_num_elements, sizeof(struct stak_composer_element_s));

    struct stak_composer_element_s* element = &stak_composer_state->elements[stak_composer_final_element];
    element->resource_handle = vc_dispmanx_resource_create(
            VC_IMAGE_RGB565,
            stak_composer_internal_fb_width,
            stak_composer_internal_fb_height,
            &vc_image_ptr
        );

    if (!element->resource_handle)
    {
        printf("composerd: Unable to create main dispmanx resource.\n");
        return 0;
    }
    printf("composerd: created resource %i\n", element->resource_handle);

    if( 1 ) {
        // open main display
        stak_composer_state->display = vc_dispmanx_display_open( 0 );
    } else {
        // open offscreen display and 512x512 resource
        stak_composer_state->display = vc_dispmanx_display_open_offscreen( element->resource_handle, DISPMANX_NO_ROTATE );
    }

    if( stak_composer_state->display == -1 ) {
        printf("Error opening new display\n");
    }
    update = vc_dispmanx_update_start( 0 );
    vc_dispmanx_display_set_background( update, stak_composer_state->display, 255, 0, 255 );
    vc_dispmanx_update_submit_sync( update );


    stak_rpc_init();

    // allocate message buffer to double the maximum message size for now
    message_buffer = malloc(256);

    // make sure the thread doesn't start terminated
    terminate = 0;

    // launch gpio update thread
    //pthread_create(&thread_rpc_respond, NULL, rpc_respond, 0);

    return 0;
}

int shutdown() {
    // shutdown and wait for thread to end
    terminate = 1;
    /*if(pthread_join(thread_rpc_respond, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return -1;
    }*/
    int element_id = 0;
    for( ; element_id < stak_composer_num_elements; element_id++ ) {
        destroy_gl_layer( element_id );
    }
    vc_dispmanx_display_close(stak_composer_state->display);


    return 0;
}

void* rpc_respond(void* arg) {
    
    return 0;
}
int update() {
    //while(!terminate) {
        static long message_buffer[10];
        struct stak_rpc_msgbuf* msg = (struct stak_rpc_msgbuf *)&message_buffer;

        // get any messages available in rpc queue
        if( stak_rpc_message_get(msg) == 0 ) {
            switch(msg->mtype) {

                // RPC FUNCTION: stak_rpc_input_get_rotary_position
                // ARGUMENTS: none
                // RETURNS: int containing rotary encoder position
                case RPC_COMPOSER_CREATE_GL_CONTEXT:
                    create_gl_layer( msg );
                    break;
                case RPC_COMPOSER_DESTROY_GL_CONTEXT:
                    destroy_gl_layer( stak_rpc_buffer_get_int( msg, 0) );
                    break;
                default:
                    break;
            }
        }
    //}
    return 0;
}
