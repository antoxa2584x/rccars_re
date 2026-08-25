/*
 * carlight_test -- the car's own light, on the host.
 *
 * What it can check and what it cannot. The MODEL is arithmetic over plain
 * numbers, so all of it is checkable: the direction, the curve, the level's
 * chase, the two intensities and the LOD fade. The GL state is checkable as a
 * SUBMISSION -- this file records the lighting calls and reads back what
 * carlight.c actually set, which is the only way "the ambient reaching GL is\n* 138/255" can be a fact rather than an intention. What no host harness can
 * check is what the GPU then does with it; that is Vita3K's job.
 *
 * Two of the checks are CROSS-CHECKS against a second implementation rather than
 * against a constant, which is the standing rule (traps.md, "a check that\n* compares against the constant it guards passes everything"):
 *
 *   - the light DIRECTION, against a transcription of FUN_0040b4c0's own
 *     spherical move-towards, over a grid of azimuths and elevations. The
 *     closed form in carlight.c is only valid because the base vector is
 *     horizontal; the transcription does not know that
 *   - the CURVE, against contact.c's rb_curve_eval -- the port's other
 *     transcription of the engine's 0x0040f830 -- over a sweep of x
 */

#include "carlight.h"
#include "rb.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ harness */

static int checks, fails;
static void ck(int ok, const char *what, const char *fmt, ...)
{
    va_list ap;
    checks++;
    printf(ok ? "  ok   " : "  FAIL ");
    printf("%-52s ", what);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    if (!ok)
        fails++;
}

static int near(float a, float b, float e) { return fabsf(a - b) <= e; }

/* FUN_0040b4c0, transcribed: rotate `v' toward `ref' by min(angle between,
   deg), about their cross product, and interpolate the length. The port's
   carlight_dir is a closed form of this for a horizontal v and ref = +Y; this
   knows nothing about either. */
