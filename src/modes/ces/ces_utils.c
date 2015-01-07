#include <bcm2835.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include "ces_utils.h"
#include <time.h>

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

int beep() {
    bcm2835_pwm_set_data(0, 256);
    nanosleep((struct timespec[]){{0, 15000000L}}, NULL);
    bcm2835_pwm_set_data(0, 0);
}
//
//
//
int get_shutter_pressed() {
    // shutter gpio state
    static int shutter_state = 1;

    // if shutter gpio has changed
    if( bcm2835_gpio_lev(pin_shutter) != shutter_state ) {
        // change shutter gpio state
        shutter_state ^= 1;

        // return if shutter state is LOW or not
        return (shutter_state == 0);
    }
    // else return false
    return 0;
}

int get_rotary_switch_position() {
    // return rotary switch state
    return bcm2835_gpio_lev( pin_rotary_switch );
}
