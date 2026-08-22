/*
 * hudshot.c -- dump the in-race HUD's geometry so it can be RENDERED off the
 * game's real textures, on the host.
 *
 * ui_test asserts where every quad went; it cannot say whether the result looks
 * like a HUD. This project's own standing rule is that shredded geometry, gold
 * water and blue sand were each only obvious in a picture, so: link ui.c and
 * race_ui.c against a recording GL stub, drive race_ui_draw at the Vita's own
 * 960x544, and print every triangle with its texture, its screen positions and
 * its UVs. rccars_re/hudshot.py composites that over the real .csi art.
 *
 * The texture handles are the INDEX INTO ITS OWN NAME TABLE plus one, so the
 * dump names its own art and the compositor needs no second table to keep in
 * step.
 *
 *   gcc -I<vita> -I<vita>/testgl -O2 hudshot.c <vita>/ui.c <vita>/race_ui.c \
 *       <vita>/dirarrow.c <vita>/msg.c -lm -o hudshot
 *
 * msg.c is on it because the MESSAGE LAYER is where the banners live now, and
 * because two of its slots -- PAUSE and BEST LAP -- had never been drawn by
 * anything: "is that the right word art at the right size in the right place" is
 * exactly what a picture answers and a check does not. Pass a slot number as the
 * second argument to see one; the default is BEST LAP.
 *
 * dirarrow.c is in it because the DIRECTION ARROW is the one HUD element whose
 * shape is not a rectangle: it is a perspective projection of two chevrons, cut
 * into a grid of quads, and "does that look like an arrow pointing left" is not
 * a question ui_test can answer. The state below turns it, so the picture shows
 * the lean and the tint together, and posts the WRONG WAY banner mid-grow.
 */
#include "ui.h"
#include "race_ui.h"
#include "dirarrow.h"
#include "msg.h"
#include "tracks.h"
#include <vitaGL.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The art, in handle order. Handle 0 stays "untextured", which is what ui_rect
   and the minimap's dark plate draw with. */
static const char *const TEXNAME[] = {
    "-", "map", "map_arrow", "map_cp",
    "place1", "place2", "place3", "place4", "place5", "place6",
    "lap", "cockpit_sp1", "cockpit_sp2", "Smash26", "Smash20",
    "coc_str_2",
    /* the message layer's six, in the order 0x4af195 loads them -- which is the
       index MSG_SLOT's `tex' column uses, so handles 16..21 are MSG_TEX_NAME */
    "msg_low_signal", "msg_pause", "msg_wrong_way",
    "msg_hits", "msg_321_s_f", "msg_bestlap",
    "font.h"
};
#define TEX_MSG_BASE 16
#define N_TEX ((int)(sizeof TEXNAME / sizeof TEXNAME[0]))

/* --------------------------------------------------------- the GL recorder */
static const char *vp, *tp, *cp;
static int vs, ts, cs;
static GLuint bound;
static int tex_on = 1;
static GLuint next_tex = 100;

void glVertexPointer(GLint s, GLenum t, GLsizei st, const void *p)
{ (void)s; (void)t; vs = st; vp = p; }
void glTexCoordPointer(GLint s, GLenum t, GLsizei st, const void *p)
{ (void)s; (void)t; ts = st; tp = p; }
void glColorPointer(GLint s, GLenum t, GLsizei st, const void *p)
{ (void)s; (void)t; cs = st; cp = p; }
void glBindTexture(GLenum t, GLuint id) { (void)t; bound = id; }
void glGenTextures(GLsizei n, GLuint *o) { GLsizei i; for (i=0;i<n;i++) o[i]=next_tex++; }
void glDeleteTextures(GLsizei n, const GLuint *o) { (void)n; (void)o; }
void glTexImage2D(GLenum a,GLint b,GLint c,GLsizei d,GLsizei e,GLint f,
                  GLenum g,GLenum h,const void*i)
{ (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i; }
void glTexParameteri(GLenum a,GLenum b,GLint c){(void)a;(void)b;(void)c;}
void glEnable(GLenum c) { if (c == GL_TEXTURE_2D) tex_on = 1; }
void glDisable(GLenum c) { if (c == GL_TEXTURE_2D) tex_on = 0; }
GLboolean glIsEnabled(GLenum c) { (void)c; return 0; }
void glEnableClientState(GLenum c) { (void)c; }
void glDisableClientState(GLenum c) { (void)c; }
void glMatrixMode(GLenum m) { (void)m; }
void glPushMatrix(void) {}
void glPopMatrix(void) {}
void glLoadIdentity(void) {}
void glLoadMatrixf(const float *m) { (void)m; }
void glMultMatrixf(const float *m) { (void)m; }
void glBlendFunc(GLenum a, GLenum b) { (void)a; (void)b; }
void glDepthMask(GLboolean b) { (void)b; }
/* scene_draw_blend's; checked in vis_test part 15, not here */
void glDepthFunc(GLenum f) { (void)f; }
void glDrawElements(GLenum a,GLsizei b,GLenum c,const void*d)
{ (void)a;(void)b;(void)c;(void)d; }
void glColor4f(float r,float g,float b,float a){(void)r;(void)g;(void)b;(void)a;}

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    GLsizei i;
    (void)mode;
    /* A vitaGL handle from glGenTextures (>= 100) is ui.c's own compiled-in
       font; anything under N_TEX is one of the named ones above. */
    printf("draw %s %d\n",
           bound >= 100 ? "font.h"
                        : (bound < (GLuint)N_TEX ? TEXNAME[bound] : "?"),
           (int)count);
    for (i = 0; i < count; i++) {
        const float *p = (const float *)(vp + (size_t)(first + i) * vs);
        const float *t = (const float *)(tp + (size_t)(first + i) * ts);
        const float *c = (const float *)(cp + (size_t)(first + i) * cs);
        printf("  %.3f %.3f  %.6f %.6f  %.3f %.3f %.3f %.3f\n",
               p[0], p[1], t[0], t[1], c[0], c[1], c[2], c[3]);
    }
}

