/* A car on its ROOF -- the one attitude no fixture had ever built.
 *
 * Reported: "car body can go through terrain when it's flipped", with a
 * screenshot of the Buggy upside down on beach_1 and the sand cutting through
 * its chassis. It was doing exactly that. The body collision proxy was a single
 * lateral triplet of r=0.051 spheres at one Z station -- rb_data.h's
 * body_sphere[1] and [2] are absent from every shipped .crs and
 * rb_gather_spheres skips a radius-0 slot -- so a car had 7 collision spheres,
 * not the 13 that function's own header describes, and NONE of them was
 * anywhere near the roof. Inverted, the shell had nothing to stand on and sank
 * until the wheel spheres caught it: 97 mm of Overkill, 64 mm of Buggy and
 * 104 mm of Hummer underground, measured against ENVIR_CAR_BODY.
 *
 * The fix fills those two slots with roof stations fitted to the shell's own top
 * face (see gen_rb_data.py). This harness is what says it worked, and it has to
 * bind to something the fix cannot move:
 *
 *   -- the DRAWN car, read out of the packed .vsc. rb_data.h reaches the runtime
 *      through gen_rb_data.py and the .vsc through pack_vsc.py; both start at
 *      RCCarsDB/Car.sb but nothing downstream is shared, so asserting that the
 *      drawn bodywork stays above the ground is not the proxy checking itself.
 *      Asserting against RB_CARS[].body_sphere would be exactly that.
 *   -- the REAL ten collision grids, not a plane. A plane cannot produce the
 *      slope, the props or the scenery the report came from.
 *
 * Run from rccars_vita/ (needs assets/):
 *   gcc -I. -Itestgl -O2 -fno-fast-math -ffp-contract=off \
 *       ../rccars_re/flipped.c scene.c carani.c col.c rb.c rbcar.c contact.c \
 *       collide.c rlog.c ../rccars_re/glstub_host.c -lm -o flipped
 */
#include "scene.h"
#include "carani.h"
#include "col.h"
#include "rbcar.h"
#include "rb_data.h"
#include "tracks.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DT (1.0f / 60.0f)

static int nfail;

static void ck(int ok, const char *what, const char *how, ...)
{
    va_list ap;
    printf("  %-4s %-58s ", ok ? "ok" : "FAIL", what);
    va_start(ap, how);
    vprintf(how, ap);
    va_end(ap);
    printf("\n");
    if (!ok)
        nfail++;
}

/* ---------------------------------------------------------------- the drawn car

   The lowest point of the bodywork, in the car's own body frame, given the
   attitude in c->m. main.c draws the model as glMultMatrixf(rb_car.m) then
   glTranslatef(0, -carani_wheel_plane_y(&rig), 0), so a model vertex lands at

       world = (model - (0, wheel_plane, 0)) * m

   and that is exactly what this walks. Wheels are excluded -- they are their own
   collision spheres and are supposed to be the low point when the car is the
   right way up. */
static float drawn_low_y(const scene_t *sc, const rb_car *c, float plane,
                         int body_only)
{
    float low = 1e30f;
    int i, k;

    for (i = 0; i < (int)sc->n_batches; i++) {
        const batch_t *b = &sc->batches[i];
        const vtx_t *v = b->rest ? b->rest : b->verts;

        if (!v || !b->nverts)
            continue;
        if (body_only && b->env != ENV_BODY)
            continue;
        /* The ANTENNA never counts, in either column. It is render-only -- it is
           in no collision proxy at all, by design, and antenna.c simulates it as
           a whip that bends. Inverted it hangs 0.2 m straight down through the
           floor, which is the one place a rigid reading of the mesh is simply
           the wrong question: a real aerial lies over. Including it would put a
           -0.2 m number in every row and drown the thing being measured. */
        if (!body_only && sc->has_rig && b->part < CARANI_MAX_PARTS
            && !strcmp(sc->rig.part[b->part].name, "ANTENNA"))
            continue;
        for (k = 0; k < (int)b->nverts; k++) {
            float mx = v[k].x, my = v[k].y - plane, mz = v[k].z;
            float wy = mx * c->m[1] + my * c->m[5] + mz * c->m[9] + c->m[13];
            if (wy < low)
                low = wy;
        }
    }
    return low;
}

/* Put the car on its roof where it already stands, and let it settle.

   The half turn is about the car's FORWARD axis, so the heading is untouched and
   only roll changes -- a car that has been rolled over, not one spun round. The
   get/set round trip is mandatory: writing body.q leaves c->m and the world
   inertia tensor stale, and rb_car_set_state is what rebuilds both. */
static void invert(rb_car *c, float lift)
{
    float y[RB_STATE_N], q[4], r[4];

    r[0] = 0.0f; r[1] = 0.0f; r[2] = 0.0f; r[3] = 1.0f;   /* 180 deg about +Z */
    memcpy(q, c->body.q, sizeof q);
    rb_quat_mul(r, q, c->body.q);
    rb_quat_normalize(c->body.q);
    c->body.x[1] += lift;
    memset(c->body.v, 0, sizeof c->body.v);
    memset(c->body.w, 0, sizeof c->body.w);
    memset(c->body.P, 0, sizeof c->body.P);
    memset(c->body.L, 0, sizeof c->body.L);
    rb_car_get_state(c, y);
    rb_car_set_state(c, y);
}

