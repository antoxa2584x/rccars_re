/* Body collision under a low ceiling -- the overhead half of the world.
 *
 * Reported from the game: "car hit some places with low space (rest place near
 * the end of the first track, changing room near the small river), causing it to
 * flip or smash through the upper wall."
 *
 * That is not an exotic case. Sampling the shipped grids on a 0.7 m lattice,
 * beach_1 has 608 sample points and urban_1 1,015 where a downward-facing face
 * sits 0.20-1.60 m above a drivable floor. The car gets under roofs constantly.
 *
 * WHAT WAS WRONG. col_sphere is a real closest-point query, so it returns
 * ceilings. rb_coll_list and rb_body_depenetrate decided which way to push a
 * contact by asking "is the sphere centre below the hit point" -- a test written
 * for a downward-ray host, and one a ceiling passes with no penetration at all.
 * Both then treated a roof touch as "sunk into the ground" and pushed the car
 * along WORLD UP, straight into the roof: rb_coll_list drove it there at
 * +0.05 m/s of "separation" and rb_body_depenetrate teleported it, so a 0.44 m
 * roof was cleared in a single step. Both now ask the surface which side of it
 * is open space, via rb_world_hit.normal.
 *
 * The fixture is a floor and a ceiling slab, wound outward, built as a REAL
 * col_t so the query runs through the real col_sphere, the real rb_coll_list and
 * the real rbcar_step rather than a stub.
 *
 * The real-grid counterpart to this is `allstarts`, where the same fix took
 * urban_2's parked tilt swing from 176.3 deg to 70.1 and country_4's from 40.7
 * to 15.7 -- cars rolling downhill into scenery, being flipped by it.
 *
 * Run from rccars_vita/:
 *   gcc -I. -O2 -fno-fast-math -ffp-contract=off ../rccars_re/ceiling.c \
 *       col.c rb.c rbcar.c contact.c collide.c carani.c -lm -o ceiling
 */
#include "col.h"
#include "rbcar.h"
#include "rb_data.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DT (1.0f / 60.0f)
#define HALF 20.0f            /* the room is 40 x 40 m */

static int nfail;

static void ck(int ok, const char *what, const char *detail)
{
    if (!ok) nfail++;
    printf("  %-4s %-52s %s\n", ok ? "ok" : "FAIL", what, detail ? detail : "");
}

/* ------------------------------------------------------------------ fixture */

static float g_tris[4 * 9];
static unsigned int *g_start;
static unsigned int *g_idx;

/* One of the two triangles covering a rectangle at height y, spanning z0..z1.
   `flip` swaps b and c, reversing the winding so (b-a)x(c-a) points -Y not +Y. */
static void put(int i, int flip, float y, float z0, float z1)
{
    float *t = &g_tris[i * 9];
    int k;

    if ((i & 1) == 0) {
        t[0] = -HALF; t[2] = z0;
        t[3] = -HALF; t[5] = z1;
        t[6] =  HALF; t[8] = z0;
    } else {
        t[0] =  HALF; t[2] = z1;
        t[3] =  HALF; t[5] = z0;
        t[6] = -HALF; t[8] = z1;
    }
    for (k = 0; k < 3; k++) t[k * 3 + 1] = y;
    if (flip)
        for (k = 0; k < 3; k++) {
            float tmp = t[3 + k]; t[3 + k] = t[6 + k]; t[6 + k] = tmp;
        }
}

/* A room: floor at y = 0 (outward normal +Y), ceiling at y = h (outward -Y).
   `roof_z0` is where the roof starts, so the car can be driven in from open
   ground under an edge -- which is the reported case, and a different one from
   being parked under it: the proxy sphere sits AHEAD of the centre of mass, so
   driving in strikes the roof edge with a lever arm. */
