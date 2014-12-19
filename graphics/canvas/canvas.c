#include "canvas.h"
#include <GLES/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <syslog.h>
#include <assert.h>
#include <stdio.h>

#define OPENGL_WIDTH 96
#define OPENGL_HEIGHT 96
static VC_RECT_T screen_rect;
stak_canvas_s* stak_canvas_create(stak_canvas_flags flags, uint32_t canvas_w, uint32_t canvas_h) {
//#define RENDER_WINDOW_ONSCREEN

    stak_canvas_s* canvas = calloc(1, sizeof(stak_canvas_s));
    bcm_host_init();
    canvas->opengl_resource = vc_dispmanx_resource_create(VC_IMAGE_RGB565, OPENGL_WIDTH, OPENGL_HEIGHT, &canvas->opengl_ptr);
    if (!canvas->opengl_resource)
    {
        syslog(LOG_ERR, "Unable to create OpenGL screen buffer");
        return 0;
    }
#ifndef RENDER_WINDOW_ONSCREEN
    // open offscreen display and 512x512 resource
    canvas->display = vc_dispmanx_display_open_offscreen( canvas->opengl_resource, DISPMANX_NO_ROTATE);
#else
    // open main display
    canvas->display = vc_dispmanx_display_open( 0 );
#endif

    canvas->scaled_resource = vc_dispmanx_resource_create(VC_IMAGE_RGB565, canvas_w, canvas_h, &canvas->scaled_ptr);
    if (!canvas->scaled_resource)
    {
        syslog(LOG_ERR, "Unable to create LCD output buffer");
        return 0;
    }

    canvas->update = vc_dispmanx_update_start( 0 );

    graphics_get_display_size(canvas->display , &canvas->screen_width, &canvas->screen_height);
    canvas->screen_width = OPENGL_WIDTH;
    canvas->screen_height = OPENGL_HEIGHT;

    vc_dispmanx_rect_set( &screen_rect, 0, 0, OPENGL_WIDTH, OPENGL_HEIGHT );
    vc_dispmanx_rect_set( &canvas->opengl_rect, 0, 0, OPENGL_WIDTH << 16, OPENGL_HEIGHT << 16 );
    vc_dispmanx_rect_set( &canvas->scaled_rect, 0, 0, canvas_w, canvas_h );

    VC_DISPMANX_ALPHA_T alpha = { DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 255, 0 };

    canvas->opengl_element = vc_dispmanx_element_add(canvas->update, canvas->display, 1 , &screen_rect,
                             canvas->opengl_resource, &canvas->opengl_rect,
                             DISPMANX_PROTECTION_NONE, &alpha, 0, DISPMANX_NO_ROTATE);

    assert(canvas->opengl_element != 0);

    canvas->nativewindow.element = canvas->opengl_element;
    canvas->nativewindow.width = OPENGL_WIDTH;
    canvas->nativewindow.height = OPENGL_HEIGHT;
    vc_dispmanx_update_submit_sync( canvas->update );

    EGLint num_config;


    static const EGLint attribute_list[] =
    {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        //EGL_LUMINANCE_SIZE, EGL_DONT_CARE,          //EGL_DONT_CARE
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_SAMPLES,        2,
        EGL_NONE
    };

    EGLConfig config;


    // get an EGL display connection
    canvas->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(canvas->egl_display == EGL_NO_DISPLAY)
        return 0;

    eglBindAPI(EGL_OPENVG_API);

    // initialize the EGL display connection
    if(eglInitialize(canvas->egl_display, NULL, NULL) == EGL_FALSE) {
        printf("Error initializing egl display\n");
        return 0;
    }

    // get an appropriate EGL frame buffer configuration
    if(eglChooseConfig(canvas->egl_display, attribute_list, &config, 1, &num_config) == EGL_FALSE){
        printf("Error setting config\n");
        return 0;
    }

    // create an EGL rendering context
    canvas->context = eglCreateContext(canvas->egl_display, config, EGL_NO_CONTEXT, NULL);
    if(canvas->context == EGL_NO_CONTEXT) {
        printf("Error creating context\n");
        return 0;
    }

    // create an EGL surface
    canvas->surface = eglCreateWindowSurface( canvas->egl_display, config, &canvas->nativewindow, NULL );
    if(canvas->surface == EGL_NO_SURFACE) {
        printf("Error creating surface\n");
        return 0;
    }

    if(eglSurfaceAttrib(canvas->egl_display, canvas->surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED) == EGL_FALSE) {
        printf("Error preserving buffer\n");
        return 0;
    }

    // connect the context to the surface
    eglMakeCurrent(canvas->egl_display, canvas->surface, canvas->surface, canvas->context);
    return canvas;
}

int stak_canvas_destroy(stak_canvas_s* canvas) {
    // clear screen
    glClear( GL_COLOR_BUFFER_BIT );
    eglSwapBuffers(canvas->egl_display, canvas->surface);

    // Release OpenGL resources
    eglMakeCurrent( canvas->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    eglDestroySurface( canvas->egl_display, canvas->surface );
    eglDestroyContext( canvas->egl_display, canvas->context );
    eglTerminate( canvas->egl_display );
    eglReleaseThread();
    vc_dispmanx_element_remove(canvas->update, canvas->opengl_element);
    vc_dispmanx_resource_delete(canvas->opengl_resource);
    vc_dispmanx_resource_delete(canvas->scaled_resource);
    vc_dispmanx_display_close(canvas->display);
    return 0;
}
int stak_canvas_copy(stak_canvas_s* canvas, char* dst, uint32_t pitch) {
    vc_dispmanx_snapshot(canvas->display, canvas->scaled_resource, 0);
    vc_dispmanx_resource_read_data(canvas->scaled_resource, &canvas->scaled_rect, dst, pitch);
    return 0;
}
int stak_canvas_swap(stak_canvas_s* canvas) {
    eglSwapBuffers(canvas->egl_display, canvas->surface);
    return 0;
}
