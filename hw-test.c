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
void* update_display(void* arg);

int get_shutter_pressed();
int get_shutter_released();
int get_rotary_pressed();
int get_rotary_released();
int get_rotary_position();

void shutter_test();
void rotary_left_test();
void rotary_right_test();
void rotary_switch_test();
void camera_test_a();
void camera_test_b();
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
pthread_t thread_rotary_poll, thread_seps114a_update;

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
	camera_test_a,
	camera_test_b,
	finished_tests
};

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

uint64_t get_time() {
    struct timespec timer;
    clock_gettime(CLOCK_MONOTONIC, &timer);
    return (uint64_t) (timer.tv_sec) * 1000000L + timer.tv_nsec / 1000L;
}

typedef struct framerate_lock {
    uint64_t last_time, current_time, start_time, delta_time;
    int frames_this_second;
    int frames_per_second;
};


const char* files[] = {
	"/root/assets/shutter.png",
	"/root/assets/rotleft.png",
	"/root/assets/rotright.png",
	"/root/assets/rotswitch.png",
	"/root/assets/camera.png",
	"/root/assets/oled.png",
	//"assets/complete.png"
};

typedef struct image_s{
	unsigned char* data;
	int w;
	int h;
};
static struct image_s images[ARRAY_COUNT(files)];


inline uint16_t rgb24_to_16 (uint32_t rgb)
{
    int blue   = (rgb >>  3) & 0x001f;
    int green  = (rgb >> 10) & 0x003f;
    int red    = (rgb >> 19) & 0x001f;
    return (red ) | (green << 5) | ( blue << 11);
}

// SIGINT signal handler
void term(int signum)
{
    printf("Terminating...\n");
    terminate = 1;
}