static void build_from(col_t *c, float h, float roof_z0)
{
    unsigned int ncell, i, k;

    memset(c, 0, sizeof(*c));
    c->minx = -HALF; c->minz = -HALF; c->cell = 3.0f;
    c->nx = (unsigned)(2 * HALF / 3.0f) + 1;
    c->nz = c->nx;
    c->ntris = 4;

    put(0, 0, 0.f, -HALF, HALF);           /* floor, the whole room */
    put(1, 0, 0.f, -HALF, HALF);
    put(2, 1, h, roof_z0, HALF);           /* ceiling, from roof_z0 onward */
    put(3, 1, h, roof_z0, HALF);
    c->tris = g_tris;

    ncell = c->nx * c->nz + 1;
    g_start = malloc(ncell * sizeof(unsigned int));
    g_idx = malloc((size_t)(ncell - 1) * 4 * sizeof(unsigned int));
    k = 0;
    for (i = 0; i + 1 < ncell; i++) {
        unsigned int j;
        g_start[i] = k;
        for (j = 0; j < 4; j++) g_idx[k++] = j;
    }
    g_start[ncell - 1] = k;
    c->start = g_start;
    c->idx = g_idx;
    c->default_surf = 0;
}

/* the whole room roofed, which is what the parked sweep wants */
static void build(col_t *c, float h) { build_from(c, h, -HALF); }

static void teardown(col_t *c)
{
    free(g_start); free(g_idx);
    memset(c, 0, sizeof(*c));
}

static double tilt_deg(const rb_car *c)
{
    double up = c->m[5];
    if (up > 1.0) up = 1.0;
    if (up < -1.0) up = -1.0;
    return acos(up) * 57.2957795;
}

/* The parked car's ride height, the top of its BODY spheres, and the top of its
 * WHEEL spheres.
 *
 * The two tops have to be kept apart. This harness is about what a roof does to
 * the BODY proxy, and now that the proxy sits inside the car rather than in the
 * air above it, the two bands nearly touch -- the Hummer's body spheres top out
 * at 0.144 m and its wheels at 0.118. A roof chosen just under the tallest
 * sphere of any kind lands below the wheel tops on that car, and then what is
 * being measured is a wheel driving into a wall. Every roof here is placed
 * strictly between the two. */
static void settled(int car, float *com_y, float *body_top, float *wheel_top)
{
    col_t col;
    rb_car c;
    float s[RB_MAX_SPHERES][4];
    int n, i;

    build(&col, 100.f);
    rbcar_init(&c, car, col_rb_world(&col), 0.f, 0.02f, 0.f, 0.f);
    for (i = 0; i < 240; i++)
        rbcar_step(&c, 0.f, 0.f, 0.f, 0, DT);
    n = rb_gather_spheres(&c, s);
    *com_y = c.body.x[1];
    *body_top = -1e30f;
    *wheel_top = -1e30f;
    for (i = 0; i < n; i++) {
        float t = s[i][1] + s[i][3];
        if (i < c.nwheels) { if (t > *wheel_top) *wheel_top = t; }
        else               { if (t > *body_top)  *body_top  = t; }
    }
    teardown(&col);
}

/* Put the car UNDER a roof without letting the spawn probe see the roof.
 *
 * rbcar_init re-probes the ground with a ceiling of y + RBCAR_PLACE_CEIL, and
 * with the body proxy now sitting INSIDE the car these roofs are lower than that
 * window -- so probing beneath one picks the ROOF as the ground and stands the
 * car on top of it before a single step runs. Build the room with the roof over
 * z >= 0 only, settle in the open half, then slide the settled car under. The
 * state round-trip is how rb_test moves a car; body.x alone leaves the matrix
 * stale. */
static void place_under(rb_car *c, int car, col_t *col)
{
    float y[RB_STATE_N];
    int i;

    rbcar_init(c, car, col_rb_world(col), 0.f, 0.02f, -4.f, 0.f);
    for (i = 0; i < 180; i++)
        rbcar_step(c, 0.f, 0.f, 0.f, 0, DT);
    c->body.x[2] = 2.f;
    rb_car_get_state(c, y);
    rb_car_set_state(c, y);
    for (i = 0; i < 30; i++)
        rbcar_step(c, 0.f, 0.f, 0.f, 0, DT);
}

/* ------------------------------------------------------------------- checks */

