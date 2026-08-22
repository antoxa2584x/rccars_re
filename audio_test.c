/*
 * audio_test.c -- host harness for mix.c, sfx.c and audio.c's decoder.
 *
 *   gcc -I../rccars_vita -O2 audio_test.c ../rccars_vita/mix.c \
 *       ../rccars_vita/audio.c ../rccars_vita/sfx.c ../rccars_vita/col.c \
 *       ../rccars_vita/rb.c ../rccars_vita/contact.c ../rccars_vita/collide.c \
 *       -lm -o audio_test
 *
 * Two parts, deliberately different in kind:
 *
 *   PART A  a SYNTHETIC bank with known PCM, so the mixer's arithmetic can be
 *           asserted exactly -- a DC sample of a known value through a known
 *           gain has one right answer.
 *   PART B  the REAL assets/sound.sbk and assets/music, so the things a
 *           synthetic fixture cannot catch are covered: that every name sfx.c
 *           asks for is actually in the bank, that all three motor families are
 *           complete, and that a real MP3 decodes.
 *
 * PART B FAILS if the assets are missing rather than skipping. rb_test spent a
 * long time printing "not VSC4 -- rig checks SKIPPED" inside a wall of passing
 * output while a real bug sat in the rig; a harness has to be able to tell you
 * it did nothing.
 */

#include "mix.h"
#include "audio.h"
#include "sfx.h"
#include "col.h"
#include "rb.h"
/* For PROP_MODELS -- part C5 checks that the sound each prop names in stone.sb
   is really in the packed bank, which is the one place that can go wrong
   silently. Header-only, so no prop.c on the link line. */
#include "prop_data.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails = 0, checks = 0;

static void ck(int cond, const char *fmt, ...)
{
    va_list ap;
    checks++;
    if (!cond) fails++;
    printf("  [%s] ", cond ? "ok  " : "FAIL");
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
}

static int near(float a, float b, float tol) { return fabsf(a - b) <= tol; }

/* ------------------------------------------------------- synthetic bank --- */

#define BANK_PATH "/tmp/rccars_audio_test.sbk"
#define SBK_RATE 22050

typedef struct { const char *name; int nsamp; int volume; short fill; } fixt;

/* Names are lowercase and SORTED, exactly as pack_snd.py writes them. */
static const fixt FIX[] = {
    { "dc_half",   1000, 100, 8000 },   /* constant, for exact gain arithmetic */
    { "dc_quiet",  1000,  50, 8000 },   /* same PCM, half the snd.dat volume */
    { "ramp",         4, 100,    0 },   /* 4 samples, for loop wraparound */
    { "short",       10, 100, 4000 },
};
#define NFIX ((int)(sizeof(FIX) / sizeof(FIX[0])))

static void write_bank(void)
{
    FILE *f = fopen(BANK_PATH, "wb");
    unsigned hdr[4] = { 0, SBK_RATE, NFIX, 0 };
    unsigned off, i, k;
    char nm[SBK_NAME];

    memcpy(hdr, "SBK1", 4);
    fwrite(hdr, 4, 4, f);
    off = 16 + NFIX * (SBK_NAME + 12);
    for (i = 0; i < NFIX; i++) {
        unsigned rec[3];
        memset(nm, 0, sizeof(nm));
        snprintf(nm, SBK_NAME, "%s", FIX[i].name);
        fwrite(nm, 1, SBK_NAME, f);
        rec[0] = off; rec[1] = FIX[i].nsamp; rec[2] = FIX[i].volume;
        fwrite(rec, 4, 3, f);
        off += FIX[i].nsamp * 2;
    }
    for (i = 0; i < NFIX; i++)
        for (k = 0; k < (unsigned)FIX[i].nsamp; k++) {
            /* `ramp` counts, so a wraparound is visible in the value itself. */
            short s = FIX[i].fill ? FIX[i].fill : (short)(1000 * (k + 1));
            fwrite(&s, 2, 1, f);
        }
    fclose(f);
}

/* ------------------------------------------------------------- measuring -- */

#define BLK 512
static short buf[BLK * 2];

static void render(mix_t *m, unsigned frames) { mix_render(m, buf, frames); }

static float peak_ch(int ch, unsigned frames)
{
    unsigned i;
    float p = 0.f;
    for (i = 0; i < frames; i++) {
        float v = fabsf((float)buf[i * 2 + ch]);
        if (v > p) p = v;
    }
    return p;
}

/* Is a voice playing this bank entry right now?
 *
 * Measuring total loudness turned out to be nearly useless for the sfx layer:
 * "a parked car still idles" passed with the idle layer deleted, because the
 * track's ambient bed was making the noise. Naming the voice is the only way to
 * assert WHICH layer is up. */
static int voice_playing(mix_t *m, const char *name)
{
    int i, snd = mix_find(m, name);
    if (snd < 0) return 0;
    for (i = 0; i < MIX_VOICES; i++)
        if (m->v[i].active && m->v[i].snd == snd) return 1;
    return 0;
}

static float rms_ch(int ch, unsigned frames)
{
    unsigned i;
    double s = 0.0;
    for (i = 0; i < frames; i++) {
        double v = buf[i * 2 + ch];
        s += v * v;
    }
    return (float)sqrt(s / (frames ? frames : 1));
}

/* ============================================================ PART A ====== */

