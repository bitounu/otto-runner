#include <application/state/state.h>
#include <stdlib.h>
#include <string.h>

struct stak_state_machine_s* stak_state_machine_create( int stack_size ) {
	struct stak_state_machine_s* state_machine = calloc( 1, sizeof( struct stak_state_machine_s ) );
	state_machine->states = calloc( stack_size, sizeof( struct stak_state_s ) );
	state_machine->size = stack_size;
	state_machine->position = 0;
	return state_machine;
}
int stak_state_machine_destroy(struct stak_state_machine_s* state_machine) {
	if ( state_machine ) {
		struct stak_state_s state;
		while (stak_state_machine_pop(state_machine, &state) != -1);
		free( state_machine->states );
		free( state_machine );
	}
    return 0;
}
int stak_state_machine_run(struct stak_state_machine_s* state_machine) {
	struct stak_state_s state;
	int error = stak_state_machine_top(state_machine, &state);
	if( error ) {
		return -1;
	} else { 
		state.draw();
    	return 0;
	}

}
int stak_state_machine_push(struct stak_state_machine_s* state_machine, struct stak_state_s* state) {
	if (( state_machine ) && ( state_machine->size ) && ( state_machine->position < state_machine->size )) {
		memcpy( &state_machine->states[state_machine->position], state, sizeof( struct stak_state_s) );
		state_machine->states[state_machine->position].init();
		state_machine->position++;
		return 0;
	}
	else return -1;
}
int stak_state_machine_pop(struct stak_state_machine_s* state_machine, struct stak_state_s* state) {
	if (( state_machine ) && ( state_machine->size ) && ( state_machine->position > 0 )) {
		memcpy( state, &state_machine->states[state_machine->position-1], sizeof( struct stak_state_s) );
		state_machine->states[state_machine->position-1].shutdown();
		state_machine->position--;
		return 0;
	}
	else return -1;
}
int stak_state_machine_top(struct stak_state_machine_s* state_machine, struct stak_state_s* state) {
	if (( state_machine ) && ( state_machine->size ) && ( state_machine->position )) {
		memcpy( state, &state_machine->states[state_machine->position-1], sizeof( struct stak_state_s) );
		return 0;
	}
	else return -1;
}