#include <bcm2835.h>

#include <assets/DejaVuSans.inc>                  // font data
#include <assets/DejaVuSerif.inc>
#include <assets/DejaVuSansMono.inc>
#include <lib/libshapes/shapes.h>
#include <modes/ces/otto_gui.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>


int (*update_call)();

int menu_state_OTTO( );
int menu_state_mode_gif( );

const int pin_pwm = 18;
const int pin_shutter = 16;
float rotation = -90;

volatile int is_processing_gif = 0;
pthread_t pthr_process_gif;


struct otto_gui_card* card;
#define BASE_DIRECTORY "/home/pi"
#define FASTCAMD_DIR BASE_DIRECTORY "/otto-sdk/fastcmd/"
#define GIF_TEMP_DIR BASE_DIRECTORY "/gif_temp/"
#define OUTPUT_DIR BASE_DIRECTORY "/output/"

int frame_count = 0;

int beep() {
    bcm2835_pwm_set_data(0, 256);
    nanosleep((struct timespec[]){{0, 10000000L}}, NULL);
    bcm2835_pwm_set_data(0, 0);
}
//
//
//
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

//
// init
//
int init() {

    system("mkdir -p " GIF_TEMP_DIR);
    system("mkdir -p " OUTPUT_DIR);
    system( FASTCAMD_DIR "start_camd.sh");
    frame_count = 0;
    // set up screen ratio
    glViewport(0, 0, (GLsizei) 96, (GLsizei) 96);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float ratio = (float)96 / (float)96;
    glFrustumf(-ratio, ratio, -1.0f, 1.0f, 1.0f, 10.0f);

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
    bcm2835_gpio_fsel(pin_pwm, BCM2835_GPIO_FSEL_ALT5);
    // set pin pull up/down status
    bcm2835_gpio_set_pud(pin_shutter, BCM2835_GPIO_PUD_UP);

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
    if( pthread_join( pthr_process_gif, NULL ) ) {
        fprintf(stderr, "Error joining gif process thread\n");
        return -1;
    }
    system( FASTCAMD_DIR "stop_camd.sh");
    system("rm -Rf " GIF_TEMP_DIR);
    return 0;
}

int get_next_file_number() {
    DIR *dirp;
    struct dirent *dp;
    int highest_number = 0;

    dirp = opendir( OUTPUT_DIR );
    while ((dp = readdir(dirp)) != NULL) {
        char num_buffer[5];
        char * pos = strstr ( dp->d_name, ".gif" );
        int offset = (int) ( pos - dp->d_name );
        int len = strlen( dp->d_name );
        if ( ( pos ) &&
             ( pos > dp->d_name + 4) &&
             ( offset == ( len - 4 ) ) ) {
            strncpy( num_buffer, pos - 4, 4 );
            int number = atoi( num_buffer ) + 1;
            if( number > highest_number ) highest_number = number;
        }
    }
    closedir(dirp);
    return highest_number;
}

//
//
//
int update() {
    //Start(96, 96);
    VGfloat color[4] = { 0, 0, 0, 1 };
    vgSetfv( VG_CLEAR_COLOR, 4, color );
    vgClear( 0, 0, 96, 96 );
    vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
    vgLoadIdentity( );
    Translate( 48, 48 );
    rotation -= 0.25f;
    if( rotation <= 0.0f ) rotation += 360.0f;
    Rotate(rotation);

#if 0
    vgScale(2, 2);
    vgTranslate(160, 60);
    vgRotate(180);
    vgTranslate(-160, -100);
    
    setfill( color );
    setstroke( color );

    StrokeWidth( 0 );
    vgLoadIdentity( );

    Background( 0, 0, 0 );
    Fill( 255,255,255,1 );

    otto_gui_card_begin( card );

    vgLoadIdentity( );

    Translate( 20, 20 );
    Rotate( 45 + rotation );
    TextMid( 0, -48 * 0.6, string_buffer, SansTypeface, 96 / 8 );

    Rotate( 180 + rotation );
    TextMid( 0, -48 * 0.6, string_buffer, SansTypeface, 96 / 8 );

    otto_gui_card_end( card );

    Rotate( 45 );
    Translate( 48, 48 );
    TextMid( 0, -48 * 0.6, string_buffer, SansTypeface, 96 / 8 );
#endif
    update_call();

    return 0;
}




int menu_state_OTTO( ) {
    Fill( 255,255,255,1 );
    TextMid( 0, 0, "Press shutter to", SansTypeface, 8 );
    TextMid( 0, -12, "enter gif mode", SansTypeface, 8 );
    if( get_shutter_pressed() ) {
        beep();
        update_call = menu_state_mode_gif;
    }
}
void* thread_process_gif(void* arg) {
    is_processing_gif = 1;
    static char system_call_string[1024];
    bcm2835_pwm_set_data(0, 256);
    
    int file_number = get_next_file_number();
    sprintf( system_call_string, "gifsicle --colors 256 " GIF_TEMP_DIR "*.gif > " OUTPUT_DIR "gif_%04i.gif ; rm " GIF_TEMP_DIR "* ; chown pi:pi " OUTPUT_DIR "$FNAME", file_number );
    system( system_call_string );
    bcm2835_pwm_set_data(0, 0);
    is_processing_gif = 0;
}
int menu_state_mode_gif_processing( ) {

    VGfloat color[4] = { 0, 255, 0, 1 };
    vgSetfv( VG_CLEAR_COLOR, 4, color );
    vgClear( 0, 0, 96, 96 );

    Fill( 0, 0, 255,1 );
    TextMid( 0, 0, "Processing...", SansTypeface, 8 );

    if( is_processing_gif == 0 )
        update_call = menu_state_OTTO;
}
int menu_state_mode_gif( ) {
    static char string_buffer[256];
    
    Fill( 255,255,255,1 );
    
    if( get_shutter_pressed( ) ) {
        beep();
        update_call = menu_state_OTTO;
    }

    if( frame_count ) {
        sprintf(string_buffer, "Frames %i", frame_count);
        TextMid( 0, 0, string_buffer, SansTypeface, 8 );
    } else {
        TextMid( 0, 0, "Rotate shutter to", SansTypeface, 8 );
        TextMid( 0, -12, "capture frames", SansTypeface, 8 );
    }
    
    int rot = stak_get_rotary_value();
    
    if( ( rot > 0 ) && ( frame_count > 0 ) ) {
        frame_count = 0;
        is_processing_gif = 1;
        update_call = menu_state_mode_gif_processing;
        pthread_create(&pthr_process_gif, NULL, thread_process_gif, NULL);

    } else if ( rot < 0 ) {
        bcm2835_pwm_set_data(0, 512);
        system( FASTCAMD_DIR "do_capture.sh");
        nanosleep((struct timespec[]){{0, 10000000L}}, NULL);
        bcm2835_pwm_set_data(0, 0);
        frame_count++;
    }
}