#ifndef STAK_API_INPUT_H
#define STAK_API_INPUT_H


struct stak_input_get_state_rpc {
	long mtype;
    int some_useless_data;
};


struct stak_input_state_s {
	int shutter;
	int rotary_button;
	int rotary_delta;
};

int stak_rpc_input_get_state();
struct stak_input_state_s stak_input_get_state();
#endif