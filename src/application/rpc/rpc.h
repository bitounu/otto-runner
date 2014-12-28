#ifndef STAK_APPLICATION_RPC_H
#define STAK_APPLICATION_RPC_H

#define RPC_DEFINE_ID(x) RPC_##x, RPC_##x##_RESPONSE

struct stak_generic_message_buf_rpc {
	long mtype;
    char mtext[1];
};

struct stak_rpc_msgbuf {
    long mtype;       /* message type, must be > 0 */
	long argument_count;
    long data[];     /* message data */ 
};


enum stak_rpc_calls {
	RPC_NONE = 0,
	RPC_DEFINE_ID(GET_STATE),
	RPC_DEFINE_ID(SET_STATE),
	RPC_LAST
};

int stak_rpc_init();
int stak_rpc_message_send(void* message, int size);
int stak_rpc_message_get(struct stak_rpc_msgbuf* message);
int stak_rpc_message_wait(struct stak_rpc_msgbuf* message, int type);

int stak_rpc_buffer_get_int(struct stak_rpc_msgbuf* buffer, int argument_number);
struct stak_input_state_s stak_input_get_state();
#endif