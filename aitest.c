/*
 * aitest.c -- the AI opponents, against the REAL ten .aip files and the REAL ten
 * collision grids.
 *
 * ai.c is a transcription of a replay, so most of what can go wrong is arithmetic
 * that still produces plausible motion: a lap that wraps to the wrong sample, a
 * rubber band pointing the wrong way, a cursor that walks the polyline at the
 * wrong rate. None of that is visible in a screenshot until you have watched a
 * full lap, so it gets checked here.
 *
 * Every check is bound to something the constant under it cannot move -- the
 * recording's own path length, the measured lap-closure gap, the shape of the
 * spline family, a ratio, a direction. Where a bound is a number it says where
 * the number was measured.
 *
 * The spine stub is opponent 0's OWN polyline, projected onto. That is a real
 * closed spine of the right length on the right track, so the lead the rubber
 * band sees is a real lead -- and it means the harness cannot accidentally test
 * the rubber band against a straight line that no car ever drives.
 *
 * MUTATION SCORE: 46 mutants, 43 killed -- 32 against the replay of which 29
 * died, and 14 against the BUMP (part 9) of which 13 did. Every survivor was
 * looked at rather than counted, and each is equivalent or measured:
 *
 *   - interpolating the MOMENTUM instead of taking it from the earlier sample.
 *     Provably dead, not merely small: ai_diff_velocity overwrites both v and P
 *     from the finite difference immediately after every pose, the way
 *     FUN_005037f0 does, so whatever the state carried is gone before anything
 *     reads it.
 *   - dropping rb_coll_resolve's 0.05 m/s separation TARGET, leaving the solve to
 *     remove the closing speed and no more. 0.05 m/s on cars doing several.
 *   - DROPPING THE REFUSED SHARE: when an opponent will not take all of its half
 *     of a separation, not handing the remainder to the other car. That was a
 *     real 11 cm of burial when the positional half ran once per tick, and
 *     AI_DEPEN_PASSES has since made it near-equivalent -- the next pass
 *     re-measures and recovers what the last one dropped. Measured: part 8's two
 *     rams are unchanged to the millimetre (0.005 and 0.007 m), and the only
 *     column that moves at all is the ten-track opponent survey, 0.031 -> 0.041 m.
 *     Kept because it is right and free, recorded here because it is no longer
 *     load-bearing on its own.
 *
 * The bump mutants that DIED are worth listing, because between them they are
 * the model: no return spring, no snap to zero, no yaw term, an immovable
 * opponent (the old one-way behaviour), no offset limit, no acceleration cap,
 * one depenetration pass, the lead measured on the bumped pose instead of the
 * recorded one, no opponent-against-opponent solve, an offset composed onto the
 * body instead of onto the recording, pushing out of every pair at once, the
 * pair denominator reduced to the player's own, and a clamp that holds the
 * position while letting the velocity grow through it.
 *
 * FOUR of those needed a check that did not exist, and each is a trap:
 *   - "the offset moved" is not "the car moved". Composing onto the body instead
 *     of onto the recording moved the car TWICE as far as its own telemetry said,
 *     and every check read the telemetry. Part 9 measures the pose against an
 *     unbumped twin.
 *   - a cap is only tested by an input that asks for more than it. A 50 m/s
 *     impulse crosses the whole offset limit inside one tick and never asks the
 *     spring for anything; a 5 m/s one asks for 35 m/s^2 for several ticks.
 *   - every check on a collision was a FLOOR ("it imparts real speed"), so an
 *     impulse sized for an immovable wall passed them all. Two equal masses share
 *     it: the struck car leaves at about half the closing speed, never more.
 *   - and the clamp's velocity kill, which is invisible in position and shows up
 *     as a car settling 11 cm inside another one.
 *
 * SEVEN earlier survivors were real gaps and are now dead. Each is a trap this
 * project has fallen into before, and one of them was a bug that SHIPPED:
 *
 *   - the lap-TIME check did not exist. The lap DISTANCE was checked and is right
 *     whatever the speed law does -- the cursor walks the recording's own
 *     polyline either way -- so every opponent ran at exactly half speed and the
 *     suite was green. What the speed law controls is the clock.
 *   - the acceleration-limit check read the very constant it guarded, so a limit
 *     of 500 passed. It is a literal 5.0 now, with the address it came from.
 *   - the droop threshold compared against len_max, which a recorded car never
 *     reaches, so `airborne` was permanently 0 and "every wheel is grounded" was
 *     invisible.
 *   - the spline-slot check compared two curves at a lead where both had already
 *     clamped to the same value.
 *   - the orientation check used a strict inequality that "snap to the earlier
 *     sample" satisfied by one float bit, at a mid-segment u where inverting the
 *     fraction is a no-op besides.
 *   - the contact fixture held the opponent STILL, so the moving-body term was
 *     untested. Part 8's third case drives an opponent into a parked player, and
 *     what separates them is the reported CLOSING SPEED (3.91 m/s against 0.15)
 *     and being knocked clear rather than bulldozed (0.75 m against 9.84).
 *   - part 8 measured a TWELVE SECOND ram and called it an arrival. Against an
 *     opponent that can be shoved, ten of those seconds are two cars grinding
 *     over each other in a state so chaotic that the depenetration pass count
 *     sweeps non-monotonically through it. It stops measuring a second after
 *     first contact now; the sustained case is part 9's, where it can be asked
 *     whether the player is stuck rather than how deep the overlap got.
 *   - two mutants were badly BUILT and survived for that reason alone: one
 *     disabled half a compound condition and left the half that mattered, the
 *     other was a comment.
 *
 * Build (from rccars_vita) -- BUILD.md has the current line, which links scene.c
 * and the host GL stub for part 7's shared rig:
 *   gcc -I. -Itestgl -O2 -fno-fast-math -ffp-contract=off ../rccars_re/aitest.c \
 *       ai.c col.c rb.c rbcar.c contact.c collide.c carani.c scene.c rlog.c \
 *       ../rccars_re/glstub_host.c -lm -o aitest
 */

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ai.h"
#include "carani.h"
#include "col.h"
#include "rb_data.h"
#include "rbcar.h"
#include "scene.h"
#include "tracks.h"

static int pass, fail;

static void ck(int ok, const char *fmt, ...)
{
    va_list ap;
    printf(ok ? "  ok   " : "  FAIL ");
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    if (ok) pass++; else fail++;
}

#define ASSETS "assets"

/* ------------------------------------------------------------- the spine stub
 *
 * Projects onto opponent 0's recorded polyline and returns the cumulative
 * distance along it. `cp` splits the lap into CPN equal arcs, which is the shape
 * a real checkpoint list has (beach_1 has four waypoints over 460 m).
 */
#define CPN 4

typedef struct {
    const ai_car *ref;
    float total;
    float *cum;            /* cumulative length at each sample */
} spine_t;

static int spine_build(spine_t *sp, const ai_car *a)
{
    int i;
    sp->ref = a;
    sp->cum = (float *)malloc(sizeof(float) * (size_t)a->n);
    if (!sp->cum)
        return 0;
    sp->cum[0] = 0.0f;
    for (i = 1; i < a->n; i++) {
        double dx = (double)a->s[i].p[0] - a->s[i - 1].p[0];
        double dy = (double)a->s[i].p[1] - a->s[i - 1].p[1];
        double dz = (double)a->s[i].p[2] - a->s[i - 1].p[2];
        sp->cum[i] = sp->cum[i - 1] + (float)sqrt(dx * dx + dy * dy + dz * dz);
    }
    sp->total = sp->cum[a->n - 1];
    return 1;
}

static void spine_free(spine_t *sp)
{
    free(sp->cum);
    sp->cum = NULL;
}

/* `hint` is ignored here on purpose: this fixture's spine is a synthetic
   polyline with no doubling back, so the windowing cp_spine_dist_near does has
   nothing to bite on, and every check in this file is about the RUBBER BAND
   rather than about the projection. vis_test owns the monotonicity of the real
   spines -- see its part 2. */
/* ai_track.lap_progress. On THIS fixture's synthetic spine the projection IS a
   sound progress measure -- the polyline is a simple loop that never passes near
   itself -- so it is the same query. On the real tracks it is not, which is why
   the game binds cp_lap_progress there instead; see checkpoint.h. */
static int spine_query(void *ctx, float x, float y, float z, float hint,
                       float *dist, int *cp);

static int progress_query(void *ctx, float x, float y, float z, float *out)
{
    return spine_query(ctx, x, y, z, -1.f, out, NULL);
}

static int spine_query(void *ctx, float x, float y, float z, float hint,
                       float *dist, int *cp)
{
    (void)hint;
    spine_t *sp = (spine_t *)ctx;
    const ai_car *a = sp->ref;
    int i, best = 0;
    double bd = 1e30;

    (void)y;
    for (i = 0; i < a->n; i++) {
        double dx = (double)a->s[i].p[0] - x;
        double dz = (double)a->s[i].p[2] - z;
        double d = dx * dx + dz * dz;
        if (d < bd) { bd = d; best = i; }
    }
    if (dist) *dist = sp->cum[best];
    if (cp) {
        int k = (int)(sp->cum[best] / (sp->total / (float)CPN));
        *cp = k < 0 ? 0 : (k >= CPN ? CPN - 1 : k);
    }
    return 1;
}

/* ------------------------------------------------------------------- helpers */

/* Step an ai_t at 1/60 with the player parked at `pp`, for `secs`. */
static void run(ai_t *ai, const ai_track *tr, const float pp[3], float secs)
{
    int i, n = (int)(secs * 60.0f + 0.5f);
    for (i = 0; i < n; i++)
        ai_step(ai, tr, pp[0], pp[1], pp[2], 0, 1.0f / 60.0f);
}

static float dist3(const float a[3], const float b[3])
{
    double dx = (double)a[0] - b[0], dy = (double)a[1] - b[1];
    double dz = (double)a[2] - b[2];
    return (float)sqrt(dx * dx + dy * dy + dz * dz);
}

static void sample_pos(const ai_car *a, int i, float out[3])
{
    out[0] = a->s[i].p[0];
    out[1] = a->s[i].p[1];
    out[2] = a->s[i].p[2];
}

/* ==========================================================================
 * part 1 -- the ten files, read through the RUNTIME loader
 * ========================================================================== */
static void part1(void)
{
    int t, total_op = 0;

    printf("\npart 1: the ten .aip files, through ai_init\n");
    for (t = 0; t < AI_N_RACES; t++) {
        ai_t ai;
        const ai_race *race = &AI_RACES[t];
        col_t col;
        int i, bad_q = 0, bad_len = 0, bad_car = 0, bad_cycle = 0;
        int probed = 0, offgrid = 0;
        int have_col;
        char cp[256];

        memset(&ai, 0, sizeof(ai));
        if (ai_init(&ai, t, ASSETS, NULL, 1 /* normal */, 0 /* single */) == 0) {
            ck(0, "%s: ai_init loaded nothing", race->track);
            continue;
        }
        total_op += ai.n;

        snprintf(cp, sizeof(cp), ASSETS "/%s.col", race->track);
        memset(&col, 0, sizeof(col));
        have_col = col_load(cp, &col);

        for (i = 0; i < ai.n; i++) {
            const ai_car *a = &ai.car[i];
            double sum = 0.0;
            int k;

            if (a->car != race->op[i].car)
                bad_car++;
            if (a->cycle_start < 1 || a->cycle_start >= a->n)
                bad_cycle++;

            for (k = 0; k < a->n; k++) {
                double qn = 0.0;
                int j;
                for (j = 0; j < 4; j++) {
                    double v = (double)a->s[k].q[j] / AI_Q_SCALE;
                    qn += v * v;
                }
                if (fabs(sqrt(qn) - 1.0) > 1e-3)
                    bad_q++;
                if (k) {
                    double dx = (double)a->s[k].p[0] - a->s[k - 1].p[0];
                    double dy = (double)a->s[k].p[1] - a->s[k - 1].p[1];
                    double dz = (double)a->s[k].p[2] - a->s[k - 1].p[2];
                    sum += sqrt(dx * dx + dy * dy + dz * dz);
                }
                if (have_col && (k % 40) == 0) {
                    float gy, nx, ny, nz;
                    probed++;
                    if (!col_ground_at(&col, a->s[k].p[0], a->s[k].p[2],
                                       a->s[k].p[1] + 0.05f, &gy, &nx, &ny, &nz)
                        || a->s[k].p[1] - gy > 3.0f)
                        offgrid++;
                }
            }
            /* The header's own path_len against the polyline it describes. Not
               self-referential: the packer wrote one from the .dat and the other
               is summed here out of the packed samples. */
            if (fabs(sum - a->path_len) > 0.001 * a->path_len + 0.01)
                bad_len++;
        }

        ck(bad_car == 0, "%s: %d opponent(s), car index matches the roster"
                         " (%d mismatch)", race->track, ai.n, bad_car);
        ck(bad_cycle == 0, "%s: every cycle start is inside its own path",
           race->track);
        ck(bad_q == 0, "%s: every quaternion is unit to 1e-3 (%d bad)",
           race->track, bad_q);
        ck(bad_len == 0, "%s: header path_len matches the packed polyline",
           race->track);
        if (have_col) {
            /* The mapping proof, re-run through the runtime reader rather than
               the packer's own. pack_ai.py measured 0% off-grid on every one and
               52-99% against any WRONG track, so 5% is two orders of magnitude
               clear of a mismatch and generous to a jump. */
            ck(probed > 100 && offgrid * 100 <= probed * 5,
               "%s: %d/%d probes have ground under them (%.1f%% off)",
               race->track, probed - offgrid, probed,
               100.0 * offgrid / (probed ? probed : 1));
            col_free(&col);
        }
        ai_free(&ai);
    }
    ck(total_op == 10 * AI_MAX_FIELD,
       "all ten tracks field %d opponents each (%d total)",
       AI_MAX_FIELD, total_op);
}

/* ==========================================================================
 * part 2 -- the advance law and the lap
 * ========================================================================== */
