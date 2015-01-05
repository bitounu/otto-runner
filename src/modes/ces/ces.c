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

const int pin_pwm = 18;
const int pin_shutter = 16;

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

    system("mkdir -p /home/pi/gif_temp/");
    system("/home/pi/fastcmd/start_camd.sh");
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
    system("/home/pi/fastcmd/stop_camd.sh");
    system("rm -Rf /home/pi/gif_temp/");
    return 0;
}


int rotary_changed(int delta) {
    if(delta < 0) {

        bcm2835_pwm_set_data(0, 256);
        printf("creating image...\n");
        system("gifsicle --colors 256 /home/pi/gif_temp/*.gif > /home/pi/gif.gif ; rm /home/pi/gif_temp/* ; chown pi:pi /home/pi/gif.gif");
        printf("image created!\n");
        bcm2835_pwm_set_data(0, 0);
        frame_count = 0;
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
    Fill(255,255,255,1);
    TextMid(96 / 2, 96 * 0.6, string_buffer, SansTypeface, 96 / 7);
    // if shutter button is pressed
    if( get_shutter_pressed() ) {
        // output
        printf("capturing frame %i", frame_count);
        system("/home/pi/fastcmd/do_capture.sh");
        frame_count++;
    }
    return 0;
}