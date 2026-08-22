/*
 * chrfloat.c -- a PROBE, not a test, in the manner of antheight.c.
 *
 * Three questions the suite cannot currently answer, all of them about the
 * dynamic layer's PLACEMENT rather than its animation:
 *
 *   1. the recorded paths against the real collision grids. chartest part 11
 *      excludes every PATH and WALKER instance -- "the road cars share one dummy
 *      pivot that is not on the ground at all" -- so nothing measures where a
 *      replayed sample actually sits. Reported as (sample y - the surface under
 *      that sample's x,z), over every sample of every path of all ten tracks.
 *   2. what a Crab's collision proxy can reach. Its sphere is r 0.2 at
 *      pivot - 0.18 (0x554b14, verified in the image), so with the pivot dropped
 *      to the ground the sphere's TOP is 0.02 m above it. Printed against the
 *      car's own gathered spheres, settled on flat ground.
 *   3. the same drive chartest part 12 runs at a Woman, run at a Crab.
 *
 * Build:
 *   gcc -I. -Itestgl -O2 ../rccars_re/chrfloat.c char.c scene.c col.c \
 *       carani.c rb.c contact.c collide.c rbcar.c rlog.c \
 *       ../rccars_re/glstub_host.c -lm -o chrfloat
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "char.h"
#include "col.h"
#include "rb.h"
#include "rbcar.h"
#include "tracks.h"

#define ASSETS "assets/"
#define NTRACK 10
static const char *TRACK[NTRACK] = {
    "beach_1", "beach_2", "beach_3", "beach_4", "country_1",
    "country_2", "country_3", "country_4", "urban_1", "urban_2"
};

static float dist3(const float a[3], const float b[3])
{
    float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

static int find_inst(const chr_t *c, const char *name)
{
    unsigned int i;
    for (i = 0; i < c->n_inst; i++)
        if (!strcmp(c->inst[i].place->name, name)) return (int)i;
    return -1;
}

/* --------------------------------------------------- 1. the recorded paths -- */

static void paths(void)
{
    unsigned int t;
    printf("== 1. every recorded sample against the surface under it\n");
    printf("   dy = sample y - the surface UNDER it; float = dy > 0.30 m\n");
    printf("   %-10s %-22s %6s  %7s %7s %7s  %6s %6s %6s\n",
           "track", "path", "n", "min dy", "mean", "max dy", "float", "nosurf",
           "roofed");
    for (t = 0; t < NTRACK; t++) {
        char p[256];
        chr_t c;
        col_t col;
        unsigned int i;
        memset(&col, 0, sizeof col);
        snprintf(p, sizeof p, ASSETS "%s.col", TRACK[t]);
        if (!col_load(p, &col)) { printf("   %s: no .col\n", TRACK[t]); continue; }
        snprintf(p, sizeof p, ASSETS "%s.chr", TRACK[t]);
        if (!char_init(&c, p, TRACK[t], &col)) { col_free(&col); continue; }
        for (i = 0; i < c.n_paths; i++) {
            const chr_path_t *pa = &c.path[i];
            double sum = 0.0;
            float lo = 1e9f, hi = -1e9f;
            unsigned int j, nsurf = 0, under = 0, got = 0, over = 0;
            for (j = 0; j < pa->n; j++) {
                float x = pa->p[j * 3], y = pa->p[j * 3 + 1], z = pa->p[j * 3 + 2];
                float g, nx, ny, nz, dy;
                /* THE SURFACE UNDER THE SAMPLE, and only under it: col_ground_at
                   returns the highest face at or BELOW its ceiling, so a ceiling
                   of y + 8 finds beach_2's overpass instead of its road and every
                   dy comes out negative. 5 cm of slack is the sampling. */
                if (!col_ground_at(&col, x, z, y + 0.05f, &g, &nx, &ny, &nz)) {
                    nsurf++;
                    /* nothing below: is there anything above? */
                    if (col_ground_at(&col, x, z, y + 8.0f, &g, &nx, &ny, &nz))
                        over++;
                    continue;
                }
                dy = y - g;
                if (dy < lo) lo = dy;
                if (dy > hi) hi = dy;
                sum += dy;
                got++;
                if (dy > 0.30f) under++;   /* floating by more than 30 cm */
            }
            printf("   %-10s %-22s %6u  %7.2f %7.2f %7.2f  %5.1f%% %5.1f%% %5.1f%%\n",
                   TRACK[t], pa->name, pa->n,
                   got ? lo : 0.0f, got ? (float)(sum / got) : 0.0f,
                   got ? hi : 0.0f,
                   pa->n ? 100.0f * under / pa->n : 0.0f,
                   pa->n ? 100.0f * nsurf / pa->n : 0.0f,
                   pa->n ? 100.0f * over / pa->n : 0.0f);
        }
        char_free(&c);
        col_free(&col);
    }
}

