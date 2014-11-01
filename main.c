#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "bcm_host.h"

#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#define RENDER_WINDOW_ONSCREEN


// video core state
typedef struct
{
    DISPMANX_DISPLAY_HANDLE_T display;
    DISPMANX_UPDATE_HANDLE_T update;
    DISPMANX_MODEINFO_T display_info;
    DISPMANX_RESOURCE_HANDLE_T opengl_resource;
    DISPMANX_RESOURCE_HANDLE_T scaled_resource;
    DISPMANX_ELEMENT_HANDLE_T opengl_element;
    uint32_t opengl_ptr, scaled_ptr;
    VC_IMAGE_TRANSFORM_T transform;
    VC_RECT_T opengl_rect, scaled_rect, fb_rect;
} VIDEOCORE_STATE_T;

// fbdev state
typedef struct
{
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    int fbfd;
    char *fbp;
} FRAMEBUFFER_STATE_T;

// opengl state
typedef struct
{
    uint32_t screen_width;
    uint32_t screen_height;
    // OpenGL|ES objects
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
} GL_STATE_T;


static GL_STATE_T gl;
static VIDEOCORE_STATE_T dispman;
static FRAMEBUFFER_STATE_T fb;
static volatile sig_atomic_t terminate = 0;
static EGL_DISPMANX_WINDOW_T nativewindow;

int shutdown()
{
    // clear screen
    glClear( GL_COLOR_BUFFER_BIT );
    eglSwapBuffers(gl.display, gl.surface);

    // Release OpenGL resources
    eglMakeCurrent( gl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    eglDestroySurface( gl.display, gl.surface );
    eglDestroyContext( gl.display, gl.context );
    eglTerminate( gl.display );
    close(fb.fbfd);
    vc_dispmanx_resource_delete(dispman.opengl_resource);
    vc_dispmanx_resource_delete(dispman.scaled_resource);
    vc_dispmanx_display_close(dispman.display);
    munmap(fb.fbp, fb.finfo.smem_len);
    close(fb.fbfd);
    return 0;
}

int init_videocore()
{
#ifndef RENDER_WINDOW_ONSCREEN
    // open offscreen display and 512x512 resource
    dispman.display = vc_dispmanx_display_open_offscreen( dispman.opengl_resource, DISPMANX_NO_ROTATE);
    dispman.opengl_resource = vc_dispmanx_resource_create(VC_IMAGE_RGB565, 512, 512, &dispman.opengl_ptr);
    if (!dispman.opengl_resource)
    {
        syslog(LOG_ERR, "Unable to create OpenGL screen buffer");
        shutdown();
        return -1;
    }
#else
    // open main display
    dispman.display = vc_dispmanx_display_open( 0 );
    dispman.opengl_resource = 0;
#endif

    dispman.scaled_resource = vc_dispmanx_resource_create(VC_IMAGE_RGB565, 96, 96, &dispman.scaled_ptr);
    if (!dispman.scaled_resource)
    {
        syslog(LOG_ERR, "Unable to create LCD output buffer");
        shutdown();
        return -1;
    }

    dispman.update = vc_dispmanx_update_start( dispman.opengl_resource );

    graphics_get_display_size(dispman.opengl_resource , &gl.screen_width, &gl.screen_height);

    vc_dispmanx_rect_set( &dispman.opengl_rect, 0, 0, gl.screen_width, gl.screen_height );
    vc_dispmanx_rect_set( &dispman.scaled_rect, 0, 0, gl.screen_width << 16, gl.screen_height << 16 );
    vc_dispmanx_rect_set( &dispman.fb_rect, 0, 0, fb.vinfo.xres, fb.vinfo.yres );

    VC_DISPMANX_ALPHA_T alpha = { DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 255, 0 };

    dispman.opengl_element = vc_dispmanx_element_add(dispman.update, dispman.display, 0 , &dispman.opengl_rect,
                             dispman.opengl_resource, &dispman.scaled_rect,
                             DISPMANX_PROTECTION_NONE, &alpha, 0, DISPMANX_NO_ROTATE);

    assert(dispman.opengl_element != 0);

    nativewindow.element = dispman.opengl_element;
    nativewindow.width = gl.screen_width;
    nativewindow.height = gl.screen_height;
    vc_dispmanx_update_submit_sync( dispman.update );
    return 0;
}

int init_opengl()
{

    EGLBoolean result;
    EGLint num_config;


    static const EGLint attribute_list[] =
    {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };

    EGLConfig config;


    // get an EGL display connection
    gl.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(gl.display != EGL_NO_DISPLAY);

    // initialize the EGL display connection
    result = eglInitialize(gl.display, NULL, NULL);
    assert(EGL_FALSE != result);

    // get an appropriate EGL frame buffer configuration
    result = eglChooseConfig(gl.display, attribute_list, &config, 1, &num_config);
    assert(EGL_FALSE != result);

    // create an EGL rendering context
    gl.context = eglCreateContext(gl.display, config, EGL_NO_CONTEXT, NULL);
    assert(gl.context != EGL_NO_CONTEXT);

    result = graphics_get_display_size(dispman.opengl_resource , &gl.screen_width, &gl.screen_height);

    // create an EGL surface
    gl.surface = eglCreateWindowSurface( gl.display, config, &nativewindow, NULL );

    // connect the context to the surface
    result = eglMakeCurrent(gl.display, gl.surface, gl.surface, gl.context);
    assert(EGL_FALSE != result);

    // set background color and clear buffers
    glClearColor(1.f, 1.f, 0.35f, 1.0f);

    // enable back face culling.
    glEnable(GL_CULL_FACE);

    glMatrixMode(GL_MODELVIEW);

    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );

    glViewport(0, 0, (GLsizei)512, (GLsizei)512);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    return 0;
}

