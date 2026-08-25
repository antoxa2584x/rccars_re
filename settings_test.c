/*
 * Host test for the settings file -- settings.c.
 *
 * Two halves, and the second is the one that matters. The round trip (write it,
 * read it back, get the same numbers) can only fail if the formatter and the
 * parser disagree; what a settings file actually does in the field is arrive
 * TRUNCATED, EDITED BY HAND or from ANOTHER BUILD, and every one of those has to
 * cost one line rather than the launch. `track 99` indexes TRACKS[], so the
 * clamp checks below stand in for an out-of-bounds read at startup, before any
 * menu exists to correct it.
 *
 * Runs entirely on files in the current directory, through settings_set_path,
 * and removes them on the way out.
 *
 * 23 MUTANTS TRIED, 21 DIE. The two that live are settings.c's stripping of `#`
 * and of `\r`, and neither is a hole in this file: `strtol` stops at both
 * characters on its own, so with the current grammar -- every value an integer --
 * no input can tell the two versions apart. The strip stays because it is what
 * makes that true BY CONSTRUCTION rather than by accident of what strtol skips,
 * and the first non-integer key added would need it. Said out loud here because a
 * survivor left unexplained is indistinguishable from a check that was never
 * written.
 */

#include "settings.h"
#include "menu.h"
#include "tracks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ui.h stubs -- menu.c is on the link line for menu_init (the source of every
   default this file claims survives a partial file), and menu_draw drags these
   in. Nothing here draws. */
void ui_init(void){}
void ui_begin(int w,int h){(void)w;(void)h;}
void ui_end(void){}
void ui_rect(float a,float b,float c,float d,float e,float f,float g,float h){
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;}
void ui_text(float x,float y,float s,float r,float g,float b,float a,const char*t){
    (void)x;(void)y;(void)s;(void)r;(void)g;(void)b;(void)a;(void)t;}
float ui_text_w(float s,const char*t){return (float)strlen(t)*10.f*s;}
float ui_text_h(float s){return 19.f*s;}

#define PATH "settings_test.tmp"

static int fails, checks;
static void ck(int ok, const char *w)
{
    checks++;
    if (!ok) { printf("  FAIL: %s\n", w); fails++; }
}

static void put(const char *text)
{
    FILE *f = fopen(PATH, "wb");
    if (!f) { printf("  FAIL: cannot write " PATH "\n"); fails++; return; }
    fwrite(text, 1, strlen(text), f);
    fclose(f);
}

