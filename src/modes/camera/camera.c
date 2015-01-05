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
#include <stak.h>

#include <pthread.h>
#include <sched.h>

#include <lib/gif-h.h>
#include <lib/camera/rpi_camera.h>
#include <bcm2835.h>
#include <lib/TinyPngOut/TinyPngOut.h>


const int camera_width = 640;
const int camera_height = 480;

uint32_t* rgb_buffer, *gif_buffer;
uint32_t* rgb_oled_buffer;
GifWriter gif_writer;
VGImage camera_image = 0;


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

    struct rpi_camera_settings settings = {
        .width = camera_width,
        .height = camera_height,
        .frame_rate = 60,
        .num_levels = 1,
        .do_argb_conversion = 1
    };

    struct rpi_camera* camera;
    camera = calloc( 1, sizeof( struct rpi_camera ) );
    rpi_camera_create( camera, settings );
    if ( !camera )
        stak_application_terminate();

    while( !stak_application_get_is_terminating() ) {
        frames_this_second++;
        current_time = stak_core_get_time();
        
        if(current_time > last_time + 1000000) {
            frames_per_second = frames_this_second;
            frames_per_second = frames_per_second;
            frames_this_second = 0;
            last_time = current_time;
            stak_log("Camera FPS: %i", frames_per_second);
        }
        int total_size = camera_width * camera_height;
        if( camera )
            if( !rpi_camera_read_frame( camera, 0 ,rgb_buffer, total_size * 4) )
                printf("yay!\n");
                //stak_application_terminate();
        /*while( current < total_size ) {
            if(omxcam_video_read_npt (&omx_buffer, 0) || omx_buffer.length == 0) {
                omxcam_perror ();
                return 0;
            }

            memcpy(rgb_buffer + current, omx_buffer.data, omx_buffer.length);
            current += omx_buffer.length/4;
        }*/
        //vgImageSubData(camera_image, rgb_buffer, camera_width*4, VG_sRGBA_8888, 0, 0, camera_width, camera_height);
        current = 0;
    }
    rpi_camera_destroy( camera );
    return 0;
}

int init() {
     // set pin type
    bcm2835_gpio_fsel(pin_shutter, BCM2835_GPIO_FSEL_INPT);
    // set pin pull up/down status
    bcm2835_gpio_set_pud(pin_shutter, BCM2835_GPIO_PUD_UP);

    GifBegin( &gif_writer, "output.gif", camera_width, camera_height, 20, 8, 0);

    rgb_buffer = calloc( 1, camera_width * camera_height * 4 );
    gif_buffer = calloc( 1, camera_width * camera_height * 4 );

    //camera_image = vgCreateImage(VG_sRGBA_8888, camera_width, camera_height, VG_IMAGE_QUALITY_NONANTIALIASED);

    pthread_create(&thread_camera_update, NULL, camera_update_thread, NULL);

    struct sched_param params;
    params.sched_priority = 1;
    pthread_setschedparam(thread_camera_update, SCHED_FIFO, &params);
    return 0;
}

int shutdown() {
    if(pthread_join(thread_camera_update, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return -1;
    }
    GifEnd( &gif_writer );
    free(rgb_buffer);
    free(gif_buffer);
    printf("Terminating!\n");
    return 0;
}


int update() {
    Start(96,96);
    //Fill(255, 255, 255, 1);
    //Circle(0.0f, 0.0f, 32.0f);
    //vgLoadIdentity();
    //vgScale( 0.2f, 0.2f );
    //vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
    //vgDrawImage(camera_image);
    if( get_shutter_pressed() ) {
        printf("Starting memcpy...\n");
        memcpy(gif_buffer, rgb_buffer, camera_width * camera_height * 4);
        printf("Ending memcpy\n");
        GifWriteFrame( &gif_writer, gif_buffer, camera_width, camera_height, 20, 8, 0 );
        printf("Wrote frame!\n");
    }
    return 0;
}
int rotary_changed(int delta) {
    printf("Rotary changed: %i\n", delta);
    return 0;
}
/*int update_display(uint8_t* buffer, int bpp, int width, int height) {
    display_buffer = (uint16_t*)buffer;
    int y = 0;
    for(;y < 96; y++) {    
        int x = 0;
        for(;x < 96; x++) {
            int y_offset = y * 96 * 4;
            uint16_t src_pixel =    ( (uint16_t) (rgb_buffer[y_offset + x * 4 + 0] & 0xF8) <<  8 ) |
                                    ( (uint16_t) (rgb_buffer[y_offset + x * 4 + 1] & 0xFC) <<  3 ) |
                                    ( (uint16_t) (rgb_buffer[y_offset + x * 4 + 2] & 0xF8) >>  3 );
            display_buffer[y * 96 + x] = src_pixel;
        }
    }

    if( get_shutter_pressed() ) {
        GifWriteFrame( &gif_writer, rgb_buffer, 666, 666, 20, 8, 0 );
        printf("Wrote frame!\n");
    }
    
    return 0;
}*/

