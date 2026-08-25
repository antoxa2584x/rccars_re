/* HOW FAR OUT IS THE PLACING -- both sides of it -- against the road itself, on
 * all ten real tracks.
 *
 * placechk and vis_test part 2 ask whether the progress measures are MONOTONIC
 * and whether they MOVE. Both are necessary and neither is sufficient: a measure
 * can be perfectly monotonic, cover exactly a lap, and still be 100 m out at
 * every instant in between, which is what shipped and what "the place updates too
 * late" was. This one asks how far out it is.
 *
 * THE TRUTH IS THE RECORDED LAP. The player is driven along the first opponent's
 * recording, so where it is round the lap is known exactly: `ai_car.dist' is
 * metres walked, `ai_car.lap_len' is what a lap of that road measures, and the
 * distance at which the lap passes each checkpoint is a closest approach away. So
 * the ideal answer at any instant is simply
 *
 *     (road fraction of a lap since cp_0) * spine_len
 *
 * because the placing's metres are DEFINED that way: one lap is spine_len
 * whatever the road measures, and the checkpoints sit at their own true
 * fractions of it (checkpoints_t.station).
 *
 * IT CARRIES THE RULE IT REPLACED AS ITS OWN CONTROL, run on the same frames --
 * the chord to the next marker, laid out on the spine's arc stations. A check
 * that has never been shown to fail is of unknown value, and rather than record
 * that this one once did, it re-proves it every run.
 *
 * AND IT CHECKS THE OPPONENTS' SIDE TOO, which nothing did: their progress has to
 * advance by exactly one spine_len per lap of road driven, and it did not -- it
 * was scaled by the whole polyline rather than by the lap, so every lap after the
 * first came up short by the recording's lead-in.
 */
#include "checkpoint.h"
#include "scene.h"
#include "col.h"
#include "ai.h"
#include "tracks.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int passed, failed;
static void ck(int ok, const char *what, const char *fmt, ...)
{
    va_list ap;
    printf("  %-4s %-64s ", ok ? "ok" : "FAIL", what);
    va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
    printf("\n");
    if (ok) passed++; else failed++;
}

static int sp(void *c, float x, float y, float z, float h, float *d, int *k)
{ return cp_spine_dist_near((const checkpoints_t *)c, x, y, z, h, d, k); }
static int lp(void *c, float x, float y, float z, float *o)
{ return cp_lap_progress((const checkpoints_t *)c, x, y, z, o); }

/* THE RULE THIS REPLACED, verbatim: a straight line to the next MARKER against
   the stretch's ARC span, on the spine's own cum[] stations. Both halves of it
   were wrong and both are in this one expression, which is why it is the
   control. */
static float old_rule(const checkpoints_t *c, float x, float y, float z)
{
    float from, to, span, d, t;
    const float *p;
    if (c->last < 0) return 0.f;
    from = c->cum[c->last][0];
    to = (c->next == 0) ? c->spine_len : c->cum[c->next][0];
    span = to - from;
    if (!(span > 1e-3f)) return from;
    p = c->cp[c->next].p[0];
    d = sqrtf((p[0]-x)*(p[0]-x) + (p[1]-y)*(p[1]-y) + (p[2]-z)*(p[2]-z));
    t = 1.f - d / span;
    if (t < 0.f) t = 0.f; else if (t > 1.f) t = 1.f;
    return from + span * t;
}

typedef struct { double sum; float worst, prev; int n, back, run, stall; } acc;

/* `moved` is how far the CAR went. A measure that holds still while the car does
 * is right, not stalled, so the stall is only counted against a driving car.
 *
 * THE STALL IS A RUN of consecutive held frames, not a count of them. These are
 * float32 metres in the 400-900 range where one ULP is 3e-5 to 6e-5 m, and a car
 * creeping along a 250 m stretch moves the answer by less than that in a frame --
 * so a handful of frames a lap round to the same float however good the rule is.
 * A run is immune to it and is what the bug actually was: the old rule held the
 * answer still for 200 consecutive frames on urban_2. */
static void feed(acc *A, float v, float ideal, float moved, float spine_len)
{
    float e = v - ideal;
    if (e < 0.f) e = -e;
    A->sum += e; A->n++;
    if (e > A->worst) A->worst = e;
    if (A->prev > -1e8f) {
        float d = v - A->prev;
        if (moved > 5e-4f && d == 0.f) {
            if (++A->run > A->stall) A->stall = A->run;
        } else {
            A->run = 0;
            /* A drop of most of a lap is the START/FINISH LINE, not a step back:
               this is a WITHIN-lap measure and it is supposed to wrap. Anything
               short of half a spine is a real reversal. Both columns wrapped
               identically before this was written down, which is what said it was
               the harness and not the subject. */
            if (d < -0.5f && d > -spine_len * 0.5f) A->back++;
        }
    }
    A->prev = v;
}

