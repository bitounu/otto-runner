#include <application/application.h>

int main(int argc, char** argv) {
    struct stak_application_s* application = 0;
	if(argc > 2) {
    	application = stak_application_create(argv[1], argv[2]);
	}
	else {
        // TODO(ryan): This will probably not work. Make actual default .so's?
    	application = stak_application_create("/stak/sdk/libotto_menu.so", "/stak/sdk/libotto_gif_mode.so");
	}
    stak_application_run(application);
    stak_application_destroy(application);
    return 0;
}
