#include <application/application.h>
#include <core/core.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <errno.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/msg.h>

#include <pthread.h>
#include <sched.h>

#include <bcm2835.h>

#include <lib/DejaVuSans.inc>                  // font data
#include <lib/DejaVuSerif.inc>
#include <lib/DejaVuSansMono.inc>
#include <lib/shapes.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <daemons/input/input.h>

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

static volatile sig_atomic_t terminate;
static void *lib_handle;
int lib_open(struct stak_state_s* app_state) {
    char *error;

    lib_handle = dlopen ("./particles.so", RTLD_LAZY);
    if (!lib_handle) {
        fputs (dlerror(), stderr);
        exit(1);
    }

    app_state->init = dlsym(lib_handle, "init");
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        exit(1);
    }
    app_state->draw = dlsym(lib_handle, "draw");
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        exit(1);
    }
    app_state->shutdown = dlsym(lib_handle, "shutdown");
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        exit(1);
    }

    return 0;
}

int lib_close(struct stak_state_s* app_state) {
    if (!lib_handle) {
        printf("No library loaded\n");
    }
    dlclose(lib_handle);
    return 0;
}

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

    stak_input_init();

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
    struct stak_state_s app_state;

    lib_open(&app_state);
    stak_state_machine_push(application->state_machine, &app_state);
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

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

int stak_application_run(struct stak_application_s* application) {
    struct sigaction action;
    int lib_fd = inotify_init();
    int lib_wd, lib_read_length;
    char lib_notify_buffer[BUF_LEN];

    if( lib_fd < 0 ) {
        perror( "inotify_init" );
        return -1;
    }
    lib_wd = inotify_add_watch( lib_fd, "./", IN_CLOSE_WRITE );
    int flags = fcntl(lib_fd, F_GETFL, 0);
    if (fcntl(lib_fd, F_SETFL, flags | O_NONBLOCK) == -1 ) {
        perror( "fcntl" );
        close(lib_fd);
        return -1;

    }

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
            stak_rpc_get_state(frames_per_second);
            //printf("FPS: %i\n", frames_per_second);
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


        // read inotify buffer to see if library has been modified
        lib_read_length = read( lib_fd, lib_notify_buffer, BUF_LEN );
        if (( lib_read_length < 0 ) && ( errno == EAGAIN)){
        }
        else if(lib_read_length >= 0) {
            int i = 0;
            while ( i < lib_read_length ) {
                struct inotify_event *event = ( struct inotify_event * ) &lib_notify_buffer[ i ];
                //if ( event->len ) {
                    if ( event->mask & IN_CLOSE_WRITE ) {
                        if ( event->mask & IN_ISDIR ) {
                            //printf( "The directory %s was modified.\n", event->name );
                        }
                        else {
                            if( strstr(event->name, "particles.so") != 0 ) {
                                struct stak_state_s app_state;
                                stak_state_machine_pop(application->state_machine, &app_state);
                                lib_close(&app_state);
                                lib_open(&app_state);
                                stak_state_machine_push(application->state_machine, &app_state);
                                printf( "The file %s was modified.\n", event->name );
                            }
                        }
                    }
                    else {
                        //printf("Received event %i on file %s\n", event->mask, event->name);
                    }
                //}
                i += EVENT_SIZE + event->len;
            }
        }
        else {
            perror( "read" );
        }
	}
    return 0;
}

int stak_application_terminate() {
	terminate = 1;
    return 0;
}