#include <application/application.h>
#include <core/core.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <pthread.h>
#include <sched.h>

#include <bcm2835.h>

#include <lib/DejaVuSans.inc>                  // font data
#include <lib/DejaVuSerif.inc>
#include <lib/DejaVuSansMono.inc>
#include <lib/shapes.h>

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

static volatile sig_atomic_t terminate;

//#define STAK_USE_THREADING

// SIGINT signal handler
void stak_application_terminate_cb(int signum)
{
    printf("Terminating...\n");
    terminate = 1;
}
#ifdef STAK_USE_THREADING
void* stak_application_hal_thread(void* arg) {

    struct stak_application_s* application = (struct stak_application_s*)arg;
    uint64_t last_time, current_time, start_time, delta_time;
    delta_time = start_time = last_time = current_time = stak_core_get_time();
    
    while(!terminate) {
        current_time = stak_core_get_time();

        stak_seps114a_update(application->display);

        delta_time = (stak_core_get_time() - current_time);
        uint64_t sleep_time = min(16000000L, 16000000L - max(0,delta_time));
        nanosleep((struct timespec[]){{0, sleep_time}}, NULL);
    }
    return 0;
}
#endif
struct stak_application_s* stak_application_create() {
	struct stak_application_s* application = calloc(1, sizeof(struct stak_application_s));
	application->state_machine = stak_state_machine_create( 32 );
    application->display = stak_seps114a_create();
    application->canvas = stak_canvas_create(STAK_CANVAS_OFFSCREEN, 96, 96);

    if(!bcm2835_init()) {
        printf("Failed to init BCM2835 library.\n");
        stak_application_terminate();
        return 0;
    }
    #ifdef STAK_USE_THREADING
    pthread_create(&application->thread_hal_update, NULL, stak_application_hal_thread, application);

    struct sched_param params;
    params.sched_priority = 1;
    pthread_setschedparam(application->thread_hal_update, SCHED_FIFO, &params);
    #endif

    // set up screen ratio
    glViewport(0, 0, (GLsizei) 96, (GLsizei) 96);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float ratio = (float)96 / (float)96;
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

    return application;
}
int stak_application_destroy(struct stak_application_s* application) {
    #ifdef STAK_USE_THREADING
    if(pthread_join(application->thread_hal_update, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return -1;
    }
    #endif

	stak_state_machine_destroy(application->state_machine);
    stak_canvas_destroy(application->canvas);
    stak_seps114a_destroy(application->display);
	free(application);
    return 0;
}
int stak_application_run(struct stak_application_s* application) {
    struct sigaction action;

	    // setup sigterm handler
    action.sa_handler = stak_application_terminate_cb;
    sigaction(SIGINT, &action, NULL);

    uint64_t last_time, current_time, start_time, delta_time;
    delta_time = start_time = last_time = current_time = stak_core_get_time();
    int frames_this_second = 0;
    int frames_per_second = 0;
	while(!terminate) {
		//stak_state_machine_top(application->state_machine, current_state);
		frames_this_second++;
        current_time = stak_core_get_time();

        
        if(current_time > last_time + 1000000) {
            frames_per_second = frames_this_second;
            frames_this_second = 0;
            last_time = current_time;
            printf("FPS: %i\n", frames_per_second);
        }


        stak_state_machine_run(application->state_machine);
        stak_canvas_swap(application->canvas);
        stak_canvas_copy(application->canvas, (char*)application->display->framebuffer, 96 * 2);
        #ifndef STAK_USE_THREADING
        stak_seps114a_update(application->display);
        #endif

        delta_time = (stak_core_get_time() - current_time);
        uint64_t sleep_time = min(16000000L, 16000000L - max(0,delta_time));
        nanosleep((struct timespec[]){{0, sleep_time}}, NULL);
	}
    return 0;
}

int stak_application_terminate() {
	terminate = 1;
    return 0;
}