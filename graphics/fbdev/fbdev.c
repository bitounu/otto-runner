#include "fbdev.h"
#include <fcntl.h>
#include <unistd.h> // for close
#include <syslog.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

int fbdev_open(char* filename, framebuffer_device_s* device){
    device->fhandle = open("/dev/fb1", O_RDWR);
    if (device->fhandle == -1)
    {
        syslog(LOG_ERR, "Unable to open secondary display");
        return -1;
    }
    if (ioctl(device->fhandle, FBIOGET_FSCREENINFO, &device->finfo))
    {
        syslog(LOG_ERR, "Unable to get secondary display information");
        return -1;
    }
    if (ioctl(device->fhandle, FBIOGET_VSCREENINFO, &device->vinfo))
    {
        syslog(LOG_ERR, "Unable to get secondary display information");
        return -1;
    }
    device->buf = (char *) mmap(0, device->finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, device->fhandle, 0);
    if (device->buf <= 0)
    {
        syslog(LOG_ERR, "Unable to create mamory mapping");
        close(device->fhandle);
        return -1;
    }
    else return 0;
}
int fbdev_close(framebuffer_device_s* device){
    munmap(device->buf, device->finfo.smem_len);
    close(device->fhandle);
    return 0;
}