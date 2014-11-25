//#include <wiringPi.h>
//#include <softTone.h>

#include <bcm2835.h>

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>


#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include <graphics/fbdev/fbdev.h>
#include <graphics/canvas/canvas.h>
#include <graphics/seps114a/seps114a.h>

// prototypes
void init();
void shutdown();
void redraw();
void update_pin_states();
void shutter_pressed();
void shutter_released();
void rotary_left();
void rotary_right();

// helper macros
#define ZERO_OBJECT(_object) memset( &_object, 0, sizeof( _object ) );


// global state
static stak_canvas_s canvas;
static framebuffer_device_s fb;
static stak_seps114a_s lcd_device;
static volatile sig_atomic_t terminate = 0;

// gpio settings and state
const int pin_shutter = 22;
const int pin_rotary_left = 25;
const int pin_rotary_right = 17;

int value_shutter = LOW;
int value_rotary_left = LOW;
int value_rotary_right = LOW;

int watch_pins[][2] = {
	{14, BCM2835_GPIO_PUD_UP},
	{15, BCM2835_GPIO_PUD_UP},
	{17, BCM2835_GPIO_PUD_UP},
	{23, BCM2835_GPIO_PUD_UP}
};
int watch_states[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};
int encoder_pin_a = 15, encoder_pin_b = 17;
int last_encoded_value = 0, encoder_value;

// SIGINT signal handler
void term(int signum)
{
    printf("Terminating...\n");
    terminate = 1;
}

int main(int argc, char** argv) {
	init();

	while(!terminate) {
		update_pin_states();
		redraw();
		//	stak_seps114a_update(&lcd_device);
	}
	shutdown();
	return 0;
}

void init() {
	struct sigaction action;

	// clear application state
	ZERO_OBJECT( fb );
	ZERO_OBJECT( canvas );
	ZERO_OBJECT( lcd_device );
	ZERO_OBJECT( action );

	if(!bcm2835_init()) {
		printf("Failed to init BCM2835 library.\n");
		return;
	}
	/*int i = 0;
	for(; i < sizeof(watch_pins)/sizeof(watch_pins[0]); i++) {
		bcm2835_gpio_fsel(watch_pins[i][0], BCM2835_GPIO_FSEL_INPT);
		bcm2835_gpio_set_pud(watch_pins[i][0], watch_pins[i][1]);
	}*/
	bcm2835_gpio_fsel(14, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(15, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(17, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(23, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(23, BCM2835_GPIO_PUD_UP);
	bcm2835_gpio_set_pud(15, BCM2835_GPIO_PUD_UP);
	bcm2835_gpio_set_pud(17, BCM2835_GPIO_PUD_UP);

	setlogmask(LOG_UPTO(LOG_DEBUG));
	openlog("fbcp", LOG_NDELAY | LOG_PID, LOG_USER);
	syslog(LOG_DEBUG, "Starting fb output");

	// setup sigterm handler
	action.sa_handler = term;
	sigaction(SIGINT, &action, NULL);

	// init seps114a
	stak_seps114a_init(&lcd_device);
	stak_canvas_create(&canvas, STAK_CANVAS_OFFSCREEN, 96, 96);
}
void shutdown() {
	stak_canvas_destroy(&canvas);
	stak_seps114a_close(&lcd_device);
}
void redraw() {
	// start with a clear screen
	glClear( GL_COLOR_BUFFER_BIT );

	//render(canvas.screen_width,canvas.screen_height);
	stak_canvas_swap(&canvas);
	stak_canvas_copy(&canvas, (char*)lcd_device.framebuffer, 96 * 2);
}
void update_encoder() {
	const int encoding_matrix[4][4] = {
		{ 0,-1, 1, 0},
		{ 1, 0, 0,-1},
		{-1, 0, 0, 1},
		{ 0, 1,-1, 0}
	};

	int encoded = (bcm2835_gpio_lev(encoder_pin_a) << 1)
				 | bcm2835_gpio_lev(encoder_pin_b);

	int change = encoding_matrix[last_encoded_value][encoded];
	encoder_value += change;
	if(change != 0)
		printf("Encoder value: %i\n", encoder_value);

	last_encoded_value = encoded;
}
void update_pin_states() {
	/*if( digitalRead(pin_shutter) != value_shutter ){
		value_shutter = !value_shutter;
		if(value_shutter == HIGH)
			shutter_pressed();
		else
			shutter_released();
	}*/

	
	update_encoder();
	if( bcm2835_gpio_lev(23) != watch_states[3] ){
		watch_states[3] = !watch_states[3];
		if(watch_states[3] == HIGH)
			printf("Pin Triggered! %i\n", 23);
		if(watch_states[3] == LOW)
			printf("Pin Released! %i\n", 23);
	}
	//bcm2835_gpio_set(23);
	/*int i = 0;
	for(; i < sizeof(watch_pins)/sizeof(watch_pins[0]); i++) {
		if( bcm2835_gpio_lev(watch_pins[i][0]) != watch_states[i] ){
			watch_states[i] = !watch_states[i];
			if(watch_states[i] == HIGH)
				printf("Pin Triggered! %i\n", watch_pins[i][0]);
			if(watch_states[i] == LOW)
				printf("Pin Released! %i\n", watch_pins[i][0]);
		}
	}*/
	/*if( digitalRead(pin_rotary_left) != value_rotary_left ){
		value_rotary_left = !value_rotary_left;
		if(value_rotary_left == HIGH)
			rotary_left();
	}

	if( digitalRead(pin_rotary_right) != value_rotary_right ){
		value_rotary_right = !value_rotary_right;
		if(value_rotary_right == HIGH)
			rotary_right();
	}*/
}
void shutter_pressed() {
	printf("Shutter pressed!\n");
}
void shutter_released() {
	printf("Shutter released!\n");
}
void rotary_left() {
	printf("Rotary turned left!\n");
}
void rotary_right() {
	printf("Rotary turned right!\n");
}