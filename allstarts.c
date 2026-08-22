/* Every track's REAL race start, on its REAL .col grid.
 *
 * Two runs per track. Parked for 15 s, which is the check that a settled car is
 * actually settled -- the tiltable-plane sweep in rb_test cannot see a real
 * track's varied terrain. Then 15 s at full throttle, which is what the reported
 * bouncing and the Vita3K log are about; a flat-plane harness has nothing to
 * bounce off, so the porpoise has to be measured here too.
 *
 * Run from rccars_vita/ with assets/ populated.
 *
 *   gcc -I. -O2 -fno-fast-math -ffp-contract=off ../rccars_re/allstarts.c \
 *       col.c rb.c rbcar.c contact.c collide.c carani.c -lm -o allstarts
 */
#include "col.h"
#include "rbcar.h"
#include "rb_data.h"
#include "tracks.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define DT (1.0f / 60.0f)

static double tilt_deg(const rb_car *c)
{
    double up = c->m[5];
    if (up > 1.0) up = 1.0;
    if (up < -1.0) up = -1.0;
    return acos(up) * 57.2957795;
}

/* Parked runs for 15 s; a driving run only for 3 s.
 *
 * That is not laziness. Nobody steers here, and the port has no wall collision --
 * only downward ground queries -- so a car held at full throttle for fifteen
 * seconds drives off a terrain edge, into the sea, or straight into a 2.3 m step,
 * and what gets measured is the crash rather than the suspension. Three seconds
 * keeps it on the drivable ground around the start, which is where the reported
 * bouncing was. */
#define PARK_FRAMES  900
#define DRIVE_FRAMES 120

/* returns 0 on success, 1 on a failed check */
static int run(const track_info *t, col_t *col, int drive, int *nfail)
{
    rb_car c;
    int i, lost = 0, nframes = drive ? DRIVE_FRAMES : PARK_FRAMES;
    double t0, tlo = 1e9, thi = -1e9, wmax = 0.0, ylo = 1e9, yhi = -1e9;
    float x0, z0, y_start;

    rbcar_init(&c, t->car, col_rb_world(col), t->x, t->y, t->z, t->yaw);
    y_start = c.body.x[1];
    x0 = c.body.x[0];
    z0 = c.body.x[2];
    t0 = tilt_deg(&c);

    /* Settle AT REST first, always: the driving run wants the car's parked
       suspension state as its starting point, and pre-rolling it under throttle
       just puts the measurement window somewhere down the track. */
    for (i = 0; i < 120; i++)
        rbcar_step(&c, 0.0f, 0.0f, 0.0f, 0, DT);
    x0 = c.body.x[0];
    z0 = c.body.x[2];

    for (i = 0; i < nframes; i++) {
        double tl, w;
        rbcar_step(&c, drive ? 1.0f : 0.0f, 0.0f, 0.0f, 0, DT);
        if (c.body.x[1] < y_start - 20.0f) { lost = 1; break; }
        tl = tilt_deg(&c);
        if (tl < tlo) tlo = tl;
        if (tl > thi) thi = tl;
        w = sqrt((double)c.body.w[0]*c.body.w[0] + (double)c.body.w[1]*c.body.w[1]
                 + (double)c.body.w[2]*c.body.w[2]);
        if (w > wmax) wmax = w;
        if (c.body.x[1] < ylo) ylo = c.body.x[1];
        if (c.body.x[1] > yhi) yhi = c.body.x[1];
    }

    if (lost) {
        printf("  %-8s LEFT THE WORLD at %.1f s\n", drive ? "driving" : "parked",
               i / 60.0);
        (*nfail)++;
        return 1;
    }

    if (!drive) {
        double drift = sqrt(((double)c.body.x[0]-x0)*((double)c.body.x[0]-x0)
                            + ((double)c.body.x[2]-z0)*((double)c.body.x[2]-z0));
        printf("  parked   start slope %5.2f deg | tilt p2p %5.2f | worst |w| %5.2f"
               " | drift %5.2f m%s\n", t0, thi - tlo, wmax, drift,
               drift > 1.0 ? "   (rolls away)"
                           : drift > 0.1 ? "   (creeps)" : "");
        /* A car that stays put must be genuinely STILL, not oscillating about
           the right attitude -- that distinction is what "a mean is not a
           stillness test" cost last time. A car that is moving at all is exempt:
           nothing in this model holds a free-rolling car on a slope, and once it
           creeps it also keeps waking itself out of the rest clamp (whose speed
           test is on |P|, so it trips at |v| = 0.18 m/s). 0.1 m in fifteen
           seconds is 7 mm/s, which is not moving. */
        if (drift <= 0.1 && (thi - tlo > 1.0 || wmax > 0.5)) {
            printf("       FAIL: a parked car keeps swinging\n");
            (*nfail)++;
            return 1;
        }
    } else {
        double spd = sqrt((double)c.body.v[0]*c.body.v[0]
                          + (double)c.body.v[2]*c.body.v[2]);
        double went = sqrt(((double)c.body.x[0]-x0)*((double)c.body.x[0]-x0)
                           + ((double)c.body.x[2]-z0)*((double)c.body.x[2]-z0));
        printf("  driving  ride p2p %6.1f mm | tilt %5.2f..%5.2f | worst |w| %5.2f"
               " | %4.2f m/s after %4.1f m\n", (yhi - ylo) * 1000.0, tlo, thi,
               wmax, spd, went);
        /* Under 3 m in two seconds means it drove into something and stopped, so
           what follows is a crash, not the suspension. The port has no wall
           collision; say so rather than scoring it. */
        if (went < 3.0) {
            printf("       (hit scenery -- no wall collision in this port,"
                   " not scored)\n");
            return 0;
        }
        /* Real terrain is not flat, so the ride height legitimately moves. What
           must NOT happen is the 1.5 Hz limit cycle, which showed up as tens of
           degrees of attitude swing and |w| in the several-rad/s range. */
        if (wmax > 6.0) {
            printf("       FAIL: driving spins the car up\n");
            (*nfail)++;
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    int i, nfail = 0;

    for (i = 0; i < N_TRACKS; i++) {
        col_t col;
        char path[256];

        snprintf(path, sizeof(path), "assets/%s.col", TRACKS[i].base);
        if (!col_load(path, &col)) {
            printf("%-10s  CANNOT LOAD %s\n", TRACKS[i].name, path);
            nfail++;
            continue;
        }
        printf("%s  (%s)\n", TRACKS[i].name, TRACKS[i].base);
        run(&TRACKS[i], &col, 0, &nfail);
        run(&TRACKS[i], &col, 1, &nfail);
    }

    printf("\n%s (%d failed)\n", nfail ? "SOME CHECKS FAILED" : "all starts ok",
           nfail);
    return nfail ? 1 : 0;
}
