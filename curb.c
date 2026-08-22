/* Does the car stop at a low obstacle, or climb through it?
 *
 * Reported as "collision missed on rock inside a cave on map 1, and gas station
 * map 2 around gas pumps curb". Both are LOW, roughly wheel-height obstacles,
 * and no fixture in this project had ever driven at one: allstarts drives on
 * open ground and says so in its own header ("the port has no wall collision"),
 * ceiling.c drives UNDER things, flipped.c turns the car over. The state
 * "car meets a kerb" was not reachable by any harness, which is exactly the gap
 * the ceiling bug taught us to look for first.
 *
 * Two halves:
 *
 *   part 1  a SYNTHETIC step of increasing height on flat ground, all three
 *           cars. The threshold is the number that matters: below it the car
 *           climbs, above it the car is stopped.
 *   part 2  the REAL reported spot -- beach_2's gas-station kerb, which is
 *           mesh pCylinder1 at y 5.23..5.46 over asphalt at 5.22..5.36, i.e.
 *           10 to 24 cm proud.
 *   part 4  a SMALL ROCK, and the car's ATTITUDE rather than its position.
 *
 * Part 4 exists because parts 1 to 3 print where the car ENDED UP and say
 * nothing about which way up it is. Reported as "in the real game the car
 * reacts softer on collision with small height obstacles like small rocks; in
 * the port it flips over on the same obstacles" -- and part 1 had been watching
 * that happen for as long as it existed. A 2 cm step, 28% of an Overkill wheel,
 * put the Buggy through 179.8 degrees of tilt, and part 1 called it "stopped"
 * and moved on, because z < STEP_Z is all it asked. A fixture that reaches the
 * state and does not judge it is not coverage; this is the third time that
 * lesson has been paid for here (rockroll flipped a car and asserted nothing,
 * ceiling.c could not build the state at all).
 *
 * Run from rccars_vita/ with assets/ populated.
 *
 *   gcc -I. -O2 -fno-fast-math -ffp-contract=off ../rccars_re/curb.c \
 *       col.c rb.c rbcar.c contact.c collide.c carani.c -lm -o curb
 */
#include "col.h"
#include "rbcar.h"
#include "rb_data.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define DT (1.0f / 60.0f)

static int nfail;

static void ck(int ok, const char *what, const char *fmt, ...)
{
    va_list ap;
    printf("  %-4s %-52s ", ok ? "ok" : "FAIL", what);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
    if (!ok)
        nfail++;
}

/* ---------------------------------------------------------------- part 1 --
 * A world that is flat ground at y = 0 with a vertical step at z = STEP_Z.
 * Ground beyond the step is the step's own top, so the car CAN legitimately end
 * up on top of it -- climbing is a real outcome, not a failure, and the point of
 * the sweep is where that stops being possible.
 */
static float g_step_h;
#define STEP_Z 3.0f

static int w_ground(void *u, float x, float z, float ceil_y,
                    float *y, float n[3])
{
    (void)u; (void)x; (void)ceil_y;
    *y = (z >= STEP_Z) ? g_step_h : 0.f;
    n[0] = 0.f; n[1] = 1.f; n[2] = 0.f;
    return 1;
}

