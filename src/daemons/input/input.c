#include <daemons/input/input.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


static int ipc_key;
static int ipc_message_queue;
int stak_input_init() {
	;
	if ((ipc_key = ftok("./ipc_keys", 'a')) == -1) {
        perror("ftok");
        return -1;
    }

    if ((ipc_message_queue = msgget(ipc_key, 0666)) == -1) {
        perror("msgget");
        return -1;
    }
    return 0;
}

/*struct stak_input_state_s stak_input_get_state() {

}*/

int stak_rpc_get_state(int fps) {
	struct stak_input_get_state_rpc message;
	message.mtype = IPC_GET_STATE;
	message.some_useless_data = fps;
    if(msgsnd(ipc_message_queue, &message, sizeof(int), 0) == -1) {
    	perror("msgsnd");
    	return -1;
    }
    return 0;
}