int open_fbdev()
{
    fb.fbfd = open("/dev/fb1", O_RDWR);
    if (fb.fbfd == -1)
    {
        syslog(LOG_ERR, "Unable to open secondary display");
        shutdown();
        return -1;
    }
    if (ioctl(fb.fbfd, FBIOGET_FSCREENINFO, &fb.finfo))
    {
        syslog(LOG_ERR, "Unable to get secondary display information");
        shutdown();
        return -1;
    }
    if (ioctl(fb.fbfd, FBIOGET_VSCREENINFO, &fb.vinfo))
    {
        syslog(LOG_ERR, "Unable to get secondary display information");
        shutdown();
        return -1;
    }
    fb.fbp = (char *) mmap(0, fb.finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb.fbfd, 0);
    if (fb.fbp <= 0)
    {
        syslog(LOG_ERR, "Unable to create mamory mapping");
        close(fb.fbfd);
        shutdown();
        return -1;
    }
    return 0;
}

void term(int signum)
{
    terminate = 1;
}


int redraw()
{
    // start with a clear screen
    glClear( GL_COLOR_BUFFER_BIT );

    eglSwapBuffers(gl.display, gl.surface);
    vc_dispmanx_snapshot(dispman.display, dispman.scaled_resource, 0);
    vc_dispmanx_resource_read_data(dispman.scaled_resource, &dispman.fb_rect, fb.fbp, fb.vinfo.xres * fb.vinfo.bits_per_pixel / 8);
    usleep(25 * 1000);
    return 0;
}



int main(int argc, char **argv)
{
    setlogmask(LOG_UPTO(LOG_DEBUG));
    openlog("fbcp", LOG_NDELAY | LOG_PID, LOG_USER);
    syslog(LOG_DEBUG, "Starting fb output");
    bcm_host_init();

    // setup sigterm handler
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);

    // clear application state
    memset( &gl, 0, sizeof( gl ) );
    memset( &fb, 0, sizeof( fb ) );
    memset( &dispman, 0, sizeof( dispman ) );

    open_fbdev();
    init_videocore();
    init_opengl();
    while (!terminate)
    {
        redraw();
    }
    shutdown();
    return 0;
}