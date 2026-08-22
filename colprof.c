/* colprof.c -- where the sim time goes, on the REAL grids, at the places a real
 * Vita said it went.
 *
 * A 545-frame hardware log put the draw side at 69% memcpy and the sim at a flat
 * 11 ms (see "The frame was 69% memcpy"). A second log, this time driving into
 * the scenery, says the opposite: draw held at 5.7 ms all session while `sim`
 * spiked to 34, 70 and 111 ms, always with the car in one of beach_1's dense
 * cells. `draw` did not move, `swap` went to zero -- the GPU was idle and the
 * CPU was in the collision queries.
 *
 * This harness is the sim-side counterpart of the draw log's regression: it puts
 * the real car on a real .col at named positions and reports, per 1/60 tick,
 *
 *   - wall-clock microseconds
 *   - how many sphere / segment / ground queries the tick issued
 *   - how many triangles those queries VISITED, and how many reached the
 *     closest-point test (col.h's `narrow`)
 *
 * so a fix can be shown to move the number it claims to move. Positions come
 * from the hardware log's own `pos=` lines at its worst frames; keep them, they
 * are the reported bug.
 *
 * Run from rccars_vita/ with assets/ populated:
 *
 *   gcc -I. -O2 -fno-fast-math -ffp-contract=off -DCOL_PROFILE \
 *       ../rccars_re/colprof.c col.c rb.c rbcar.c contact.c collide.c \
 *       carani.c prop.c -lm -o colprof
 *   ./colprof [track]
 */
#include "col.h"
#include "rbcar.h"
#include "rb_data.h"
#include "prop.h"
#include "prop_data.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#define DT (1.0f / 60.0f)

/* prop.c reaches the scene only to draw, and prop_init is handed scene = NULL
   here, so neither is ever called -- same two stubs proptest.c carries, and they
   say so out loud rather than returning quietly. */
int scene_model_index(const scene_t *s, const char *name)
{ (void)s; (void)name; printf("  !! scene_model_index reached with no scene\n"); return -1; }
void scene_draw_model(const scene_t *s, unsigned int m, const float mat[16])
{ (void)s; (void)m; (void)mat; printf("  !! scene_draw_model reached with no scene\n"); }

typedef struct {
    const char *what;
    float x, y, z, yaw;
} spot;

/* beach_1, straight off the hardware log's `pos=` line at each of its worst
 * frames -- the number after the name is the sim time that frame took. The
 * parked start is the control: the car is asleep there (FUN_004f6610), which is
 * why it costs 0.7 ms while everything else costs tens. */
static const spot beach1[] = {
    { "race start (asleep, 0.7 ms)",   -9.03f, -0.09f, -55.57f,  13.f },
    { "dense cell 520 tris (111 ms)",   0.02f,  2.25f,  -8.89f,  -5.f },
    { "dense cell 582 tris (82 ms)",    3.32f,  1.27f,  18.18f,   0.f },
    { "dense cell 556 tris (70 ms)",   11.38f,  2.24f, -22.38f,  25.f },
    { "dense cell 379 tris (80 ms)",  -17.60f,  0.05f,  -4.09f,   0.f },
    { "dense cell 318 tris (66 ms)",  -18.49f,  0.00f,  -9.01f,   0.f },
    { "dense cell 321 tris (44 ms)",    2.87f,  1.25f,  42.24f,   0.f },
};

static double now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

/* Ticks per spot. 60 = one second of driving, which is long enough for the
 * suspension to settle out of the teleport and short enough not to leave the
 * cell being measured. */
#define TICKS 60

static void run(col_t *col, const spot *s, int with_props, props_t *pw)
{
    rb_car c;
    col_prof_t before;
    double t0, t1;
    int i;

    rbcar_init(&c, 1, col_rb_world(col), s->x, s->y, s->z, s->yaw);
    /* Settle first, and NOT under the counters: a teleported car spends its
       first few ticks resolving whatever it was dropped into, which is not what
       a driving frame costs. */
    for (i = 0; i < 30; i++)
        rbcar_step(&c, 0.0f, 0.0f, 0.0f, 0, DT);
    if (with_props)
        prop_reset(pw);

    before = col_prof;
    t0 = now_us();
    for (i = 0; i < TICKS; i++) {
        rbcar_step(&c, 1.0f, 0.0f, 0.2f, 0, DT);
        if (with_props)
            prop_step(pw, &c, DT);
    }
    t1 = now_us();

#define D(f) ((double)(col_prof.f - before.f) / TICKS)
    printf("  %-32s %7.0f us  %6.1f sph %5.1f seg %5.1f gnd  "
           "%8.0f tris %8.0f narrow\n",
           s->what, (t1 - t0) / TICKS,
           D(sphere), D(segment), D(ground), D(tris), D(narrow));
#undef D
}

