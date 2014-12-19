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
#include <sched.h>


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

#include "lib/DejaVuSans.inc"                  // font data
#include "lib/DejaVuSerif.inc"
#include "lib/DejaVuSansMono.inc"

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


#define NUM_PARTICLES 50

typedef struct particle {
    double x, y;
    double vx, vy;
    int r, g, b;
    int radius;
} particle_t;

particle_t particles[NUM_PARTICLES];

int showTrails = 1;
int directionRTL = 0;
int alternate = 1;
double gravity = 0.05;

// Initialize _all_ the particles
void initParticles(int w, int h) {
    int i;
    for (i = 0; i < NUM_PARTICLES; i++) {
        particle_t *p = &particles[i];

        p->x = 48.0;
        p->y = 48.0;
        p->vx = (rand() % 6) - 3;
        p->vy = (rand() % 6) - 3;
        p->r = rand() % 256;
        p->g = rand() % 256;
        p->b = rand() % 256;
        p->radius = (rand() % 5) + 5;

        if (directionRTL) {
            p->vx *= -1.0;
            p->x = w;
        }
    }
}

void paintBG(int w, int h) {
    //if (!showTrails)
    //    return Background(0, 0, 0);

    Fill(0, 0, 0, 1);
    Rect(0, 0, w, h);
}

void draw(int w, int h) {
    int i;
    particle_t *p;

    paintBG(w, h);

    for (i = 0; i < NUM_PARTICLES; i++) {
        p = &particles[i];

        Fill(p->r, p->g, p->b, 1.0);
        Circle(p->x, p->y, p->radius);

        // Apply the velocity
        p->x += p->vx;
        p->y += p->vy;

        p->vx *= 0.98;
        if (p->vy > 0.0)
            p->vy *= 0.97;

        // Gravity
        p->vy -= gravity;

        // Stop particles leaving the canvas  
        if (p->x < -4.0)
            p->x = w + 4.0;
        if (p->x > w + 4.0)
            p->x = -4.0;

        // When particle reaches the bottom of screen reset velocity & start posn
        if (p->y < -4.0) {
            p->x = 0.0;
            p->y = 0.0;
            p->vx = (rand() % 6) - 3;
            p->vy = (rand() % 10);

            if (directionRTL) {
                p->vx *= -1.0;
                p->x = w;
            }
        }

        if (p->y > h + 4.0)
            p->y = -4.0;
    }

   //End();
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
        //stak_seps114a_update(&lcd_device);

        delta_time = (get_time() - current_time);
        uint64_t sleep_time = min(33000000L, 33000000L - max(0,delta_time));
        nanosleep((struct timespec[]){{0, sleep_time}}, NULL);
	}
	shutdown();
	return 0;
}
void init_shapes_state() {
    // set up screen ratio
    glViewport(0, 0, (GLsizei) canvas.screen_width, (GLsizei) canvas.screen_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float ratio = (float)canvas.screen_width / (float)canvas.screen_height;
    glFrustumf(-ratio, ratio, -1.0f, 1.0f, 1.0f, 10.0f);

    SansTypeface = loadfont(DejaVuSans_glyphPoints,
                DejaVuSans_glyphPointIndices,
                DejaVuSans_glyphInstructions,
                DejaVuSans_glyphInstructionIndices,
                DejaVuSans_glyphInstructionCounts,
                DejaVuSans_glyphAdvances, DejaVuSans_characterMap, DejaVuSans_glyphCount);

    SerifTypeface = loadfont(DejaVuSerif_glyphPoints,
                 DejaVuSerif_glyphPointIndices,
                 DejaVuSerif_glyphInstructions,
                 DejaVuSerif_glyphInstructionIndices,
                 DejaVuSerif_glyphInstructionCounts,
                 DejaVuSerif_glyphAdvances, DejaVuSerif_characterMap, DejaVuSerif_glyphCount);

    MonoTypeface = loadfont(DejaVuSansMono_glyphPoints,
                DejaVuSansMono_glyphPointIndices,
                DejaVuSansMono_glyphInstructions,
                DejaVuSansMono_glyphInstructionIndices,
                DejaVuSansMono_glyphInstructionCounts,
                DejaVuSansMono_glyphAdvances, DejaVuSansMono_characterMap, DejaVuSansMono_glyphCount);

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

    glClearColor(0,0,0,1);

    init_shapes_state();
    initParticles(96, 96);

	pthread_create(&thread_seps114a_update, NULL, update_display, NULL);

    struct sched_param params;
    params.sched_priority = 1;//sched_get_priority_max(SCHED_FIFO);
    pthread_setschedparam(thread_seps114a_update, SCHED_FIFO, &params);
}

void shutdown() {
	if(pthread_join(thread_seps114a_update, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return;
	}
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
    Start(96,96);
    draw(96,96);
    Fill(255, 255, 255, 0.75);
    TextMid(96 / 2, 96 * 0.6, "OT", MonoTypeface, 96 / 8);
    TextMid(96 / 2, 96 * 0.8, "TO", MonoTypeface, 96 / 8);
}

void redraw() {

	render(canvas.screen_width,canvas.screen_height);
	stak_canvas_swap(&canvas);
	stak_canvas_copy(&canvas, (char*)lcd_device.framebuffer, 96 * 2);
}
