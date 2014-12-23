#include <apis/input/input.h>
#include <application/rpc/rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

static int value;
static int last_value;
// prototypes
int init();
int shutdown();
int update();

int init() {
    printf("Testing.\n");

    stak_rpc_init();

    value = 0;
    last_value = 0;

    return 0;
}

int shutdown() {
    return 0;
}


int update() {
    value = stak_rpc_input_get_rotary_position( );

    if(value != last_value) {
    	printf("value: %i\n", value);
    	last_value = value;
    }
    nanosleep( (struct timespec[] ){ { 0, 33000000L } }, NULL);
    return 0;
}