int main(void)
{
    int t, done = 0, worst_stall = 0, worst_stall_old = 0;
    int n_back = 0, n_worse = 0, n_wide = 0, n_fit = 0, n_oppdrift = 0;
    double gold = 0.0, gnew = 0.0, worst_opp = 0.0;

    printf("track        spine |   OLD RULE  mean   worst stall back "
           "|   SHIPPED  mean   worst stall back | opp drift/lap\n");
    for (t = 0; t < N_TRACKS; t++) {
        char p[160]; scene_t sc; col_t col; checkpoints_t cps; ai_t ai; ai_track tr;
        int i, k, n;
        float mk[CP_MAX][3], frac[CP_MAX], lap_len = 0.f;
        float lastx, lastz, d0 = 0.f, oppdrift = 0.f;
        const ai_car *a;
        acc O, N;

        memset(&O, 0, sizeof O); memset(&N, 0, sizeof N);
        O.prev = N.prev = -1e9f;

        snprintf(p, sizeof p, "assets/%s.vsc", TRACKS[t].base);
        memset(&sc, 0, sizeof sc);
        if (!scene_load(p, &sc)) continue;
        snprintf(p, sizeof p, "assets/%s.col", TRACKS[t].base);
        memset(&col, 0, sizeof col);
        col_load(p, &col);
        cp_init(&cps, &sc, &col);
        /* cp_init only enables the progression when the ARROW textures loaded and
           a host stub has none, so cp_step would latch nothing at all. */
        cps.enabled = 1;
        memset(&ai, 0, sizeof ai);
        if (!ai_init(&ai, t, "assets", col_rb_world(&col), 1, 0)) {
            scene_release(&sc); col_free(&col); continue;
        }
        a = &ai.car[0];

        /* AS THE APP DOES, and in the app's order: fit the stations off the
           recordings, install them, and only then restart -- cp_restart reads
           road_len to place the grid. */
        n = cps.n < CP_MAX ? cps.n : CP_MAX;
        for (i = 0; i < n; i++) {
            mk[i][0] = cps.cp[i].p[0][0];
            mk[i][1] = cps.cp[i].p[0][1];
            mk[i][2] = cps.cp[i].p[0][2];
        }
        if (ai_cp_fractions(&ai, (const float (*)[3])mk, n, frac, &lap_len)
            && cp_set_stations(&cps, frac, n, lap_len))
            n_fit++;

        /* WHERE cp_0 FALLS ON THIS CAR'S OWN RECORDING, as a distance from
           sample 0. The fit gives its fraction of the LOOP; the lead-in puts that
           on the same ruler as `dist`, and a lap comes off because the loop
           reaches cp_0 near its END while the car meets it near the start. */
        d0 = a->lead_in + (frac[0] - 1.f) * a->lap_len;

        cp_restart(&cps, a->rec_x[0], a->rec_x[1], a->rec_x[2]);
        lastx = a->rec_x[0]; lastz = a->rec_x[2];
        memset(&tr, 0, sizeof tr);
        tr.ctx = &cps; tr.spine = sp; tr.lap_progress = lp;
        tr.spine_len = cps.spine_len;

        for (i = 0; i < 30000; i++) {
            const float *x = a->rec_x;      /* the recorded pose, unbumped */
            float F, ideal, moved; int kk;
            cp_step(&cps, x[0], x[1], x[2], 1.f/60.f);
            ai_step(&ai, &tr, x[0], x[1], x[2], cps.lap, 1.f/60.f);
            moved = sqrtf((x[0]-lastx)*(x[0]-lastx) + (x[2]-lastz)*(x[2]-lastz));
            lastx = x[0]; lastz = x[2];
            if (cps.last < 0) continue;                 /* the opening stretch */

            /* THE TRUTH: the fraction of a lap since cp_0, straight off the
               recording's own odometer. */
            F = (float)fmod((a->dist - d0) / a->lap_len, 1.0);
            if (F < 0.f) F += 1.f;
            ideal = F * cps.spine_len;

            /* which stretch that really is, by the fitted stations */
            kk = n - 1;
            for (k = 0; k < n; k++) {
                float hi = (k+1 < n) ? frac[k+1] : 1.f;
                if (F >= frac[k] && F < hi) { kk = k; break; }
            }
            /* Where the latch and the road disagree about which stretch the car
               is on there is no truth to compare against -- skipped rather than
               scored, because scoring it would measure the CURSOR. */
            if (kk != cps.last) continue;

            feed(&O, old_rule(&cps, x[0], x[1], x[2]), ideal, moved,
                 cps.spine_len);
            feed(&N, cps.prog, ideal, moved, cps.spine_len);
        }

        /* THE OPPONENTS' SIDE: their progress must advance by exactly one
           spine_len for every lap of road driven. Measured over the whole run,
           which is several laps, so a per-lap error shows up multiplied. */
        {
            double laps = a->dist / a->lap_len;
            if (laps > 0.5)
                oppdrift = (float)((a->spine_dist / cps.spine_len - laps)
                                   / laps * cps.spine_len);
        }

        if (N.n > 0) {
            double mo = O.sum / O.n, mn = N.sum / N.n;
            printf("%-11s %5.0f | %9.1fm %6.1fm %5d %5d "
                   "| %9.1fm %6.1fm %5d %5d | %8.2fm\n",
                   TRACKS[t].base, cps.spine_len,
                   mo, O.worst, O.stall, O.back,
                   mn, N.worst, N.stall, N.back, oppdrift);
            gold += mo; gnew += mn; done++;
            if (N.stall > worst_stall) worst_stall = N.stall;
            if (O.stall > worst_stall_old) worst_stall_old = O.stall;
            if (N.back) n_back++;
            if (mn >= mo) n_worse++;
            if (mn > cps.spine_len * 0.05) n_wide++;
            if (fabs(oppdrift) > 1.0) n_oppdrift++;
            if (fabs(oppdrift) > fabs(worst_opp)) worst_opp = oppdrift;
        }
        ai_free(&ai); scene_release(&sc); col_free(&col);
    }

    printf("\n");
    ck(done == 10, "all ten tracks drive the placing", "%d of 10", done);
    ck(n_fit == 10,
       "every track's checkpoint stations fit off its own recordings",
       "%d of 10", n_fit);
    /* THE OPPONENTS' SIDE. A lap of road has to be a lap of the placing; scaling
       by the whole polyline instead of by the lap made it 8 to 17 m short, every
       lap, compounding. One metre a lap is the bound. */
    ck(n_oppdrift == 0,
       "an opponent covers exactly one spine length per lap of road",
       "%d track(s) drift; worst %.2f m/lap", n_oppdrift, worst_opp);
    /* THE PLAYER'S SIDE. The first two are the reported bug: the old rule stalled
       and it stepped back, and either alone is a place that updates too late.
    
       THE OLD RULE DOES NOT TRIP THE STALL CHECK and the column says so, rather
       than claiming a control that is not there: on these frames its distance to
       the next marker changes every frame too. Its failure here is ACCURACY. The
       stalls it really showed -- 280 frames pinned at 0 before the first crossing,
       200 more mid-stretch on urban_2 -- were the pre-start branch and the
       crossing step, which are not on these frames and which placechk and
       vis_test part 2 cover. This is a guard against a regression, not evidence
       about the past one. */
    ck(worst_stall < 6,
       "the player's answer moves with the car, every frame",
       "worst run %d frames, against the old rule's %d",
       worst_stall, worst_stall_old);
    ck(n_back == 0,
       "and never steps backwards while the car drives forwards",
       "%d track(s)", n_back);
    /* 5% of a lap. The shipped pipeline measures 0.3% to 4.6% and the old one
       3.9% to 14.0%, so the bound is close under the subject rather than
       generous -- and it separates the two on eight of the ten. beach_4 and
       urban_2 are the pair it does not separate, which is what the control below
       is for; between them nothing here passes for the wrong reason. */
    ck(n_wide == 0,
       "and is within 5% of a lap of the road on every track",
       "%d track(s) wider", n_wide);
    /* THE CONTROL, re-proved every run rather than recorded once. */
    ck(n_worse == 0,
       "and beats the rule it replaced on every one -- the control, so this "
       "check is never vacuous",
       "%d no better; means %.1f m against %.1f m",
       n_worse, gnew / (done ? done : 1), gold / (done ? done : 1));

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
/* glBlendFunc, glColor4f, glDepthMask and glDepthFunc all come from
   glstub_host.c -- one definition site, or the link sees them twice. */
void glDrawArrays(unsigned m, int f, int c) { (void)m; (void)f; (void)c; }