/* The shell's own height, model space. The tolerance below is a fraction of it,
   so that "the bodywork is not in the ground" is stated relative to the car
   rather than as a number somebody chose. */
static float shell_height(const scene_t *sc)
{
    float lo = 1e30f, hi = -1e30f;
    int i, k;

    for (i = 0; i < (int)sc->n_batches; i++) {
        const batch_t *b = &sc->batches[i];
        const vtx_t *v = b->rest ? b->rest : b->verts;

        if (!v || !b->nverts || b->env != ENV_BODY)
            continue;
        for (k = 0; k < (int)b->nverts; k++) {
            if (v[k].y < lo) lo = v[k].y;
            if (v[k].y > hi) hi = v[k].y;
        }
    }
    return (hi > lo) ? (hi - lo) : 0.0f;
}

int main(void)
{
    static const char *NAME[3] = { "Overkill", "Buggy", "Hummer" };
    /* Where each car's shell ended up BEFORE the roof stations, on the same
       fixture, for the row-by-row comparison. Measured, not remembered:
       ../rccars_re/shellbox.py against RCCarsDB/Car.sb. */
    static const float WAS_BURIED[3] = { 0.097f, 0.064f, 0.104f };
    scene_t sc;
    col_t col;
    rb_car c;
    int car, t, i;
    float plane, worst_all = 1e30f;
    float worst_frac = 0.0f;

    printf("== a car on its roof: does the bodywork stay out of the ground ==\n");

    /* ---- part 1: every car, on every track's own race start ---- */
    for (car = 0; car < 3; car++) {
        char p[256];
        float sh;

        snprintf(p, sizeof p, "assets/car%d.vsc", car + 1);
        memset(&sc, 0, sizeof sc);
        if (!scene_load(p, &sc)) {
            printf("  FAIL cannot load %s -- a missing fixture is a FAILED "
                   "check, not a skip\n", p);
            nfail++;
            continue;
        }

        sh = shell_height(&sc);
        printf("\n%s  (shell %.3f m tall; it used to sink %.0f mm)\n",
               NAME[car], sh, WAS_BURIED[car] * 1000.0);
        if (sh <= 0.0f) {
            printf("  FAIL no ENV_BODY batches in %s -- the fixture cannot "
                   "measure anything\n", p);
            nfail++;
        }
        for (t = 0; t < N_TRACKS; t++) {
            float low_body, low_all, ground, gap_body, gap_all;
            float n[3];

            snprintf(p, sizeof p, "assets/%s.col", TRACKS[t].base);
            if (!col_load(p, &col)) {
                printf("  FAIL cannot load %s\n", p);
                nfail++;
                continue;
            }

            rbcar_init(&c, car, col_rb_world(&col), TRACKS[t].x, TRACKS[t].y,
                       TRACKS[t].z, TRACKS[t].yaw);
            carani_bind(&sc.rig, &c);
            plane = carani_wheel_plane_y(&sc.rig);

            for (i = 0; i < 120; i++)                 /* settle upright first */
                rbcar_step(&c, 0.0f, 0.0f, 0.0f, 0, DT);
            invert(&c, 0.30f);
            for (i = 0; i < 900; i++)                 /* 15 s on its roof */
                rbcar_step(&c, 0.0f, 0.0f, 0.0f, 0, DT);

            low_body = drawn_low_y(&sc, &c, plane, 1);
            low_all = drawn_low_y(&sc, &c, plane, 0);
            if (!col_ground_at(&col, c.body.x[0], c.body.x[2],
                               c.body.x[1] + 1.0f, &ground,
                               &n[0], &n[1], &n[2]))
                ground = 0.0f;
            gap_body = low_body - ground;
            gap_all = low_all - ground;
            if (gap_body < worst_all)
                worst_all = gap_body;
            if (sh > 0.0f && -gap_body / sh > worst_frac)
                worst_frac = -gap_body / sh;

            printf("  %-11s shell %+.3f m / whole car %+.3f m above the "
                   "surface under it\n", TRACKS[t].base, gap_body, gap_all);
            col_free(&col);
        }
        scene_release(&sc);
    }

    /* Stated as a FRACTION OF THE SHELL, not as a distance, because a distance
       here would be a number nobody could argue with and the shells are 111,
       157 and 118 mm tall.
       A car resting on its roof is in a live contact and buzzes: the port has no
       carSubstepContact bisection, so the body ends each substep a little inside
       the surface, is lifted back out, and falls the 1.0 mm slack plus the
       0.51 mm depenetration margin before the gate re-fires. The measurement
       therefore lands anywhere in that cycle. A tenth of the shell is comfortably
       above that band and far below the 58-81% of shell that was underground
       before the roof stations went in, so it separates the two states without
       being tuned to either.

       This is the check the fix has to pass. It cannot be satisfied by the proxy
       agreeing with itself: the number on the left comes out of car<n>.vsc via
       pack_vsc.py and the number on the right out of rb_data.h via
       gen_rb_data.py, and the two pipelines share nothing below Car.sb. */
    printf("\n");
    ck(worst_frac < 0.10f, "no car's shell enters the ground on any track",
       "deepest %.1f%% of a shell (worst gap %+.3f m); it was 58-81%% before",
       worst_frac * 100.0, worst_all);

    printf("\n%d checks failed\n", nfail);
    return nfail ? 1 : 0;
}