static void part_a_bank(mix_t *m)
{
    int i;
    printf("\n-- A1 bank index --\n");
    ck(m->n_ents == NFIX, "%u entries", m->n_ents);
    ck(m->rate == SBK_RATE, "bank rate %u", m->rate);

    i = mix_find(m, "dc_half");
    ck(i >= 0, "mix_find finds dc_half (%d)", i);
    ck(mix_find(m, "DC_HALF") == i,
       "mix_find is case-insensitive -- snd.dat says ArrowFocus, the file is arrowfocus");
    ck(mix_find(m, "nope") < 0, "mix_find returns -1 for an absent name");
    ck(m->ents[i].volume == 100 &&
       m->ents[mix_find(m, "dc_quiet")].volume == 50,
       "snd.dat volumes survive the pack");

    printf("\n-- A2 paging --\n");
    ck(mix_resident(m) == 0, "nothing resident before a load");
    mix_load(m, i);
    ck(mix_resident(m) == 1000 * 2, "one load pages in %u bytes", mix_resident(m));
    mix_load(m, i);
    mix_unload(m, i);
    mix_trim(m);
    ck(mix_resident(m) == 1000 * 2,
       "refcounted: two loads and one unload leave it resident");
    mix_unload(m, i);
    mix_trim(m);
    ck(mix_resident(m) == 0, "the last unload frees it");

    /* A voice reading PCM that trim is about to free must be silenced, or the
       mixer walks freed memory on the very next block. */
    mix_load(m, i);
    {
        mix_voice v = mix_play(m, i, 1.f, 1.f, 1, 0);
        ck(mix_alive(m, v), "a voice starts on a paged-in sound");
        mix_unload(m, i);
        mix_trim(m);
        ck(!mix_alive(m, v), "mix_trim kills a voice whose PCM it frees");
    }
    ck(mix_play(m, i, 1.f, 1.f, 0, 0).slot < 0,
       "playing a sound that is not paged in is silent, not a crash");
}

static void part_a_voices(mix_t *m)
{
    int dc = mix_find(m, "dc_half");
    int sh = mix_find(m, "short");
    mix_voice a, b;
    int i, n;

    printf("\n-- A3 voices --\n");
    mix_load(m, dc);
    mix_load(m, sh);
    mix_stop_all(m);

    a = mix_play(m, dc, 1.f, 1.f, 1, 0);
    ck(a.slot >= 0 && mix_alive(m, a), "play returns a live handle");
    mix_stop(m, a);
    ck(!mix_alive(m, a), "a stopped handle is dead");
    b = mix_play(m, dc, 1.f, 1.f, 1, 0);
    ck(b.slot == a.slot && !mix_alive(m, a) && mix_alive(m, b),
       "the generation counter makes the recycled slot's old handle stale");
    mix_stop_all(m);

    /* Stealing. The contract is NOT "a full pool refuses new sounds" -- that
       would mean 24 stray one-shots permanently lock out the engine. It is:
       equal priority may steal, lower priority may not. */
    for (i = 0, n = 0; i < MIX_VOICES; i++)
        if (mix_play(m, dc, 1.f, 1.f, 0, 0).slot >= 0) n++;
    ck(n == MIX_VOICES, "all %d voices allocate", MIX_VOICES);
    ck(mix_play(m, dc, 1.f, 1.f, 0, 0).slot >= 0,
       "an equal-priority sound still starts on a full pool");
    ck(mix_play(m, dc, 1.f, 1.f, 1, 9).slot >= 0,
       "and a higher-priority one certainly does");

    /* Now fill it with LOOPS at engine priority and try to steal with a
       one-shot at sfx.c's PRIO_ONESHOT. This is the check that matters: the
       engine note must not drop out because a wheel hit something. */
    mix_stop_all(m);
    for (i = 0; i < MIX_VOICES; i++)
        mix_play(m, dc, 1.f, 1.f, 1, 4);
    ck(mix_play(m, dc, 1.f, 1.f, 0, 0).slot < 0,
       "a one-shot CANNOT steal a higher-priority loop -- the engine layers "
       "outrank every impact");
    mix_stop_all(m);
}

static void part_a_render(mix_t *m)
{
    int dc = mix_find(m, "dc_half");
    int dq = mix_find(m, "dc_quiet");
    int rp = mix_find(m, "ramp");
    int sh = mix_find(m, "short");
    float p_full, p_half;

    printf("\n-- A4 rendering --\n");
    mix_load(m, dq);
    mix_load(m, rp);
    mix_master(m, 1.f, 1.f);

    /* exact gain arithmetic: 8000 * gain * volume% * master */
    mix_stop_all(m);
    mix_play(m, dc, 1.f, 1.f, 1, 0);
    render(m, 64);
    p_full = peak_ch(0, 64);
    ck(near(p_full, 8000.f, 2.f), "gain 1.0, volume 100 -> %.0f (want 8000)", p_full);

    mix_stop_all(m);
    mix_play(m, dq, 1.f, 1.f, 1, 0);
    render(m, 64);
    p_half = peak_ch(0, 64);
    ck(near(p_half, 4000.f, 2.f),
       "the same PCM at snd.dat volume 50 -> %.0f (want 4000)", p_half);

    mix_stop_all(m);
    mix_play(m, dc, 0.25f, 1.f, 1, 0);
    render(m, 64);
    ck(near(peak_ch(0, 64), 2000.f, 2.f), "caller gain 0.25 -> %.0f", peak_ch(0, 64));

    mix_stop_all(m);
    mix_master(m, 0.5f, 1.f);
    mix_play(m, dc, 1.f, 1.f, 1, 0);
    render(m, 64);
    ck(near(peak_ch(0, 64), 4000.f, 2.f), "master sfx 0.5 -> %.0f", peak_ch(0, 64));
    mix_master(m, 1.f, 1.f);

    /* A looping voice must still be producing sound long after its own length.
       The bank rate is half the mixer rate, so 4 source samples last 8 frames. */
    /* A looping voice must still be producing sound long after its own length.
       Measuring rms over the WHOLE block is not enough: 8 frames of full-scale
       burst inside 256 frames of silence still clears any sane threshold, so a
       loop that ended immediately passed this. Assert the voice is alive and
       that the TAIL -- past any possible first pass -- is still sounding. */
    mix_stop_all(m);
    {
        mix_voice v = mix_play(m, rp, 1.f, 1.f, 1, 0);
        render(m, 256);                  /* 4 source samples = 8 frames */
        ck(mix_alive(m, v), "a looping voice is still alive 256 frames on");
        render(m, 256);
        ck(rms_ch(0, 256) > 500.f,
           "and still sounding in the tail (rms %.0f) -- it wrapped, it did not "
           "just play once", rms_ch(0, 256));
    }

    /* ... and a one-shot must NOT be. `short` is 10 source samples = 20 frames. */
    mix_stop_all(m);
    {
        mix_voice v = mix_play(m, sh, 1.f, 1.f, 0, 0);
        render(m, 64);
        ck(!mix_alive(m, v), "a one-shot ends when it runs out of samples");
        render(m, 64);
        ck(peak_ch(0, 64) == 0.f, "and renders silence after that");
    }

    /* pitch consumes source faster: at 2.0 the same one-shot ends in half the
       frames, which is the only observable the step actually has */
    mix_stop_all(m);
    {
        mix_voice v = mix_play(m, sh, 1.f, 2.f, 0, 0);
        render(m, 8);
        ck(mix_alive(m, v), "pitch 2.0: alive after 8 frames");
        render(m, 8);
        ck(!mix_alive(m, v), "pitch 2.0: 10 samples are gone within 16 frames");
    }
    mix_stop_all(m);
}

