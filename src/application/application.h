#ifndef STAK_APPLICATION_H
#define STAK_APPLICATION_H
#include <application/state/state.h>
#include <pthread.h>
#if 0
	#include <graphics/canvas/canvas.h>
	#include <graphics/seps114a/seps114a.h>
#endif
struct stak_application_s{
	struct stak_state_machine_s* state_machine;
	char* plugin_name;
#if 0
	stak_canvas_s* canvas;
	stak_seps114a_s* display;
#endif
	pthread_t thread_hal_update;
};
struct stak_application_s* stak_application_create(char* plugin_name);
int stak_application_destroy();
int stak_application_run();
int stak_application_terminate();
#endif