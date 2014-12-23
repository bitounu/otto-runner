#ifndef STAK_API_INPUT_H
#define STAK_API_INPUT_H
#include <application/rpc/rpc.h>


// return type for 'stak_rpc_input_get_rotary_position' call
struct stak_input_get_rotary_position_rpc {
	long mtype;
    int rotary_position;
};


// list of all RPC calls for input api.
enum stak_rpc_input_calls {
	RPC_INPUT_START = 128,					// each api should have a unique starting value
	RPC_DEFINE_ID(GET_ROTARY_POSITION),
	RPC_INPUT_END
};

int stak_rpc_input_get_rotary_position();
#endif