/* The log samples one frame in sixty, so the seven positions above are the worst
 * frames it HAPPENED to print, not the worst the track has. This walks every
 * non-empty cell in the grid, parks the car on the surface there and drives it
 * for a few ticks, and reports the tail of the distribution -- which is what a
 * hitch is. Positions where the car cannot be placed (no ground under the cell
 * centre, or the sea) are skipped and counted.
 */
static void sweep(col_t *col)
{
    unsigned int ix, iz, placed = 0, skipped = 0;
    double worst = 0.0, worst_x = 0, worst_z = 0, sum = 0.0;
    double hist[6] = {0,0,0,0,0,0};   /* <0.5, <1, <2, <4, <8, >=8 ms on a Vita */
    unsigned long worst_tris = 0;

    for (iz = 0; iz < col->nz; iz++) {
        for (ix = 0; ix < col->nx; ix++) {
            unsigned int cell = iz * col->nx + ix;
            float x, z, gy, gn[3];
            rb_car c;
            col_prof_t before;
            double t0, t1, us;
            int i;

            if (col->start[cell + 1] == col->start[cell])
                continue;
            x = col->minx + (ix + 0.5f) * col->cell;
            z = col->minz + (iz + 0.5f) * col->cell;
            if (!col_ground_at(col, x, z, 1e9f, &gy, &gn[0], &gn[1], &gn[2])) {
                skipped++;
                continue;
            }
            rbcar_init(&c, 1, col_rb_world(col), x, gy + 0.05f, z, 0.f);
            for (i = 0; i < 20; i++)
                rbcar_step(&c, 0.0f, 0.0f, 0.0f, 0, DT);
            /* Fell out of the world, or was never on it: not a drivable spot. */
            if (c.body.x[1] < gy - 5.0f) { skipped++; continue; }

            before = col_prof;
            t0 = now_us();
            for (i = 0; i < 10; i++)
                rbcar_step(&c, 1.0f, 0.0f, 0.1f, 0, DT);
            t1 = now_us();
            us = (t1 - t0) / 10.0;
            placed++;
            sum += us;
            /* Bucket by the VITA cost, not the host's: the hardware log's mean
               sim of 11.2 ms against this harness's 90 us for the same driving
               puts the ARM at ~62x this machine per tick. Bucket edges are
               therefore host microseconds / 62. */
            if (us < 8.0) hist[0]++;
            else if (us < 16.0) hist[1]++;
            else if (us < 32.0) hist[2]++;
            else if (us < 65.0) hist[3]++;
            else if (us < 130.0) hist[4]++;
            else hist[5]++;
            if (us > worst) {
                worst = us; worst_x = x; worst_z = z;
                worst_tris = col_prof.tris - before.tris;
            }
        }
    }
    printf("  %u spots driven, %u unplaceable\n", placed, skipped);
    printf("  mean %.0f us/tick, worst %.0f us at (%.1f, %.1f) "
           "visiting %lu tris/tick\n", placed ? sum / placed : 0.0,
           worst, worst_x, worst_z, worst_tris / 10);
    printf("  est. Vita ms/tick   <0.5:%4.0f%%  <1:%4.0f%%  <2:%4.0f%%  "
           "<4:%4.0f%%  <8:%4.0f%%  8+:%4.0f%%\n",
           100*hist[0]/placed, 100*hist[1]/placed, 100*hist[2]/placed,
           100*hist[3]/placed, 100*hist[4]/placed, 100*hist[5]/placed);
}

/* ------------------------------------------------------------------------- */
/* the checks -- a broad phase that rejects too much is a hole in the world   */
/* ------------------------------------------------------------------------- */

static int fails;

static void ck(int ok, const char *what, const char *fmt, ...)
{
    va_list ap;
    printf("  %-4s %-56s ", ok ? "ok" : "FAIL", what);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    if (!ok)
        fails++;
}

