#include <apis/composer/composer.h>
#include <application/rpc/rpc.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <VG/openvg.h>
#include <assert.h>

int stak_rpc_composer_create_gl_context( struct stak_composer_gl_info_s* gl_info, int width, int height ) {
	// create a simple message to tell the daemon which function
	// we want to call.
	{
		/*long message[] = {
			RPC_COMPOSER_CREATE_GL_CONTEXT,
			2,
			width,
			height
		};

		// send message to daemon
		if(stak_rpc_message_send((void*)&message, 2)) {
	        printf("Error occurred sending message\n");
	        return -1;
		}
	}
	
	{
		long message[10];
	    // wait for a response or return -1 if an error occurred or
	    // call was interrupted by system
	    if(stak_rpc_message_wait((void*)&message, RPC_COMPOSER_CREATE_GL_CONTEXT_RESPONSE)) {
	        printf("Error occurred waiting for response\n");
	        return -1;
	    }*/

		DISPMANX_ELEMENT_HANDLE_T dispman_element;
		DISPMANX_DISPLAY_HANDLE_T dispman_display;
		DISPMANX_UPDATE_HANDLE_T dispman_update;
		VC_RECT_T dst_rect;
		VC_RECT_T src_rect;

		//graphics_get_display_size(0 /* LCD */, &width, &height);
		//assert( s >= 0 );

		dst_rect.x = 0;
		dst_rect.y = 0;
		dst_rect.width = width;
		dst_rect.height = height;
		  
		src_rect.x = 0;
		src_rect.y = 0;
		src_rect.width = width << 16;
		src_rect.height = height << 16;        

		dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
		dispman_update = vc_dispmanx_update_start( 0 );
		     
		dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
		  1/*layer*/, &dst_rect, 0/*src*/,
		  &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);
		  
		
	    EGLint num_config;


	    static const EGLint attribute_list[] =
	    {
	        EGL_RED_SIZE, 8,
	        EGL_GREEN_SIZE, 8,
	        EGL_BLUE_SIZE, 8,
	        EGL_ALPHA_SIZE, 8,
	        //EGL_LUMINANCE_SIZE, EGL_DONT_CARE,          //EGL_DONT_CARE
	        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	        EGL_SAMPLES,        2,
	        EGL_NONE
	    };


	    if( gl_info == 0 ) {
	    	gl_info = calloc(1, sizeof(struct stak_composer_gl_info_s));
	    }
	    EGLConfig config;
    	EGL_DISPMANX_WINDOW_T nativewindow;
#if 0
    	nativewindow.element = stak_rpc_buffer_get_int((struct stak_rpc_msgbuf*)&message, 0);
    	nativewindow.width = stak_rpc_buffer_get_int((struct stak_rpc_msgbuf*)&message, 1);
    	nativewindow.height = stak_rpc_buffer_get_int((struct stak_rpc_msgbuf*)&message, 2);
#else
		nativewindow.element = dispman_element;
		nativewindow.width = width;
		nativewindow.height = height;
		vc_dispmanx_update_submit_sync( dispman_update );
		static const EGLint s_configAttribs[] =
		{
			EGL_RED_SIZE,		8,
			EGL_GREEN_SIZE, 	8,
			EGL_BLUE_SIZE,		8,
			EGL_ALPHA_SIZE, 	8,
			EGL_LUMINANCE_SIZE, EGL_DONT_CARE,			//EGL_DONT_CARE
			EGL_SURFACE_TYPE,	EGL_WINDOW_BIT,
			EGL_SAMPLES,		1,
			EGL_NONE
		};
		EGLint numconfigs;

		gl_info->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		eglInitialize(gl_info->egl_display, NULL, NULL);
		assert(eglGetError() == EGL_SUCCESS);
		eglBindAPI(EGL_OPENVG_API);

		eglChooseConfig(gl_info->egl_display, s_configAttribs, &config, 1, &numconfigs);
		assert(eglGetError() == EGL_SUCCESS);
		assert(numconfigs == 1);

		gl_info->surface = eglCreateWindowSurface(gl_info->egl_display, config, &nativewindow, NULL);
		assert(eglGetError() == EGL_SUCCESS);
		gl_info->context = eglCreateContext(gl_info->egl_display, config, NULL, NULL);
		assert(eglGetError() == EGL_SUCCESS);
		eglMakeCurrent(gl_info->egl_display, gl_info->surface, gl_info->surface, gl_info->context);
		assert(eglGetError() == EGL_SUCCESS);
#endif

    	printf("Element: %i\n", nativewindow.element);
    	printf("Width: %i\n", nativewindow.width);
    	printf("Height: %i\n", nativewindow.height);
    	//gl_info->composer_layer = stak_rpc_buffer_get_int((struct stak_rpc_msgbuf*)&message, 3);
    	//printf("Layer: %i\n", gl_info->composer_layer);

	    // get an EGL display connection
	    /*gl_info->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	    if(gl_info->egl_display == EGL_NO_DISPLAY) {
	        printf("Error getting egl display\n");
	        return -1;
	    }

	    eglBindAPI(EGL_OPENVG_API);

	    // initialize the EGL display connection
	    if(eglInitialize(gl_info->egl_display, NULL, NULL) == EGL_FALSE) {
	        printf("Error initializing egl display\n");
	        return -1;
	    }
	    printf("Test %i\n", __LINE__);

	    // get an appropriate EGL frame buffer configuration
	    if(eglChooseConfig(gl_info->egl_display, attribute_list, &config, 1, &num_config) == EGL_FALSE){
	        printf("Error setting config\n");
	        return -1;
	    }
	    printf("Test %i\n", __LINE__);

	    // create an EGL surface
	    gl_info->surface = eglCreateWindowSurface( gl_info->egl_display, config, &nativewindow, NULL );
	    if(gl_info->surface <= 0) {
	        printf("Error creating surface\n");
	        return -1;
	    }
	    printf("Test %i\n", __LINE__);

	    // create an EGL rendering context
	    gl_info->context = eglCreateContext(gl_info->egl_display, config, NULL, NULL);
	    if(gl_info->context <= 0) {
	        printf("Error creating context\n");
	        return -1;
	    }
	    printf("Test %i\n", __LINE__);

	    if(eglSurfaceAttrib(gl_info->egl_display, gl_info->surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED) == EGL_FALSE) {
	        printf("Error preserving buffer\n");
	        return -1;
	    }
	    printf("Test %i\n", __LINE__);

	    // connect the context to the surface
	    if(eglMakeCurrent(gl_info->egl_display, gl_info->surface, gl_info->surface, gl_info->context) == EGL_FALSE) {
	        printf("Error making display current\n");
	        return -1;
	    }
	    printf("Test %i\n", __LINE__);

	    int error = eglGetError();
	    if(error != EGL_SUCCESS) {
	        printf("EGL Error occurred %i\n", error);
	        return -1;
	    }*/

	    return 0;
	}
}

int stak_rpc_composer_destroy_gl_context( struct stak_composer_gl_info_s* gl_info ) {
	// create a simple message to tell the daemon which function
	// we want to call.
	/*long message[] = {
		RPC_COMPOSER_DESTROY_GL_CONTEXT,
		1,
		gl_info->composer_layer
	};*/
	eglMakeCurrent( gl_info->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    eglDestroySurface( gl_info->egl_display, gl_info->surface );
    eglDestroyContext( gl_info->egl_display, gl_info->context );
    eglTerminate( gl_info->egl_display );
    eglReleaseThread();

	// send message to daemon
	return 0; //stak_rpc_message_send((void*)&message, 1);
}