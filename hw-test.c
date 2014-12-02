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
#include <pthread.h>


#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include <graphics/fbdev/fbdev.h>
#include <graphics/canvas/canvas.h>
#include <graphics/seps114a/seps114a.h>
#include <io/bq27510/bq27510.h>

// prototypes
void init();
void shutdown();
void redraw();
void update_pin_states();
void* update_encoder(void* arg);

int get_shutter_pressed();
int get_shutter_released();
int get_rotary_pressed();
int get_rotary_released();
int get_rotary_position();

void shutter_test();
void rotary_left_test();
void rotary_right_test();
void rotary_switch_test();
void camera_test();
void finished_tests();


// helper macros
#define ZERO_OBJECT(_object) memset( &_object, 0, sizeof( _object ) )
#define ARRAY_COUNT(_array) ( sizeof(_array) / sizeof(_array[0]) )

// global state
static stak_canvas_s canvas;
static framebuffer_device_s fb;
static stak_seps114a_s lcd_device;
static stak_bq27510_device_s gauge_device;
static volatile sig_atomic_t terminate = 0;
pthread_t thread_rotary_poll;

// gpio settings and state
const int pin_shutter = 23;
const int pin_rotary_button = 17;
const int pin_rotary_a = 15;
const int pin_rotary_b = 14;
const int pin_pwm = 18;

int value_shutter = LOW;
int value_rotary_left = LOW;
int value_rotary_right = LOW;
int shutter_state = 1;
int rotary_switch_State = 1;
volatile int last_encoded_value = 0, encoder_value = 0;
int testing_position = 0;


// simple test framework
void run_test();
void next_test();

int current_test = 0;

typedef void ( *test_fptr_t )( void );

test_fptr_t tests[] = {
	shutter_test,
	rotary_left_test,
	rotary_right_test,
	rotary_switch_test,
	camera_test,
	finished_tests
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
		stak_seps114a_update(&lcd_device);
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
    ZERO_OBJECT( gauge_device );
	ZERO_OBJECT( action );

	if(!bcm2835_init()) {
		printf("Failed to init BCM2835 library.\n");
		return;
	}
	bcm2835_gpio_fsel(pin_rotary_button, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(pin_rotary_a, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(pin_rotary_b, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(pin_pwm, BCM2835_GPIO_FSEL_ALT5);
	bcm2835_gpio_fsel(23, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(23, BCM2835_GPIO_PUD_UP);
	bcm2835_gpio_set_pud(pin_rotary_button, BCM2835_GPIO_PUD_UP);
	bcm2835_gpio_set_pud(pin_rotary_a, BCM2835_GPIO_PUD_UP);
	bcm2835_gpio_set_pud(pin_rotary_b, BCM2835_GPIO_PUD_UP);
	bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_16);
	bcm2835_pwm_set_mode(0, 1, 1);
	bcm2835_pwm_set_range(0, 1000);
	bcm2835_pwm_set_data(0, 0);

	setlogmask(LOG_UPTO(LOG_DEBUG));
	openlog("fbcp", LOG_NDELAY | LOG_PID, LOG_USER);
	syslog(LOG_DEBUG, "Starting fb output");

	// setup sigterm handler
	action.sa_handler = term;
	sigaction(SIGINT, &action, NULL);

	// setup bq27510 gas gauge
    stak_bq27510_open("/dev/i2c-1", &gauge_device);
    stak_bq27510_close(&gauge_device);

	// init seps114a
	stak_seps114a_init(&lcd_device);
	stak_canvas_create(&canvas, STAK_CANVAS_OFFSCREEN, 96, 96);

	pthread_create(&thread_rotary_poll, NULL, update_encoder, NULL);
	printf("Please press shutter button.\n");
}
void shutdown() {
	if(pthread_join(thread_rotary_poll, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return;
	}
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
void* update_encoder(void* arg) {
	const int encoding_matrix[4][4] = {
		{ 0,-1, 1, 0},
		{ 1, 0, 0,-1},
		{-1, 0, 0, 1},
		{ 0, 1,-1, 0}
	};

	while(!terminate) {
		int encoded = (bcm2835_gpio_lev(pin_rotary_a) << 1)
					 | bcm2835_gpio_lev(pin_rotary_b);

		int change = encoding_matrix[last_encoded_value][encoded];
		encoder_value += change;
		if(change != 0)
			bcm2835_pwm_set_data(0, (1024/32)*encoder_value);
		last_encoded_value = encoded;
	}
}

void run_test() {
	tests[current_test]();
}
void next_test() {
	if(current_test < ARRAY_COUNT(tests))
		current_test++;
}

void update_pin_states() {

	if( bcm2835_gpio_lev(pin_shutter) != shutter_state ){
		shutter_state = !shutter_state;
	}

	if( bcm2835_gpio_lev(pin_rotary_button) != rotary_switch_State ){
		rotary_switch_State = !rotary_switch_State;
	}
}

int get_shutter_pressed() {
	return (shutter_state == LOW);
}
int get_rotary_pressed() {
	return (rotary_switch_State == HIGH);
}
int get_rotary_position() {
	return encoder_value;
}

void shutter_test() {
	if(get_shutter_pressed()) {
		next_test();
		printf("Please rotate rotary to the left.\n");
		testing_position = encoder_value;
	}
}
void rotary_left_test() {
	if(encoder_value < testing_position - 15) {
		next_test();
		printf("Please rotate rotary to the right.\n");
		testing_position = encoder_value;
	}
}
void rotary_right_test() {
	if(encoder_value > testing_position + 15) {
		next_test();
		printf("Please press Rotary switch.\n");
	}
}
void rotary_switch_test() {
	if(get_rotary_pressed()) {
		next_test();
		printf("Please press shutter when camera input verified on OLED.\n");
	}
}
void camera_test() {
	if(get_shutter_pressed()) {
		next_test();
		printf("Tests succeeded!\n");
	}
}
void finished_tests() {
	terminate = 1;
}