/* An INDEPENDENT lower bound on "is there something within r of this point".
 *
 * Deliberately not a second copy of tri_closest: it samples each triangle on a
 * barycentric lattice, so it can only ever find a point the true closest-point
 * test would also have found, at a distance the true test can only beat. That
 * makes it a one-way oracle, which is the useful direction here -- if the
 * sampler finds a point inside the sphere then col_sphere MUST report a hit, and
 * no amount of agreement between two copies of the same algorithm would have
 * shown that. Returns the deepest sampled penetration, 0 for none.
 *
 * `N` is the lattice density. 8 gives 45 samples per triangle; the collision
 * triangles here have a median edge of 0.36 m, so that is a sample every few
 * centimetres against a 0.07 m wheel.
 */
#define LATTICE 8
static double sampled_depth(const col_t *c, const float p[3], float r)
{
    unsigned int cell, k;
    int cx, cz, ix, iz, i, j;
    double worst = 0.0;

    cx = (int)((p[0] - c->minx) / c->cell);
    cz = (int)((p[2] - c->minz) / c->cell);
    for (iz = cz - 1; iz <= cz + 1; iz++) {
        for (ix = cx - 1; ix <= cx + 1; ix++) {
            if (ix < 0 || iz < 0 || (unsigned)ix >= c->nx || (unsigned)iz >= c->nz)
                continue;
            cell = (unsigned)iz * c->nx + (unsigned)ix;
            for (k = c->start[cell]; k < c->start[cell + 1]; k++) {
                const float *t = &c->tris[(size_t)c->idx[k] * 9];
                for (i = 0; i <= LATTICE; i++) {
                    for (j = 0; i + j <= LATTICE; j++) {
                        double u = (double)i / LATTICE, v = (double)j / LATTICE;
                        double w = 1.0 - u - v, d2 = 0.0;
                        int a;
                        for (a = 0; a < 3; a++) {
                            double q = u * t[a] + v * t[3 + a] + w * t[6 + a];
                            double e = q - p[a];
                            d2 += e * e;
                        }
                        if (d2 < (double)r * r) {
                            double d = (double)r - sqrt(d2);
                            if (d > worst) worst = d;
                        }
                    }
                }
            }
        }
    }
    return worst;
}

static void checks(col_t *col)
{
    unsigned int ix, iz, probes = 0, oracle_hits = 0, missed = 0, shallow = 0;
    unsigned long tris_seen, narrow_seen;
    double worst_short = 0.0;
    col_prof_t before;

    /* Probe every non-empty cell at three heights around whatever surface is
       there, with a wheel-sized sphere. */
    for (iz = 0; iz < col->nz; iz++) {
        for (ix = 0; ix < col->nx; ix++) {
            unsigned int cell = iz * col->nx + ix;
            float gy, gn[3], p[3];
            int step;

            if (col->start[cell + 1] == col->start[cell])
                continue;
            p[0] = col->minx + (ix + 0.5f) * col->cell;
            p[2] = col->minz + (iz + 0.5f) * col->cell;
            if (!col_ground_at(col, p[0], p[2], 1e9f, &gy, &gn[0], &gn[1], &gn[2]))
                continue;

            for (step = -1; step <= 1; step++) {
                rb_world_hit hits[8];
                int nh = 0, got, h;
                double deep = 0.0, oracle;
                const float r = 0.07f;           /* the Overkill's wheel */

                p[1] = gy + step * 0.05f;
                probes++;
                got = col_sphere(col, p, r, hits, 8, &nh);
                for (h = 0; h < nh; h++) {
                    double dx = p[0] - hits[h].point[0];
                    double dy = p[1] - hits[h].point[1];
                    double dz = p[2] - hits[h].point[2];
                    double d = r - sqrt(dx*dx + dy*dy + dz*dz);
                    if (d > deep) deep = d;
                }
                oracle = sampled_depth(col, p, r);
                if (oracle > 0.0) {
                    oracle_hits++;
                    if (!got) {
                        missed++;
                    } else if (deep < oracle - 1e-4) {
                        /* The lattice found a deeper point than the query did,
                           which a true closest-point test cannot allow. */
                        shallow++;
                        if (oracle - deep > worst_short)
                            worst_short = oracle - deep;
                    }
                }
            }
        }
    }
    ck(oracle_hits > 100, "the oracle found geometry to find",
       "%u of %u probes are inside something", oracle_hits, probes);
    ck(missed == 0, "col_sphere reports every hit an independent sampler finds",
       "%u missed of %u", missed, oracle_hits);
    ck(shallow == 0, "and never shallower than the sampler's deepest point",
       "%u short, worst by %.4f m", shallow, worst_short);

    /* And the reject has to actually reject: this is the performance claim, and
       the bound is set from all ten shipped grids rather than from one.
       Measured, on the dense cells of each: 0.9, 0.9, 0.9, 1.0, 1.0, 1.1, 1.1,
       1.2, 1.2, 1.2%. A 0.07 m sphere in a 3 m column of a whole level simply
       cannot reach most of it.
       2% is therefore ~1.7x the worst real track and still tight enough to catch
       the mutations that matter: dropping BOTH rejects gives 47.5%, and dropping
       a SINGLE axis -- which is exact, and so invisible to the three checks
       above -- gives 2.9%. A looser 10% let that one live. */
    before = col_prof;
    {
        unsigned int n = 0;
        for (iz = 0; iz < col->nz && n < 200; iz++)
            for (ix = 0; ix < col->nx && n < 200; ix++) {
                unsigned int cell = iz * col->nx + ix;
                float gy, gn[3], p[3];
                rb_world_hit hits[8];
                int nh = 0;
                if (col->start[cell + 1] - col->start[cell] < 100)
                    continue;                     /* dense cells only */
                p[0] = col->minx + (ix + 0.5f) * col->cell;
                p[2] = col->minz + (iz + 0.5f) * col->cell;
                if (!col_ground_at(col, p[0], p[2], 1e9f, &gy, &gn[0], &gn[1], &gn[2]))
                    continue;
                p[1] = gy;
                col_sphere(col, p, 0.07f, hits, 8, &nh);
                n++;
            }
    }
    tris_seen = col_prof.tris - before.tris;
    narrow_seen = col_prof.narrow - before.narrow;
    ck(tris_seen > 1000, "the dense-cell sample visited a real workload",
       "%lu triangles", tris_seen);
    ck(narrow_seen * 50 < tris_seen,
       "the broad phase rejects the column a wheel cannot reach",
       "%lu of %lu triangles reached the closest-point test (%.1f%%)",
       narrow_seen, tris_seen, tris_seen ? 100.0 * narrow_seen / tris_seen : 0.0);
}

