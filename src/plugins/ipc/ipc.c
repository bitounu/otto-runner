#include <apis/input/input.h>
#include <application/rpc/rpc.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

struct stak_rpc_msgbuf* message;

int init() {
    
    stak_rpc_init();
    printf("spock: ready to receive messages, captain.\n");

    message = malloc(256);

    setbuf(stdout, NULL);
    return 0;
}

int shutdown() {
    return 0;
}


int draw() {
    struct stak_input_get_state_rpc* msg = 0;
    if( stak_rpc_message_get(message) == 0) {
        switch(message->mtype) {
            case RPC_GET_STATE:
                msg = (struct stak_input_get_state_rpc*)message;
                printf("%i ", msg->some_useless_data);
                msg->mtype = RPC_GET_STATE_RESPONSE;
                if(msg->some_useless_data > 4)
                    msg->some_useless_data = 0;
                else msg->some_useless_data++;
                stak_rpc_message_send(msg, sizeof(struct stak_input_get_state_rpc));
                break;
            default:
                //printf("Received unknown message type %l\n", message->mtype);
                break;
        }
    }
    return 0;
}