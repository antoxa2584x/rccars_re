/*
 * rockroll.c -- split the known flat-ground limit cycle into HEAVE, PITCH and
 * ROLL.
 *
 * rb_test reports the cycle as "ride p2p 131 mm, worst tilt 16.7 deg", where
 * tilt is the angle between the body's up axis and the world's -- one number
 * that mixes pitch and roll and so cannot say whether the car is porpoising
 * (nose up and down, fore-aft) or ROCKING (side to side). "The car bounces a lot
 * sideways" is a report about roll, and the two have different causes and
 * different fixes, so measure them apart.
 *
 * Flat ground only. There is nothing to bounce off, so everything here is the
 * model's own.
 *
 * Build (from rccars_vita/):
 *   gcc -I. -O2 -fno-fast-math -ffp-contract=off \
 *       ../rccars_re/rockroll.c rb.c rbcar.c contact.c collide.c -lm -o rockroll
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "rb.h"
#include "rbcar.h"
#include "rb_data.h"

static int cb_ground(void *ctx, float x, float z, float ceil_y,
                     float *y, float n[3])
{
    (void)ctx; (void)x; (void)z; (void)ceil_y;
    *y = 0.0f;
    n[0] = 0.0f; n[1] = 1.0f; n[2] = 0.0f;
    return 1;
}

static int cb_sphere(void *ctx, const float c[3], float r,
                     rb_world_hit *hits, int max_hits, int *n_hits)
{
    (void)ctx;
    if (n_hits) *n_hits = 0;
    if (c[1] - r >= 0.0f) return 0;
    if (max_hits > 0 && hits) {
        hits[0].point[0] = c[0];
        hits[0].point[1] = 0.0f;
        hits[0].point[2] = c[2];
        hits[0].surface = 0;
        if (n_hits) *n_hits = 1;
    }
    return 1;
}

static int cb_segment(void *ctx, const float a[3], const float b[3])
{
    (void)ctx;
    return (a[1] < 0.0f || b[1] < 0.0f);
}

/* The body matrix is row-major row-vector: row 0 is the car's own X (its LEFT),
   row 1 its up, row 2 its nose. ROLL is how far its X has lifted out of the
   horizontal, PITCH how far its nose has. Both signed, both in degrees. */
static void attitude(const rb_car *c, float *roll, float *pitch)
{
    *roll  = (float)(asin(c->m[1] > 1.0 ? 1.0 : (c->m[1] < -1.0 ? -1.0 : c->m[1]))
                     * RB_RAD2DEG);
    *pitch = (float)(asin(c->m[9] > 1.0 ? 1.0 : (c->m[9] < -1.0 ? -1.0 : c->m[9]))
                     * RB_RAD2DEG);
}