/* Closest point on {flat ground} U {the step's vertical face} U {its top}. */
static int w_sphere(void *u, const float c[3], float r,
                    rb_world_hit *hits, int max_hits, int *n)
{
    int k = 0;
    (void)u;
    *n = 0;
    if (max_hits <= 0)
        return 0;
    /* the ground the car is on */
    {
        float gy = (c[2] >= STEP_Z) ? g_step_h : 0.f;
        if (c[1] - gy <= r && k < max_hits) {
            hits[k].point[0] = c[0]; hits[k].point[1] = gy; hits[k].point[2] = c[2];
            hits[k].surface = 0;
            hits[k].normal[0] = 0.f; hits[k].normal[1] = 1.f; hits[k].normal[2] = 0.f;
            k++;
        }
    }
    /* the step's vertical face at z = STEP_Z, spanning y in [0, step_h] */
    if (g_step_h > 0.f && k < max_hits) {
        float py = c[1];
        if (py < 0.f) py = 0.f;
        if (py > g_step_h) py = g_step_h;
        {
            float dz = c[2] - STEP_Z, dy = c[1] - py;
            if (dz * dz + dy * dy <= r * r && c[2] < STEP_Z) {
                hits[k].point[0] = c[0];
                hits[k].point[1] = py;
                hits[k].point[2] = STEP_Z;
                hits[k].surface = 0;
                hits[k].normal[0] = 0.f;
                hits[k].normal[1] = 0.f;
                hits[k].normal[2] = -1.f;   /* faces back toward the car */
                k++;
            }
        }
    }
    *n = k;
    return k;
}

static int w_segment(void *u, const float a[3], const float b[3])
{
    (void)u;
    return (a[2] < STEP_Z) != (b[2] < STEP_Z) && g_step_h > 0.f;
}

static rb_world step_world(void)
{
    rb_world w;
    memset(&w, 0, sizeof(w));
    w.ground = w_ground;
    w.sphere = w_sphere;
    w.segment = w_segment;
    return w;
}

/* Drive at the step from z = 0 and report where the car ends up. */
static float drive_at_step(int car, float h, float *out_z, float *out_y)
{
    rb_car c;
    rb_world w = step_world();
    int i;
    g_step_h = h;
    rbcar_init(&c, car, &w, 0.f, 0.2f, 0.f, 0.f);   /* +Z is forward */
    for (i = 0; i < 120; i++)
        rbcar_step(&c, 0.f, 0.f, 0.f, 0, DT);
    for (i = 0; i < 360; i++)
        rbcar_step(&c, 1.f, 0.f, 0.f, 0, DT);
    *out_z = c.body.x[2];
    *out_y = c.body.x[1];
    return c.body.x[2];
}

/* ------------------------------------------------------------- part 3 --
 * A KERB: a block of finite thickness standing on flat ground, with flat
 * ground beyond it. Unlike part 1's step, "inside" is a state that exists
 * here and is never correct -- the car may stop short or ride over, but its
 * body must not come to rest within the block's volume.
 *
 * This is the state the hardware screenshot caught and that no fixture in the
 * project could previously build. col_sphere is a closest-point query, so once
 * a body sphere is deeper inside the block than its own radius it reports
 * nothing at all, `emb` reads 0 and the car rests in the concrete.
 */
#define BLK_Z0 3.0f
#define BLK_Z1 6.0f
static float g_blk_h = 0.23f;

static int b_ground(void *u, float x, float z, float ceil_y, float *y, float n[3])
{
    (void)u; (void)x; (void)ceil_y;
    *y = (z > BLK_Z0 && z < BLK_Z1) ? g_blk_h : 0.f;
    n[0] = 0.f; n[1] = 1.f; n[2] = 0.f;
    return 1;
}

/* Closest point on the block's box plus the ground plane. */
static int b_sphere(void *u, const float c[3], float r,
                    rb_world_hit *hits, int max_hits, int *n)
{
    int k = 0;
    (void)u;
    *n = 0;
    if (c[1] - 0.f <= r && k < max_hits) {          /* the ground */
        hits[k].point[0] = c[0]; hits[k].point[1] = 0.f; hits[k].point[2] = c[2];
        hits[k].surface = 0;
        hits[k].normal[0] = 0.f; hits[k].normal[1] = 1.f; hits[k].normal[2] = 0.f;
        k++;
    }
    if (k < max_hits) {                              /* the block */
        float qy = c[1] < 0.f ? 0.f : (c[1] > g_blk_h ? g_blk_h : c[1]);
        float qz = c[2] < BLK_Z0 ? BLK_Z0 : (c[2] > BLK_Z1 ? BLK_Z1 : c[2]);
        float dy = c[1] - qy, dz = c[2] - qz;
        if (dy * dy + dz * dz <= r * r) {
            /* nearest face decides the outward normal */
            hits[k].point[0] = c[0]; hits[k].point[1] = qy; hits[k].point[2] = qz;
            hits[k].surface = 0;
            hits[k].normal[0] = 0.f;
            if (c[1] > g_blk_h) {
                hits[k].normal[1] = 1.f; hits[k].normal[2] = 0.f;
            } else {
                hits[k].normal[1] = 0.f;
                hits[k].normal[2] = (c[2] < BLK_Z0) ? -1.f : 1.f;
            }
            k++;
        }
    }
    *n = k;
    return k;
}

