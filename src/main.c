#include <application/application.h>

int main(int argc, char** argv) {
    struct stak_application_s* application = 0;
	if(argc > 1) {
    	application = stak_application_create(argv[1]);
	}
	else {
    	application = stak_application_create("./test.so");
	}
    stak_application_run(application);
    stak_application_destroy(application);
    return 0;
}