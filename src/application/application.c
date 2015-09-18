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

#include <stak.h>

#include <pthread.h>
#include <sched.h>

#include <bcm2835.h>

#include <otto/otto.hpp>
#include <otto/system.hpp>
#include <otto/devices/disk.hpp>
#include <otto/io/rotary.hpp>
#include <otto/io/buttons.hpp>

static volatile sig_atomic_t terminate;
static void *lib_handle;
#define stak_log //

typedef struct {
    int ( *init )                       ( void );                   // int init                     ( void );
    int ( *update )                     ( float delta );            // int update                   ( float delta );
    int ( *draw )                       ( void );                   // int draw                     ( void );
    int ( *shutdown )                   ( void );                   // int shutdown                 ( void );
    int ( *shutter_button_released )    ( void );                   // int shutter_button_release   ( void );
    int ( *shutter_button_pressed )     ( void );                   // int shutter_button_press     ( void );
    int ( *power_button_released )      ( void );                   // int power_button_release     ( void );
    int ( *power_button_pressed )       ( void );                   // int power_button_press       ( void );
    int ( *crank_released )             ( void );                   // int crank_up                 ( void );
    int ( *crank_pressed )              ( void );                   // int crank_down               ( void );
    int ( *crank_rotated )              ( int amount );             // int crank_rotated            ( int amount );
    char *assets_path;
    int isInitialized;
} stak_state_s;

static stak_state_s menu_state;
static stak_state_s mode_state;
static stak_state_s *active_mode = &menu_state;

const int pin_shutter_button = 16;
const int pin_power_button = 4;
const int pin_rotary_button = 17;
const int pin_rotary_a = 15;
const int pin_rotary_b = 14;

typedef struct {
    int state, is_changed;
} button_state;

static button_state rotary_button = { 1, 0 };
static button_state shutter_button = { 1, 0 };
static button_state power_button = { 1, 0 };

volatile int last_encoded_value = 0, encoder_value = 0, encoder_delta = 0, encoder_last_delta = 0;
volatile static int shutter_state = -1, power_state = -1, rotary_toggle_state = -1;


