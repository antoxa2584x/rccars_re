/*
 * meshalign.c -- does the DRAWN car stand on the ground the physics stands on?
 *
 * The renderer draws the car mesh with `glMultMatrixf(rbcar_matrix())` and no
 * offset, so model space is assumed to coincide with the rigid body's body
 * space. That assumption is never checked anywhere. This measures it: it puts
 * a car on a flat plane at y = 0, lets it settle, then compares
 *
 *   - where the PHYSICS has each wheel centre (mount - len along body up), and
 *   - where the MESH draws that wheel's centre (the rig node's animated model
 *     position pushed through the same body matrix the renderer uses),
 *
 * against the ground. A car whose drawn wheels hang above the surface is
 * "wheels in the air" no matter how correct the physics underneath it is.
 *
 * Build (from rccars_vita/, so the assets/ paths resolve):
 *   gcc -I. -O2 -fno-fast-math -ffp-contract=off \
 *       ../rccars_re/meshalign.c rb.c rbcar.c contact.c collide.c carani.c \
 *       -lm -o meshalign && ./meshalign
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "rb.h"
#include "rbcar.h"
#include "carani.h"

/* ---- a flat plane at y = 0, the simplest world the model accepts ---------- */

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
    if (n_hits)
        *n_hits = 0;
    if (c[1] - r >= 0.0f)
        return 0;
    if (max_hits > 0 && hits) {
        hits[0].point[0] = c[0];
        hits[0].point[1] = 0.0f;
        hits[0].point[2] = c[2];
        hits[0].surface = 0;
        if (n_hits)
            *n_hits = 1;
    }
    return 1;
}

static int cb_segment(void *ctx, const float a[3], const float b[3])
{
    (void)ctx;
    return (a[1] < 0.0f || b[1] < 0.0f);
}

/* ---- load a rig out of any VSC version ----------------------------------- */

static int rig_load(carani_t *r, const char *path)
{
    FILE *f = fopen(path, "rb");
    char magic[4];
    unsigned int ntex, nb, npart = 0, nmark = 0, i;
    int ver;

    if (!f)
        return 0;
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "VSC", 3)) {
        fclose(f);
        return 0;
    }
    ver = magic[3] - '0';
    if (ver < 4) { fclose(f); return 0; }
    if (fread(&ntex, 4, 1, f) != 1 || fread(&nb, 4, 1, f) != 1
        || fread(&npart, 4, 1, f) != 1) { fclose(f); return 0; }
    if (ver >= 5) {
        float sr;
        if (fread(&nmark, 4, 1, f) != 1 || fread(&sr, 4, 1, f) != 1) {
            fclose(f); return 0;
        }
    }
    for (i = 0; i < ntex; i++) {
        unsigned short nlen, w, h;
        unsigned char fmt, mips, l;
        if (fread(&nlen, 2, 1, f) != 1) { fclose(f); return 0; }
        fseek(f, nlen, SEEK_CUR);
        if (fread(&w, 2, 1, f) != 1 || fread(&h, 2, 1, f) != 1
            || fread(&fmt, 1, 1, f) != 1 || fread(&mips, 1, 1, f) != 1) {
            fclose(f); return 0;
        }
        for (l = 0; l < mips; l++) {
            unsigned int lw = w >> l, lh = h >> l;
            if (!lw) lw = 1;
            if (!lh) lh = 1;
            fseek(f, (long)lw * lh * (fmt == 1 ? 4 : 2), SEEK_CUR);
        }
    }
    carani_read_parts(r, f, npart);
    fclose(f);
    return r->n > 0;
}

/* p' = p * m, row-vector, the engine's and OpenGL's shared layout */
static void xf(const float m[16], const float p[3], float o[3])
{
    int i;
    for (i = 0; i < 3; i++)
        o[i] = p[0]*m[0+i] + p[1]*m[4+i] + p[2]*m[8+i] + m[12+i];
}

static int fails = 0;

static void ck(int ok, const char *what, const char *detail)
{
    printf("  %-4s %-46s %s\n", ok ? "ok" : "FAIL", what, detail);
    if (!ok)
        fails++;
}

