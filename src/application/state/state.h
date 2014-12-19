#ifndef STAK_STATE_H
#define STAK_STATE_H
struct stak_state_s{
	int (*init)();
	int (*draw)();
	int (*shutdown)();
};

struct stak_state_machine_s{
	struct stak_state_s* states;
	int size;
	int position;
};

struct stak_state_machine_s* stak_state_machine_create(int stack_size);
int stak_state_machine_destroy(struct stak_state_machine_s* state_machine);
int stak_state_machine_run(struct stak_state_machine_s* state_machine);
int stak_state_machine_push(struct stak_state_machine_s* state_machine, struct stak_state_s* state);
int stak_state_machine_pop(struct stak_state_machine_s* state_machine, struct stak_state_s* state);
int stak_state_machine_top(struct stak_state_machine_s* state_machine, struct stak_state_s* state);

#endif