/* Nonzero if a->b crosses the block's surface. */
static int b_segment(void *u, const float a[3], const float b[3])
{
    int ai, bi;
    (void)u;
    ai = (a[2] > BLK_Z0 && a[2] < BLK_Z1 && a[1] < g_blk_h);
    bi = (b[2] > BLK_Z0 && b[2] < BLK_Z1 && b[1] < g_blk_h);
    if (ai != bi)
        return 1;
    /* straight through, in one side and out the other */
    if (!ai && !bi && ((a[2] < BLK_Z0) != (b[2] < BLK_Z0))
        && (a[1] < g_blk_h || b[1] < g_blk_h))
        return 1;
    return 0;
}

/* ---------------------------------------------------------------- part 4 --
 * A rock: a box of finite extent in BOTH horizontal axes, so it can be met
 * square on (the whole axle at once, which pitches) or one-sided (one pair of
 * wheels, which rolls). Part 1's step spans the world and can only pitch.
 */
static float RK_H = 0.03f;
static float RK_Z0 = 3.00f, RK_Z1 = 3.30f;
static float RK_X0 = -9.0f, RK_X1 = 9.0f;

static int rk_in(float x, float z)
{
    return x > RK_X0 && x < RK_X1 && z > RK_Z0 && z < RK_Z1;
}

static int rk_ground(void *u, float x, float z, float ceil_y, float *y, float n[3])
{
    (void)u; (void)ceil_y;
    *y = rk_in(x, z) ? RK_H : 0.f;
    n[0] = 0.f; n[1] = 1.f; n[2] = 0.f;
    return 1;
}

static int rk_sphere(void *u, const float c[3], float r,
                     rb_world_hit *hits, int max_hits, int *n)
{
    int k = 0;
    (void)u;
    *n = 0;
    if (max_hits <= 0)
        return 0;
    if (c[1] <= r) {                                  /* the ground plane */
        hits[k].point[0] = c[0]; hits[k].point[1] = 0.f; hits[k].point[2] = c[2];
        hits[k].surface = 0;
        hits[k].normal[0] = 0.f; hits[k].normal[1] = 1.f; hits[k].normal[2] = 0.f;
        k++;
    }
    if (k < max_hits && RK_H > 0.f) {                 /* the rock */
        float qx = c[0] < RK_X0 ? RK_X0 : (c[0] > RK_X1 ? RK_X1 : c[0]);
        float qy = c[1] < 0.f   ? 0.f   : (c[1] > RK_H  ? RK_H  : c[1]);
        float qz = c[2] < RK_Z0 ? RK_Z0 : (c[2] > RK_Z1 ? RK_Z1 : c[2]);
        float dx = c[0] - qx, dy = c[1] - qy, dz = c[2] - qz;
        if (dx*dx + dy*dy + dz*dz <= r*r) {
            hits[k].point[0] = qx; hits[k].point[1] = qy; hits[k].point[2] = qz;
            hits[k].surface = 0;
            hits[k].normal[0] = 0.f; hits[k].normal[1] = 0.f; hits[k].normal[2] = 0.f;
            if (c[1] > RK_H)       hits[k].normal[1] =  1.f;
            else if (c[2] < RK_Z0) hits[k].normal[2] = -1.f;
            else if (c[2] > RK_Z1) hits[k].normal[2] =  1.f;
            else if (c[0] < RK_X0) hits[k].normal[0] = -1.f;
            else                   hits[k].normal[0] =  1.f;
            k++;
        }
    }
    *n = k;
    return k;
}

