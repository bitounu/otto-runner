#ifndef OTTO_STAK_CANVAS_H
#define OTTO_STAK_CANVAS_H
#include <bcm_host.h>
#include <GLES/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
typedef struct {
    DISPMANX_DISPLAY_HANDLE_T display;
    DISPMANX_UPDATE_HANDLE_T update;
    DISPMANX_MODEINFO_T display_info;
    DISPMANX_RESOURCE_HANDLE_T opengl_resource;
    DISPMANX_RESOURCE_HANDLE_T scaled_resource;
    DISPMANX_ELEMENT_HANDLE_T opengl_element;
    VC_IMAGE_TRANSFORM_T transform;
    VC_RECT_T opengl_rect, scaled_rect, fb_rect;
    uint32_t opengl_ptr, scaled_ptr;
    uint32_t screen_width;
    uint32_t screen_height;
    // OpenGL|ES objects
    EGLDisplay egl_display;
    EGLSurface surface;
    EGLContext context;
    EGL_DISPMANX_WINDOW_T nativewindow;
}stak_canvas_s;

typedef enum {
	STAK_CANVAS_OFFSCREEN = 0x01,
	STAK_CANVAS_ONSCREEN = 0x02
}stak_canvas_flags;
stak_canvas_s* stak_canvas_create(stak_canvas_flags flags, uint32_t canvas_w, uint32_t canvas_h);
int stak_canvas_destroy(stak_canvas_s* canvas);
int stak_canvas_copy(stak_canvas_s* canvas, uint8_t* dst, uint32_t pitch);
int stak_canvas_swap(stak_canvas_s* canvas);

#endif