static void part2(void)
{
    ai_t ai;
    ai_car *a;
    float pp[3] = { 1e6f, 0.0f, 1e6f };   /* a player nowhere near the track */
    float p_before[3], p_cyc[3], p_last[3], p_zero[3];
    int i, laps, lap_ticks, bad_cursor = 0, bad_u = 0;
    double travelled;

    printf("\npart 2: the cursor advance, the polyline and the lap\n");
    memset(&ai, 0, sizeof(ai));
    if (ai_init(&ai, 0, ASSETS, NULL, 1, 0) < 1) {
        ck(0, "beach_1 loaded");
        return;
    }
    a = &ai.car[0];

    /* With no spine bound the coefficient is the static product alone, which for
       a single race on beach_1 is CoeffCommonOpponents = 0.85. */
    ck(fabs(ai.coeff_static - AI_RACES[0].coeff_common) < 1e-6,
       "single race: the static coefficient is the track's own %.2f (%.4f)",
       AI_RACES[0].coeff_common, ai.coeff_static);

    /* One lap. The distance the cursor walks must be the recording's own path
       length, less the run-up it never repeats -- it starts at sample 0 and ends
       at the last one, so exactly path_len the first time round. */
    travelled = 0.0;
    laps = 0;
    lap_ticks = 0;
    for (i = 0; i < 60 * 600 && laps == 0; i++) {
        lap_ticks++;
        float before[3], after[3];
        memcpy(before, a->rb.body.x, sizeof(before));
        ai_step(&ai, NULL, pp[0], pp[1], pp[2], 0, 1.0f / 60.0f);
        memcpy(after, a->rb.body.x, sizeof(after));
        travelled += dist3(before, after);
        laps = a->lap;
        if (a->cursor < 0 || a->cursor >= a->n)
            bad_cursor++;
        if (a->u < -1e-4f || a->u > 1.0f + 1e-4f)
            bad_u++;
    }
    ck(laps == 1, "beach_1 slot 0 completes a lap (%d)", laps);
    ck(bad_cursor == 0, "the cursor stays inside the path");
    ck(bad_u == 0, "u stays in 0..1");
    /* travelled is the CHORD sum through space, a->dist is what the advance
       law thinks it walked. They are the same polyline, so they agree; and both
       agree with the recording's own length. */
    ck(fabs(travelled - a->path_len) < 0.02 * a->path_len,
       "a lap covers the recorded %.1f m (%.1f m through space, %.1f m by the"
       " advance)", a->path_len, travelled, a->dist);

    /* AND IT COVERS IT IN THE RIGHT TIME, which is a different claim and the one
     * that matters. The distance is right whatever the speed law does -- the
     * cursor walks the recording's own polyline either way -- so the distance
     * check above passed for weeks while every opponent ran at exactly half
     * speed. What the speed law controls is the CLOCK: replaying a lap that took
     * `duration` seconds at `coeff` times its recorded speed must take
     * duration / coeff. Nothing in ai.c can move both sides of that. */
    {
        double want = (double)a->duration / (double)ai.coeff_static;
        double got = (double)lap_ticks / 60.0;
        ck(fabs(got - want) < 0.05 * want,
           "and it covers it in the right TIME: %.1f s against the recording's"
           " own %.1f s over a coefficient of %.3f = %.1f s",
           got, a->duration, ai.coeff_static, want);
    }

    /* THE LAP WRAPS TO cycle_start, NOT TO ZERO. Bound to geometry rather than
     * to the index: at the wrap the car must be where the END of the path is,
     * which is within pack_ai.py's measured 0.21 m worst lap gap of
     * s[cycle_start] -- and s[0] is metres away from both, because the run-up
     * starts on the grid and the lap rejoins further round. */
    memcpy(p_before, a->rb.body.x, sizeof(p_before));
    sample_pos(a, a->cycle_start, p_cyc);
    sample_pos(a, a->n - 1, p_last);
    sample_pos(a, 0, p_zero);
    ck(a->cursor == a->cycle_start,
       "the cursor wraps to the cycle start %d, not 0", a->cycle_start);
    ck(dist3(p_before, p_cyc) < 0.5f,
       "at the wrap the car is at the cycle start (%.2f m)",
       dist3(p_before, p_cyc));
    ck(dist3(p_cyc, p_zero) > 2.0f,
       "and the cycle start is a real distance from sample 0 (%.2f m), so the"
       " check above can tell them apart", dist3(p_cyc, p_zero));
    ck(dist3(p_last, p_cyc) < 0.5f,
       "the recording's own lap closes there too (%.2f m)",
       dist3(p_last, p_cyc));

    /* The second lap must be the CYCLE length, shorter than the first by the
       run-up. */
    {
        double lap2 = 0.0;
        int l0 = a->lap;
        for (i = 0; i < 60 * 400 && a->lap == l0; i++) {
            float before[3], after[3];
            memcpy(before, a->rb.body.x, sizeof(before));
            ai_step(&ai, NULL, pp[0], pp[1], pp[2], 0, 1.0f / 60.0f);
            memcpy(after, a->rb.body.x, sizeof(after));
            lap2 += dist3(before, after);
        }
        ck(a->lap == l0 + 1, "and a second lap (%d)", a->lap);
        ck(lap2 < a->path_len - 5.0f,
           "the second lap is shorter than the first by the run-up"
           " (%.1f m against %.1f)", lap2, a->path_len);
    }
    ai_free(&ai);
}

/* ==========================================================================
 * part 3 -- the rubber band
 * ========================================================================== */
static void part3(void)
{
    ai_t ai;
    int slot, mode, bad_dir = 0, bad_range = 0;
    float lo, hi;

    printf("\npart 3: the rubber band\n");
    memset(&ai, 0, sizeof(ai));
    if (ai_init(&ai, 0, ASSETS, NULL, 1, 0) < 1) {
        ck(0, "beach_1 loaded");
        return;
    }

    /* DIRECTION, over both families and all five slots. A trailing opponent
     * (negative lead) must be told to go faster than a leading one. Bound to the
     * ordering, not to any value, so it survives a spline edit and dies on a
     * sign flip. */
    for (mode = 0; mode < 2; mode++) {
        ai.championship = mode;
        for (slot = 0; slot < 5; slot++) {
            float prev = 1e30f;
            float l;
            for (l = -60.0f; l <= 60.0f; l += 1.0f) {
                float c = ai_coeff(&ai, slot, l, 0) / ai.coeff_static;
                if (c > prev + 1e-4f)
                    bad_dir++;
                prev = c;
                if (c < 0.55f || c > 1.25f)
                    bad_range++;
            }
        }
    }
    ai.championship = 0;
    ck(bad_dir == 0, "the coefficient never increases with the lead"
                     " (both families, all five slots)");
    ck(bad_range == 0, "and stays inside the splines' own 0.6..1.2 band");

    lo = ai_coeff(&ai, 0, -40.0f, 0) / ai.coeff_static;
    hi = ai_coeff(&ai, 0, 45.0f, 0) / ai.coeff_static;
    ck(lo > 1.05f && hi < 0.85f,
       "slot 0: 40 m behind asks for %.3f, 45 m ahead for %.3f", lo, hi);

    /* THE FIVE OPPONENTS MUST BE ON FIVE DIFFERENT CURVES. Two claims, because
     * either one alone can be satisfied by a broken binding: that the loaded
     * cars carry distinct slots, and that the slots actually behave differently
     * at the same lead. The second is the reason the first matters -- slot 4 is
     * meant to fight harder for the same gap than slot 0 does. */
    {
        int i, j, dup = 0, same = 0;
        for (i = 0; i < ai.n; i++) {
            for (j = i + 1; j < ai.n; j++)
                if (ai.car[i].slot == ai.car[j].slot)
                    dup++;
            for (j = i + 1; j < ai.n; j++)
                if (fabs(ai_coeff(&ai, ai.car[i].slot, 20.0f, 0)
                         - ai_coeff(&ai, ai.car[j].slot, 20.0f, 0)) < 1e-4f)
                    same++;
        }
        ck(dup == 0 && ai.n == AI_MAX_FIELD,
           "the %d opponents hold %d distinct spline slots", ai.n, ai.n);
        /* At a 20 m LEAD, not a 45 m deficit: below -40 m the first two curves
           have both clamped to the same 1.1 and cannot be told apart, which is a
           property of the data rather than of the binding. 20 m is inside every
           curve's own range, so all five answer differently. */
        ck(same == 0, "and no two of them answer a 20 m lead alike"
                      " (slot 0 %.3f, slot 4 %.3f)",
           ai_coeff(&ai, 0, 20.0f, 0) / ai.coeff_static,
           ai_coeff(&ai, 4, 20.0f, 0) / ai.coeff_static);
    }

    /* The _A family is a TIGHTER band around the player -- it slows a leader
       further and lets a trailer catch up less. gen_ai_data.py says so from the
       spline points; this is the same claim through the code path. */
    {
        float p_lo, p_hi, a_lo, a_hi;
        ai.championship = 0;
        p_lo = ai_coeff(&ai, 0, -40.0f, 0) / ai.coeff_static;
        p_hi = ai_coeff(&ai, 0, 44.0f, 0) / ai.coeff_static;
        ai.championship = 1;
        a_lo = ai_coeff(&ai, 0, -40.0f, 0) / ai.coeff_static;
        a_hi = ai_coeff(&ai, 0, 44.0f, 0) / ai.coeff_static;
        ai.championship = 0;
        ck(a_hi < p_hi && a_lo < p_lo,
           "the championship's _A family slows a leader harder (%.3f vs %.3f)"
           " and catches up less (%.3f vs %.3f)", a_hi, p_hi, a_lo, p_lo);
    }

    /* THE FRONT SHIFT. Only with the player two or more checkpoints ahead AND a
     * positive lead. gap 1 must change nothing; gap 2 must reduce it; a negative
     * lead must be untouched at any gap. */
    {
        float g0 = ai_coeff(&ai, 0, 20.0f, 0);
        float g1 = ai_coeff(&ai, 0, 20.0f, 1);
        float g2 = ai_coeff(&ai, 0, 20.0f, 2);
        float g9 = ai_coeff(&ai, 0, 200.0f, 9);
        float n0 = ai_coeff(&ai, 0, -20.0f, 0);
        float n2 = ai_coeff(&ai, 0, -20.0f, 2);
        float c0, c2;
        ck(g1 == g0, "one checkpoint of gap changes nothing");
        /* Two or more SUBTRACTS from the lead, so the coefficient goes UP: the
           shift exists to pull back a lead that only reads positive because the
           opponent is a lap up, and a smaller lead is further left on a curve
           that only ever falls. Asserting the direction of the SHIFT, not of the
           coefficient. */
        ck(g2 > g0 + 1e-4f,
           "two subtracts from the lead, so the coefficient rises"
           " (%.4f -> %.4f)", g0, g2);
        ck(n2 == n0, "and a trailing opponent is untouched at any gap");
        /* At a lead the spline has already clamped, the shift cannot do anything
           -- which is why this mechanism is nearly inert. Worth pinning so the
           claim in ai_coeff's comment is checked rather than asserted. */
        ck(fabs(g9 - ai_coeff(&ai, 0, 200.0f, 0)) < 1e-6f,
           "and past the spline's own clamp it changes nothing at all");
        /* The championship never sees it: same mode gate as the family. */
        ai.championship = 1;
        c0 = ai_coeff(&ai, 0, 20.0f, 0);
        c2 = ai_coeff(&ai, 0, 20.0f, 2);
        ai.championship = 0;
        ck(c0 == c2, "and the championship is not subject to it at all");
    }

    /* THE EFFECT IS REAL, measured as a behaviour rather than as a number: the
     * same opponent, the same time, a player pinned far ahead against far
     * behind, must cover materially different ground. The spline band is
     * 1.1/0.8 on slot 0, so the ratio should be near 1.375; require 15% so a
     * spline edit does not break it but a dead rubber band does. */
    {
        ai_t a1, a2;
        spine_t sp;
        ai_track tr;
        float back[3], fwd[3];
        float d_chase, d_hold;

        memset(&a1, 0, sizeof(a1));
        memset(&a2, 0, sizeof(a2));
        ai_init(&a1, 0, ASSETS, NULL, 1, 0);
        ai_init(&a2, 0, ASSETS, NULL, 1, 0);
        spine_build(&sp, &a1.car[0]);
        tr.ctx = &sp;
        tr.spine = spine_query;
    tr.lap_progress = progress_query;
        tr.spine_len = sp.total;

        /* Park the player on the spine, once a long way round from the start and
           once just behind it, so the AI reads a big positive and a big negative
           lead. */
        {
            int i_far = a1.car[0].n * 3 / 4;
            int i_near = 5;
            sample_pos(&a1.car[0], i_far, fwd);   /* player miles ahead */
            sample_pos(&a1.car[0], i_near, back); /* player at the back */
        }
        run(&a1, &tr, fwd, 20.0f);     /* player ahead -> AI told to hurry?  */
        run(&a2, &tr, back, 20.0f);    /* player behind -> AI told to wait   */
        d_chase = a1.car[0].dist;
        d_hold = a2.car[0].dist;
        ck(d_chase > d_hold * 1.15f,
           "20 s with the player ahead covers %.1f m, with the player behind"
           " %.1f m (%.0f%% more)", d_chase, d_hold,
           100.0 * (d_chase / (d_hold > 0.01f ? d_hold : 1.0f) - 1.0));
        spine_free(&sp);
        ai_free(&a1);
        ai_free(&a2);
    }

    /* THE 5 m/s^2 LIMIT. Step with a coefficient that wants a big jump and check
       the commanded speed cannot move faster than that. */
    {
        ai_t b;
        float pp[3] = { 1e6f, 0.0f, 1e6f };
        float prev, worst = 0.0f;
        int i, worst_ok = 1;
        memset(&b, 0, sizeof(b));
        ai_init(&b, 0, ASSETS, NULL, 1, 0);
        prev = b.car[0].speed;
        for (i = 0; i < 600; i++) {
            float d;
            ai_step(&b, NULL, pp[0], pp[1], pp[2], 0, 1.0f / 60.0f);
            d = (float)fabs(b.car[0].speed - prev);
            if (d > worst)
                worst = d;
            if (d > 5.0f / 60.0f + 0.02f)
                worst_ok = 0;
            prev = b.car[0].speed;
        }
        /* 5.0 WRITTEN OUT, not AI_ACCEL_LIMIT. Reading the constant this check
           exists to guard makes the check pass for any value of it -- which it
           did, until a mutant that set the limit to 500 sailed through. The
           number is FUN_00503880's own immediate at its moveTowards call. */
        ck(worst_ok, "the commanded speed never changes faster than 5 m/s^2"
                     " (worst %.2f)", worst * 60.0f);
        ai_free(&b);
    }
    ai_free(&ai);
}

/* ==========================================================================
 * part 4 -- the pose, and what the car rig reads off it
 * ========================================================================== */
