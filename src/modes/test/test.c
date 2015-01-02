#include <apis/composer/composer.h>
#include <application/rpc/rpc.h>
#include <lib/shapes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// prototypes
int init();
int shutdown();
int update();
const int WIDTH = 128;
const int HEIGHT = 128;

static struct stak_composer_gl_info_s gl_info;
#define NUM_PARTICLES 50

typedef struct particle {
    double x, y;
    double vx, vy;
    int r, g, b;
    int radius;
} particle_t;

particle_t particles[NUM_PARTICLES];

int showTrails = 1;
int directionRTL = 0;
int alternate = 1;
double gravity = 0.5;

// Initialize _all_ the particles
void initParticles(int w, int h) {
    int i;
    for (i = 0; i < NUM_PARTICLES; i++) {
        particle_t *p = &particles[i];

        p->x = 48.0;
        p->y = 48.0;
        p->vx = (rand() % 6) - 3;
        p->vy = (rand() % 6) - 3;
        p->r = rand() % 256;
        p->g = rand() % 256;
        p->b = rand() % 256;
        p->radius = (rand() % 5) + 5;

        if (directionRTL) {
            p->vx *= -1.0;
            p->x = w;
        }
    }
}
void paintBG(int w, int h) {

    Fill(0, 0, 0, 1);
    Rect(0, 0, w, h);
}
void draw_particles(int w, int h) {
    int i;
    particle_t *p;

    paintBG(w, h);

    for (i = 0; i < NUM_PARTICLES; i++) {
        p = &particles[i];

        Fill(p->r, p->g, p->b, 1.0);
        Circle(p->x, p->y, p->radius);

        // Apply the velocity
        p->x += p->vx;
        p->y += p->vy;

        p->vx *= 0.98;
        if (p->vy > 0.0)
            p->vy *= 0.97;

        // Gravity
        p->vy -= gravity;

        // Stop particles leaving the canvas  
        if (p->x < -4.0)
            p->x = w + 4.0;
        if (p->x > w + 4.0)
            p->x = -4.0;

        // When particle reaches the bottom of screen reset velocity & start posn
        if (p->y < -4.0) {
            p->x = 0.0;
            p->y = 0.0;
            p->vx = (rand() % 12) - 6;
            p->vy = (rand() % 10);

            if (directionRTL) {
                p->vx *= -1.0;
                p->x = w;
            }
        }

        if (p->y > h + 4.0)
            p->y = -4.0;
    }
}

int init() {
    printf("Testing.\n");
    bcm_host_init();

    stak_rpc_init();

    int context = stak_rpc_composer_create_gl_context(&gl_info, WIDTH, HEIGHT);
    //printf("Layer: %i\n", gl_info->composer_layer);

    // set up screen ratio
    glViewport(0, 0, (GLsizei) WIDTH, (GLsizei) HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float ratio = (float)WIDTH / (float)HEIGHT;
    glFrustumf(-ratio, ratio, -1.0f, 1.0f, 1.0f, 10.0f);

    initParticles(WIDTH, HEIGHT);


    printf("Display: %i\n", gl_info.egl_display);
    printf("Surface: %i\n", gl_info.surface);
    printf("Context: %i\n", gl_info.context);

    return 0;
}

int shutdown() {
    printf("Shutting down...\n");
	uint32_t error = vgGetError();
    if( error != VG_NO_ERROR ) {
    	printf("VG Error occurred: %i\n", error);
        return -1;
    }
    error = eglGetError();
    if(error != EGL_SUCCESS) {
	   	printf("EGL Error occurred %i\n", error);
	    return -1;
	}
	stak_rpc_composer_destroy_gl_context( &gl_info );
    return 0;
}


int update() {
    eglSwapBuffers( gl_info.egl_display, gl_info.surface );
	float clearColor[4] = {1,1,1,1};
	vgSetfv(VG_CLEAR_COLOR, 4, clearColor);
	vgClear(0, 0, WIDTH, HEIGHT);
	vgLoadIdentity();

    uint32_t error = vgGetError();
    if( error != VG_NO_ERROR ) {
        printf("VG Error occurred: %i\n", error);
        return -1;
    }
    error = eglGetError();
    if(error != EGL_SUCCESS) {
        printf("EGL Error occurred %i\n", error);
        return -1;
    }

    Start(WIDTH, HEIGHT);
    draw_particles(WIDTH, HEIGHT);
    vgFlush();
    
    return 0;
}
