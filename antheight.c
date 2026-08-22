/* Where is the antenna, and where is the collision proxy?
 *
 * Reported: "seems like the car still hits roofs with the antenna." The antenna
 * is render-only -- it is not in rb_gather_spheres, which is the sole source of
 * every collision proxy in the port -- so the question worth answering is what
 * the car IS hitting roofs with, and whether that thing happens to sit at the
 * antenna's height. If it does, the two are indistinguishable on screen.
 *
 * Everything is reported as a height above the ground the parked car stands on,
 * so the drawn car and the collision proxy are directly comparable:
 *
 *     world_y = com_y + (model_y - wheel_plane_y)
 *
 * which is exactly what main.c draws -- glMultMatrixf(rb_car.m) then
 * glTranslatef(0, -carani_wheel_plane_y(&rig), 0).
 *
 * Run from rccars_vita/:
 *   gcc -I. -Itestgl -O2 -fno-fast-math -ffp-contract=off \
 *       ../rccars_re/antheight.c scene.c antenna.c carani.c col.c rb.c rbcar.c \
 *       contact.c collide.c rlog.c -lm -o antheight
 */
#include "scene.h"
#include "antenna.h"
#include "carani.h"
#include "col.h"
#include "rbcar.h"
#include "prop.h"
#include "prop_data.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DT (1.0f / 60.0f)
#define HALF 20.0f

static float g_tris[2 * 9];
static unsigned int *g_start, *g_idx;

/* a bare floor at y = 0, outward normal +Y */
static void build(col_t *c)
{
    static const float quad[2][9] = {
        { -HALF, 0.f, -HALF,   -HALF, 0.f,  HALF,    HALF, 0.f, -HALF },
        {  HALF, 0.f,  HALF,    HALF, 0.f, -HALF,   -HALF, 0.f,  HALF },
    };
    unsigned int ncell, i, k;

    memset(c, 0, sizeof(*c));
    c->minx = -HALF; c->minz = -HALF; c->cell = 3.0f;
    c->nx = (unsigned)(2 * HALF / 3.0f) + 1;
    c->nz = c->nx;
    c->ntris = 2;
    memcpy(g_tris, quad, sizeof(quad));
    c->tris = g_tris;

    ncell = c->nx * c->nz + 1;
    g_start = malloc(ncell * sizeof(unsigned int));
    g_idx = malloc((size_t)(ncell - 1) * 2 * sizeof(unsigned int));
    k = 0;
    for (i = 0; i + 1 < ncell; i++) {
        g_start[i] = k; g_idx[k++] = 0; g_idx[k++] = 1;
    }
    g_start[ncell - 1] = k;
    c->start = g_start; c->idx = g_idx;
}

