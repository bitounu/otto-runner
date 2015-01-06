#ifndef CES_UTILS_H
#define CES_UTILS_H

#define pin_pwm 18
#define pin_shutter 16
#define pin_rotary_switch 17
#define BASE_DIRECTORY "/home/pi"
#define FASTCAMD_DIR BASE_DIRECTORY "/otto-sdk/fastcmd/"
#define GIF_TEMP_DIR BASE_DIRECTORY "/gif_temp/"
#define OUTPUT_DIR BASE_DIRECTORY "/output/"

int get_next_file_number();
int get_shutter_pressed();
int get_rotary_switch_position();
#endif