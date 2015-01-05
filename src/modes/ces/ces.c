#include <bcm2835.h>

#include <assets/DejaVuSans.inc>                  // font data
#include <assets/DejaVuSerif.inc>
#include <assets/DejaVuSansMono.inc>
#include <lib/libshapes/shapes.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>

const int pin_pwm = 18;
const int pin_shutter = 16;
#define BASE_DIRECTORY "/home/pi"
#define FASTCAMD_DIR BASE_DIRECTORY "/otto-sdk/fastcmd/"
#define GIF_TEMP_DIR BASE_DIRECTORY "/gif_temp/"
#define OUTPUT_DIR BASE_DIRECTORY "/output/"

int frame_count = 0;
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

    return 0;
}

//
//
//
int shutdown() {
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

int rotary_changed(int delta) {
    static char system_string[1024];
    if( (delta > 0) && (frame_count > 0) ) {

        bcm2835_pwm_set_data(0, 256);
        printf("creating image...\n");
        
        int file_number = get_next_file_number();
        sprintf( system_string, "gifsicle --colors 256 " GIF_TEMP_DIR "*.gif > " OUTPUT_DIR "gif_%04i.gif ; rm " GIF_TEMP_DIR "* ; chown pi:pi " OUTPUT_DIR "$FNAME", file_number );
        system( system_string );
        bcm2835_pwm_set_data(0, 0);
        frame_count = 0;
    }
    else if ( delta < 0 ) {
        printf("capturing frame %i", frame_count);
        bcm2835_pwm_set_data(0, 512);
        system( FASTCAMD_DIR "do_capture.sh");
        nanosleep((struct timespec[]){{0, 10000000L}}, NULL);
        bcm2835_pwm_set_data(0, 0);
        frame_count++;
    }
    return 0;
}

//
//
//
int update() {
    static char string_buffer[256];
    sprintf(string_buffer, "Frames %i", frame_count);
    
    

    //Start(96, 96);
    VGfloat color[4] = { 0, 0, 0, 1 };
    vgSetfv(VG_CLEAR_COLOR, 4, color);
    vgClear(0, 0, 96, 96);
    setfill(color);
    setstroke(color);
    StrokeWidth(0);
    vgLoadIdentity();
    Background(0, 0, 0);
    Fill(255,255,255,1);
    Translate(48, 48);
    TextMid(0, -48 * 0.6, string_buffer, SansTypeface, 96 / 8);
    Rotate(180);
    TextMid(0, -48 * 0.6, string_buffer, SansTypeface, 96 / 8);
    return 0;
}