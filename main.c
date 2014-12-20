#include <application/application.h>
#include <application/state/state.h>
#include <core/core.h>
#include <lib/shapes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char** argv) {
    struct stak_application_s* application = 0;
    application = stak_application_create();
    stak_application_run(application);
    stak_application_destroy(application);
    return 0;
}