int main(int argc, char **argv)
{
    const char *track = argc > 1 ? argv[1] : "beach_1";
    int track_idx = argc > 2 ? atoi(argv[2]) : 0;   /* PROP_TRACKS row */
    char path[256];
    col_t col;
    props_t pw;
    unsigned int i, ncell, nonempty = 0, worst = 0;
    double mean = 0.0;

    /* A bare name is a track in assets/; anything ending in .col is a path, so a
       trial repack at a different cell size can be measured without disturbing
       the shipped grid. */
    if (strlen(track) > 4 && !strcmp(track + strlen(track) - 4, ".col"))
        snprintf(path, sizeof(path), "%s", track);
    else
        snprintf(path, sizeof(path), "assets/%s.col", track);
    if (!col_load(path, &col)) {
        printf("FAILED to load %s\n", path);
        return 2;
    }
    ncell = col.nx * col.nz;
    for (i = 0; i < ncell; i++) {
        unsigned int n = col.start[i + 1] - col.start[i];
        if (!n) continue;
        nonempty++;
        mean += n;
        if (n > worst) worst = n;
    }
    printf("%s: %u tris, %ux%u cells of %.2f m, %u non-empty, "
           "mean %.0f tris/cell, worst %u\n\n",
           track, col.ntris, col.nx, col.nz, col.cell, nonempty,
           nonempty ? mean / nonempty : 0.0, worst);

    printf("per 1/60 tick, car only:\n");
    for (i = 0; i < sizeof(beach1) / sizeof(beach1[0]); i++)
        run(&col, &beach1[i], 0, &pw);

    /* The props are the other half of the report ("collision with map objects
       like chairs"): each one runs its own ground query per tick, and prop_step
       is called on the same banked time the car is. */
    memset(&pw, 0, sizeof(pw));
    prop_init(&pw, NULL, &col, track_idx);
    printf("\nper 1/60 tick, with %d props placed:\n", pw.n);
    for (i = 0; i < sizeof(beach1) / sizeof(beach1[0]); i++)
        run(&col, &beach1[i], 1, &pw);

    printf("\nevery drivable cell in the grid:\n");
    sweep(&col);

    printf("\nthe broad phase must not change the answer:\n");
    checks(&col);

    col_free(&col);
    printf("\n%s\n", fails ? "SOME CHECKS FAILED" : "all colprof checks passed");
    return fails ? 1 : 0;
}
