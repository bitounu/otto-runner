#include <apis/composer/composer.h>
#include <application/rpc/rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// prototypes
int init();
int shutdown();
int update();

int init() {
    printf("Testing.\n");

    stak_rpc_init();

    int context = stak_rpc_composer_create_gl_context();
    printf("Context: %i\n", context);

    return 0;
}

int shutdown() {
    return 0;
}


int update() {
    nanosleep( (struct timespec[] ){ { 0, 33000000L } }, NULL);
    return 0;
}