static void part_a_pan(mix_t *m)
{
    mix_v v;
    float l, r, g_at_min, g_mid, g_at_max;

    printf("\n-- A5 distance and pan --\n");
    memset(&v, 0, sizeof(v));
    v.snd = -1;                       /* voice_targets is not involved here */
    v.gain = 1.f;
    v.positional = 1;
    v.rmin = 1.f;
    v.rmax = 10.f;

    mix_listener(m, 0.f, 0.f, 0.f, 0.f);   /* looking down +Z */

    /* distance, measured straight ahead so the pan is symmetric */
    v.x = 0.f; v.y = 0.f; v.z = 1.f;
    mix_pan(m, &v, &l, &r);
    g_at_min = 0.5f * (l + r);
    ck(near(l, r, 1e-4f), "a source dead ahead is centred (l %.3f r %.3f)", l, r);
    ck(near(g_at_min, 1.f, 1e-3f), "full gain at rmin (%.3f)", g_at_min);

    v.z = 5.f;
    mix_pan(m, &v, &l, &r);
    g_mid = 0.5f * (l + r);

    v.z = 10.f;
    mix_pan(m, &v, &l, &r);
    g_at_max = 0.5f * (l + r);
    ck(g_at_max == 0.f, "silent AT rmax (%.4f)", g_at_max);
    ck(g_mid < g_at_min && g_mid > g_at_max,
       "and monotone between: %.3f -> %.3f -> %.3f", g_at_min, g_mid, g_at_max);

    v.z = 20.f;
    mix_pan(m, &v, &l, &r);
    ck(l == 0.f && r == 0.f, "silent beyond rmax");

    /* The rolloff has to ARRIVE at zero, not stop somewhere and get cut off by
       the rmax branch. Without the normalisation the curve is still at 1/10 of
       full gain when it hits the wall, which is an audible click as a source
       goes out of range -- and every check above passes anyway, because they
       all sample at or past rmax. */
    v.z = 9.9f;
    mix_pan(m, &v, &l, &r);
    ck(0.5f * (l + r) < 0.02f,
       "and approaches it continuously: %.4f just inside rmax, not a cliff",
       0.5f * (l + r));

    /* pan. The listener faces +Z at yaw 0, and right is (cos yaw, 0, -sin yaw),
       so +X is the RIGHT ear. Getting this backwards swaps the whole world and
       is invisible in any mono measurement. */
    v.z = 0.f; v.x = 1.f;
    mix_pan(m, &v, &l, &r);
    ck(r > l, "a source at +X is louder on the RIGHT (l %.3f r %.3f)", l, r);
    ck(l > 0.15f && near(r, 1.f, 1e-3f),
       "a hard pan attenuates the FAR ear without silencing it, and leaves the "
       "near one alone (l %.3f r %.3f)", l, r);
    v.x = -1.f;
    mix_pan(m, &v, &l, &r);
    ck(l > r, "a source at -X is louder on the LEFT (l %.3f r %.3f)", l, r);

    /* turn the listener 90 degrees: what was to the right is now dead ahead */
    mix_listener(m, 0.f, 0.f, 0.f, 90.f);
    v.x = 1.f; v.z = 0.f;
    mix_pan(m, &v, &l, &r);
    ck(near(l, r, 1e-3f),
       "yaw 90 puts the +X source ahead, so it re-centres (l %.3f r %.3f)", l, r);
    mix_listener(m, 0.f, 0.f, 0.f, 0.f);

    /* a non-positional voice ignores all of it */
    v.positional = 0;
    v.x = 1000.f;
    mix_pan(m, &v, &l, &r);
    ck(l == 1.f && r == 1.f, "a non-positional voice ignores distance");
    v.positional = 1;
    v.rmax = 0.f;
    mix_pan(m, &v, &l, &r);
    ck(l == 1.f && r == 1.f,
       "rmax 0 means global -- that is what the tracks' ground_noise_snd carries");
}