/* ------------------------------------------------------ 2. the crab's reach -- */

static void crab_reach(void)
{
    col_t col;
    chr_t c;
    rb_car a;
    int ci, k, n;
    float cs[RB_MAX_SPHERES][4], hs[CHR_PROXY_MAX][4];
    float lo = 1e9f, hi = -1e9f;

    printf("\n== 2. what the Crab's sphere can reach\n");
    memset(&col, 0, sizeof col);
    if (!col_load(ASSETS "beach_1.col", &col)) return;
    if (!char_init(&c, ASSETS "beach_1.chr", "beach_1", &col)) {
        col_free(&col); return;
    }
    rbcar_init(&a, 0, col_rb_world(&col), TRACKS[0].x, TRACKS[0].y, TRACKS[0].z,
               TRACKS[0].yaw);
    for (k = 0; k < 120; k++) rbcar_step(&a, 0.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
    n = rb_gather_spheres(&a, cs);
    {
        float g = 0.f, nx, ny, nz;
        col_ground_at(&col, a.body.x[0], a.body.x[2], a.body.x[1] + 1.0f, &g,
                      &nx, &ny, &nz);
        printf("   the car settled: body y %.3f, ground %.3f\n", a.body.x[1], g);
        for (k = 0; k < n; k++) {
            float bot = cs[k][1] - cs[k][3] - g, top = cs[k][1] + cs[k][3] - g;
            if (bot < lo) lo = bot;
            if (top > hi) hi = top;
        }
        printf("   %d car spheres span %.3f .. %.3f m above the ground\n",
               n, lo, hi);
    }
    ci = find_inst(&c, "Crab_NearYacht");
    if (ci >= 0) {
        float g = 0.f, nx, ny, nz;
        const chr_inst_t *in = &c.inst[ci];
        int nh = char_proxy(&c, (unsigned)ci, hs);
        col_ground_at(&col, in->x, in->z, in->y + 1.0f, &g, &nx, &ny, &nz);
        printf("   %s: pivot y %.3f (authored %.3f), ground %.3f, scale %.1f\n",
               in->place->name, in->y, in->place->y, g, in->scale);
        for (k = 0; k < nh; k++)
            printf("   proxy[%d] r %.3f spans %.3f .. %.3f m above the ground\n",
                   k, hs[k][3], hs[k][1] - hs[k][3] - g, hs[k][1] + hs[k][3] - g);
        printf("   the crab model is %.3f m tall drawn (%.3f x scale %.1f)\n",
               (c.model[in->model].bmax[1] - c.model[in->model].bmin[1])
               * in->scale,
               c.model[in->model].bmax[1] - c.model[in->model].bmin[1],
               in->scale);
    }
    char_free(&c);
    col_free(&col);
}

/* ------------------------------------------- 3. drive a real car at a Crab -- */

static void drive_at(const char *who)
{
    col_t col;
    chr_t c;
    rb_car a, b;
    int wi, step, touched = 0, hit_steps = 0;
    float start[3], wx, wy, wz, da, db, closest = 1e9f;

    memset(&col, 0, sizeof col);
    if (!col_load(ASSETS "beach_1.col", &col)) return;
    if (!char_init(&c, ASSETS "beach_1.chr", "beach_1", &col)) {
        col_free(&col); return;
    }
    wi = find_inst(&c, who);
    if (wi < 0) { printf("   no %s\n", who); char_free(&c); col_free(&col); return; }
    {   /* everything else out of the way -- part 12's lesson */
        unsigned int i;
        for (i = 0; i < c.n_inst; i++)
            if ((int)i != wi) { c.inst[i].x += 1000.f; c.inst[i].z += 1000.f; }
    }
    rbcar_init(&a, 0, col_rb_world(&col), TRACKS[0].x, TRACKS[0].y, TRACKS[0].z,
               TRACKS[0].yaw);
    rbcar_init(&b, 0, col_rb_world(&col), TRACKS[0].x, TRACKS[0].y, TRACKS[0].z,
               TRACKS[0].yaw);
    for (step = 0; step < 60; step++) {
        rbcar_step(&a, 0.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
        rbcar_step(&b, 0.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
    }
    {
        const float *cm = rbcar_matrix(&a);
        float g = 0.f, nx, ny, nz;
        wx = cm[12] + cm[8] * 4.0f;
        wz = cm[14] + cm[10] * 4.0f;
        wy = col_ground_at(&col, wx, wz, cm[13] + 1.0f, &g, &nx, &ny, &nz)
                 ? g : cm[13];
    }
    memcpy(start, a.body.x, sizeof start);
    for (step = 0; step < 240; step++) {
        int n;
        float p[3] = { wx, wy, wz }, d;
        c.inst[wi].x = wx; c.inst[wi].y = wy; c.inst[wi].z = wz;
        rbcar_step(&a, 1.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
        n = char_car_solid(&c, &a, NULL);
        touched += n;
        if (n) hit_steps++;
        d = dist3(a.body.x, p);
        if (d < closest) closest = d;
        {   /* the control: nothing solid */
            const chr_proxy_t *save = c.model[c.inst[wi].model].proxy;
            c.model[c.inst[wi].model].proxy = NULL;
            rbcar_step(&b, 1.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
            char_car_solid(&c, &b, NULL);
            c.model[c.inst[wi].model].proxy = save;
        }
    }
    da = dist3(a.body.x, start);
    db = dist3(b.body.x, start);
    printf("   %-16s %3d contacts over %3d steps, travelled %5.2f m against "
           "the control's %5.2f m, closest approach %.2f m\n",
           who, touched, hit_steps, da, db, closest);
    char_free(&c);
    col_free(&col);
}


/* ------------------------------- 4. where a path replayer STARTS the race -- */

/*
 * A PATH or WALKER instance is not dropped and not seeded from its path: it sits
 * on the authored pivot until char_step runs it, and char_step culls anything
 * further than CHR_STEP_DIST from the CAR. So what matters is where that pivot
 * is, how far it is from the path the instance is supposed to be replaying, and
 * whether the car ever comes close enough to start it.
 */
static void pivots(void)
{
    unsigned int t;
    printf("\n== 4. the pivot a path replayer sits on until it is stepped\n");
    for (t = 0; t < NTRACK; t++) {
        char p[256];
        chr_t c;
        col_t col;
        unsigned int i;
        memset(&col, 0, sizeof col);
        snprintf(p, sizeof p, ASSETS "%s.col", TRACK[t]);
        if (!col_load(p, &col)) continue;
        snprintf(p, sizeof p, ASSETS "%s.chr", TRACK[t]);
        if (!char_init(&c, p, TRACK[t], &col)) { col_free(&col); continue; }
        for (i = 0; i < c.n_inst; i++) {
            const chr_inst_t *in = &c.inst[i];
            const chr_path_t *pa;
            float g = 0.0f, nx, ny, nz;
            float near2 = 1e18f, start2;
            unsigned int j;
            int on;
            if (in->place->kind != CHR_KIND_PATH
                && in->place->kind != CHR_KIND_WALKER) continue;
            on = col_ground_at(&col, in->x, in->z, in->y + 0.05f, &g, &nx, &ny, &nz);
            if (in->path >= 0) {
                pa = &c.path[in->path];
                for (j = 0; j < pa->n; j++) {
                    float dx = pa->p[j * 3] - in->x, dz = pa->p[j * 3 + 2] - in->z;
                    float d2 = dx * dx + dz * dz;
                    if (d2 < near2) near2 = d2;
                }
            } else near2 = -1.0f;
            {
                float dx = TRACKS[t].x - in->x, dz = TRACKS[t].z - in->z;
                start2 = dx * dx + dz * dz;
            }
            printf("   %-10s %-20s %-9s pivot (%8.2f %7.2f %8.2f)  ground %s%7.2f "
                   "-> floats %6.2f m   nearest path sample %6.2f m   race start %7.2f m %s\n",
                   TRACK[t], in->place->name, in->place->model,
                   in->x, in->y, in->z, on ? " " : "?", on ? g : 0.0f,
                   on ? in->y - g : 0.0f,
                   near2 >= 0.0f ? sqrtf(near2) : -1.0f, sqrtf(start2),
                   sqrtf(start2) <= CHR_STEP_DIST ? "(in range at the line)" : "");
        }
        char_free(&c);
        col_free(&col);
    }
}

/* ------------------- 5. the burrowers' authored pivots against the ground -- */

/*
 * The CRAB and SPDR proxies are single spheres BELOW the pivot -- 0.18 m for the
 * crab (0x554b14) and CdtShiftY 0.24 m for the spider -- so where the pivot is
 * decides whether the sphere is in the sand or on it. Printed for every crab and
 * spider on every track, with the two holes each one shuttles between, since
 * those carry authored y values of their own.
 */
static void burrowers(void)
{
    unsigned int t;
    printf("\n== 5. the burrowers' authored pivots and holes against the ground\n");
    for (t = 0; t < NTRACK; t++) {
        char p[256];
        chr_t c;
        col_t col;
        unsigned int i;
        memset(&col, 0, sizeof col);
        snprintf(p, sizeof p, ASSETS "%s.col", TRACK[t]);
        if (!col_load(p, &col)) continue;
        snprintf(p, sizeof p, ASSETS "%s.chr", TRACK[t]);
        if (!char_init(&c, p, TRACK[t], &col)) { col_free(&col); continue; }
        for (i = 0; i < c.n_inst; i++) {
            const chr_inst_t *in = &c.inst[i];
            const chr_place_t *pl = in->place;
            float g = 0.0f, ga = 0.0f, gb = 0.0f, nx, ny, nz;
            int on, oa = 0, ob = 0;
            if (pl->kind != CHR_KIND_BURROW) continue;
            /* the pivot's own ground, probed from the AUTHORED y, generous both
               ways -- the pivot floats, so a tight probe from it finds nothing */
            on = col_ground_at(&col, pl->x, pl->z, pl->y + 8.0f, &g, &nx, &ny, &nz);
            if (pl->have & CHR_MARK_A)
                oa = col_ground_at(&col, pl->mark_a.x, pl->mark_a.z,
                                   pl->mark_a.y + 8.0f, &ga, &nx, &ny, &nz);
            if (pl->have & CHR_MARK_B)
                ob = col_ground_at(&col, pl->mark_b.x, pl->mark_b.z,
                                   pl->mark_b.y + 8.0f, &gb, &nx, &ny, &nz);
            printf("   %-10s %-16s %-7s scale %.1f  authored y %7.3f ground %7.3f"
                   " -> floats %6.3f   holeA %6.3f  holeB %6.3f\n",
                   TRACK[t], pl->name, pl->model, pl->scale, pl->y,
                   on ? g : 0.0f, on ? pl->y - g : 0.0f,
                   oa ? pl->mark_a.y - ga : 0.0f, ob ? pl->mark_b.y - gb : 0.0f);
        }
        char_free(&c);
        col_free(&col);
    }
}

int main(void)
{
    paths();
    crab_reach();
    printf("\n== 3. the same drive part 12 runs at a Woman\n");
    drive_at("Woman");
    drive_at("Crab_NearYacht");
    drive_at("Crab_NearStream");
    pivots();
    burrowers();
    return 0;
}
