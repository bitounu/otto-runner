#include <wiringPi.h>
#include <softTone.h>

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

// global state
static stak_canvas_s canvas;
static framebuffer_device_s fb;
static stak_seps114a_s lcd_device;
static volatile sig_atomic_t terminate = 0;

// gpio settings and state
const int pin_shutter = 0;
const int pin_rotary_left = 0;
const int pin_rotary_right = 0;

int value_shutter = LOW;
int value_rotary_left = LOW;
int value_rotary_right = LOW;

#define ZERO_OBJECT(_object) memset( &_object, 0, sizeof( _object ) );

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
}

void init() {
	wiringPiSetup();
	pinMode(pin_shutter, INPUT);
	pinMode(pin_rotary_left, INPUT);
	pinMode(pin_rotary_right, INPUT);

	setlogmask(LOG_UPTO(LOG_DEBUG));
	openlog("fbcp", LOG_NDELAY | LOG_PID, LOG_USER);
	syslog(LOG_DEBUG, "Starting fb output");

	// setup sigterm handler
	struct sigaction action;
	ZERO_OBJECT( action );
	action.sa_handler = term;
	sigaction(SIGINT, &action, NULL);

	// clear application state
	ZERO_OBJECT( fb );
	ZERO_OBJECT( canvas );
	ZERO_OBJECT( lcd_device );

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
void update_pin_states() {
	if( digitalRead(pin_shutter) != value_shutter ){
		value_shutter = !value_shutter;
		if(value_shutter == HIGH)
			shutter_pressed();
		else
			shutter_released();
	}

	if( digitalRead(pin_rotary_left) != value_rotary_left ){
		value_rotary_left = !value_rotary_left;
		if(value_rotary_left == HIGH)
			rotary_left();
	}

	if( digitalRead(pin_rotary_right) != value_rotary_right ){
		value_rotary_right = !value_rotary_right;
		if(value_rotary_right == HIGH)
			rotary_right();
	}
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