static void part_a_ramp(mix_t *m)
{
    int dc = mix_find(m, "dc_half");
    mix_voice v;
    float first, last;

    printf("\n-- A6 gain ramp --\n");
    mix_stop_all(m);
    mix_master(m, 1.f, 1.f);
    v = mix_play(m, dc, 0.f, 1.f, 1, 0);
    render(m, 64);                       /* settle at zero */

    mix_set(m, v, 1.f, 1.f);
    render(m, 64);
    first = fabsf((float)buf[0]);
    last = fabsf((float)buf[63 * 2]);
    ck(first < 0.25f * last,
       "gain is ramped across the block, not stepped at its edge "
       "(first %.0f, last %.0f) -- a step is a click at the frame rate",
       first, last);
    ck(near(last, 8000.f, 400.f), "and reaches the target by the block end (%.0f)", last);
    mix_stop_all(m);
}

static void part_a_music(mix_t *m)
{
    short pcm[256 * 2];
    int i;
    unsigned space0;

    printf("\n-- A7 music ring --\n");
    mix_music_reset(m);
    mix_music_gain(m, 1.f);
    mix_master(m, 1.f, 1.f);
    space0 = mix_music_space(m);
    ck(space0 > 0, "the ring reports space when empty (%u frames)", space0);

    for (i = 0; i < 256; i++) { pcm[i * 2] = 6000; pcm[i * 2 + 1] = -6000; }
    mix_music_write(m, pcm, 256);
    ck(mix_music_space(m) == space0 - 256, "writing 256 frames consumes 256 of space");

    mix_stop_all(m);
    render(m, 128);
    ck(near(peak_ch(0, 128), 6000.f, 2.f), "music reaches the left  (%.0f)", peak_ch(0, 128));
    ck(near(peak_ch(1, 128), 6000.f, 2.f), "music reaches the right (%.0f)", peak_ch(1, 128));

    /* the remaining 128 frames, then underrun */
    render(m, 128);
    ck(near(peak_ch(0, 128), 6000.f, 2.f), "the rest of the ring drains");
    render(m, 128);
    ck(peak_ch(0, 128) == 0.f,
       "an underrun renders SILENCE, not a repeat of the last block");
    /* The audible symptom above depends on what happens to be sitting in the
       ring past the write cursor, which in a fresh ring is zeroes -- so a mixer
       that read straight past `wr` still measured silent. The invariant is what
       actually matters: the read cursor must never overtake the write cursor,
       or the next real write is heard behind stale audio for a whole lap. */
    ck(m->music.rd <= m->music.wr,
       "and the read cursor never overtakes the write cursor (rd %u, wr %u)",
       m->music.rd, m->music.wr);

    mix_music_write(m, pcm, 256);
    mix_master(m, 1.f, 0.25f);
    render(m, 128);
    ck(near(peak_ch(0, 128), 1500.f, 4.f),
       "the music master scales it independently of sfx (%.0f)", peak_ch(0, 128));
    mix_master(m, 1.f, 1.f);
    mix_music_reset(m);
}

/* ============================================================ PART B ====== */

/* Every name sfx.c can ask the bank for. If one of these is missing the game
   plays silence at that moment and nothing else complains, which is exactly the
   class of bug a name table invites. */
static const char *const REQUIRED[] = {
    "motor_idle", "motor_engine", "motor_accel1", "motor_decel1",
    "motor_reverse", "motor_engine_ws", "motor_off",
    "motor_b_idle", "motor_b_engine", "motor_b_accel1", "motor_b_decel1",
    "motor_b_reverse", "motor_b_engine_ws", "motor_b_off",
    "motor_h_idle", "motor_h_engine", "motor_h_accel1", "motor_h_decel1",
    "motor_h_reverse", "motor_h_engine_ws", "motor_h_off",
    "car_surf_asphalt", "car_surf_sand", "car_surf_wetsand", "car_surf_gravel",
    "car_surf_stone", "car_surf_grass", "car_surf_metal", "car_surf_wood",
    "car_surf_water",
    "car_hit_asphalt", "car_hit_sand", "car_hit_gravel", "car_hit_stone",
    "car_hit_metal", "car_hit_wood", "car_hit_water",
    "arrowfocus", "arrowpress", "buttonpress", "buttonfocus", "msgboxerror",
    "car_landing", "water_splash", "cp", "cp_wrongway", "prestart", "cp_reset",
    "car_break", "car_cdt_obj",
    "noise1", "noise2", "noise3", "noise4",
};
#define N_REQUIRED ((int)(sizeof(REQUIRED) / sizeof(REQUIRED[0])))

static const char *ASSETS = "../rccars_vita/assets";