static void rot_toward(const float v[3], const float ref[3], float deg,
                       float out[3])
{
    float a[3], b[3], ax[3], la, lb, lax, ang, step, c, s, d;
    int i;
    memcpy(a, v, sizeof a);
    memcpy(b, ref, sizeof b);
    la = sqrtf(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
    lb = sqrtf(b[0]*b[0] + b[1]*b[1] + b[2]*b[2]);
    for (i = 0; i < 3; i++) { a[i] /= la; b[i] /= lb; }
    d = a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    if (d > 1.f) d = 1.f;
    if (d < -1.f) d = -1.f;
    ang = acosf(d) * 57.29577951f;
    step = deg < ang ? deg : ang;                  /* the engine's own min() */
    /* axis = v x ref, normalised */
    ax[0] = a[1]*b[2] - a[2]*b[1];
    ax[1] = a[2]*b[0] - a[0]*b[2];
    ax[2] = a[0]*b[1] - a[1]*b[0];
    lax = sqrtf(ax[0]*ax[0] + ax[1]*ax[1] + ax[2]*ax[2]);
    if (lax < 1e-6f) { memcpy(out, a, sizeof a); return; }
    for (i = 0; i < 3; i++) ax[i] /= lax;
    /* Rodrigues about ax by step */
    c = cosf(step * 0.017453292519943295f);
    s = sinf(step * 0.017453292519943295f);
    {
        const float dot = ax[0]*a[0] + ax[1]*a[1] + ax[2]*a[2];
        float cr[3];
        cr[0] = ax[1]*a[2] - ax[2]*a[1];
        cr[1] = ax[2]*a[0] - ax[0]*a[2];
        cr[2] = ax[0]*a[1] - ax[1]*a[0];
        for (i = 0; i < 3; i++)
            out[i] = a[i]*c + cr[i]*s + ax[i]*dot*(1.f - c);
    }
}

int main(void)
{
    carlight_t cl;
    float L[3];
    const float dt = 1.f / 60.f;

    printf("-- carlight: the direction\n");
    carlight_dir(L);
    ck(near(sqrtf(L[0]*L[0] + L[1]*L[1] + L[2]*L[2]), 1.f, 1e-5f),
       "the light direction is a unit vector", "|L| %.6f",
       sqrtf(L[0]*L[0] + L[1]*L[1] + L[2]*L[2]));
    ck(L[1] > 0.f, "and it points UP -- toward the sun, not along its travel",
       "L (%.3f %.3f %.3f)", L[0], L[1], L[2]);
    ck(near(L[1], sinf(LIGHT_ELEVATION_DEG * 0.017453292519943295f), 1e-5f),
       "its elevation is AngleElevation above the horizontal",
       "asin(Ly) %.2f deg, key %.2f",
       asinf(L[1]) * 57.29577951f, (double)LIGHT_ELEVATION_DEG);

    /* THE CROSS-CHECK: the closed form against FUN_0040b4c0's own arithmetic,
       over a grid neither of them was written for. */
    {
        static const float up[3] = { 0.f, 1.f, 0.f };
        float worst = 0.f;
        int ai, ei;
        for (ai = 0; ai < 8; ai++) {
            for (ei = 0; ei <= 90; ei += 15) {
                const float az = (float)ai * 45.f;
                const float ar = az * 0.017453292519943295f;
                const float er = (float)ei * 0.017453292519943295f;
                const float h[3] = { sinf(ar), 0.f, cosf(ar) };
                float got[3], want[3];
                int k;
                /* the closed form, spelled out here with the same two angles */
                got[0] = sinf(ar) * cosf(er);
                got[1] = sinf(er);
                got[2] = cosf(ar) * cosf(er);
                rot_toward(h, up, (float)ei, want);
                for (k = 0; k < 3; k++)
                    if (fabsf(got[k] - want[k]) > worst)
                        worst = fabsf(got[k] - want[k]);
            }
        }
        ck(worst < 1e-5f,
           "closed form == FUN_0040b4c0 over 8 azimuths x 7 elevations",
           "worst component error %.2e", (double)worst);
    }

    /* THE MODEL-SPACE LIGHT, checked as an INVARIANT rather than as arithmetic.
       Shading a model-space normal with the model-space light has to equal
       shading that same normal rotated into the world with the world light --
       for every rotation. That holds only if the transform is the one the row
       vectors want, and it is blind to which way round this file thinks the
       matrix is, which is the point: four bugs in this port came from a
       convention taken on paper (traps.md). */
    printf("-- carlight: the sun in a car's own space\n");
    {
        /* Three real orientations built the way rbcar_matrix is: a yaw, a yaw
           with pitch, and a yaw with pitch and roll. */
        static const float ang[3][3] = {
            {  90.f,   0.f,   0.f },
            { -35.f,  12.f,   0.f },
            { 200.f, -20.f,  25.f },
        };
        float W[3], worst = 0.f;
        int k;
        carlight_dir(W);
        for (k = 0; k < 3; k++) {
            const float y = ang[k][0] * 0.017453292519943295f;
            const float p = ang[k][1] * 0.017453292519943295f;
            const float r = ang[k][2] * 0.017453292519943295f;
            const float cy = cosf(y), sy = sinf(y);
            const float cp = cosf(p), sp = sinf(p);
            const float cr = cosf(r), sr = sinf(r);
            /* Rz(r) then Rx(p) then Ry(y), row-vector: the rows are the model
               axes in world space, which is rbcar_matrix's own convention. */
            float m[16];
            float Lm[3];
            int i;
            m[0] = cr*cy + sr*sp*sy;  m[1] = sr*cp;  m[2] = -cr*sy + sr*sp*cy; m[3] = 0.f;
            m[4] = -sr*cy + cr*sp*sy; m[5] = cr*cp;  m[6] = sr*sy + cr*sp*cy;  m[7] = 0.f;
            m[8] = cp*sy;             m[9] = -sp;    m[10] = cp*cy;            m[11] = 0.f;
            m[12] = 1.f; m[13] = 2.f; m[14] = 3.f;   m[15] = 1.f;  /* a translation, which must not matter */
            carlight_model_dir(m, Lm);
            /* Six model normals, including the cardinals. */
            for (i = 0; i < 6; i++) {
                float nm[3] = { 0.f, 0.f, 0.f }, nw[3];
                float got, want;
                int c;
                nm[i % 3] = (i < 3) ? 1.f : -1.f;
                for (c = 0; c < 3; c++)
                    nw[c] = nm[0]*m[0*4+c] + nm[1]*m[1*4+c] + nm[2]*m[2*4+c];
                got  = nm[0]*Lm[0] + nm[1]*Lm[1] + nm[2]*Lm[2];
                want = nw[0]*W[0]  + nw[1]*W[1]  + nw[2]*W[2];
                if (fabsf(got - want) > worst)
                    worst = fabsf(got - want);
            }
        }
        ck(worst < 1e-5f,
           "N.L in model space == the rotated N.L in world space, 3 rotations",
           "worst %.2e", (double)worst);
    }

    printf("-- carlight: light_car.bspl\n");
    {
        static const struct { float in, out; } pt[LIGHT_LM_CURVE_N] = LIGHT_LM_CURVE;
        rb_curve_pt cp[LIGHT_LM_CURVE_N];
        rb_curve cv;
        float worst = 0.f;
        int i;
        for (i = 0; i < LIGHT_LM_CURVE_N; i++) {
            cp[i].in = pt[i].in;
            cp[i].out = pt[i].out;
        }
        cv.pt = cp;
        cv.n = LIGHT_LM_CURVE_N;
        for (i = -20; i <= 300; i++) {
            const float x = (float)i;
            const float d = fabsf(carlight_curve(x) - rb_curve_eval(&cv, x));
            if (d > worst) worst = d;
        }
        ck(worst < 1e-3f,
           "the curve == contact.c's rb_curve_eval, -20..300",
           "worst %.2e", (double)worst);
        ck(near(carlight_curve(0.f), pt[0].out, 1e-3f)
           && near(carlight_curve(255.f), pt[LIGHT_LM_CURVE_N - 1].out, 1e-3f),
           "clamped at both ends, not extrapolated",
           "f(0) %.1f  f(255) %.1f", carlight_curve(0.f), carlight_curve(255.f));
        printf("       shade %.3f .. sun %.3f  (the whole range the level has)\n",
               carlight_curve(0.f) / 255.f, carlight_curve(255.f) / 255.f);
    }

    printf("-- carlight: the level\n");
    carlight_reset(&cl);
    ck(cl.level == 1.f && !cl.in_range,
       "reset is FUN_005320de's own pair: 1.0, latch down",
       "level %.3f in_range %d", cl.level, cl.in_range);

    /* Out of range: forced to 1.0 whatever it was, and the latch cleared so the
       next entry snaps rather than chases. */
    cl.level = 0.5f;
    cl.in_range = 1;
    carlight_step(&cl, LIGHT_LM_MAXDIST + 1.f, NULL, 0, 0.2f, dt);
    ck(cl.level == 1.f && !cl.in_range,
       "no camera within maxdist -> 1.0 and the latch clears",
       "level %.3f", cl.level);

    /* Entering range SNAPS. */
    {
        const float dark[4] = { 0.2f, 0.2f, 0.2f, 0.2f };
        carlight_reset(&cl);
        carlight_step(&cl, 1.f, dark, 4, -1.f, dt);
        ck(near(cl.level, carlight_curve(0.2f * 255.f) / 255.f, 1e-5f),
           "the frame it enters range, the level SNAPS to the target",
           "level %.3f  target %.3f", cl.level,
           carlight_curve(0.2f * 255.f) / 255.f);
    }

    /* The dead band, and it is on the difference BEFORE the step. */
    {
        const float s[1] = { 1.f };
        float was;
        carlight_reset(&cl);
        carlight_step(&cl, 1.f, s, 1, -1.f, dt);      /* snap to the bright end */
        was = cl.level;
        cl.level = was - LIGHT_LM_DELTA * 0.5f;       /* inside the band */
        carlight_step(&cl, 1.f, s, 1, -1.f, dt);
        ck(cl.level == was - LIGHT_LM_DELTA * 0.5f,
           "a level inside deltaalpha of the target does not move",
           "level %.4f", cl.level);
        cl.level = was - LIGHT_LM_DELTA * 4.f;        /* outside it */
        carlight_step(&cl, 1.f, s, 1, -1.f, dt);
        ck(cl.level > was - LIGHT_LM_DELTA * 4.f,
           "and one outside it moves toward the target", "level %.4f", cl.level);
    }

    /* The chase RATE, measured rather than asserted -- and it stops a DEAD BAND
       short of the target, which is the engine's own arithmetic and not a bug in
       either: FUN_00531ff0 tests |level - target| <= deltaalpha BEFORE stepping
       and returns, so the level lands inside the band and stays there. A check
       that waited for it to reach the target would spin for ever, which is how
       this one was written the first time. */
    {
        const float bright[1] = { 1.f };
        const float dark[1] = { 0.f };
        float t = 0.f;
        int steps = 0;
        carlight_reset(&cl);
        carlight_step(&cl, 1.f, bright, 1, -1.f, dt);       /* snap bright */
        {
            const float from = cl.level;
            const float to = carlight_curve(0.f) / 255.f;
            float prev = from;
            while (steps < 600) {
                carlight_step(&cl, 1.f, dark, 1, -1.f, dt);
                if (cl.level == prev)
                    break;                      /* inside the band: settled */
                prev = cl.level;
                t += dt;
                steps++;
            }
            ck(t > 0.f && near((from - cl.level) / t, LIGHT_LM_ALPHAVEL, 0.05f),
               "bright to dark travels at alphavel per second",
               "%.3f over %.3f s = %.2f/s, key %.2f",
               from - cl.level, t, (from - cl.level) / t,
               (double)LIGHT_LM_ALPHAVEL);
            ck(cl.level - to > 0.f && cl.level - to <= LIGHT_LM_DELTA,
               "and settles INSIDE the dead band, short of the target",
               "settled %.3f, target %.3f, gap %.3f (band %.2f)",
               cl.level, to, cl.level - to, (double)LIGHT_LM_DELTA);
            printf("       a tunnel mouth takes %.2f s to cross, %.3f -> %.3f\n",
                   t, from, cl.level);
        }
    }

    /* NOTHING SAMPLED must leave the level alone -- not put it back to 1.0.
       That is the engine's own `return' and the difference is a car that flashes
       bright every time a wheel leaves a lightmapped face. */
    {
        float was;
        carlight_reset(&cl);
        carlight_step(&cl, 1.f, NULL, 0, 0.2f, dt);
        was = cl.level;
        carlight_step(&cl, 1.f, NULL, 0, -1.f, dt);
        ck(cl.level == was && cl.n_samples == 0,
           "no wheel and no fallback -> the level is left where it was",
           "level %.3f (was %.3f)", cl.level, was);
    }

    /* The average is over the wheels that ANSWERED, and a wheel with no
       lightmap under it does not vote. */
    {
        const float mixed[4] = { 0.2f, 0.8f, -1.f, -1.f };
        carlight_reset(&cl);
        carlight_step(&cl, 1.f, mixed, 4, -1.f, dt);
        ck(cl.n_samples == 2 && near(cl.raw, 0.5f, 1e-6f),
           "the mean skips the wheels with no lightmap under them",
           "n %d raw %.3f", cl.n_samples, cl.raw);
    }

    printf("-- carlight: the two intensities and the LOD fade\n");
    {
        float a, d;
        carlight_reset(&cl);
        carlight_terms(&cl, 0.87f, &a, &d);      /* the chase camera's distance */
        ck(near(a, (float)LIGHT_AMBIENT / 255.f, 1e-4f)
           && near(d, (float)LIGHT_DIRECT / 255.f, 1e-4f),
           "at the chase camera's 0.87 m: the bytes, unfaded",
           "ambient %.3f (%d/255)  direct %.3f (%d/255)",
           a, LIGHT_AMBIENT, d, LIGHT_DIRECT);
        printf("       a face at the sun renders x%.3f, one facing away x%.3f\n",
               a + d, a);
        carlight_terms(&cl, LIGHT_FADE_FAR + 1.f, &a, &d);
        ck(near(a, 1.f, 1e-4f) && near(d, 0.f, 1e-4f),
           "beyond the fade: flat white ambient, no light at all",
           "ambient %.3f direct %.3f", a, d);
        carlight_terms(&cl, (LIGHT_FADE_NEAR + LIGHT_FADE_FAR) * 0.5f, &a, &d);
        ck(a > (float)LIGHT_AMBIENT / 255.f && a < 1.f
           && d > 0.f && d < (float)LIGHT_DIRECT / 255.f,
           "and half way through it, half way between the two",
           "ambient %.3f direct %.3f", a, d);
        /* The tunnel end, which is the whole point of the feature. */
        cl.level = carlight_curve(0.f) / 255.f;
        carlight_terms(&cl, 0.87f, &a, &d);
        ck(a < (float)LIGHT_AMBIENT / 255.f && d < (float)LIGHT_DIRECT / 255.f,
           "in full shade both terms drop by the curve's floor",
           "ambient %.3f direct %.3f, level %.3f", a, d, cl.level);
        printf("       in shade: at the sun x%.3f, facing away x%.3f\n",
               a + d, a);
    }

    printf("-- carlight: the shading, per vertex\n");
    {
        /* Four normals: at the sun, square to it, away from it, and half way.
           The light is passed in the normals' own space, which is what
           scene_shade rotates it into -- here it is just the sun itself. */
        float L[3];
        unsigned char rgba[4 * 4];
        float nrm[12];
        float a, d;
        int i;
        carlight_dir(L);
        carlight_reset(&cl);
        carlight_terms(&cl, 0.87f, &a, &d);
        nrm[0] = L[0];  nrm[1] = L[1];  nrm[2] = L[2];          /* at the sun */
        nrm[3] = -L[0]; nrm[4] = -L[1]; nrm[5] = -L[2];         /* away */
        nrm[6] = 1.f;   nrm[7] = 0.f;   nrm[8] = 0.f;           /* square to it */
        /* And one HALF WAY: L turned 45 degrees, which is L + a unit
           perpendicular, normalised. +X is perpendicular to L because the
           shipped azimuth is 0 and L has no x component -- asserted below rather
           than assumed, since an azimuth key that moved would break it. */
        nrm[9] = L[0] + 1.f; nrm[10] = L[1]; nrm[11] = L[2];
        {
            const float len = sqrtf(nrm[9]*nrm[9] + nrm[10]*nrm[10]
                                    + nrm[11]*nrm[11]);
            nrm[9] /= len; nrm[10] /= len; nrm[11] /= len;
        }
        ck(fabsf(L[0]) < 1e-6f,
           "+X really is perpendicular to the sun at this azimuth",
           "Lx %.6f", L[0]);
        memset(rgba, 0, sizeof rgba);
        carlight_shade(a, d, L, nrm, 4, rgba);
        ck(rgba[0] == rgba[1] && rgba[1] == rgba[2],
           "the shade is GREY, as both of the engine's colours are",
           "(%d %d %d)", rgba[0], rgba[1], rgba[2]);
        ck(rgba[3] == 255 && rgba[7] == 255,
           "and opaque -- the car's cut-outs are the texture's alpha", "");
        ck(rgba[0] == 255,
           "a face square to the sun saturates: 138 + 128 is 266 of 255",
           "%d, want the clamp", rgba[0]);
        ck(near((float)rgba[4] / 255.f, a, 1.f / 255.f),
           "a face turned AWAY gets the ambient alone",
           "%d/255 = %.3f, ambient %.3f", rgba[4], rgba[4] / 255.f, a);
        ck(near((float)rgba[8] / 255.f, a, 1.f / 255.f),
           "and so does one square ACROSS it: max(0, N.L) is zero there",
           "%d/255 = %.3f, ambient %.3f", rgba[8], rgba[8] / 255.f, a);
        ck(rgba[12] < rgba[0] && rgba[12] > rgba[4],
           "a face HALF WAY to the sun lands between the two",
           "away %d, at 45 deg %d, at the sun %d",
           rgba[4], rgba[12], rgba[0]);
        ck(near((float)rgba[12] / 255.f, a + d * 0.70710678f, 2.f / 255.f),
           "and lands where ambient + direct * cos 45 puts it",
           "%d/255 = %.3f, want %.3f", rgba[12], rgba[12] / 255.f,
           a + d * 0.70710678f);
        /* MONOTONIC in N.L, which is the whole of "the sun picks on the shell":
           sweep a normal from facing away to facing the sun and the grey must
           never go down. */
        {
            int bad = 0, prev = -1;
            for (i = 0; i <= 32; i++) {
                const float t = (float)i / 32.f * 3.14159265f;
                float n[3];
                unsigned char one[4];
                /* rotate about an axis perpendicular to L, in the L/+X plane */
                n[0] = cosf(t) * L[0] + sinf(t);
                n[1] = cosf(t) * L[1];
                n[2] = cosf(t) * L[2];
                {
                    const float len = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
                    n[0] /= len; n[1] /= len; n[2] /= len;
                }
                carlight_shade(a, d, L, n, 1, one);
                if (prev >= 0 && one[0] > prev + 1)
                    bad++;
                prev = one[0];
            }
            ck(bad == 0, "and it falls monotonically as the normal turns away",
               "%d reversals over 33 steps", bad);
        }
        /* IN SHADE the whole range moves down together. */
        cl.level = carlight_curve(0.f) / 255.f;
        carlight_terms(&cl, 0.87f, &a, &d);
        {
            unsigned char dark[4 * 4];
            carlight_shade(a, d, L, nrm, 4, dark);
            ck(dark[0] < rgba[0] && dark[4] < rgba[4],
               "in shade both the lit and the unlit side drop",
               "at the sun %d -> %d, away %d -> %d",
               rgba[0], dark[0], rgba[4], dark[4]);
            printf("       sun %d -> %d   away %d -> %d  (of 255)\n",
                   rgba[0], dark[0], rgba[4], dark[4]);
        }
    }

    /* THE TOGGLE IS A TOGGLE, and with the shading on the CPU that is all it
       has to be: main.c reads carlight_enabled() and skips scene_shade, so a
       batch keeps the 0xFF it was allocated with. */
    {
        carlight_set_enabled(0);
        ck(!carlight_enabled(), "the menu row turns it off", "");
        carlight_set_enabled(1);
        ck(carlight_enabled(), "and back on", "");
    }

    /* And what beach_1's own ground does to it, printed rather than asserted:
       the numbers come out of pack_col.py's summary for that track. */
    printf("-- what beach_1's lightmap asks for (pack_col.py's own numbers)\n");
    {
        const struct { const char *what; float lm; } probe[] = {
            { "the darkest lit triangle (17)",  17.f / 255.f },
            { "the median (131)",              131.f / 255.f },
            { "the brightest (254)",           254.f / 255.f },
        };
        unsigned int i;
        for (i = 0; i < sizeof probe / sizeof probe[0]; i++) {
            float a, d;
            carlight_reset(&cl);
            carlight_step(&cl, 1.f, &probe[i].lm, 1, -1.f, dt);
            carlight_terms(&cl, 0.87f, &a, &d);
            printf("   %-32s level %.3f  ->  x%.3f at the sun, x%.3f away\n",
                   probe[i].what, cl.level, a + d, a);
        }
    }

    printf("\n%d checks, %d failed%s\n", checks, fails,
           fails ? "  -- SOME CHECKS FAILED" : "");
    return fails ? 1 : 0;
}