int main(void)
{
    static const char *files[3] = { "assets/car1.vsc", "assets/car2.vsc",
                                    "assets/car3.vsc" };
    static const char *names[3] = { "Overkill", "Buggy", "Hummer" };
    int car;

    printf("== antenna height vs collision proxy height, above the ground ==\n");

    for (car = 0; car < 3; car++) {
        scene_t sc;
        antenna_t ant;
        col_t col;
        rb_car c;
        float s[RB_MAX_SPHERES][4];
        float plane, com, ant_base = 0.f, ant_tip = 0.f, mesh_top = -1e30f;
        int n, i, ok;

        memset(&sc, 0, sizeof(sc));
        if (!scene_load(files[car], &sc)) {
            printf("\n%s: %s did not load -- skipped\n", names[car], files[car]);
            continue;
        }
        antenna_init(&ant, &sc, car);

        /* the tallest drawn vertex in the whole car, antenna included */
        for (i = 0; i < (int)sc.n_batches; i++) {
            const batch_t *b = &sc.batches[i];
            const vtx_t *v = b->rest ? b->rest : b->verts;
            unsigned int k;
            if (!v) continue;
            for (k = 0; k < b->nverts; k++)
                if (v[k].y > mesh_top) mesh_top = v[k].y;
        }

        build(&col);
        rbcar_init(&c, car, col_rb_world(&col), 0.f, 0.02f, 0.f, 0.f);
        for (i = 0; i < 240; i++) rbcar_step(&c, 0.f, 0.f, 0.f, 0, DT);
        com = c.body.x[1];
        n = rb_gather_spheres(&c, s);
        /* the rig resolves wheel node <-> rb wheel from the MOUNTS, so it needs
           the initialised car -- same order main.c uses in respawn() */
        carani_bind(&sc.rig, &c);
        plane = carani_wheel_plane_y(&sc.rig);

        printf("\n%s  (com stands at %.3f m, wheel plane %.4f in the model)\n",
               names[car], com, plane);

        ok = ant.ready;
        if (ok) {
            ant_base = com + (ant.base_y - plane);
            ant_tip  = com + (ant.tip_y  - plane);
            printf("  antenna mesh   : %.3f -> %.3f m   (RENDER ONLY -- it is in"
                   " no collision proxy)\n", ant_base, ant_tip);
        } else {
            printf("  antenna mesh   : no ANTENNA part in this .vsc\n");
        }
        printf("  tallest drawn vertex on the whole car: %.3f m\n",
               com + (mesh_top - plane));

        for (i = c.nwheels; i < n; i++)
            printf("  body sphere %d  : %.3f -> %.3f m   centre (%+.3f %+.3f "
                   "%+.3f) r %.4f\n", i - c.nwheels,
                   s[i][1] - s[i][3], s[i][1] + s[i][3],
                   s[i][0], s[i][1], s[i][2], s[i][3]);

        if (ok && n > c.nwheels) {
            float top = s[c.nwheels][1] + s[c.nwheels][3];
            float bot = s[c.nwheels][1] - s[c.nwheels][3];
            int overlaps = (top >= ant_base && bot <= ant_tip);
            printf("  --> the proxy %s the antenna's height band"
                   " (%.3f-%.3f vs %.3f-%.3f)\n",
                   overlaps ? "SITS INSIDE" : "does not reach",
                   bot, top, ant_base, ant_tip);
            printf("  --> a roof is felt at %.3f m; the car's own bodywork tops"
                   " out at %.3f m\n", top, com + (mesh_top - plane) < ant_tip
                   ? com + (mesh_top - plane) : ant_tip);
        }

        free(g_start); free(g_idx);
        memset(&col, 0, sizeof(col));
        scene_release(&sc);

        /* Reported separately: "hitable objects hit only with wheels, not body."
           prop.c collides props against rb_gather_spheres -- the same list -- so
           whether a body sphere can EVER touch a prop is arithmetic: does the
           body band reach down to the top of a prop standing on the ground? */
        if (n > c.nwheels) {
            float blo = s[c.nwheels][1] - s[c.nwheels][3];
            unsigned int mi;
            int reachable = 0, total = 0;
            float tallest = 0.f;
            const char *tallest_name = "";

            for (mi = 0; mi < sizeof(PROP_MODELS)/sizeof(PROP_MODELS[0]); mi++) {
                const prop_model_t *pm = &PROP_MODELS[mi];
                float lo = 1e30f, hi = -1e30f, h;
                int k;
                for (k = 0; k < pm->n_spheres; k++) {
                    float oy = pm->sphere[k][1], r = pm->sphere[k][3];
                    if (oy - r < lo) lo = oy - r;
                    if (oy + r > hi) hi = oy + r;
                }
                h = hi - lo;                    /* its proxy's total height */
                total++;
                if (h > tallest) { tallest = h; tallest_name = pm->name; }
                if (h >= blo) reachable++;
            }
            printf("  props on the same ground: tallest proxy is %s at %.3f m,"
                   " body band starts at %.3f m\n",
                   tallest_name, tallest, blo);
            printf("  --> %d of %d prop models can reach a body sphere at all;"
                   " the rest are hittable ONLY by wheels\n", reachable, total);
        }
    }
    return 0;
}
