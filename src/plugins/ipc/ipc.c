#include <daemons/input/input.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static int ipc_message_queue;

struct stak_rpc_msgbuf* message;

int init() {
    
    stak_input_init();
    printf("spock: ready to receive messages, captain.\n");

    message = malloc(256);

    
    return 0;
}

int shutdown() {
    return 0;
}


int draw() {
    if( stak_remote_input_get_state(message) == 0) {
        switch(message->mtype) {
            case IPC_GET_STATE:
                printf("%i \n",
                        ((struct stak_input_get_state_rpc*)message)->some_useless_data);
                break;
            default:
                printf("Received unknown message type %l\n", message->mtype);
                break;
        }
    }
    return 0;
}