static void run(int car, const char *what, float throttle, float steer)
{
    rb_car c;
    rb_world w;
    int n;
    float rlo = 1e9f, rhi = -1e9f, plo = 1e9f, phi = -1e9f;
    float ylo = 1e9f, yhi = -1e9f;
    float wx = 0.0f, wz = 0.0f;

    memset(&w, 0, sizeof w);
    w.ground = cb_ground;
    w.sphere = cb_sphere;
    w.segment = cb_segment;
    rbcar_init(&c, car, &w, 0.0f, 0.0f, 0.0f, 0.0f);

    for (n = 0; n < 120; n++)                    /* settle at rest */
        rbcar_step(&c, 0.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
    /* Then settle AGAIN under the test's own inputs. Without this the window
       below swallows the launch transient -- the car squatting once from its
       parked attitude to its power-on one -- and reports that single swing as if
       it were the steady state. It printed 48 degrees of "pitch p2p" for a car
       that pitched once and then held. */
    for (n = 0; n < 240; n++)
        rbcar_step(&c, throttle, 0.0f, steer, 0, 1.0f / 60.0f);
    for (n = 0; n < 600; n++) {                  /* 10 s of the steady state */
        float roll, pitch, ay, az;
        rbcar_step(&c, throttle, 0.0f, steer, 0, 1.0f / 60.0f);
        attitude(&c, &roll, &pitch);
        if (roll < rlo) rlo = roll;
        if (roll > rhi) rhi = roll;
        if (pitch < plo) plo = pitch;
        if (pitch > phi) phi = pitch;
        if (c.body.x[1] < ylo) ylo = c.body.x[1];
        if (c.body.x[1] > yhi) yhi = c.body.x[1];
        /* angular rate about the car's own nose (roll) and X (pitch) */
        ay = fabsf(c.body.w[0]); az = fabsf(c.body.w[2]);
        if (az > wz) wz = az;
        if (ay > wx) wx = ay;
    }
    printf("car %d %-22s heave p2p %6.1f mm | roll p2p %6.2f deg "
           "[%+.2f,%+.2f] | pitch p2p %6.2f deg [%+.2f,%+.2f] | "
           "peak |w| roll %5.2f pitch %5.2f\n",
           car + 1, what, 1000.0f * (yhi - ylo),
           rhi - rlo, rlo, rhi, phi - plo, plo, phi, wz, wx);
}

/* An upside-down car, dropped on flat ground and left alone.
 *
 * Upright, the suspension is the only thing holding the car up, and it acts
 * along the body's Y axis through the WHEEL spheres. Inverted, the wheels point
 * at the sky and can never reach the ground, so the suspension contributes
 * nothing and the only thing left is rb_body_contact_stop -- the port's invented
 * stand-in for the untranscribed body-sphere contact solve. That is a positional
 * PROJECTION, not a force: it pushes each penetrating body sphere back out along
 * its contact normal and deletes the momentum going into the surface. So gravity
 * never accumulates, and whatever tangential velocity the car had it keeps,
 * because the projection has no friction.
 *
 * The prediction is a car that hovers at whatever height the body spheres decide
 * and slides forever. Measure both. */
static void flipped(int car)
{
    rb_car c;
    rb_world w;
    int n;
    float y0, drift;
    float x0, z0;

    memset(&w, 0, sizeof w);
    w.ground = cb_ground;
    w.sphere = cb_sphere;
    w.segment = cb_segment;
    rbcar_init(&c, car, &w, 0.0f, 0.0f, 0.0f, 0.0f);

    /* half a turn about Z, and lift it clear so it has to fall */
    c.body.q[0] = 0.0f; c.body.q[1] = 0.0f; c.body.q[2] = 0.0f; c.body.q[3] = 1.0f;
    c.body.x[1] = 0.40f;
    {
        float y[RB_STATE_N];
        rb_car_get_state(&c, y);
        rb_car_set_state(&c, y);
    }
    y0 = c.body.x[1];
    x0 = c.body.x[0];
    z0 = c.body.x[2];
    for (n = 0; n < 900; n++)                    /* 15 s */
        rbcar_step(&c, 0.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
    drift = sqrtf((c.body.x[0]-x0)*(c.body.x[0]-x0)
                  + (c.body.x[2]-z0)*(c.body.x[2]-z0));
    printf("car %d upside down: released at y=%.3f, after 15 s y=%.3f "
           "(body half-height %.3f) | drifted %.2f m | still moving %.3f m/s\n",
           car + 1, y0, c.body.x[1], 0.5f * RB_CARS[car].extent[1], drift,
           sqrtf(c.body.v[0]*c.body.v[0] + c.body.v[1]*c.body.v[1]
                 + c.body.v[2]*c.body.v[2]));
}

int main(void)
{
    puts("flat ground, 10 s of steady state after settling\n");
    run(0, "parked", 0.0f, 0.0f);
    run(0, "flat out, straight", 1.0f, 0.0f);
    run(0, "flat out, full lock", 1.0f, 1.0f);
    run(1, "flat out, straight", 1.0f, 0.0f);
    run(2, "flat out, straight", 1.0f, 0.0f);
    puts("");
    flipped(0);
    flipped(1);
    flipped(2);
    return 0;
}