static int exists(const char *p)
{
    FILE *f = fopen(p, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static char *slurp(const char *p, char *buf, int n)
{
    FILE *f = fopen(p, "rb");
    size_t got = 0;
    if (f) { got = fread(buf, 1, (size_t)n - 1, f); fclose(f); }
    buf[got] = 0;
    return buf;
}

/* A fresh menu, as main() has it before settings_load runs. */
static void fresh(menu_t *m)
{
    menu_init(m, 0, TRACKS[0].car);
}

int main(void)
{
    menu_t m, d;
    char buf[4096];
    int i;

    /* ---- 0. the first save writes even when every value is zero ----------- */
    /* BEFORE settings_set_path, which resets the snapshot: this is about the
       state the module starts a LAUNCH in. An all-zero menu is a reachable one
       -- both volumes down, the car light off, track 1, car 1, stock everything
       -- and a "we already know what is on the card" flag initialised the wrong
       way would silently drop exactly that save, since it compares equal to a
       zeroed snapshot. Writes and then removes the module's default file, whose
       name it asks for rather than assumes. */
    {
        const char *deflt = settings_path();
        memset(&m, 0, sizeof m);
        ck(settings_save_if_changed(&m) == 1,
           "with nothing yet known to be on the card, an all-zero menu is written");
        remove(deflt);
    }

    settings_set_path(PATH);
    remove(PATH);

    /* ---- 1. no file at all ------------------------------------------------ */
    fresh(&d);
    fresh(&m);
    ck(settings_load(&m) == 0, "no file: load reports nothing read");
    ck(memcmp(&m, &d, sizeof m) == 0,
       "no file: NOTHING in the menu is touched, not one field");

    /* ---- 2. the round trip ------------------------------------------------ */
    fresh(&m);
    m.track = 7;
    m.car = 2;
    m.skin[0] = 1; m.skin[1] = 3; m.skin[2] = 2;
    m.tires = 2; m.reso = 1; m.boost = 3;
    m.vol_sfx = 4; m.vol_music = 9;
    m.tex_quality = 2; m.tex_swap_rb = 1; m.car_light = 0;
    ck(settings_save(&m) == 1, "save writes the file");
    ck(exists(PATH), "and the file is there afterwards");
    {
        menu_t r;
        fresh(&r);
        ck(settings_load(&r) == 1, "load reports a file read");
        ck(r.track == 7 && r.car == 2, "track and car come back");
        ck(r.skin[0] == 1 && r.skin[1] == 3 && r.skin[2] == 2,
           "the paint comes back PER CAR -- three separate slots");
        ck(r.tires == 2 && r.reso == 1 && r.boost == 3,
           "the three upgrades come back");
        ck(r.vol_sfx == 4 && r.vol_music == 9, "both volumes come back");
        ck(r.tex_quality == 2 && r.tex_swap_rb == 1,
           "the two texture rows come back");
        ck(r.car_light == 0,
           "car lighting comes back OFF -- the one field whose default is 1, so a "
           "load that quietly did nothing would still look right");

        /* ---- 3. and NOTHING ELSE comes back ------------------------------- */
        ck(r.req_track == -1 && r.req_car == -1,
           "load raises no load request -- main.c does the first load itself");
        ck(r.req_reload == 0, "and no reload request");
        ck(r.open == 0 && r.row == 0 && r.cue == 0,
           "the menu is not left open on a saved row");
        ck(r.skins == 1,
           "`skins` is NOT restored: it is counted out of the packed car, and a "
           "stale one would offer paint that is not in the scene");
    }

    /* ---- 4. a file with one key in it ------------------------------------- */
    put("vol_music 2\n");
    fresh(&m);
    ck(settings_load(&m) == 1, "one-key file is read");
    ck(m.vol_music == 2, "the key it has is applied");
    ck(m.vol_sfx == d.vol_sfx && m.car_light == d.car_light
       && m.track == d.track && m.boost == d.boost,
       "every key it does NOT have keeps menu_init's default");

    /* ---- 5. comments, blank lines, whitespace, unknown keys --------------- */
    put("# a comment\n"
        "\n"
        "   \t \n"
        "track 3   # inline comment\n"
        "somethingelse 5\n"
        "\ttires 2\r\n"
        "vol_sfx\n"                     /* key with no value */
        "= = =\n");
    fresh(&m);
    ck(settings_load(&m) == 1, "a file of junk and two real lines is read");
    ck(m.track == 3, "a value with a comment after it is read");
    ck(m.tires == 2, "an indented line with a CRLF ending is read");
    ck(m.vol_sfx == d.vol_sfx, "a key with no value changes nothing");

    /* A comment that contains NUMBERS. This is where stripping it early earns
       its keep: `skin # 1 2 3` must be a skin line with no values on it, not
       three cars repainted out of the prose. */
    put("skin # 1 2 3\n");
    fresh(&m);
    settings_load(&m);
    ck(m.skin[0] == d.skin[0] && m.skin[1] == d.skin[1] && m.skin[2] == d.skin[2],
       "numbers inside a comment are not values");

    /* A SHORT skin line leaves the cars it does not mention alone. Through
       settings_parse rather than a file, because that is the only way to start
       from something other than the defaults -- against a default of 0 a field
       wrongly zeroed and a field correctly left alone look identical. */
    {
        settings_t s;
        menu_t z;
        fresh(&z);
        settings_from_menu(&z, &s);
        s.skin[0] = 1; s.skin[1] = 3; s.skin[2] = 2;
        ck(settings_parse("skin 0\n", &s) == 1, "a one-value skin line parses");
        ck(s.skin[0] == 0 && s.skin[1] == 3 && s.skin[2] == 2,
           "and repaints only the car it names");
    }

    /* ---- 6. the prefix trap ---------------------------------------------- */
    /* `car` is a prefix of `car_light`, and a bare strncmp would let either line
       set the other. Both directions, because only one of them is obvious. */
    put("car_light 0\n");
    fresh(&m);
    settings_load(&m);
    ck(m.car_light == 0 && m.car == d.car,
       "`car_light` does not write `car`");
    put("car 2\n");
    fresh(&m);
    settings_load(&m);
    ck(m.car == 2 && m.car_light == d.car_light,
       "`car` does not write `car_light`");
    /* And the general form, which does not depend on the order the keys happen
       to be tested in: a key needs a SEPARATOR after it, so `track5` is not
       `track` with a 5 stuck to it. Without that, this line sets track to 7. */
    put("track5 7\n");
    fresh(&m);
    settings_load(&m);
    ck(m.track == d.track, "a key with junk stuck to it is not that key");

    /* ---- 7. everything out of range is clamped, not obeyed ---------------- */
    put("track 99\ncar -5\nskin 9 -1 400\ntires 8\nreso -3\nboost 77\n"
        "vol_sfx 500\nvol_music -20\ntex_quality 7\ntex_swap_rb 3\n"
        "car_light -1\n");
    fresh(&m);
    ck(settings_load(&m) == 1, "a file of impossible values is still read");
    ck(m.track >= 0 && m.track < N_TRACKS, "track is clamped INTO TRACKS[]");
    ck(m.track == N_TRACKS - 1, "and clamped to the end, not reset to zero");
    ck(m.car >= 0 && m.car < MENU_N_CARS, "car is clamped into RB_CARS[]");
    for (i = 0; i < MENU_N_CARS; i++)
        ck(m.skin[i] >= 0 && m.skin[i] < MENU_SKINS, "each skin is clamped");
    ck(m.tires == 3 && m.reso == 0 && m.boost == 3, "the upgrades are clamped");
    ck(m.vol_sfx == MENU_VOL_STEPS && m.vol_music == 0,
       "the volumes are clamped to their own ends");
    ck(m.tex_quality == MENU_TEXQUAL_LEVELS - 1, "quality is clamped");
    ck(m.tex_swap_rb == 1 && m.car_light == 1,
       "the two flags become 0 or 1 -- `3` and `-1` are both true");

    /* ---- 8. a file from a later version is not read at all ---------------- */
    {
        char later[128];
        snprintf(later, sizeof later, "version %d\ntrack 5\n",
                 SETTINGS_VERSION + 1);
        put(later);
        fresh(&m);
        ck(settings_load(&m) == 0, "a newer version reports nothing read");
        ck(m.track == d.track,
           "and applies NONE of it -- the same key may mean something else");
    }
    {
        char earlier[128];
        snprintf(earlier, sizeof earlier, "version %d\ntrack 5\n",
                 SETTINGS_VERSION);
        put(earlier);
        fresh(&m);
        ck(settings_load(&m) == 1 && m.track == 5,
           "this version's own file is read");
    }

    /* ---- 9. binary rubbish ------------------------------------------------ */
    {
        FILE *f = fopen(PATH, "wb");
        if (f) {
            for (i = 0; i < 700; i++) {
                unsigned char c = (unsigned char)(i * 37 + 1);   /* never 0 */
                fwrite(&c, 1, 1, f);
            }
            fclose(f);
        }
        fresh(&m);
        settings_load(&m);              /* must not crash or read past the end */
        ck(m.track >= 0 && m.track < N_TRACKS
           && m.car >= 0 && m.car < MENU_N_CARS
           && m.vol_sfx >= 0 && m.vol_sfx <= MENU_VOL_STEPS,
           "700 bytes of rubbish leave every field in range");
    }

    /* ---- 10. save_if_changed does not touch the card for nothing ---------- */
    fresh(&m);
    m.vol_sfx = 3;
    ck(settings_save(&m) == 1, "save for the compare test");
    remove(PATH);                        /* the file's absence is the detector */
    ck(settings_save_if_changed(&m) == 0,
       "an unchanged menu writes NOTHING -- a menu opened to look costs no I/O");
    ck(!exists(PATH), "and really did not write: the file is still gone");
    m.vol_sfx = 8;
    ck(settings_save_if_changed(&m) == 1, "a changed value does write");
    ck(exists(PATH), "and the file is back");
    /* And the same after a LOAD, which is the common case by far: the app starts,
       reads the file, and the player opens the menu to look at something. What
       was just READ counts as what is on the card, exactly as what was written
       does -- so the file here is a hand-written one that agrees with NEITHER the
       defaults nor the last save, and only a load that recorded it can know that
       nothing needs writing. */
    put("track 6\nvol_music 1\n");
    fresh(&m);
    ck(settings_load(&m) == 1, "a hand-written file loads");
    remove(PATH);
    ck(settings_save_if_changed(&m) == 0,
       "a menu closed unchanged after a LOAD writes nothing either");
    ck(!exists(PATH), "and really did not write: the file is still gone");
    /* A change the CLAMP flattens is not a change: 4 and 4 are the same file. */
    m.tex_swap_rb = 1;
    ck(settings_save_if_changed(&m) == 1, "a flag change writes");
    remove(PATH);
    m.tex_swap_rb = 2;                   /* clamps to the 1 already saved */
    ck(settings_save_if_changed(&m) == 0,
       "a change the clamp flattens away writes nothing");
    ck(!exists(PATH), "and really did not write");

    /* ---- 11. the first save always writes -------------------------------- */
    /* A card with no file on it must not be mistaken for one that agrees. */
    settings_set_path(PATH);             /* also resets the snapshot */
    remove(PATH);
    fresh(&m);
    ck(settings_save_if_changed(&m) == 1,
       "with nothing known to be saved, even the defaults are written");

    /* ---- 12. the written file is self-describing and parseable ------------ */
    fresh(&m);
    m.track = 4;
    m.skin[1] = 2;
    settings_save(&m);
    slurp(PATH, buf, sizeof buf);
    ck(strstr(buf, "version") != NULL, "the file names its version");
    ck(strstr(buf, "#") != NULL, "and carries its own comments");
    ck(strlen(buf) < 1024, "and fits the formatter's buffer with room to spare");
    {
        /* Parsed straight out of the text, with no file in between: the parser
           must accept every line the formatter writes, comments included. */
        settings_t s;
        menu_t z;
        fresh(&z);
        settings_from_menu(&z, &s);
        ck(settings_parse(buf, &s) == 1, "the formatter's own output parses");
        ck(s.track == 4 && s.skin[1] == 2,
           "and carries the values it was given");
    }

    /* ---- 12b. the FILE never carries an out-of-range value ---------------- */
    /* Clamped on the way out as well as in, because the file is what the next
       launch -- and anyone editing it by hand -- takes as the range. Read back
       through settings_parse with NO clamp after it, so this sees what is really
       written rather than what loading would have repaired. */
    fresh(&m);
    m.track = 99;
    m.vol_sfx = 77;
    m.skin[2] = 400;
    settings_save(&m);
    slurp(PATH, buf, sizeof buf);
    {
        settings_t s;
        menu_t z;
        fresh(&z);
        settings_from_menu(&z, &s);
        settings_parse(buf, &s);
        ck(s.track >= 0 && s.track < N_TRACKS
           && s.vol_sfx >= 0 && s.vol_sfx <= MENU_VOL_STEPS
           && s.skin[2] >= 0 && s.skin[2] < MENU_SKINS,
           "an impossible value is clamped BEFORE it is written, not after");
    }

    /* ---- 13. a truncated write ------------------------------------------- */
    /* What a power cut during the write leaves: a prefix of a good file. Every
       prefix has to be either applied or ignored, never misread. Straight through
       parse+clamp rather than settings_load, because a hundred loads is a hundred
       card writes' worth of log lines and the FAIL lines have to stay findable --
       the file path adds nothing here, the buffer being four times the file. */
    slurp(PATH, buf, sizeof buf);
    ck(strlen(buf) > 100, "there is a written file to truncate");
    for (i = 1; i < (int)strlen(buf); i++) {
        char cut[4096];
        settings_t s;
        menu_t z;
        memcpy(cut, buf, (size_t)i);
        cut[i] = 0;
        fresh(&z);
        settings_from_menu(&z, &s);
        if (!settings_parse(cut, &s))
            continue;                    /* refused outright, which is fine */
        settings_clamp(&s);
        if (!(s.track >= 0 && s.track < N_TRACKS
              && s.car >= 0 && s.car < MENU_N_CARS
              && s.tires >= 0 && s.tires < 4
              && s.reso >= 0 && s.reso < 4
              && s.boost >= 0 && s.boost < 4
              && s.vol_sfx >= 0 && s.vol_sfx <= MENU_VOL_STEPS
              && s.vol_music >= 0 && s.vol_music <= MENU_VOL_STEPS
              && s.tex_quality >= 0 && s.tex_quality < MENU_TEXQUAL_LEVELS
              && s.skin[0] >= 0 && s.skin[0] < MENU_SKINS)) {
            printf("  FAIL: a %d-byte prefix left a field out of range\n", i);
            fails++;
            break;
        }
    }
    /* And once through the real file path, so the truncated FILE is exercised
       too and not only the truncated string. */
    {
        char cut[4096];
        size_t half = strlen(buf) / 2;
        memcpy(cut, buf, half);
        cut[half] = 0;
        put(cut);
        fresh(&m);
        settings_load(&m);
        ck(m.track >= 0 && m.track < N_TRACKS
           && m.vol_sfx >= 0 && m.vol_sfx <= MENU_VOL_STEPS,
           "half a written file loads to something in range");
    }

    remove(PATH);
    remove(PATH SETTINGS_TMP_SUFFIX);
    printf("%d checks -- %s\n", checks,
           fails ? "SOME CHECKS FAILED" : "all settings checks passed");
    return fails ? 1 : 0;
}
