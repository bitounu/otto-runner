#include <application/rpc/rpc.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


static int ipc_key;
static int ipc_message_queue;
static int ipc_shm_id;


int stak_rpc_init() {
	if ((ipc_key = ftok("./main", 'a')) == -1) {
        perror("ftok");
        return -1;
    }

    if ((ipc_message_queue = msgget(ipc_key, 0666 | IPC_CREAT)) == -1) {
        perror("msgget");
        return -1;
    }
    return 0;
}

int stak_rpc_message_send(void* message, int size) {
    if(msgsnd(ipc_message_queue, message, sizeof(long)*(size+1), 0) == -1) {
        perror("msgsnd");
        return -1;
    }
    return 0;

}

int stak_rpc_message_get(struct stak_rpc_msgbuf* message) {
    if (msgrcv(ipc_message_queue, message, 128, 0, 0) == -1) {
        perror("msgrcv");
        return -1;
    }
    return 0;
}

int stak_rpc_message_wait(struct stak_rpc_msgbuf* message, int type) {
    if (msgrcv(ipc_message_queue, message, 128, type, 0) == -1) {
        perror("msgrcv");
        return -1;
    }
    return 0;
}

int stak_rpc_buffer_get_int(struct stak_rpc_msgbuf* buffer, int argument_number) {
    if( (buffer == 0) ||
        (argument_number < 0) ||
        (argument_number > buffer->argument_count) ) {
        return -1;
    }
    return buffer->data[argument_number];
}