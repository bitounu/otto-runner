#ifndef STAK_APPLICATION_H
#define STAK_APPLICATION_H
#include <application/state/state.h>
struct stak_application_s{
	struct stak_state_machine_s* state_machine;
};
struct stak_application_s* stak_application_create();
int stak_application_destroy();
int stak_application_run();
int stak_application_terminate();
#endif