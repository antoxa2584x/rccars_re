/*
 * proptest.c -- the knockable props, against the REAL collision grids and the
 * REAL car.
 *
 * Everything here binds to a consequence rather than to the constant that
 * produced it. Checking that a ball's restitution is 0.7 tests nothing but that
 * prop_data.h says 0.7; checking that a ball bounces higher than a can tests
 * that the number reaches the solver and means what it says. This project has
 * written a self-referential assertion six times (CLAUDE.md, "Testing"), so the
 * rule here is: assert a RATIO, an ORDER, or a physical fact the constant cannot
 * move.
 *
 * Build (no Vita toolchain needed):
 *   gcc -I../rccars_vita -O2 -fno-fast-math -ffp-contract=off \
 *       proptest.c ../rccars_vita/prop.c ../rccars_vita/col.c \
 *       ../rccars_vita/rb.c ../rccars_vita/rbcar.c ../rccars_vita/contact.c \
 *       ../rccars_vita/collide.c ../rccars_vita/carani.c \
 *       ../rccars_vita/rlog.c -lm -o proptest
 */
#include "prop.h"
#include "rbcar.h"
#include "tracks.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * prop.c reaches scene.c for two things and this harness has neither a scene nor
 * a GL. Stubbed here rather than linking scene.c, because what is under test is
 * the physics -- vis_test is where the drawing is checked, against the recording
 * stand-in. prop_init is handed scene = NULL, so neither is ever reached; these
 * exist to satisfy the linker and they SAY SO by failing loudly if they are.
 */
int scene_model_index(const scene_t *s, const char *name)
{ (void)s; (void)name; printf("  !! scene_model_index reached with no scene\n"); return -1; }
void scene_draw_model(const scene_t *s, unsigned int m, const float mat[16])
{ (void)s; (void)m; (void)mat; printf("  !! scene_draw_model reached with no scene\n"); }

static int checks, fails;

static void ck(int ok, const char *what, const char *fmt, ...)
{
    va_list ap;
    checks++;
    if (!ok) fails++;
    printf("  %-4s %-58s ", ok ? "ok" : "FAIL", what);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
}

/* tracks.h already carries the ten tracks, in METRES, with their race starts --
   the same table main.c drives from. No second copy here. */
static int load_track_col(col_t *c, int i)
{
    char path[256];
    snprintf(path, sizeof path, "/mnt/c/tools/rccars_vita/assets/%s.col",
             TRACKS[i].base);
    return col_load(path, c);
}

/* prop.c's sphere_world is static; this is the same expression, and it has to
   stay the same -- it is what decides whether a prop is resting or hovering. */
static float sphere_world_t(const prop_t *p, const prop_model_t *pm, int i,
                            const float m[16], float out[3])
{
    float local[3], rot[3];
    local[0] = pm->sphere[i][0];
    local[1] = pm->sphere[i][1] - pm->com_y;
    local[2] = pm->sphere[i][2];
    rb_mat3_mul_vec3(m, local, rot);
    out[0] = p->pos[0] + rot[0];
    out[1] = p->pos[1] + rot[1];
    out[2] = p->pos[2] + rot[2];
    return pm->sphere[i][3];
}

static float vlen3(const float a[3])
{ return sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]); }

/*
 * A dead-flat synthetic plane, one cell, two triangles, all malloc'd so col_free
 * can have it back.
 *
 * The wind check needs this and a real track cannot give it. On beach_1 the
 * control object -- a ball with no fWindBlow -- rolled 0.43 m down the beach
 * while the tumbleweed drifted 0.30 m, so the "no wind" case moved FURTHER than
 * the "wind" case and the check failed for a reason that had nothing to do with
 * wind. A slope is not a control when what is being measured is horizontal
 * drift. (CLAUDE.md: "A control that resonates with what it controls for is not
 * a control.") On flat ground gravity has no horizontal component at all, so
 * anything that moves sideways did so because of the wind.
 */