static void part_b(void)
{
    char bank[256], mus[256];
    mix_t *m;
    int i, missing = 0, rc;

    printf("\n-- B1 the real bank --\n");
    snprintf(bank, sizeof(bank), "%s/sound.sbk", ASSETS);
    snprintf(mus, sizeof(mus), "%s/music", ASSETS);

    rc = audio_init(bank, mus);
    ck(rc == 0, "audio_init on the real assets (rc %d)", rc);
    if (rc != 0) {
        printf("  ** assets/sound.sbk is missing. Run:\n"
               "     python3 pack_snd.py --out %s\n", ASSETS);
        return;                       /* the ck above already FAILED */
    }
    m = audio_mix();
    ck(m->n_ents > 100, "%u sounds in the bank", m->n_ents);
    ck(m->rate == 22050 || m->rate == 44100, "bank rate %u", m->rate);

    for (i = 0; i < N_REQUIRED; i++)
        if (mix_find(m, REQUIRED[i]) < 0) {
            printf("       missing: %s\n", REQUIRED[i]);
            missing++;
        }
    ck(missing == 0, "all %d names sfx.c asks for are in the bank", N_REQUIRED);

    /* The three motor families must be the same shape, or one car ends up with
       layers the others have and no error anywhere. */
    {
        static const char *const suf[] = { "idle", "engine", "accel1", "decel1",
                                           "reverse", "engine_ws", "off" };
        static const char *const pre[] = { "motor_", "motor_b_", "motor_h_" };
        int c, s, complete = 1;
        char nm[64];
        for (c = 0; c < 3; c++)
            for (s = 0; s < 7; s++) {
                snprintf(nm, sizeof(nm), "%s%s", pre[c], suf[s]);
                if (mix_find(m, nm) < 0) complete = 0;
            }
        ck(complete, "all three cars have a complete 7-sound motor family");
    }

    printf("\n-- B2 the real playlist --\n");
    ck(audio_music_count(AUDIO_MUSIC_RACE) > 0,
       "%d cycling tracks (Autoexec.gm's `cycle 1`)",
       audio_music_count(AUDIO_MUSIC_RACE));
    ck(audio_music_count(AUDIO_MUSIC_MENU) > 0,
       "%d menu tracks (`cycle 0`)", audio_music_count(AUDIO_MUSIC_MENU));

    /* Decode a real MP3 and require actual audio out of it. A decoder that
       returns frames of silence would satisfy any structural check. */
    {
        unsigned before, wrote = 0, i2;
        long long energy = 0;
        audio_music_group(AUDIO_MUSIC_RACE);
        before = mix_music_space(m);
        for (i2 = 0; i2 < 40; i2++) audio_pump_host();
        wrote = before - mix_music_space(m);
        ck(wrote > 0, "the MP3 decoder filled %u frames of the ring", wrote);

        mix_stop_all(m);
        mix_master(m, 1.f, 1.f);
        mix_music_gain(m, 1.f);
        /* Drain the WHOLE ring, not the first block. These tracks open on a
           near-silent lead-in -- the first 512 frames are 12 ms of it -- and a
           check that samples only the start reports a working decoder as
           silent. (It did, before this loop existed.) */
        {
            unsigned blocks = 0;
            long long best = 0;
            while (mix_music_space(m) < before && blocks < 200) {
                long long e = 0;
                render(m, BLK);
                for (i2 = 0; i2 < BLK * 2; i2++) e += (long long)buf[i2] * buf[i2];
                if (e > best) best = e;
                blocks++;
            }
            energy = best;
        }
        ck(energy > (long long)BLK * 2 * 100 * 100,
           "and it is real audio, not silence (peak block rms %.0f)",
           sqrt((double)energy / (BLK * 2)));
        ck(audio_music_title()[0] != 0, "playing '%s'", audio_music_title());
    }
}

/* ============================================================ PART C ====== */
/* sfx.c against a fake car. The point is the LAYER LOGIC -- which loop is up
   and how loud -- not the sample values, which part A already pins. */

static rb_car CAR;
static col_t COL;

/* A one-cell grid holding one quad of a KNOWN material, distinct from the
   default. Without a real surf array col_material_at returns the default before
   it ever reads one, so "the material comes from the grid" was passing against
   code that ignored the grid entirely. */
static float COL_TRIS[2 * 9] = {
    -5.f, 0.f, -5.f,   5.f, 0.f, -5.f,   5.f, 0.f, 5.f,
    -5.f, 0.f, -5.f,   5.f, 0.f,  5.f,  -5.f, 0.f, 5.f,
};
static unsigned COL_START[2] = { 0, 2 };
static unsigned COL_IDX[2] = { 0, 1 };
static unsigned char COL_SURF[2] = { SURF_WOOD, SURF_WOOD };

static void fake_col(void)
{
    memset(&COL, 0, sizeof(COL));
    COL.minx = -5.f; COL.minz = -5.f; COL.cell = 10.f;
    COL.nx = 1; COL.nz = 1; COL.ntris = 2;
    COL.tris = COL_TRIS;
    COL.start = COL_START;
    COL.idx = COL_IDX;
    COL.surf = COL_SURF;
    COL.default_surf = SURF_SAND;      /* deliberately NOT the packed material */
}

static void fake_car(float speed, float throttle, int contacts, int gear)
{
    int i;
    memset(&CAR, 0, sizeof(CAR));
    CAR.nwheels = 4;
    CAR.car_index = 0;
    CAR.body.v[2] = speed;             /* travelling along world +Z */
    /* identity pose: body +Z is world +Z, so fwd_speed == speed */
    CAR.m[0] = CAR.m[5] = CAR.m[10] = CAR.m[15] = 1.f;
    CAR.in.throttle = throttle;
    CAR.gear = gear;
    CAR.tune.speed_base_max = 27.f;    /* the Overkill's, km/h */
    for (i = 0; i < 4; i++) CAR.tune.resonator_speed[i] = 1.f;
    for (i = 0; i < contacts && i < 4; i++) CAR.hit[i].active = 1;
    for (i = 0; i < 4; i++) CAR.wheel[i].radius = 0.07f;
}

/* Total energy the mixer produces for the current state, as a stand-in for
   "how loud is the car". */
static float loudness(int frames)
{
    mix_render(audio_mix(), buf, frames);
    return rms_ch(0, frames) + rms_ch(1, frames);
}