/* The mechanism, directly: what normal does a ceiling touch produce? */
static void part_normal(void)
{
    col_t col;
    rb_car c;
    rb_coll_contact ct[RB_MAX_SPHERES * 4];
    float com, top, wtop, h;
    int n, i, ncei = 0, nup = 0;
    char buf[128];

    printf("\n-- part 1: the normal a ceiling contact produces --\n");
    settled(0, &com, &top, &wtop);
    h = top - 0.01f;                     /* 10 mm into the body sphere */

    build_from(&col, h, 0.f);
    place_under(&c, 0, &col);
    rbcar_step(&c, 0.f, 0.f, 0.f, 0, DT);
    n = rb_coll_list(&c, 0.006f, 1, ct, (int)(sizeof(ct) / sizeof(ct[0])));

    for (i = 0; i < n; i++) {
        if (ct[i].point[1] <= c.body.x[1]) continue;   /* not the ceiling */
        ncei++;
        if (ct[i].normal[1] > 0.f) nup++;
    }
    sprintf(buf, "roof at %.3f m, %d body contact(s), %d on the ceiling",
            h, n, ncei);
    ck(ncei > 0, "a roof 10 mm into the body sphere IS felt", buf);
    sprintf(buf, "%d of %d point up into the roof", nup, ncei);
    ck(ncei > 0 && nup == 0,
       "and every ceiling contact pushes AWAY from the ceiling", buf);
    teardown(&col);
}

/* The consequence: sweep the roof down through the whole band where it touches
 * the car, and require the car to stay under it and on the floor.
 *
 * The sweep matters. A single height is not a test of a threshold -- the old
 * code was harmless until the top sphere reached the roof and catastrophic one
 * millimetre later, so the step has to be small enough to land inside the band.
 */
static void part_sweep(int car)
{
    float com, top, wtop, h, lo, hi;
    int tested = 0, through = 0, lifted = 0;
    double worst_rise = 0.0, worst_tilt = 0.0, worst_rest = 0.0;
    float at_rise = 0.f, at_rest = 0.f;
    char buf[160];

    settled(car, &com, &top, &wtop);
    /* Only the band where the roof meets the BODY spheres and nothing else:
       from just clear of them down to just above the wheels. Below `lo` the roof
       is level with the tyres and the test stops being about body collision. */
    hi = top + 0.02f;
    lo = wtop + 0.005f;
    printf("\n-- part 2: car %d, roof swept %.3f -> %.3f m "
           "(ride height %.3f, body spheres top out at %.3f, wheels at %.3f) --\n",
           car + 1, hi, lo, com, top, wtop);

    for (h = hi; h > lo; h -= 0.001f) {
        col_t col;
        rb_car c;
        int i;
        double ymax = -1e30;

        build_from(&col, h, 0.f);
        place_under(&c, car, &col);
        if (c.body.x[1] > h) { teardown(&col); continue; }

        tested++;
        for (i = 0; i < 300; i++) {
            rbcar_step(&c, 0.f, 0.f, 0.f, 0, DT);
            if (c.body.x[1] > ymax) ymax = c.body.x[1];
            if (tilt_deg(&c) > worst_tilt) worst_tilt = tilt_deg(&c);
        }
        if (ymax > h) through++;                        /* got above the roof */
        if (c.body.x[1] - com > 0.100f) lifted++;
        if (ymax - com > worst_rise) { worst_rise = ymax - com; at_rise = h; }
        if (c.body.x[1] - com > worst_rest) {
            worst_rest = c.body.x[1] - com; at_rest = h;
        }
        teardown(&col);
    }

    /* The band is only millimetres tall now -- the body proxy lives inside the
       shell, so it clears the tyres by 9 mm on the Overkill, 25 mm on the Hummer
       and NOT AT ALL on the Buggy, whose wheels stand 23 mm above its body
       spheres. A car with no band is a legitimate answer, not a skipped test,
       and it is exactly why part 2b stopped trying to find one. */
    sprintf(buf, "%d roof heights tested, band %.3f -> %.3f", tested, hi, lo);
    ck(tested >= 5 || top <= wtop,
       "the sweep covered the body band, or there is none", buf);

    /* THE reported bug. Under the old rule the car ended up standing on the roof
       -- resting at exactly roof + its own ride height, a lift of 350-450 mm. */
    sprintf(buf, "%d of %d rose above the roof", through, tested);
    ck(through == 0, "the car never gets above the ceiling", buf);

    /* The bound is 100 mm and not tighter ON PURPOSE, and the slack is not
       margin -- it is a second, known defect showing through. The body proxy
       sits 0.22-0.41 m ABOVE and 0.32-0.50 m AHEAD of the centre of mass on a
       car 0.42 m long (see the proxy dump above; CLAUDE.md has the CdtDelta
       conversion down as unresolved). A roof pressing on a lever that long
       pitches the car rather than squashing it, and pitching a car up on its
       rear wheels genuinely does raise its centre of mass -- the Hummer reaches
       77 mm that way with entirely correct normals. What this still catches is
       the failure it was written for, which was 4x bigger than the bound. */
    sprintf(buf, "peak %.1f mm at roof %.3f (old code: ~440 mm)",
            worst_rise * 1000.0, at_rise);
    ck(worst_rise < 0.100, "a ceiling never lifts the car off the floor", buf);
    sprintf(buf, "worst %.1f mm at roof %.3f", worst_rest * 1000.0, at_rest);
    ck(lifted == 0, "and it is still on the floor when it settles", buf);
    sprintf(buf, "worst tilt %.1f deg", worst_tilt);
    ck(worst_tilt < 45.0, "and never flips it", buf);
}

