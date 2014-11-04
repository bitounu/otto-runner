#ifndef OTTO_FBDEV_H
#define OTTO_FBDEV_H
#include <linux/fb.h>

typedef struct {
    int fhandle;
	struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    char *buf;
} framebuffer_device_s;

int fbdev_open(char* filename, framebuffer_device_s* device);
int fbdev_close(framebuffer_device_s* device);
#endif