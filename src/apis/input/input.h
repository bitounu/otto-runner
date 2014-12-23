#ifndef STAK_DAEMONS_INPUT_H
#define STAK_DAEMONS_INPUT_H

struct stak_generic_message_buf_rpc {
	long mtype;
    char mtext[1];
};

struct stak_input_get_state_rpc {
	long mtype;
    int some_useless_data;
};
struct stak_rpc_msgbuf {
    long mtype;       /* message type, must be > 0 */
    char mtext[];    /* message data */ 
};


struct stak_input_state_s {
	int shutter;
	int rotary_button;
	int rotary_delta;
};


enum stak_ipc_calls {
	IPC_GET_STATE = 1
};

int stak_input_init();
int stak_rpc_input_get_state();
int stak_remote_input_get_state(struct stak_rpc_msgbuf* message);
struct stak_input_state_s stak_input_get_state();
#endif