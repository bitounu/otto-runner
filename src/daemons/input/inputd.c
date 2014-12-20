#include <daemons/input/input.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static int ipc_key;
static int ipc_message_queue;

struct msgbuf {
    long mtype;       /* message type, must be > 0 */
    char mtext[1];    /* message data */ 
};

// Input
int main(int argc, char** argv) {
	struct stak_input_get_state_rpc state_rpc;
	struct stak_input_state_s istate = {
		.shutter = 1,
		.rotary_button = 0,
		.rotary_delta = 3,
	};
	ipc_key = ftok("./ipc_keys", 'a');
    ipc_message_queue = msgget(ipc_key, 0666 | IPC_CREAT);

    if ((ipc_message_queue = msgget(ipc_key, 0666)) == -1) { /* connect to the queue */
        perror("msgget");
        return -1;
    }
    
    printf("spock: ready to receive messages, captain.\n");

    char buffer[1024];
    for(;;) { /* Spock never quits! */
        if (msgrcv(ipc_message_queue, &buffer, sizeof(state_rpc.some_useless_data), 0, 0) == -1) {
            perror("msgrcv");
        	return -1;
        }else {
        	struct msgbuf* message = (struct msgbuf*)buffer;
        	switch(message->mtype) {
        		case IPC_GET_STATE:
        			printf("Received IPC_GET_STATE\n");
        			printf("fps: \"%i\"\n", ((struct stak_input_get_state_rpc*)buffer)->some_useless_data);
        			break;
        		default:
        			printf("Received unknown message type\n");
        			break;
        	}

        }
    }
}

// union?