//
// lib_open
//
int lib_open(const char* plugin_name, stak_state_s* app_state) {
    char *error;

    lib_handle = dlopen (plugin_name, RTLD_LAZY);
    printf("loading %s\n", plugin_name);
    if (!lib_handle) {
        fputs (dlerror(), stderr);
        exit(1);
    }

    // Generate the assets path for this lib
    {
        char *slash_pos = strrchr(plugin_name, '/');
        size_t prefix_size = slash_pos ? (size_t)(slash_pos + 1 - plugin_name) : 0;
        app_state->assets_path = malloc(prefix_size + strlen("assets/"));
        strncpy(app_state->assets_path, plugin_name, prefix_size);
        strcat(app_state->assets_path, "assets/");
    }

    // int (*init)           ( void );
    app_state->init = dlsym(lib_handle, "init");
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        exit(1);
    }

    // int (*update)         ( float delta );
    app_state->update = dlsym(lib_handle, "update");
    if ((error = dlerror()) != NULL) app_state->update = 0;

    // int (*draw)           ( void );
    app_state->draw = dlsym(lib_handle, "draw");
    if ((error = dlerror()) != NULL) app_state->draw = 0;

    // int (*shutter_button_released)     ( void );
    app_state->shutter_button_released = dlsym(lib_handle, "shutter_button_released");
    if ((error = dlerror()) != NULL) app_state->shutter_button_released = 0;

    // int (*shutter_button_pressed)   ( void );
    app_state->shutter_button_pressed = dlsym(lib_handle, "shutter_button_pressed");
    if ((error = dlerror()) != NULL) app_state->shutter_button_pressed = 0;

    // int (*power_button_released)       ( void );
    app_state->power_button_released = dlsym(lib_handle, "power_button_released");
    if ((error = dlerror()) != NULL) app_state->power_button_released = 0;

    // int (*power_button_pressed)     ( void );
    app_state->power_button_pressed = dlsym(lib_handle, "power_button_pressed");
    if ((error = dlerror()) != NULL) app_state->power_button_pressed = 0;

    // int (*crank_pressed)       ( void );
    app_state->crank_pressed = dlsym(lib_handle, "crank_pressed");
    if ((error = dlerror()) != NULL) app_state->crank_pressed = 0;

    // int (*crank_released)     ( void );
    app_state->crank_released = dlsym(lib_handle, "crank_released");
    if ((error = dlerror()) != NULL) app_state->crank_released = 0;

    // int (*crank_rotated)  ( int amount );
    app_state->crank_rotated = dlsym(lib_handle, "crank_rotated");
    if ((error = dlerror()) != NULL) app_state->crank_rotated = 0;

    // int (*shutdown)       ( void );
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
int lib_close(stak_state_s* app_state) {
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
// activate_mode
//
static void activate_mode(stak_state_s *mode) {
    active_mode = mode;
    if (!mode->isInitialized) {
        if (mode->init) mode->init();
        mode->isInitialized = 1;
    }
}

static stak_state_s *mode_queued_for_activation = 0;

void stak_activate_mode() {
    mode_queued_for_activation = &mode_state;
}

void stak_load_mode(char *mode_filename) {
    if (mode_state.isInitialized && mode_state.shutdown) {
        mode_state.shutdown();
    }

    lib_open(mode_filename, &mode_state);

    mode_state.isInitialized = 0;
}

//
// stak_assets_path
//

const char *stak_assets_path() {
    return active_mode->assets_path;
}

//
// stak_application_terminate_cb
//
void stak_application_terminate_cb(int signum)
{
    stak_log("Terminating...%s", "");
    stak_application_terminate();
}

//
// stak_application_create
//
struct stak_application_s* stak_application_create(char* menu_filename) {
    struct stak_application_s* application = calloc(1, sizeof(struct stak_application_s));
    application->menu_filename = malloc( strlen( menu_filename ) + 1 );
    strcpy( application->menu_filename, menu_filename );

#if STAK_ENABLE_SEPS114A
    application->display = stak_seps114a_create();
    application->canvas = stak_canvas_create(STAK_CANVAS_OFFSCREEN, 96, 96);

    if(!bcm2835_init()) {
        printf("Failed to init BCM2835 library.\n");
        stak_application_terminate();
        return 0;
    }
#endif

    lib_open(application->menu_filename, &menu_state);

    menu_state.isInitialized = 0;

    activate_mode(&menu_state);

    return application;
}

//
// stak_application_destroy
//
int stak_application_destroy(struct stak_application_s* application) {
    // if shutdown method exists, run it
    if(menu_state.shutdown) menu_state.shutdown();
    if(mode_state.shutdown) mode_state.shutdown();

    /*if(pthread_join(application->thread_hal_update, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return -1;
    }*/
#if STAK_ENABLE_SEPS114A
    stak_canvas_destroy(application->canvas);
    stak_seps114a_destroy(application->display);
#endif
    free(application);
    return 0;
}


#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )


static int signum( int x ) { return x > 0 ? 1 : x < 0 ? -1 : 0; }
static void encoder_callback( int delta ){ encoder_value += delta; };
static void shutter_callback( int isPressed ){ shutter_state = isPressed; };
static void power_callback( int isPressed ){ power_state = isPressed; };
//
// stak_application_run
//
int stak_application_run(struct stak_application_s* application) {
    struct sigaction action;

#if 0
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
#endif

    ottoHardwareInit();

    //bcm2835_gpio_fsel(pin_shutter_button, BCM2835_GPIO_FSEL_INPT);
    //bcm2835_gpio_fsel(pin_rotary_a, BCM2835_GPIO_FSEL_INPT);
    //bcm2835_gpio_fsel(pin_rotary_b, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(pin_rotary_button, BCM2835_GPIO_FSEL_INPT);
    //bcm2835_gpio_fsel(pin_power_button, BCM2835_GPIO_FSEL_INPT);

    // set pin pull up/down status
    //bcm2835_gpio_set_pud(pin_shutter_button, BCM2835_GPIO_PUD_UP);
    //bcm2835_gpio_set_pud(pin_rotary_a, BCM2835_GPIO_PUD_UP);
    //bcm2835_gpio_set_pud(pin_rotary_b, BCM2835_GPIO_PUD_UP);
    bcm2835_gpio_set_pud(pin_rotary_button, BCM2835_GPIO_PUD_UP);

    //pthread_create(&application->thread_hal_update, NULL, update_encoder, NULL);

        // setup sigterm handler
    action.sa_handler = stak_application_terminate_cb;
    sigaction(SIGINT, &action, NULL);

    uint64_t last_second_time, last_frame_time, current_time, delta_time;
    delta_time = last_second_time = last_frame_time = current_time = stak_core_get_time();
    int frames_this_second = 0;
    int frames_per_second = 0;
    int rotary_last_direction = -1;
    int rotary_last_successful_direction = 1;
    int rotary_last_value = 0;

    ottoRotarySetCallback( encoder_callback );
    ottoButtonShutterSetCallback( shutter_callback );
    ottoButtonPowerSetCallback( power_callback );

    while(!terminate) {
        frames_this_second++;
        current_time = stak_core_get_time();

        if(current_time > last_second_time + 1000000) {
            frames_per_second = frames_this_second;
            frames_per_second = frames_per_second;
            frames_this_second = 0;
            last_second_time = current_time;
            stak_log("FPS: %i", frames_per_second);
        }

        if(mode_queued_for_activation) {
            activate_mode(mode_queued_for_activation);
            mode_queued_for_activation = 0;
        }

        if (active_mode->crank_rotated) {
            if (rotary_last_value != encoder_value) {
                int delta = rotary_last_value - encoder_value;
                int direction = signum(delta);
                if (direction == rotary_last_direction ||
                    direction == rotary_last_successful_direction) {
                    active_mode->crank_rotated(delta);
                    rotary_last_successful_direction = direction;
                }
                rotary_last_direction = direction;
                rotary_last_value = encoder_value;
            }
        }


        if( shutter_state != -1 ) {
            if( ( shutter_state == 0 ) && ( active_mode->shutter_button_pressed ) ){
                active_mode->shutter_button_pressed();
            }else if( active_mode->shutter_button_released ) {
                active_mode->shutter_button_released();
            }
            shutter_state = -1;
        }

        if( power_state != -1 ) {
            if( active_mode != &menu_state ) {
                activate_mode(&menu_state);
            }

            if( ( power_state == 0 ) && ( active_mode->power_button_pressed ) ){
                active_mode->power_button_pressed();
            }else if ( active_mode->power_button_released ){
                active_mode->power_button_released();
            }
            power_state = -1;
        }


        uint64_t frame_delta_time = current_time - last_frame_time;
        last_frame_time = current_time;

        if(active_mode->update) {
            float dt = ((float)frame_delta_time) / 1000000.0f;
            active_mode->update(dt);
        }

        if(active_mode->draw) {
            active_mode->draw();
        }

#if STAK_ENABLE_SEPS114A
    #if 1
        stak_canvas_swap(application->canvas);
        stak_canvas_copy(application->canvas, (uint8_t*)application->display->framebuffer, 96 * 2);
    #endif
    #if 1
        stak_seps114a_update(application->display);
    #endif
#endif

        delta_time = (stak_core_get_time() - current_time);
        uint64_t sleep_time = min(33000000L, 33000000L - max(0L, delta_time * 1000L));
        nanosleep((struct timespec[]){ { 0, sleep_time } }, NULL);

        // char* plugin_file_name = 0;

        // {
        //     int start_pos = strrchr( application->plugin_name, '/' ) + 1 - application->plugin_name;
        //     int length = strlen(application->plugin_name) - start_pos;
        //     plugin_file_name = malloc(length + 1);
        //     strcpy(plugin_file_name, application->plugin_name + start_pos);
        // }

        // // read inotify buffer to see if library has been modified
        // lib_read_length = read( lib_fd, lib_notify_buffer, BUF_LEN );
        // if (( lib_read_length < 0 ) && ( errno == EAGAIN)){
        // }
        // else if(lib_read_length >= 0) {
        //     int i = 0;
        //     //while ( i < lib_read_length ) {
        //         struct inotify_event *event = ( struct inotify_event * ) &lib_notify_buffer[ i ];
        //         if ( event->mask & IN_CLOSE_WRITE ) {
        //             if ( ( ! (event->mask & IN_ISDIR) ) && ( strstr(event->name, plugin_file_name) != 0 ) ) {

        //                 // if shutdown method exists, run it
        //                 if(app_state.shutdown) {
        //                     app_state.shutdown();
        //                 }

        //                 // close currently open lib and reload it
        //                 lib_close(&app_state);
        //                 lib_open(application->plugin_name, &app_state);

        //                 // if init method exists, run it
        //                 if(app_state.init) {
        //                     app_state.init();
        //                 }
        //             }
        //         }
        //         i += EVENT_SIZE + event->len;
        //     //}
        // }
        // else {
        //     perror( "read" );
        // }
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
//
// stak_application_terminate
//
int stak_application_get_is_terminating() {
    return terminate;
}

int error_throw( const char* file, int line, const char* function,const char* string ) {
    printf( "\33[36m[\33[35m %s \33[36;1m@\33[0;33m %4i \33[36m] \33[0;34m %64s \33[31mERROR\33[0;39m %s\n", file, line, function, string );
    return -1;
}

int stak_get_rotary_value( ) {
    return encoder_delta;
}
