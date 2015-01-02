#include <bcm2835.h>


//#include <graphics/canvas/canvas.h>
//#include <graphics/seps114a/seps114a.h>
//#include <GLES2/gl2.h>
//#include <EGL/egl.h>
//#include <EGL/eglext.h>

//#include "VG/openvg.h"
//#include "VG/vgu.h"

const int pin_shutter = 16;

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
    // init bcm lib
    if(!bcm2835_init()) {
        printf("Failed to init BCM2835 library.\n");
        return;
    }
    
    // set pin type
    bcm2835_gpio_fsel(pin_shutter, BCM2835_GPIO_FSEL_INPT);
    // set pin pull up/down status
    bcm2835_gpio_set_pud(pin_shutter, BCM2835_GPIO_PUD_UP);
    return 0;
}

//
//
//
int shutdown() {
    return 0;
}

//
//
//
int update() {
    // if shutter button is pressed
    if( get_shutter_pressed() ) {
        // output
        printf("click\n");
    }
    return 0;
}