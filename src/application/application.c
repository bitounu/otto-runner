#include <application/application.h>
#include <core/core.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

static volatile sig_atomic_t terminate = 0;
//static pthread_t thread_seps114a_update;


#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

// SIGINT signal handler
void stak_application_terminate_cb(int signum)
{
    printf("Terminating...\n");
    terminate = 1;
}
int stak_application_hal_thread() {

    return 0;
}

struct stak_application_s* stak_application_create() {
	struct stak_application_s* application = calloc(1, sizeof(struct stak_application_s));
	application->state_machine = stak_state_machine_create( 32 );
    return application;
}
int stak_application_destroy(struct stak_application_s* application) {
	stak_state_machine_destroy(application->state_machine);
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
    /*int frames_this_second = 0;
    int frames_per_second = 0;*/
    //init();
	while(!terminate) {
		//stak_state_machine_top(application->state_machine, current_state);
		//frames_this_second++;
        current_time = stak_core_get_time();

        
        /*if(current_time > last_time + 1000000) {
            frames_per_second = frames_this_second;
            frames_this_second = 0;
            last_time = current_time;
        }*/


        stak_state_machine_run(application->state_machine);
        //stak_seps114a_update(&lcd_device);

        delta_time = (stak_core_get_time() - current_time);
        uint64_t sleep_time = min(33000000L, 33000000L - max(0,delta_time));
        nanosleep((struct timespec[]){{0, sleep_time}}, NULL);
	}
    //shutdown();
    return 0;
}

int stak_application_terminate() {
	terminate = 1;
    return 0;
}