static int rk_segment(void *u, const float a[3], const float b[3])
{
    int i;
    (void)u;
    if (RK_H <= 0.f)
        return 0;
    for (i = 0; i <= 8; i++) {           /* sampled: the box is small */
        float t = i / 8.0f;
        float x = a[0] + t*(b[0]-a[0]);
        float y = a[1] + t*(b[1]-a[1]);
        float z = a[2] + t*(b[2]-a[2]);
        if (rk_in(x, z) && y < RK_H)
            return 1;
    }
    return 0;
}

/* Drive at the rock at full throttle and return the WORST tilt reached, in
   degrees from world up. */
static float rock_tilt(int car, float h, float x0, float x1)
{
    rb_car c;
    rb_world w;
    int i;
    float worst = 0.f;

    memset(&w, 0, sizeof(w));
    w.ground = rk_ground; w.sphere = rk_sphere; w.segment = rk_segment;
    RK_H = h; RK_X0 = x0; RK_X1 = x1;

    rbcar_init(&c, car, &w, 0.f, 0.2f, 0.f, 0.f);
    for (i = 0; i < 120; i++)                 /* settle */
        rbcar_step(&c, 0.f, 0.f, 0.f, 0, DT);
    for (i = 0; i < 300; i++) {
        double d;
        rbcar_step(&c, 1.f, 0.f, 0.f, 0, DT);
        d = c.m[5];                           /* row 1 . world up */
        if (d >  1.0) d =  1.0;
        if (d < -1.0) d = -1.0;
        d = acos(d) * 57.295776;
        if (d > worst)
            worst = (float)d;
    }
    return worst;
}

