#include <apis/input/input.h>
#include <application/rpc/rpc.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int stak_rpc_input_get_state(int fps) {
	struct stak_input_get_state_rpc message;
	message.mtype = RPC_GET_STATE;
    message.some_useless_data = fps;
	if(stak_rpc_message_send(&message, sizeof(struct stak_input_get_state_rpc)))
        return -1;
    if(stak_rpc_message_wait(&message, RPC_GET_STATE_RESPONSE))
        return -1;

    return message.some_useless_data;
}