/* The reported case, and the one the parked sweep CANNOT stand in for: driving
 * in from open ground under the edge of a low roof.
 *
 * Parked, the roof arrives everywhere at once and presses straight down. Driven,
 * it arrives at the leading proxy sphere first -- and that sphere is 0.32-0.50 m
 * AHEAD of the centre of mass -- so the contact comes in with a long lever and
 * the car's own momentum behind it. That is what "hit some places with low space
 * and flipped or smashed through the upper wall" describes, and the old code
 * answered it with an upward impulse. */
static void part_drive_under(int car)
{
    col_t col;
    rb_car c;
    float com, top, wtop, h;
    int i;
    double ymax = -1e30, worst_tilt = 0.0;
    int above = 0, best_i = -1;
    float maxz = -1e30f;
    char buf[160];

    settled(car, &com, &top, &wtop);
    /* 20 mm of clearance over THE TALLEST THING THE CAR COLLIDES WITH, whichever
       that is -- and it is no longer always the tyres.
       This used to be `wtop + 0.020f`, on the reasoning that the body proxy's
       top was within a centimetre of the tyre tops (the Buggy's 23 mm BELOW
       them), so a roof low enough to reach the body was one the tyres hit first.
       That was true of a proxy with no roof stations in it. It is false now: the
       roof stations put the body top at 0.251 / 0.202 / 0.248 against tyre tops
       of 0.144 / 0.135 / 0.119, and taking the gap off the tyres alone asked the
       car to drive under a roof 87 mm BELOW its own bodywork -- which it cannot
       do, and should not. The car refusing that gap is the fix working; the
       fixture demanding it was the fixture measuring the old proxy.
       Before the ceiling fix a gap this size threw the car through the roof;
       before the cdt fix the car did not fit under 0.51 m at all. */
    h = (top > wtop ? top : wtop) + 0.020f;

    printf("\n-- part 2b: car %d driven under a roof edge at z=0, roof %.3f m"
           " (wheels top out at %.3f, body at %.3f) --\n",
           car + 1, h, wtop, top);

    build_from(&col, h, 0.f);
    /* start in the open half, nose pointing at the roof edge (+Z is forward) */
    rbcar_init(&c, car, col_rb_world(&col), 0.f, 0.02f, -4.f, 0.f);
    for (i = 0; i < 420; i++) {
        rbcar_step(&c, 1.0f, 0.f, 0.f, 0, DT);
        /* Stop at the edge of the fixture's floor. At full throttle the car
           covers the room in about six seconds, and driving off the end and
           tumbling into the void is not a roof result -- it read as a 179 deg
           flip until this was here. */
        if (c.body.x[2] > HALF - 4.f) break;
        if (c.body.x[2] > maxz) maxz = c.body.x[2];
        if (c.body.x[2] < -0.5f) continue;        /* not at the roof yet */
        if (c.body.x[1] > ymax) { ymax = c.body.x[1]; best_i = i; }
        if (tilt_deg(&c) > worst_tilt) worst_tilt = tilt_deg(&c);
        if (c.body.x[1] > h) above = 1;
    }

    /* Fixture validity, not a result: did the car actually get to the roof? It
       does not have to get all the way under. The Buggy's leading proxy sphere
       is 0.496 m in front of its centre of mass -- further than the car is long
       -- so it strikes the roof edge like a wall and is deflected back out,
       under old and new code alike. That is the proxy defect, not this one. */
    sprintf(buf, "reached z=%+.2f (roof edge at 0.00), ended %.2f m/s",
            maxz, rbcar_speed(&c));
    ck(maxz > 3.0f, "the car drives right through the gap", buf);

    /* The two that are clean, and between them they kill every old-code run:
       the Buggy went THROUGH (peak 0.497 under a 0.313 roof) and all three
       flipped (136.5, 179.1, 91.8 deg). */
    sprintf(buf, "peak y %.3f at frame %d, roof at %.3f", ymax, best_i, h);
    ck(!above, "and never rose through it", buf);
    sprintf(buf, "worst tilt %.1f deg (old code: 136.5 / 179.1 / 91.8)",
            worst_tilt);
    ck(worst_tilt < 45.0, "nor was flipped by it", buf);

    /* Reported, not asserted. The lift here is set by how hard the leading
       phantom sphere hits the roof edge, so it measures the proxy offset more
       than it measures this fix. Old code: 145 / 439 / 187 mm. */
    printf("       (lifted %.1f mm above its free ride height)\n",
           (ymax - com) * 1000.0);
    teardown(&col);
}