int main(void)
{
    static const char *names[3] = { "Overkill", "Buggy", "Hummer" };
    int car;

    puts("== part 1: a vertical step on flat ground ==");
    puts("   the car starts 3 m short and drives at it for 6 s\n");
    for (car = 0; car < 3; car++) {
        float h;
        float climbed_max = 0.f, stopped_min = 1e9f;
        printf("  %s\n", names[car]);
        for (h = 0.02f; h <= 0.42f; h += 0.02f) {
            float z, y;
            const char *verdict;
            drive_at_step(car, h, &z, &y);
            /* past the step and up on it = climbed; short of it = stopped;
               past it but still low = went THROUGH, which is the bug */
            if (z < STEP_Z) {
                verdict = "stopped";
                if (h < stopped_min) stopped_min = h;
            } else if (y > h * 0.5f) {
                verdict = "climbed";
                if (h > climbed_max) climbed_max = h;
            } else {
                verdict = "THROUGH";
            }
            printf("     step %5.3f m -> z %6.2f  y %6.3f   %s\n",
                   h, z, y, verdict);
        }
        printf("     highest climbed %.3f m, lowest stopped %.3f m\n\n",
               climbed_max, stopped_min < 1e8f ? stopped_min : 0.f);
    }

    /* -------------------------------------------------------- part 2 -- */
    {
        col_t col;
        rb_car c;
        int i;
        float z0, x0;
        puts("== part 2: beach_2's gas-station kerb, on the real grid ==");
        if (!col_load("assets/beach_2.col", &col)) {
            ck(0, "beach_2.col loads", "");
            return nfail ? 1 : 0;
        }
        /* the kerb is pCylinder1, x 36.77..51.17, y 5.23..5.46, z 11.10..17.13,
           on asphalt at 5.22..5.36. Approach it from -Z at its middle. */
        rbcar_init(&c, 0, col_rb_world(&col), 44.0f, 5.6f, 6.5f, 0.f);
        for (i = 0; i < 120; i++)
            rbcar_step(&c, 0.f, 0.f, 0.f, 0, DT);
        z0 = c.body.x[2]; x0 = c.body.x[0];
        printf("  parked at (%.2f, %.3f, %.2f)\n", x0, c.body.x[1], z0);
        for (i = 0; i < 420; i++)
            rbcar_step(&c, 1.f, 0.f, 0.f, 0, DT);
        printf("  after 7 s at full throttle: (%.2f, %.3f, %.2f)\n",
               c.body.x[0], c.body.x[1], c.body.x[2]);
        printf("  the kerb top is 5.46, the asphalt 5.22-5.36\n");
        col_free(&col);
    }

    /* -------------------------------------------------------- part 3 -- */
    {
        int car;
        puts("\n== part 3: a 0.23 m kerb standing on flat ground ==");
        puts("   the car must never come to REST inside the block's volume");
        for (car = 0; car < 3; car++) {
            rb_car c;
            rb_world w;
            int i, inside;
            memset(&w, 0, sizeof(w));
            w.ground = b_ground; w.sphere = b_sphere; w.segment = b_segment;
            /* start ON TOP of the kerb and drive off its far edge -- which is
               how the photographed car got in: not head-on into the face, but
               over the lip, where the body swings past the vertical side while
               the wheels are still finding the drop. */
            rbcar_init(&c, car, &w, 0.f, g_blk_h + 0.2f, BLK_Z1 - 1.2f, 0.f);
            for (i = 0; i < 120; i++)
                rbcar_step(&c, 0.f, 0.f, 0.f, 0, DT);
            for (i = 0; i < 480; i++)
                rbcar_step(&c, 1.f, 0.f, 0.f, 0, DT);
            inside = (c.body.x[2] > BLK_Z0 && c.body.x[2] < BLK_Z1
                      && c.body.x[1] < g_blk_h - 0.01f);
            ck(!inside, names[car], "ended at z %.2f y %.3f (block z %.1f..%.1f, "
               "top %.2f)%s", c.body.x[2], c.body.x[1], BLK_Z0, BLK_Z1, g_blk_h,
               inside ? "  <-- INSIDE THE KERB" : "");
        }
    }

    /* -------------------------------------------------------- part 4 -- */
    {
        int car;
        puts("\n== part 4: a small rock, and which way up the car ends ==");
        puts("   a rock no taller than HALF a wheel radius must not throw the");
        puts("   car onto its side, whether it is met square or one-sided\n");
        for (car = 0; car < 3; car++) {
            /* The bound is the car's OWN wheel radius out of rb_data.h, not a
               height typed in here: half a radius is 36 / 25 / 30 mm on the
               three cars, which is what "a small rock" means to each of them.
               A wheel climbs a step of half its radius by rolling -- there is
               no mechanism in the recovered model by which that should tilt a
               car at all, so the 45 degree bound is loose by design and still
               catches the fault, which produced 89 to 180 degrees. */
            float r = RB_CARS[car].radius;
            float worst_sq = 0.f, worst_one = 0.f;
            float at_sq = 0.f, at_one = 0.f;
            float h;
            for (h = 0.005f; h <= r * 0.5f + 1e-4f; h += 0.005f) {
                float t;
                /* square on: the whole axle meets it at once */
                t = rock_tilt(car, h, -9.f, 9.f);
                if (t > worst_sq) { worst_sq = t; at_sq = h; }
                /* one-sided: only the left-hand wheels ride over it, which is
                   the case that can ROLL the car rather than pitch it */
                t = rock_tilt(car, h, 0.05f, 0.35f);
                if (t > worst_one) { worst_one = t; at_one = h; }
            }
            ck(worst_sq < 45.f, names[car],
               "square on: worst tilt %6.2f deg at %.0f mm (wheel r %.0f mm)",
               worst_sq, at_sq * 1000.f, r * 1000.f);
            ck(worst_one < 45.f, "  and one-sided",
               "worst tilt %6.2f deg at %.0f mm", worst_one, at_one * 1000.f);
        }
    }

    printf("\n%d failed\n", nfail);
    return nfail ? 1 : 0;
}
