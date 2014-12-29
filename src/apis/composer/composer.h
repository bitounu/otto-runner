#ifndef STAK_API_COMPOSER_H
#define STAK_API_COMPOSER_H
#include <application/rpc/rpc.h>
#include <GLES/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

// list of all RPC calls for input api.
enum stak_rpc_input_calls {
	RPC_COMPOSER_START = 256,					// each api should have a unique starting value
	RPC_DEFINE_ID(COMPOSER_CREATE_GL_CONTEXT),
	RPC_DEFINE_ID(COMPOSER_DESTROY_GL_CONTEXT),
	RPC_COMPOSER_END
};

struct stak_composer_gl_info_s{
    // OpenGL|ES objects
    EGLDisplay egl_display;
    EGLSurface surface;
    EGLContext context;
    uint32_t composer_layer;
};

int stak_rpc_composer_create_gl_context( struct stak_composer_gl_info_s* gl_info, int width, int height );
int stak_rpc_composer_destroy_gl_context( struct stak_composer_gl_info_s* );
#endif