#include <apis/input/input.h>
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

    stak_input_init();
    value = 0;
    return 0;
}

int shutdown() {
    return 0;
}


int draw() {
    value = stak_rpc_input_get_state( value++ );
    nanosleep( (struct timespec[] ){ { 1, 0 } }, NULL);
    return 0;
}
