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

int (*update_call)();

int menu_state_OTTO( );
int menu_state_mode_gif( );


int rotary_count_amount = 0;
int time_since_rotation_started = 0;

float rotation = -90;


volatile int is_processing_gif = 0;
pthread_t pthr_process_gif;


struct otto_gui_card* card;

int frame_count = 0;


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