static void part4(void)
{
    ai_t ai;
    ai_car *a;
    float pp[3] = { 1e6f, 0.0f, 1e6f };
    float p0[3];
    int i, moved = 0, spun = 0, susp_moved = 0, steered = 0;
    float lmin = 9.0f, lmax = -9.0f, smin = 9e9f, smax = -9e9f;

    printf("\npart 4: the pose, and the rig's inputs\n");
    memset(&ai, 0, sizeof(ai));
    if (ai_init(&ai, 0, ASSETS, NULL, 1, 0) < 1) {
        ck(0, "beach_1 loaded");
        return;
    }
    a = &ai.car[0];

    /* At rest the pose is sample 0 exactly. */
    sample_pos(a, 0, p0);
    ck(dist3(a->rb.body.x, p0) < 1e-4f,
       "before the first step the car is at sample 0 (%.5f m)",
       dist3(a->rb.body.x, p0));

    for (i = 0; i < 60 * 30; i++) {
        int k;
        ai_step(&ai, NULL, pp[0], pp[1], pp[2], 0, 1.0f / 60.0f);
        if (dist3(a->rb.body.x, p0) > 1.0f)
            moved = 1;
        for (k = 0; k < a->rb.nwheels; k++) {
            if (fabs(a->rb.wheel[k].spin_w) > 1.0f)
                spun = 1;
            if (a->rb.wheel[k].len < lmin) lmin = a->rb.wheel[k].len;
            if (a->rb.wheel[k].len > lmax) lmax = a->rb.wheel[k].len;
        }
        if (a->rb.steer < smin) smin = a->rb.steer;
        if (a->rb.steer > smax) smax = a->rb.steer;
    }
    susp_moved = (lmax - lmin) > 0.01f;
    steered = (smax - smin) > 5.0f;

    ck(moved, "the car moves");
    /* Bound to the rolling relation, not to a spin value: a wheel rolling
       without slip turns at v / r, and the Overkill's radius is 0.0718 m, so at
       a few m/s the rate is tens of rad/s. */
    ck(spun, "the wheels turn (spin rate reached v/r scale)");
    /* AT THE ROLLING RATE -- but only where that relation holds. A wheel in the
     * air is chased at 5 rad/s^2 toward zero rather than at 200 toward its patch
     * speed, so testing one arbitrary instant tests whichever of the two the loop
     * happened to stop on. This version steps until the car is GROUNDED and
     * moving and tests there, which is where v/r is the claim. */
    {
        ai_t b;
        int found = 0;
        memset(&b, 0, sizeof(b));
        ai_init(&b, 0, ASSETS, NULL, 1, 0);
        for (i = 0; i < 60 * 120 && !found; i++) {
            ai_car *c = &b.car[0];
            ai_step(&b, NULL, pp[0], pp[1], pp[2], 0, 1.0f / 60.0f);
            if (c->airborne || c->speed < 3.0f || i < 240)
                continue;
            found = 1;
            {
                float want = c->speed / c->rb.wheel[0].radius;
                float got = (float)fabs(c->rb.wheel[0].spin_w);
                ck(fabs(got - want) < 0.35f * want + 2.0f,
                   "a grounded wheel turns at v/r: %.1f rad/s against %.1f",
                   got, want);
            }
        }
        ck(found, "and a grounded moving instant was found to test");
        ai_free(&b);
    }
    /* The two things carani draws the suspension and the front wheels from, both
       of which come straight out of the recorded state. A frozen value here is a
       car whose wheels are welded on -- which is exactly the bug the antenna had
       when it went into a vertex buffer. */
    ck(susp_moved, "the recorded suspension reaches the rig (%.3f..%.3f m)",
       lmin, lmax);
    ck(steered, "the recorded steer angle reaches the rig (%.1f..%.1f deg)",
       smin, smax);
    /* And it is the RECORDED angle, not something derived: the range must sit
       inside the 35 degrees FUN_004fddd0 clamps to and reach a real lock. */
    ck(smax <= 35.5f && smin >= -35.5f && (smax > 10.0f || smin < -10.0f),
       "the steer range is a driver's, inside the +-35 deg clamp");

    /* AIRBORNE HAS TO BE REACHABLE. These are recorded laps with jumps in them,
     * so `airborne` must be 1 sometimes and 0 mostly -- and it has to agree with
     * the suspension the pose just wrote. A flag that is permanently 0 is what
     * this looked like when the threshold was len_max instead of len_free, and a
     * mutant that reported every wheel grounded was then invisible. Measured
     * over beach_1's five recordings: all wheels hanging on 0.6% to 2.8% of
     * samples. */
    {
        ai_t b;
        int air = 0, ground = 0, disagree = 0, steps = 0;
        memset(&b, 0, sizeof(b));
        ai_init(&b, 0, ASSETS, NULL, 1, 0);
        for (i = 0; i < 60 * 90; i++) {
            int k, loaded = 0;
            ai_step(&b, NULL, pp[0], pp[1], pp[2], 0, 1.0f / 60.0f);
            steps++;
            for (k = 0; k < b.car[0].rb.nwheels; k++)
                if (b.car[0].rb.hit[k].active)
                    loaded++;
            if (b.car[0].airborne) air++; else ground++;
            if ((loaded == 0) != (b.car[0].airborne != 0))
                disagree++;
        }
        ck(air > 0 && ground > air,
           "the car leaves the ground sometimes and not usually"
           " (%d of %d samples airborne, %.1f%%)",
           air, steps, 100.0 * air / steps);
        ck(disagree == 0,
           "and the airborne flag always agrees with the wheel contacts");
        ai_free(&b);
    }

    /* len_extra reaches the wheels that EXIST and not the ones that do not. It
       is 1.4 mm, invisible on its own, and rb_wheel_frame draws through it. */
    {
        int k, zero_real = 0, nonzero_absent = 0;
        for (k = 0; k < RB_MAX_WHEELS; k++) {
            if (k < a->rb.nwheels) {
                if (a->rb.wheel[k].len_extra <= 0.0f)
                    zero_real++;
            } else if (a->rb.wheel[k].len_extra != 0.0f) {
                nonzero_absent++;
            }
        }
        ck(zero_real == 0 && nonzero_absent == 0,
           "len_extra is set on all %d wheels and on none of the %d absent"
           " ones (%.4f m)", a->rb.nwheels, RB_MAX_WHEELS - a->rb.nwheels,
           a->rb.wheel[0].len_extra);
    }

    /* THE ORIENTATION IS INTERPOLATED, not snapped to the nearer sample. Step to
     * the middle of a segment and require the pose's quaternion to sit strictly
     * between its two neighbours -- closer to each than they are to each other.
     * Without this, replacing the slerp with "take the earlier sample" is
     * invisible: at 100 Hz the two differ by a fraction of a degree, which is
     * exactly the argument ai.c makes for slerp-vs-nlerp not mattering, and it
     * cuts both ways. */
    {
        ai_t b;
        int found = 0;
        memset(&b, 0, sizeof(b));
        ai_init(&b, 0, ASSETS, NULL, 1, 0);
        for (i = 0; i < 60 * 60 && !found; i++) {
            ai_car *c;
            ai_step(&b, NULL, pp[0], pp[1], pp[2], 0, 1.0f / 60.0f);
            c = &b.car[0];
            /* Deliberately OFF-CENTRE. A mid-segment u makes u^2 and (1-u)^2
               nearly equal, so inverting the fraction is invisible there --
               which a mutant proved. Ask for a lopsided one. */
            if (c->cursor < 1 || c->u < 0.72f || c->u > 0.95f)
                continue;
            {
                double qa[4], qb[4], dab = 0, da = 0, db = 0;
                int j;
                for (j = 0; j < 4; j++) {
                    qa[j] = (double)c->s[c->cursor - 1].q[j] / AI_Q_SCALE;
                    qb[j] = (double)c->s[c->cursor].q[j] / AI_Q_SCALE;
                    dab += (qb[j] - qa[j]) * (qb[j] - qa[j]);
                    da += ((double)c->rb.body.q[j] - qa[j])
                          * ((double)c->rb.body.q[j] - qa[j]);
                    db += ((double)c->rb.body.q[j] - qb[j])
                          * ((double)c->rb.body.q[j] - qb[j]);
                }
                if (dab < 1e-12)
                    continue;         /* the two samples agree; nothing to see */
                found = 1;
                /* QUANTITATIVE, not `closer to each than they are to each
                 * other`. On a short arc the chord scales with the fraction, so
                 * the SQUARED distances go as u^2 and (1-u)^2 -- and the strict
                 * inequality was worthless: snapping to the earlier sample left
                 * db exactly equal to dab and it still compared as less, by one
                 * float bit. A margin is what makes this a check. */
                {
                    double fa = da / dab, fb = db / dab;
                    double wa = (double)c->u * c->u;
                    double wb = (1.0 - c->u) * (1.0 - c->u);
                    ck(fabs(fa - wa) < 0.08 && fabs(fb - wb) < 0.08,
                       "the orientation is interpolated, at the right fraction:"
                       " u=%.2f gives %.3f/%.3f against u^2=%.3f, (1-u)^2=%.3f",
                       c->u, fa, fb, wa, wb);
                }
            }
        }
        ck(found, "and a mid-segment orientation pair was found to test");
        ai_free(&b);
    }

    /* The interpolation endpoints. Drive to a known (cursor, u) and check the
       pose against the two samples it straddles. */
    {
        float pa[3], pb[3], mid[3];
        int c = a->cursor;
        sample_pos(a, c - 1, pa);
        sample_pos(a, c, pb);
        memcpy(mid, a->rb.body.x, sizeof(mid));
        ck(dist3(mid, pa) <= dist3(pa, pb) + 1e-3f
           && dist3(mid, pb) <= dist3(pa, pb) + 1e-3f,
           "the interpolated pose lies between its two samples");
    }
    ai_free(&ai);
}

/* ==========================================================================
 * part 5 -- laps, placings and the championship roster
 * ========================================================================== */
static void part5(void)
{
    ai_t easy, hard;
    int t, differs = 0;

    printf("\npart 5: the roster, placings and difficulty\n");

    /* The AI<n>Races mask has to actually drop opponents at EASY. beach_1's five
       masks are HARD, all, NORMAL+HARD, all, all -- so EASY fields three. */
    memset(&easy, 0, sizeof(easy));
    memset(&hard, 0, sizeof(hard));
    ai_init(&easy, 0, ASSETS, NULL, 0, 1);
    ai_init(&hard, 0, ASSETS, NULL, 2, 1);
    /* THE GRID IS THE SAME SIZE AT EVERY DIFFICULTY, and the mask picks WHICH
     * three. That is not an assumption about the cap -- it is what the shipped
     * masks say: at EASY exactly three of the five qualify on every one of the
     * ten tracks, so `AI<n>Races` was authored for a three-car field. What the
     * difficulty changes is the line-up, and beach_1's slot 0 (HARD only) is the
     * one to look for. */
    ck(easy.n == AI_MAX_FIELD && hard.n == AI_MAX_FIELD,
       "beach_1 championship fields %d at EASY and %d at HARD",
       easy.n, hard.n);
    {
        int i, differ = 0, promoted = 0;
        for (i = 0; i < easy.n; i++) {
            int j, found = 0;
            for (j = 0; j < hard.n; j++)
                if (strcmp(easy.car[i].path, hard.car[j].path) == 0)
                    found = 1;
            if (!found)
                differ++;
            /* Whichever cars do start keep their own FILE slot, so the spline a
               given opponent races against does not move with the difficulty. */
            for (j = 0; j < hard.n; j++)
                if (strcmp(easy.car[i].path, hard.car[j].path) == 0
                    && easy.car[i].slot != hard.car[j].slot)
                    promoted++;
        }
        ck(differ > 0, "and the LINE-UP differs (%d of EASY's %d do not race at"
                       " HARD)", differ, easy.n);
        ck(promoted == 0, "while every car that starts in both keeps its own"
                          " spline slot");
    }
    /* The difficulty coefficient only applies in a championship. */
    {
        ai_t single;
        memset(&single, 0, sizeof(single));
        ai_init(&single, 0, ASSETS, NULL, 0, 0);
        ck(fabs(single.coeff_static - AI_RACES[0].coeff_common) < 1e-6
           && fabs(hard.coeff_static
                   - AI_RACES[0].coeff_common * AI_DIFFICULTY[2]) < 1e-6,
           "the difficulty coefficient applies in the championship only"
           " (%.4f single, %.4f hard)", single.coeff_static,
           hard.coeff_static);
        ai_free(&single);
    }
    ai_free(&easy);
    ai_free(&hard);

    /* Placings, on a real spine. Park the player at the start and let the field
       run: after a while every opponent must be ahead of it, so the player is
       last. */
    {
        ai_t ai;
        spine_t sp;
        ai_track tr;
        float pp[3];
        int i, ahead = 0;
        memset(&ai, 0, sizeof(ai));
        ai_init(&ai, 0, ASSETS, NULL, 1, 0);
        spine_build(&sp, &ai.car[0]);
        tr.ctx = &sp;
        tr.spine = spine_query;
    tr.lap_progress = progress_query;
        tr.spine_len = sp.total;
        sample_pos(&ai.car[0], 0, pp);
        run(&ai, &tr, pp, 20.0f);
        for (i = 0; i < ai.n; i++)
            if (ai.car[i].spine_dist > ai.player_dist)
                ahead++;
        ck(ahead == ai.n, "20 s in, all %d opponents are ahead of a parked"
                          " player (%d)", ai.n, ahead);
        ck(ai_player_place(&ai) == ai.n + 1,
           "so the player is placed last (%d of %d)",
           ai_player_place(&ai), ai.n + 1);
        /* And the lead the rubber band read is positive for all of them, which
           is the sign convention the direction check in part 3 depends on. */
        {
            int neg = 0;
            for (i = 0; i < ai.n; i++)
                if (ai.car[i].lead <= 0.0f)
                    neg++;
            ck(neg == 0, "and every lead reads positive (%d negative)", neg);
        }
        spine_free(&sp);
        ai_free(&ai);
    }

    /* Every track's ROSTER still carries all five entries -- the cap is on how
       many start, not on what is read. */
    for (t = 0; t < AI_N_RACES; t++)
        if (AI_RACES[t].n != AI_MAX_OPPONENTS)
            differs++;
    ck(differs == 0, "all ten rosters still carry %d entries while %d start",
       AI_MAX_OPPONENTS, AI_MAX_FIELD);
}

/* ==========================================================================
 * part 6 -- a bad file must not be believed
 * ========================================================================== */
