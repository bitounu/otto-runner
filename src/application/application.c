#include <application/application.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
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

static volatile sig_atomic_t terminate;
static void *lib_handle;

struct stak_state_s{
    int (*init)();
    int (*update)();
    int (*shutdown)();
};

static struct stak_state_s app_state;

//
// lib_open
//
int lib_open(char* plugin_name, struct stak_state_s* app_state) {
    char *error;

    lib_handle = dlopen (plugin_name, RTLD_LAZY);
    printf("loading %s\n", plugin_name);
    if (!lib_handle) {
        fputs (dlerror(), stderr);
        exit(1);
    }

    app_state->init = dlsym(lib_handle, "init");
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        exit(1);
    }
    app_state->update = dlsym(lib_handle, "update");
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

//
// lib_close
//
int lib_close(struct stak_state_s* app_state) {
    if (!lib_handle) {
        printf("No library loaded\n");
    }
    dlclose(lib_handle);
    return 0;
}

//
// stak_core_get_time
//
uint64_t stak_core_get_time() {
    struct timespec timer;
    clock_gettime(CLOCK_MONOTONIC, &timer);
    return (uint64_t) (timer.tv_sec) * 1000000L + timer.tv_nsec / 1000L;
}

//
// stak_application_terminate_cb
//
void stak_application_terminate_cb(int signum)
{
    printf("Terminating...\n");
    terminate = 1;
}

//
// stak_application_create
//
struct stak_application_s* stak_application_create(char* plugin_name) {

    struct stak_application_s* application = calloc(1, sizeof(struct stak_application_s));
    application->plugin_name = malloc( strlen( plugin_name ) + 1 );
    strcpy( application->plugin_name, plugin_name );

#if STAK_ENABLE_SEPS114A
    application->display = stak_seps114a_create();
    application->canvas = stak_canvas_create(STAK_CANVAS_OFFSCREEN, 96, 96);

    if(!bcm2835_init()) {
        printf("Failed to init BCM2835 library.\n");
        stak_application_terminate();
        return 0;
    }
#endif

    lib_open(application->plugin_name, &app_state);
    app_state.init();
    return application;
}

//
// stak_application_destroy
//
int stak_application_destroy(struct stak_application_s* application) {
    app_state.shutdown();

#if STAK_ENABLE_SEPS114A
    stak_canvas_destroy(application->canvas);
    stak_seps114a_destroy(application->display);
#endif
    free(application);
    return 0;
}

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

//
// stak_application_run
//
int stak_application_run(struct stak_application_s* application) {
    struct sigaction action;
    int lib_fd = inotify_init();
    int lib_wd, lib_read_length;
    char lib_notify_buffer[BUF_LEN];

    if( lib_fd < 0 ) {
        perror( "inotify_init" );
        return -1;
    }
    lib_wd = inotify_add_watch( lib_fd, "./build/", IN_CLOSE_WRITE );
    if( lib_wd < 0 ) {
        perror( "inotify_add_watch" );
        return -1;
    }
    int flags = fcntl(lib_fd, F_GETFL, 0);
    if (fcntl(lib_fd, F_SETFL, flags | O_NONBLOCK) == -1 ) {
        perror( "fcntl" );
        close(lib_fd);
        return -1;

    }

        // setup sigterm handler
    action.sa_handler = stak_application_terminate_cb;
    sigaction(SIGINT, &action, NULL);

    uint64_t last_time, current_time, delta_time;
    delta_time = last_time = current_time = stak_core_get_time();
    int frames_this_second = 0;
    int frames_per_second = 0;
    while(!terminate) {
        frames_this_second++;
        current_time = stak_core_get_time();

        
        if(current_time > last_time + 1000000) {
            frames_per_second = frames_this_second;
            frames_per_second = frames_per_second;
            frames_this_second = 0;
            last_time = current_time;
            printf("FPS: %i\n", frames_per_second);
        }

        app_state.update();

#if STAK_ENABLE_SEPS114A
        stak_canvas_swap(application->canvas);
        stak_canvas_copy(application->canvas, (char*)application->display->framebuffer, 96 * 2);
        stak_seps114a_update(application->display);
#endif

        delta_time = (stak_core_get_time() - current_time);
        uint64_t sleep_time = min(16000000L, 16000000L - max(0,delta_time));
        nanosleep((struct timespec[]){{0, sleep_time}}, NULL);

        char* plugin_file_name = 0;

        {
            int start_pos = strrchr( application->plugin_name, '/' ) + 1 - application->plugin_name;
            int length = strlen(application->plugin_name) - start_pos;
            plugin_file_name = malloc(length + 1);
            strcpy(plugin_file_name, application->plugin_name + start_pos);
        }

        // read inotify buffer to see if library has been modified
        lib_read_length = read( lib_fd, lib_notify_buffer, BUF_LEN );
        if (( lib_read_length < 0 ) && ( errno == EAGAIN)){
        }
        else if(lib_read_length >= 0) {
            int i = 0;
            //while ( i < lib_read_length ) {
                struct inotify_event *event = ( struct inotify_event * ) &lib_notify_buffer[ i ];
                if ( event->mask & IN_CLOSE_WRITE ) {
                    if ( ( ! (event->mask & IN_ISDIR) ) && ( strstr(event->name, plugin_file_name) != 0 ) ) {
                        app_state.shutdown();
                        lib_close(&app_state);
                        lib_open(application->plugin_name, &app_state);
                        app_state.init();
                    }
                }
                i += EVENT_SIZE + event->len;
            //}
        }
        else {
            perror( "read" );
        }
    }
    return 0;
}

//
// stak_application_terminate
//
int stak_application_terminate() {
    terminate = 1;
    return 0;
}