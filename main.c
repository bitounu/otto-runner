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
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "graphics/fbdev/fbdev.h"
#include "graphics/canvas/canvas.h"
#include "graphics/seps114a/seps114a.h"
#define UNREF(X) ((void)(X))

#include "tiger.h"
// video core state
static stak_canvas_s canvas;
static framebuffer_device_s fb;
static stak_seps114a_s lcd_device;
static volatile sig_atomic_t terminate = 0;
static float rotateN = 0.0f;

int shutdown()
{
    stak_canvas_destroy(&canvas);
    stak_seps114a_close(&lcd_device);
    //fbdev_close(&fb);
    return 0;
}

void term(int signum)
{
    terminate = 1;
}
typedef struct
{
    VGFillRule      m_fillRule;
    VGPaintMode     m_paintMode;
    VGCapStyle      m_capStyle;
    VGJoinStyle     m_joinStyle;
    float           m_miterLimit;
    float           m_strokeWidth;
    VGPaint         m_fillPaint;
    VGPaint         m_strokePaint;
    VGPath          m_path;
} PathData;

typedef struct
{
    PathData*           m_paths;
    int                 m_numPaths;
} PS;

PS* PS_construct(const char* commands, int commandCount, const float* points, int pointCount)
{
    PS* ps = (PS*)malloc(sizeof(PS));
    int p = 0;
    int c = 0;
    int i = 0;
    int paths = 0;
    int maxElements = 0;
    unsigned char* cmd;
    UNREF(pointCount);

    while(c < commandCount)
    {
        int elements, e;
        c += 4;
        p += 8;
        elements = (int)points[p++];
        assert(elements > 0);
        if(elements > maxElements)
            maxElements = elements;
        for(e=0;e<elements;e++)
        {
            switch(commands[c])
            {
            case 'M': p += 2; break;
            case 'L': p += 2; break;
            case 'C': p += 6; break;
            case 'E': break;
            default:
                assert(0);      //unknown command
            }
            c++;
        }
        paths++;
    }

    ps->m_numPaths = paths;
    ps->m_paths = (PathData*)malloc(paths * sizeof(PathData));
    cmd = (unsigned char*)malloc(maxElements);

    i = 0;
    p = 0;
    c = 0;
    while(c < commandCount)
    {
        int elements, startp, e;
        float color[4];

        //fill type
        int paintMode = 0;
        ps->m_paths[i].m_fillRule = VG_NON_ZERO;
        switch( commands[c] )
        {
        case 'N':
            break;
        case 'F':
            ps->m_paths[i].m_fillRule = VG_NON_ZERO;
            paintMode |= VG_FILL_PATH;
            break;
        case 'E':
            ps->m_paths[i].m_fillRule = VG_EVEN_ODD;
            paintMode |= VG_FILL_PATH;
            break;
        default:
            assert(0);      //unknown command
        }
        c++;

        //stroke
        switch( commands[c] )
        {
        case 'N':
            break;
        case 'S':
            paintMode |= VG_STROKE_PATH;
            break;
        default:
            assert(0);      //unknown command
        }
        ps->m_paths[i].m_paintMode = (VGPaintMode)paintMode;
        c++;

        //line cap
        switch( commands[c] )
        {
        case 'B':
            ps->m_paths[i].m_capStyle = VG_CAP_BUTT;
            break;
        case 'R':
            ps->m_paths[i].m_capStyle = VG_CAP_ROUND;
            break;
        case 'S':
            ps->m_paths[i].m_capStyle = VG_CAP_SQUARE;
            break;
        default:
            assert(0);      //unknown command
        }
        c++;

        //line join
        switch( commands[c] )
        {
        case 'M':
            ps->m_paths[i].m_joinStyle = VG_JOIN_MITER;
            break;
        case 'R':
            ps->m_paths[i].m_joinStyle = VG_JOIN_ROUND;
            break;
        case 'B':
            ps->m_paths[i].m_joinStyle = VG_JOIN_BEVEL;
            break;
        default:
            assert(0);      //unknown command
        }
        c++;

        //the rest of stroke attributes
        ps->m_paths[i].m_miterLimit = points[p++];
        ps->m_paths[i].m_strokeWidth = points[p++];

        //paints
        color[0] = points[p++];
        color[1] = points[p++];
        color[2] = points[p++];
        color[3] = 1.0f;
        ps->m_paths[i].m_strokePaint = vgCreatePaint();
        vgSetParameteri(ps->m_paths[i].m_strokePaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
        vgSetParameterfv(ps->m_paths[i].m_strokePaint, VG_PAINT_COLOR, 4, color);

        color[0] = points[p++];
        color[1] = points[p++];
        color[2] = points[p++];
        color[3] = 1.0f;
        ps->m_paths[i].m_fillPaint = vgCreatePaint();
        vgSetParameteri(ps->m_paths[i].m_fillPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
        vgSetParameterfv(ps->m_paths[i].m_fillPaint, VG_PAINT_COLOR, 4, color);

        //read number of elements

        elements = (int)points[p++];
        assert(elements > 0);
        startp = p;
        for(e=0;e<elements;e++)
        {
            switch( commands[c] )
            {
            case 'M':
                cmd[e] = VG_MOVE_TO | VG_ABSOLUTE;
                p += 2;
                break;
            case 'L':
                cmd[e] = VG_LINE_TO | VG_ABSOLUTE;
                p += 2;
                break;
            case 'C':
                cmd[e] = VG_CUBIC_TO | VG_ABSOLUTE;
                p += 6;
                break;
            case 'E':
                cmd[e] = VG_CLOSE_PATH;
                break;
            default:
                assert(0);      //unknown command
            }
            c++;
        }

        ps->m_paths[i].m_path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, (unsigned int)VG_PATH_CAPABILITY_ALL);
        vgAppendPathData(ps->m_paths[i].m_path, elements, cmd, points + startp);
        i++;
    }
    free(cmd);
    return ps;
}

void PS_destruct(PS* ps)
{
    int i;
    assert(ps);
    for(i=0;i<ps->m_numPaths;i++)
    {
        vgDestroyPaint(ps->m_paths[i].m_fillPaint);
        vgDestroyPaint(ps->m_paths[i].m_strokePaint);
        vgDestroyPath(ps->m_paths[i].m_path);
    }
    free(ps->m_paths);
    free(ps);
}

void PS_render(PS* ps)
{
    int i;
    //assert(ps);
    vgSeti(VG_BLEND_MODE, VG_BLEND_SRC_OVER);

    for(i=0;i<ps->m_numPaths;i++)
    {
        vgSeti(VG_FILL_RULE, ps->m_paths[i].m_fillRule);
        vgSetPaint(ps->m_paths[i].m_fillPaint, VG_FILL_PATH);

        if(ps->m_paths[i].m_paintMode & VG_STROKE_PATH)
        {
            vgSetf(VG_STROKE_LINE_WIDTH, ps->m_paths[i].m_strokeWidth);
            vgSeti(VG_STROKE_CAP_STYLE, ps->m_paths[i].m_capStyle);
            vgSeti(VG_STROKE_JOIN_STYLE, ps->m_paths[i].m_joinStyle);
            vgSetf(VG_STROKE_MITER_LIMIT, ps->m_paths[i].m_miterLimit);
            vgSetPaint(ps->m_paths[i].m_strokePaint, VG_STROKE_PATH);
        }

        vgDrawPath(ps->m_paths[i].m_path, ps->m_paths[i].m_paintMode);
    }
    //assert(vgGetError() == VG_NO_ERROR);
}

PS* tiger = NULL;

/*--------------------------------------------------------------*/

void render(int w, int h)
{
        float clearColor[4] = {1,1,1,1};
        float scale = w / (tigerMaxX - tigerMinX);
        rotateN += 1.0f;

        vgSetfv(VG_CLEAR_COLOR, 4, clearColor);
        vgClear(0, 0, w, h);

        vgLoadIdentity();
        vgTranslate(w*0.5f, h*0.5f);
        vgRotate(rotateN);
        vgTranslate(-w*0.5f, -h*0.5f);
        vgScale(scale, scale);
        vgTranslate(-tigerMinX, -tigerMinY + 0.5f * (h / scale - (tigerMaxY - tigerMinY)));

        PS_render(tiger);
        //assert(vgGetError() == VG_NO_ERROR);

}

/*--------------------------------------------------------------*/

void init()
{
    tiger = PS_construct(tigerCommands, tigerCommandCount, tigerPoints, tigerPointCount);
}

/*--------------------------------------------------------------*/

void deinit(void)
{
    PS_destruct(tiger);
}

int redraw()
{
    // start with a clear screen
    glClear( GL_COLOR_BUFFER_BIT );

    render(canvas.screen_width,canvas.screen_height);
    stak_canvas_swap(&canvas);
    stak_canvas_copy(&canvas, (char*)lcd_device.framebuffer, 96 * 16 / 8);
    //usleep(25 * 1000);
    return 0;
}

uint64_t get_time() {
    struct timespec timer;
    clock_gettime(CLOCK_MONOTONIC, &timer);
    return (uint64_t) (timer.tv_sec) * 1000000L + timer.tv_nsec / 1000L;
}
int main(int argc, char **argv)
{
    setlogmask(LOG_UPTO(LOG_DEBUG));
    openlog("fbcp", LOG_NDELAY | LOG_PID, LOG_USER);
    syslog(LOG_DEBUG, "Starting fb output");
    uint64_t last_time, current_time;
    current_time = get_time();
    last_time = current_time;
    int frames_this_second = 0;
    int frames_per_second = 0;


    // setup sigterm handler
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);

    // clear application state
    memset( &fb, 0, sizeof( fb ) );
    memset( &canvas, 0, sizeof( canvas ) );
    memset( &lcd_device, 0, sizeof( lcd_device ) );

    //fbdev_open("/dev/fb1", &fb);
    stak_seps114a_init(&lcd_device);
    stak_canvas_create(&canvas, STAK_CANVAS_OFFSCREEN, 96, 96);

    printf("Created canvas of size: %ix%i\n", canvas.screen_width, canvas.screen_height);

    init( );

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

        redraw();
        stak_seps114a_update(&lcd_device);

    }
    deinit();
    shutdown();
    return 0;
}