static void make_plane(col_t *c, float half, float y)
{
    static const int NT = 2;
    memset(c, 0, sizeof *c);
    c->minx = -half; c->minz = -half;
    c->cell = 2.f * half;
    c->nx = 1; c->nz = 1;
    c->ntris = NT;
    c->tris = malloc((size_t)NT * 9 * sizeof(float));
    c->start = malloc(2 * sizeof(unsigned int));
    c->idx = malloc(NT * sizeof(unsigned int));
    {
        float *t = c->tris;
        float v[4][3] = { { -half, y, -half }, {  half, y, -half },
                          {  half, y,  half }, { -half, y,  half } };
        int tri[2][3] = { { 0, 1, 2 }, { 0, 2, 3 } }, i, k, j;
        for (i = 0; i < NT; i++)
            for (k = 0; k < 3; k++)
                for (j = 0; j < 3; j++)
                    t[i * 9 + k * 3 + j] = v[tri[i][k]][j];
    }
    c->start[0] = 0; c->start[1] = NT;
    c->idx[0] = 0; c->idx[1] = 1;
}

/* A prop on its own over flat-enough ground, dropped from `h` above where it
   was placed, stepped for `secs`. Returns the peak height it reached AFTER the
   first ground contact -- i.e. how high it bounced. */
static float bounce_peak(int model, const col_t *col, float secs)
{
    props_t pr;
    prop_t *p;
    float y0, peak = -1e30f;
    int touched = 0, i;

    memset(&pr, 0, sizeof pr);
    pr.col = col;
    pr.enabled = 1;
    pr.n = 1;
    p = &pr.p[0];
    memset(p, 0, sizeof *p);
    p->model = model;
    p->q[0] = 1.f;
    p->awake = 1;
    /* Somewhere the grid answers. beach_1's start is flat sand. */
    p->pos[0] = TRACKS[0].x;
    p->pos[2] = TRACKS[0].z;
    if (!col_ground_at(col, p->pos[0], p->pos[2], 1e9f, &y0, NULL, NULL, NULL))
        return 0.f;
    p->pos[1] = y0 + 0.60f;
    prop_matrix(p, p->m);

    for (i = 0; i < (int)(secs * 240.f); i++) {
        float before = p->pos[1];
        prop_step(&pr, NULL, 1.f / 240.f);
        if (!touched && p->pos[1] <= before && p->v[1] >= 0.f)
            touched = 1;                    /* velocity flipped: it has landed */
        if (touched && p->pos[1] > peak)
            peak = p->pos[1];
    }
    return touched ? peak - y0 : 0.f;
}