/* -------------------------------------------------------------------- main */
int main(int argc, char **argv)
{
    race_ui_tex T;
    race_ui_t R;
    race_ui_state S;
    int track = argc > 1 ? atoi(argv[1]) : 0;
    int want_slot = argc > 2 ? atoi(argv[2]) : MSG_BEST_LAP;
    int i;

    ui_init();

    memset(&T, 0, sizeof T);
    T.map = 1; T.arrow = 2; T.cp = 3;
    for (i = 0; i < 6; i++) T.place[i] = (unsigned int)(4 + i);
    T.lap = 10; T.dial = 11; T.dial_arc = 12;
    T.font_big = 13; T.font_small = 14;

    race_ui_init(&R, &T);
    race_ui_set_track(&R, track, T.map);
    race_ui_start(&R);
    race_ui_step(&R, 76.92f);          /* the clock reading opt_cock_all shows */

    memset(&S, 0, sizeof S);
    /* AT THE RACE START, out of tracks.h -- the .sb files, where MAP_CALIB is the
       exe, so where the marker lands against the painted ribbon compares two
       independent sources. READ IT AS A POINTER AND NOT AS A VERDICT: a glance at
       this picture at 165 px a track said the transform was right, and
       mapcheck.py then MEASURED 3 to 19 m of error on eight of the ten. Use that
       for the number; use this to see what the error looks like. The heading is
       the start's own. */
    S.car_x = TRACKS[track].x;
    S.car_z = TRACKS[track].z;
    S.car_yaw = TRACKS[track].yaw * 0.017453292519943295f;
    S.n_others = 3;
    for (i = 0; i < 3; i++) {
        S.other_x[i] = TRACKS[track].x + 8.f * (float)(i + 1);
        S.other_z[i] = TRACKS[track].z - 5.f * (float)(i + 1);
        /* Three different headings, so the picture shows whether their markers
           turn at all -- they used to be drawn unrotated. */
        S.other_yaw[i] = 2.0f * (float)i;
    }
    S.n_cp = 3;
    for (i = 0; i < 3; i++) {
        S.cp_x[i] = MAP_CALIB[track].world[i][0];
        S.cp_z[i] = MAP_CALIB[track].world[i][1];
    }
    S.cp_next = 1;
    S.place = 3; S.n_racers = 4;
    S.lap = 1; S.n_laps = 0;
    S.speed = 12.96f; S.speed_max = 20.f;       /* 29 mph, as the shot reads */
    /* Deliberately NOT full: a dial at 100% draws the same arc whichever way the
       sweep runs, so a picture taken there cannot show a reversed fill. */
    S.boost = 70.f; S.boost_max = 100.f;

    printf("screen 960 544\n");
    race_ui_draw(&R, &S, 960, 544);

    /* THE DIRECTION ARROW, turned. Driven rather than assigned: the angle is
       slewed and the tint chased, so a picture of a hand-set `ang' would not be
       a picture of anything the app can reach. The aim point is 50 degrees off
       the car's own heading, which is a corner rather than a cardinal -- and
       +a and -a agree at every cardinal. */
    {
        dirarrow_t D;
        dirarrow_tex DT;
        dirarrow_in DI;
        int f;
        DT.arrow = 15;
        dirarrow_init(&D, &DT);
        memset(&DI, 0, sizeof DI);
        DI.valid = 1;
        DI.car_x = 0.f; DI.car_z = 0.f; DI.car_yaw = 0.f;
        DI.aim_x = 40.f * 0.766f; DI.aim_z = 40.f * 0.643f;   /* 50 deg right */
        DI.prev_x = 0.f; DI.prev_z = -40.f;
        DI.seg_frac = 0.35f;      /* between the ramp's ends, so the tint shows */
        DI.cp_dist = 40.f;        /* clear of ARW_BLINK_NEAR, so it holds still */
        for (f = 0; f < 300; f++)
            dirarrow_step(&D, &DI, 1.f / 60.f);
        /* And the banner, part way up its own grow-in: at full size the ramp
           could be running either way and the picture could not tell. */
        dirarrow_draw(&D, 960, 544);
    }

    /* THE MESSAGE LAYER. Driven rather than assigned, and stopped PART WAY up
       its grow-in: at full size the ramp could be running either way and the
       picture could not tell. */
    {
        msg_t M;
        unsigned int MT[MSG_N_TEX];
        int i, f;
        for (i = 0; i < MSG_N_TEX; i++)
            MT[i] = (unsigned int)(TEX_MSG_BASE + i);
        msg_init(&M, MT);
        if (msg_post(&M, want_slot, MSG_BEST_LIFE, 0.f, 1)) {
            for (f = 0; f < 24; f++)
                msg_step(&M, 1.f / 60.f);
            msg_draw(&M, 960, 544);
        }
    }
    return 0;
}
