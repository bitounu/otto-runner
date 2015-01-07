#include <bcm2835.h>

#include <assets/DejaVuSans.inc>                  // font data
#include <assets/DejaVuSerif.inc>
#include <assets/DejaVuSansMono.inc>
#include <lib/libshapes/shapes.h>
#include <modes/ces/otto_gui.h>
#include <application/application.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include "ces_utils.h"

#include <omxcam.h>
int (*update_call)();

int menu_state_OTTO( );
int menu_state_mode_gif( );

#define video_format VG_sXBGR_8888
VGImage img;
VGubyte *data, *buffer;


int rotary_count_amount = 0;
int time_since_rotation_started = 0;

float rotation = -90;


volatile int is_processing_gif = 0;
volatile int is_camera_on = 1;
pthread_t pthr_process_gif;


struct otto_gui_card* card;

int frame_count = 0;
#if 0
pthread_t thread_camera_update;

void* camera_update_thread(void* arg) {
    
    uint64_t last_time, current_time, delta_time;
    last_time = current_time = stak_core_get_time();
    int frames_this_second = 0;
    int frames_per_second = 0;
    static uint32_t current = 0;
    static omxcam_video_settings_t settings;
    static omxcam_buffer_t omx_buffer;
    int camera_was_on = 0;

    //system( FASTCAMD_DIR "stop_camd.sh");
    omxcam_video_init (&settings);
    data = (VGubyte *) malloc(96*96*4);
    buffer = (VGubyte *) malloc(96*96*4);

    settings.camera.width = 96;
    settings.camera.height = 96;
    settings.camera.rotation = OMXCAM_ROTATION_90;
    settings.camera.mirror = OMXCAM_MIRROR_VERTICAL;
    settings.camera.exposure = OMXCAM_EXPOSURE_AUTO;

    //RGB, 640x480
    settings.format = OMXCAM_FORMAT_RGBA8888;
    omxcam_video_start_npt (&settings);
    
    while( !stak_application_get_is_terminating() ) {
        if ( ( is_camera_on ) && ( !camera_was_on ) ) {
            camera_was_on = 1;
        }

        if( !is_camera_on ) {
            if ( camera_was_on ) {
                omxcam_video_stop_npt ();
                camera_was_on = 0;
                //system( FASTCAMD_DIR "start_camd.sh");
            }
            nanosleep((struct timespec[]){{0, 33000000L}}, NULL);
            continue;
        }
        frames_this_second++;
        current_time = stak_core_get_time();

        
        if(current_time > last_time + 1000000) {
            frames_per_second = frames_this_second;
            frames_per_second = frames_per_second;
            frames_this_second = 0;
            last_time = current_time;
            printf("Camera FPS: %i\n", frames_per_second);
        }
        while( current < 4608 * 8) {
            if(omxcam_video_read_npt (&omx_buffer, 0) || omx_buffer.length == 0) {
                omxcam_perror ();
                return 0;
            }

            memcpy(buffer + current, omx_buffer.data, omx_buffer.length);
            current += omx_buffer.length;
        }
        current = 0;
        delta_time = (stak_core_get_time() - current_time);
        uint64_t sleep_time = min(33000000L, 33000000L - max(0,delta_time));
        nanosleep((struct timespec[]){{0, sleep_time}}, NULL);
    }
    if ( camera_was_on )
        omxcam_video_stop_npt ();
    return 0;
}
#endif 
//
// init
//
int init() {

    // create temp and output directories
    system("mkdir -p " GIF_TEMP_DIR);
    system("mkdir -p " OUTPUT_DIR);

    // start fastcamd daemon
    system( FASTCAMD_DIR "start_camd.sh");
    frame_count = 0;

    // set up screen ratio
    /*glViewport(0, 0, (GLsizei) 96, (GLsizei) 96);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float ratio = (float)96 / (float)96;
    glFrustumf(-ratio, ratio, -1.0f, 1.0f, 1.0f, 10.0f);*/

    SansTypeface = loadfont(DejaVuSans_glyphPoints,
        DejaVuSans_glyphPointIndices,
        DejaVuSans_glyphInstructions,
        DejaVuSans_glyphInstructionIndices,
        DejaVuSans_glyphInstructionCounts,
        DejaVuSans_glyphAdvances, DejaVuSans_characterMap, DejaVuSans_glyphCount);

    SerifTypeface = loadfont(DejaVuSerif_glyphPoints,
        DejaVuSerif_glyphPointIndices,
        DejaVuSerif_glyphInstructions,
        DejaVuSerif_glyphInstructionIndices,
        DejaVuSerif_glyphInstructionCounts,
        DejaVuSerif_glyphAdvances, DejaVuSerif_characterMap, DejaVuSerif_glyphCount);

    MonoTypeface = loadfont(DejaVuSansMono_glyphPoints,
        DejaVuSansMono_glyphPointIndices,
        DejaVuSansMono_glyphInstructions,
        DejaVuSansMono_glyphInstructionIndices,
        DejaVuSansMono_glyphInstructionCounts,
        DejaVuSansMono_glyphAdvances, DejaVuSansMono_characterMap, DejaVuSansMono_glyphCount);
    
    // set pin type
    bcm2835_gpio_fsel(pin_shutter, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(pin_rotary_switch, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(pin_pwm, BCM2835_GPIO_FSEL_ALT5);
    // set pin pull up/down status
    bcm2835_gpio_set_pud(pin_shutter, BCM2835_GPIO_PUD_UP);
    bcm2835_gpio_set_pud(pin_rotary_switch, BCM2835_GPIO_PUD_UP);

    bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_16);
    bcm2835_pwm_set_mode(0, 1, 1);
    bcm2835_pwm_set_range(0, 1000);
    bcm2835_pwm_set_data(0, 0);

    otto_gui_card_init( card );

    update_call = menu_state_OTTO;
#if 0
    pthread_create(&thread_camera_update, NULL, camera_update_thread, NULL);
#endif
    
    img = vgCreateImage(video_format, 96, 96, VG_IMAGE_QUALITY_BETTER);

    return 0;
}

//
//
//
int shutdown() {
    // make sure to wait for processing thread to exit
    // this will output error if the thread isn't running
    if( pthread_join( pthr_process_gif, NULL ) ) {
        fprintf(stderr, "Error joining gif process thread\n");
    }

#if 0
    if( pthread_join( thread_camera_update, NULL ) ) {
        fprintf(stderr, "Error joining gif process thread\n");
    }

#endif 

    // stop fastcamd and delete temp directory
    system( FASTCAMD_DIR "stop_camd.sh");
    system("rm -Rf " GIF_TEMP_DIR);
    return 0;
}


//
//
//
int update() {
    // clear background to black
    VGfloat color[4] = { 0, 0, 0, 1 };
    vgSetfv( VG_CLEAR_COLOR, 4, color );
    vgClear( 0, 0, 96, 96 );

    // reset matrix to identity
    vgLoadIdentity( );
    
    // translate to the center of the screen
    Translate( 48, 48 );

    // calculate text rotation
    rotation -= 0.25f;
    if( rotation <= 0.0f ) rotation += 360.0f;
    Rotate(rotation);

    // call current state
    update_call();

    return 0;
}

#if 0
int menu_state_camera_output( ) {
    is_camera_on = 1;
    memcpy(data, buffer, 96*96*4);
    vgImageSubData(img, data, 96*4, video_format, 0, 0, 96, 96);
    vgLoadIdentity( );
    Rotate(90);
    vgSetPixels(0, 0, img, 0, 0, 96, 96);
    //vgDrawImage(img);
    if( get_shutter_pressed() ) {
        beep();
        //is_camera_on = 0;
        update_call = menu_state_OTTO;
    }
}
#endif

// called when rotary is down
int menu_state_OTTO( ) {
    Fill( 255,255,255,1 );
    TextMid( 0, 0, "Press rotary to", SansTypeface, 8 );
    TextMid( 0, -12, "enter gif mode", SansTypeface, 8 );

    // if rotary is up, beep and switch to gif mode
    if( !get_rotary_switch_position( ) ) {
        beep();
        update_call = menu_state_mode_gif;
    }
#if 0
    if ( get_shutter_pressed() ) {
        beep();
        update_call = thread_process_gif;
    }
#endif
}

// runs gifsicle to process gifs
void* thread_process_gif(void* arg) {
    static char system_call_string[1024];

    is_processing_gif = 1;

    // setup buzzer to make noise while processing
    bcm2835_pwm_set_data(0, 256);
    
    // return the number to use for the next image file
    int file_number = get_next_file_number();

    // create system call string based on calculated file number
    sprintf( system_call_string, "gifsicle --colors 256 " GIF_TEMP_DIR "*.gif > " OUTPUT_DIR "gif_%04i.gif ; rm " GIF_TEMP_DIR "* ; chown pi:pi " OUTPUT_DIR "$FNAME", file_number );

    // run processing call
    system( system_call_string );

    // once finished, turn off buzzer
    bcm2835_pwm_set_data(0, 0);
    is_processing_gif = 0;
}

int menu_state_mode_gif_processing( ) {
    // clear background color to green
    VGfloat color[4] = { 0, 255, 0, 1 };
    vgSetfv( VG_CLEAR_COLOR, 4, color );
    vgClear( 0, 0, 96, 96 );

    // and write "Processing..." on the screen
    Fill( 0, 0, 255,1 );
    TextMid( 0, 0, "Processing...", SansTypeface, 8 );

    if( is_processing_gif == 0 )
        update_call = menu_state_OTTO;
}

int menu_state_mode_gif( ) {
    // string buffer used for writing out frame count
    static char string_buffer[ 256 ];
    
    Fill( 255,255,255,1 );
    
    // if rotary switch is pressed, beep and switch to menu state
    if( get_rotary_switch_position( ) ) {
        beep();
        update_call = menu_state_OTTO;
    }

    // if we have captured any frames, output the count to the screen
    if( frame_count ) {
        sprintf(string_buffer, "Frames %i", frame_count);
        TextMid( 0, 0, string_buffer, SansTypeface, 8 );
    } else {
        TextMid( 0, 0, "Rotate rotary to", SansTypeface, 8 );
        TextMid( 0, -12, "capture frames", SansTypeface, 8 );
    }
    
    int rot = stak_get_rotary_value();
    
    // if rotary is turning left and we have frames
    if( ( rot < 0 ) && ( frame_count ) ) {
        
        // increase rotary count amount
        rotary_count_amount += -rot;

        // if we haven't started a countdown timer 
        if ( time_since_rotation_started == 0 )
            time_since_rotation_started = stak_core_get_time();

        if ( stak_core_get_time() >= time_since_rotation_started + 10000000000L) {
            time_since_rotation_started = 0;
        }else  if( rotary_count_amount > 10 ) {
            // reset gif mode state
            rotary_count_amount = 0;
            frame_count = 0;
            is_processing_gif = 1;
            rotary_count_amount = 0;
            time_since_rotation_started = 0;

            // setup processing state and call processing thread
            update_call = menu_state_mode_gif_processing;
            pthread_create(&pthr_process_gif, NULL, thread_process_gif, NULL);
        }

    // if we're rotating clockwise
    } else if ( rot > 0 ) {
        // capture frame and beep
        system( FASTCAMD_DIR "do_capture.sh");
        beep();
        frame_count++;
    }
}