#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>


#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <VG/openvg.h>
#include <VG/vgu.h>
#define NANOVG_GLES2_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>
#include <graphics/fbdev/fbdev.h>
#include <graphics/canvas/canvas.h>
#include <graphics/seps114a/seps114a.h>
#include <graphics/tinypng/TinyPngOut.h>
#include <io/bq27510/bq27510.h>
#define UNREF(X) ((void)(X))

#include "tiger.h"
// video core state
static stak_canvas_s canvas;
static framebuffer_device_s fb;
static stak_seps114a_s lcd_device;
static stak_bq27510_device_s gauge_device;
static volatile sig_atomic_t terminate = 0;
static NVGcontext* vg = NULL;
static float rotateN = 0.0f;

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

int shutdown()
{
    stak_canvas_destroy(&canvas);
    stak_seps114a_close(&lcd_device);
    //fbdev_close(&fb);
    return 0;
}

void term(int signum)
{
    printf("Terminating...\n");
    terminate = 1;
}

/*--------------------------------------------------------------*/

void render(int w, int h)
{
    float clearColor[4] = {1,1,1,1};
    nvgResetTransform(vg);
    nvgFillColor(vg, nvgRGBA(28,30,34,192));
    nvgStrokeColor(vg, nvgRGBA(0,0,0,32));
    nvgCircle(vg, w*0.5f, h*0.5f, 30.5f);
}

/*--------------------------------------------------------------*/

void init()
{
    vg = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
    if (vg == NULL) {
        printf("Could not init nanovg.\n");
        return -1;
    }
    glClearColor(1,1,1,1);
}

/*--------------------------------------------------------------*/

void deinit(void)
{
    nvgDeleteGLES2(vg);
}

int redraw()
{
    // start with a clear screen
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    nvgBeginFrame(vg, canvas.screen_width, canvas.screen_height, 1.0f);

    render(canvas.screen_width,canvas.screen_height);

    glEnable(GL_DEPTH_TEST);

    stak_canvas_swap(&canvas);
    stak_canvas_copy(&canvas, (char*)lcd_device.framebuffer, 96 * 2);
    return 0;
}

uint64_t get_time() {
    struct timespec timer;
    clock_gettime(CLOCK_MONOTONIC, &timer);
    return (uint64_t) (timer.tv_sec) * 1000000L + timer.tv_nsec / 1000L;
}
int write_buffer_to_png() {
    static char outbuffer[96*96*3];

    uint8_t r,g,b;
    uint16_t color;
    int x, y;

    for( y = 0 ; y < 96; y++) {
        for( x = 0 ; x < 96; x++ ) {
            color = lcd_device.framebuffer[y*96+x];
            r = ((color) & 0xF800) >> 8;
            g = ((color) & 0x7E0) >> 3;
            b = ((color) & 0x1F) << 3;
            outbuffer[y*(96*3)+x*3] = r;
            outbuffer[y*(96*3)+x*3+1] = g;
            outbuffer[y*(96*3)+x*3+2] = b;
            //printf("%4x ", color);
        }
        //printf("\n");
    }

    FILE *fout = fopen("output.png", "wb");
    struct TinyPngOut pngout;
    if (fout == NULL || TinyPngOut_init(&pngout, fout, 96, 96) != TINYPNGOUT_OK)
        goto error;
    
    // Write image data
    if (TinyPngOut_write(&pngout, (unsigned char*)outbuffer, 96 * 96) != TINYPNGOUT_OK)
        goto error;
    
    // Check for proper completion
    if (TinyPngOut_write(&pngout, NULL, 0) != TINYPNGOUT_DONE)
        goto error;
    fclose(fout);
    return 0;
    
error:
    printf("Error\n");
    if (fout != NULL)
        fclose(fout);
    return 1;
}
#define ZERO_OBJECT(_object) memset( &_object, 0, sizeof( _object ) );
int main(int argc, char **argv)
{
    setlogmask(LOG_UPTO(LOG_DEBUG));
    openlog("fbcp", LOG_NDELAY | LOG_PID, LOG_USER);
    syslog(LOG_DEBUG, "Starting fb output");
    uint64_t last_time, current_time, start_time, delta_time;
    delta_time = start_time = last_time = current_time = get_time();
    int frames_this_second = 0;
    int frames_per_second = 0;


    // setup sigterm handler
    struct sigaction action;
    ZERO_OBJECT( action );
    action.sa_handler = term;
    sigaction(SIGINT, &action, NULL);

    // clear application state
    ZERO_OBJECT( fb );
    ZERO_OBJECT( canvas );
    ZERO_OBJECT( lcd_device );
    ZERO_OBJECT( gauge_device );
    //memset( &fb, 0, sizeof( fb ) );
    //memset( &canvas, 0, sizeof( canvas ) );
    //memset( &lcd_device, 0, sizeof( lcd_device ) );
    //memset( &gauge_device, 0, sizeof( gauge_device ) );

    //fbdev_open("/dev/fb1", &fb);
    stak_seps114a_init(&lcd_device);
    stak_canvas_create(&canvas, STAK_CANVAS_OFFSCREEN, 96, 96);

    printf("Created canvas of size: %ix%i\n", canvas.screen_width, canvas.screen_height);

    init( );
    stak_bq27510_open("/dev/i2c-1", &gauge_device);
    stak_bq27510_close(&gauge_device);

    while (!terminate)
    {
        frames_this_second++;
        current_time = get_time();

        
        if(current_time > last_time + 1000000) {
            frames_per_second = frames_this_second;
            frames_this_second = 0;
            last_time = current_time;
            printf("FPS: %i\n", frames_per_second);
        }

        /*if(current_time > start_time + 30000000) {
            terminate = 1;
        }*/
        redraw();
        //write_buffer_to_png();
        stak_seps114a_update(&lcd_device);
        delta_time = (get_time() - current_time);
        //printf("delta_time: %i\n", delta_time);
        usleep(min(33000L, 33000L - max(0,delta_time)));

    }
    deinit();
    shutdown();
    return 0;
}