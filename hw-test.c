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

// helper macros
#define ZERO_OBJECT(_object) memset( &_object, 0, sizeof( _object ) )
#define ARRAY_COUNT(_array) ( sizeof(_array) / sizeof(_array[0]) )

// global state
static stak_canvas_s canvas;
static framebuffer_device_s fb;
static stak_seps114a_s lcd_device;
static stak_bq27510_device_s gauge_device;
static volatile sig_atomic_t terminate = 0;

// gpio settings and state
const int pin_power = 4;
const int pin_shutter = 16;
const int pin_pwm = 18;				// 18

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
	"assets/shutter.png",
	"assets/camera.png",
	"assets/rotleft.png",
	"assets/rotright.png",
	"assets/rotswitch.png"
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

		uint8_t falling = bcm2835_gpio_lev( pin_power );
		if(!falling) {
			printf("low\n");
		}
		//redraw();

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
	bcm2835_gpio_fsel(pin_pwm, BCM2835_GPIO_FSEL_ALT5);
	bcm2835_gpio_fsel(pin_shutter, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(pin_power, BCM2835_GPIO_FSEL_INPT);
    //bcm2835_gpio_set_pud(pin_power, BCM2835_GPIO_PUD_UP);
	bcm2835_gpio_fen(pin_shutter);
	//bcm2835_gpio_set_pud(pin_shutter, BCM2835_GPIO_PUD_UP);
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

	int gauge_error = stak_bq27510_open("/dev/i2c-1", &gauge_device);
    stak_bq27510_close(&gauge_device);

    if(gauge_error != 0) {
    	int x, y;
    	for(y = 0; y < 96; y++) {
			for(x = 0; x < 96; x++) {
				((uint16_t*)lcd_device.framebuffer)[y * 96 + x] = rgb24_to_16(0xff000000);
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

}

void shutdown() {
	bcm2835_pwm_set_data(0, 0);
	stak_canvas_destroy(&canvas);
	stak_seps114a_close(&lcd_device);
	int cur_image = 0;
	for(cur_image = 0; cur_image < ARRAY_COUNT(files); cur_image++) {
		free(images[cur_image].data);
	}
}

void draw_image(int image) {
	int x, y;
	if(images[image].data == 0)
		return;
	for(y = 0; y < 96; y++) {
		for(x = 0; x < 96; x++) {
			((uint16_t*)lcd_device.framebuffer)[y * 96 + x] = rgb24_to_16(( (uint32_t*) images[image].data)[y * images[image].w + x]);
		}
	}
}
void redraw() {
	// start with a clear screen
	glClear( GL_COLOR_BUFFER_BIT );

	uint8_t falling = bcm2835_gpio_lev( pin_power );

	//render(canvas.screen_width,canvas.screen_height);
	
	draw_image( falling );
	//run_test();
	//stak_canvas_swap(&canvas);
	//stak_canvas_copy(&canvas, (char*)lcd_device.framebuffer, 96 * 2);
	stak_seps114a_update(&lcd_device);
}