int main(int argc, char** argv) {
    uint64_t last_time, current_time, start_time, delta_time;
    delta_time = start_time = last_time = current_time = get_time();
    int frames_this_second = 0;
    int frames_per_second = 0;
	init();

	while(!terminate) {

		frames_this_second++;
        current_time = get_time();

        
        if(current_time > last_time + 1000000) {
            frames_per_second = frames_this_second;
            frames_this_second = 0;
            last_time = current_time;
        }


		update_pin_states();
		redraw();

        delta_time = (get_time() - current_time);
        uint64_t sleep_time = min(33000000L, 33000000L - max(0,delta_time));
        nanosleep((struct timespec[]){{0, sleep_time}}, NULL);
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


	// init seps114a
	stak_seps114a_init(&lcd_device);
	stak_canvas_create(&canvas, STAK_CANVAS_OFFSCREEN, 96, 96);

	// setup bq27510 gas gauge
    int gauge_error = stak_bq27510_open("/dev/i2c-1", &gauge_device);
    stak_bq27510_close(&gauge_device);

    if(gauge_error != 0) {
    	int x, y;
    	for(y = 0; y < 96; y++) {
			for(x = 0; x < 96; x++) {
				((uint16_t*)lcd_device.framebuffer)[y * 96 + x] = rgb24_to_16(0x00ff0000);
			}
		}
		stak_seps114a_update(&lcd_device);
		nanosleep((struct timespec[]){{5, 0}}, NULL);
		terminate = 1;
    }
    else {
    	int cur_image = 0;
		for(cur_image = 0; cur_image < ARRAY_COUNT(files); cur_image++) {
			int error = lodepng_decode32_file(&images[cur_image].data, &images[cur_image].w, &images[cur_image].h, files[cur_image]);
			if(error != 0) {
				printf("Failed to load file: %s\n", files[cur_image]);
				terminate = 1;
			}
		}
    }
	pthread_create(&thread_rotary_poll, NULL, update_encoder, NULL);
	//pthread_create(&thread_seps114a_update, NULL, update_display, NULL);
	
	printf("Please press shutter button.\n");
}

void shutdown() {
	if(pthread_join(thread_rotary_poll, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return;
	}
	/*if(pthread_join(thread_seps114a_update, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return;
	}*/
	stak_canvas_destroy(&canvas);
	stak_seps114a_close(&lcd_device);
	int cur_image = 0;
	for(cur_image = 0; cur_image < ARRAY_COUNT(files); cur_image++) {
		free(images[cur_image].data);
	}
}

void* update_display(void* arg) {

    uint64_t last_time, current_time, start_time, delta_time;
    delta_time = start_time = last_time = current_time = get_time();
    int frames_this_second = 0;
    int frames_per_second = 0;
	
	while(!terminate) {

		/*frames_this_second++;
        current_time = get_time();

        
        if(current_time > last_time + 1000000) {
            frames_per_second = frames_this_second;
            frames_this_second = 0;
            last_time = current_time;
        }
		stak_seps114a_update(&lcd_device);
        delta_time = (get_time() - current_time);
        uint64_t sleep_time = min(16000000L, 16000000L - max(0,delta_time));
        nanosleep((struct timespec[]){{0, sleep_time}}, NULL);*/
        nanosleep((struct timespec[]){{0, 333000000L}}, NULL);
	}
	return 0;
}

void draw_image(int image) {
	int x, y;
	for(y = 0; y < 96; y++) {
		for(x = 0; x < 96; x++) {
			((uint16_t*)lcd_device.framebuffer)[y * 96 + x] = rgb24_to_16(( (uint32_t*) images[image].data)[y * images[image].w + x]);
		}
	}
}
void redraw() {
	// start with a clear screen
	glClear( GL_COLOR_BUFFER_BIT );

	//render(canvas.screen_width,canvas.screen_height);
	run_test();
	//stak_canvas_swap(&canvas);
	//stak_canvas_copy(&canvas, (char*)lcd_device.framebuffer, 96 * 2);
	stak_seps114a_update(&lcd_device);
}

void* update_encoder(void* arg) {
	uint64_t last_time, current_time, start_time, delta_time;
    delta_time = start_time = last_time = current_time = get_time();
    int frames_this_second = 0;
    int frames_per_second = 0;
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
		//if(change != 0)
		//	bcm2835_pwm_set_data(0, (1024/32)*encoder_value);
		last_encoded_value = encoded;

		frames_this_second++;
        current_time = get_time();

        
        if(current_time > last_time + 1000000) {
            frames_per_second = frames_this_second;
            frames_this_second = 0;
            last_time = current_time;
            //printf("SEPS114A FPS: %i\n", frames_per_second);
        }
        delta_time = (get_time() - current_time);
        uint64_t sleep_time = min(16000000L, 16000000L - max(0,delta_time));
        nanosleep((struct timespec[]){{0, sleep_time}}, NULL);
	}
	return 0;
}

void run_test() {
	tests[current_test]();
}

void next_test() {
	bcm2835_pwm_set_data(0, (1024/32)*12);
	nanosleep((struct timespec[]){{0, 500000000L}}, NULL);
	bcm2835_pwm_set_data(0, 0);
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
	draw_image(0);
	if(get_shutter_pressed()) {
		next_test();
		testing_position = encoder_value;
	}
}
void rotary_left_test() {
	draw_image(1);
	if(encoder_value < testing_position - 15) {
		next_test();
		testing_position = encoder_value;
	}
}
void rotary_right_test() {
	draw_image(2);
	if(encoder_value > testing_position + 15) {
		next_test();
	}
}
void rotary_switch_test() {
	draw_image(3);
	if(get_rotary_pressed()) {
		next_test();
	}
}
void camera_test_a() {
	draw_image(4);
	stak_seps114a_update(&lcd_device);
	nanosleep((struct timespec[]){{3, 0}}, NULL);
	next_test();
}
void camera_test_b() {
	draw_image(5);
	if(get_shutter_pressed()) {
		next_test();
	}
}
void finished_tests() {
	terminate = 1;
}