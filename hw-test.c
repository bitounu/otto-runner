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

int get_shutter_pressed();
int get_shutter_released();
int get_rotary_pressed();
int get_rotary_released();
int get_rotary_position();

void shutter_test();
void rotary_left_test();
void rotary_right_test();


// helper macros
#define ZERO_OBJECT(_object) memset( &_object, 0, sizeof( _object ) )
#define ARRAY_COUNT(_array) (sizeof(_array) / sizeof(_array[0]))

// global state
static stak_canvas_s canvas;
static framebuffer_device_s fb;
static stak_seps114a_s lcd_device;
static volatile sig_atomic_t terminate = 0;

// gpio settings and state
const int pin_shutter = 23;
const int pin_rotary_button = 14;
const int pin_rotary_a = 15;
const int pin_rotary_b = 17;

int value_shutter = LOW;
int value_rotary_left = LOW;
int value_rotary_right = LOW;
int shutter_state = 1;
int rotary_switch_State = 0;
int last_encoded_value = 0, encoder_value;


// simple test framework
void run_test();
void next_test();

int current_test = 0;

typedef void ( *test_fptr_t )( void );

test_fptr_t tests[] = {
	shutter_test
};


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
	bcm2835_gpio_fsel(pin_rotary_button, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(pin_rotary_a, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(pin_rotary_b, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(23, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(23, BCM2835_GPIO_PUD_UP);
	bcm2835_gpio_set_pud(pin_rotary_button, BCM2835_GPIO_PUD_UP);
	bcm2835_gpio_set_pud(pin_rotary_a, BCM2835_GPIO_PUD_UP);
	bcm2835_gpio_set_pud(pin_rotary_b, BCM2835_GPIO_PUD_UP);

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
	run_test();
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

	int encoded = (bcm2835_gpio_lev(pin_rotary_a) << 1)
				 | bcm2835_gpio_lev(pin_rotary_b);

	int change = encoding_matrix[last_encoded_value][encoded];
	encoder_value += change;
	if(change != 0) {
		printf("Encoder value: %i\n", encoder_value);
	}

	last_encoded_value = encoded;
}

void run_test() {
	tests[current_test]();
}
void next_test() {
	if(current_test < ARRAY_COUNT(tests))
		current_test++;
}

void update_pin_states() {
	update_encoder();

	if( bcm2835_gpio_lev(pin_shutter) != shutter_state ){
		shutter_state = !shutter_state;
	}

	if( bcm2835_gpio_lev(pin_rotary_button) != rotary_switch_State ){
		rotary_switch_State = !rotary_switch_State;
	}
}

int get_shutter_pressed() {
	return (shutter_state == HIGH);
}
int get_rotary_pressed() {
	return (rotary_switch_State == HIGH);
}
int get_rotary_position() {
	return encoder_value;
}

void shutter_test() {
	printf("Please press the shutter switch.\n");
	if(get_shutter_pressed()) {
		next_test();
	}
}