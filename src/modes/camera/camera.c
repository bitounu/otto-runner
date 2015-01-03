#include <assets/DejaVuSans.inc>                  // font data
#include <assets/DejaVuSerif.inc>
#include <assets/DejaVuSansMono.inc>
#include <lib/libshapes/shapes.h>
#include <application/application.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <pthread.h>
#include <sched.h>

#include <lib/gif-h.h>
#include <omxcam.h>
#include <bcm2835.h>
#include <lib/TinyPngOut/TinyPngOut.h>

static uint16_t* display_buffer = 0;
char rgb_buffer[96*96*3];
GifWriter gif_writer;
//GLuint camera_texture;


const int pin_shutter = 16;
int get_shutter_pressed() {
    // shutter gpio state
    static int shutter_state = 1;

    // if shutter gpio has changed
    if(bcm2835_gpio_lev(pin_shutter) != shutter_state) {
        // change shutter gpio state
        shutter_state ^= 1;

        // return if shutter state is LOW or not
        return (shutter_state == 0);
    }
    // else return false
    return 0;
}

// prototypes
int init();
int shutdown();
int update();

pthread_t thread_camera_update;

void* camera_update_thread(void* arg) {
    
    uint64_t last_time, current_time;
    last_time = current_time = stak_core_get_time();
    int frames_this_second = 0;
    int frames_per_second = 0;
    static uint32_t current = 0;
    static omxcam_video_settings_t settings;
    static omxcam_buffer_t omx_buffer;

    omxcam_video_init (&settings);

    settings.camera.width = 96;
    settings.camera.height = 96;

    //RGB, 640x480
    settings.format = OMXCAM_FORMAT_RGB888;
    omxcam_video_start_npt (&settings);
    
    while(!stak_application_get_is_terminating()) {
        frames_this_second++;
        current_time = stak_core_get_time();

        
        if(current_time > last_time + 1000000) {
            frames_per_second = frames_this_second;
            frames_per_second = frames_per_second;
            frames_this_second = 0;
            last_time = current_time;
            printf("Camera FPS: %i\n", frames_per_second);
        }
        while( current < 4608 * 6) {
            if(omxcam_video_read_npt (&omx_buffer, 0) || omx_buffer.length == 0) {
                omxcam_perror ();
                return 0;
            }

            memcpy(rgb_buffer + current, omx_buffer.data, omx_buffer.length);
            current += omx_buffer.length;
        }
        current = 0;
    }
    omxcam_video_stop_npt ();
    return 0;
}

int init() {
     // set pin type
    bcm2835_gpio_fsel(pin_shutter, BCM2835_GPIO_FSEL_INPT);
    // set pin pull up/down status
    bcm2835_gpio_set_pud(pin_shutter, BCM2835_GPIO_PUD_UP);

    GifBegin( &gif_writer, "output.gif", 96, 96, 20, 8, 0);

    /*glGenTextures(1, &camera_texture);

    glBindTexture(GL_TEXTURE_2D, camera_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, IMAGE_SIZE_WIDTH, IMAGE_SIZE_HEIGHT, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);*/


    /* Create EGL Image 
    eglImage = eglCreateImageKHR(
        state->display,
        state->context,
        EGL_GL_TEXTURE_2D_KHR,
        (EGLClientBuffer)camera_texture,
        0);

    if (eglImage == EGL_NO_IMAGE_KHR)
    {
        printf("eglCreateImageKHR failed.\n");
        exit(1);
    }*/

    pthread_create(&thread_camera_update, NULL, camera_update_thread, NULL);

    //struct sched_param params;
    //params.sched_priority = 1;
    //pthread_setschedparam(camera_update_thread, SCHED_FIFO, &params);
    return 0;
}

int shutdown() {
    if(pthread_join(thread_camera_update, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return -1;
    }
    GifEnd( &gif_writer );
    printf("Terminating!\n");
    return 0;
}


int update() {
    return 0;
}
int rotary_changed(int delta) {
    printf("Rotary changed: %i\n", delta);
    return 0;
}
int update_display(char* buffer, int bpp, int width, int height) {
    display_buffer = (uint16_t*)buffer;
    int y = 0;
    for(;y < 96; y++) {    
        int x = 0;
        for(;x < 96; x++) {
            int y_offset = y * 96 * 3;
            uint16_t src_pixel =    ( (uint16_t) (rgb_buffer[y_offset + x * 3 + 0] & 0xF8) <<  8 ) |
                                    ( (uint16_t) (rgb_buffer[y_offset + x * 3 + 1] & 0xFC) <<  3 ) |
                                    ( (uint16_t) (rgb_buffer[y_offset + x * 3 + 2] & 0xF8) >>  3 );
            display_buffer[y * 96 + x] = src_pixel;
        }
    }

    if( get_shutter_pressed() ) {
        GifWriteFrame( &gif_writer, rgb_buffer, 96, 96, 20, 8, 0 );
        printf("Wrote frame!\n");
    }
    
    return 0;
}

