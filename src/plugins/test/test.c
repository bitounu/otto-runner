#include <apis/input/input.h>
#include <application/rpc/rpc.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

static int value;
// prototypes
int init();
int shutdown();
int redraw();

int init() {
    printf("Testing.\n");

    stak_rpc_init();
    value = 0;
    return 0;
}

int shutdown() {
    return 0;
}


int draw() {
    value = stak_rpc_input_get_state( value++ );
    nanosleep( (struct timespec[] ){ { 0, 1250000000L } }, NULL);
    return 0;
}