int main(void)
{
    col_t col;
    int t, i;

    printf("RC Cars -- knockable props\n");

    /* ---- part 1: the data itself --------------------------------------- */
    printf("\n-- part 1: the model table --\n");
    {
        int bad_r = 0, bad_m = 0, n_wind = 0, n_sph = 0;
        for (i = 0; i < PROP_N_MODELS; i++) {
            const prop_model_t *m = &PROP_MODELS[i];
            int j;
            if (m->mass <= 0.f || m->mass > 5.f) bad_m++;
            n_wind += m->wind;
            n_sph += m->n_spheres;
            for (j = 0; j < m->n_spheres; j++)
                /* A proxy sphere bigger than the car is the shape of the bug
                   that put a 5.6 m radius on a traffic cone. */
                if (m->sphere[j][3] <= 0.f || m->sphere[j][3] > 0.5f) bad_r++;
        }
        ck(PROP_N_MODELS == 13, "all thirteen models are present",
           "%d models", PROP_N_MODELS);
        ck(bad_m == 0, "every mass is a plausible real-world one",
           "%d outside 0..5 kg", bad_m);
        ck(bad_r == 0,
           "no proxy sphere is larger than the car it has to be hit by",
           "%d over 0.5 m radius", bad_r);
        ck(n_sph > 0, "the proxies are populated", "%d spheres over 13 models",
           n_sph);
        /* stone.sb gives fWindBlow to exactly one object, the tumbleweed. If a
           second ever appears the wind code is being applied to something the
           data never asked for. */
        ck(n_wind == 1, "exactly one model carries fWindBlow", "%d do", n_wind);
    }

    /* ---- part 2: every placement resolves ------------------------------- */
    printf("\n-- part 2: the 129 placements --\n");
    {
        int total = 0, bad = 0, empty = 0;
        for (t = 0; t < 10; t++) {
            const prop_track_t *pt = &PROP_TRACKS[t];
            if (pt->n == 0) empty++;
            for (i = 0; i < pt->n; i++) {
                total++;
                if (pt->p[i].model < 0 || pt->p[i].model >= PROP_N_MODELS)
                    bad++;
            }
        }
        ck(total == 129, "every prop the ten tracks place is in the table",
           "%d placements", total);
        ck(bad == 0, "and every one names a model that exists",
           "%d unresolved", bad);
        ck(empty == 0, "no track came out with none", "%d empty", empty);
    }

    /* ---- part 3: they settle on the real ground ------------------------- */
    printf("\n-- part 3: settling on the ten real collision grids --\n");
    for (t = 0; t < 10; t++) {
        props_t pr;
        int below = 0, moving = 0, asleep = 0, floating = 0, n;
        float worst_drop = 0.f, worst_float = 0.f;

        if (!load_track_col(&col, t)) {
            ck(0, "the .col loads", "%s missing", TRACKS[t].base);
            continue;
        }
        /* prop_init settles them itself now -- that is the thing under test. */
        prop_init(&pr, NULL, &col, t);
        n = pr.n;
        for (i = 0; i < 240 * 5; i++)
            prop_step(&pr, NULL, 1.f / 240.f);

        for (i = 0; i < n; i++) {
            const prop_t *p = &pr.p[i];
            const prop_model_t *pm = &PROP_MODELS[p->model];
            float gy, drop;
            if (!p->awake) asleep++;
            if (vlen3(p->v) > 0.3f) moving++;
            /* CEILING. An unbounded probe returns the topmost triangle in the
               cell, which over an overpass or inside a building is the ROOF --
               beach_2's race start hit exactly this and needed RBCAR_PLACE_CEIL.
               Probing from just above the prop asks the question that was meant:
               what is the surface it is resting on. */
            if (col_ground_at(&col, p->pos[0], p->pos[2], p->pos[1] + 0.25f, &gy,
                              NULL, NULL, NULL)) {
                float m16[16], lowest = 1e30f;
                int j;
                drop = gy - p->pos[1];
                if (drop > 0.05f) below++;
                if (drop > worst_drop) worst_drop = drop;
                /*
                 * AND THAT IT IS NOT IN THE AIR. The first version of this part
                 * only asked whether a prop had sunk, so 129 props hanging a
                 * median 0.47 m above the terrain passed it cleanly -- and that
                 * is precisely what shipped and got reported as "hitable objects
                 * float in air". Assert the value, not that something happened.
                 *
                 * Measured on the LOWEST PROXY SPHERE, because that is what
                 * rests: a bottle's origin is 0.18 m off the ground when it is
                 * standing perfectly correctly on its base.
                 */
                prop_matrix(p, m16);
                for (j = 0; j < pm->n_spheres; j++) {
                    float c3[3], r = sphere_world_t(p, pm, j, m16, c3);
                    if (c3[1] - r < lowest) lowest = c3[1] - r;
                }
                if (pm->n_spheres && lowest - gy > 0.05f) {
                    floating++;
                    if (lowest - gy > worst_float) worst_float = lowest - gy;
                }
            }
        }
        ck(below == 0 && moving == 0 && floating == 0,
           "props rest ON the surface -- not through it and not above it",
           "%-10s %2d props, %d sunk, %d FLOATING (worst %.3f m), %d moving",
           TRACKS[t].base, n, below, floating, worst_float, moving);
        col_free(&col);
    }

    /* ---- part 4: the car knocks them ------------------------------------ */
    printf("\n-- part 4: hitting them with the real car --\n");
    if (load_track_col(&col, 0)) {
        struct { int model; const char *name; float moved, peak; } run[3];
        int k;
        /* Same hit, three different masses: a balloon (0.15 kg), a can (0.1) and
           a traffic cone (1.0). The heavy one must move least. */
        const int MODELS[3] = { 12, 1, 5 };      /* Ballon, Cola, Conus */
        const char *NAMES[3] = { "Ballon", "Cola", "Conus" };

        for (k = 0; k < 3; k++) {
            props_t pr;
            rb_car car;
            float start[3], gy;
            int step;

            memset(&pr, 0, sizeof pr);
            pr.col = &col;
            pr.enabled = 1;
            pr.n = 1;
            memset(&pr.p[0], 0, sizeof pr.p[0]);
            pr.p[0].model = MODELS[k];
            pr.p[0].q[0] = 1.f;
            pr.p[0].awake = 1;

            rbcar_init(&car, 0, col_rb_world(&col), TRACKS[0].x, TRACKS[0].y,
                       TRACKS[0].z, TRACKS[0].yaw);

            /* 2 m ahead, and offset onto a WHEEL LINE.
               The car's proxy is four wheel spheres at +-0.141 m plus three body
               spheres on the centre line, and the body spheres sit high. A 0.10 m
               can placed dead ahead therefore passes cleanly BETWEEN the front
               wheels and under the body -- the fixture was measuring whether the
               car straddles a can, which it does. Offsetting by the half-track
               asks the question that was meant. */
            {
                const float *m = rbcar_matrix(&car);
                pr.p[0].pos[0] = car.body.x[0] + m[8] * 2.0f + m[0] * 0.141f;
                pr.p[0].pos[2] = car.body.x[2] + m[10] * 2.0f + m[2] * 0.141f;
                if (col_ground_at(&col, pr.p[0].pos[0], pr.p[0].pos[2], 1e9f,
                                  &gy, NULL, NULL, NULL))
                    pr.p[0].pos[1] = gy + PROP_MODELS[MODELS[k]].com_y + 0.02f;
                prop_matrix(&pr.p[0], pr.p[0].m);
            }
            memcpy(start, pr.p[0].pos, sizeof start);

            /* PEAK SPEED, not distance travelled.
               Total displacement over three seconds measures how long the car
               BULLDOZES the prop, which favours the big slow cone and reverses
               the ordering being looked for. The impulse is what mass governs,
               so watch the speed the hit imparts. */
            run[k].peak = 0.f;
            for (step = 0; step < 240 * 3; step++) {
                float sp;
                rbcar_step(&car, 1.f, 0.f, 0.f, 0, 1.f / 240.f);
                prop_step(&pr, &car, 1.f / 240.f);
                sp = sqrtf(pr.p[0].v[0] * pr.p[0].v[0]
                           + pr.p[0].v[2] * pr.p[0].v[2]);
                if (sp > run[k].peak) run[k].peak = sp;
            }
            run[k].model = MODELS[k];
            run[k].name = NAMES[k];
            run[k].moved = sqrtf((pr.p[0].pos[0] - start[0])
                                 * (pr.p[0].pos[0] - start[0])
                                 + (pr.p[0].pos[2] - start[2])
                                 * (pr.p[0].pos[2] - start[2]));
        }
        for (k = 0; k < 3; k++)
            ck(run[k].moved > 0.15f, "the car knocks it somewhere",
               "%-7s moved %.2f m", run[k].name, run[k].moved);
        /* The ORDER is the check, never a particular speed.
           This also only means anything because the car impulse uses a two-body
           reduced mass: under the infinite-mass form the delivered speed is
           (1+e)*v_closing for every prop, the mass cancels exactly, and all
           three come out identical. See PROP_CAR_MASS. */
        ck(run[2].peak < run[1].peak,
           "a 1.0 kg cone is thrown slower than a 0.1 kg can by the same hit",
           "cone %.2f m/s, can %.2f m/s", run[2].peak, run[1].peak);
        ck(run[0].peak > run[2].peak,
           "and a 0.15 kg balloon faster than the cone",
           "balloon %.2f m/s, cone %.2f m/s", run[0].peak, run[2].peak);
        col_free(&col);
    } else {
        ck(0, "beach_1.col loads for the car test", "missing");
    }

    /* ---- part 5: restitution actually reaches the solver ---------------- */
    printf("\n-- part 5: coeffHookNorm, as a consequence --\n");
    if (load_track_col(&col, 0)) {
        float ball = bounce_peak(0, &col, 4.f);       /* Ball,  0.7 */
        float can  = bounce_peak(1, &col, 4.f);       /* Cola,  0.3 */
        ck(ball > can,
           "a 0.7-restitution ball bounces higher than a 0.3 can",
           "ball %.3f m, can %.3f m", ball, can);
        col_free(&col);
    }

    /* ---- part 6: wind, and only on the one that asked for it ------------ */
    printf("\n-- part 6: fWindBlow --\n");
    {
        float drift[2];
        int k;
        const int M[2] = { 6, 0 };                    /* Klubok (wind), Ball */
        make_plane(&col, 40.f, 0.f);
        for (k = 0; k < 2; k++) {
            props_t pr;
            float x0, z0;
            int step;
            memset(&pr, 0, sizeof pr);
            pr.col = &col; pr.enabled = 1; pr.n = 1;
            memset(&pr.p[0], 0, sizeof pr.p[0]);
            pr.p[0].model = M[k];
            pr.p[0].q[0] = 1.f;
            pr.p[0].awake = 1;
            pr.p[0].pos[0] = x0 = 0.f;
            pr.p[0].pos[2] = z0 = 0.f;
            pr.p[0].pos[1] = 0.5f;
            prop_matrix(&pr.p[0], pr.p[0].m);
            /* Kept awake: the sleep rule would otherwise stop both of them and
               the comparison would be 0 against 0, which passes for the wrong
               reason. */
            for (step = 0; step < 240 * 3; step++) {
                pr.p[0].awake = 1;
                pr.p[0].rest_t = 0.f;
                prop_step(&pr, NULL, 1.f / 240.f);
            }
            drift[k] = sqrtf((pr.p[0].pos[0] - x0) * (pr.p[0].pos[0] - x0)
                             + (pr.p[0].pos[2] - z0) * (pr.p[0].pos[2] - z0));
        }
        /* On the flat the control must be very nearly still, so this is an
           order AND a floor: "the other one did not move" is half the claim. */
        ck(drift[0] > 0.05f && drift[1] < 0.01f,
           "on the flat only the fWindBlow object drifts",
           "Klubok %.3f m, Ball %.3f m", drift[0], drift[1]);
        col_free(&col);
    }

    /* ---- part 7: the hit EVENT the sound hangs off ---------------------- *
     *
     * prop_t.hit is what main.c turns into sfx_prop_hit, and the one thing it has
     * to get right is that it is an EDGE. `touched` -- which is all this struct
     * used to carry, and it said "telemetry only" -- stays high for as long as the
     * car leans on the object, so a can pinned under a wheel would fire a sound
     * every frame for as long as the player held it there.
     *
     * Driven with the real car on the real grid so the contact is a real one, and
     * the car drives THROUGH the can rather than up to it, which is what a player
     * does. */
    printf("\n-- part 7: the hit event --\n");
    if (load_track_col(&col, 0)) {
        props_t pr;
        rb_car car;
        float gy, peak_speed = 0.f, first_speed = -1.f;
        int step, events = 0, touch_steps = 0, first_step = -1, last_step = -1;
        int off_contact = 0;

        memset(&pr, 0, sizeof pr);
        pr.col = &col;
        pr.enabled = 1;
        pr.n = 1;
        memset(&pr.p[0], 0, sizeof pr.p[0]);
        pr.p[0].model = 1;                          /* Cola, 0.1 kg */
        pr.p[0].q[0] = 1.f;
        pr.p[0].awake = 1;

        rbcar_init(&car, 0, col_rb_world(&col), TRACKS[0].x, TRACKS[0].y,
                   TRACKS[0].z, TRACKS[0].yaw);
        /* On a wheel line, for the reason part 4 spells out: dead ahead the can
           passes between the wheels and under the body and is never touched. */
        {
            const float *m = rbcar_matrix(&car);
            pr.p[0].pos[0] = car.body.x[0] + m[8] * 2.0f + m[0] * 0.141f;
            pr.p[0].pos[2] = car.body.x[2] + m[10] * 2.0f + m[2] * 0.141f;
            if (col_ground_at(&col, pr.p[0].pos[0], pr.p[0].pos[2], 1e9f,
                              &gy, NULL, NULL, NULL))
                pr.p[0].pos[1] = gy + PROP_MODELS[1].com_y + 0.02f;
            prop_matrix(&pr.p[0], pr.p[0].m);
        }

        for (step = 0; step < 240 * 3; step++) {
            rbcar_step(&car, 1.f, 0.f, 0.f, 0, 1.f / 240.f);
            prop_step(&pr, &car, 1.f / 240.f);
            if (pr.p[0].touched) {
                touch_steps++;
                if (first_step < 0) first_step = step;
                last_step = step;
            }
            if (pr.p[0].hit) {
                events++;
                if (first_speed < 0.f) first_speed = pr.p[0].hit_speed;
                if (pr.p[0].hit_speed > peak_speed)
                    peak_speed = pr.p[0].hit_speed;
                if (!pr.p[0].touched) off_contact++;
            }
        }
        printf("  %d contact steps (%d..%d), %d hit events, first at %.2f m/s, "
               "peak %.2f\n", touch_steps, first_step, last_step, events,
               first_speed, peak_speed);

        ck(touch_steps > 0, "the car reaches the can at all",
           "%d contact steps", touch_steps);
        ck(events >= 1, "and raises at least one hit event", "%d", events);
        ck(off_contact == 0, "no hit event fires on a step with no contact",
           "%d did", off_contact);
        /* One knock, or at most a follow-up as it tumbles. Note what this fixture
           CANNOT show: a 0.1 kg can is thrown clear of a 2 kg car in two steps at
           240 Hz, so `touched` is only ever high for those two steps and an
           edge and a copy of `touched` are nearly indistinguishable here. The
           held-contact block below is where that distinction is actually made. */
        ck(events <= 2, "driving through it knocks it once, not continuously",
           "%d events over %d contact steps", events, touch_steps);
        /* The speed the sound is scaled by is a real closing speed: positive, and
           no faster than the car can go (speedBaseMax 27 km/h = 7.5 m/s). */
        ck(first_speed > 0.05f && peak_speed < 9.0f,
           "hit_speed is a plausible closing speed",
           "first %.2f, peak %.2f m/s", first_speed, peak_speed);
        col_free(&col);
    } else {
        ck(0, "beach_1.col loads for the hit-event test", "missing");
    }

    /* And a HELD contact does not re-arm. A can wedged inside a front wheel's
       sphere with the car standing still: contact every step, and it must not
       keep firing. This is the case the edge exists for and the one a
       `hit = touched` mutant cannot survive. */
    if (load_track_col(&col, 0)) {
        props_t pr;
        rb_car car;
        int step, held = 0, held_events = 0;

        memset(&pr, 0, sizeof pr);
        pr.col = &col;
        pr.enabled = 1;
        pr.n = 1;
        memset(&pr.p[0], 0, sizeof pr.p[0]);
        pr.p[0].model = 1;
        pr.p[0].q[0] = 1.f;
        pr.p[0].awake = 1;

        rbcar_init(&car, 0, col_rb_world(&col), TRACKS[0].x, TRACKS[0].y,
                   TRACKS[0].z, TRACKS[0].yaw);
        for (step = 0; step < 240; step++) {
            /* Re-planted on the wheel every step, which is what keeps the contact
               alive: left alone the car's own impulse throws it clear in a frame
               and the run would measure one knock, not a held one. */
            const float *m = rbcar_matrix(&car);
            pr.p[0].pos[0] = car.body.x[0] + m[8] * 0.149f + m[0] * 0.141f;
            pr.p[0].pos[1] = car.body.x[1];
            pr.p[0].pos[2] = car.body.x[2] + m[10] * 0.149f + m[2] * 0.141f;
            pr.p[0].awake = 1;
            prop_matrix(&pr.p[0], pr.p[0].m);
            prop_step(&pr, &car, 1.f / 240.f);
            if (pr.p[0].touched) held++;
            if (pr.p[0].hit) held_events++;
        }
        printf("  wedged under a wheel: %d contact steps, %d events\n",
               held, held_events);
        ck(held > 200, "a can held against a wheel stays in contact",
           "%d steps of 240", held);
        /* THE POINT, and this is the fixture that can make it: one continuous
           contact is ONE knock. `hit = touched` gives 240 events here, so that
           mutant dies on this line and on no other in the file. */
        ck(held_events == 1,
           "and a HELD contact is one event, not one per step",
           "%d events over %d contact steps", held_events, held);
        col_free(&col);
    }

    printf("\n%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