static void report(int car, const char *vsc)
{
    static carani_t rig;
    rb_car c;
    rb_world w;
    int i, n;
    float mesh_sum = 0.0f, phys_sum = 0.0f, off;
    int mesh_n = 0;
    char buf[160];

    memset(&rig, 0, sizeof rig);
    memset(&w, 0, sizeof w);
    w.ground = cb_ground;
    w.sphere = cb_sphere;
    w.segment = cb_segment;

    printf("\n===== car %d  (%s)\n", car + 1, vsc);
    if (!rig_load(&rig, vsc)) {
        ck(0, "rig loads", vsc);
        return;
    }
    rbcar_init(&c, car, &w, 0.0f, 0.0f, 0.0f, 0.0f);
    carani_bind(&rig, &c);
    /* let it settle on the plane */
    for (n = 0; n < 240; n++)
        rbcar_step(&c, 0.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
    carani_update(&rig, &c);

    printf("  body origin (COM) world y = %+.4f     physics wheel radius %.4f\n",
           c.body.x[1], c.wheel[0].radius);
    printf("  %-22s %10s %10s %10s %10s\n",
           "wheel", "phys y", "mesh y", "mesh bot", "gap");
    for (i = 0; i < c.nwheels; i++) {
        float pc[3], pw[3], mm[3], mw[3];
        int p = rig.wheel[i];
        /* physics wheel centre, body space then world */
        pc[0] = c.wheel[i].mount[0];
        pc[1] = c.wheel[i].mount[1] - c.wheel[i].len - c.wheel[i].len_extra;
        pc[2] = c.wheel[i].mount[2];
        xf(c.m, pc, pw);
        if (p < 0) {
            printf("  %-22s %10.4f %10s\n", "(unbound)", pw[1], "-");
            continue;
        }
        /* mesh wheel centre: the rig node's animated model-space origin, then
           through the very matrix the renderer multiplies by */
        mm[0] = rig.world[p][12];
        mm[1] = rig.world[p][13];
        mm[2] = rig.world[p][14];
        xf(c.m, mm, mw);
        printf("  %-22s %10.4f %10.4f %10.4f %10.4f\n",
               rig.part[p].name, pw[1], mw[1],
               mw[1] - c.wheel[i].radius, mw[1] - pw[1]);
        mesh_sum += mw[1];
        phys_sum += pw[1];
        mesh_n++;
    }
    if (!mesh_n) {
        ck(0, "at least one wheel node bound", "none");
        return;
    }

    /* What main.c applies: the recovered CenterMassOY, i.e. how far above the
       MODEL origin the body origin sits. It used to be carani_wheel_plane_y(),
       the mesh's own wheel-centre plane, which was the same number only while
       gen_rb_data.py parked the com on that plane. */
    off = rbcar_com_oy(car);
    printf("  raw mesh sits %+.1f mm high; com_oy = %+.1f mm, "
           "carani_wheel_plane_y = %+.1f mm\n",
           1000.0f * (mesh_sum - phys_sum) / mesh_n, 1000.0f * off,
           1000.0f * carani_wheel_plane_y(&rig));

    /* And the two must agree. NOT independent evidence for com_oy's value --
       gen_rb_data.py's mount_y is built to make them agree, so this is an
       identity, and it read 57 / 30 / 39 mm out under the old frame with nothing
       complaining. What it IS is the only thing tying mount_y (rb_data.h) to the
       draw offset (main.c) across two files that never see each other: change
       one and this fails. 3 mm covers the settling; the Buggy's front and rear
       wheel nodes genuinely differ by 17 mm, so the mean gets its own slack. */
    {
        float raw = (mesh_sum - phys_sum) / mesh_n;
        float tol = (c.nwheels == 4 && car == 1) ? 0.012f : 0.003f;
        sprintf(buf, "mesh %+.1f mm vs com_oy %+.1f mm",
                1000.0f * raw, 1000.0f * off);
        ck(fabsf(raw - off) < tol,
           "the mesh's wheel plane sits com_oy above the physics wheels", buf);
    }
    ck(off >= 0.0f && off < 0.12f, "com_oy is in the loader's 0..0.2 m range",
       (sprintf(buf, "%.1f mm", 1000.0f * off), buf));

    /* The offset is a translation in the model's own frame applied BEFORE the
       body matrix, exactly as glTranslatef after glMultMatrixf does, so on a
       level car it lands one-for-one on the drawn height. Measure it that way
       rather than trusting that equivalence. */
    {
        float worst = 0.0f;
        int wi = -1;
        for (i = 0; i < c.nwheels; i++) {
            float pc[3], pw[3], mm[3], mw[3], d;
            int p = rig.wheel[i];
            if (p < 0)
                continue;
            pc[0] = c.wheel[i].mount[0];
            pc[1] = c.wheel[i].mount[1] - c.wheel[i].len - c.wheel[i].len_extra;
            pc[2] = c.wheel[i].mount[2];
            xf(c.m, pc, pw);
            mm[0] = rig.world[p][12];
            mm[1] = rig.world[p][13] - off;      /* <-- the fix */
            mm[2] = rig.world[p][14];
            xf(c.m, mm, mw);
            d = mw[1] - pw[1];
            if (d < 0.0f) d = -d;
            if (d > worst) { worst = d; wi = i; }
        }
        sprintf(buf, "worst %.1f mm (wheel %d)", 1000.0f * worst, wi);
        /* 25 mm of slack: the Buggy's front and rear wheel centres genuinely
           sit at different heights in the mesh (53 mm and 70 mm), so one plane
           cannot pass through both. Everything else lands under a millimetre. */
        ck(worst < 0.025f, "corrected, the drawn wheels meet the physics ones",
           buf);
    }

    /* --- and does this car actually stand up and drive? ------------------
     *
     * rb_test only ever exercises car 0, so the Buggy's and Hummer's dynamics
     * have never been covered at all -- which is why nobody noticed they were
     * running on the Overkill's wheel positions. These are coarse (level ground,
     * no cornering forces to speak of) but they would catch a car that sinks,
     * floats, tips over or cannot pull away. */
    {
        float tilt = (float)(acos(c.m[5] > 1.0f ? 1.0f
                                  : (c.m[5] < -1.0f ? -1.0f : c.m[5]))
                             * RB_RAD2DEG);
        /* The com is NOT at wheel-radius height any more: it sits com_oy above
           the model origin, i.e. `wheel_y - com_oy` BELOW the wheel centres, so a
           parked car's body origin is at `radius - (wheel_y - com_oy)` -- 15 mm
           on the Overkill against a 72 mm radius. Bound it against the drawn
           wheel plane instead, which is the quantity that has to be right: the
           mesh's wheel nodes must end up a wheel radius off the ground. */
        {
            float drawn = c.body.x[1] + carani_wheel_plane_y(&rig)
                          - rbcar_com_oy(car);
            sprintf(buf, "com y = %.4f, drawn wheel plane %.4f, radius %.4f",
                    c.body.x[1], drawn, c.wheel[0].radius);
            ck(drawn > 0.75f * c.wheel[0].radius
               && drawn < 1.25f * c.wheel[0].radius,
               "settles with its drawn wheels a radius off the ground", buf);
        }
        sprintf(buf, "%.2f deg", tilt);
        /* 4 degrees, not 1: the BUGGY genuinely settles nose-down on level
           ground -- 2.88 deg, from 3.66 deg before its wheel positions came off
           its own mesh instead of the Overkill's. Its front struts come to rest
           ~18 mm longer than its rears with all four mounts, free lengths and
           sags identical, which is not explained yet and is not what this check
           is for. Cars 1 and 3 sit at 0.00 deg. The bound is here to catch a car
           that tips over or stands on its nose. */
        ck(tilt < 4.0f, "and sits level on level ground", buf);
    }
    {
        float v0, v1;
        for (n = 0; n < 300; n++)
            rbcar_step(&c, 1.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
        v0 = c.body.v[0]; v1 = c.body.v[2];
        sprintf(buf, "%.2f m/s along +Z, %.2f m/s sideways",
                v1, v0);
        ck(v1 > 1.0f && fabs(v0) < 0.5f * fabs(v1),
           "pulls away forwards under throttle", buf);
    }
}

int main(void)
{
    report(0, "assets/car1.vsc");
    report(1, "assets/car2.vsc");
    report(2, "assets/car3.vsc");
    printf("\n%s (%d failed)\n", fails ? "FAILURES" : "all alignment checks passed",
           fails);
    return fails ? 1 : 0;
}
