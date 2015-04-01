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

volatile int last_encoded_value = 0, encoder_value = 0, encoder_delta = 0, encoder_last_delta = 0;
volatile static int shutter_state = -1, power_state = -1, rotary_toggle_state = -1;
static pthread_mutex_t display_mutex;

#if 0
int check_button_changed(button_state *button, int pin) {
    if (!button->is_changed) {
        int state = bcm2835_gpio_lev(pin);
        button->is_changed = state != button->state;
        button->state = state;
    }
    return button->is_changed;
}

//
// shutter button
//
int get_shutter_button_pressed() {
    return check_button_changed(&shutter_button, pin_shutter_button) && shutter_button.state == 0;
}
int get_shutter_button_released() {
    return check_button_changed(&shutter_button, pin_shutter_button) && shutter_button.state == 1;
}
int get_shutter_button_state() {
    return ( shutter_button.state == 1 );
}

//
// power button
//
int get_power_button_pressed() {
    return check_button_changed(&power_button, pin_power_button) && power_button.state == 0;
}
int get_power_button_released() {
    return check_button_changed(&power_button, pin_power_button) && power_button.state == 1;
}
int get_power_button_state() {
    return ( power_button.state == 1 );
}

//
// crank
//
int get_crank_pressed() {
    return check_button_changed(&rotary_button, pin_rotary_button) && rotary_button.state == 1;
}
int get_crank_released() {
    return check_button_changed(&rotary_button, pin_rotary_button) && rotary_button.state == 0;
}
int get_crank_state() {
    return ( rotary_button.state == 1 );
}

#endif

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
    if (!mode->isInitialized && mode->init) {
        mode->init();
        mode->isInitialized = 1;
    }
}

static stak_state_s *mode_queued_for_activation = 0;

void stak_activate_mode() {
    mode_queued_for_activation = &mode_state;
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
struct stak_application_s* stak_application_create(char* menu_filename, char* mode_filename) {

    struct stak_application_s* application = calloc(1, sizeof(struct stak_application_s));
    application->menu_filename = malloc( strlen( menu_filename ) + 1 );
    strcpy( application->menu_filename, menu_filename );
    application->mode_filename = malloc( strlen( mode_filename ) + 1 );
    strcpy( application->mode_filename, mode_filename );

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
    lib_open(application->mode_filename, &mode_state);

    menu_state.isInitialized = 0;
    mode_state.isInitialized = 0;

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

    ottoHardwareTerminate();

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


static void encoder_callback( int delta ){ encoder_value += delta; };
static void shutter_callback( int isPressed ){ shutter_state = isPressed; };
static void power_callback( int isPressed ){ power_state = isPressed; };


//
// update_encoder
//
static void* thread_display(void* arg) {
    
    struct stak_application_s* application = (struct stak_application_s*) arg;

#if 0
    // set thread priority
    struct sched_param schedule;
    memset (&schedule, 0, sizeof(schedule)) ;
    int priority = 1;

    if (priority > sched_get_priority_max (SCHED_RR))
        schedule.sched_priority = sched_get_priority_max (SCHED_RR) ;
    else
        schedule.sched_priority = priority;

    sched_setscheduler (0, SCHED_RR, &schedule);
#endif

    uint64_t last_second_time, last_frame_time, current_time, delta_time;
    delta_time = last_second_time = last_frame_time = current_time = stak_core_get_time();
    int frames_this_second = 0;
    int frames_per_second = 0;


    while( !stak_application_get_is_terminating() ) {
        current_time = stak_core_get_time();

        if(current_time > last_second_time + 1000000) {
            frames_per_second = frames_this_second;
            frames_per_second = frames_per_second;
            frames_this_second = 0;
            last_second_time = current_time;
            printf("OLED FPS: %i\n", frames_per_second);
            //std::cout << "OLED FPS: " << frames_per_second << " Delta: " << delta_time << std::endl;
        }

        //pthread_mutex_lock(&display_mutex);
        frames_this_second++;
        
        stak_seps114a_update( application->display );
        
        //pthread_mutex_unlock(&display_mutex);
        delta_time = (stak_core_get_time() - current_time);
    }
    return 0;
}


//
// stak_application_run
//
int stak_application_run(struct stak_application_s* application) {
    struct sigaction action;
    pthread_t pth_display;

    ottoHardwareInit();

    ottoRotarySetCallback( encoder_callback );
    ottoButtonShutterSetCallback( shutter_callback );
    ottoButtonPowerSetCallback( power_callback );

    if( pthread_mutex_init(&display_mutex, NULL) != 0)
        return -1;

    pthread_create(&pth_display, NULL, thread_display, (void*) application);

        // setup sigterm handler
    action.sa_handler = stak_application_terminate_cb;
    sigaction(SIGINT, &action, NULL);

    uint64_t last_second_time, last_frame_time, current_time, delta_time;
    delta_time = last_second_time = last_frame_time = current_time = stak_core_get_time();
    int frames_this_second = 0;
    int frames_per_second = 0;
    int rotary_last_value = 0;

    while( !stak_application_get_is_terminating() ) {
        frames_this_second++;
        current_time = stak_core_get_time();

        /*if( current_time > last_second_time + 1000000 ) {
            frames_per_second = frames_this_second;
            frames_per_second = frames_per_second;
            frames_this_second = 0;
            last_second_time = current_time;
            stak_log("FPS: %i", frames_per_second);
        }*/

        if( mode_queued_for_activation ) {
            activate_mode(mode_queued_for_activation);
            mode_queued_for_activation = 0;
        }

        if( active_mode->crank_rotated ) {
            if(rotary_last_value != encoder_value) {
                active_mode->crank_rotated(rotary_last_value - encoder_value);
                rotary_last_value = encoder_value;
            }
        }

        if( shutter_state != -1 ) {
            if( shutter_state == 0 ) {
                active_mode->shutter_button_released();
            }else if( active_mode->shutter_button_pressed ) {
                active_mode->shutter_button_pressed();
            }
            shutter_state = -1;
        }

        if( power_state != -1 ) {
            if( active_mode != &menu_state ) {
                activate_mode(&menu_state);

                if( power_state == 0 ) {
                    active_mode->power_button_released();
                }else {
                    active_mode->power_button_pressed();
                }
            }else {
                if( power_state == 0 ) {
                    active_mode->power_button_released();
                }else {
                    active_mode->power_button_pressed();
                }
            }
            power_state = -1;
        }

        uint64_t frame_delta_time = current_time - last_frame_time;
        last_frame_time = current_time;

        if(active_mode->update) {
            float dt = ((float)frame_delta_time) / 1000000.0f;
            active_mode->update(dt);
        }
        //pthread_mutex_lock(&display_mutex);
        if(active_mode->draw) {
            active_mode->draw();
        }
        stak_canvas_swap(application->canvas);
        stak_canvas_copy(application->canvas, (uint8_t*)application->display->framebuffer, 96 * 2);
        //pthread_mutex_unlock(&display_mutex);

        delta_time = (stak_core_get_time() - current_time);
        uint64_t sleep_time = min(33000000L, 33000000L - max(0L, delta_time * 1000L));
        nanosleep((struct timespec[]){ { 0, sleep_time } }, NULL);
    }
    if(pthread_join(pth_display, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return -1;
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