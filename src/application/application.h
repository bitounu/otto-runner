#ifndef STAK_APPLICATION_H
#define STAK_APPLICATION_H
#include <pthread.h>

#define STAK_ENABLE_SEPS114A 1
#define STAK_ENABLE_DYLIBS 0

#if STAK_ENABLE_SEPS114A
	#include <graphics/canvas/canvas.h>
	#include <graphics/seps114a/seps114a.h>
#endif

struct stak_application_s{
	struct stak_state_machine_s* state_machine;
	char* plugin_name;
#if STAK_ENABLE_SEPS114A
	stak_canvas_s* canvas;
	stak_seps114a_s* display;
#endif
	pthread_t thread_hal_update;
};
struct stak_application_s* stak_application_create(char* plugin_name);
int stak_application_destroy();
int stak_application_run();
int stak_application_terminate();
int stak_application_get_is_terminating();

uint64_t stak_core_get_time();

int stak_get_rotary_value();


#define max(a,b) \
({ __typeof__ (a) _a = (a); \
 __typeof__ (b) _b = (b); \
 _a > _b ? _a : _b; })

#define min(a,b) \
({ __typeof__ (a) _a = (a); \
 __typeof__ (b) _b = (b); \
 _a < _b ? _a : _b; })


#endif