static void part6(void)
{
    ai_t ai;
    FILE *f;
    long size;
    unsigned char *buf;
    const char *tmp = "/tmp/aitest_bad";
    char src[192];

    printf("\npart 6: refusing a bad file\n");

    memset(&ai, 0, sizeof(ai));
    ck(ai_init(&ai, 0, "/tmp/aitest_nowhere", NULL, 1, 0) == 0,
       "a missing .aip loads nothing and does not crash");
    ai_free(&ai);

    memset(&ai, 0, sizeof(ai));
    ck(ai_init(&ai, -1, ASSETS, NULL, 1, 0) == 0
       && ai_init(&ai, AI_N_RACES, ASSETS, NULL, 1, 0) == 0,
       "an out-of-range track index loads nothing");
    ai_free(&ai);

    /* A TRUNCATED file. The record still claims its full sample count, so the
     * only thing standing between this and a read past the end of the block is
     * ai_init's range check. Cut it in the middle of the sample blocks so the
     * records survive and the data does not. */
    snprintf(src, sizeof(src), ASSETS "/beach_1.aip");
    f = fopen(src, "rb");
    if (!f) {
        ck(0, "could not open %s to build a truncated copy", src);
        return;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (unsigned char *)malloc((size_t)size);
    if (!buf || fread(buf, 1, (size_t)size, f) != (size_t)size) {
        ck(0, "could not read %s", src);
        fclose(f);
        free(buf);
        return;
    }
    fclose(f);
    {
        char dir[192], path[224];
        snprintf(dir, sizeof(dir), "%s", tmp);
        /* one directory, one file called beach_1.aip inside it */
#ifdef _WIN32
        _mkdir(dir);
#else
        {
            char cmd[300];
            snprintf(cmd, sizeof(cmd), "mkdir -p %s", dir);
            if (system(cmd) != 0)
                ck(0, "mkdir %s", dir);
        }
#endif
        snprintf(path, sizeof(path), "%s/beach_1.aip", dir);
        f = fopen(path, "wb");
        if (f) {
            /* keep the header and the five records, drop most of the samples */
            long keep = 12 + 5 * AI_RECORD_BYTES + 1000;
            fwrite(buf, 1, (size_t)(keep < size ? keep : size), f);
            fclose(f);
            memset(&ai, 0, sizeof(ai));
            ck(ai_init(&ai, 0, dir, NULL, 1, 0) == 0,
               "a truncated .aip is refused rather than pointed into");
            ai_free(&ai);
        } else {
            ck(0, "could not write %s", path);
        }

        /* AND THE RECORD ARRAY ITSELF, which is a different question from the
         * one above: that file keeps all five records and cuts the SAMPLES, so
         * it exercises the per-slot sample-block bound and says nothing about
         * the array the loop walks to REACH it. A header claiming five
         * opponents over one record's worth of bytes read past the blob at
         * ai.c's `memcpy(u16, r + 48, ...)` -- an ASan heap-buffer-overflow,
         * 40 bytes past a 120-byte allocation.
         *
         * Simply cutting inside the record array is NOT an oracle: the garbage
         * records that follow are then rejected by the per-slot sample bound as
         * often as not, so the check passes on the broken code in a plain build
         * and only fails under ASan -- which is the same "build layout decides
         * the verdict" hazard this file already carries for part 4. So slot 0 is
         * made GENUINELY VALID over a compact copy of its own first two real
         * samples, and the header still claims five. The old bound accepts the
         * file (one record's worth of bytes is all it asked for), loads slot 0
         * and returns 1; the record-array bound refuses it and returns 0. That
         * is a deterministic kill with no dependence on what is past the end. */
        f = fopen(path, "wb");
        if (f) {
            unsigned char small[12 + AI_RECORD_BYTES + 2 * AI_SAMPLE_BYTES];
            unsigned int n_s = 2, cyc = 0, off = 12 + AI_RECORD_BYTES;
            unsigned int src_off;
            memcpy(small, buf, 12);                       /* AIP1, count 5 */
            memcpy(small + 12, buf + 12, AI_RECORD_BYTES); /* record 0 verbatim */
            memcpy(&src_off, buf + 12 + 64 + 8, 4);        /* its real samples */
            memcpy(small + 12 + 64 + 0, &n_s, 4);
            memcpy(small + 12 + 64 + 4, &cyc, 4);
            memcpy(small + 12 + 64 + 8, &off, 4);
            memcpy(small + off, buf + src_off, 2 * AI_SAMPLE_BYTES);
            fwrite(small, 1, sizeof(small), f);
            fclose(f);
            memset(&ai, 0, sizeof(ai));
            ck(ai_init(&ai, 0, dir, NULL, 1, 0) == 0,
               "an .aip whose opponent count outruns its records is refused"
               " (and slot 0 alone is valid, so only the count can refuse it)");
            ai_free(&ai);
        } else {
            ck(0, "could not write %s", path);
        }
    }
    free(buf);
}

/* ==========================================================================
 * part 7 -- the shared rig, which is the one thing main.c's draw_ai invented
 *
 * Up to five opponents draw from at most three car scenes, so several of them
 * share ONE scene -- and carani writes its animated matrices INTO that scene's
 * rig. draw_ai's answer is to animate and draw each car before touching the
 * next, and if that ordering is ever broken every opponent on a model draws in
 * the last one's pose. Nothing compiles main.c, so the ordering itself cannot be
 * checked here; what can, and what the whole scheme rests on, is that one bind
 * really does serve several cars and that carani_update genuinely re-poses.
 * ========================================================================== */
static void part7(void)
{
    ai_t ai;
    scene_t sc;
    float pp[3] = { 1e6f, 0.0f, 1e6f };
    int i, a = -1, b = -1;

    printf("\npart 7: one rig, several opponents\n");
    memset(&sc, 0, sizeof(sc));
    if (!scene_load(ASSETS "/car1.vsc", &sc)) {
        ck(0, "car1.vsc loaded");
        return;
    }
    if (!sc.has_rig) {
        ck(0, "car1.vsc carries a rig");
        scene_release(&sc);
        return;
    }
    memset(&ai, 0, sizeof(ai));
    if (ai_init(&ai, 0, ASSETS, NULL, 1, 0) < 2) {
        ck(0, "beach_1 loaded at least two opponents");
        scene_release(&sc);
        return;
    }
    /* Two Overkills. beach_1's slots 0, 2 and 4 all are. */
    for (i = 0; i < ai.n; i++) {
        if (ai.car[i].car != 0)
            continue;
        if (a < 0) a = i; else if (b < 0) b = i;
    }
    if (a < 0 || b < 0) {
        ck(0, "beach_1 fields two opponents on the same model");
        ai_free(&ai);
        scene_release(&sc);
        return;
    }
    ck(1, "slots %d and %d are both car 0, so they must share one scene", a, b);

    /* Run them apart, so their poses genuinely differ. */
    run(&ai, NULL, pp, 12.0f);

    /* ONE bind, for the model. carani_bind resolves the wheel mapping off the
       MOUNT positions, which belong to the model and not to the car, so the
       binding taken against one opponent has to serve the other. */
    carani_bind(&sc.rig, &ai.car[a].rb);
    {
        float ma[CARANI_MAX_PARTS][16];
        float mb[CARANI_MAX_PARTS][16];
        float mc[CARANI_MAX_PARTS][16];
        int n = sc.rig.n, differ = 0, same = 0, k;

        carani_update(&sc.rig, &ai.car[a].rb);
        memcpy(ma, sc.rig.draw, sizeof(float) * 16 * (size_t)n);
        carani_update(&sc.rig, &ai.car[b].rb);
        memcpy(mb, sc.rig.draw, sizeof(float) * 16 * (size_t)n);
        /* Back to the first one: the rig must return to exactly the pose it had,
           which is what makes animate-then-draw-per-car sound. A rig that
           accumulated instead of recomputing would drift here. */
        carani_update(&sc.rig, &ai.car[a].rb);
        memcpy(mc, sc.rig.draw, sizeof(float) * 16 * (size_t)n);

        for (k = 0; k < n; k++) {
            if (memcmp(ma[k], mb[k], sizeof(ma[k])) != 0)
                differ++;
            if (memcmp(ma[k], mc[k], sizeof(ma[k])) == 0)
                same++;
        }
        ck(differ > 0, "posing the second car moves the shared rig (%d of %d"
                       " parts)", differ, n);
        ck(same == n, "and posing the first one again restores it exactly"
                      " (%d of %d parts)", same, n);
        /* The parts that move must include the WHEELS -- the suspension and the
           steer are what the recorded state carries, so if the rig moved but the
           wheels did not, the poses differ for some other reason. */
        {
            int wheels_moved = 0;
            for (k = 0; k < RB_MAX_WHEELS; k++) {
                int p = sc.rig.wheel[k];
                if (p >= 0 && p < n
                    && memcmp(ma[p], mb[p], sizeof(ma[p])) != 0)
                    wheels_moved++;
            }
            ck(wheels_moved >= 2,
               "and the wheel nodes are among them (%d moved)", wheels_moved);
        }
    }
    ai_free(&ai);
    scene_release(&sc);
}

/* ==========================================================================
 * part 8 -- the opponents are SOLID
 *
 * This is the one part of the AI that reaches into the player's own physics, so
 * it gets the treatment a physics change gets: a fixture that drives a real car
 * at a real opponent on a real grid, and a check that it is a NO-OP when nothing
 * is touching.
 *
 * The opponent is held still by simply not calling ai_step -- ai_reset leaves it
 * posed at sample 0, which is its own grid slot -- so this is a static obstacle
 * of exactly the right shape in exactly the right place, and the collision is
 * the only thing moving the player besides its own driving.
 * ========================================================================== */
static void part8(void)
{
    col_t col;
    ai_t ai;
    rb_car pl;
    const rb_world *w;
    float gy, nx, ny, nz;
    float ox, oy, oz, sx, sz, yaw;
    float ps[RB_MAX_SPHERES][4], os[RB_MAX_SPHERES][4];
    int i;

    printf("\npart 8: solid opponents\n");
    memset(&col, 0, sizeof(col));
    if (!col_load(ASSETS "/beach_1.col", &col)) {
        ck(0, "beach_1.col loaded");
        return;
    }
    w = col_rb_world(&col);
    memset(&ai, 0, sizeof(ai));
    if (ai_init(&ai, 0, ASSETS, w, 1, 0) < 1) {
        ck(0, "beach_1 opponents loaded");
        col_free(&col);
        return;
    }
    /* THE DECISION IS HELD OFF HERE. This fixture is about the REACTION --
       the contact solve's two-moving-bodies relative velocity and its pair
       denominator -- and it needs an opponent that arrives. One that steers
       round the parked player measures nothing about either. The decision
       has part 10. See ai_t.steer_off. */
    ai.steer_off = 1;
    ox = ai.car[0].rb.body.x[0];
    oy = ai.car[0].rb.body.x[1];
    oz = ai.car[0].rb.body.x[2];

    /* ---- 1. NO CONTACT MUST BE A NO-OP, byte for byte.
     *
     * The whole field is at the start line; put the player 40 m away and drive
     * it for five seconds twice, once calling ai_collide_player every tick and
     * once not, and require the two cars to end IDENTICAL. Anything less and the
     * transcribed model has quietly acquired a second force. */
    {
        rb_car a2, b2;
        float px = ox + 40.0f, pz = oz + 40.0f;
        int touched = 0;
        if (!col_ground_at(&col, px, pz, 1e9f, &gy, &nx, &ny, &nz))
            gy = oy;
        rbcar_init(&a2, 0, w, px, gy, pz, 0.0f);
        rbcar_init(&b2, 0, w, px, gy, pz, 0.0f);
        for (i = 0; i < 300; i++) {
            rbcar_step(&a2, 1.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
            if (ai_collide_player(&ai, &a2, 1.0f / 60.0f) > 0.0f)
                touched = 1;
            rbcar_step(&b2, 1.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
        }
        ck(!touched, "40 m from the field, nothing reports a contact");
        ck(memcmp(a2.body.x, b2.body.x, sizeof(a2.body.x)) == 0
           && memcmp(a2.body.q, b2.body.q, sizeof(a2.body.q)) == 0
           && memcmp(a2.body.P, b2.body.P, sizeof(a2.body.P)) == 0
           && memcmp(a2.body.L, b2.body.L, sizeof(a2.body.L)) == 0,
           "and five seconds of driving is BYTE-IDENTICAL with the contact call"
           " in and out");
    }

    /* ---- 2. DRIVING AT ONE MUST NOT GO THROUGH IT.
     *
     * Twice: from 3 m, and from 12 m so the car arrives at full speed. The
     * long run-up is the one that tests TUNNELLING -- at 7.5 m/s a 1/60 tick is
     * 12.5 cm, which is bigger than some of the proxy spheres, so a fast car
     * skipping through in one step is a real possibility and not a theoretical
     * one.
     *
     * What is NOT checked is which side of the opponent the player ends up on.
     * An earlier version did, along the approach axis, and it failed on correct
     * behaviour: the car glanced off and slid AROUND, 12 cm clear the whole way,
     * with its centre crossing the opponent's on one axis. Going around is not
     * going through, and the property that separates them is the proxy
     * separation, which is what these check.
     *
     * SINCE OPPONENTS BECAME BUMPABLE THIS FIXTURE HAS TO TRACK ITS TARGET, and
     * both halves of that bit it. `ox, oz` is where the opponent STARTED, and a
     * bumped one leaves: homing on the old spot drives the player into the side
     * of a car that has already moved, and measuring "how close did the centres
     * come" against a place the opponent has vacated reports a car going THROUGH
     * one when it is doing nothing of the kind (0.038 m, on a run whose live
     * separation never dropped below 0.365). Both read the pose every tick now.
     * And the field is reset BETWEEN the two runs, because the 3 m ram leaves the
     * opponent shoved half a metre off its line with nothing to pull it back --
     * part 8 does not call ai_step, so the return spring never runs here.
     *
     * IT ALSO STOPS MEASURING A SECOND AFTER FIRST CONTACT, and that is the more
     * interesting of the two. It used to drive for the full twelve seconds, which
     * against a movable opponent is not an arrival at all -- it is ten seconds of
     * one car bulldozing another that has long since reached its offset limit,
     * with the two climbing over each other. That state is chaotic: sweeping the
     * depenetration pass count over it gives 2 to 8 cm of residual overlap in no
     * order at all, so a bound on it is a bound on noise. Over the second after
     * contact the same sweep is clean and monotone (see AI_DEPEN_PASSES). The
     * car still DRIVES for the full twelve seconds; what changed is how long the
     * assertions look. A sustained shove is a real thing and is checked in part
     * 9, where it can be asked the question that suits it -- can the player still
     * get out -- rather than this one.
     */
    {
        static const float START[2] = { 3.0f, 12.0f };
        int run;
        for (run = 0; run < 2; run++) {
            float worst_overlap = 0.0f, closest = 1e9f;
            float vmax = 0.0f, ymax = -1e9f, vhit = 0.0f;
            int np, no, hits = 0, first_hit = -1;

            ai_reset(&ai);
            ox = ai.car[0].rb.body.x[0];
            oy = ai.car[0].rb.body.x[1];
            oz = ai.car[0].rb.body.x[2];

            /* SPAWN ON THE OPPONENT'S OWN PATH, at about START[run] metres
             * along it, and aim back down it at the opponent. The path is
             * drivable by construction -- part 1 measures 0% of it off the grid
             * -- which a blind offset is not: +12 m along +X from beach_1's
             * start line is off the end of the sand, and the first version of
             * this check spawned there, never reached the opponent at all, and
             * fell far enough to hit the 20 m/s momentum clamp. */
            {
                float run_m = 0.0f;
                int k = 0;
                while (k + 1 < ai.car[0].n && run_m < START[run]) {
                    const ai_sample *s0 = &ai.car[0].s[k];
                    const ai_sample *s1 = &ai.car[0].s[k + 1];
                    run_m += (float)sqrt(
                        (double)(s1->p[0] - s0->p[0]) * (s1->p[0] - s0->p[0])
                        + (double)(s1->p[1] - s0->p[1]) * (s1->p[1] - s0->p[1])
                        + (double)(s1->p[2] - s0->p[2]) * (s1->p[2] - s0->p[2]));
                    k++;
                }
                sx = ai.car[0].s[k].p[0];
                sz = ai.car[0].s[k].p[2];
            }
            if (!col_ground_at(&col, sx, sz, 1e9f, &gy, &nx, &ny, &nz))
                gy = oy;
            /* Heading so local +Z points at the opponent: sb2obj's rotY puts +Z
               on (sin yaw, 0, cos yaw), so yaw = atan2(dx, dz). */
            yaw = (float)(atan2(ox - sx, oz - sz) * 57.29577951);
            rbcar_init(&pl, 0, w, sx, gy, sz, yaw);

            for (i = 0; i < 60 * 12; i++) {
                float d, h, v, st;
                /* WHERE THE OPPONENT IS NOW, not where it started. */
                float tx = ai.car[0].rb.body.x[0];
                float tz = ai.car[0].rb.body.x[2];
                int p, o;
                v = (float)sqrt((double)pl.body.v[0] * pl.body.v[0]
                                + (double)pl.body.v[2] * pl.body.v[2]);
                /* HOME IN, rather than aim once and hope. Over 12 m of sloping
                 * beach a car driving straight ahead does not arrive: the first
                 * version of this missed by 0.59 m at 6.7 m/s, which reads
                 * exactly like tunnelling and was not.
                 *
                 * The steer sign is the port's own documented convention and
                 * this fixture is a live check on it: the signed angle is
                 * positive when the target is to the car's LEFT (+X), a positive
                 * BODY steer angle points the wheels left, and rbcar_step
                 * negates its input to get that angle -- so closing on the
                 * target needs -angle. Get it backwards and the car veers away
                 * and the contact check below fails loudly. */
                {
                    float fwd[3], dir[3], dl, ang, cr;
                    fwd[0] = pl.m[8]; fwd[1] = pl.m[9]; fwd[2] = pl.m[10];
                    dir[0] = tx - pl.body.x[0];
                    dir[1] = 0.0f;
                    dir[2] = tz - pl.body.x[2];
                    dl = (float)sqrt((double)dir[0] * dir[0]
                                     + (double)dir[2] * dir[2]);
                    if (dl > 1e-4f) {
                        dir[0] /= dl; dir[2] /= dl;
                        ang = (float)(acos(fmax(-1.0, fmin(1.0,
                                  (double)fwd[0] * dir[0]
                                  + (double)fwd[2] * dir[2]))) * 57.29577951);
                        cr = fwd[2] * dir[0] - fwd[0] * dir[2];
                        if (cr < 0.0f)
                            ang = -ang;
                        st = -ang / 30.0f;
                        if (st > 1.0f) st = 1.0f;
                        if (st < -1.0f) st = -1.0f;
                    } else {
                        st = 0.0f;
                    }
                }
                rbcar_step(&pl, 1.0f, 0.0f, st, 0, 1.0f / 60.0f);
                h = ai_collide_player(&ai, &pl, 1.0f / 60.0f);
                if (h > 0.0f) {
                    if (!hits) {
                        vhit = v;
                        first_hit = i;
                    }
                    hits++;
                }
                /* the arrival, not the grind -- see above */
                if (first_hit >= 0 && i > first_hit + 60)
                    continue;
                d = (float)hypot(pl.body.x[0] - tx, pl.body.x[2] - tz);
                if (d < closest)
                    closest = d;
                np = rb_gather_spheres(&pl, ps);
                no = rb_gather_spheres(&ai.car[0].rb, os);
                for (p = 0; p < np; p++)
                    for (o = 0; o < no; o++) {
                        double ex = (double)ps[p][0] - os[o][0];
                        double ey = (double)ps[p][1] - os[o][1];
                        double ez = (double)ps[p][2] - os[o][2];
                        double sum = (double)ps[p][3] + os[o][3];
                        double ov = sum - sqrt(ex * ex + ey * ey + ez * ez);
                        if (ov > worst_overlap)
                            worst_overlap = (float)ov;
                    }
                v = (float)sqrt((double)pl.body.v[0] * pl.body.v[0]
                                + (double)pl.body.v[1] * pl.body.v[1]
                                + (double)pl.body.v[2] * pl.body.v[2]);
                if (v > vmax) vmax = v;
                if (pl.body.x[1] > ymax) ymax = pl.body.x[1];
            }
            ck(hits > 0, "%.0f m run-up down its own path: contact reported"
                         " (%d ticks, first at %.2f m/s)",
               START[run], hits, vhit);
            /* THE ANTI-TUNNELLING PAIR. The proxies must never be found deeply
               inside each other, and the two centres must stay apart -- a car
               that went through has to pass close to the other's centre. The
               bounds are 3x and about half the measured values, so they are not
               a restatement of the run. */
            ck(worst_overlap < 0.03f,
               "  worst overlap left standing %.3f m, against spheres 3-7 cm in"
               " radius", worst_overlap);
            /* 0.06 m was set against an opponent that could not move. Now that it
               can, the pair comes apart rather than the player squeezing past, and
               the measured minimum is 0.32 m -- but the BOUND stays where it was,
               because it is the tunnelling bound and tunnelling is what it is for.
               Raising it to just under what the run happens to achieve would be
               restating the run. */
            ck(closest > 0.06f, "  closest the two centres came was %.3f m",
               closest);
            /* AND IT MUST NOT LAUNCH THE CAR. A contact solve that
               over-corrects shows up here first: rb_coll_resolve forced every
               tick is documented as launching a car off the ground. */
            ck(vmax < 12.0f, "  not launched (peak speed %.1f m/s against a"
                             " 7.5 m/s top speed)", vmax);
            ck(ymax - gy < 1.0f, "  nor thrown into the air (peak %.2f m above"
                                 " its own spawn ground)", ymax - gy);
        }
    }

    /* ---- 3. AN OPPONENT THAT DRIVES INTO THE PLAYER MUST SHOVE IT.
     *
     * This is the case the moving-body term exists for, and the only one that can
     * see it: rb_coll_resolve's own relative velocity is a body against a world
     * that does not move, and ai_collide_player subtracts the opponent's point
     * velocity so a car being rear-ended is pushed along instead of being read as
     * already separating. Every other check here holds the opponent still -- part
     * 8 deliberately does not call ai_step -- so with the opponent stationary a
     * mutant that dropped that term was invisible.
     *
     * Park the player a few metres up the opponent's own path and let the
     * opponent drive at it. It must be reported, and the player must end up
     * FURTHER ALONG the opponent's direction of travel than it started.
     */
    {
        float run_m = 0.0f, sxp, szp, d_along = 0.0f, best_hit = 0.0f;
        float start[3], dirv[3] = { 0.0f, 0.0f, 0.0f };
        float vpeak = 0.0f, ospeed = 0.0f, after[3];
        int k = 0, hits = 0, hit_frame = -1;

        ai_reset(&ai);
        while (k + 1 < ai.car[0].n && run_m < 6.0f) {
            const ai_sample *s0 = &ai.car[0].s[k];
            const ai_sample *s1 = &ai.car[0].s[k + 1];
            run_m += (float)sqrt(
                (double)(s1->p[0] - s0->p[0]) * (s1->p[0] - s0->p[0])
                + (double)(s1->p[1] - s0->p[1]) * (s1->p[1] - s0->p[1])
                + (double)(s1->p[2] - s0->p[2]) * (s1->p[2] - s0->p[2]));
            k++;
        }
        sxp = ai.car[0].s[k].p[0];
        szp = ai.car[0].s[k].p[2];
        if (!col_ground_at(&col, sxp, szp, 1e9f, &gy, &nx, &ny, &nz))
            gy = oy;
        rbcar_init(&pl, 0, w, sxp, gy, szp, 0.0f);
        memcpy(start, pl.body.x, sizeof(start));
        /* Seeded, not left on the stack: if no hit ever lands the window is
           never written, and a d_along built out of stack garbage is exactly the
           class of bug this file already carries one of (see ai_init in
           CLAUDE.md's Known issues). Seeded from `start` it reads 0 and the
           check fails, which is the right answer for "nothing hit it". */
        memcpy(after, start, sizeof(after));

        for (i = 0; i < 60 * 12; i++) {
            float h;
            ai_step(&ai, NULL, pl.body.x[0], pl.body.x[1], pl.body.x[2], 0,
                    1.0f / 60.0f);
            /* A tiny throttle, ONLY so the rest clamp cannot put the player to
               sleep before the opponent arrives: FUN_004f6610 zeroes P and L and
               skips the whole step after two seconds under 0.18 m/s, and an
               impulse written into a car that then skips its step goes nowhere.
               0.02 is far too little to move it on its own -- measured below. */
            rbcar_step(&pl, 0.02f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
            h = ai_collide_player(&ai, &pl, 1.0f / 60.0f);
            if (h > 0.0f) {
                hits++;
                if (h > best_hit) {
                    best_hit = h;
                    memcpy(dirv, ai.car[0].rb.body.v, sizeof(dirv));
                    ospeed = ai.car[0].speed;
                    hit_frame = i;
                }
            }
            /* The displacement is read HALF A SECOND after the hardest hit, not
             * at the end of the run -- see the check below for why the end of
             * the run stopped meaning anything. */
            if (hit_frame >= 0 && i == hit_frame + 30)
                memcpy(after, pl.body.x, sizeof(after));
            {
                float v = (float)sqrt((double)pl.body.v[0] * pl.body.v[0]
                                      + (double)pl.body.v[2] * pl.body.v[2]);
                if (v > vpeak)
                    vpeak = v;
            }
        }
        ck(hits > 0, "an opponent driving into a parked player is reported"
                     " (%d ticks, hardest %.2f m/s)", hits, best_hit);
        if (hits > 0) {
            double dl = sqrt((double)dirv[0] * dirv[0] + (double)dirv[2] * dirv[2]);
            if (hit_frame < 0 || hit_frame + 30 >= 60 * 12)
                memcpy(after, pl.body.x, sizeof(after));
            if (dl > 1e-4) {
                d_along = (float)(((double)(after[0] - start[0]) * dirv[0]
                                   + (double)(after[2] - start[2]) * dirv[2])
                                  / dl);
            }
            /* BOTH ENDS. Knocked clear, not bulldozed: an opponent that drives
             * THROUGH the player, pushing it out of the way a centimetre a tick
             * for as long as it takes, moves it much further than a hit does.
             * Measured: 0.75 m when the closing speed is read properly, 9.84 m
             * when the opponent's own point velocity is dropped from it.
             *
             * THE WINDOW IS HALF A SECOND AFTER THE HARDEST HIT, and it has to
             * be. This used to read the position at the END of the 12 s run,
             * which worked only because the player was being brought to a dead
             * STOP two seconds after the knock -- 4.45 m/s to 0.00 with no brake
             * applied, on sand. That was wheel_probe measuring its retraction
             * along the contact normal instead of along the strut (see its note
             * in collide.c): every ripple in the terrain failed to resolve, and
             * a failed retraction is a contact event, so the car was standing on
             * the brakes the whole way. With that fixed the car coasts, as a
             * free-rolling car does -- 12.8 m over ten seconds down a 4.6 degree
             * slope, which is 0.69 m/s^2 of deceleration against bearing drag
             * and air alone -- and the run's end says nothing about the knock.
             *
             * The knock itself never moved: 0.29 m before, 0.28 m after, at the
             * hit frame, and 1.61 against 1.72 m half a second later.
             *
             * THE LOWER BOUND IS RE-DERIVED FOR THE WINDOW, not carried over.
             * 0.25 m was picked against a 12 s total and is far too loose for
             * half a second -- bulldozing is slow by construction, a centimetre
             * a tick, and it still clears 0.25 m. Measured on the mutant this
             * check names, the one that drops the opponent's own point velocity
             * from vrel: 0.42 m in the window against 1.72 m for a real knock.
             * 1.0 m sits 2.4x above the mutant and 1.6x below the real hit.
             * (That mutant also dies on the closing-speed check below, 0.27 m/s
             * reported against 5.25 m/s of opponent, so the suite catches it
             * either way -- but the check that names a failure mode should be
             * the one that sees it.) */
            ck(d_along > 1.0f && d_along < 5.0f,
               "and the player is knocked ALONG the opponent's travel, not"
               " bulldozed (%.2f m)", d_along);
            /* THE REPORTED CLOSING SPEED IS THE ONE THAT SEPARATES THEM, and it
             * is also what the sound's gain reads. A parked player hit by a car
             * doing `ospeed` closes at about `ospeed`; with the opponent read as
             * stationary the same hit reports 0.15 m/s. */
            ck(ospeed > 1.0f && best_hit > 0.5f * ospeed,
               "and the closing speed is the opponent's own: %.2f m/s reported"
               " against %.2f m/s of opponent", best_hit, ospeed);
            /* AND IT IS THE IMPULSE DOING IT, not just the positional push.
             * The distance alone cannot tell them apart -- shoving the car out of
             * the way a centimetre a tick moves it just as far -- so bind to the
             * SPEED the hit imparts. rb_coll_resolve's law gives
             * dv = 0.05 - vrel, so a 3.9 m/s closing speed should leave the
             * player doing metres per second; with the opponent's own point
             * velocity dropped from vrel it would leave it doing the 0.05 m/s
             * separation term and nothing else. 1.0 m/s sits 20x above that. */
            /* AND NOT MORE SPEED THAN THE CAR THAT HIT IT HAD. This is the
             * two-body split, and it is the one thing that says the impulse was
             * shared: two cars of the same mass meeting mostly inelastically
             * leave the struck one at about HALF the closing speed, and it can
             * never leave at more than all of it. Forget the opponent's share of
             * the denominator and the player takes an impulse sized for a solve
             * against an immovable wall -- which is what this used to be, and
             * which nothing else here can see, because every other check on the
             * hit is a floor. */
            ck(vpeak > 0.30f * best_hit && vpeak < 0.85f * best_hit,
               "and it takes about HALF of it, as an equal mass must (%.2f m/s"
               " off a %.2f m/s closing speed)", vpeak, best_hit);
            ck(vpeak > 1.0f,
               "and the hit imparts real SPEED (%.2f m/s peak, against the"
               " 0.05 m/s the separation term alone would give)", vpeak);
        } else {
            ck(0, "and the player is shoved along the opponent's travel");
        }
    }

    /* ---- 4. the returned speed IS the closing speed. Drive at it again and
     * compare the first reported impact against the speed the car had on the
     * tick before it. */
    {
        float prev = 0.0f, first = -1.0f, at = 0.0f;
        /* Put the field back on its grid: the block above LET the opponent drive,
           so without this it is most of a lap away and there is nothing to hit. */
        ai_reset(&ai);
        if (!col_ground_at(&col, ox + 3.0f, oz, 1e9f, &gy, &nx, &ny, &nz))
            gy = oy;
        yaw = (float)(atan2(ox - (ox + 3.0f), 0.0) * 57.29577951);
        rbcar_init(&pl, 0, w, ox + 3.0f, gy, oz, yaw);
        for (i = 0; i < 60 * 6 && first < 0.0f; i++) {
            float h;
            prev = (float)sqrt((double)pl.body.v[0] * pl.body.v[0]
                               + (double)pl.body.v[2] * pl.body.v[2]);
            rbcar_step(&pl, 1.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
            h = ai_collide_player(&ai, &pl, 1.0f / 60.0f);
            if (h > 0.0f) { first = h; at = prev; }
        }
        ck(first > 0.0f && first <= at + 0.5f && first > at * 0.25f,
           "the reported impact %.2f m/s is the closing speed (car was doing"
           " %.2f)", first, at);
    }

    ai_free(&ai);
    col_free(&col);
}

/* ==========================================================================
 * part 9 -- the opponents are BUMPABLE
 *
 * Part 8 asks whether an opponent is SOLID. This asks the other half: that a hit
 * moves it, that it comes back, and -- the design claim the whole thing rests on
 * -- that none of that reaches the replay.
 *
 * Every check here is bound to something the bump's own constants cannot move:
 * a bit-for-bit comparison against an unbumped twin, a sign that comes out of the
 * geometry, a ratio, or the car's own recovered dimensions. The one place a
 * recovered number appears (the grip cap) it is written out here from RB_CARS
 * rather than read off the ai_car, so a mutant has to break it twice.
 * ========================================================================== */

/* The impulse that will move a 2 kg car at `mps` metres per second. */
static void bump_j(const ai_car *a, float dx, float dy, float dz, float mps,
                   float j[3])
{
    double n = sqrt((double)dx * dx + (double)dy * dy + (double)dz * dz);
    double k = (n > 1e-9) ? (double)a->rb.body.mass * mps / n : 0.0;
    j[0] = (float)(dx * k);
    j[1] = (float)(dy * k);
    j[2] = (float)(dz * k);
}

/* The worst overlap between two proxies, metres, 0 for none. */
static float proxy_overlap(const rb_car *a, const rb_car *b)
{
    float as[RB_MAX_SPHERES][4], bs[RB_MAX_SPHERES][4];
    int na = rb_gather_spheres(a, as), nb = rb_gather_spheres(b, bs), i, k;
    double worst = 0.0;

    for (i = 0; i < na; i++)
        for (k = 0; k < nb; k++) {
            double ex = (double)as[i][0] - bs[k][0];
            double ey = (double)as[i][1] - bs[k][1];
            double ez = (double)as[i][2] - bs[k][2];
            double sum = (double)as[i][3] + bs[k][3];
            double ov = sum - sqrt(ex * ex + ey * ey + ez * ez);
            if (ov > worst)
                worst = ov;
        }
    return (float)worst;
}

static void part9(void)
{
    col_t col;
    ai_t ai, ref;
    spine_t sp;
    ai_track tr;
    float pp[3];
    int i;

    printf("\npart 9: bumpable opponents\n");
    memset(&col, 0, sizeof(col));
    if (!col_load(ASSETS "/beach_1.col", &col)) {
        ck(0, "beach_1.col loaded");
        return;
    }
    memset(&ai, 0, sizeof(ai));
    memset(&ref, 0, sizeof(ref));
    if (ai_init(&ai, 0, ASSETS, col_rb_world(&col), 1, 0) < 1
        || ai_init(&ref, 0, ASSETS, col_rb_world(&col), 1, 0) < 1) {
        ck(0, "beach_1 opponents loaded twice");
        col_free(&col);
        return;
    }
    /* THE DECISION IS HELD OFF HERE. This fixture is about the REACTION --
       the contact solve's two-moving-bodies relative velocity and its pair
       denominator -- and it needs an opponent that arrives. One that steers
       round the parked player measures nothing about either. The decision
       has part 10. See ai_t.steer_off. */
    ai.steer_off = ref.steer_off = 1;
    if (!spine_build(&sp, &ref.car[0])) {
        ck(0, "spine built");
        return;
    }
    tr.ctx = &sp;
    tr.spine = spine_query;
    tr.lap_progress = progress_query;
    tr.spine_len = sp.total;
    /* A FIELD OF ONE for the first three cases, so the impulse is the only thing
     * that can move this car. Left at the layout's three, the very first version
     * of case 1 shoved opponent 0 half a metre off the grid and INTO its
     * neighbour -- the offset came back pointing somewhere else entirely, the car
     * picked up 21 degrees of yaw from an impulse through its own centre of mass,
     * and it never got back to its line because it was leaning on another car.
     * Which is opponent-against-opponent contact working, and is case 5's
     * business; here it is a confound. */
    ai.n = 1;
    ref.n = 1;
    /* The player is parked far away throughout part 9's first four cases: this is
       about the opponent and the impulse, not about a collision. */
    pp[0] = ref.car[0].rb.body.x[0] + 500.0f;
    pp[1] = ref.car[0].rb.body.x[1];
    pp[2] = ref.car[0].rb.body.x[2] + 500.0f;

    /* ---- 1. A HIT MOVES IT, IT COMES BACK, AND THE REPLAY NEVER NOTICES.
     *
     * Two identical fields, stepped identically. One of them takes an impulse
     * worth 2 m/s sideways at its centre of mass. Everything the REPLAY owns --
     * the cursor, the fraction through the segment, the lap, the distance walked,
     * the commanded speed, the recorded speed, the rubber-band coefficient -- is
     * then compared against the twin EVERY TICK and must be bit-identical, and
     * once the offset has gone the POSE must be bit-identical too.
     *
     * That last one is the check with no tunable in it at all. It can only pass
     * if the offset returns to EXACTLY zero (AI_BUMP_SNAP) and if the pose is
     * rebuilt from the recording rather than accumulated -- so it kills, at once,
     * a missing snap, an offset that leaks into the recorded pose, and a
     * quaternion renormalised on a car that was never turned. */
    {
        int diverged = -1, ident_at = -1, k;
        float peak = 0.0f, peak_x = 0.0f, peak_pose = 0.0f, j[3];
        float ret_t = -1.0f;

        for (i = 0; i < 60 * 6; i++) {
            if (i == 60) {
                bump_j(&ai.car[0], 1.0f, 0.0f, 0.0f, 2.0f, j);
                ai_bump_impulse(&ai, 0, ai.car[0].rb.body.x, j);
            }
            ai_step(&ai,  &tr, pp[0], pp[1], pp[2], 0, 1.0f / 60.0f);
            ai_step(&ref, &tr, pp[0], pp[1], pp[2], 0, 1.0f / 60.0f);

            if (ai.car[0].bump > peak) {
                peak = ai.car[0].bump;
                peak_x = ai.car[0].off[0];
                /* WHERE THE CAR ACTUALLY IS, against the twin. `off` is a field;
                   this is the pose, and the two have to agree or ai_bump_apply is
                   not doing what the field says. A version that added the offset
                   to wherever the body already was instead of rebuilding from the
                   recording passed every other check here -- it moved the car
                   twice as far as its own telemetry claimed. */
                peak_pose = dist3(ai.car[0].rb.body.x, ref.car[0].rb.body.x);
            }
            if (i > 60 && peak > 0.0f && ret_t < 0.0f
                && ai.car[0].bump < 0.1f * peak)
                ret_t = (float)(i - 60) / 60.0f;

            if (diverged < 0
                && (ai.car[0].cursor != ref.car[0].cursor
                    || ai.car[0].u    != ref.car[0].u
                    || ai.car[0].lap  != ref.car[0].lap
                    || ai.car[0].dist != ref.car[0].dist
                    || ai.car[0].speed != ref.car[0].speed
                    || ai.car[0].speed_rec != ref.car[0].speed_rec
                    || ai.car[0].coeff != ref.car[0].coeff))
                diverged = i;
            if (ident_at < 0 && i > 60
                && memcmp(ai.car[0].rb.body.x, ref.car[0].rb.body.x,
                          sizeof(ai.car[0].rb.body.x)) == 0
                && memcmp(ai.car[0].rb.body.q, ref.car[0].rb.body.q,
                          sizeof(ai.car[0].rb.body.q)) == 0)
                ident_at = i;
        }

        /* A wheel radius is 0.072 m; anything under that is not a bump, it is
           float noise. The upper bound is the car's own length. */
        ck(peak > 0.072f && peak < RB_CARS[0].extent[2],
           "a 2 m/s impulse moves the opponent %.3f m off its line (between a"
           " wheel radius and a car length)", peak);
        ck(peak_x > 0.9f * peak,
           "  and it moves the way it was PUSHED (%.3f of %.3f m on +X)",
           peak_x, peak);
        /* The tolerance is for the terrain follow, which is real displacement the
           offset field does not carry: 5 cm is generous on beach_1's slope over a
           quarter-metre sideways, and an eighth of what getting the composition
           wrong costs. */
        ck(fabs((double)peak_pose - peak) < 0.05,
           "  and the CAR is where the offset says: %.3f m from the unbumped"
           " twin against an offset of %.3f", peak_pose, peak);
        ck(diverged < 0,
           "  while the REPLAY is bit-identical to the unbumped twin on every"
           " tick: cursor, u, lap, distance, both speeds and the coefficient");
        ck(ai.car[0].bump == 0.0f && ai.car[0].off[0] == 0.0f
           && ai.car[0].offv[0] == 0.0f,
           "  the offset returns to EXACTLY zero, not to nearly zero");
        ck(ident_at > 0,
           "  and the recovered car is then bit-identical to the twin, pose and"
           " orientation (from tick %d, %.2f s after the hit)",
           ident_at, (float)(ident_at - 60) / 60.0f);
        /* Not a teleport and not a sulk. A quarter second is faster than any car
           corrects a half-metre shove; four is slower than a race cares about. */
        ck(ret_t > 0.25f && ret_t < 4.0f,
           "  the return takes %.2f s to shed 90%% of it -- driving, not"
           " snapping", ret_t);
        (void)k;
    }

    /* ---- 2. IT CANNOT BE SHOVED OFF THE TRACK, AND IT PULLS ITSELF BACK NO
     * HARDER THAN ITS TYRES COULD.
     *
     * A 50 m/s impulse -- seven times the car's own top speed -- must not put it
     * anywhere a 2 m/s one could not eventually reach, and the recovery from it
     * must stay inside grip * gravity. The cap is written out here from the car's
     * own recovered tyre coefficient rather than read off the ai_car, so it is a
     * second copy: a mutant that removes the cap has to survive both. */
    {
        float j[3], worst = 0.0f, worst_acc = 0.0f;
        float prev[3];
        const float cap = RB_CARS[0].tune.coeff_rear_tires * RB_GRAVITY;

        ai_reset(&ai);
        bump_j(&ai.car[0], 1.0f, 0.0f, 0.3f, 50.0f, j);
        ai_bump_impulse(&ai, 0, ai.car[0].rb.body.x, j);
        memcpy(prev, ai.car[0].offv, sizeof(prev));
        for (i = 0; i < 60 * 5; i++) {
            float acc;
            int at_limit = ai.car[0].bump > 0.95f * ai.car[0].bump_limit;
            ai_step(&ai, &tr, pp[0], pp[1], pp[2], 0, 1.0f / 60.0f);
            if (ai.car[0].bump > worst)
                worst = ai.car[0].bump;
            acc = dist3(ai.car[0].offv, prev) * 60.0f;
            /* Not the tick the impulse lands on, which is the impulse; and not
               while the offset is pinned at its limit, where the velocity change
               is the CLAMP discarding a shove that is not going anywhere rather
               than the car pulling on its tyres. Measuring that read 41.6 m/s^2
               of "recovery" from a 50 m/s hit, which is the clamp's arithmetic
               and not an acceleration at all. */
            if (i > 0 && !at_limit && acc > worst_acc)
                worst_acc = acc;
            memcpy(prev, ai.car[0].offv, sizeof(prev));
        }
        /* The limit is two proxy reaches, so it has to be about a car's size --
           checked against the car's own length rather than against the constant
           that produced it. */
        ck(worst <= ai.car[0].bump_limit + 1e-4f
           && ai.car[0].bump_limit > 0.5f * RB_CARS[0].extent[2]
           && ai.car[0].bump_limit < 2.0f * RB_CARS[0].extent[2],
           "a 50 m/s impulse moves it %.3f m, held at its %.3f m limit (between"
           " half a car length and two)", worst, ai.car[0].bump_limit);
        ck(worst_acc < cap * 1.05f,
           "  and it pulls itself back at %.2f m/s^2 against its own grip times"
           " gravity, %.2f", worst_acc, cap);
        ck(ai.car[0].bump == 0.0f,
           "  and five seconds later it is back on its line exactly");

        /* AND THE CAP HAS TO BE REACHED FOR THAT TO MEAN ANYTHING. A 50 m/s
         * impulse crosses the whole offset limit inside a single tick, so it
         * spends its whole life pinned at the clamp and never asks the spring
         * for anything -- a build with the cap deleted passed the check above
         * unchanged. What asks for more than the cap is a MODERATE shove still
         * travelling outward: at 5 m/s the damping term alone wants
         * 2*w*5 = 35 m/s^2 for the several ticks it takes to reach the limit. */
        {
            float pv[3], acc, wa2 = 0.0f;
            ai_reset(&ai);
            bump_j(&ai.car[0], 1.0f, 0.0f, 0.0f, 5.0f, j);
            ai_bump_impulse(&ai, 0, ai.car[0].rb.body.x, j);
            memcpy(pv, ai.car[0].offv, sizeof(pv));
            for (i = 0; i < 60 * 3; i++) {
                ai_step(&ai, &tr, pp[0], pp[1], pp[2], 0, 1.0f / 60.0f);
                acc = dist3(ai.car[0].offv, pv) * 60.0f;
                if (i > 0 && ai.car[0].bump < 0.95f * ai.car[0].bump_limit
                    && acc > wa2)
                    wa2 = acc;
                memcpy(pv, ai.car[0].offv, sizeof(pv));
            }
            ck(wa2 > 0.5f * cap && wa2 < cap * 1.05f,
               "  a 5 m/s shove has it pulling at %.2f m/s^2 -- at the cap of"
               " %.2f and not through it", wa2, cap);
        }
    }

    /* ---- 3. THE YAW COMES OUT OF THE GEOMETRY.
     *
     * Three impulses, all the same sideways push, at the nose, at the tail and
     * through the centre of mass. The two ends must turn the car OPPOSITE ways
     * and the middle one must not turn it at all. No sign convention is asserted
     * and no magnitude is: what is checked is that the lever arm is in there,
     * which is the whole content of ai_take_impulse's r x j. */
    {
        float yaw_nose = 0.0f, yaw_tail = 0.0f, yaw_mid = 0.0f;
        int c;
        for (c = 0; c < 3; c++) {
            float j[3], p[3], best = 0.0f;
            const float *m;
            ai_reset(&ai);
            m = ai.car[0].rb.m;                       /* row 2 is body +Z */
            p[0] = ai.car[0].rb.body.x[0];
            p[1] = ai.car[0].rb.body.x[1];
            p[2] = ai.car[0].rb.body.x[2];
            if (c != 2) {
                float d = (c == 0 ? 0.2f : -0.2f);
                p[0] += m[8] * d;
                p[1] += m[9] * d;
                p[2] += m[10] * d;
            }
            /* sideways in the car's own frame: row 0 is body +X */
            bump_j(&ai.car[0], m[0], m[1], m[2], 2.0f, j);
            ai_bump_impulse(&ai, 0, p, j);
            for (i = 0; i < 60 * 2; i++) {
                ai_step(&ai, &tr, pp[0], pp[1], pp[2], 0, 1.0f / 60.0f);
                if (fabs((double)ai.car[0].off_yaw) > fabs((double)best))
                    best = ai.car[0].off_yaw;
            }
            if (c == 0) yaw_nose = best;
            else if (c == 1) yaw_tail = best;
            else yaw_mid = best;
        }
        ck(yaw_nose * yaw_tail < 0.0f
           && fabs((double)yaw_nose) > 0.01 && fabs((double)yaw_tail) > 0.01,
           "a shove at the nose and the same shove at the tail turn it OPPOSITE"
           " ways (%.1f and %.1f degrees)",
           yaw_nose * 57.2957795f, yaw_tail * 57.2957795f);
        ck(fabs((double)yaw_mid)
           < 0.2 * fabs((double)(yaw_nose < 0 ? -yaw_nose : yaw_nose)),
           "  and one through the centre of mass barely turns it at all (%.2f"
           " degrees)", yaw_mid * 57.2957795f);
    }

    spine_free(&sp);
    ai_free(&ai);
    ai_free(&ref);
    col_free(&col);

    /* ---- 4. THE REPORTED CASE: A BUGGY UNDER A HUMMER.
     *
     * "player car buggy could stuck in ai opponent hummer". country_3's first
     * opponent IS a Hummer (fishers/hummer1, and ai_data.h has the roster), which
     * is 15 proxy spheres and the biggest car in the game, and the player here is
     * the Buggy. The player is parked ON the Hummer's own recorded line -- the
     * worst place to be, and the one a real player finds by spinning -- and the
     * Hummer is driven into it at its own recorded speed.
     *
     * What is checked is not that they never overlap; two cars in contact
     * overlap. It is that the state does not PERSIST: the pair must not stay
     * deeply inside each other, and when it is over the player must be free and
     * able to drive off. Being shoved is the correct outcome and being stuck is
     * the bug.
     */
    {
        ai_t h;
        rb_car pl, ctl;
        const rb_world *w;
        float gy, nx, ny, nz, start[3], j[3];
        float worst = 0.0f, deep_run = 0, deep_worst = 0, hbump = 0.0f;
        int k = 0, hit_ticks = 0;
        float run_m = 0.0f;

        memset(&col, 0, sizeof(col));
        if (!col_load(ASSETS "/country_3.col", &col)) {
            ck(0, "country_3.col loaded");
            return;
        }
        w = col_rb_world(&col);
        memset(&h, 0, sizeof(h));
        if (ai_init(&h, 6, ASSETS, w, 1, 0) < 1) {
            ck(0, "country_3 opponents loaded");
            col_free(&col);
            return;
        }
        /* THE DECISION IS HELD OFF HERE. This fixture is about the REACTION --
           the contact solve's two-moving-bodies relative velocity and its pair
           denominator -- and it needs an opponent that arrives. One that steers
           round the parked player measures nothing about either. The decision
           has part 10. See ai_t.steer_off. */
        h.steer_off = 1;
        ck(h.car[0].car == 2 && RB_CARS[h.car[0].car].nwheels == 6,
           "country_3's first opponent is the Hummer (%s, %d wheels, %d proxy"
           " spheres)", h.car[0].name, RB_CARS[h.car[0].car].nwheels,
           RB_CARS[h.car[0].car].nwheels + 9);

        /* Six metres up its own path, which is drivable by construction. */
        while (k + 1 < h.car[0].n && run_m < 6.0f) {
            const ai_sample *s0 = &h.car[0].s[k], *s1 = &h.car[0].s[k + 1];
            run_m += dist3(s1->p, s0->p);
            k++;
        }
        start[0] = h.car[0].s[k].p[0];
        start[2] = h.car[0].s[k].p[2];
        if (!col_ground_at(&col, start[0], start[2], 1e9f, &gy, &nx, &ny, &nz))
            gy = h.car[0].s[k].p[1];
        start[1] = gy;
        /* car 1, the Buggy, sideways across the path so it cannot be nudged
           along and out of the way for free. */
        rbcar_init(&pl, 1, w, start[0], gy, start[2], 90.0f);

        /* PHASE A -- RUN OVER. The Buggy sits still, sideways across the
         * Hummer's line, and the Hummer arrives at its own recorded speed.
         *
         * A CONTROL CAR sits beside it in the arithmetic sense: the same Buggy at
         * the same spawn for the same six seconds with no Hummer anywhere near
         * it. What that is for is the one failure being run over can produce that
         * looks like nothing at all -- being pressed INTO THE GROUND. The
         * separation between two cars has no idea the world is there, and a car
         * pushed into terrain does not report as broken: it is upright, awake,
         * all four wheels "touching", and it simply does not drive. Every check
         * on WHERE the car ended up passes straight through that; one number on
         * how far its springs are compressed does not.
         *
         * It has never fired. That is worth saying rather than leaving implied:
         * ai_pair_resolve gives the vertical separation to whichever car is on
         * top precisely so it cannot, and with that rule replaced by a mass split
         * this reads 0.145 m against the control's 0.146. It is here because it
         * is the cheap witness for a failure mode the rest of the fixture is
         * blind to, not because it caught something. */
        rbcar_init(&ctl, 1, w, start[0], gy, start[2], 90.0f);
        for (i = 0; i < 60 * 6; i++) {
            rbcar_step(&ctl, 0.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
        }
        for (i = 0; i < 60 * 6; i++) {
            float ov;
            ai_step(&h, NULL, pl.body.x[0], pl.body.x[1], pl.body.x[2], 0,
                    1.0f / 60.0f);
            rbcar_step(&pl, 0.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
            if (ai_collide_player(&h, &pl, 1.0f / 60.0f) > 0.0f)
                hit_ticks++;
            ov = proxy_overlap(&pl, &h.car[0].rb);
            if (ov > worst) worst = ov;
            if (h.car[0].bump > hbump) hbump = h.car[0].bump;
            if (ov > 0.02f) {
                deep_run++;
                if (deep_run > deep_worst) deep_worst = deep_run;
            } else {
                deep_run = 0;
            }
        }
        ck(hit_ticks > 0, "the Hummer reaches the parked Buggy (%d contact"
                          " ticks)", hit_ticks);
        ck(dist3(pl.body.x, start) > 0.1f,
           "  and SHOVES it %.2f m rather than driving over it on the spot",
           dist3(pl.body.x, start));
        ck(hbump > 0.01f,
           "  while the Hummer is itself pushed %.3f m off its line -- the"
           " report's own pairing, both ways", hbump);
        {
            float lr = 0.0f, lc = 0.0f;
            for (i = 0; i < 4; i++) {
                lr += pl.wheel[i].len;
                lc += ctl.wheel[i].len;
            }
            lr *= 0.25f;
            lc *= 0.25f;
            ck(lr > 0.75f * lc,
               "  and it is NOT pressed into the ground: springs at %.3f m"
               " against %.3f on a Buggy that was not run over", lr, lc);
        }

        /* PHASE B -- HEAD-ON, WHICH IS THE REPORTED CONFIGURATION.
         *
         * The field goes back on its grid and the Buggy is put on the Hummer's
         * own path eight metres up it, facing back down it, and driven at the
         * Hummer flat out for six seconds. They meet in about a second at a
         * combined ten metres a second, which is the hardest thing these two cars
         * can do to each other, and then the Buggy stays on the throttle.
         *
         * ON THE PATH AND FACING ALONG IT, not at an angle across open ground.
         * The first version of this drove the Buggy off after phase A and it
         * covered 2.77 m before stopping dead -- and the SAME DRIVE with the
         * contact call taken out stopped at the same 2.77 m, in the same
         * centimetre. It had driven into country_3's scenery and been held there
         * by the world, which is the col_sphere burial CLAUDE.md already carries
         * as an open bug and is emphatically not this fixture's business. The
         * control run is what separated the two, and the lesson is the cheap one:
         * before believing a fixture that reports a car going nowhere, run it
         * again with the thing under test removed.
         *
         * What is checked is not that they never overlap -- two cars in contact
         * overlap. It is that the state does not PERSIST. */
        {
            int ram_hits = 0;
            float end_ov;
            ai_reset(&h);
            run_m = 0.0f;
            k = 0;
            while (k + 1 < h.car[0].n && run_m < 8.0f) {
                const ai_sample *s0 = &h.car[0].s[k], *s1 = &h.car[0].s[k + 1];
                run_m += dist3(s1->p, s0->p);
                k++;
            }
            start[0] = h.car[0].s[k].p[0];
            start[2] = h.car[0].s[k].p[2];
            if (!col_ground_at(&col, start[0], start[2], 1e9f, &gy, &nx, &ny,
                               &nz))
                gy = h.car[0].s[k].p[1];
            start[1] = gy;
            /* facing back down the path at the Hummer: sb2obj's rotY puts local
               +Z on (sin yaw, 0, cos yaw), so yaw = atan2(dx, dz). */
            rbcar_init(&pl, 1, w,  start[0], gy, start[2],
                       (float)(atan2(h.car[0].rb.body.x[0] - start[0],
                                     h.car[0].rb.body.x[2] - start[2])
                               * 57.29577951));
            worst = 0.0f;
            deep_run = deep_worst = 0.0f;
            for (i = 0; i < 60 * 6; i++) {
                float ov, st = 0.0f, fwd[3], dir[3], dl, ang, cr;
                const float *m;
                ai_step(&h, NULL, pl.body.x[0], pl.body.x[1], pl.body.x[2], 0,
                        1.0f / 60.0f);
                m = pl.m;
                fwd[0] = m[8]; fwd[2] = m[10];
                dir[0] = h.car[0].rb.body.x[0] - pl.body.x[0];
                dir[2] = h.car[0].rb.body.x[2] - pl.body.x[2];
                dl = (float)sqrt((double)dir[0] * dir[0]
                                 + (double)dir[2] * dir[2]);
                if (dl > 1e-4f) {
                    dir[0] /= dl; dir[2] /= dl;
                    ang = (float)(acos(fmax(-1.0, fmin(1.0,
                              (double)fwd[0] * dir[0]
                              + (double)fwd[2] * dir[2]))) * 57.29577951);
                    cr = fwd[2] * dir[0] - fwd[0] * dir[2];
                    if (cr < 0.0f) ang = -ang;
                    st = -ang / 30.0f;
                    if (st > 1.0f) st = 1.0f;
                    if (st < -1.0f) st = -1.0f;
                }
                rbcar_step(&pl, 1.0f, 0.0f, st, 0, 1.0f / 60.0f);
                if (ai_collide_player(&h, &pl, 1.0f / 60.0f) > 0.0f)
                    ram_hits++;
                ov = proxy_overlap(&pl, &h.car[0].rb);
                if (ov > worst) worst = ov;
                if (h.car[0].bump > hbump) hbump = h.car[0].bump;
                if (ov > 0.02f) {
                    deep_run++;
                    if (deep_run > deep_worst) deep_worst = deep_run;
                } else {
                    deep_run = 0;
                }
            }
            end_ov = proxy_overlap(&pl, &h.car[0].rb);
            ck(ram_hits > 0,
               "head-on, a Buggy at a Hummer: they meet (%d contact ticks over"
               " six seconds)", ram_hits);
            ck(worst < 0.05f,
               "  worst overlap %.3f m, against a Buggy wheel sphere of %.3f",
               worst, RB_CARS[1].tune.cdt_rad_wheel);
            /* A second of it. Two cars in contact touch for as long as they are
               in contact; what a wedge looks like is one of them INSIDE the
               other and staying there. */
            ck(deep_worst < 60.0f,
               "  never deeply inside for more than %.2f s at a stretch --"
               " shoved, not stuck", deep_worst / 60.0f);
            ck(end_ov == 0.0f,
               "  and the two are cleanly apart at the end (%.3f m of overlap)",
               end_ov);
        }
        ai_free(&h);
        col_free(&col);
        (void)j;
        (void)hit_ticks;
    }

    /* ---- 5. OPPONENT AGAINST OPPONENT, ON ALL TEN TRACKS.
     *
     * The field is solid to itself now, which is a new way for a race to go
     * wrong: three cars replaying three different laps of the same track can
     * share a corner, and if they shove each other into a state they cannot get
     * out of, every track is broken from the start line.
     *
     * So this is a survey rather than a single fixture -- a minute of racing on
     * every one of the ten, with no player anywhere near -- and it prints what it
     * measures. The bounds are the ones that separate "they touch" from "they are
     * broken": nobody leaves their limit, and nobody is still off their line at
     * the end of it.
     */
    {
        int t, bad_limit = 0, bad_end = 0, touched_tracks = 0;
        float worst_off = 0.0f, worst_ov = 0.0f;
        /* AND HOW FAR OFF THE GROUND, which is a SEPARATE question from how far
         * off the line and is the one this survey used to leave unasked. The
         * offset budget is one magnitude over all three axes, so a field that
         * spends the whole of it STRAIGHT UP satisfies every bound above --
         * which is exactly what shipped: country_1 flew two of its three cars at
         * 0.596 and 0.618 m of lift, on cars a quarter of a metre tall, and
         * "no opponent ever passes its own bump limit" reported zero violations
         * the whole time. See ai_pair_resolve on the ratchet that did it. */
        float worst_up = 0.0f, up_bound = 0.0f;
        int bad_up = 0, worst_track = -1, up_track = -1, moved_tracks = 0;

        for (t = 0; t < AI_N_RACES; t++) {
            ai_t f;
            char path[128];
            int c, d, touched = 0, moved = 0;
            float far3[3];

            snprintf(path, sizeof(path), ASSETS "/%s.col", AI_RACES[t].track);
            memset(&col, 0, sizeof(col));
            if (!col_load(path, &col))
                continue;
            memset(&f, 0, sizeof(f));
            if (ai_init(&f, t, ASSETS, col_rb_world(&col), 1, 0) < 1) {
                col_free(&col);
                continue;
            }
            far3[0] = f.car[0].rb.body.x[0] + 1000.0f;
            far3[1] = f.car[0].rb.body.x[1];
            far3[2] = f.car[0].rb.body.x[2] + 1000.0f;
            for (i = 0; i < 60 * 60; i++) {
                ai_step(&f, NULL, far3[0], far3[1], far3[2], 0, 1.0f / 60.0f);
                for (c = 0; c < f.n; c++) {
                    float sp[RB_MAX_SPHERES][4], lo = 1e30f, hi = -1e30f, tall;
                    int ns, q;

                    /* "MOVED", NOT "TOUCHED" -- and since the steering decision
                       exists those are two different things. `bump` is |off| and
                       the decision writes off, so counting it as a contact makes
                       every track report opponents touching the moment one of
                       them chooses a line round another. What touching means is
                       proxies overlapping, and that is measured below. */
                    if (f.car[c].bump > 0.0f)
                        moved = 1;
                    if (f.car[c].bump > f.car[c].bump_limit + 1e-3f)
                        bad_limit++;
                    if (f.car[c].bump > worst_off) {
                        worst_off = f.car[c].bump;
                        worst_track = t;
                    }
                    /* ONE CAR HEIGHT, off the car's own proxy rather than typed
                       in: a bump may put an opponent on top of another opponent
                       and that is the most a contact can honestly do. */
                    ns = rb_gather_spheres(&f.car[c].rb, sp);
                    for (q = 0; q < ns; q++) {
                        if (sp[q][1] - sp[q][3] < lo) lo = sp[q][1] - sp[q][3];
                        if (sp[q][1] + sp[q][3] > hi) hi = sp[q][1] + sp[q][3];
                    }
                    tall = ns > 0 ? hi - lo : 0.25f;
                    if (f.car[c].off[1] > worst_up) {
                        worst_up = f.car[c].off[1];
                        up_track = t;
                        up_bound = tall;
                    }
                    if (f.car[c].off[1] > tall)
                        bad_up++;
                }
                for (c = 0; c < f.n; c++)
                    for (d = c + 1; d < f.n; d++) {
                        float ov = proxy_overlap(&f.car[c].rb, &f.car[d].rb);
                        if (ov > 0.0f)
                            touched = 1;
                        if (ov > worst_ov)
                            worst_ov = ov;
                    }
            }
            /* Let it settle with nothing touching it, then require every car to
               be back on its own line: an offset that never returns is a field
               permanently driving somewhere it was not recorded. */
            for (i = 0; i < 60 * 3; i++)
                ai_step(&f, NULL, far3[0], far3[1], far3[2], 0, 1.0f / 60.0f);
            for (c = 0; c < f.n; c++)
                if (f.car[c].bump > 0.05f)
                    bad_end++;
            if (touched)
                touched_tracks++;
            if (moved)
                moved_tracks++;
            ai_free(&f);
            col_free(&col);
        }
        printf("       ten tracks, a minute each: %d of them have opponents"
               " whose proxies actually TOUCH and %d where one moves off its"
               " line at all, worst offset %.3f m (%s), worst overlap %.3f m\n",
               touched_tracks, moved_tracks, worst_off,
               worst_track >= 0 ? AI_RACES[worst_track].track : "-", worst_ov);
        ck(bad_limit == 0, "no opponent ever passes its own bump limit"
                           " (%d violations)", bad_limit);
        ck(bad_end == 0, "and after three quiet seconds every car on every track"
                         " is back on its line (%d not)", bad_end);
        ck(worst_ov < 0.10f,
           "and two opponents are never more than %.3f m inside each other",
           worst_ov);
        ck(bad_up == 0,
           "and none of them is ever lifted more than one car height off its own"
           " line (worst %.3f m on %s, against a %.3f m car; %d violations)",
           worst_up, up_track >= 0 ? AI_RACES[up_track].track : "-", up_bound,
           bad_up);
    }
}


/* ---------------------------------------------------------------------------
 * part 10 -- THE STEERING DECISION
 *
 * Part 9 measures what happens when an opponent is HIT. This measures what
 * happens when it can see the hit coming: it looks 2.7 m up its own recorded
 * path, and if a car is in the corridor it picks a side and goes round.
 *
 * Every check here is against a TWIN with ai_t.steer_off set -- the same track,
 * the same profile, the same tick count, the decision the only difference -- so
 * "it avoided" is a comparison and not an assertion, and so is "it changed
 * nothing when there was nothing to avoid".
 */
static void part10(void)
{
    col_t col;
    int i;

    printf("\npart 10: the steering decision\n");

    /* ---- 1. A NO-OP WITH NOTHING IN THE WAY, and BIT-IDENTICAL.
     *
     * The decision writes the same offset a shove does, so the first thing it
     * has to be is invisible when there is nothing to decide about. Not "close":
     * an unobstructed opponent must be the same car it was before this existed,
     * because every measurement in this file is taken on one. */
    {
        ai_t on, off;
        float far3[3];
        int diff = 0;

        memset(&col, 0, sizeof(col));
        if (!col_load(ASSETS "/beach_1.col", &col)) {
            ck(0, "beach_1.col loaded");
            return;
        }
        memset(&on, 0, sizeof(on));
        memset(&off, 0, sizeof(off));
        if (ai_init(&on, 0, ASSETS, col_rb_world(&col), 1, 0) < 1
            || ai_init(&off, 0, ASSETS, col_rb_world(&col), 1, 0) < 1) {
            ck(0, "beach_1 opponents loaded");
            col_free(&col);
            return;
        }
        off.steer_off = 1;
        /* ONE CAR. The property under test is "nothing in the corridor changes
           nothing", and beach_1's three opponents pass close enough to each
           other to have something in the corridor -- which is the FEATURE, and
           part 10's third case measures it. Cutting the field to one leaves a
           car with an empty world, which is the case that has to be inert. */
        on.n = off.n = 1;
        far3[0] = on.car[0].rb.body.x[0] + 1000.0f;
        far3[1] = on.car[0].rb.body.x[1];
        far3[2] = on.car[0].rb.body.x[2] + 1000.0f;
        for (i = 0; i < 60 * 30; i++) {
            int c;
            ai_step(&on,  NULL, far3[0], far3[1], far3[2], 0, 1.0f / 60.0f);
            ai_step(&off, NULL, far3[0], far3[1], far3[2], 0, 1.0f / 60.0f);
            for (c = 0; c < on.n; c++) {
                if (memcmp(on.car[c].rb.body.x, off.car[c].rb.body.x,
                           sizeof(on.car[c].rb.body.x)) != 0
                    || memcmp(on.car[c].rb.body.q, off.car[c].rb.body.q,
                              sizeof(on.car[c].rb.body.q)) != 0
                    || on.car[c].cursor != off.car[c].cursor
                    || on.car[c].u != off.car[c].u
                    || on.car[c].dist != off.car[c].dist)
                    diff++;
            }
        }
        ck(diff == 0,
           "with no player near, thirty seconds of beach_1 is BIT-IDENTICAL with"
           " the decision on and off (%d divergent car-ticks of %d)",
           diff, 60 * 30 * on.n);
        ai_free(&on);
        ai_free(&off);
        col_free(&col);
    }

    /* ---- 2. IT GOES ROUND A PARKED PLAYER, AND THE REPLAY NEVER KNOWS.
     *
     * The same fixture part 9 rams with: a Buggy parked sideways across
     * country_3's Hummer's own recorded line, six metres up the path. With the
     * decision off the Hummer drives into it -- that is part 9's whole subject.
     * With it on the Hummer should pass without touching, and the replay behind
     * it should be untouched to the BIT, exactly as a bump leaves it.
     */
    {
        ai_t h, ref;
        rb_car pl, pl2;
        const rb_world *w;
        float gy, nx, ny, nz, start[3];
        float run_m = 0.0f, worst_ov_on = 0.0f, worst_ov_off = 0.0f;
        float near_on = 1e9f, near_off = 1e9f;
        float widest = 0.0f, yaw_peak = 0.0f;
        float bang_on = 0.0f, bang_off = 0.0f;
        int k = 0, hit_on = 0, hit_off = 0, replay_diff = 0, flips = 0;
        int last_side = 0;

        memset(&col, 0, sizeof(col));
        if (!col_load(ASSETS "/country_3.col", &col)) {
            ck(0, "country_3.col loaded");
            return;
        }
        w = col_rb_world(&col);
        memset(&h, 0, sizeof(h));
        memset(&ref, 0, sizeof(ref));
        if (ai_init(&h, 6, ASSETS, w, 1, 0) < 1
            || ai_init(&ref, 6, ASSETS, w, 1, 0) < 1) {
            ck(0, "country_3 opponents loaded");
            col_free(&col);
            return;
        }
        ref.steer_off = 1;
        /* ONE OPPONENT. country_3's three start 0.74 and 1.40 m apart and their
           three recordings run nose to tail for the lap, so a three-car field
           makes this fixture mostly about the Hummer and its team-mates. What is
           under test is the pass round the PARKED PLAYER. */
        h.n = ref.n = 1;

        while (k + 1 < h.car[0].n && run_m < 6.0f) {
            const ai_sample *s0 = &h.car[0].s[k], *s1 = &h.car[0].s[k + 1];
            run_m += dist3(s1->p, s0->p);
            k++;
        }
        start[0] = h.car[0].s[k].p[0];
        start[2] = h.car[0].s[k].p[2];
        if (!col_ground_at(&col, start[0], start[2], 1e9f, &gy, &nx, &ny, &nz))
            gy = h.car[0].s[k].p[1];
        start[1] = gy;
        rbcar_init(&pl,  1, w, start[0], gy, start[2], 90.0f);
        rbcar_init(&pl2, 1, w, start[0], gy, start[2], 90.0f);

        /* TWELVE SECONDS, not six. Six is how long the Hummer takes to REACH the
           parked car -- part 9's own phase A reports one contact tick over it --
           so a six-second window measures the approach and not the pass. */
        for (i = 0; i < 60 * 12; i++) {
            float ov, wide, hb;
            int c;

            ai_step(&h,   NULL, pl.body.x[0],  pl.body.x[1],  pl.body.x[2],
                    0, 1.0f / 60.0f);
            ai_step(&ref, NULL, pl2.body.x[0], pl2.body.x[1], pl2.body.x[2],
                    0, 1.0f / 60.0f);
            hb = ai_collide_player(&h, &pl, 1.0f / 60.0f);
            if (hb > 0.0f) { hit_on++; if (hb > bang_on) bang_on = hb; }
            hb = ai_collide_player(&ref, &pl2, 1.0f / 60.0f);
            if (hb > 0.0f) { hit_off++; if (hb > bang_off) bang_off = hb; }
            rbcar_step(&pl,  0.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
            rbcar_step(&pl2, 0.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);

            ov = proxy_overlap(&pl, &h.car[0].rb);
            if (ov > worst_ov_on)
                worst_ov_on = ov;
            ov = proxy_overlap(&pl2, &ref.car[0].rb);
            if (ov > worst_ov_off)
                worst_ov_off = ov;
            /* HOW CLOSE IT ACTUALLY GOT, centre to centre -- the width of the
               pass, which "it moved" and "it yawed" do not measure at all. */
            {
                float d = dist3(h.car[0].rb.body.x, pl.body.x);
                if (d < near_on) near_on = d;
                d = dist3(ref.car[0].rb.body.x, pl2.body.x);
                if (d < near_off) near_off = d;
            }

            wide = h.car[0].steer_want < 0.0f ? -h.car[0].steer_want
                                              :  h.car[0].steer_want;
            if (wide > widest)
                widest = wide;
            /* A WEAVE IS A SIDE CHANGE WITHOUT AN INTERVENING RELEASE. Going
               left round one car and right round the next one two seconds later
               is two decisions, not a weave, so the counter has to reset through
               zero rather than remember the last non-zero side. */
            if (h.car[0].steer_side == 0)
                last_side = 0;
            else {
                if (last_side != 0 && h.car[0].steer_side != last_side)
                    flips++;
                last_side = h.car[0].steer_side;
            }
            {
                float y = h.car[0].off_yaw < 0.0f ? -h.car[0].off_yaw
                                                  :  h.car[0].off_yaw;
                if (y > yaw_peak)
                    yaw_peak = y;
            }
            /* THE REPLAY, BIT FOR BIT, against the twin that did not steer.
               This is the guarantee the whole design rests on: a decision may
               touch the offset and NOTHING else. */
            for (c = 0; c < h.n; c++)
                if (h.car[c].cursor != ref.car[c].cursor
                    || h.car[c].u != ref.car[c].u
                    || h.car[c].lap != ref.car[c].lap
                    || h.car[c].dist != ref.car[c].dist
                    || h.car[c].speed != ref.car[c].speed
                    || h.car[c].speed_rec != ref.car[c].speed_rec
                    || h.car[c].coeff != ref.car[c].coeff
                    || memcmp(h.car[c].rec_x, ref.car[c].rec_x,
                              sizeof(h.car[c].rec_x)) != 0)
                    replay_diff++;
        }

        printf("       parked Buggy on the Hummer's line, twelve seconds:"
               " overlap %.3f m and %.2f m/s of bang deciding, against %.3f m"
               " and %.2f m/s not; it moved %.3f m across and held %.1f deg\n",
               worst_ov_on, bang_on, worst_ov_off, bang_off, widest,
               yaw_peak * 57.29577951f);
        /* BOTH ENDS, because "it never touched" is also what a broken fixture
           says. The twin has to hit for the pass to mean anything. */
        ck(hit_off > 0 && worst_ov_off > 0.01f,
           "  the twin that does not steer DOES drive into the parked player"
           " (%d contact ticks, %.3f m of overlap, %.2f m/s)",
           hit_off, worst_ov_off, bang_off);
        /* IT TURNS A HIT INTO A GRAZE, and that is the honest claim rather than
         * "it never touches". Clearing a Buggy parked SIDEWAYS wants
         * reach + reach + a wheel = 0.670 m of lateral room and the Hummer's
         * whole offset budget is 0.661, so the geometry does not permit a clean
         * pass here -- the car goes as wide as it is allowed and brushes. See
         * ai.h, "the steering decision", on the budget. */
        ck(worst_ov_on < worst_ov_off * 0.5f && bang_on < bang_off,
           "  and the one that steers turns it into a graze -- %.3f m of overlap"
           " against %.3f, %.2f m/s against %.2f",
           worst_ov_on, worst_ov_off, bang_on, bang_off);
        ck(near_on > near_off,
           "  it passes WIDER than the twin does rather than steering into it"
           " -- %.3f m at the closest against %.3f", near_on, near_off);
        ck(widest > 0.05f && widest <= h.car[0].bump_limit,
           "  it moved a real distance across and stayed inside its own budget"
           " (%.3f m of %.3f)", widest, h.car[0].bump_limit);
        /* A car that slides sideways with its nose straight ahead is the tell
           that this is an offset and not a driver. */
        ck(yaw_peak > 0.02f,
           "  and it POINTS into the move rather than sliding flat"
           " (%.1f degrees of heading held)", yaw_peak * 57.29577951f);
        ck(flips == 0,
           "  it commits to one side rather than weaving (%d changes of side)",
           flips);
        ck(replay_diff == 0,
           "  and the replay behind it is BIT-IDENTICAL to the twin's --"
           " cursor, u, lap, distance, both speeds and the coefficient"
           " (%d divergent car-ticks)", replay_diff);
        ai_free(&h);
        ai_free(&ref);
        col_free(&col);
    }

    /* ---- 3. THE FIELD, ON ALL TEN, WITH AND WITHOUT.
     *
     * The headline: does deciding actually reduce contact between opponents, or
     * does it just move them about? A minute on each track, no player, twice.
     */
    {
        int t, tracks_on = 0, tracks_off = 0;
        float ov_on = 0.0f, ov_off = 0.0f;

        for (t = 0; t < AI_N_RACES; t++) {
            int pass;
            for (pass = 0; pass < 2; pass++) {
                ai_t f;
                char path[128];
                int c, d, touched = 0;
                float far3[3], worst = 0.0f;

                snprintf(path, sizeof(path), ASSETS "/%s.col",
                         AI_RACES[t].track);
                memset(&col, 0, sizeof(col));
                if (!col_load(path, &col))
                    continue;
                memset(&f, 0, sizeof(f));
                if (ai_init(&f, t, ASSETS, col_rb_world(&col), 1, 0) < 1) {
                    col_free(&col);
                    continue;
                }
                f.steer_off = pass;      /* 0 decides, 1 does not */
                far3[0] = f.car[0].rb.body.x[0] + 1000.0f;
                far3[1] = f.car[0].rb.body.x[1];
                far3[2] = f.car[0].rb.body.x[2] + 1000.0f;
                for (i = 0; i < 60 * 60; i++) {
                    ai_step(&f, NULL, far3[0], far3[1], far3[2], 0,
                            1.0f / 60.0f);
                    for (c = 0; c < f.n; c++)
                        for (d = c + 1; d < f.n; d++) {
                            float ov = proxy_overlap(&f.car[c].rb,
                                                     &f.car[d].rb);
                            if (ov > 0.0f)
                                touched = 1;
                            if (ov > worst)
                                worst = ov;
                        }
                }
                if (pass == 0) {
                    if (touched) tracks_on++;
                    if (worst > ov_on) ov_on = worst;
                } else {
                    if (touched) tracks_off++;
                    if (worst > ov_off) ov_off = worst;
                }
                ai_free(&f);
                col_free(&col);
            }
        }
        printf("       ten tracks, a minute each, no player: %d tracks touch"
               " deciding against %d not, worst overlap %.3f m against %.3f\n",
               tracks_on, tracks_off, ov_on, ov_off);
        ck(tracks_on <= tracks_off && ov_on <= ov_off,
           "the field touches itself less when it can see the other cars"
           " (%d/%d tracks, %.3f/%.3f m)", tracks_on, tracks_off, ov_on,
           ov_off);
    }
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("aitest -- the AI opponents against the real .aip files\n");
    part1();
    part2();
    part3();
    part4();
    part5();
    part6();
    part7();
    part8();
    part9();
    part10();
    printf("\n%d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