/* Control: a roof out of reach must change nothing at all. Without this, code
 * that simply ignored ceilings would pass part 2 outright. */
static void part_control(void)
{
    col_t col;
    rb_car c;
    float com, top, wtop;
    int i;
    char buf[128];

    printf("\n-- part 3: control, a roof the car cannot reach --\n");
    settled(0, &com, &top, &wtop);

    build(&col, top + 0.50f);
    rbcar_init(&c, 0, col_rb_world(&col), 0.f, 0.02f, 0.f, 0.f);
    for (i = 0; i < 300; i++)
        rbcar_step(&c, 0.f, 0.f, 0.f, 0, DT);
    sprintf(buf, "%.4f vs %.4f with no roof at all", c.body.x[1], com);
    ck(fabs(c.body.x[1] - com) < 1e-4, "a roof 0.5 m clear leaves the car alone",
       buf);
    teardown(&col);
}

/* Diagnostics, not a check: where the collision proxy actually sits. */
static void part_proxy(void)
{
    int car;

    printf("\n-- the body collision proxy, for reference --\n");
    for (car = 0; car < 3; car++) {
        col_t col;
        rb_car c;
        float s[RB_MAX_SPHERES][4];
        int n, i;

        build(&col, 100.f);
        rbcar_init(&c, car, col_rb_world(&col), 0.f, 0.02f, 0.f, 0.f);
        for (i = 0; i < 240; i++) rbcar_step(&c, 0.f, 0.f, 0.f, 0, DT);
        n = rb_gather_spheres(&c, s);
        printf("  car %d: com y %.4f, %d spheres\n", car + 1, c.body.x[1], n);
        for (i = c.nwheels; i < n; i++)
            printf("      body sphere r=%.4f at (%+.3f %+.3f %+.3f)"
                   "  = %+.3f above / %+.3f ahead of the com\n",
                   s[i][3], s[i][0], s[i][1], s[i][2],
                   s[i][1] - c.body.x[1], s[i][2] - c.body.x[2]);
        teardown(&col);
    }
}

int main(void)
{
    int car;

    printf("== body collision under a low ceiling ==\n");
    part_proxy();
    part_normal();
    for (car = 0; car < 3; car++)
        part_sweep(car);
    for (car = 0; car < 3; car++)
        part_drive_under(car);
    part_control();

    printf("\n%s (%d failed)\n",
           nfail ? "SOME CHECKS FAILED" : "all ceiling checks passed", nfail);
    return nfail ? 1 : 0;
}
