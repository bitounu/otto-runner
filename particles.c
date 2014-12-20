#include <lib/shapes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// prototypes
int init();
int shutdown();
int redraw();


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
double gravity = 1.0;

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
    initParticles(96, 96);
    return 0;
}

int shutdown() {
    return 0;
}


int draw() {
    Start(96,96);
    draw_particles(96,96);
    Fill(255, 255, 255, 0.75);
    TextMid(96 / 2, 96 * 0.6, "OT", MonoTypeface, 96 / 8);
    TextMid(96 / 2, 96 * 0.8, "TO", MonoTypeface, 96 / 8);
    return 0;
}