static void part_c(void)
{
    float eye[3] = { 0.f, 0.f, -3.f };
    float idle_loud, fast_loud, air_loud, ground_loud;

    printf("\n-- C1 the engine layers --\n");
    if (sfx_init() != 0) {
        ck(0, "sfx_init");
        return;
    }
    /* Part B left a decoded track in the music ring, and sfx_pause deliberately
       does NOT stop the music. Silence it so the measurements below are of the
       thing they name -- otherwise C4's "the menu stops the loops" reads the
       soundtrack and fails on working code. */
    mix_music_gain(audio_mix(), 0.f);
    sfx_set_car(0);
    fake_col();
    sfx_set_track(0, &COL);

    /* At rest, no throttle: the car still idles. Asserted by NAME -- the first
       version of this measured total loudness and passed with the idle layer
       deleted, because it was hearing the track's ambient bed. */
    fake_car(0.f, 0.f, 4, 0);
    sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
    idle_loud = loudness(BLK);
    ck(voice_playing(audio_mix(), "motor_idle"),
       "a parked car runs the idle loop");
    ck(idle_loud > 1.f, "and makes a sound (rms %.0f)", idle_loud);

    /* The ambient bed the .sb names for this track. beach_1 says noise1. */
    ck(voice_playing(audio_mix(), "noise1"),
       "the track's own ambient bed is running (beach_1 -> noise1)");

    /* Flat out: the running loop takes over from idle. */
    fake_car(7.0f, 1.f, 4, 0);
    sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
    fast_loud = loudness(BLK);
    ck(voice_playing(audio_mix(), "motor_engine"),
       "flat out runs the engine loop");
    ck(!voice_playing(audio_mix(), "motor_idle"),
       "and the idle layer has faded out entirely");
    ck(voice_playing(audio_mix(), "motor_accel1"),
       "with the accel layer under it (chn.dat's car_motor_2)");
    ck(fast_loud > idle_loud,
       "flat out is louder than idle (%.0f vs %.0f)", fast_loud, idle_loud);

    printf("\n-- C2 the surface loop --\n");
    fake_car(5.f, 1.f, 4, 0);
    sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
    sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
    ground_loud = loudness(BLK);
    ck(sfx_current_surface() == SURF_WOOD,
       "the material comes from the GRID, not the track default "
       "(got '%s', default is '%s')",
       sfx_surface_name(sfx_current_surface()), sfx_surface_name(SURF_SAND));
    ck(voice_playing(audio_mix(), "car_surf_wood"),
       "and it plays that material's loop");

    fake_car(5.f, 1.f, 0, 0);
    sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
    sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
    air_loud = loudness(BLK);
    ck(!voice_playing(audio_mix(), "car_surf_wood"),
       "airborne STOPS the surface loop -- there are no tyres on anything");
    ck(air_loud < ground_loud,
       "and is quieter overall (%.0f vs %.0f)", air_loud, ground_loud);

    /* WATER OVERRIDES THE MATERIAL, and only when a wheel is actually wet.
     *
     * This is the sfx half of the pier report: driving out over the sea on a
     * wooden deck came back car_surf_water, because rb_world.water answers per
     * COLUMN and in_water used to be a copy of that answer. The gate now lives in
     * rb_collide (see collide.c, and wetcheck's pier section for the same thing
     * measured on the real grid) -- what is asserted HERE is the rule this file
     * owns: that sfx_update reads the flag and does not invent it. Both
     * directions, from the same fixture, so a mutant that hardcodes either one
     * cannot pass. */
    fake_car(5.f, 1.f, 4, 0);
    CAR.hit[0].in_water = 1;
    sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
    sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
    ck(sfx_current_surface() == SURF_WATER,
       "one wet wheel puts the surface loop on water (got '%s')",
       sfx_surface_name(sfx_current_surface()));
    ck(voice_playing(audio_mix(), "car_surf_water"),
       "and it is car_surf_water that plays");

    fake_car(5.f, 1.f, 4, 0);           /* same grid, same place, wheels dry */
    sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
    sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
    ck(sfx_current_surface() == SURF_WOOD,
       "and with the wheels DRY over the same cell it is the grid's own "
       "material again (got '%s')", sfx_surface_name(sfx_current_surface()));
    ck(voice_playing(audio_mix(), "car_surf_wood")
       && !voice_playing(audio_mix(), "car_surf_water"),
       "car_surf_wood is up and car_surf_water is not -- a deck over the sea "
       "sounds like a deck");

    printf("\n-- C3 volumes --\n");
    sfx_volumes(0.5f, 0.25f);
    ck(near(sfx_vol_sfx(), 0.5f, 1e-6f) && near(sfx_vol_music(), 0.25f, 1e-6f),
       "sfx_volumes round-trips");
    sfx_volumes(5.f, -1.f);
    ck(near(sfx_vol_sfx(), 1.f, 1e-6f) && near(sfx_vol_music(), 0.f, 1e-6f),
       "and clamps to 0..1");
    sfx_volumes(1.f, 1.f);

    /* Muting must actually mute: a master that only scales new voices leaves
       every already-running loop at full level. */
    fake_car(5.f, 1.f, 4, 0);
    sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
    sfx_volumes(0.f, 0.f);
    mix_render(audio_mix(), buf, BLK);      /* one block for the ramp to land */
    mix_render(audio_mix(), buf, BLK);
    ck(peak_ch(0, BLK) == 0.f && peak_ch(1, BLK) == 0.f,
       "volume 0 silences voices that were ALREADY playing (peak %.0f)",
       peak_ch(0, BLK));
    sfx_volumes(1.f, 1.f);

    printf("\n-- C4 pause --\n");
    /* Cleared first, and this matters. sfx_pause stops the LOOPS; it deliberately
       does not cut one-shots off, so a cue still ringing from an earlier part
       answers for the loops here. C2's wet-wheel case fires water_splash, which is
       an 11-second sample, and it took this check from 0 to a peak of 8432 without
       anything about pausing having changed. Isolate the fixture. */
    mix_stop_all(audio_mix());
    fake_car(5.f, 1.f, 4, 0);
    sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
    ck(voice_playing(audio_mix(), "motor_engine"),
       "(the engine loop is up before the menu opens)");
    sfx_pause(1);
    /* By NAME as well as by energy, which is this file's own rule -- a peak of
       zero is also what a broken bank gives you. */
    ck(!voice_playing(audio_mix(), "motor_engine")
       && !voice_playing(audio_mix(), "motor_idle")
       && !voice_playing(audio_mix(), "car_surf_wood")
       && !voice_playing(audio_mix(), "noise1"),
       "the menu stops every loop by name -- engine, idle, surface and bed");
    mix_render(audio_mix(), buf, BLK);
    mix_render(audio_mix(), buf, BLK);
    ck(peak_ch(0, BLK) == 0.f,
       "and nothing is left making noise -- the world freezes but the audio "
       "thread does not (peak %.0f)", peak_ch(0, BLK));
    sfx_pause(0);

    /* ---- C5 the knockable props ---------------------------------------- *
     *
     * Reported as "hitable objects sounds missing", and they were: nothing in the
     * port had ever called anything for a prop, and car_cdt_obj was paged in at
     * sfx_init and then never played by anyone.
     *
     * Asserted BY NAME throughout, for the reason voice_playing exists at all --
     * with the engine and the ambient bed running, "something got louder" is no
     * evidence that a bottle made a noise. */
    printf("\n-- C5 prop hits --\n");
    {
        float at[3] = { 1.f, 0.f, -3.f };
        int i, k, resolved = 0, i_cola = -1, i_ball = -1;

        /* Every model's recovered sound is really in the bank. This is what
           catches a rename in snd.dat or a wav that stopped being packed; the
           failure mode there is silence, which complains about nothing. */
        for (i = 0; i < PROP_N_MODELS; i++) {
            if (mix_find(audio_mix(), PROP_MODELS[i].sound) >= 0)
                resolved++;
            else
                printf("    %s -> %s NOT IN BANK\n",
                       PROP_MODELS[i].name, PROP_MODELS[i].sound);
            if (!strcmp(PROP_MODELS[i].name, "Cola")) i_cola = i;
            if (!strcmp(PROP_MODELS[i].name, "Ball")) i_ball = i;
        }
        ck(resolved == PROP_N_MODELS,
           "all %d prop models' sounds resolve in the bank (%d did)",
           PROP_N_MODELS, resolved);
        ck(i_cola >= 0 && i_ball >= 0, "Cola and Ball are both in PROP_MODELS");

        if (i_cola >= 0 && i_ball >= 0) {
        /* Different objects make DIFFERENT noises, which is the entire reason for
           reading stone.sb instead of picking one impact sound. Bound to two
           models the data gives different wavs: if that stopped being true this
           fails rather than quietly testing nothing. */
        ck(strcmp(PROP_MODELS[i_cola].sound, PROP_MODELS[i_ball].sound) != 0,
           "a can and a ball do not share a sound ('%s' vs '%s')",
           PROP_MODELS[i_cola].sound, PROP_MODELS[i_ball].sound);

        /* A real knock plays that model's own wav, and the car's cue with it. */
        fake_car(3.f, 0.f, 4, 0);
        sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
        mix_stop_all(audio_mix());
        sfx_prop_hit(i_cola, at, 3.0f);
        ck(voice_playing(audio_mix(), PROP_MODELS[i_cola].sound),
           "hitting a Cola plays '%s'", PROP_MODELS[i_cola].sound);
        ck(voice_playing(audio_mix(), "car_cdt_obj"),
           "and the car's own car_cdt_obj cue with it");

        /* The cooldown. prop.c's edge covers a contact the car HOLDS; this covers
           one that keeps breaking and remaking, which is what a can trapped under
           a wheel does -- every re-make is a fresh edge. */
        mix_stop_all(audio_mix());
        sfx_prop_hit(i_cola, at, 3.0f);
        ck(!voice_playing(audio_mix(), PROP_MODELS[i_cola].sound),
           "a second hit on the same model inside the cooldown is dropped");
        ck(!voice_playing(audio_mix(), PROP_MODELS[i_ball].sound),
           "(and the ball has not been touched yet)");
        sfx_prop_hit(i_ball, at, 3.0f);
        ck(voice_playing(audio_mix(), PROP_MODELS[i_ball].sound),
           "but a different model is not gated by it -- the cooldown is per model");

        /* sfx_update is what ages the cooldowns; a second of it clears them. */
        for (k = 0; k < 60; k++)
            sfx_update(&CAR, eye, 0.f, 1.f / 60.f);

        /* A nudge is not a knock. Without a floor, every prop the car merely
           leans against announces itself. 2 cm/s is far below sfx.c's own
           PROP_MIN_SPEED and this must not depend on its exact value. */
        mix_stop_all(audio_mix());
        sfx_prop_hit(i_cola, at, 0.02f);
        ck(!voice_playing(audio_mix(), PROP_MODELS[i_cola].sound),
           "a 2 cm/s nudge plays nothing");

        /* A hard knock is louder than a gentle one. Read off the VOICE'S OWN
           gain, not the rendered mix, so the engine and the bed cannot answer
           for it. */
        {
            int snd = mix_find(audio_mix(), PROP_MODELS[i_cola].sound);
            float soft = -1.f, hard = -1.f;

            for (k = 0; k < 60; k++)
                sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
            mix_stop_all(audio_mix());
            sfx_prop_hit(i_cola, at, 0.5f);
            for (i = 0; i < MIX_VOICES; i++)
                if (audio_mix()->v[i].active && audio_mix()->v[i].snd == snd)
                    soft = audio_mix()->v[i].gain;
            for (k = 0; k < 60; k++)
                sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
            mix_stop_all(audio_mix());
            sfx_prop_hit(i_cola, at, 6.0f);
            for (i = 0; i < MIX_VOICES; i++)
                if (audio_mix()->v[i].active && audio_mix()->v[i].snd == snd)
                    hard = audio_mix()->v[i].gain;
            printf("    gain: 0.5 m/s -> %.3f, 6 m/s -> %.3f\n", soft, hard);
            ck(soft > 0.f && hard > soft * 1.4f,
               "a hard knock is much louder than a gentle one (%.3f vs %.3f)",
               hard, soft);
        }

        /* It is POSITIONAL, over the radii stone.sb declares, so a prop struck
           across the map cannot be heard. The authored rmax is 8 m. */
        {
            float far_[3] = { 400.f, 0.f, 0.f };
            float l = 0.f, r = 0.f;
            int snd = mix_find(audio_mix(), PROP_MODELS[i_ball].sound);

            for (k = 0; k < 60; k++)
                sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
            mix_stop_all(audio_mix());
            mix_listener(audio_mix(), 0.f, 0.f, 0.f, 0.f);
            sfx_prop_hit(i_ball, far_, 4.0f);
            for (i = 0; i < MIX_VOICES; i++)
                if (audio_mix()->v[i].active && audio_mix()->v[i].snd == snd)
                    mix_pan(audio_mix(), &audio_mix()->v[i], &l, &r);
            ck(l == 0.f && r == 0.f,
               "a prop struck 400 m away is inaudible (l %.3f r %.3f), and the "
               "radius that says so is the authored %.0f m",
               l, r, PROP_MODELS[i_ball].snd_rmax);

            /* ...and one at the listener's feet is not. Otherwise "inaudible"
               above could be passing because nothing plays at all. */
            for (k = 0; k < 60; k++)
                sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
            mix_stop_all(audio_mix());
            l = r = 0.f;
            sfx_prop_hit(i_ball, (const float[3]){ 0.f, 0.f, 0.f }, 4.0f);
            for (i = 0; i < MIX_VOICES; i++)
                if (audio_mix()->v[i].active && audio_mix()->v[i].snd == snd)
                    mix_pan(audio_mix(), &audio_mix()->v[i], &l, &r);
            ck(l > 0.f && r > 0.f,
               "while the same knock at the listener's feet is (l %.3f r %.3f)",
               l, r);
        }
        }
    }

    /* ---- C6 the characters' voices, asked for BY NAME -------------------- *
     *
     * sfx_char_wav takes the wav a model declares in its own MOD_SNDCHANNEL
     * (char.c's chr_model_wav) instead of a voice the caller picked. It exists
     * because the caller's own rule -- "a Woman sounds like a woman, everything
     * else sounds like a man" -- gave a squashed CRAB a man's voice, and the Crab
     * declares no channel at all.
     *
     * So the pair that matters is: a name the models do declare PLAYS, and NULL
     * (a Crab, a Spider, a Vulture, a road car) plays nothing. Both halves, or a
     * version that plays man_voice for everything passes the first.
     */
    {
        const float at[3] = { 0.f, 0.f, 0.f };
        int i;
        printf("\n-- C6 character voices --\n");
        for (i = 0; i < 4; i++) {
            const char *w[4] = { "man_voice", "woman_voice", "dog_attack",
                                 "seagull_vzliot" };
            mix_stop_all(audio_mix());
            sfx_update(&CAR, eye, 0.f, 1.f / 60.f);   /* clear the cooldown */
            sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
            mix_stop_all(audio_mix());
            sfx_char_wav(w[i], at, 1.f);
            ck(voice_playing(audio_mix(), w[i]),
               "%s plays when a model names it", w[i]);
        }
        /* The Crab's answer, which is NULL, and a name no voice carries. */
        for (i = 0; i < 30; i++) sfx_update(&CAR, eye, 0.f, 1.f / 60.f);
        mix_stop_all(audio_mix());
        sfx_char_wav(NULL, at, 1.f);
        sfx_char_wav("crab_voice_that_does_not_exist", at, 1.f);
        {
            int k, live = 0;
            for (k = 0; k < MIX_VOICES; k++)
                if (audio_mix()->v[k].active) live++;
            ck(live == 0,
               "a model with no MOD_SNDCHANNEL is SILENT -- the Crab and the "
               "Spider (%d voices up)", live);
        }
    }

    sfx_shutdown();
}

/* ========================================================================== */

int main(void)
{
    mix_t m;
    int rc;

    printf("audio_test -- mix.c / sfx.c / audio.c\n");

    write_bank();
    rc = mix_init(&m, BANK_PATH);
    if (rc != 0) {
        printf("  [FAIL] mix_init on the synthetic bank (rc %d)\n", rc);
        return 1;
    }
    if (mix_music_init(&m, 4096) != 0) {
        printf("  [FAIL] mix_music_init\n");
        return 1;
    }

    part_a_bank(&m);
    part_a_voices(&m);
    part_a_render(&m);
    part_a_pan(&m);
    part_a_ramp(&m);
    part_a_music(&m);
    mix_free(&m);

    part_b();
    part_c();
    audio_shutdown();

    printf("\n%d checks, %d failed\n", checks, fails);
    if (fails) printf("FAILED\n");
    else printf("all audio checks passed\n");
    return fails ? 1 : 0;
}
