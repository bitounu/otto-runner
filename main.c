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
#include <VG/openvg.h>
#include <VG/vgu.h>
//#define NANOVG_GLES2_IMPLEMENTATION
//#include <nanovg.h>
//#include <nanovg_gl.h>
#include <graphics/fbdev/fbdev.h>
#include <graphics/canvas/canvas.h>
#include <graphics/seps114a/seps114a.h>
#include <io/bq27510/bq27510.h>
#include <lib/shapes.h>

// prototypes
void init();
void shutdown();
void redraw();
void* update_display(void* arg);


// helper macros
#define ZERO_OBJECT(_object) memset( &_object, 0, sizeof( _object ) )
#define ARRAY_COUNT(_array) ( sizeof(_array) / sizeof(_array[0]) )

// global state
static stak_canvas_s canvas;
static framebuffer_device_s fb;
static stak_seps114a_s lcd_device;
static volatile sig_atomic_t terminate = 0;
pthread_t thread_seps114a_update;
//static NVGcontext* vg = NULL;


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


		redraw();

        delta_time = (get_time() - current_time);
        uint64_t sleep_time = min(33000000L, 33000000L - max(0,delta_time));
        nanosleep((struct timespec[]){{0, sleep_time}}, NULL);
	}
	shutdown();
	return 0;
}
extern STATE_T *state;
void init_shapes_state() {
    memset(state, 0, sizeof(*state));
    state->screen_width = canvas->screen_width;
    state->screen_height = canvas->screen_height;
    state->display = canvas->egl_display;
    state->surface = canvas->surface;
    state->context = canvas->context;
    // set up screen ratio
    glViewport(0, 0, (GLsizei) state->screen_width, (GLsizei) state->screen_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float ratio = (float)state->screen_width / (float)state->screen_height;
    glFrustumf(-ratio, ratio, -1.0f, 1.0f, 1.0f, 10.0f);
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
        terminate = 1;
		return;
	}

	setlogmask(LOG_UPTO(LOG_DEBUG));
	openlog("fbcp", LOG_NDELAY | LOG_PID, LOG_USER);
	syslog(LOG_DEBUG, "Starting fb output");

	// setup sigterm handler
	action.sa_handler = term;
	sigaction(SIGINT, &action, NULL);


	// init seps114a
	stak_seps114a_init(&lcd_device);
	if(stak_canvas_create(&canvas, STAK_CANVAS_OFFSCREEN, 96, 96) == -1) {
		printf("Error creating stak canvas\n");
	}


    /*vg = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
    if (vg == NULL) {
        printf("Could not init nanovg.\n");
        terminate = 1;
        return;
    }*/
    glClearColor(1,1,1,1);

    init_shapes_state();

	pthread_create(&thread_seps114a_update, NULL, update_display, NULL);
}

void shutdown() {
	if(pthread_join(thread_seps114a_update, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return;
	}
    //nvgDeleteGLES2(vg);
	stak_canvas_destroy(&canvas);
	stak_seps114a_close(&lcd_device);
}

void* update_display(void* arg) {

    uint64_t last_time, current_time, start_time, delta_time;
    delta_time = start_time = last_time = current_time = get_time();
    int frames_this_second = 0;
    int frames_per_second = 0;
	
	while(!terminate) {

		frames_this_second++;
        current_time = get_time();

        
        if(current_time > last_time + 1000000) {
            frames_per_second = frames_this_second;
            frames_this_second = 0;
            last_time = current_time;
        }
		stak_seps114a_update(&lcd_device);
        delta_time = (get_time() - current_time);
        uint64_t sleep_time = min(33000000L, 33000000L - max(0,delta_time));
        nanosleep((struct timespec[]){{0, sleep_time}}, NULL);
	}
	return 0;
}

void render(int w, int h)
{
    float clearColor[4] = {1,1,1,1};
    VGfloat dotcolor[4] = {0, 0, 0, 0.3};
    Start(96,96);
    RGBA(.5, 0, 0, 0.3, dotcolor);
    setfill(dotcolor);
    Circle(32, 32, 10);
    //	nvgResetTransform(vg);
    //	nvgFillColor(vg, nvgRGBA(28,30,34,192));
    //	nvgStrokeColor(vg, nvgRGBA(0,0,0,32));
    //	nvgCircle(vg, w*0.5f, h*0.5f, 30.5f);
}

void redraw() {
	// start with a clear screen
	//glClear( GL_COLOR_BUFFER_BIT );

	render(canvas.screen_width,canvas.screen_height);
	stak_canvas_swap(&canvas);
	stak_canvas_copy(&canvas, (char*)lcd_device.framebuffer, 96 * 2);
}
