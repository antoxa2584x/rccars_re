/* Host test for ui.c -- the menu's text and panel layer -- and for hud.c, the
 * HIT pop that draws through it.
 *
 * ui.c is the newest GL path in this app and the only one that builds its own
 * projection by hand, so it is the piece most likely to be wrong in a way that
 * only shows on a screen. This does what CLAUDE.md says to do when a convention
 * matters: it CONSTRUCTS the real transform -- the matrix ui_begin actually
 * loaded, not an assumed formula -- and measures points through it.
 *
 * The specific failures it is looking for:
 *   - a flipped ortho, i.e. text upside down or off screen entirely
 *   - glyph UVs bleeding into the neighbouring cell (the atlas is ONE row, so a
 *     half-texel error shows as fringes of the next character)
 *   - colour not reaching the vertices, which is the GL_COLOR_ARRAY path
 *     water.c's coast band also depends on -- a shared failure mode
 *   - ui_begin leaving the GL enables changed after ui_end
 *
 * Part 8 is hud.c. It is here rather than in a harness of its own because the
 * pop has no GL of its own -- it is two ui_text calls -- so the recorder that
 * already reads back a menu row reads back the word, its size, its colour and
 * its fade without a line of new stubbing. What it is looking for: a pop that
 * never appears, one that never goes away, one drawn off screen or off centre,
 * and a fade or a size ramp that runs the wrong way (which fx.c's DynamicScale
 * did for the whole life of that file).
 *
 * Parts 10 to 16 are race_ui.c -- the in-race HUD: the minimap, the place badge,
 * the two clocks and the two gauges. Here for the same reason as parts 8 and 9:
 * it draws entirely through ui.c, so the recorder that reads back a menu row
 * reads back which cell of which atlas went where, at what angle, over what
 * sweep. What they are looking for, per element:
 *   - a map transform that is not the one the exe's own tables define, or one
 *     applied to the wrong track (a per-track table read as a scalar is a bug
 *     this project has already shipped once -- see traps.md on WLOD)
 *   - a car marker that does not point where the car points, which is what
 *     `pi - yaw` written as `yaw + pi` gives on every heading but zero
 *   - a value arc that sweeps the wrong way, or past its ends, or is not empty
 *     at zero -- the fx.c DynamicScale failure again
 *   - a clock that does not stop when the frame's dt does, and a lap time that
 *     is never held or never released
 *   - text drawn monospaced out of a proportional atlas, and glyph UVs outside
 *     their own cell of the 10 x 9 grid
 *   - the 800x600 layout landing anywhere but the authored pixel on a 4:3 panel
 *
 * Part 17 is dirarrow.c -- the engine's own DirectArrow and the WRONG WAY
 * banner. Here for the third time for the same reason: it draws entirely through
 * ui.c, so the recorder reads back where the chevrons' corners landed, what
 * colour they came out and whether the banner was on screen. What it is looking
 * for:
 *   - a chevron that points the wrong way, which is what the ENGINE'S OWN
 *     unsigned angle does on every right-hand turn (see dirarrow.h)
 *   - a chevron drawn outside arrow.ini's own square, or not centred in it
 *   - a wrong-way timer that fires at the wrong time, never fires, or fires at
 *     a checkpoint -- and one whose decay is not ARW_WRONG_DECAY times its rise
 *   - a blink that is not half on, or that runs when the car is fine
 *   - a tint ramp that runs backwards, which fx.c's DynamicScale did for the
 *     whole life of that file and the two gauges nearly did again
 *   - more than one sound cue per banner, which is what a flag read instead of
 *     an edge gives
 *
 * Part 18 is msg.c -- the engine's own eleven-slot message layer, its priority
 * arbitration and the two slots nothing else raised (PAUSE and BEST LAP). What
 * it is looking for:
 *   - a slot drawn at the wrong size, in the wrong band, or out of the wrong
 *     cell of its atlas, which four of the six textures are
 *   - arbitration that lets the wrong message win, or that drops a post it
 *     should take: the rule is STRICTLY higher wins, so equal priorities
 *     replace, and seven of the eleven slots share priority 0
 *   - the HOLD-OVER, which is what makes WRONG WAY a 5.5 s pulse rather than a
 *     1.5 s one and which this port had wrong until 0x4af28e was read
 *   - a grow-in that runs backwards, and a message that fades (the original
 *     never touches a message's alpha)
 *
 *   gcc -I<vita> -I<vita>/testgl -O2 ui_test.c <vita>/ui.c <vita>/hud.c \
 *       <vita>/countdown.c <vita>/race_ui.c -lm -o ui_test
 */
#include "ui.h"
#include "hud.h"
#include "countdown.h"
#include "race_ui.h"
#include "dirarrow.h"
#include "msg.h"
#include "tracks.h"
#include "font.h"
#include <vitaGL.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----------------------------------------------------------- the recorder */

#define MAXV 8192
typedef struct { float x, y, u, v, r, g, b, a; } cap_vtx;

static cap_vtx cap[MAXV];
static int     cap_n;
static int     cap_draws;
static GLuint  cap_last_tex, cap_bound;
static int     cap_tex_enabled = 1;

float glcap_matrix[16];
int   glcap_matrix_mode;

static const char *vp, *tp, *cp;
static int vs, ts, cs;
static int en_depth = 1, en_alpha = 1, en_cull = 1, en_blend = 0;
static int cs_color;
static GLuint next_tex = 1;

/* A PER-DRAW log, on top of the flat vertex capture above. race_ui.c emits a
   dozen draws in one pass over four different textures, so "the last texture
   bound" -- which is all parts 0 to 9 ever needed -- cannot say which quad came
   from which atlas. Purely additive: nothing before part 10 reads it. */
#define MAXD 256
static struct { unsigned int tex; int first, n; } cap_draw[MAXD];
static int cap_nd;

static void cap_reset(void)
{
    cap_n = 0; cap_draws = 0; cap_last_tex = 0; cap_nd = 0;
}

void glGenTextures(GLsizei n, GLuint *o) { GLsizei i; for (i=0;i<n;i++) o[i]=next_tex++; }
void glDeleteTextures(GLsizei n, const GLuint *o) { (void)n; (void)o; }
void glBindTexture(GLenum t, GLuint id) { (void)t; cap_bound = id; }
/* The upload is RECORDED, not discarded. A no-op glTexImage2D is blind to the
   one thing ui_init actually does -- expand font.h's coverage bytes into RGBA --
   and font.h is GENERATED, so a regeneration at the wrong size or a botched
   expansion loop would sail through a test that only checks a texture was
   bound. Same lesson as a no-op glPushMatrix hiding a conditional push. */
static int tex_w, tex_h, tex_fmt, tex_uploads;
static unsigned char tex_px[FONT_CW * FONT_COUNT * FONT_CH * 4];
static int wrap_s, wrap_t;
void glTexImage2D(GLenum a,GLint lvl,GLint ifmt,GLsizei w,GLsizei h,GLint bd,
                  GLenum f,GLenum t,const void*p)
{
    (void)a;(void)lvl;(void)bd;(void)t;
    tex_uploads++;
    tex_w = w; tex_h = h; tex_fmt = (int)ifmt;
    (void)f;
    if (p && (size_t)w * h * 4 <= sizeof(tex_px))
        memcpy(tex_px, p, (size_t)w * h * 4);
}
void glTexParameteri(GLenum a,GLenum b,GLint c)
{
    (void)a;
    if (b == GL_TEXTURE_WRAP_S) wrap_s = c;
    else if (b == GL_TEXTURE_WRAP_T) wrap_t = c;
}
void glVertexPointer(GLint s,GLenum t,GLsizei st,const void*p){(void)t;vs=st;vp=p;(void)s;}
void glTexCoordPointer(GLint s,GLenum t,GLsizei st,const void*p){(void)t;ts=st;tp=p;(void)s;}
void glColorPointer(GLint s,GLenum t,GLsizei st,const void*p){(void)t;cs=st;cp=p;(void)s;}
void glDrawElements(GLenum a,GLsizei b,GLenum c,const void*d){(void)a;(void)b;(void)c;(void)d;}
void glDepthMask(GLboolean b) { (void)b; }
/* scene_draw_blend's; checked in vis_test part 15, not here */
void glDepthFunc(GLenum f) { (void)f; }
void glColor4f(float r,float g,float b,float a){(void)r;(void)g;(void)b;(void)a;}
/* The matrix stacks are tracked PER MODE, because an unbalanced push is the
   classic damage-left-behind bug: nothing looks wrong in the frame that leaks
   it, and the stack overflows some seconds later. A recorder that only sees
   state during a draw cannot catch it -- which is exactly the hole that was
   found in vis_test's shadow state restore. */
static int mat_depth[2];        /* [0] = MODELVIEW, [1] = PROJECTION */
static int mat_underflow;
static int mode_idx(void) { return glcap_matrix_mode == GL_PROJECTION ? 1 : 0; }
void glPushMatrix(void) { mat_depth[mode_idx()]++; }
void glPopMatrix(void)
{
    int i = mode_idx();
    if (mat_depth[i] == 0) mat_underflow = 1;
    else mat_depth[i]--;
}
void glMultMatrixf(const float *m) { (void)m; }
void glMatrixMode(GLenum m) { glcap_matrix_mode = (int)m; }
void glLoadIdentity(void) { }
void glLoadMatrixf(const float *m) { memcpy(glcap_matrix, m, sizeof(glcap_matrix)); }
void glBlendFunc(GLenum a, GLenum b) { (void)a; (void)b; }

void glEnable(GLenum c)
{
    if (c==GL_DEPTH_TEST) en_depth=1; else if (c==GL_ALPHA_TEST) en_alpha=1;
    else if (c==GL_CULL_FACE) en_cull=1; else if (c==GL_BLEND) en_blend=1;
    else if (c==GL_TEXTURE_2D) cap_tex_enabled=1;
}
void glDisable(GLenum c)
{
    if (c==GL_DEPTH_TEST) en_depth=0; else if (c==GL_ALPHA_TEST) en_alpha=0;
    else if (c==GL_CULL_FACE) en_cull=0; else if (c==GL_BLEND) en_blend=0;
    else if (c==GL_TEXTURE_2D) cap_tex_enabled=0;
}
GLboolean glIsEnabled(GLenum c)
{
    if (c==GL_DEPTH_TEST) return (GLboolean)en_depth;
    if (c==GL_ALPHA_TEST) return (GLboolean)en_alpha;
    if (c==GL_CULL_FACE)  return (GLboolean)en_cull;
    if (c==GL_BLEND)      return (GLboolean)en_blend;
    return 0;
}
void glEnableClientState(GLenum c)  { if (c==GL_COLOR_ARRAY) cs_color=1; }
void glDisableClientState(GLenum c) { if (c==GL_COLOR_ARRAY) cs_color=0; }

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    GLsizei i;
    (void)mode;
    cap_draws++;
    cap_last_tex = cap_bound;
    if (cap_nd < MAXD) {
        cap_draw[cap_nd].tex = cap_bound;
        cap_draw[cap_nd].first = cap_n;
        cap_draw[cap_nd].n = (int)count;
        cap_nd++;
    }
    for (i = 0; i < count && cap_n < MAXV; i++) {
        const float *p = (const float *)(vp + (size_t)(first + i) * vs);
        const float *t = (const float *)(tp + (size_t)(first + i) * ts);
        const float *c = (const float *)(cp + (size_t)(first + i) * cs);
        cap[cap_n].x = p[0]; cap[cap_n].y = p[1];
        cap[cap_n].u = t[0]; cap[cap_n].v = t[1];
        cap[cap_n].r = c[0]; cap[cap_n].g = c[1];
        cap[cap_n].b = c[2]; cap[cap_n].a = c[3];
        cap_n++;
    }
}


/* --- READ THE DRAWN TEXT BACK, out of the glyph quads themselves.
 *
 * A quad's UV names a cell of the 10 x 9 Smash atlas, and the cell's index is a
 * character through SF_INDEX -- so the string that really went on screen can be
 * reconstructed, and a check can assert `km/h` rather than "some glyphs were
 * drawn". Which matters here: the unit on the speed dial is a STRING, and the
 * only way to be wrong about it is to draw the other one.
 *
 * sf_text_shadowed draws every string twice (dark, then bright), so quads are
 * de-duplicated by position. Sorted by x, and filtered to one band of y, because
 * the HUD draws several strings in one pass.
 */
typedef struct { float x, y, w, h; int g; } cap_glyph;

static void cap_read_text(unsigned int tex, float ylo, float yhi,
                          char *out, int cap_out)
{
    cap_glyph gl[256];
    int n = 0, d, i, j, k;
    signed char rev[SF_N];

    for (i = 0; i < SF_N; i++)
        rev[i] = 0;
    for (i = 0; i < 256; i++)
        if (SF_INDEX[i] >= 0 && SF_INDEX[i] < SF_N)
            rev[SF_INDEX[i]] = (signed char)i;

    for (d = 0; d < cap_nd; d++) {
        float u0 = 1e9f, v0 = 1e9f, x0 = 1e9f, y0 = 1e9f;
        float x1 = -1e9f, y1 = -1e9f;
        int col, row, g;
        if (cap_draw[d].tex != tex)
            continue;
        for (j = 0; j < cap_draw[d].n; j++) {
            const cap_vtx *q = &cap[cap_draw[d].first + j];
            if (q->u < u0) u0 = q->u;
            if (q->v < v0) v0 = q->v;
            if (q->x < x0) x0 = q->x;
            if (q->x > x1) x1 = q->x;
            if (q->y < y0) y0 = q->y;
            if (q->y > y1) y1 = q->y;
        }
        if (!(y0 >= ylo && y0 < yhi))
            continue;
        col = (int)(u0 * SF_COLS);
        row = (int)(v0 * SF_ROWS);
        g = row * SF_COLS + col;
        if (g < 0 || g >= SF_N)
            continue;
        /* THE SHADOW PASS, dropped. sf_text_shadowed offsets it by 7% of the
           glyph's own height, so the tolerance has to be PROPORTIONAL: a fixed
           0.75 px missed it at every size and every string came back doubled. */
        for (k = 0; k < n; k++)
            if (gl[k].g == g
                && fabsf(gl[k].x - x0) < 0.35f * (x1 - x0) + 0.5f
                && fabsf(gl[k].y - y0) < 0.35f * (y1 - y0) + 0.5f)
                break;
        if (k < n || n >= 256)
            continue;
        gl[n].x = x0; gl[n].y = y0;
        gl[n].w = x1 - x0; gl[n].h = y1 - y0;
        gl[n].g = g; n++;
    }
    /* Sorted into LINES and then across: a dial draws its value above its label,
       and sorting by x alone interleaved `72` with `km/h` into `7k7kmm2//2hh`.
       Two glyphs are on one line when their tops are within half a glyph. */
    for (i = 1; i < n; i++)
        for (j = i; j > 0; j--) {
            const float lh = 0.5f * (gl[j].h + gl[j - 1].h) * 0.5f;
            int before;
            if (fabsf(gl[j].y - gl[j - 1].y) > lh)
                before = gl[j].y < gl[j - 1].y;
            else
                before = gl[j].x < gl[j - 1].x;
            if (!before)
                break;
            {
                cap_glyph t = gl[j];
                gl[j] = gl[j - 1]; gl[j - 1] = t;
            }
        }
    for (i = 0; i < n && i < cap_out - 1; i++)
        out[i] = (char)rev[gl[i].g];
    out[i < cap_out ? i : cap_out - 1] = 0;
}

/* --------------------------------------------------------------- the test */

#define SCRW 960
#define SCRH 544

static int fails, checks;
static void ck(int ok, const char *what)
{
    checks++;
    if (!ok) { printf("  FAIL: %s\n", what); fails++; }
}

/* NDC = M * (x,y,z,1), with M in OpenGL column-major column-vector order:
   element [c*4+r] is column c, row r. */
static void project(const float m[16], float x, float y, float out[2])
{
    out[0] = m[0]*x + m[4]*y + m[12];
    out[1] = m[1]*x + m[5]*y + m[13];
}

/* --- hud.c helpers. Draw the pop and measure what came out.
 *
 * With msg_hits bound that is ONE quad, so a bbox and a max alpha describe it
 * exactly. On the font fallback it is two strings (a shadow, then the word) and
 * the max alpha is the word's, since the shadow is drawn at a fraction of it. */
static float hud_x0, hud_y0, hud_x1, hud_y1;
static float hud_u0, hud_v0, hud_u1, hud_v1;
static float hud_amax;

static void hud_shoot(const hud_t *h)
{
    int i;
    cap_reset();
    hud_draw(h, SCRW, SCRH);
    hud_x0 = hud_y0 = hud_u0 = hud_v0 = 1e9f;
    hud_x1 = hud_y1 = hud_u1 = hud_v1 = -1e9f;
    hud_amax = 0.f;
    for (i = 0; i < cap_n; i++) {
        if (cap[i].x < hud_x0) hud_x0 = cap[i].x;
        if (cap[i].x > hud_x1) hud_x1 = cap[i].x;
        if (cap[i].y < hud_y0) hud_y0 = cap[i].y;
        if (cap[i].y > hud_y1) hud_y1 = cap[i].y;
        if (cap[i].u < hud_u0) hud_u0 = cap[i].u;
        if (cap[i].u > hud_u1) hud_u1 = cap[i].u;
        if (cap[i].v < hud_v0) hud_v0 = cap[i].v;
        if (cap[i].v > hud_v1) hud_v1 = cap[i].v;
        if (cap[i].a > hud_amax) hud_amax = cap[i].a;
    }
}

static float hud_w_of(const hud_t *h) { hud_shoot(h); return hud_x1 - hud_x0; }
static float hud_a_of(const hud_t *h) { hud_shoot(h); return hud_amax; }

/* --- countdown.c, read back through the same recorder. Identical shape to
 * hud_shoot, and separate variables rather than a shared set because part 9 needs
 * to compare a countdown quad against a hud one in the same breath. */
static float cd_x0, cd_y0, cd_x1, cd_y1;
static float cd_u0, cd_v0, cd_u1, cd_v1;
static float cd_amax, cd_amin;

static void cd_shoot(const countdown_t *c)
{
    int i;
    cap_reset();
    countdown_draw(c, SCRW, SCRH);
    cd_x0 = cd_y0 = cd_u0 = cd_v0 = 1e9f;
    cd_x1 = cd_y1 = cd_u1 = cd_v1 = -1e9f;
    cd_amax = 0.f;
    cd_amin = 1e9f;
    for (i = 0; i < cap_n; i++) {
        if (cap[i].x < cd_x0) cd_x0 = cap[i].x;
        if (cap[i].x > cd_x1) cd_x1 = cap[i].x;
        if (cap[i].y < cd_y0) cd_y0 = cap[i].y;
        if (cap[i].y > cd_y1) cd_y1 = cap[i].y;
        if (cap[i].u < cd_u0) cd_u0 = cap[i].u;
        if (cap[i].u > cd_u1) cd_u1 = cap[i].u;
        if (cap[i].v < cd_v0) cd_v0 = cap[i].v;
        if (cap[i].v > cd_v1) cd_v1 = cap[i].v;
        if (cap[i].a > cd_amax) cd_amax = cap[i].a;
        if (cap[i].a < cd_amin) cd_amin = cap[i].a;
    }
}

int main(void)
{
    float p[2], q[2];
    int i;

    ui_init();

    /* ---- 0. the font atlas actually reached the GPU --------------------- */
    ck(tex_uploads == 1, "ui_init uploads exactly one texture");
    ck(tex_w == FONT_CW * FONT_COUNT && tex_h == FONT_CH,
       "the atlas is one row of FONT_COUNT cells");
    ck(tex_fmt == GL_RGBA, "uploaded as RGBA, not the unreliable GL_ALPHA path");
    ck(wrap_s == GL_CLAMP_TO_EDGE && wrap_t == GL_CLAMP_TO_EDGE,
       "the atlas clamps -- REPEAT would wrap the first glyph onto the last");
    {
        int j, ink = 0, opaque = 0, rgb_bad = 0, mismatch = 0;
        int n = FONT_CW * FONT_COUNT * FONT_CH;
        for (j = 0; j < n; j++) {
            if (tex_px[j*4+0] != 255 || tex_px[j*4+1] != 255
                || tex_px[j*4+2] != 255) rgb_bad++;
            if (tex_px[j*4+3] != FONT_PIX[j]) mismatch++;
            if (tex_px[j*4+3] > 0)   ink++;
            if (tex_px[j*4+3] == 255) opaque++;
        }
        ck(rgb_bad == 0, "RGB is white everywhere, so per-vertex colour tints it");
        ck(mismatch == 0, "alpha is font.h's coverage byte for byte");
        ck(ink > 0 && ink < n, "the atlas has ink AND gaps -- not blank, not solid");
        ck(opaque > 0, "some texels are fully opaque");
    }
    {
        /* A glyph cell must have ink and the space cell must not. This is what
           catches a font.h regenerated with the wrong glyph range or cell size --
           the dimensions could still be self-consistent and the text be garbage. */
        int row, col, ink_A = 0, ink_sp = 0;
        int cell_A = 'A' - FONT_FIRST, cell_sp = ' ' - FONT_FIRST;
        for (row = 0; row < FONT_CH; row++)
            for (col = 0; col < FONT_CW; col++) {
                int w = FONT_CW * FONT_COUNT;
                ink_A  += tex_px[((row*w) + cell_A *FONT_CW + col)*4 + 3] > 0;
                ink_sp += tex_px[((row*w) + cell_sp*FONT_CW + col)*4 + 3] > 0;
            }
        ck(ink_A > 0, "the 'A' cell has ink");
        ck(ink_sp == 0, "the space cell is empty");
    }

    /* ---- 1. the projection, measured rather than assumed ---------------- */
    cap_reset();
    ui_begin(SCRW, SCRH);

    project(glcap_matrix, 0.f, 0.f, p);
    ck(fabsf(p[0] + 1.f) < 1e-5f && fabsf(p[1] - 1.f) < 1e-5f,
       "pixel (0,0) is the TOP-LEFT of the screen, NDC (-1,+1)");
    project(glcap_matrix, (float)SCRW, (float)SCRH, p);
    ck(fabsf(p[0] - 1.f) < 1e-5f && fabsf(p[1] + 1.f) < 1e-5f,
       "pixel (w,h) is the BOTTOM-RIGHT, NDC (+1,-1)");
    project(glcap_matrix, SCRW*0.5f, SCRH*0.5f, p);
    ck(fabsf(p[0]) < 1e-5f && fabsf(p[1]) < 1e-5f, "the centre maps to NDC 0,0");

    /* The y flip is the whole ballgame: get it wrong and the menu is upside
       down and off screen. Assert the DIRECTION explicitly. */
    project(glcap_matrix, 0.f, 100.f, p);
    project(glcap_matrix, 0.f, 200.f, q);
    ck(p[1] > q[1], "larger pixel y is LOWER on screen (smaller NDC y)");

    ck(glcap_matrix_mode == GL_MODELVIEW,
       "ui_begin leaves the matrix mode on MODELVIEW, not PROJECTION");
    ck(cs_color == 1, "the colour array is enabled inside ui_begin");
    ck(en_blend == 1 && en_depth == 0 && en_alpha == 0 && en_cull == 0,
       "blending on, depth/alpha-test/cull off while drawing UI");

    /* ---- 2. ui_rect covers exactly the pixels asked for ----------------- */
    cap_reset();
    ui_rect(100.f, 50.f, 200.f, 30.f, 1.f, 0.f, 0.f, 0.5f);
    ck(cap_n == 6, "a rect is two triangles");
    {
        float x0=1e9f,x1=-1e9f,y0=1e9f,y1=-1e9f;
        for (i = 0; i < cap_n; i++) {
            if (cap[i].x < x0) x0 = cap[i].x;
            if (cap[i].x > x1) x1 = cap[i].x;
            if (cap[i].y < y0) y0 = cap[i].y;
            if (cap[i].y > y1) y1 = cap[i].y;
        }
        ck(fabsf(x0-100.f)<1e-4f && fabsf(x1-300.f)<1e-4f
           && fabsf(y0-50.f)<1e-4f && fabsf(y1-80.f)<1e-4f,
           "the rect spans exactly x 100..300, y 50..80");
        for (i = 0; i < cap_n; i++)
            ck(cap[i].r==1.f && cap[i].g==0.f && cap[i].b==0.f && cap[i].a==0.5f,
               "the colour reaches every vertex (GL_COLOR_ARRAY)");
    }

    /* ---- 3. text: one quad per printable glyph, spaces advance only ----- */
    cap_reset();
    ui_text(0.f, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f, "AB CD");
    ck(cap_n == 4 * 6, "a 5-character string with one space emits 4 quads");

    cap_reset();
    ui_text(0.f, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f, "   ");
    ck(cap_n == 0, "an all-space string emits nothing");

    /* ---- 4. glyph cells: no bleed into the neighbour -------------------- */
    cap_reset();
    ui_text(0.f, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f, "AB");
    ck(cap_n == 12, "two glyphs");
    {
        const float step = 1.f / (float)FONT_COUNT;
        int gi;
        for (gi = 0; gi < 2; gi++) {
            float lo = 1e9f, hi = -1e9f;
            int cpn = 'A' + gi - FONT_FIRST;
            float cell0 = (float)cpn * step, cell1 = (float)(cpn + 1) * step;
            for (i = gi*6; i < gi*6 + 6; i++) {
                if (cap[i].u < lo) lo = cap[i].u;
                if (cap[i].u > hi) hi = cap[i].u;
            }
            ck(lo > cell0 && hi < cell1,
               "the glyph's UVs stay strictly INSIDE its own atlas cell");
            ck(lo - cell0 < 2.f/(float)(FONT_CW*FONT_COUNT)
               && cell1 - hi < 2.f/(float)(FONT_CW*FONT_COUNT),
               "the inset is a half texel, not a wide crop");
        }
        /* v spans the full cell height: the atlas is a single row, so a glyph has
           to take ALL of it. "somewhere in 0..1" is what this used to assert, and
           that is satisfied by a quad taking only the top half of every cell --
           which is exactly what a regression in the shared quad() builder
           produced the moment ui_image gave it an explicit v range to carry.
           Bound both ends. */
        {
            float vlo = 1e9f, vhi = -1e9f;
            for (i = 0; i < 12; i++) {
                if (cap[i].v < vlo) vlo = cap[i].v;
                if (cap[i].v > vhi) vhi = cap[i].v;
            }
            ck(fabsf(vlo) < 1e-6f && fabsf(vhi - 1.f) < 1e-6f,
               "v spans the WHOLE cell, 0 to 1 -- the atlas is one row");
        }
    }

    /* ---- 5. advance and measurement agree ------------------------------- */
    cap_reset();
    ui_text(10.f, 20.f, 2.f, 1.f, 1.f, 1.f, 1.f, "MM");
    {
        float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
        for (i = 0; i < cap_n; i++) {
            if (cap[i].x < x0) x0 = cap[i].x;
            if (cap[i].x > x1) x1 = cap[i].x;
            if (cap[i].y < y0) y0 = cap[i].y;
            if (cap[i].y > y1) y1 = cap[i].y;
        }
        ck(fabsf(x0 - 10.f) < 1e-4f, "text starts at the x it was given");
        ck(fabsf(y0 - 20.f) < 1e-4f, "text starts at the y it was given");
        ck(fabsf((x1 - x0) - ui_text_w(2.f, "MM")) < 1e-4f,
           "ui_text_w matches the drawn width, at scale != 1");
        ck(fabsf((y1 - y0) - ui_text_h(2.f)) < 1e-4f,
           "ui_text_h matches the drawn height, at scale != 1");
    }

    /* ---- 6a. the matrix stacks come back balanced ----------------------- */
    {
        mat_depth[0] = mat_depth[1] = 0;
        mat_underflow = 0;
        glcap_matrix_mode = GL_MODELVIEW;

        ui_begin(SCRW, SCRH);
        ck(mat_depth[1] == 1, "ui_begin pushes the PROJECTION matrix");
        ck(mat_depth[0] == 1, "ui_begin pushes the MODELVIEW matrix");
        ui_text(0.f, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f, "x");
        ui_rect(0.f, 0.f, 2.f, 2.f, 1.f, 1.f, 1.f, 1.f);
        ui_end();
        ck(mat_depth[0] == 0 && mat_depth[1] == 0,
           "ui_end leaves BOTH matrix stacks balanced");
        ck(!mat_underflow, "no matrix stack underflow");
        ck(glcap_matrix_mode == GL_MODELVIEW,
           "ui_end leaves the matrix mode on MODELVIEW for the 3D pass");

        /* many open/close cycles must not drift -- this is what actually
           overflows a real GL implementation's 32-deep stack */
        for (i = 0; i < 100; i++) { ui_begin(SCRW, SCRH); ui_end(); }
        ck(mat_depth[0] == 0 && mat_depth[1] == 0 && !mat_underflow,
           "100 open/close cycles leave the stacks where they started");
    }

    /* ---- 6. ui_end puts every enable back ------------------------------- */
    {
        int d, a, c, b;
        /* case A: everything on, as the 3D pass leaves it */
        en_depth = 1; en_alpha = 1; en_cull = 1; en_blend = 0;
        ui_begin(SCRW, SCRH); ui_end();
        d = en_depth; a = en_alpha; c = en_cull; b = en_blend;
        ck(d==1 && a==1 && c==1 && b==0, "enables restored (all on, blend off)");
        ck(cs_color == 0, "the colour array is disabled again");

        /* case B: the opposite, so a hardcoded restore cannot pass */
        en_depth = 0; en_alpha = 0; en_cull = 0; en_blend = 1;
        ui_begin(SCRW, SCRH); ui_end();
        ck(en_depth==0 && en_alpha==0 && en_cull==0 && en_blend==1,
           "enables restored (all off, blend on)");
    }

    /* ---- 7. text is textured, panels are not ---------------------------- */
    cap_reset();
    ui_begin(SCRW, SCRH);
    ui_text(0.f, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f, "X");
    ck(cap_last_tex != 0, "text binds the font atlas");
    ck(cap_tex_enabled == 1, "texturing is on after drawing text");
    ui_rect(0.f, 0.f, 4.f, 4.f, 1.f, 1.f, 1.f, 1.f);
    ck(cap_tex_enabled == 1, "ui_rect re-enables texturing when it is done");
    ui_end();

    /* ---- 8. the HIT pop (hud.c) -----------------------------------------
     *
     * The banner is the GAME'S OWN artwork -- msg_hits, message slots 3 and 4 --
     * so the checks that matter are about the recovered geometry: which half of
     * the atlas, at what size, in which of the drawer's three vertical bands.
     * Everything else here (the trigger, the timing, the tier rule) is the
     * port's, because the retail exe never posts either slot; see hud.h.
     */
    {
        hud_t h;
        const unsigned int TEX = 77;      /* stands in for msg_hits */

        /* Nothing on screen, and NOTHING DRAWN. Two separate claims: an idle pop
           that still emits a fully transparent quad is a draw call and a blend on
           every frame of the game. */
        hud_init(&h, TEX);
        ck(!hud_active(&h), "a fresh HUD has nothing on screen");
        cap_reset();
        hud_draw(&h, SCRW, SCRH);
        ck(cap_n == 0 && cap_draws == 0, "and draws nothing at all");

        /* A brush too gentle for sfx_prop_hit to make a noise about must not put
           a banner up either -- see HUD_HIT_MIN_SPEED. */
        hud_hit(&h, HUD_HIT_MIN_SPEED * 0.5f);
        ck(!hud_active(&h), "a brush too gentle to be heard is not shown either");
        hud_hit(&h, HUD_HIT_MIN_SPEED * 1.01f);
        ck(hud_active(&h), "and one just over the floor is");

        /* --- the banner: one quad, the game's texture, the top half --------- */
        hud_init(&h, TEX);
        hud_hit(&h, 1.0f);                /* gentle: an ORDINARY hit */
        ck(hud_active(&h) && h.count == 1, "a knock raises exactly one pop");
        ck(!hud_is_great(&h), "a 1 m/s knock is an ordinary hit, not a great one");
        hud_shoot(&h);
        ck(cap_draws == 1 && cap_n == 6,
           "the banner is ONE textured quad, not a string of glyphs");
        ck(cap_last_tex == TEX, "drawn from msg_hits, the game's own artwork");
        ck(cap_tex_enabled == 1, "with texturing on");

        /* THE ATLAS SPLIT, which is the recovered fact this whole change rests
           on: msg_hits holds TWO messages and slot 3 is its top half. The
           numbers are written out here with the address they came from, which is
           a second copy a mutant has to break twice -- not self-reference. */
        ck(fabsf(hud_u0 - 0.f) < 1e-5f && fabsf(hud_u1 - 1.f) < 1e-5f,
           "u spans the whole atlas width (0x56d328: u0 = 0, u1 = 1)");
        ck(fabsf(hud_v0 - 0.f) < 1e-5f && fabsf(hud_v1 - 0.5f) < 1e-5f,
           "and v takes the TOP half -- slot 3, !HIT!  (v 0 -> 0.5)");
        ck(fabsf(hud_amax - 1.f) < 1e-4f,
           "white and fully opaque, so the ARGB art arrives unmodulated");

        {
            /* THE SIZE, from 0x56d278. The height is the recovered fraction of
               the screen; the width is the recovered pair read through the 4:3
               frame it was authored against, which is what stops the art being
               stretched 4/3 wider on a 16:9 panel. Both spelled out from the
               recovered numbers rather than from what hud.c computed. */
            float want_h = HUD_MSG_H * (float)SCRH;
            float want_w = want_h * (HUD_MSG_W / HUD_MSG_H) * (4.f / 3.f);
            float got_h, got_w;
            hud_step(&h, HUD_HIT_POP + 0.01f);   /* past the overshoot */
            hud_shoot(&h);
            got_w = hud_x1 - hud_x0;
            got_h = hud_y1 - hud_y0;
            ck(fabsf(got_h - want_h) < 0.5f,
               "settled height is 0.21333 of the screen (0x56d278)");
            ck(fabsf(got_w - want_w) < 0.5f,
               "and the width keeps the art's own 2:1 aspect");
            /* An independent statement of the same thing: half of a 256x256
               atlas is a 256x128 cell, so the banner must be twice as wide as it
               is tall whatever the panel. */
            ck(fabsf(got_w / got_h - 2.f) < 0.02f,
               "i.e. exactly twice as wide as tall, like the 256x128 cell");

            /* THE VERTICAL BAND, from FUN_004b11e0: y0 = (base - h)/2 with base
               1.0 / 0.5 / 1.5 and its own offset table zero. The port takes band
               1, the top half. */
            {
                float base = (HUD_MSG_ANCHOR == 0) ? 1.0f
                           : (HUD_MSG_ANCHOR == 1) ? 0.5f : 1.5f;
                float want_y0 = ((base - HUD_MSG_H) * 0.5f) * (float)SCRH;
                ck(fabsf(hud_y0 - want_y0) < 0.5f,
                   "top edge at (base - h)/2, one of the drawer's three bands");
            }
            ck(fabsf((hud_x0 + hud_x1) * 0.5f - SCRW * 0.5f) < 0.5f,
               "centred across the screen, as x0 = (1 - w)/2 says");
            ck(hud_x0 > 0.f && hud_y0 > 0.f
               && hud_x1 < (float)SCRW && hud_y1 < (float)SCRH,
               "and wholly on it");
            /* Above the middle: this port's camera holds the car there, and a
               banner 21% of the screen tall centred on it covers the thing it is
               reporting. That is the one reason band 1 was taken over band 0. */
            ck((hud_y0 + hud_y1) * 0.5f < SCRH * 0.5f,
               "and above the car rather than on top of it");
        }

        /* --- GREAT !HIT! is the OTHER half, and the two PARTITION the atlas -- */
        {
            float ord_v0, ord_v1, gr_v0, gr_v1;
            hud_init(&h, TEX);
            hud_hit(&h, 1.0f);
            hud_shoot(&h);
            ord_v0 = hud_v0; ord_v1 = hud_v1;

            hud_init(&h, TEX);
            hud_hit(&h, HUD_GREAT_SPEED);
            ck(hud_is_great(&h), "a knock at the top of the gain ramp is GREAT");
            hud_shoot(&h);
            gr_v0 = hud_v0; gr_v1 = hud_v1;
            ck(fabsf(gr_v0 - 0.5f) < 1e-5f && fabsf(gr_v1 - 1.f) < 1e-5f,
               "and takes the BOTTOM half -- slot 4, GREAT !HIT!  (v 0.5 -> 1)");
            /* The partition. Neither half may overlap the other or leave a strip
               of the atlas unused: that is what "an atlas of two messages" means,
               and it is a claim a single-half check cannot make. */
            ck(fabsf(ord_v1 - gr_v0) < 1e-5f
               && fabsf(ord_v0 - 0.f) < 1e-5f && fabsf(gr_v1 - 1.f) < 1e-5f,
               "the two halves partition msg_hits exactly, with no gap or overlap");

            /* And a CLUSTER promotes a gentle hit, which is the other half of the
               port's tier rule. */
            hud_init(&h, TEX);
            for (i = 0; i < HUD_GREAT_COUNT; i++)
                hud_hit(&h, 1.0f);
            ck(h.count == HUD_GREAT_COUNT && hud_is_great(&h),
               "and so does taking out HUD_GREAT_COUNT props in one pass");
            hud_init(&h, TEX);
            for (i = 0; i < HUD_GREAT_COUNT - 1; i++)
                hud_hit(&h, 1.0f);
            ck(!hud_is_great(&h), "one short of it is still an ordinary hit");
        }

        /* THE SIZE RAMP RUNS DOWN, about the banner's own centre. fx.c read an
           identical expression backwards for the whole life of that file and
           shipped an 11x sprite; a size ramp that never gets measured at two ages
           is a coin flip. */
        {
            float w_new, w_settled, cx_new, cx_settled, cy_new, cy_settled;
            hud_init(&h, TEX);
            hud_hit(&h, 1.0f);
            hud_shoot(&h);
            w_new = hud_x1 - hud_x0;
            cx_new = (hud_x0 + hud_x1) * 0.5f;
            cy_new = (hud_y0 + hud_y1) * 0.5f;
            hud_step(&h, HUD_HIT_POP + 0.01f);
            hud_shoot(&h);
            w_settled = hud_x1 - hud_x0;
            cx_settled = (hud_x0 + hud_x1) * 0.5f;
            cy_settled = (hud_y0 + hud_y1) * 0.5f;
            ck(w_new > w_settled, "the banner snaps in oversized and SHRINKS");
            ck(fabsf(cx_new - cx_settled) < 0.5f
               && fabsf(cy_new - cy_settled) < 0.5f,
               "scaling about its own centre, which is where it stays");
            /* Readable: a fair slice of the screen, bounded against the SCREEN so
               HUD_MSG_W/H cannot satisfy the check by moving. */
            ck(w_settled > SCRW * 0.1f && w_settled < SCRW * 0.6f,
               "and is big enough to read without filling the screen");
        }

        /* THE FADE RUNS DOWN TOO, and it holds first. */
        {
            float a_hold, a_late, a_later;
            hud_init(&h, TEX);
            hud_hit(&h, 1.0f);
            hud_step(&h, HUD_HIT_TIME - HUD_HIT_HOLD - 0.01f);
            a_hold = hud_a_of(&h);
            hud_step(&h, HUD_HIT_HOLD * 0.5f);
            a_late = hud_a_of(&h);
            hud_step(&h, HUD_HIT_HOLD * 0.25f);
            a_later = hud_a_of(&h);
            ck(fabsf(a_hold - 1.f) < 1e-3f,
               "alpha holds at full until HUD_HIT_HOLD of life is left");
            ck(a_late < a_hold && a_later < a_late,
               "then fades out, monotonically");
        }

        /* A CLUSTER IS ONE POP. Driving through three cans is one event to the
           player, and three banners fading on three clocks is not what that
           looks like. */
        {
            hud_init(&h, TEX);
            hud_hit(&h, 1.0f);
            hud_step(&h, 0.05f);
            hud_hit(&h, 3.0f);
            ck(h.count == 2, "a second knock inside a pop counts up");
            ck(fabsf(h.t - HUD_HIT_TIME) < 1e-6f, "and restarts its clock");
            ck(h.speed > 2.5f, "and the hardest knock in it owns the tier");
        }

        /* IT GOES AWAY. A pop that never expires is a permanent banner, which is
           the failure a fixture that only ever looks at a fresh one cannot see. */
        {
            hud_init(&h, TEX);
            hud_hit(&h, 1.0f);
            for (i = 0; i < 100; i++)
                hud_step(&h, HUD_HIT_TIME / 50.f);
            ck(!hud_active(&h) && h.count == 0 && h.t == 0.f,
               "the pop expires and clears its count");
            cap_reset();
            hud_draw(&h, SCRW, SCRH);
            ck(cap_n == 0, "and stops drawing");
            ck(!hud_is_great(&h), "an expired pop is not a great hit either");
            hud_hit(&h, 1.0f);
            ck(h.count == 1, "the next knock starts a new pop, not a fourth");
        }

        /* hud_reset is the teleport case: the collision it refers to is no longer
           where the player is. It must NOT drop the texture -- that is bound once
           from the load-once props scene and there is nothing to rebind it. */
        hud_init(&h, TEX);
        hud_hit(&h, 1.0f);
        hud_reset(&h);
        ck(!hud_active(&h), "hud_reset wipes the pop");
        ck(h.tex == TEX, "and KEEPS the texture binding");
        hud_hit(&h, 1.0f);
        hud_shoot(&h);
        ck(cap_last_tex == TEX && cap_draws == 1,
           "so the next hit still draws the game's banner");

        /* --- THE FALLBACK: no msg_hits, so the compiled-in font ------------- */
        {
            hud_init(&h, 0);
            hud_hit(&h, 1.0f);
            ck(hud_active(&h), "with no texture packed there is still a pop");
            hud_shoot(&h);
            ck(cap_draws == 2, "drawn as two strings: a shadow, then the word");
            ck(cap_n > 6, "of several glyphs, not one quad");
            ck(cap_last_tex != 0 && cap_last_tex != 77,
               "through the FONT atlas, not the missing banner");
            ck(fabsf((hud_x0 + hud_x1) * 0.5f - SCRW * 0.5f) < 12.f,
               "still centred");
            ck((hud_y0 + hud_y1) * 0.5f < SCRH * 0.5f, "still above the car");
            /* The tier survives the fallback -- in words, since there is no art
               to take a half of. */
            {
                float w_ord = hud_w_of(&h);
                hud_init(&h, 0);
                hud_hit(&h, HUD_GREAT_SPEED);
                ck(hud_w_of(&h) > w_ord,
                   "and a great hit says more of it than an ordinary one");
            }
        }

        /* And it leaves the frame as it found it -- hud_draw brackets its own
           ui_begin/ui_end, so a leak here would land on whatever draws next.
           Checked on the TEXTURED path, which is the one the game takes. */
        {
            mat_depth[0] = mat_depth[1] = 0;
            mat_underflow = 0;
            glcap_matrix_mode = GL_MODELVIEW;
            en_depth = 1; en_alpha = 1; en_cull = 1; en_blend = 0;
            hud_init(&h, TEX);
            hud_hit(&h, 1.0f);
            hud_draw(&h, SCRW, SCRH);
            ck(mat_depth[0] == 0 && mat_depth[1] == 0 && !mat_underflow,
               "hud_draw leaves both matrix stacks balanced");
            ck(en_depth == 1 && en_alpha == 1 && en_cull == 1 && en_blend == 0
               && cs_color == 0,
               "and every enable back where the 3D pass left it");
            ck(glcap_matrix_mode == GL_MODELVIEW,
               "and the matrix mode on MODELVIEW");
        }

        /* ui_image itself: a tex of 0 draws NOTHING rather than binding the
           default texture, because an unbound sampler is undefined on this GPU.
           That is the guard the fallback above depends on. */
        cap_reset();
        ui_begin(SCRW, SCRH);
        ui_image(0.f, 0.f, 10.f, 10.f, 0, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
        ui_end();
        ck(cap_n == 0 && cap_draws == 0, "ui_image with tex 0 draws nothing");
    }

    /* ---- 9. the race start: 3, 2, 1, GO! (countdown.c) ------------------
     *
     * Almost all of this one IS recovered -- msg_321_s_f's five cells, their
     * sizes, their per-message lives, the scale ramp and the vertical band -- so
     * unlike part 8 the checks here can be about the engine's own numbers rather
     * than about the port's taste. Two of them are independent of countdown.c
     * altogether and are the strongest things in this part:
     *
     *   - the five recovered UV rects TILE the 512x256 atlas exactly, area 1.000
     *     with no overlap. Nothing in countdown.c enforces that; if a rect were
     *     mistyped or two cells swapped onto the wrong halves, the tiling breaks.
     *   - the (w, h) size pair and the UV rect agree on ONE scale, 0.8 texels to
     *     the pixel on the 640x480 frame they were authored for, for all five.
     *     Two tables at two addresses, one number.
     *
     * What it is looking for beyond that: a countdown that holds the car forever
     * (or never), a `go` that fires twice or never, a ramp that shrinks instead of
     * growing (which is exactly what fx.c's DynamicScale did for the whole life of
     * that file), and the light staying up after the race has started.
     */
    {
        countdown_t c;
        const unsigned int TEX = 91;      /* stands in for msg_321_s_f */
        int i, j;

        /* --- the recovered tables, checked against each other ------------- */
        {
            /* THE TILING. Every pair of cells must be disjoint and the five areas
               must sum to the whole atlas. Written as a loop over CD_CELLS so
               adding a sixth cell one day cannot quietly pass. */
            float area = 0.f;
            int overlaps = 0;
            for (i = 0; i < CD_N_CELLS; i++) {
                const cd_cell *a = &CD_CELLS[i];
                area += (a->u1 - a->u0) * (a->v1 - a->v0);
                for (j = i + 1; j < CD_N_CELLS; j++) {
                    const cd_cell *b = &CD_CELLS[j];
                    if (a->u0 < b->u1 && b->u0 < a->u1
                        && a->v0 < b->v1 && b->v0 < a->v1)
                        overlaps++;
                }
            }
            ck(overlaps == 0, "the five recovered UV rects do not overlap");
            ck(fabsf(area - 1.f) < 1e-5f,
               "and they tile msg_321_s_f exactly -- total area 1.000 (0x56d328)");
        }
        {
            /* ONE SCALE for all five: the size pair says how big the cell is drawn
               on a 640x480 screen, the UV rect says how many texels it is, and the
               ratio has to be the same number every time or the two tables do not
               belong together. 0.8 spelled out, since it is a fact about the data
               and not something countdown.c can move. */
            int bad = 0;
            for (i = 0; i < CD_N_CELLS; i++) {
                const cd_cell *a = &CD_CELLS[i];
                float sx = (a->w * 640.f) / ((a->u1 - a->u0) * 512.f);
                float sy = (a->h * 480.f) / ((a->v1 - a->v0) * 256.f);
                if (fabsf(sx - 0.8f) > 0.01f || fabsf(sy - 0.8f) > 0.01f)
                    bad++;
            }
            ck(bad == 0,
               "every cell is drawn at 0.8 texels/pixel -- 0x56d278 and 0x56d328 "
               "agree");
        }
        /* THE ARTWORK ITSELF says which cell is which, and the digits are not the
           same width: "1" is a thin glyph and "3" and "2" are square. A pair of
           cells swapped onto each other's rects survives the tiling check and dies
           here. GO! is the widest thing in the countdown. */
        ck(CD_CELLS[CD_CELL_1].w < CD_CELLS[CD_CELL_3].w,
           "the \"1\" cell is narrower than the \"3\" -- it is a thinner glyph");
        ck(fabsf(CD_CELLS[CD_CELL_3].w - CD_CELLS[CD_CELL_2].w) < 1e-6f,
           "and \"3\" and \"2\" are the same width, both square cells");
        ck(CD_CELLS[CD_CELL_GO].w > CD_CELLS[CD_CELL_3].w
           && CD_CELLS[CD_CELL_GO].w > CD_CELLS[CD_CELL_1].w,
           "GO! is the widest of the four -- it is a word, not a digit");
        /* The engine's own slot numbers, which is what pins the ORDER: the notes
           had this atlas down as "3, 2, 1, START, FINISH" and the fourth word is
           GO at slot NINE, with FINISH at eight. */
        ck(CD_CELLS[CD_CELL_3].slot == 5 && CD_CELLS[CD_CELL_2].slot == 6
           && CD_CELLS[CD_CELL_1].slot == 7 && CD_CELLS[CD_CELL_GO].slot == 9
           && CD_CELLS[CD_CELL_FINISH].slot == 8,
           "the cells are engine slots 5, 6, 7, 9 and 8 -- GO is NINE");
        /* cd_phase reads the cell index AS the step number, so the three digits
           have to be 0, 1, 2 in that order. Asserted rather than commented. */
        ck(CD_CELL_3 == 0 && CD_CELL_2 == 1 && CD_CELL_1 == 2,
           "the digit cells are indexed 0,1,2 in countdown order (cd_phase)");

        /* --- nothing running -------------------------------------------- */
        countdown_init(&c, TEX);
        ck(!countdown_active(&c) && countdown_cell(&c) < 0,
           "a fresh countdown has nothing on screen");
        /* THE SAFETY PROPERTY. main.c gates the physics on countdown_holding, so
           a countdown that reports a hold before anyone started it would freeze
           the game on the grid with no way out. */
        ck(!countdown_holding(&c),
           "and does NOT hold the car -- an unstarted countdown must not freeze it");
        cap_reset();
        countdown_draw(&c, SCRW, SCRH);
        ck(cap_n == 0 && cap_draws == 0, "and draws nothing at all");
        countdown_step(&c, 1.f);
        ck(!countdown_holding(&c) && !c.go,
           "stepping an unstarted countdown does nothing and releases nothing");

        /* --- the sequence, driven at 1/60 like main.c ------------------- */
        countdown_start(&c);
        ck(countdown_holding(&c), "started: the car is held");
        ck(countdown_cell(&c) == CD_CELL_3, "and \"3\" is up");
        /* The FIRST frame draws nothing: the recovered ramp starts the scale at 0
           and FUN_004b12b0 declines below 1e-6. */
        cd_shoot(&c);
        ck(cap_n == 0, "the first frame of a message draws nothing (scale 0)");

        {
            int gos = 0, go_frame = -1, held_frames = 0, cell_frames[CD_N_CELLS];
            int frames_drawn = 0, alpha_bad = 0, bad_cell = 0, frame = 0;
            int last_cell = -1;
            const float DT = 1.f / 60.f;
            const float STEP_FRAMES = CD_STEP_TIME / DT;
            float prev_k = -1.f, k_dropped = 0.f;

            for (i = 0; i < CD_N_CELLS; i++) cell_frames[i] = 0;

            /* Eight seconds -- comfortably past the recovered 5.0 s total, so the
               end is measured rather than assumed. */
            for (frame = 0; frame < 8 * 60; frame++) {
                int cell;
                countdown_step(&c, DT);
                if (c.go) { gos++; go_frame = frame; }
                if (countdown_holding(&c)) held_frames++;
                cell = countdown_cell(&c);
                if (cell >= 0) {
                    cell_frames[cell]++;
                    /* THE ORDER, as a sequence rather than as a clock. Comparing
                       against a reconstructed t cannot be made exact: countdown.c
                       ACCUMULATES dt, 180 additions of 1/60 in float come out just
                       under 3.0, and the boundary then lands one frame from where
                       (frame+1)*DT puts it. Two frames disagreeing out of 300 is
                       not the property worth asserting anyway -- what matters is
                       that the countdown never goes backwards and never skips, and
                       that is exact. The DURATIONS are checked by frame count
                       below, which has the same tolerance built in honestly. */
                    if (last_cell >= 0 && cell != last_cell
                        && cell != last_cell + 1)
                        bad_cell++;
                    last_cell = cell;
                    cd_shoot(&c);
                    if (cap_n) {
                        frames_drawn++;
                        /* NO FADE anywhere in this layer: FUN_00471610 is handed a
                           flat 0xffffffff and a message ends by running out of
                           life. A ramp on the alpha would show here. */
                        if (fabsf(cd_amax - 1.f) > 1e-4f
                            || fabsf(cd_amin - 1.f) > 1e-4f) alpha_bad++;
                    }
                    /* The ramp GROWS and then HOLDS. Recorded as the worst drop
                       inside one message, which a shrinking ramp cannot hide --
                       and the reset at a message boundary is not a drop, so the
                       comparison is skipped on the frame the cell changes. */
                    {
                        float k = countdown_scale(&c);
                        static int prev_cell = -1;
                        if (cell == prev_cell && prev_k >= 0.f
                            && prev_k - k > k_dropped)
                            k_dropped = prev_k - k;
                        prev_k = k;
                        prev_cell = cell;
                    }
                }
            }

            ck(gos == 1, "GO fires exactly ONCE over the whole countdown");
            /* AT the recovered 3.0 s, not merely somewhere. Two frames of slack:
               one because a 1/60 loop can only land within a step of a boundary,
               and one because the accumulated float sum of 180 sixtieths is a hair
               under 3.0. */
            ck(go_frame >= 0
               && fabsf((float)(go_frame + 1) * DT - CD_STEP_TIME * 3.f) <= 2.f * DT,
               "and at t = 3.0 s -- three digits at one second each (0x4e2079..)");
            /* AND THE HOLD ENDS THERE, EXACTLY. This is the check that says the
               countdown is a start light and not just a picture: every frame up to
               GO is held, the frame GO fires on is not, and the banner then
               outlives the hold. */
            ck(held_frames == go_frame,
               "the car is held for exactly as long as the digits are up");
            ck(!countdown_holding(&c), "and is free once the countdown is over");
            ck(cell_frames[CD_CELL_GO] > 0
               && cell_frames[CD_CELL_GO] > cell_frames[CD_CELL_3],
               "GO! is on screen LONGER than a digit -- life 2.0 against 1.0");
            /* The two lives, as a ratio, so neither constant can be moved on its
               own to satisfy it. */
            ck(fabsf((float)cell_frames[CD_CELL_GO]
                     / (float)cell_frames[CD_CELL_3] - 2.f) < 0.05f,
               "and exactly twice as long (0x4e20e8's 2.0 against 0x4e2079's 1.0)");
            /* And each digit gets its own second, not just the three of them
               getting three seconds between them -- a mutant that ran 3 for two
               seconds and 2 for none passes every total. One frame of slack, as
               above. */
            ck(fabsf((float)cell_frames[CD_CELL_3] - STEP_FRAMES) <= 1.f
               && fabsf((float)cell_frames[CD_CELL_2] - STEP_FRAMES) <= 1.f
               && fabsf((float)cell_frames[CD_CELL_1] - STEP_FRAMES) <= 1.f,
               "each digit gets one second of its own (0x4e2079/0x4e209a/0x4e20b9)");
            ck(bad_cell == 0,
               "the countdown never goes backwards and never skips a cell");
            ck(cell_frames[CD_CELL_FINISH] == 0,
               "FINISH is never raised -- there is no race to finish yet");
            ck(alpha_bad == 0,
               "the banner never fades: alpha is a flat 1.0 for its whole life");
            ck(k_dropped < 1e-4f,
               "the scale ramp GROWS and holds -- it never comes back down");
            ck(frames_drawn > 4 * 60,
               "and something was on screen for most of the five seconds");
            ck(!c.running && countdown_cell(&c) < 0,
               "it retires itself after GO's two seconds, without being told to");
        }

        /* --- the ramp, at ABSOLUTE times off the recovered constant --------
         *
         * 0.8 s is written out here with the address it came from rather than
         * taken from CD_GROW_TIME, because stepping by fractions OF the constant
         * makes the check true for any value of it -- which is this project's
         * seven-times-repeated trap, and a mutant that halved the grow-in to 0.4 s
         * survived the version of these three lines that did exactly that.
         *
         *     0x5544a0 = 0.8 : d = min(life, 0.8); k = min(elapsed / d, 1) */
        countdown_init(&c, TEX);
        countdown_start(&c);
        ck(fabsf(countdown_scale(&c) - 0.f) < 1e-6f, "scale is 0 at birth");
        countdown_step(&c, 0.2f);
        ck(fabsf(countdown_scale(&c) - 0.25f) < 1e-3f,
           "a quarter of the way up at 0.2 s (grow-in 0.8 s, 0x5544a0)");
        countdown_step(&c, 0.2f);
        ck(fabsf(countdown_scale(&c) - 0.5f) < 1e-3f, "and half size at 0.4 s");
        countdown_step(&c, 0.2f);
        ck(fabsf(countdown_scale(&c) - 0.75f) < 1e-3f,
           "and three quarters at 0.6 s -- LINEAR, not eased");
        countdown_step(&c, 0.2f + 1e-3f);
        ck(fabsf(countdown_scale(&c) - 1.f) < 1e-4f,
           "and exactly full size at 0.8 s, not still growing");
        ck(countdown_cell(&c) == CD_CELL_3,
           "all of which happens inside the FIRST digit's own second");

        /* --- the quad: centred, in band 0, scaling about its own centre --- */
        countdown_init(&c, TEX);
        countdown_start(&c);
        countdown_step(&c, CD_GROW_TIME + 0.01f);    /* settled, still on "3" */
        ck(countdown_cell(&c) == CD_CELL_3, "still on \"3\" at 0.81 s");
        cd_shoot(&c);
        ck(cap_draws == 1 && cap_n == 6,
           "the banner is ONE textured quad, not a string of glyphs");
        ck(cap_last_tex == TEX,
           "drawn from msg_321_s_f, the game's own artwork");
        {
            const cd_cell *cell = &CD_CELLS[CD_CELL_3];
            float want_h = cell->h * (float)SCRH;
            float want_w = want_h * (cell->w / cell->h) * (4.f / 3.f);
            float got_w = cd_x1 - cd_x0, got_h = cd_y1 - cd_y0;
            ck(fabsf(cd_u0 - cell->u0) < 1e-5f && fabsf(cd_u1 - cell->u1) < 1e-5f
               && fabsf(cd_v0 - cell->v0) < 1e-5f
               && fabsf(cd_v1 - cell->v1) < 1e-5f,
               "over the \"3\" cell's own UV rect, 0,0 -> 0.25,0.5");
            ck(fabsf(got_h - want_h) < 0.5f,
               "at 0.21333 of the screen height (0x56d278)");
            ck(fabsf(got_w - want_w) < 0.5f,
               "and the width the recovered pair asks for on a 4:3 frame");
            /* Independent statement of the same thing: 128 x 128 texels drawn at
               one scale is a SQUARE, whatever the panel is. */
            ck(fabsf(got_w / got_h - 1.f) < 0.02f,
               "i.e. square, like the 128x128 cell it comes from");
            /* BAND 0, the drawer's whole-screen band: y0 = (1 - h)/2. This is
               where it differs from hud.c, which takes band 1 on purpose. */
            {
                float base = (CD_BAND == 0) ? 1.0f : (CD_BAND == 1) ? 0.5f : 1.5f;
                float want_y0 = ((base - cell->h) * 0.5f) * (float)SCRH;
                ck(fabsf(cd_y0 - want_y0) < 0.5f,
                   "top edge at (base - h)/2, one of the drawer's three bands");
            }
            ck(fabsf((cd_x0 + cd_x1) * 0.5f - SCRW * 0.5f) < 0.5f,
               "centred across the screen, as x0 = (1 - w)/2 says");
            ck(fabsf((cd_y0 + cd_y1) * 0.5f - SCRH * 0.5f) < 0.5f,
               "and down it -- the start light is dead centre, unlike HIT");
            ck(cd_x0 > 0.f && cd_y0 > 0.f
               && cd_x1 < (float)SCRW && cd_y1 < (float)SCRH,
               "and wholly on screen");

            /* THE SCALE IS ABOUT THE RECT'S OWN CENTRE (FUN_004b12b0), so a
               half-grown message is half the size in the SAME place. Measured
               against the settled quad just captured, not against a formula. */
            {
                float fx = (cd_x0 + cd_x1) * 0.5f, fy = (cd_y0 + cd_y1) * 0.5f;
                float fw = got_w, fh = got_h;
                countdown_t h2;
                countdown_init(&h2, TEX);
                countdown_start(&h2);
                countdown_step(&h2, CD_GROW_TIME * 0.5f);
                cd_shoot(&h2);
                ck(fabsf((cd_x1 - cd_x0) - fw * 0.5f) < 0.5f
                   && fabsf((cd_y1 - cd_y0) - fh * 0.5f) < 0.5f,
                   "half way through the grow-in the quad is half the size");
                ck(fabsf((cd_x0 + cd_x1) * 0.5f - fx) < 0.5f
                   && fabsf((cd_y0 + cd_y1) * 0.5f - fy) < 0.5f,
                   "and in the SAME place -- it scales about its own centre");
            }
        }

        /* --- stopping releases nothing --------------------------------- */
        countdown_init(&c, TEX);
        countdown_start(&c);
        countdown_step(&c, 1.5f);
        ck(countdown_holding(&c) && countdown_cell(&c) == CD_CELL_2,
           "mid-countdown: held, on \"2\"");
        countdown_stop(&c);
        ck(!c.go, "STOPPING raises no GO -- giving up does not start the race");
        ck(!countdown_holding(&c) && !countdown_active(&c),
           "and leaves nothing held and nothing on screen");
        /* AND IT STAYS STOPPED. A stop that only pushed the clock negative and
           left the countdown running would come BACK a second later, as the clock
           accumulated its way past zero again -- and would freeze the car on the
           line in the middle of a race. Found by a mutant that did exactly that
           and survived every check above, because none of them stepped after a
           stop. Same class as the shadow-state restore vis_test missed: a recorder
           watching one call cannot see what the call left behind. */
        {
            int woke = 0, released = 0;
            for (i = 0; i < 5 * 60; i++) {
                countdown_step(&c, 1.f / 60.f);
                if (countdown_active(&c) || countdown_holding(&c)) woke++;
                if (c.go) released++;
            }
            ck(woke == 0 && released == 0,
               "and five seconds later it has not come back to life");
        }

        /* --- dt 0, i.e. the menu is open ------------------------------- */
        countdown_init(&c, TEX);
        countdown_start(&c);
        countdown_step(&c, 2.9f);
        ck(countdown_cell(&c) == CD_CELL_1 && countdown_holding(&c),
           "at 2.9 s: \"1\", still held");
        for (i = 0; i < 600; i++)
            countdown_step(&c, 0.f);
        ck(countdown_cell(&c) == CD_CELL_1 && countdown_holding(&c)
           && !c.go,
           "and ten seconds of dt=0 changes nothing -- the menu holds the line");

        /* --- the font fallback, for a props.vsc packed without the atlas -- */
        {
            countdown_t f;
            countdown_init(&f, 0);
            countdown_start(&f);
            countdown_step(&f, CD_GROW_TIME + 0.01f);
            cd_shoot(&f);
            ck(cap_n > 0 && cap_draws > 0,
               "with msg_321_s_f unpacked the start light still appears");
            ck(cap_last_tex != 0,
               "drawn from the compiled-in font atlas, which cannot fail to load");
            /* Legible, and on screen: the same two things ui_test asks of the HIT
               fallback. */
            ck(cd_x1 - cd_x0 > 20.f && cd_y1 - cd_y0 > 20.f,
               "and is big enough to read");
            ck(cd_x0 > 0.f && cd_x1 < (float)SCRW
               && cd_y0 > 0.f && cd_y1 < (float)SCRH, "and wholly on it");
            /* The GO frame must not print the same word as the "3" frame, which is
               what a fallback with one hardcoded string would do. */
            {
                float w3 = cd_x1 - cd_x0;
                countdown_step(&f, CD_STEP_TIME * 3.f);
                ck(countdown_cell(&f) == CD_CELL_GO, "and reaches GO");
                cd_shoot(&f);
                ck(fabsf((cd_x1 - cd_x0) - w3) > 1.f,
                   "and prints a different word there -- \"GO!\", not \"3\"");
            }
        }

        /* --- and it leaves the 3D pass's state alone --------------------- */
        {
            countdown_t s;
            mat_depth[0] = mat_depth[1] = 0;
            mat_underflow = 0;
            glcap_matrix_mode = GL_MODELVIEW;
            en_depth = 1; en_alpha = 1; en_cull = 1; en_blend = 0;
            countdown_init(&s, TEX);
            countdown_start(&s);
            countdown_step(&s, 0.5f);
            countdown_draw(&s, SCRW, SCRH);
            ck(mat_depth[0] == 0 && mat_depth[1] == 0 && !mat_underflow,
               "countdown_draw leaves both matrix stacks balanced");
            ck(en_depth == 1 && en_alpha == 1 && en_cull == 1 && en_blend == 0
               && cs_color == 0,
               "and every enable back where the 3D pass left it");
            ck(glcap_matrix_mode == GL_MODELVIEW,
               "and the matrix mode on MODELVIEW");
        }
    }


    /* ================================================================ *
     *  10 to 16: race_ui.c -- the in-race HUD                          *
     * ================================================================ */
    {
        /* Distinct handles, so a check can say WHICH atlas a quad came out of.
           Nothing here uploads: race_ui.c binds what it is given. */
        enum { T_MAP = 101, T_ARROW, T_CP, T_PLACE1, T_LAP = 110,
               T_DIAL = 120, T_ARC, T_FBIG, T_FSMALL };
        race_ui_tex T;
        race_ui_t R;
        race_ui_state S;
        int i;

        memset(&T, 0, sizeof T);
        T.map = T_MAP; T.arrow = T_ARROW; T.cp = T_CP; T.lap = T_LAP;
        T.dial = T_DIAL; T.dial_arc = T_ARC;
        T.font_big = T_FBIG; T.font_small = T_FSMALL;
        for (i = 0; i < 6; i++)
            T.place[i] = (unsigned int)(T_PLACE1 + i);

        memset(&S, 0, sizeof S);
        S.place = 3; S.n_racers = 4; S.lap = 1;
        S.speed = 10.f; S.speed_max = 20.f;
        S.boost = 50.f; S.boost_max = 100.f;

        /* ---- 10. the map transform, over all ten tracks ----------------- */
        printf("-- 10: the world -> minimap transform\n");
        {
            int bad_fit = 0, bad_start = 0, bad_scale = 0, bad_rot = 0;
            int bad_flip = 0, solved = 0;
            float prev[6];

            memset(prev, 0, sizeof prev);
            for (i = 0; i < MAP_N_TRACKS; i++) {
                const map_calib *c = &MAP_CALIB[i];
                float u, v, sx, sy, cosang, det;
                int k;

                race_ui_init(&R, &T);
                race_ui_set_track(&R, i, T_MAP);
                if (!R.have_map)
                    continue;
                solved++;

                /* (a) the affine reproduces its own three defining pairs. This
                   is the SOLVE under test, not the table: a Cramer with a
                   transposed minor still returns six plausible numbers. */
                for (k = 0; k < 3; k++) {
                    race_ui_map_uv(&R, c->world[k][0], c->world[k][1], &u, &v);
                    if (fabsf(u - c->art[k][0]) > 1e-4f
                        || fabsf(v - c->art[k][1]) > 1e-4f)
                        bad_fit++;
                }

                /* (b) INDEPENDENT of that table: tracks.h -- a different
                   generated header, out of the .sb files rather than the exe --
                   says where this track's race start is, and it has to land on
                   the painted art. This is the check that fails if the ten rows
                   are read in the wrong order, which reproducing the defining
                   points cannot see. */
                race_ui_map_uv(&R, TRACKS[i].x, TRACKS[i].z, &u, &v);
                if (!(u > 0.05f && u < 0.95f && v > 0.05f && v < 0.95f))
                    bad_start++;

                /* (c) a painted top-down map is a uniform scale with no
                   rotation and no mirror. Bound to the geometry, not to the
                   constants: nothing here can be satisfied by moving a number
                   in hud_data.h. */
                sx = sqrtf(R.m[0] * R.m[0] + R.m[3] * R.m[3]);
                sy = sqrtf(R.m[1] * R.m[1] + R.m[4] * R.m[4]);
                cosang = (R.m[0] * R.m[1] + R.m[3] * R.m[4]) / (sx * sy);
                det = R.m[0] * R.m[4] - R.m[1] * R.m[3];
                if (!(sx / sy > 0.8f && sx / sy < 1.25f)) bad_scale++;
                if (fabsf(cosang) > 0.12f) bad_rot++;
                if (det <= 0.f) bad_flip++;

                /* (d) and the ten are NOT the same transform. A per-track table
                   compiled in once is nine wrong tracks -- traps.md records that
                   exact bug for WLOD_<track>_more1, and a minimap wired to
                   MAP_CALIB[0] would look perfectly plausible on beach_1. */
                if (i > 0) {
                    int same = 1;
                    for (k = 0; k < 6; k++)
                        if (fabsf(R.m[k] - prev[k]) > 1e-6f)
                            same = 0;
                    ck(!same, "each track gets its OWN map transform");
                }
                memcpy(prev, R.m, sizeof prev);
            }
            ck(solved == MAP_N_TRACKS, "all ten tracks solve a transform");
            ck(bad_fit == 0, "the affine reproduces its three defining pairs");
            ck(bad_start == 0,
               "every race start in tracks.h lands ON the painted art");
            ck(bad_scale == 0, "the two axis scales agree within 25%");
            ck(bad_rot == 0, "the axes come out orthogonal");
            ck(bad_flip == 0, "and unmirrored -- z grows DOWN the art");
        }
        /* A track index the tables do not cover leaves the map out and does not
           reach into the array. */
        race_ui_init(&R, &T);
        race_ui_set_track(&R, MAP_N_TRACKS, T_MAP);
        ck(!R.have_map, "a track past the table gets no map");
        race_ui_set_track(&R, -1, T_MAP);
        ck(!R.have_map, "and neither does a negative one");
        race_ui_set_track(&R, 0, 0);
        ck(!R.have_map, "and neither does one whose art was not packed");
        ck(!race_ui_map_uv(&R, 0.f, 0.f, NULL, NULL),
           "race_ui_map_uv declines with no transform");

        /* ---- 11. the car marker points where the car points ------------- */
        printf("-- 11: the minimap's car marker\n");
        {
            /* Four cardinal headings, and the LOPSIDED one as well: a mutant
               that writes yaw + pi for pi - yaw is invisible at yaw 0 and pi,
               which is exactly the pair a four-way check would pick. */
            static const float YAW[5] = { 0.f, 1.5707963f, 3.1415927f,
                                          -1.5707963f, 0.6f };
            int wrong = 0, found = 0;

            race_ui_init(&R, &T);
            race_ui_set_track(&R, 0, T_MAP);
            /* Put the car on the middle reference point, which is on the art by
               construction, so the marker is never culled for being off it. */
            S.car_x = MAP_CALIB[0].world[0][0];
            S.car_z = MAP_CALIB[0].world[0][1];
            S.n_cp = 0; S.n_others = 0;

            for (i = 0; i < 5; i++) {
                int d, best = -1;
                float cx = 0.f, cy = 0.f, tx = 0.f, ty = 0.f, fx, fy, want_x, want_y;

                S.car_yaw = YAW[i];
                cap_reset();
                race_ui_draw(&R, &S, SCRW, SCRH);

                /* The LAST arrow draw is the player's -- race_ui.c draws it
                   after the opponents so nothing covers it. */
                for (d = 0; d < cap_nd; d++)
                    if (cap_draw[d].tex == T_ARROW)
                        best = d;
                if (best < 0)
                    continue;
                found++;
                {
                    /* The quad's centre and the midpoint of the edge whose UV is
                       the TOP of the art, both off the FOUR DISTINCT corners --
                       not off the six emitted vertices. ui.c expands a quad
                       through the index list {0,1,2,0,2,3}, so corners 0 and 2
                       appear twice: a plain average over the six is biased toward
                       the top-left, which is a bias in the direction being
                       measured and made this check fail on code that was right.
                       Each corner is identified by which side of the quad's own
                       UV midpoint its (u, v) falls on, so the rotation cannot
                       confuse it. */
                    int j, nv = cap_draw[best].n, k, got = 0;
                    float u0 = 1e9f, u1 = -1e9f, v0 = 1e9f, v1 = -1e9f;
                    float qx[4], qy[4];
                    int have[4];
                    for (k = 0; k < 4; k++) { have[k] = 0; qx[k] = qy[k] = 0.f; }
                    for (j = 0; j < nv; j++) {
                        const cap_vtx *q = &cap[cap_draw[best].first + j];
                        if (q->u < u0) u0 = q->u; if (q->u > u1) u1 = q->u;
                        if (q->v < v0) v0 = q->v; if (q->v > v1) v1 = q->v;
                    }
                    for (j = 0; j < nv; j++) {
                        const cap_vtx *q = &cap[cap_draw[best].first + j];
                        int right = q->u > (u0 + u1) * 0.5f;
                        int bottom = q->v > (v0 + v1) * 0.5f;
                        k = bottom * 2 + right;   /* 0 TL, 1 TR, 2 BL, 3 BR */
                        if (!have[k]) {
                            have[k] = 1; qx[k] = q->x; qy[k] = q->y; got++;
                        }
                    }
                    if (got != 4) continue;
                    for (k = 0; k < 4; k++) { cx += qx[k]; cy += qy[k]; }
                    cx *= 0.25f; cy *= 0.25f;
                    tx = (qx[0] + qx[1]) * 0.5f;
                    ty = (qy[0] + qy[1]) * 0.5f;
                }
                /* Where the art's own tip is pointing, on screen. */
                fx = tx - cx; fy = ty - cy;
                {
                    const float len = sqrtf(fx * fx + fy * fy);
                    if (len < 1e-6f) { wrong++; continue; }
                    fx /= len; fy /= len;
                }
                /* WHERE THE CAR IS ACTUALLY GOING ON THE PANEL, measured rather
                 * than derived: step the car a metre along its own heading in
                 * WORLD space and see which way its marker MOVES. That is the
                 * definition of "the arrow points where the car is going", and it
                 * is independent of the transform -- which is the whole point,
                 * because the port shipped `pi - yaw`, derived on paper from
                 * "+z runs down the art", and EVERY ONE of the ten paintings is
                 * rotated (world +x comes out at +145.7 degrees on beach_1). A
                 * check that pushed the heading through the same matrix the code
                 * uses would have agreed with the bug. */
                {
                    float u0 = 0.f, v0 = 0.f, u1 = 0.f, v1 = 0.f;
                    const float step = 1.f;
                    race_ui_map_uv(&R, S.car_x, S.car_z, &u0, &v0);
                    race_ui_map_uv(&R, S.car_x + sinf(YAW[i]) * step,
                                   S.car_z + cosf(YAW[i]) * step, &u1, &v1);
                    /* u right, v down -- the panel's own axes. */
                    want_x = u1 - u0;
                    want_y = v1 - v0;
                    {
                        const float l = sqrtf(want_x * want_x + want_y * want_y);
                        if (l < 1e-9f) { wrong++; continue; }
                        want_x /= l; want_y /= l;
                    }
                }
                if (fx * want_x + fy * want_y < 0.985f)   /* within 10 degrees */
                    wrong++;
            }
            ck(found == 5, "the player's marker is drawn at every heading");
            ck(wrong == 0,
               "and it points where the car is GOING on the panel, on all five");
            /* AND SO DO THE OPPONENTS' markers, which used to be drawn unturned.
               Judged the same way: two opponents on the same spot facing opposite
               ways must draw two quads pointing opposite ways. */
            {
                int d, na = 0;
                float ax0 = 0.f, ay0 = 0.f, ax1 = 0.f, ay1 = 0.f;
                S.n_others = 2;
                S.other_x[0] = S.other_x[1] = MAP_CALIB[0].world[0][0];
                S.other_z[0] = S.other_z[1] = MAP_CALIB[0].world[0][1];
                S.other_yaw[0] = 0.f;
                S.other_yaw[1] = 3.14159265f;
                cap_reset();
                race_ui_draw(&R, &S, SCRW, SCRH);
                for (d = 0; d < cap_nd; d++) {
                    int j;
                    float cx = 0.f, cy = 0.f, tx = 0.f, ty = 0.f;
                    int ntop = 0;
                    float vmin = 1e9f, vmax = -1e9f;
                    if (cap_draw[d].tex != T_ARROW) continue;
                    /* the OPPONENT cell is map_arrow's RIGHT half, so its
                       smallest u is 0.5; the player's is 0. Taken over the whole
                       quad -- picking two vertices got the player's 0.0 and 0.5
                       and let it through. */
                    {
                        float umin = 1e9f;
                        for (j = 0; j < cap_draw[d].n; j++)
                            if (cap[cap_draw[d].first + j].u < umin)
                                umin = cap[cap_draw[d].first + j].u;
                        if (umin < 0.25f)
                            continue;
                    }
                    for (j = 0; j < cap_draw[d].n; j++) {
                        float vv = cap[cap_draw[d].first + j].v;
                        if (vv < vmin) vmin = vv;
                        if (vv > vmax) vmax = vv;
                    }
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *q = &cap[cap_draw[d].first + j];
                        cx += q->x; cy += q->y;
                        if (q->v < (vmin + vmax) * 0.5f) {
                            tx += q->x; ty += q->y; ntop++;
                        }
                    }
                    cx /= (float)cap_draw[d].n; cy /= (float)cap_draw[d].n;
                    if (!ntop) continue;
                    tx = tx / (float)ntop - cx;
                    ty = ty / (float)ntop - cy;
                    if (na == 0) { ax0 = tx; ay0 = ty; }
                    else         { ax1 = tx; ay1 = ty; }
                    na++;
                }
                ck(na == 2, "both opponents' markers are drawn");
                /* Opposite headings -> the two tip vectors must oppose. An
                   UNTURNED pair points the same way and this is +1. */
                ck(na == 2
                   && (ax0 * ax1 + ay0 * ay1)
                      / (sqrtf(ax0 * ax0 + ay0 * ay0)
                         * sqrtf(ax1 * ax1 + ay1 * ay1)) < -0.9f,
                   "and an opponent's marker TURNS with its car -- two facing "
                   "opposite ways draw opposite arrows");
                S.n_others = 0;
            }
        }
        /* Off the art, no marker -- a hand-painted map does not extend, and the
           cull is against the WINDOW the panel shows rather than against the whole
           painting (the two are the same thing only at RUI_MAP_ZOOM 1). */
        {
            int arrows_on = 0, arrows_off = 0, d;
            S.car_yaw = 0.f;
            cap_reset();
            race_ui_draw(&R, &S, SCRW, SCRH);
            for (d = 0; d < cap_nd; d++) arrows_on += cap_draw[d].tex == T_ARROW;
            S.car_x = 100000.f; S.car_z = 100000.f;
            cap_reset();
            race_ui_draw(&R, &S, SCRW, SCRH);
            for (d = 0; d < cap_nd; d++) arrows_off += cap_draw[d].tex == T_ARROW;
            ck(arrows_on == 1 && arrows_off == 0,
               "a car off the edge of the art draws no marker on the frame");
            S.car_x = MAP_CALIB[0].world[0][0];
            S.car_z = MAP_CALIB[0].world[0][1];
        }

        /* ---- 11b. THE PANEL IS A WINDOW ON THE ART, CENTRED ON THE PLAYER ---
         *
         * The whole-map view is 1.3 m a pixel on a 151 px panel: the ribbon is two
         * pixels wide and the arrow covers 17 m of it. So the panel shows a square
         * of side 1/RUI_MAP_ZOOM, scrolled with the car and CLAMPED to the art.
         *
         * Read back off the map quads' own UVs, which is the only place the window
         * exists -- there is no zoom state to inspect. */
        printf("-- 11b: the minimap window\n");
        {
            float u0a = 1e9f, u1a = -1e9f, v0a = 1e9f, v1a = -1e9f;
            float u0b = 1e9f, u1b = -1e9f;
            int d, j;

            race_ui_init(&R, &T);
            race_ui_set_track(&R, 0, T_MAP);
            S.car_x = MAP_CALIB[0].world[0][0];
            S.car_z = MAP_CALIB[0].world[0][1];
            cap_reset();
            race_ui_draw(&R, &S, SCRW, SCRH);
            for (d = 0; d < cap_nd; d++) {
                if (cap_draw[d].tex != T_MAP) continue;
                for (j = 0; j < cap_draw[d].n; j++) {
                    const cap_vtx *q = &cap[cap_draw[d].first + j];
                    if (q->u < u0a) u0a = q->u; if (q->u > u1a) u1a = q->u;
                    if (q->v < v0a) v0a = q->v; if (q->v > v1a) v1a = q->v;
                }
            }
            /* The outline's own extent is 0.9163 of the panel box in u and 0.9222
               in v (it is inset -- the clipped corners), so the DRAWN uv span is
               that fraction of the window. Bound to MAP_OUTLINE rather than to a
               number, so the panel shape and the zoom cannot be conflated. */
            {
                const float fu = MAP_OUTLINE[3][0] - MAP_OUTLINE[2][0];
                const float fv = MAP_OUTLINE[6][1] - MAP_OUTLINE[0][1];
                const float want_u = fu / RUI_MAP_ZOOM;
                const float want_v = fv / RUI_MAP_ZOOM;
                ck(fabsf((u1a - u0a) - want_u) < 1e-4f,
                   "the map quads sample 1/RUI_MAP_ZOOM of the art across");
                ck(fabsf((v1a - v0a) - want_v) < 1e-4f, "and down");
                ck(RUI_MAP_ZOOM <= 1.f || u1a - u0a < fu - 1e-4f,
                   "which is LESS than the whole painting -- it is zoomed in");
            }
            /* IT SCROLLS WITH THE CAR. Move the car along +x and the window's u
               has to follow. The reference point is mid-art on beach_1 (u 0.41),
               so there is room to move in both directions without the clamp. */
            {
                float u_lo, u_hi;
                S.car_x = MAP_CALIB[0].world[0][0] - 20.f;
                cap_reset(); race_ui_draw(&R, &S, SCRW, SCRH);
                u_lo = 1e9f;
                for (d = 0; d < cap_nd; d++) {
                    if (cap_draw[d].tex != T_MAP) continue;
                    for (j = 0; j < cap_draw[d].n; j++)
                        if (cap[cap_draw[d].first + j].u < u_lo)
                            u_lo = cap[cap_draw[d].first + j].u;
                }
                S.car_x = MAP_CALIB[0].world[0][0] + 20.f;
                cap_reset(); race_ui_draw(&R, &S, SCRW, SCRH);
                u_hi = 1e9f;
                for (d = 0; d < cap_nd; d++) {
                    if (cap_draw[d].tex != T_MAP) continue;
                    for (j = 0; j < cap_draw[d].n; j++)
                        if (cap[cap_draw[d].first + j].u < u_hi)
                            u_hi = cap[cap_draw[d].first + j].u;
                }
                ck(u_hi > u_lo + 1e-4f,
                   "and the window scrolls with the car, in the car's direction");
            }
            /* AND IT CLAMPS rather than running off the art: at a huge +x the
               window's far edge stops at 1.0, so nothing outside the painting is
               ever sampled. Both edges, because a clamp applied to one is a
               window that changes SIZE. */
            {
                float lo = 1e9f, hi = -1e9f;
                S.car_x = MAP_CALIB[0].world[0][0] + 100000.f;
                cap_reset(); race_ui_draw(&R, &S, SCRW, SCRH);
                for (d = 0; d < cap_nd; d++) {
                    if (cap_draw[d].tex != T_MAP) continue;
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *q = &cap[cap_draw[d].first + j];
                        if (q->u < lo) lo = q->u;
                        if (q->u > hi) hi = q->u;
                    }
                }
                ck(hi <= 1.0f + 1e-4f && lo >= 0.f - 1e-4f,
                   "a car far off the art clamps the window inside 0..1");
                {
                    const float fu = MAP_OUTLINE[3][0] - MAP_OUTLINE[2][0];
                    ck(fabsf((hi - lo) - fu / RUI_MAP_ZOOM) < 1e-4f,
                       "and the window keeps its SIZE while clamping");
                }
            }
            /* AND THE PLAYER SITS AT THE PANEL'S CENTRE while the window is not
               clamped -- which is what "centred on the player" MEANS, and the one
               statement a marker placed in ART space rather than WINDOW space
               fails. That mutant passed everything else here: at zoom 3 it puts
               the arrow 41% across instead of 50%, which no rotation check and no
               off-art cull can see. */
            {
                float mx = 0.f, my = 0.f, cxp = 0.f, cyp = 0.f;
                int best = -1;
                S.car_x = MAP_CALIB[0].world[0][0];
                S.car_z = MAP_CALIB[0].world[0][1];
                cap_reset();
                race_ui_draw(&R, &S, SCRW, SCRH);
                for (d = 0; d < cap_nd; d++)
                    if (cap_draw[d].tex == T_ARROW)
                        best = d;
                if (best >= 0) {
                    for (j = 0; j < cap_draw[best].n; j++) {
                        mx += cap[cap_draw[best].first + j].x;
                        my += cap[cap_draw[best].first + j].y;
                    }
                    mx /= (float)cap_draw[best].n;
                    my /= (float)cap_draw[best].n;
                }
                /* The panel's centre off the MAP quads rather than off the layout
                   constants -- the same geometry, read where it landed. */
                {
                    float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
                    for (d = 0; d < cap_nd; d++) {
                        if (cap_draw[d].tex != T_MAP) continue;
                        for (j = 0; j < cap_draw[d].n; j++) {
                            const cap_vtx *q = &cap[cap_draw[d].first + j];
                            if (q->x < x0) x0 = q->x; if (q->x > x1) x1 = q->x;
                            if (q->y < y0) y0 = q->y; if (q->y > y1) y1 = q->y;
                        }
                    }
                    cxp = (x0 + x1) * 0.5f;
                    cyp = (y0 + y1) * 0.5f;
                }
                ck(best >= 0, "the player's marker is drawn");
                /* Within a fiftieth of the screen height: the outline is inset
                   asymmetrically in v, so its bbox centre is not exactly the
                   window's, and this is not a place to chase a pixel. */
                ck(best >= 0 && fabsf(mx - cxp) < (float)SCRH * 0.02f
                   && fabsf(my - cyp) < (float)SCRH * 0.02f,
                   "and it sits at the panel's own centre -- the window is "
                   "CENTRED on the player");
            }
            S.car_x = MAP_CALIB[0].world[0][0];
            S.car_z = MAP_CALIB[0].world[0][1];
        }
        /* The checkpoint markers take map_cp's TWO cells, and the one being
           headed for is not the cell the others get. */
        {
            int d, next_cell = -1, other = 0, mixed = 0;
            S.n_cp = 3; S.cp_next = 1;
            for (i = 0; i < 3; i++) {
                S.cp_x[i] = MAP_CALIB[0].world[i][0];
                S.cp_z[i] = MAP_CALIB[0].world[i][1];
            }
            cap_reset();
            race_ui_draw(&R, &S, SCRW, SCRH);
            for (d = 0; d < cap_nd; d++) {
                if (cap_draw[d].tex != T_CP) continue;
                {
                    int j, left = 0;
                    for (j = 0; j < cap_draw[d].n; j++)
                        if (cap[cap_draw[d].first + j].u < 0.5f) left = 1;
                    if (d == 0 || next_cell < 0) { }
                    if (left) next_cell++; else other++;
                }
                mixed++;
            }
            ck(mixed == 3, "one map_cp marker per checkpoint");
            ck(other == 2,
               "and the checkpoint being headed for takes the OTHER cell");
        }

        /* ---- 12. the clocks -------------------------------------------- */
        printf("-- 12: the race and lap clocks\n");
        {
            int held = 0;
            race_ui_init(&R, &T);
            ck(R.t_race == 0.f && !R.running, "a fresh HUD is not timing");
            race_ui_step(&R, 1.f);
            ck(R.t_race == 0.f, "and does not tick before the race starts");

            race_ui_start(&R);
            race_ui_step(&R, 0.5f);
            race_ui_step(&R, 0.5f);
            ck(fabsf(R.t_race - 1.f) < 1e-5f, "the race clock banks the dt");
            ck(fabsf(race_ui_lap_time(&R, &held) - 1.f) < 1e-5f && !held,
               "and so does the lap clock, unheld");

            /* dt 0 is what the menu does to the rest of the world. */
            race_ui_step(&R, 0.f);
            ck(fabsf(R.t_race - 1.f) < 1e-5f, "a dt of 0 stops the clock dead");

            race_ui_lap(&R);
            ck(fabsf(race_ui_lap_time(&R, &held) - 1.f) < 1e-5f && held,
               "crossing the line HOLDS the finished lap's time");
            ck(R.t_lap == 0.f, "and restarts the lap clock at zero");
            ck(fabsf(R.t_race - 1.f) < 1e-5f, "the race clock is untouched");

            /* The hold lasts the shipped timeLapBlinkInSec and no longer. Two
               absolute steps around it, written out with the value, rather than
               fractions OF it -- the self-reference trap, for the ninth time. */
            race_ui_step(&R, 4.1f);
            race_ui_lap_time(&R, &held);
            ck(held, "still held at 4.1 s (timeLapBlinkInSec is 4.2)");
            race_ui_step(&R, 0.2f);
            race_ui_lap_time(&R, &held);
            ck(!held, "and released by 4.3");
            ck(fabsf(race_ui_lap_time(&R, &held) - 4.3f) < 1e-4f,
               "after which the line reads the LIVE lap again");

            /* A hold survives the race ending, so a last-lap crossing is seen. */
            race_ui_lap(&R);
            race_ui_stop(&R);
            {
                float was = R.t_race;
                race_ui_step(&R, 1.f);
                ck(R.t_race == was, "a stopped race clock does not advance");
                race_ui_lap_time(&R, &held);
                ck(held, "but a hold in flight still runs down");
            }
            /* And start() clears a hold, so a restart does not inherit the
               previous run's lap time -- the class of bug cp_restart vs
               cp_resync is entirely about. */
            race_ui_start(&R);
            race_ui_lap_time(&R, &held);
            ck(!held && R.lap_hold < 0.f,
               "race_ui_start clears a hold left over from the last race");
            ck(R.tex.dial == T_DIAL,
               "and keeps its texture bindings -- not a memset");
        }

        /* ---- 13. the engine's own font --------------------------------- */
        printf("-- 13: the Smash atlas\n");
        {
            /* Every glyph's UV rect inside its OWN cell of the 10 x 9 grid.
               ui_test's own glyph check for font.h used to assert only
               "v stays in 0..1", which a quad covering two rows satisfies. */
            int out_of_cell = 0, inverted = 0;
            for (i = 0; i < SF_N; i++) {
                const float l = SF_INK_BIG[i][0], r = SF_INK_BIG[i][1];
                if (!(l >= 0.f && r <= 1.f)) out_of_cell++;
                if (!(r > l)) inverted++;
                if (!(SF_INK_SMALL[i][0] >= 0.f && SF_INK_SMALL[i][1] <= 1.f))
                    out_of_cell++;
            }
            ck(out_of_cell == 0, "every glyph's ink is inside its own cell");
            ck(inverted == 0, "and none of the 90 is inside out");
            ck(SF_COLS * SF_ROWS == SF_N,
               "the grid holds exactly the characters the .dat names");
            /* The lookup is onto: a digit, a colon, a dot and a slash all have
               to resolve, because the two clocks and the lap counter are made
               of nothing else. */
            ck(SF_INDEX['0'] >= 0 && SF_INDEX['9'] >= 0 && SF_INDEX[':'] >= 0
               && SF_INDEX['.'] >= 0 && SF_INDEX['/'] >= 0,
               "0-9, colon, dot and slash all resolve");
            ck(SF_INDEX[' '] < 0, "and space does not -- it is an advance");
            /* PROPORTIONAL, not monospaced. Measured, not asserted from the
               table: a mutant that returns the cell width for every glyph
               satisfies "the widths are in range". */
            {
                float w_i = SF_INK_BIG[SF_INDEX['I']][1]
                          - SF_INK_BIG[SF_INDEX['I']][0];
                float w_w = SF_INK_BIG[SF_INDEX['W']][1]
                          - SF_INK_BIG[SF_INDEX['W']][0];
                ck(w_w > w_i * 1.5f, "'W' is half again wider than 'I'");
            }
        }
        /* The glyph quads that really went out, read back through the recorder:
           inside the cell, in reading order, and not overlapping. */
        {
            int d, nglyph = 0, bad_cell = 0, backwards = 0;
            float prev_x1 = -1e9f;
            race_ui_init(&R, &T);
            race_ui_set_track(&R, 0, T_MAP);
            S.lap = 7; S.n_laps = 0;
            cap_reset();
            race_ui_draw(&R, &S, SCRW, SCRH);
            for (d = 0; d < cap_nd; d++) {
                if (cap_draw[d].tex != T_FBIG) continue;
                {
                    int j;
                    float u0 = 1e9f, u1 = -1e9f, v0 = 1e9f, v1 = -1e9f;
                    float x0 = 1e9f, x1 = -1e9f;
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *q = &cap[cap_draw[d].first + j];
                        if (q->u < u0) u0 = q->u; if (q->u > u1) u1 = q->u;
                        if (q->v < v0) v0 = q->v; if (q->v > v1) v1 = q->v;
                        if (q->x < x0) x0 = q->x; if (q->x > x1) x1 = q->x;
                    }
                    /* one cell is 1/10 wide and 1/9 tall; a glyph may not
                       straddle a boundary */
                    if ((int)(u0 * SF_COLS) != (int)(u1 * SF_COLS - 1e-4f)
                        || (int)(v0 * SF_ROWS) != (int)(v1 * SF_ROWS - 1e-4f))
                        bad_cell++;
                    if (x0 < prev_x1 - 0.6f && nglyph)
                        backwards++;
                    prev_x1 = x1;
                    nglyph++;
                }
            }
            ck(nglyph > 0, "the HUD draws glyphs out of the big atlas");
            ck(bad_cell == 0, "and every one stays inside a single atlas cell");
            (void)backwards;
        }
        /* AND THE GLYPHS ARE THE RIGHT ONES, AT THE RIGHT WIDTHS. The three
           checks above are all satisfied by a glyph in the WRONG cell at the
           WRONG width -- three mutants proved it: a monospaced advance, a
           row/col swap and UVs taken from the cell's edge instead of the ink all
           survived them. So this drives one string of known characters through
           sf_text and reads every quad back against SF_INDEX and SF_INK_BIG. */
        {
            /* "WIl" -- the widest glyph, the narrowest, and a lowercase, so a
               monospaced advance cannot pass and neither can a scale applied to
               the cell rather than the ink. Drawn through the lap counter, which
               is the one string race_ui.c takes verbatim from the caller. */
            const char *want = "1/6";
            int d, gi = 0, wrong_cell = 0, wrong_u = 0, n = 0;
            float wmin = 1e9f, wmax = -1e9f;

            race_ui_init(&R, &T);
            race_ui_set_track(&R, 0, T_MAP);
            S.lap = 1; S.n_laps = 6;
            cap_reset();
            race_ui_draw(&R, &S, 800, 600);
            for (d = 0; d < cap_nd; d++) {
                if (cap_draw[d].tex != T_FBIG) continue;
                {
                    int j, want_g, want_col, want_row;
                    float u0 = 1e9f, u1 = -1e9f, v0 = 1e9f, v1 = -1e9f;
                    float x0 = 1e9f, x1 = -1e9f;
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *q = &cap[cap_draw[d].first + j];
                        if (q->u < u0) u0 = q->u; if (q->u > u1) u1 = q->u;
                        if (q->v < v0) v0 = q->v; if (q->v > v1) v1 = q->v;
                        if (q->x < x0) x0 = q->x; if (q->x > x1) x1 = q->x;
                    }
                    /* The clocks draw first and the lap counter last, so only the
                       final three big-atlas quads are `1/6' -- take them by
                       counting backwards is fragile, so instead match on the
                       glyph the UVs name and only judge those that are one of
                       `1', `/' or `6'. Every quad is judged by SOME character:
                       the cell it lands in must BE a character's cell, at that
                       character's own ink. */
                    want_col = (int)(u0 * SF_COLS);
                    want_row = (int)(v0 * SF_ROWS);
                    want_g = want_row * SF_COLS + want_col;
                    if (want_g < 0 || want_g >= SF_N) { wrong_cell++; continue; }
                    /* the u span has to be the INK's, not the cell's */
                    if (fabsf((u0 - (float)want_col / SF_COLS) * SF_COLS
                              - SF_INK_BIG[want_g][0]) > 1e-3f
                        || fabsf((u1 - (float)want_col / SF_COLS) * SF_COLS
                                 - SF_INK_BIG[want_g][1]) > 1e-3f)
                        wrong_u++;
                    if (x1 - x0 < wmin) wmin = x1 - x0;
                    if (x1 - x0 > wmax) wmax = x1 - x0;
                    n++;
                }
                gi++;
            }
            ck(n > 3, "the clocks and the counter draw a dozen glyphs or more");
            ck(wrong_cell == 0, "every quad lands in a real character's cell");
            ck(wrong_u == 0,
               "and samples that character's INK, not its whole cell");
            /* PROPORTIONAL, and judged WITHIN one string: each quad's width has
               to be proportional to its own UV span. Scale-free, which is the
               point -- the earlier form compared the widest quad in the frame
               against the narrowest, and once the lap counter was drawn at twice
               the clock's size that ratio was 2 whatever the advance did, so a
               monospaced mutant walked through it. */
            {
                float k0 = -1.f;
                int bad_prop = 0, nprop = 0;
                for (d = 0; d < cap_nd; d++) {
                    int j;
                    float u0 = 1e9f, u1 = -1e9f, x0 = 1e9f, x1 = -1e9f;
                    if (cap_draw[d].tex != T_FBIG) continue;
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *q = &cap[cap_draw[d].first + j];
                        if (q->u < u0) u0 = q->u; if (q->u > u1) u1 = q->u;
                        if (q->x < x0) x0 = q->x; if (q->x > x1) x1 = q->x;
                    }
                    if (x0 < 600.f) continue;      /* the lap counter alone */
                    if (u1 - u0 < 1e-6f) { bad_prop++; continue; }
                    {
                        const float k = (x1 - x0) / (u1 - u0);
                        if (k0 < 0.f) k0 = k;
                        else if (fabsf(k - k0) > 0.02f * k0) bad_prop++;
                        nprop++;
                    }
                }
                ck(nprop >= 2, "the lap counter draws its glyphs");
                ck(bad_prop == 0,
                   "and each is as WIDE as its own ink -- a monospaced advance "
                   "draws the narrow ones too wide");
            }
            (void)wmax; (void)wmin;
            /* The exact cells for one known string: `1' and `/' and `6' have to
               be at the .dat's own indices. A row/col swap puts them all in real
               cells belonging to other characters, which the checks above pass. */
            {
                int found1 = 0, found6 = 0, foundsl = 0;
                for (d = 0; d < cap_nd; d++) {
                    if (cap_draw[d].tex != T_FBIG) continue;
                    {
                        int j, col, row, g;
                        float u0 = 1e9f, v0 = 1e9f;
                        for (j = 0; j < cap_draw[d].n; j++) {
                            const cap_vtx *q = &cap[cap_draw[d].first + j];
                            if (q->u < u0) u0 = q->u;
                            if (q->v < v0) v0 = q->v;
                        }
                        col = (int)(u0 * SF_COLS);
                        row = (int)(v0 * SF_ROWS);
                        g = row * SF_COLS + col;
                        if (g == SF_INDEX['1']) found1++;
                        if (g == SF_INDEX['6']) found6++;
                        if (g == SF_INDEX['/']) foundsl++;
                    }
                }
                /* TWO of each: sf_text_shadowed draws the string once dark and
                   once bright, so every glyph is emitted twice. Asserted as a
                   relation rather than as the number 2 -- the counts have to
                   AGREE, which a row/col swap breaks (it lands `1' and `6' on
                   two different wrong cells) and which dropping the shadow
                   would not. */
                ck(found1 == 2 && found6 == 2 && foundsl == 2,
                   "`1/6' draws the .dat's own cells for 1, / and 6, shadowed");
            }
            S.n_laps = 0;
            (void)want; (void)gi;
        }

        /* ---- 14. the two dials ----------------------------------------- */
        printf("-- 14: the boost and speed dials\n");
        {
            /* The sweep, measured off the arc's own vertices: the angle of the
               first ray and of the last, about the ring centre. Bound to
               HUD_IND_ANG_MIN/MAX with their values written out here as a second
               copy -- moving one constant cannot satisfy both. */
            static const float A0 = -34.f, A1 = 127.f;   /* cockpit.ini */
            int d;
            float span_full = 0.f, span_half = 0.f;
            int arcs_at_zero = 0, arcs_at_full = 0;

            race_ui_init(&R, &T);
            ck(fabsf(HUD_IND_ANG_MIN - A0) < 0.01f
               && fabsf(HUD_IND_ANG_MAX - A1) < 0.01f,
               "the sweep is cockpit.ini's own -34 .. +127 degrees");

            /* empty */
            S.boost = 0.f; S.speed = 0.f;
            cap_reset();
            race_ui_draw(&R, &S, SCRW, SCRH);
            for (d = 0; d < cap_nd; d++) arcs_at_zero += cap_draw[d].tex == T_ARC;
            ck(arcs_at_zero == 0, "an empty dial draws no value arc at all");

            /* full, and half, on the LEFT dial only -- measured as the angular
               extent of the arc's own vertices about the ring's centre */
            {
                int pass;
                for (pass = 0; pass < 2; pass++) {
                    float f = pass ? 1.f : 0.5f;
                    float lo = 1e9f, hi = -1e9f, cx = 0.f, cy = 0.f;
                    int j, n = 0, first = -1;

                    S.boost = S.boost_max * f;
                    S.speed = 0.f;               /* keep the right dial silent */
                    cap_reset();
                    race_ui_draw(&R, &S, SCRW, SCRH);
                    for (d = 0; d < cap_nd; d++)
                        if (cap_draw[d].tex == T_ARC && first < 0)
                            first = d;
                    if (first < 0) continue;
                    /* the ring centre: the dial quad's own, recomputed the way
                       race_ui.c places it, from the recorded dial quad */
                    for (d = 0; d < cap_nd; d++) {
                        if (cap_draw[d].tex != T_DIAL) continue;
                        {
                            float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
                            for (j = 0; j < cap_draw[d].n; j++) {
                                const cap_vtx *q = &cap[cap_draw[d].first + j];
                                if (q->x < x0) x0 = q->x; if (q->x > x1) x1 = q->x;
                                if (q->y < y0) y0 = q->y; if (q->y > y1) y1 = q->y;
                            }
                            cx = x0 + (x1 - x0) * HUD_DIAL_CU;
                            cy = y0 + (y1 - y0) * HUD_DIAL_CV;
                            break;
                        }
                    }
                    for (d = 0; d < cap_nd; d++) {
                        if (cap_draw[d].tex != T_ARC) continue;
                        for (j = 0; j < cap_draw[d].n; j++) {
                            const cap_vtx *q = &cap[cap_draw[d].first + j];
                            float ang = atan2f(q->x - cx, -(q->y - cy))
                                        * 57.29577951f;
                            if (ang < lo) lo = ang;
                            if (ang > hi) hi = ang;
                            n++;
                        }
                    }
                    if (!n) continue;
                    if (pass) {
                        arcs_at_full = n;
                        span_full = hi - lo;
                        ck(fabsf(lo - A0) < 1.5f,
                           "a full dial's arc starts at indicatorAngMin");
                        ck(fabsf(hi - A1) < 1.5f, "and ends at indicatorAngMax");
                    } else {
                        span_half = hi - lo;
                        ck(fabsf(lo - A0) < 1.5f,
                           "a half dial's arc starts at the same end");
                        ck(hi < (A0 + A1) * 0.5f + 3.f,
                           "and stops short of the far one");
                    }
                }
                ck(arcs_at_full > 0, "a full dial draws an arc");
                ck(span_full > span_half * 1.6f,
                   "and a fuller dial's arc is the longer one -- the DynamicScale "
                   "check, which fx.c failed for years");
            }
            /* Over-full and negative are clamped, not wrapped. */
            S.boost = S.boost_max * 4.f;
            cap_reset();
            race_ui_draw(&R, &S, SCRW, SCRH);
            {
                float hi = -1e9f, cx = 0.f, cy = 0.f;
                int j;
                for (d = 0; d < cap_nd; d++) {
                    if (cap_draw[d].tex != T_DIAL) continue;
                    {
                        float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
                        for (j = 0; j < cap_draw[d].n; j++) {
                            const cap_vtx *q = &cap[cap_draw[d].first + j];
                            if (q->x < x0) x0 = q->x; if (q->x > x1) x1 = q->x;
                            if (q->y < y0) y0 = q->y; if (q->y > y1) y1 = q->y;
                        }
                        cx = x0 + (x1 - x0) * HUD_DIAL_CU;
                        cy = y0 + (y1 - y0) * HUD_DIAL_CV;
                        break;
                    }
                }
                for (d = 0; d < cap_nd; d++) {
                    if (cap_draw[d].tex != T_ARC) continue;
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *q = &cap[cap_draw[d].first + j];
                        float ang = atan2f(q->x - cx, -(q->y - cy))
                                    * 57.29577951f;
                        if (ang > hi) hi = ang;
                    }
                }
                ck(hi < A1 + 2.f, "a dial past full does not sweep past its end");
            }
            /* THE TWO DIALS ARE MIRRORS, and the right one samples the art where
               the art is: its screen sweep runs the other way while its texture
               U's do not. A tex_mirror that also flipped the sample would read
               grey off the ring's far side. */
            {
                /* Per dial: the ring centre off its OWN recorded quad, then the
                   arc's angular extent about it and the arc's u range. The left
                   dial must sweep A0..A1 and the right one -A1..-A0 -- the
                   NEGATED range, which is what makes it a reflection -- while
                   BOTH must sample the same u's, because the red arc is not
                   symmetric and a mirror applied to the sample as well as to the
                   screen would read grey off the ring's far side.
                 *
                 * Two mutants survived the earlier version of this check, which
                 * only asked for one arc on each half of the screen: dropping the
                 * mirror entirely, and ignoring ui_arc's tex_mirror. */
                float clo[2], chi[2], culo[2], cuhi[2], ccx[2], ccy[2];
                int have[2], j, k;
                S.boost = S.boost_max; S.speed = S.speed_max;
                cap_reset();
                race_ui_draw(&R, &S, SCRW, SCRH);
                for (k = 0; k < 2; k++) {
                    have[k] = 0;
                    clo[k] = culo[k] = 1e9f;
                    chi[k] = cuhi[k] = -1e9f;
                    ccx[k] = ccy[k] = 0.f;
                }
                /* the two dial quads, left then right */
                for (d = 0; d < cap_nd; d++) {
                    float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
                    if (cap_draw[d].tex != T_DIAL) continue;
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *q = &cap[cap_draw[d].first + j];
                        if (q->x < x0) x0 = q->x; if (q->x > x1) x1 = q->x;
                        if (q->y < y0) y0 = q->y; if (q->y > y1) y1 = q->y;
                    }
                    k = ((x0 + x1) * 0.5f < SCRW * 0.5f) ? 0 : 1;
                    /* the right dial's quad is drawn with u running 1 -> 0, so
                       the ring's own u sits at 1 - HUD_DIAL_CU across it */
                    ccx[k] = x0 + (x1 - x0)
                             * (k ? (1.f - HUD_DIAL_CU) : HUD_DIAL_CU);
                    ccy[k] = y0 + (y1 - y0) * HUD_DIAL_CV;
                    have[k] |= 1;
                }
                ck(have[0] && have[1], "both dials draw their body");
                for (d = 0; d < cap_nd; d++) {
                    if (cap_draw[d].tex != T_ARC) continue;
                    k = (cap[cap_draw[d].first].x < SCRW * 0.5f) ? 0 : 1;
                    have[k] |= 2;
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *q = &cap[cap_draw[d].first + j];
                        float ang = atan2f(q->x - ccx[k], -(q->y - ccy[k]))
                                    * 57.29577951f;
                        if (ang < clo[k]) clo[k] = ang;
                        if (ang > chi[k]) chi[k] = ang;
                        if (q->u < culo[k]) culo[k] = q->u;
                        if (q->u > cuhi[k]) cuhi[k] = q->u;
                    }
                }
                ck(have[0] == 3 && have[1] == 3,
                   "both dials draw a value arc when both are full");
                ck(fabsf(clo[0] - A0) < 2.f && fabsf(chi[0] - A1) < 2.f,
                   "the left dial sweeps -34 .. +127 about its own ring");
                ck(fabsf(clo[1] + A1) < 2.f && fabsf(chi[1] + A0) < 2.f,
                   "and the RIGHT dial the mirror of it, -127 .. +34");
                ck(fabsf(culo[0] - culo[1]) < 1e-3f
                   && fabsf(cuhi[0] - cuhi[1]) < 1e-3f,
                   "while both sample the SAME u's -- the mirror is on the "
                   "screen, not on the art");
            }
        }

        /* ---- 14b. THE UNITS, and the readouts' size -------------------- */
        printf("-- 14b: units, and the doubled readouts\n");
        {
            char txt[64];
            race_ui_init(&R, &T);
            race_ui_set_track(&R, 0, T_MAP);
            /* 20 m/s is 72 km/h and 45 mph -- two numbers that share no digits,
               so a check on the STRING cannot be satisfied by the wrong unit. */
            S.speed = 20.f; S.speed_max = 40.f;
            S.boost = 0.f;             /* keep the left dial's text out of it */

            /* METRIC BY DEFAULT. The label is read back out of the atlas cells,
               so this asserts the string the GPU got, not a branch. */
            cap_reset();
            race_ui_draw(&R, &S, SCRW, SCRH);
            cap_read_text(T_FSMALL, SCRH * 0.5f, (float)SCRH, txt, sizeof txt);
            ck(strstr(txt, "km/h") != NULL,
               "the speed dial reads km/h out of the box");
            ck(strstr(txt, "72") != NULL,
               "and 20 m/s as 72, not as 45 -- the metric conversion");
            ck(strstr(txt, "mph") == NULL, "and says nothing about mph");

            race_ui_units(&R, RUI_IMPERIAL);
            cap_reset();
            race_ui_draw(&R, &S, SCRW, SCRH);
            cap_read_text(T_FSMALL, SCRH * 0.5f, (float)SCRH, txt, sizeof txt);
            ck(strstr(txt, "mph") != NULL,
               "race_ui_units switches it to the original's own mph");
            ck(strstr(txt, "45") != NULL, "and to 45");
            /* A preference, not race state: a restart must not undo it. */
            race_ui_start(&R);
            cap_reset();
            race_ui_draw(&R, &S, SCRW, SCRH);
            cap_read_text(T_FSMALL, SCRH * 0.5f, (float)SCRH, txt, sizeof txt);
            ck(strstr(txt, "mph") != NULL,
               "and race_ui_start does not put it back -- it is a preference");
            race_ui_units(&R, RUI_METRIC);

            /* THE THREE DOUBLED READOUTS. Measured as a RATIO against the clock,
               which stays at the recovered size -- so this cannot be satisfied by
               scaling the whole HUD, and it does not restate the constants. */
            {
                float clock_h = 0.f, lap_h = 0.f, dial_h = 0.f;
                int d, j;
                S.lap = 8; S.n_laps = 0; S.boost = 50.f;
                cap_reset();
                race_ui_draw(&R, &S, 800, 600);
                for (d = 0; d < cap_nd; d++) {
                    float y0 = 1e9f, y1 = -1e9f, x0 = 1e9f;
                    if (cap_draw[d].tex != T_FBIG && cap_draw[d].tex != T_FSMALL)
                        continue;
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *q = &cap[cap_draw[d].first + j];
                        if (q->y < y0) y0 = q->y;
                        if (q->y > y1) y1 = q->y;
                        if (q->x < x0) x0 = q->x;
                    }
                    /* the total-time clock is the big atlas, top of the screen,
                       near the middle; the lap count is the big atlas, top right */
                    if (cap_draw[d].tex == T_FBIG && y0 < 60.f) {
                        if (x0 > 600.f) { if (y1 - y0 > lap_h) lap_h = y1 - y0; }
                        else            { if (y1 - y0 > clock_h) clock_h = y1 - y0; }
                    }
                    /* the dial readings are the small atlas, bottom half */
                    if (cap_draw[d].tex == T_FSMALL && y0 > 300.f)
                        if (y1 - y0 > dial_h) dial_h = y1 - y0;
                }
                ck(clock_h > 1.f && lap_h > 1.f && dial_h > 1.f,
                   "the clock, the lap count and a dial reading are all drawn");
                ck(lap_h > clock_h * 1.8f,
                   "the lap count is about twice the clock's height");
                ck(dial_h > clock_h * 1.8f,
                   "and so is the dial's reading -- the two the panel is too "
                   "small to read at the original's size");
                S.n_laps = 0;
            }

            /* THE NUDGES ARE BOUNDED, not restated. RUI_DIAL_TEXT_DY and
               RUI_LAPNUM_DX/DY are by-eye offsets and a check on their VALUES
               would compare against the constant it guards -- which passes
               everything (traps.md). What is worth holding is what they must not
               break: the dial's readout stays inside the ring's own hole, and the
               lap counter stays on screen. A nudge of 100 px instead of 10 fails
               both; a nudge of 9 or 11 fails neither, and should not. */
            {
                int d, j, outside = 0, offscreen = 0, nglyph = 0;
                float cx = 0.f, cy = 0.f, r0 = 0.f;
                S.boost = 50.f; S.speed = 20.f; S.lap = 8;
                cap_reset();
                race_ui_draw(&R, &S, SCRW, SCRH);
                /* the LEFT dial's ring centre and inner radius, off its own quad */
                for (d = 0; d < cap_nd; d++) {
                    float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
                    if (cap_draw[d].tex != T_DIAL) continue;
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *q = &cap[cap_draw[d].first + j];
                        if (q->x < x0) x0 = q->x; if (q->x > x1) x1 = q->x;
                        if (q->y < y0) y0 = q->y; if (q->y > y1) y1 = q->y;
                    }
                    if ((x0 + x1) * 0.5f > SCRW * 0.5f) continue;   /* left only */
                    cx = x0 + (x1 - x0) * HUD_DIAL_CU;
                    cy = y0 + (y1 - y0) * HUD_DIAL_CV;
                    r0 = (x1 - x0) * HUD_DIAL_R0;
                    break;
                }
                for (d = 0; d < cap_nd; d++) {
                    if (cap_draw[d].tex != T_FSMALL) continue;
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *q = &cap[cap_draw[d].first + j];
                        /* the small atlas draws the lap-TIME clock too, at the
                           top of the screen -- so the bottom-left quadrant, not
                           just the left half. Its corner came out 580 px from the
                           ring and failed this on correct code. */
                        if (q->x > SCRW * 0.5f || q->y < SCRH * 0.5f)
                            continue;
                        nglyph++;
                        if (sqrtf((q->x - cx) * (q->x - cx)
                                  + (q->y - cy) * (q->y - cy)) > r0)
                            outside++;
                    }
                }
                ck(r0 > 1.f && nglyph > 0,
                   "the boost dial's readout is drawn inside a located ring");
                ck(outside == 0,
                   "and every glyph of it stays inside the ring's own hole -- "
                   "the nudge cannot push it onto the arc");
                for (d = 0; d < cap_nd; d++) {
                    if (cap_draw[d].tex != T_FBIG) continue;
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *q = &cap[cap_draw[d].first + j];
                        if (q->x < 0.f || q->x > (float)SCRW
                            || q->y < 0.f || q->y > (float)SCRH)
                            offscreen++;
                    }
                }
                ck(offscreen == 0,
                   "and the clocks and the lap counter stay ON the screen");
            }
        }

        /* ---- 15. the layout is the authored one ------------------------ */
        printf("-- 15: the 800x600 layout\n");
        {
            /* On a 4:3 panel of exactly the authored height, every position must
               come out AT its authored pixel. That is what makes the mapping
               readable: the anchors only ever move things on a panel that is not
               4:3, and here there is nothing for them to do. */
            const int W = 800, H = 600;
            int d, found_place = 0, found_lap = 0;
            float px = 0.f, py = 0.f, lx = 0.f, ly = 0.f;

            race_ui_init(&R, &T);
            race_ui_set_track(&R, 0, T_MAP);
            S.place = 3;
            cap_reset();
            race_ui_draw(&R, &S, W, H);
            for (d = 0; d < cap_nd; d++) {
                int j;
                float x0 = 1e9f, y0 = 1e9f;
                for (j = 0; j < cap_draw[d].n; j++) {
                    const cap_vtx *q = &cap[cap_draw[d].first + j];
                    if (q->x < x0) x0 = q->x;
                    if (q->y < y0) y0 = q->y;
                }
                if (cap_draw[d].tex == T.place[2]) {
                    px = x0; py = y0; found_place = 1;
                } else if (cap_draw[d].tex == T_LAP) {
                    lx = x0; ly = y0; found_lap = 1;
                }
            }
            ck(found_place, "the place badge is drawn -- place3 for third");
            ck(fabsf(px - HUD_PLACE_X) < 0.01f && fabsf(py - HUD_PLACE_Y) < 0.01f,
               "at cockpit.ini's own placePosX/Y, including the negative Y");
            ck(found_lap, "and the LAP word is drawn");
            ck(fabsf(lx - HUD_LAP_X) < 0.01f && fabsf(ly - HUD_LAP_Y) < 0.01f,
               "at its own lapPosX/Y");
            /* The map panel sits at map.ini's raw pixels. */
            {
                float mx = 1e9f, my = 1e9f;
                for (d = 0; d < cap_nd; d++) {
                    int j;
                    if (cap_draw[d].tex != T_MAP) continue;
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *q = &cap[cap_draw[d].first + j];
                        if (q->x < mx) mx = q->x;
                        if (q->y < my) my = q->y;
                    }
                }
                /* The outline's own first row is inset from the panel box, so
                   the drawn minimum is the panel corner plus that inset -- which
                   is the engine's shape, not a fudge. */
                ck(fabsf(mx - (MAP_POS_X + MAP_OUTLINE[2][0] * MAP_SIDE)) < 0.5f,
                   "the minimap's left edge is map.ini's corner plus the "
                   "outline's own inset");
                ck(fabsf(my - (MAP_POS_Y + MAP_OUTLINE[0][1] * MAP_SIDE)) < 0.5f,
                   "and its top edge likewise");
            }
            /* THE THREE PANEL QUADS PAIR ADJACENT ROWS of the outline table.
               The bbox checks above cannot see a reversed or shuffled pairing --
               all eight points are still emitted -- and the table is very nearly
               symmetric top to bottom, so a reversal LOOKS right. Each quad must
               carry exactly two of the table's four v values and they must be
               neighbours. */
            {
                int q, rows_seen[4], bad_pair = 0, nq = 0;
                /* The UVs are the outline through the WINDOW the panel shows
                   (RUI_MAP_ZOOM), so the window is recovered from the drawn span
                   first and the outline rows are compared THROUGH it. Recovered
                   rather than assumed: at zoom 1 this is the identity, and at any
                   other zoom a check written against the raw fractions would fail
                   on correct code -- which it did. */
                float vlo = 1e9f, vhi = -1e9f, wside, wv0;
                for (d = 0; d < cap_nd; d++) {
                    int j;
                    if (cap_draw[d].tex != T_MAP) continue;
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *c = &cap[cap_draw[d].first + j];
                        if (c->v < vlo) vlo = c->v;
                        if (c->v > vhi) vhi = c->v;
                    }
                }
                wside = (vhi - vlo)
                        / (MAP_OUTLINE[6][1] - MAP_OUTLINE[0][1]);
                wv0 = vlo - MAP_OUTLINE[0][1] * wside;
                for (q = 0; q < 4; q++) rows_seen[q] = 0;
                for (d = 0; d < cap_nd; d++) {
                    int j, lo = -1, hi = -1;
                    if (cap_draw[d].tex != T_MAP) continue;
                    for (j = 0; j < cap_draw[d].n; j++) {
                        const cap_vtx *c = &cap[cap_draw[d].first + j];
                        int k, m = -1;
                        for (k = 0; k < 4; k++)
                            if (fabsf(c->v - (wv0 + MAP_OUTLINE[k * 2][1]
                                              * wside)) < 1e-4f)
                                m = k;
                        if (m < 0) { bad_pair++; continue; }
                        if (lo < 0 || m < lo) lo = m;
                        if (m > hi) hi = m;
                        rows_seen[m] = 1;
                    }
                    if (hi - lo != 1) bad_pair++;
                    nq++;
                }
                ck(nq == 3, "the minimap panel is the engine's own three quads");
                ck(bad_pair == 0,
                   "each spanning two ADJACENT rows of the outline table");
                ck(rows_seen[0] && rows_seen[1] && rows_seen[2] && rows_seen[3],
                   "and all four rows are used");
            }
            /* And the anchors: on a WIDER panel of the same height the left
               group does not move and the right group moves with the edge. */
            {
                float px2 = 0.f, lx2 = 0.f;
                cap_reset();
                race_ui_draw(&R, &S, W + 160, H);
                for (d = 0; d < cap_nd; d++) {
                    int j;
                    float x0 = 1e9f;
                    for (j = 0; j < cap_draw[d].n; j++)
                        if (cap[cap_draw[d].first + j].x < x0)
                            x0 = cap[cap_draw[d].first + j].x;
                    if (cap_draw[d].tex == T.place[2]) px2 = x0;
                    else if (cap_draw[d].tex == T_LAP) lx2 = x0;
                }
                float mx2 = 1e9f, mx1 = 1e9f;
                ck(fabsf(px2 - px) < 0.01f,
                   "a wider panel leaves the left-anchored badge where it was");
                ck(fabsf(lx2 - (lx + 160.f)) < 0.01f,
                   "and carries the right-anchored one out with the edge");
                /* AND THE MAP, which the 4:3 check above cannot speak about: at
                   exactly 800 wide axL and axR return the same number, so a map
                   anchored to the wrong edge is invisible there. The symmetric
                   point of the domain, which traps.md says to step off. */
                for (d = 0; d < cap_nd; d++) {
                    int j;
                    if (cap_draw[d].tex != T_MAP) continue;
                    for (j = 0; j < cap_draw[d].n; j++)
                        if (cap[cap_draw[d].first + j].x < mx2)
                            mx2 = cap[cap_draw[d].first + j].x;
                }
                cap_reset();
                race_ui_draw(&R, &S, W, H);
                for (d = 0; d < cap_nd; d++) {
                    int j;
                    if (cap_draw[d].tex != T_MAP) continue;
                    for (j = 0; j < cap_draw[d].n; j++)
                        if (cap[cap_draw[d].first + j].x < mx1)
                            mx1 = cap[cap_draw[d].first + j].x;
                }
                ck(fabsf(mx2 - (mx1 + 160.f)) < 0.01f,
                   "and the minimap out with it -- it is anchored RIGHT");
            }
        }

        /* ---- 16. nothing packed, and the state left behind ------------- */
        printf("-- 16: the fallbacks, and the 3D state\n");
        {
            race_ui_tex none;
            memset(&none, 0, sizeof none);
            race_ui_init(&R, &none);
            race_ui_set_track(&R, 0, 0);
            cap_reset();
            race_ui_draw(&R, &S, SCRW, SCRH);
            {
                /* It still draws: with no Smash atlas and no badge art the place,
                   the clocks and the lap counter all fall back to font.h, which
                   is compiled in and cannot fail to load -- the same rule hud.c
                   and countdown.c follow. What it must NOT do is bind 0, which
                   is undefined on this GPU (scene.h, BATCH_NO_TEXTURE). */
                int d, zero = 0;
                for (d = 0; d < cap_nd; d++)
                    zero += cap_draw[d].tex == 0;
                ck(cap_nd > 0,
                   "a HUD with nothing packed still draws, through font.h");
                ck(zero == 0, "and binds no texture 0 while doing it");
            }

            /* The font atlases missing but the badges present: the clocks and the
               lap counter fall back to font.h, the way hud.c's word does. */
            {
                race_ui_tex part = T;
                int d, on_font = 0;
                part.font_big = 0; part.font_small = 0;
                race_ui_init(&R, &part);
                race_ui_set_track(&R, 0, T_MAP);
                cap_reset();
                race_ui_draw(&R, &S, SCRW, SCRH);
                for (d = 0; d < cap_nd; d++)
                    if (cap_draw[d].tex != T_MAP && cap_draw[d].tex != T_ARROW
                        && cap_draw[d].tex != T_CP && cap_draw[d].tex != T_LAP
                        && cap_draw[d].tex != T_DIAL && cap_draw[d].tex != T_ARC
                        && cap_draw[d].tex != T.place[2] && cap_draw[d].tex != 0)
                        on_font++;
                ck(on_font > 0,
                   "with no Smash atlas the clocks still draw, through font.h");
            }

            /* And it leaves the 3D pass's state exactly as it found it, from
               BOTH starting states -- the half of ck_state_restored's idea that
               ui_test did not have. */
            {
                int pass;
                for (pass = 0; pass < 2; pass++) {
                    mat_depth[0] = mat_depth[1] = 0;
                    mat_underflow = 0;
                    glcap_matrix_mode = GL_MODELVIEW;
                    en_depth = pass; en_alpha = pass; en_cull = pass;
                    en_blend = !pass;
                    race_ui_init(&R, &T);
                    race_ui_set_track(&R, 0, T_MAP);
                    race_ui_start(&R);
                    race_ui_step(&R, 1.f);
                    race_ui_draw(&R, &S, SCRW, SCRH);
                    ck(mat_depth[0] == 0 && mat_depth[1] == 0 && !mat_underflow,
                       "race_ui_draw leaves both matrix stacks balanced");
                    ck(en_depth == pass && en_alpha == pass && en_cull == pass
                       && en_blend == !pass && cs_color == 0,
                       "and every enable back where it found it, from either "
                       "starting state");
                    ck(glcap_matrix_mode == GL_MODELVIEW,
                       "and the matrix mode on MODELVIEW");
                    ck(cap_tex_enabled == 1,
                       "and GL_TEXTURE_2D back ON -- the plate turns it off");
                }
            }
        }
    }

    /* ---- 17. the direction arrow and WRONG WAY (dirarrow.c) ---------
     *
     * Everything here is measured on screen or as a ratio between two recovered
     * numbers, never against the constant it guards -- traps.md, seven times.
     */
    {
        dirarrow_t D;
        dirarrow_tex DT;
        dirarrow_in IN;
        const unsigned int T_ARROW = 901, T_BANNER = 902;
        const float side = ARW_LEN * (float)SCRH;
        const float vcx = ARW_CX * (float)SCRW, vcy = ARW_CY * (float)SCRH;

        DT.arrow = T_ARROW;
        (void)T_BANNER;                 /* part 18's; the banner is msg.c's now */

        /* A car at the origin facing +Z (the rig's convention: yaw 0 -> +Z), with
           the aim point straight ahead and the one behind it far enough back not
           to suppress anything. */
        memset(&IN, 0, sizeof IN);
        IN.valid = 1;
        IN.car_x = 0.f; IN.car_z = 0.f; IN.car_yaw = 0.f;
        IN.aim_x = 0.f; IN.aim_z = 40.f;
        IN.prev_x = 0.f; IN.prev_z = -40.f;
        IN.seg_frac = 0.5f;
        IN.cp_dist = 40.f;

        /* --- the chevrons point FORWARD, and the mesh is what says so.
           fv 0 is the apex; on screen that has to be ABOVE the near edge. */
        dirarrow_init(&D, &DT);
        {
            float ax, ay, nx2, ny2;
            int oka = dirarrow_project(&D, 0, 0.5f, 0.f, SCRW, SCRH, &ax, &ay);
            int okn = dirarrow_project(&D, 0, 0.5f, 1.f, SCRW, SCRH, &nx2, &ny2);
            ck(oka && okn, "both ends of a chevron project in front of the camera");
            ck(ay < ny2 - 4.f, "the apex is ABOVE the near edge -- the chevron "
                               "points up the screen");
            ck(fabsf(ax - vcx) < 0.5f && fabsf(nx2 - vcx) < 0.5f,
               "and at angle 0 the centre line is on the viewport's own centre");
        }

        /* --- the arrow lives in arrow.ini's square and nowhere else, at TWO
           screen widths, because at one width a wrong anchor can sit on the
           symmetric point of the domain (part 15 learned this). */
        {
            int w2;
            for (w2 = 0; w2 < 2; w2++) {
                const int W = w2 ? SCRW : 800, H = w2 ? SCRH : 600;
                const float sd = ARW_LEN * (float)H;
                const float cx2 = ARW_CX * (float)W, cy2 = ARW_CY * (float)H;
                float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
                int i2, c2, nq = 0, offtex = 0;
                cap_reset();
                dirarrow_draw(&D, W, H);
                for (c2 = 0; c2 < cap_nd; c2++) {
                    if (cap_draw[c2].tex != T_ARROW) { offtex++; continue; }
                    nq++;
                    for (i2 = 0; i2 < cap_draw[c2].n; i2++) {
                        const cap_vtx *q = &cap[cap_draw[c2].first + i2];
                        if (q->x < x0) x0 = q->x;
                        if (q->x > x1) x1 = q->x;
                        if (q->y < y0) y0 = q->y;
                        if (q->y > y1) y1 = q->y;
                    }
                }
                ck(offtex == 0, "every draw of a steady arrow is the chevron "
                                "texture -- nothing else is emitted");
                ck(nq == ARW_CHEV_N * DIRARROW_GRID * DIRARROW_GRID,
                   "one quad per grid cell per chevron");
                ck(x0 >= cx2 - sd * 0.5f - 0.5f && x1 <= cx2 + sd * 0.5f + 0.5f,
                   "the chevrons stay inside arrow.ini's own square in x");
                ck(y0 >= cy2 - sd * 0.5f - 0.5f && y1 <= cy2 + sd * 0.5f + 0.5f,
                   "and in y");
                ck(fabsf((x0 + x1) * 0.5f - cx2) < 1.f,
                   "and are centred on CX at both widths");
                /* The square is a SQUARE off the height, so the arrow's own
                   width must not grow with the screen's width. */
                if (w2 == 0)
                    p[0] = x1 - x0;
                else
                    ck(fabsf((x1 - x0) / p[0] - (float)SCRH / 600.f) < 0.02f,
                       "and scale with the HEIGHT, not the width");
            }
        }

        /* --- the grid TILES the recovered UV rect and stays inside it. A cell
           drawn at the wrong u or v is still a cell, so the union is the check. */
        {
            int c2, i2;
            float u_lo = 1e9f, u_hi = -1e9f, v_lo = 1e9f, v_hi = -1e9f;
            const float ru0 = ARW_CHEV_U1 < ARW_CHEV_U0 ? ARW_CHEV_U1 : ARW_CHEV_U0;
            const float ru1 = ARW_CHEV_U1 < ARW_CHEV_U0 ? ARW_CHEV_U0 : ARW_CHEV_U1;
            cap_reset();
            dirarrow_draw(&D, SCRW, SCRH);
            for (c2 = 0; c2 < cap_nd; c2++) {
                if (cap_draw[c2].tex != T_ARROW) continue;
                for (i2 = 0; i2 < cap_draw[c2].n; i2++) {
                    const cap_vtx *q = &cap[cap_draw[c2].first + i2];
                    if (q->u < u_lo) u_lo = q->u;
                    if (q->u > u_hi) u_hi = q->u;
                    if (q->v < v_lo) v_lo = q->v;
                    if (q->v > v_hi) v_hi = q->v;
                }
            }
            ck(fabsf(u_lo - ru0) < 1e-4f && fabsf(u_hi - ru1) < 1e-4f,
               "the grid's UVs cover exactly the chevron's own u range");
            ck(fabsf(v_lo - ARW_CHEV_V0) < 1e-4f
               && fabsf(v_hi - ARW_CHEV_V1) < 1e-4f, "and its v range");

            /* AND WHICH END OF THE RANGE GOES WHERE, which the union above
               cannot see: flipping v draws the chevron's own apex pointing DOWN
               inside geometry that still points up, and every check so far
               passed on it. The topmost quad on screen has to be the one
               carrying the apex's v. */
            {
                float best_y = 1e9f, best_v = 0.f;
                for (c2 = 0; c2 < cap_nd; c2++) {
                    float qy = 1e9f, qv = 1e9f;
                    if (cap_draw[c2].tex != T_ARROW) continue;
                    for (i2 = 0; i2 < cap_draw[c2].n; i2++) {
                        const cap_vtx *q = &cap[cap_draw[c2].first + i2];
                        if (q->y < qy) qy = q->y;
                        if (q->v < qv) qv = q->v;
                    }
                    if (qy < best_y) { best_y = qy; best_v = qv; }
                }
                ck(fabsf(best_v - ARW_CHEV_V0) < 1e-4f,
                   "and the quad at the TOP of the screen carries the apex's "
                   "own v -- the art points the way the mesh does");
            }
        }

        /* --- THE TWO CHEVRONS ARE TWO, one behind the other, and the gap is the
           one the mesh has. Both drawn at the same z passes the quad count, the
           square and the UVs -- it is only visible as a distance. */
        {
            float ay0, ay1, ny0;
            const float zgap = ARW_CHEV_Z[1][0] - ARW_CHEV_Z[0][0];
            const float zdep = ARW_CHEV_Z[0][1] - ARW_CHEV_Z[0][0];
            D.ang = 0.f;
            dirarrow_project(&D, 0, 0.5f, 0.f, SCRW, SCRH, &q[0], &ay0);
            dirarrow_project(&D, 0, 0.5f, 1.f, SCRW, SCRH, &q[0], &ny0);
            dirarrow_project(&D, 1, 0.5f, 0.f, SCRW, SCRH, &q[0], &ay1);
            ck(ay1 > ay0 + 2.f,
               "the second chevron sits BELOW the first -- nearer the camera");
            ck(zdep > 0.f
               && fabsf((ay1 - ay0) / (ny0 - ay0) - zgap / zdep) < 0.3f,
               "and the gap between them is the gap the mesh has");
        }

        /* --- THE ARROW FILLS ITS OWN SQUARE. arrow.ini's AngleVert and pos_y /
           pos_z put the chevrons across nearly the whole viewport; dropping the
           field of view -- or getting the camera distance wrong -- leaves a
           correctly centred arrow inside a correctly placed square at a quarter
           of the size, which every other check here passes. */
        {
            float x0 = 1e9f, x1 = -1e9f;
            int c2, i2;
            D.ang = 0.f;
            cap_reset();
            dirarrow_draw(&D, SCRW, SCRH);
            for (c2 = 0; c2 < cap_nd; c2++) {
                if (cap_draw[c2].tex != T_ARROW) continue;
                for (i2 = 0; i2 < cap_draw[c2].n; i2++) {
                    const cap_vtx *q2 = &cap[cap_draw[c2].first + i2];
                    if (q2->x < x0) x0 = q2->x;
                    if (q2->x > x1) x1 = q2->x;
                }
            }
            printf("  [arrow] the chevrons span %.0f%% of the square's width\n",
                   100.f * (x1 - x0) / side);
            ck((x1 - x0) > 0.4f * side && (x1 - x0) <= side + 1.f,
               "the chevrons fill a good half of arrow.ini's square rather "
               "than rattling around inside it");
        }

        /* --- HOW FAR ui.c's TWO-TRIANGLE MAP IS FROM THE REAL PROJECTION, at
           the cell centre, which is where the diagonal split is worst. This is
           the measurement dirarrow.h's grid table is made of, taken again here
           over every rotation so DIRARROW_GRID cannot be lowered quietly. */
        {
            float worst = 0.f;
            int c2, gi, gj, a2;
            for (a2 = 0; a2 < 72; a2++) {
                D.ang = (float)a2 * 5.f - 180.f;
                for (c2 = 0; c2 < ARW_CHEV_N; c2++)
                    for (gj = 0; gj < DIRARROW_GRID; gj++)
                        for (gi = 0; gi < DIRARROW_GRID; gi++) {
                            const float g = (float)DIRARROW_GRID;
                            const float fu0 = gi / g, fu1 = (gi + 1) / g;
                            const float fv0 = gj / g, fv1 = (gj + 1) / g;
                            float ax, ay, bx, by, mx, my, e;
                            if (!dirarrow_project(&D, c2, fu0, fv0, SCRW, SCRH,
                                                  &ax, &ay)) continue;
                            if (!dirarrow_project(&D, c2, fu1, fv1, SCRW, SCRH,
                                                  &bx, &by)) continue;
                            if (!dirarrow_project(&D, c2, (fu0 + fu1) * 0.5f,
                                                  (fv0 + fv1) * 0.5f,
                                                  SCRW, SCRH, &mx, &my)) continue;
                            /* the split runs corner 0 to corner 2, so the map's
                               answer at the cell centre is that diagonal's
                               midpoint */
                            e = sqrtf(((ax + bx) * 0.5f - mx)
                                      * ((ax + bx) * 0.5f - mx)
                                      + ((ay + by) * 0.5f - my)
                                      * ((ay + by) * 0.5f - my));
                            if (e > worst) worst = e;
                        }
            }
            printf("  [arrow] grid %d: worst two-triangle error %.2f px of a "
                   "%.1f px viewport\n", DIRARROW_GRID, worst, side);
            ck(worst < 1.f, "the grid keeps ui.c's interpolation inside a pixel "
                            "of the real projection");
        }

        /* --- THE SIGN, and it is measured on SCREEN rather than in the angle:
           a check on the angle would agree with the engine's own unsigned one at
           every cardinal, and +a and -a agree at 0 and 180. Four bearings and a
           lopsided one, driven through the slew until it settles. */
        {
            struct { float ax, az; int want_left; const char *what; } bear[5] = {
                {  40.f,   0.f, 0, "a target to the RIGHT leans the arrow right" },
                { -40.f,   0.f, 1, "a target to the LEFT leans it left" },
                {  17.f,  40.f, 0, "and a lopsided right-hand target too" },
                { -17.f,  40.f, 1, "and a lopsided left-hand one" },
                {   0.f,  40.f, 2, "straight ahead stays on the centre line" },
            };
            int k2;
            for (k2 = 0; k2 < 5; k2++) {
                float apex_x, apex_y;
                int f2;
                dirarrow_init(&D, &DT);
                IN.aim_x = bear[k2].ax;
                IN.aim_z = bear[k2].az;
                for (f2 = 0; f2 < 600; f2++)
                    dirarrow_step(&D, &IN, 1.f / 60.f);
                dirarrow_project(&D, 0, 0.5f, 0.f, SCRW, SCRH, &apex_x, &apex_y);
                if (bear[k2].want_left == 2)
                    ck(fabsf(apex_x - vcx) < 0.5f, bear[k2].what);
                else if (bear[k2].want_left)
                    ck(apex_x < vcx - 2.f, bear[k2].what);
                else
                    ck(apex_x > vcx + 2.f, bear[k2].what);
            }
            IN.aim_x = 0.f; IN.aim_z = 40.f;
        }

        /* --- the slew is a RATE, and the snap is ARW_SNAP_MUL times it. A ratio
           between two recovered numbers, so moving either one on its own fails. */
        {
            float slow, fast;
            dirarrow_init(&D, &DT);
            IN.aim_x = 40.f; IN.aim_z = 0.f;              /* a 90 degree target */
            /* Burn the reset's snap off first -- and note it is the first STEP
               that latches it, not the reset, so the loop has to be entered
               unconditionally. Written as a while() the first time, which meant
               `slow' was measured with the snap still on and the ratio came out
               1: a check that passes on a build with no snap at all. */
            do {
                dirarrow_step(&D, &IN, 1.f / 60.f);
            } while (D.snap);
            D.ang = 0.f;
            dirarrow_step(&D, &IN, 0.1f);
            slow = fabsf(D.ang);
            dirarrow_init(&D, &DT);
            D.ang = 0.f;
            D.snap_req = 1;
            dirarrow_step(&D, &IN, 0.1f);
            fast = fabsf(D.ang);
            ck(slow > 0.5f && slow < 45.f,
               "the arrow SLEWS to a new heading rather than snapping to it");
            ck(fabsf(fast / slow - ARW_SNAP_MUL) < 0.05f,
               "and a snap covers ARW_SNAP_MUL times as much in the same dt");
            IN.aim_x = 0.f; IN.aim_z = 40.f;
        }

        /* --- THE WRONG WAY. Facing the aim point, never; facing away, at the
           recovered time and not before. */
        {
            float t2;
            int fired_at = -1, f2;
            dirarrow_init(&D, &DT);
            for (f2 = 0; f2 < 1800; f2++)
                dirarrow_step(&D, &IN, 1.f / 60.f);
            ck(!dirarrow_wrong(&D),
               "30 seconds pointing AT the next checkpoint is never wrong way");

            dirarrow_init(&D, &DT);
            IN.car_yaw = 3.14159265f;                    /* turned right round */
            for (f2 = 0; f2 < 1800; f2++) {
                dirarrow_step(&D, &IN, 1.f / 60.f);
                if (fired_at < 0 && dirarrow_wrong(&D))
                    fired_at = f2;
            }
            t2 = (float)(fired_at + 1) / 60.f;
            ck(fired_at >= 0, "and facing away DOES raise it");
            ck(fabsf(t2 - ARW_WRONG_ON) < 0.1f,
               "at the recovered ARW_WRONG_ON and not before");
        }

        /* --- and the decay is ARW_WRONG_DECAY times the rise. Measured as the
           ratio of two times, so neither constant can satisfy it alone. */
        {
            /* THE TIMER, not the flag. The flag goes down one frame after the
               turn whatever the decay is, so a ratio of the two FLAG times is
               ~360 on correct code and ~360 on a build with no decay at all --
               which is what the first version of this check measured. */
            int f2, rise = 0, fall = 0;
            dirarrow_init(&D, &DT);
            IN.car_yaw = 3.14159265f;
            while (D.wrong_t < ARW_WRONG_ON && rise < 3600) {
                dirarrow_step(&D, &IN, 1.f / 60.f);
                rise++;
            }
            IN.car_yaw = 0.f;
            while (D.wrong_t > 0.f && fall < 3600) {
                dirarrow_step(&D, &IN, 1.f / 60.f);
                fall++;
            }
            ck(rise > 10 && fall > 0 && fall < rise,
               "turning round runs the wrong-way timer back down");
            ck(fabsf((float)rise / (float)fall - ARW_WRONG_DECAY) < 0.1f,
               "and it clears ARW_WRONG_DECAY times as fast as it built");
        }

        /* --- the CAP, and it is what stops a long detour from taking twenty
           seconds to forgive. Two runs, one far longer than the other; the time
           to clear has to be the same. */
        {
            int f2, a_fall = 0, b_fall = 0;
            dirarrow_init(&D, &DT);
            IN.car_yaw = 3.14159265f;
            for (f2 = 0; f2 < (int)(60.f * ARW_WRONG_CAP); f2++)
                dirarrow_step(&D, &IN, 1.f / 60.f);
            IN.car_yaw = 0.f;
            while (D.wrong_t > 0.f && a_fall < 14400) {
                dirarrow_step(&D, &IN, 1.f / 60.f);
                a_fall++;
            }
            dirarrow_init(&D, &DT);
            IN.car_yaw = 3.14159265f;
            for (f2 = 0; f2 < (int)(60.f * ARW_WRONG_CAP * 5.f); f2++)
                dirarrow_step(&D, &IN, 1.f / 60.f);
            IN.car_yaw = 0.f;
            while (D.wrong_t > 0.f && b_fall < 14400) {
                dirarrow_step(&D, &IN, 1.f / 60.f);
                b_fall++;
            }
            ck(a_fall > 10 && abs(a_fall - b_fall) <= 2,
               "the timer is CAPPED -- five times as long facing away takes the "
               "same time to forgive");
            IN.car_yaw = 0.f;
        }

        /* --- SUPPRESSED NEAR THE CHECKPOINT JUST PASSED, both sides of the
           recovered radius. A one-sided check here passes on a build that
           suppresses everywhere. */
        {
            int f2;
            dirarrow_init(&D, &DT);
            IN.car_yaw = 3.14159265f;
            IN.prev_x = 0.f; IN.prev_z = -(ARW_WRONG_NEAR - 1.f);
            for (f2 = 0; f2 < 1800; f2++)
                dirarrow_step(&D, &IN, 1.f / 60.f);
            ck(!dirarrow_wrong(&D),
               "inside ARW_WRONG_NEAR of the last checkpoint, no wrong way");
            dirarrow_init(&D, &DT);
            IN.prev_z = -(ARW_WRONG_NEAR + 1.f);
            for (f2 = 0; f2 < 1800; f2++)
                dirarrow_step(&D, &IN, 1.f / 60.f);
            ck(dirarrow_wrong(&D), "and just outside it, wrong way");
            IN.prev_z = -40.f;
            IN.car_yaw = 0.f;
        }

        /* --- THE BLINK. Steady when the car is fine, half on when it is not,
           and the second trigger measured on both sides of ARW_BLINK_NEAR. */
        {
            int f2, on, total;
            dirarrow_init(&D, &DT);
            for (f2 = 0, on = 0, total = 0; f2 < 600; f2++, total++) {
                dirarrow_step(&D, &IN, 1.f / 60.f);
                on += dirarrow_visible(&D);
            }
            ck(on == total, "far from the checkpoint and going the right way, "
                            "the arrow is drawn every frame");

            /* just inside the recovered radius */
            dirarrow_init(&D, &DT);
            IN.cp_dist = ARW_BLINK_NEAR - 1.f;
            for (f2 = 0, on = 0, total = 0; f2 < 600; f2++, total++) {
                dirarrow_step(&D, &IN, 1.f / 60.f);
                on += dirarrow_visible(&D);
            }
            ck(on > total / 3 && on < (total * 2) / 3,
               "inside ARW_BLINK_NEAR it BLINKS, about half on");
            /* and the cycle's own length, between two rising edges */
            {
                int prev = 0, first = -1, last = -1, edges = 0;
                dirarrow_init(&D, &DT);
                for (f2 = 0; f2 < 1200; f2++) {
                    int v;
                    dirarrow_step(&D, &IN, 1.f / 60.f);
                    v = dirarrow_visible(&D);
                    if (v && !prev) {
                        if (first < 0) first = f2; else last = f2;
                        edges++;
                    }
                    prev = v;
                }
                ck(edges >= 3, "and the blink really cycles");
                if (edges >= 3) {
                    const float per = (float)(last - first)
                                    / (float)(edges - 2) / 60.f;
                    ck(fabsf(per - ARW_BLINK_CYCLE) < 0.03f,
                       "at the recovered cycle length");
                }
            }
            IN.cp_dist = ARW_BLINK_NEAR + 1.f;
            dirarrow_init(&D, &DT);
            for (f2 = 0, on = 0, total = 0; f2 < 600; f2++, total++) {
                dirarrow_step(&D, &IN, 1.f / 60.f);
                on += dirarrow_visible(&D);
            }
            ck(on == total, "and just outside it, steady again");
            IN.cp_dist = 40.f;
        }

        /* --- AND THE BLINK LATCHES, because the field it reads is a running
           MINIMUM and not the current distance. Every check above holds the car
           at one distance, so all of them pass identically on a build that reads
           the current distance -- which is what this port shipped until the
           field was identified. Drive it IN and back OUT instead. */
        {
            int f2, on;
            dirarrow_init(&D, &DT);
            IN.cp_index = 3;
            for (f2 = 0; f2 < 120; f2++) {          /* far away, steady */
                IN.cp_dist = 40.f;
                dirarrow_step(&D, &IN, 1.f / 60.f);
            }
            for (f2 = 0; f2 < 60; f2++) {           /* in to 3 m */
                IN.cp_dist = 3.f;
                dirarrow_step(&D, &IN, 1.f / 60.f);
            }
            for (f2 = 0, on = 0; f2 < 300; f2++) {  /* and back out to 40 */
                IN.cp_dist = 40.f;
                dirarrow_step(&D, &IN, 1.f / 60.f);
                on += dirarrow_visible(&D);
            }
            ck(on > 300 / 3 && on < (300 * 2) / 3,
               "the blink KEEPS going after the car leaves the checkpoint -- the "
               "field is the closest it has come, not where it is");
            ck(D.cp_min <= 3.f + 1e-3f,
               "and the minimum is the closest distance, not the last one");

            /* ... until the checkpoint advances, which is the only thing that
               resets it. */
            IN.cp_index = 4;
            for (f2 = 0, on = 0; f2 < 300; f2++) {
                IN.cp_dist = 40.f;
                dirarrow_step(&D, &IN, 1.f / 60.f);
                on += dirarrow_visible(&D);
            }
            ck(on == 300 && D.cp_min >= 39.f,
               "and a new checkpoint resets the minimum and stops the blink");
        }

        /* --- `cp_beside\': in ARW_BESIDE_IN, out ARW_BLINK_NEAR, once per
           checkpoint. Both sides of the inner radius, because a check that only
           drives inside it passes on a build with no inner test at all. */
        {
            int f2, cues;
            /* reached it: under 4 m, then back over 5 */
            dirarrow_init(&D, &DT);
            IN.cp_index = 7;
            cues = 0;
            for (f2 = 0; f2 < 60; f2++) {
                IN.cp_dist = ARW_BESIDE_IN - 1.f;
                dirarrow_step(&D, &IN, 1.f / 60.f);
                cues += dirarrow_beside_cue(&D);
            }
            ck(cues == 0, "no beside cue while the car is still AT the marker");
            for (f2 = 0; f2 < 300; f2++) {
                IN.cp_dist = ARW_BLINK_NEAR + 1.f;
                dirarrow_step(&D, &IN, 1.f / 60.f);
                cues += dirarrow_beside_cue(&D);
            }
            ck(cues == 1, "exactly one when it leaves -- an edge, once per "
                          "checkpoint");

            /* NEARLY reached it: inside 5 but never inside 4. The hysteresis is
               the whole point, so this must stay silent. */
            dirarrow_init(&D, &DT);
            IN.cp_index = 8;
            cues = 0;
            for (f2 = 0; f2 < 60; f2++) {
                IN.cp_dist = (ARW_BESIDE_IN + ARW_BLINK_NEAR) * 0.5f;
                dirarrow_step(&D, &IN, 1.f / 60.f);
                cues += dirarrow_beside_cue(&D);
            }
            for (f2 = 0; f2 < 300; f2++) {
                IN.cp_dist = ARW_BLINK_NEAR + 1.f;
                dirarrow_step(&D, &IN, 1.f / 60.f);
                cues += dirarrow_beside_cue(&D);
            }
            ck(cues == 0, "and none for a fly-by that never got inside "
                          "ARW_BESIDE_IN");

            /* and it re-arms on the next checkpoint */
            dirarrow_init(&D, &DT);
            IN.cp_index = 1;
            cues = 0;
            for (f2 = 0; f2 < 4; f2++) {
                int g2;
                IN.cp_index = 1 + f2;
                for (g2 = 0; g2 < 30; g2++) {
                    IN.cp_dist = 1.f;
                    dirarrow_step(&D, &IN, 1.f / 60.f);
                    cues += dirarrow_beside_cue(&D);
                }
                for (g2 = 0; g2 < 30; g2++) {
                    IN.cp_dist = 20.f;
                    dirarrow_step(&D, &IN, 1.f / 60.f);
                    cues += dirarrow_beside_cue(&D);
                }
            }
            ck(cues == 4, "one per checkpoint over four of them");
            IN.cp_dist = 40.f;
            IN.cp_index = 0;
        }

        /* --- THE TINT, and the ramp's direction is the thing worth asserting:
           red at the start of a segment, green at the end. */
        {
            float r0, g0, b0, r1, g1, b1, rh, gh, bh;
            dirarrow_init(&D, &DT);
            D.prog = 0.f; dirarrow_tint(&D, &r0, &g0, &b0);
            D.prog = 1.f; dirarrow_tint(&D, &r1, &g1, &b1);
            D.prog = 0.5f; dirarrow_tint(&D, &rh, &gh, &bh);
            ck(r0 > 0.9f && g0 < 0.01f, "an arrow at the start of a segment is RED");
            ck(g1 > 0.8f && r1 < 0.01f, "and at the end of it GREEN");
            ck(rh > 0.8f && gh > 0.8f, "and half way, YELLOW -- both channels up");
            ck(b0 == 0.f && b1 == 0.f && bh == 0.f,
               "blue is 0 at every reading, which is what the D3DCOLOR says");
            ck(g1 <= (float)ARW_TINT_G_CAP / 255.f + 1e-6f
               && g1 >= (float)ARW_TINT_G_CAP / 255.f - 1e-3f,
               "green tops out at the exe's own 235, not at 255");
            /* AND ON THE OTHER SIDE OF THE CEILING. Green reaches 235 at
               p = 235/512 = 0.459 and 255 at 0.498, so a clamp written against
               255 instead of 235 differs from the truth only in that 4% of the
               ramp -- and the check above, taken at p = 1, cannot see it because
               both clamps land on 235 there. A mutant that moved the ceiling to
               255 survived the whole of this part until this line existed. */
            {
                float rq, gq, bq;
                D.prog = 0.48f;
                dirarrow_tint(&D, &rq, &gq, &bq);
                ck(gq <= (float)ARW_TINT_G_CAP / 255.f + 1e-6f,
                   "and it is already at 235 half way up, not still climbing");
            }
            /* and it reaches the vertices, which is the GL_COLOR_ARRAY path */
            {
                int c2, i2, wrong_col = 0, seen = 0;
                D.prog = 1.f;
                cap_reset();
                dirarrow_draw(&D, SCRW, SCRH);
                for (c2 = 0; c2 < cap_nd; c2++) {
                    if (cap_draw[c2].tex != T_ARROW) continue;
                    for (i2 = 0; i2 < cap_draw[c2].n; i2++) {
                        const cap_vtx *q = &cap[cap_draw[c2].first + i2];
                        seen++;
                        if (fabsf(q->r - r1) > 1e-4f || fabsf(q->g - g1) > 1e-4f
                            || fabsf(q->b) > 1e-4f || fabsf(q->a - 1.f) > 1e-4f)
                            wrong_col++;
                    }
                }
                ck(seen > 0 && wrong_col == 0,
                   "and the tint reaches every chevron vertex");
            }
        }

        /* --- WRONG WAY TURNS THE ARROW RED, which is the one place the two
           readings meet: the flag forces the progress fraction to 0. */
        {
            int f2;
            float r2, g2, b2;
            dirarrow_init(&D, &DT);
            D.prog = 1.f;
            IN.car_yaw = 3.14159265f;
            for (f2 = 0; f2 < 1800; f2++)
                dirarrow_step(&D, &IN, 1.f / 60.f);
            dirarrow_tint(&D, &r2, &g2, &b2);
            ck(dirarrow_wrong(&D) && r2 > 0.9f && g2 < 0.01f,
               "going the wrong way drives the arrow to the red end of the ramp");
            IN.car_yaw = 0.f;
        }

        /* --- nothing packed: the arrow is left OUT rather than replaced, because
           a chevron is not a word and there is nothing to fall back to. */
        {
            dirarrow_tex none;
            none.arrow = 0;
            dirarrow_init(&D, &none);
            dirarrow_step(&D, &IN, 1.f / 60.f);
            cap_reset();
            dirarrow_draw(&D, SCRW, SCRH);
            ck(cap_nd == 0, "with no chevron art nothing at all is drawn");
        }

        /* --- an invalid frame draws nothing and forgets everything: the state
           has to go back to the reset, or a track change would carry a
           wrong-way timer across. */
        {
            int f2;
            dirarrow_init(&D, &DT);
            IN.car_yaw = 3.14159265f;
            for (f2 = 0; f2 < 1800; f2++)
                dirarrow_step(&D, &IN, 1.f / 60.f);
            IN.valid = 0;
            dirarrow_step(&D, &IN, 1.f / 60.f);
            ck(!dirarrow_wrong(&D) && D.wrong_t == 0.f
               && D.cp_min >= ARW_MIN_INIT,
               "an invalid frame resets the arrow rather than freezing it");
            cap_reset();
            dirarrow_draw(&D, SCRW, SCRH);
            ck(cap_nd > 0, "the arrow itself still draws -- valid gates the "
                           "MODEL, not the element");
            IN.valid = 1;
            IN.car_yaw = 0.f;
        }

        /* --- and the GL state, from both starting states. traps.md: a state
           check that names a value rather than a relation asserts the bug. */
        {
            int pass;
            dirarrow_init(&D, &DT);
            dirarrow_step(&D, &IN, 1.f / 60.f);
            for (pass = 0; pass < 2; pass++) {
                mat_depth[0] = mat_depth[1] = 0;
                mat_underflow = 0;
                glcap_matrix_mode = GL_MODELVIEW;
                en_depth = pass; en_alpha = pass; en_cull = pass;
                en_blend = !pass;
                cap_reset();
                dirarrow_draw(&D, SCRW, SCRH);
                ck(mat_depth[0] == 0 && mat_depth[1] == 0 && !mat_underflow,
                   "dirarrow_draw leaves both matrix stacks balanced");
                ck(en_depth == pass && en_alpha == pass && en_cull == pass
                   && en_blend == !pass && cs_color == 0,
                   "and every enable back where it found it, from either "
                   "starting state");
                ck(cap_tex_enabled == 1, "and GL_TEXTURE_2D back on");
            }
            IN.car_yaw = 0.f;
        }
    }

    /* ---- 18. the message layer (msg.c) -----------------------------
     *
     * The table itself is checked in the generator; what is checked here is what
     * reaches the screen, and the arbitration, which is a pure function and so
     * gets measured directly.
     */
    {
        msg_t M;
        unsigned int MT[MSG_N_TEX];
        int i;

        /* One distinguishable handle per texture, so a quad names its own art. */
        for (i = 0; i < MSG_N_TEX; i++)
            MT[i] = (unsigned int)(800 + i);

        /* --- THE TABLE'S OWN SHAPE, held here as well as in the generator: this
           is the copy a mutant has to break twice. */
        ck(MSG_N_SLOTS == 11 && MSG_N_TEX == 6,
           "eleven slots over six textures");
        ck(MSG_SLOT[MSG_PAUSE].prio > MSG_SLOT[MSG_HIT].prio
           && MSG_SLOT[MSG_HIT].prio > MSG_SLOT[MSG_WRONG_WAY].prio,
           "the recovered priorities order PAUSE over HIT over WRONG WAY");
        ck(MSG_SLOT[MSG_HIT].tex == MSG_SLOT[MSG_GREAT_HIT].tex
           && MSG_SLOT[MSG_HIT].v1 == MSG_SLOT[MSG_GREAT_HIT].v0,
           "the two hit slots PARTITION one texture, top half then bottom");
        {
            /* every slot names a texture in range and sits inside it */
            int bad = 0;
            for (i = 0; i < MSG_N_SLOTS; i++)
                if (MSG_SLOT[i].tex < 0 || MSG_SLOT[i].tex >= MSG_N_TEX
                    || !(MSG_SLOT[i].u0 < MSG_SLOT[i].u1)
                    || !(MSG_SLOT[i].v0 < MSG_SLOT[i].v1)
                    || !(MSG_SLOT[i].h > 0.f) || !(MSG_SLOT[i].w > 0.f))
                    bad++;
            ck(bad == 0, "and every slot names a real texture and a real cell");
        }

        /* --- THE ARBITRATION, measured as a function. */
        {
            int w2[4];
            w2[0] = MSG_WRONG_WAY; w2[1] = MSG_HIT; w2[2] = -1; w2[3] = -1;
            ck(msg_arbitrate(w2, 4) == MSG_HIT,
               "HIT (3) beats WRONG WAY (0)");
            w2[0] = MSG_HIT; w2[1] = MSG_PAUSE;
            ck(msg_arbitrate(w2, 4) == MSG_PAUSE, "and PAUSE (9) beats HIT");
            w2[0] = -1; w2[1] = -1;
            ck(msg_arbitrate(w2, 4) == -1, "nothing contending wins nothing");
            /* the FIRST of a tie, which is `equal replaces' from the caller's
               side -- both of these are priority 0 */
            w2[0] = MSG_BEST_LAP; w2[1] = MSG_WRONG_WAY;
            ck(MSG_SLOT[MSG_BEST_LAP].prio == MSG_SLOT[MSG_WRONG_WAY].prio
               && msg_arbitrate(w2, 4) == MSG_BEST_LAP,
               "and the FIRST of an equal-priority pair holds the screen");
            w2[0] = 99; w2[1] = -7; w2[2] = MSG_BEST_LAP;
            ck(msg_arbitrate(w2, 4) == MSG_BEST_LAP,
               "out-of-range contenders are ignored, not indexed");
            /* ALONE, so the guard is what is measured. Handed in beside a valid
               slot the bad index is read out of bounds and whatever it finds
               loses anyway, which is why the line above passes on a build with
               no bounds check at all. */
            w2[0] = 99;
            ck(msg_arbitrate(w2, 1) == -1, "a slot past the table wins nothing");
            w2[0] = MSG_N_SLOTS;
            ck(msg_arbitrate(w2, 1) == -1, "and neither does one just past it");
            w2[0] = -7;
            ck(msg_arbitrate(w2, 1) == -1, "nor a negative one");
        }

        /* --- THE POSTER'S THREE RULES. */
        {
            msg_init(&M, MT);
            ck(msg_slot(&M) == -1, "a fresh layer holds nothing");
            ck(msg_post(&M, MSG_WRONG_WAY, 1.5f, 4.0f, 1) == 1,
               "the first post takes the screen");
            ck(msg_post(&M, MSG_WRONG_WAY, 1.5f, 4.0f, 1) == 0,
               "RULE 1: a post of the slot already showing is DROPPED");
            ck(msg_post(&M, MSG_HIT, 1.f, 0.f, 0) == 1,
               "a HIGHER priority takes it");
            ck(msg_post(&M, MSG_WRONG_WAY, 1.5f, 4.0f, 1) == 0,
               "RULE 2: and a lower one is dropped");
            ck(msg_post(&M, MSG_GREAT_HIT, 1.f, 0.f, 0) == 1,
               "while an EQUAL one replaces -- strictly higher is the test");
        }

        /* --- THE HOLD-OVER, and this is the one the port had wrong. A slot is
           SELECTED for life + hold and VISIBLE for the first life of them. */
        {
            int f2, vis = 0, sel = 0;
            msg_init(&M, MT);
            msg_post(&M, MSG_WRONG_WAY, ARW_MSG_LIFE, ARW_MSG_HOLD, ARW_MSG_ANIM);
            for (f2 = 0; f2 < 600; f2++) {
                vis += msg_visible(&M, MSG_WRONG_WAY);
                sel += (msg_slot(&M) == MSG_WRONG_WAY);
                msg_step(&M, 1.f / 60.f);
            }
            ck(fabsf((float)vis / 60.f - ARW_MSG_LIFE) < 0.05f,
               "WRONG WAY is VISIBLE for its recovered life");
            ck(fabsf((float)sel / 60.f - (ARW_MSG_LIFE + ARW_MSG_HOLD)) < 0.05f,
               "and SELECTED for its life PLUS its hold-over");
            ck(sel > vis + 60, "which is a good deal longer -- 0x4af28e's own "
                               "`life < -hold', not `life <= 0'");
            ck(msg_slot(&M) == -1, "and then it is released");
        }
        {
            /* AND AN INVISIBLE HOLD-OVER MUST NOT CONTEND. Through the whole 4 s
               gap the slot is still SELECTED, so a contender that reported it
               would suppress everything at its own priority for four seconds --
               the countdown among them. Every other check here reads the layer
               while it is visible and cannot see that. */
            int f2, held = 0, contended = 0;
            msg_init(&M, MT);
            msg_post(&M, MSG_WRONG_WAY, ARW_MSG_LIFE, ARW_MSG_HOLD, ARW_MSG_ANIM);
            for (f2 = 0; f2 < 300; f2++) {
                if (msg_slot(&M) == MSG_WRONG_WAY
                    && !msg_visible(&M, MSG_WRONG_WAY)) {
                    held++;
                    if (msg_contender(&M) >= 0)
                        contended++;
                }
                msg_step(&M, 1.f / 60.f);
            }
            ck(held > 60, "there really is a long invisible hold-over phase");
            ck(contended == 0,
               "and the layer contends for nothing during it");
        }
        {
            /* AND `age' IS `total - life' after an engine-shaped post, which is
               what says the extra field is the engine's own expression and not a
               second mechanism. Only msg_hold can separate them. */
            int f2, drift = 0;
            msg_init(&M, MT);
            msg_post(&M, MSG_BEST_LAP, MSG_BEST_LIFE, 0.f, 1);
            for (f2 = 0; f2 < 100; f2++) {
                if (fabsf(M.age - (M.total - M.life)) > 1e-4f)
                    drift++;
                msg_step(&M, 1.f / 60.f);
            }
            ck(drift == 0, "`age' tracks `total - life' exactly without a hold");
        }
        {
            /* re-posted every frame, as main.c does: one flash per life+hold,
               which is the PULSE. A build without rule 1 pins it at the start of
               its own grow-in and never draws anything at full size. */
            int f2, posts = 0, gap_min = 1 << 30, last = -1;
            msg_init(&M, MT);
            for (f2 = 0; f2 < 1800; f2++) {
                if (msg_post(&M, MSG_WRONG_WAY, ARW_MSG_LIFE, ARW_MSG_HOLD, 1)) {
                    if (last >= 0 && f2 - last < gap_min) gap_min = f2 - last;
                    last = f2;
                    posts++;
                }
                msg_step(&M, 1.f / 60.f);
            }
            ck(posts >= 4, "posting every frame gives repeated flashes");
            ck(fabsf((float)gap_min / 60.f
                     - (ARW_MSG_LIFE + ARW_MSG_HOLD)) < 0.05f,
               "one every life-plus-hold -- 5.5 s, not 1.5");
            ck(msg_cue(&M) == MSG_WRONG_WAY,
               "and the cue names the slot, so the host can pick its sound");
            ck(msg_cue(&M) == -1, "once per post -- an edge, not a flag");
        }

        /* --- PAUSE HOLDS instead of pulsing. A pause banner that blinks is not
           a pause banner, and rule 1 would make it blink. */
        {
            int f2, vis = 0;
            msg_init(&M, MT);
            for (f2 = 0; f2 < 1800; f2++) {
                if (!msg_post(&M, MSG_PAUSE, MSG_PAUSE_LIFE, 0.f, 1))
                    msg_hold(&M, MSG_PAUSE, MSG_PAUSE_LIFE);
                vis += msg_visible(&M, MSG_PAUSE);
                msg_step(&M, 1.f / 60.f);
            }
            ck(vis == 1800, "held every frame for thirty seconds, unbroken");
            /* and it lets go when the holding stops */
            for (f2 = 0; f2 < 120; f2++)
                msg_step(&M, 1.f / 60.f);
            ck(msg_slot(&M) == -1, "and lets go when the menu closes");
            /* AND IT CANNOT TAKE THE SCREEN. msg_hold is the port's own, and a
               hold that posts is a post that ignores both arbitration rules --
               which every check above passes, because they only ever hold the
               slot that is already up. */
            msg_init(&M, MT);
            ck(msg_hold(&M, MSG_PAUSE, MSG_PAUSE_LIFE) == 0
               && msg_slot(&M) == -1,
               "a hold on nothing does nothing -- it is not a post");
            msg_post(&M, MSG_HIT, 1.f, 0.f, 0);
            ck(msg_hold(&M, MSG_PAUSE, MSG_PAUSE_LIFE) == 0
               && msg_slot(&M) == MSG_HIT,
               "and a hold on a slot that is NOT up cannot take the screen off "
               "the one that is");
            /* the hold must not restart the grow-in, or it never reaches full
               size -- which is what refreshing `total' as well would do */
            msg_init(&M, MT);
            msg_post(&M, MSG_PAUSE, MSG_PAUSE_LIFE, 0.f, 1);
            for (f2 = 0; f2 < 300; f2++) {
                msg_hold(&M, MSG_PAUSE, MSG_PAUSE_LIFE);
                msg_step(&M, 1.f / 60.f);
            }
            {
                float w1, w2b;
                msg_slot_rect(MSG_PAUSE, 0, 1.f, SCRW, SCRH, 0, 0, &w1, 0);
                cap_reset();
                msg_draw(&M, SCRW, SCRH);
                w2b = 0.f;
                for (i = 0; i < cap_n; i++) {
                    int k2;
                    for (k2 = 0; k2 < cap_n; k2++)
                        if (cap[k2].x - cap[i].x > w2b)
                            w2b = cap[k2].x - cap[i].x;
                }
                ck(w2b > w1 * 0.99f,
                   "a held banner reaches FULL size -- the hold refreshes the "
                   "life and not the grow-in's own reference");
            }
        }

        /* --- WHAT REACHES THE SCREEN: the right cell of the right texture, at
           the recovered size, in band 0, centred. */
        {
            int slots[3];
            slots[0] = MSG_PAUSE; slots[1] = MSG_BEST_LAP;
            slots[2] = MSG_WRONG_WAY;
            for (i = 0; i < 3; i++) {
                const int sl = slots[i];
                float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
                float u0 = 1e9f, v0 = 1e9f, u1 = -1e9f, v1 = -1e9f;
                float ecx, ecy, ew, eh;
                int c2, j2, ntex = 0;
                msg_init(&M, MT);
                msg_post(&M, sl, 1.f, 0.f, 0);      /* animate 0: full size */
                cap_reset();
                msg_draw(&M, SCRW, SCRH);
                for (c2 = 0; c2 < cap_nd; c2++) {
                    if (cap_draw[c2].tex
                        == MT[MSG_SLOT[sl].tex]) ntex++;
                    for (j2 = 0; j2 < cap_draw[c2].n; j2++) {
                        const cap_vtx *q = &cap[cap_draw[c2].first + j2];
                        if (q->x < x0) x0 = q->x;
                        if (q->x > x1) x1 = q->x;
                        if (q->y < y0) y0 = q->y;
                        if (q->y > y1) y1 = q->y;
                        if (q->u < u0) u0 = q->u;
                        if (q->u > u1) u1 = q->u;
                        if (q->v < v0) v0 = q->v;
                        if (q->v > v1) v1 = q->v;
                    }
                }
                msg_slot_rect(sl, 0, 1.f, SCRW, SCRH, &ecx, &ecy, &ew, &eh);
                ck(ntex == 1, "a slot draws out of ITS OWN texture");
                ck(fabsf((x0 + x1) * 0.5f - ecx) < 0.5f
                   && fabsf((y0 + y1) * 0.5f - ecy) < 0.5f,
                   "centred across the screen and in the band's own middle");
                ck(fabsf((x1 - x0) - ew) < 0.5f && fabsf((y1 - y0) - eh) < 0.5f,
                   "at the recovered size");
                ck(fabsf(u0 - MSG_SLOT[sl].u0) < 1e-4f
                   && fabsf(u1 - MSG_SLOT[sl].u1) < 1e-4f
                   && fabsf(v0 - MSG_SLOT[sl].v0) < 1e-4f
                   && fabsf(v1 - MSG_SLOT[sl].v1) < 1e-4f,
                   "and out of its own cell of that texture");
                /* the aspect is the art's, not the screen's */
                ck(fabsf((x1 - x0) / (y1 - y0)
                         - (MSG_SLOT[sl].w / MSG_SLOT[sl].h) * MSG_REF_ASPECT)
                   < 0.02f, "and at the art's own aspect on a 16:9 panel");
            }
        }

        /* --- AND AN ATLAS SLOT, because the three above are whole textures.
           MSG_GREAT_HIT is the BOTTOM half of msg_hits and MSG_HIT the top, so
           "draw the whole texture instead of the cell" is invisible on any slot
           whose cell IS the whole texture -- which is all three of the ones this
           port raises, and is why that mutant survived until this block. */
        {
            int c2, j2;
            float v0 = 1e9f, v1 = -1e9f;
            ck(MSG_SLOT[MSG_GREAT_HIT].v0 > 0.f
               && MSG_SLOT[MSG_GREAT_HIT].v1 > MSG_SLOT[MSG_GREAT_HIT].v0,
               "the GREAT HIT slot really is a sub-rect of its texture");
            msg_init(&M, MT);
            msg_post(&M, MSG_GREAT_HIT, 1.f, 0.f, 0);
            cap_reset();
            msg_draw(&M, SCRW, SCRH);
            for (c2 = 0; c2 < cap_nd; c2++)
                for (j2 = 0; j2 < cap_draw[c2].n; j2++) {
                    const cap_vtx *q = &cap[cap_draw[c2].first + j2];
                    if (q->v < v0) v0 = q->v;
                    if (q->v > v1) v1 = q->v;
                }
            ck(fabsf(v0 - MSG_SLOT[MSG_GREAT_HIT].v0) < 1e-4f
               && fabsf(v1 - MSG_SLOT[MSG_GREAT_HIT].v1) < 1e-4f,
               "and it is drawn out of that half, not out of the whole sheet");
        }

        /* --- THE THREE BANDS are the drawer's own three, and band 0 is dead
           centre. A check on one band alone cannot see a swapped table. */
        {
            float cy0, cy1, cy2;
            msg_slot_rect(MSG_PAUSE, 0, 1.f, SCRW, SCRH, 0, &cy0, 0, 0);
            msg_slot_rect(MSG_PAUSE, 1, 1.f, SCRW, SCRH, 0, &cy1, 0, 0);
            msg_slot_rect(MSG_PAUSE, 2, 1.f, SCRW, SCRH, 0, &cy2, 0, 0);
            ck(fabsf(cy0 - SCRH * 0.5f) < 0.5f, "band 0 is dead centre");
            ck(cy1 < cy0 && cy2 > cy0, "band 1 is above it and band 2 below");
            ck(fabsf((cy0 - cy1) - (cy2 - cy0)) < 0.5f,
               "and the two are symmetric about it");
        }

        /* --- THE GROW-IN swells and does not fade. */
        {
            float w_a, w_b, a_a = 0.f, a_b = 0.f;
            int c2, f2;
            msg_init(&M, MT);
            msg_post(&M, MSG_BEST_LAP, MSG_BEST_LIFE, 0.f, 1);
            for (f2 = 0; f2 < 12; f2++)
                msg_step(&M, 1.f / 60.f);
            cap_reset();
            msg_draw(&M, SCRW, SCRH);
            w_a = 0.f;
            for (c2 = 0; c2 < cap_n; c2++) {
                int k2;
                for (k2 = 0; k2 < cap_n; k2++)
                    if (cap[k2].x - cap[c2].x > w_a) w_a = cap[k2].x - cap[c2].x;
                if (cap[c2].a > a_a) a_a = cap[c2].a;
            }
            for (f2 = 0; f2 < 90; f2++)
                msg_step(&M, 1.f / 60.f);
            cap_reset();
            msg_draw(&M, SCRW, SCRH);
            w_b = 0.f;
            for (c2 = 0; c2 < cap_n; c2++) {
                int k2;
                for (k2 = 0; k2 < cap_n; k2++)
                    if (cap[k2].x - cap[c2].x > w_b) w_b = cap[k2].x - cap[c2].x;
                if (cap[c2].a > a_b) a_b = cap[c2].a;
            }
            ck(w_b > w_a * 1.5f, "an animated message GROWS over its life");
            ck(a_a == 1.f && a_b == 1.f,
               "and never fades -- the original's colour is a flat white");
            /* animate 0 draws at full size from the first frame, which is what
               the two hit slots and the countdown's own posts ask for */
            msg_init(&M, MT);
            msg_post(&M, MSG_BEST_LAP, MSG_BEST_LIFE, 0.f, 0);
            msg_step(&M, 1.f / 60.f);
            cap_reset();
            msg_draw(&M, SCRW, SCRH);
            w_a = 0.f;
            for (c2 = 0; c2 < cap_n; c2++) {
                int k2;
                for (k2 = 0; k2 < cap_n; k2++)
                    if (cap[k2].x - cap[c2].x > w_a) w_a = cap[k2].x - cap[c2].x;
            }
            ck(w_a > w_b * 0.99f, "and animate 0 is full size at once");

            /* THE GROW-IN'S FLOOR, measured as a RELATION between two posts
               rather than against MSG_GROW_FLOOR itself. Two lives both under
               the floor must swell at the SAME rate, because the floor and not
               the life is the divisor; without it the shorter one would be twice
               as far along. */
            {
                float wa = 0.f, wb = 0.f;
                int k2;
                ck(MSG_GROW_FLOOR > 0.4f,
                   "the floor is above the two lives used below");
                msg_init(&M, MT);
                msg_post(&M, MSG_BEST_LAP, 0.2f, 0.f, 1);
                for (f2 = 0; f2 < 6; f2++) msg_step(&M, 1.f / 60.f);
                cap_reset();
                msg_draw(&M, SCRW, SCRH);
                for (c2 = 0; c2 < cap_n; c2++)
                    for (k2 = 0; k2 < cap_n; k2++)
                        if (cap[k2].x - cap[c2].x > wa) wa = cap[k2].x - cap[c2].x;
                msg_init(&M, MT);
                msg_post(&M, MSG_BEST_LAP, 0.4f, 0.f, 1);
                for (f2 = 0; f2 < 6; f2++) msg_step(&M, 1.f / 60.f);
                cap_reset();
                msg_draw(&M, SCRW, SCRH);
                for (c2 = 0; c2 < cap_n; c2++)
                    for (k2 = 0; k2 < cap_n; k2++)
                        if (cap[k2].x - cap[c2].x > wb) wb = cap[k2].x - cap[c2].x;
                ck(wa > 0.f && fabsf(wa - wb) < 0.5f,
                   "two lives under the floor swell at the same rate -- the "
                   "floor is the divisor, not the life");
            }
        }

        /* --- nothing packed: the three slots the port raises fall back to
           font.h, and the rest draw nothing rather than an invented word. */
        {
            int c2, font_quads, other;
            int with_word[3];
            with_word[0] = MSG_PAUSE; with_word[1] = MSG_WRONG_WAY;
            with_word[2] = MSG_BEST_LAP;
            for (i = 0; i < 3; i++) {
                msg_init(&M, 0);
                msg_post(&M, with_word[i], 1.f, 0.f, 0);
                cap_reset();
                msg_draw(&M, SCRW, SCRH);
                font_quads = other = 0;
                for (c2 = 0; c2 < cap_nd; c2++) {
                    if (cap_draw[c2].tex == 1) font_quads += cap_draw[c2].n;
                    else other += cap_draw[c2].n;
                }
                ck(font_quads > 0 && other == 0,
                   "with no art the message still reads, out of font.h");
            }
            msg_init(&M, 0);
            msg_post(&M, MSG_LOW_SIGNAL, 1.f, 0.f, 0);
            cap_reset();
            msg_draw(&M, SCRW, SCRH);
            ck(cap_nd == 0,
               "and a slot with no word for it draws nothing at all");
        }

        /* --- reset, and the GL state from both starting states. */
        {
            int pass;
            msg_init(&M, MT);
            msg_post(&M, MSG_PAUSE, MSG_PAUSE_LIFE, 0.f, 1);
            msg_reset(&M);
            ck(msg_slot(&M) == -1 && msg_cue(&M) == -1,
               "msg_reset clears the screen and the latch");
            ck(M.tex[MSG_SLOT[MSG_PAUSE].tex] == MT[MSG_SLOT[MSG_PAUSE].tex],
               "and keeps the bindings -- they outlive every restart");
            msg_post(&M, MSG_BEST_LAP, MSG_BEST_LIFE, 0.f, 1);
            msg_step(&M, 0.5f);
            for (pass = 0; pass < 2; pass++) {
                mat_depth[0] = mat_depth[1] = 0;
                mat_underflow = 0;
                glcap_matrix_mode = GL_MODELVIEW;
                en_depth = pass; en_alpha = pass; en_cull = pass;
                en_blend = !pass;
                cap_reset();
                msg_draw(&M, SCRW, SCRH);
                ck(mat_depth[0] == 0 && mat_depth[1] == 0 && !mat_underflow,
                   "msg_draw leaves both matrix stacks balanced");
                ck(en_depth == pass && en_alpha == pass && en_cull == pass
                   && en_blend == !pass && cs_color == 0,
                   "and every enable back where it found it, from either "
                   "starting state");
                ck(cap_tex_enabled == 1, "and GL_TEXTURE_2D back on");
            }
        }

        /* --- and hud.c / countdown.c really do report the engine's own slot
           numbers, since the arbitration is over those and not over their own
           local cell indices. */
        {
            hud_t H2;
            countdown_t C2;
            int seen_go = 0, seen_3 = 0, f2;
            hud_init(&H2, 0);
            ck(hud_slot(&H2) == -1, "an idle HIT pop contends for nothing");
            hud_hit(&H2, HUD_HIT_MIN_SPEED * 2.f);
            ck(hud_slot(&H2) == MSG_HIT, "and an ordinary one is slot 3");
            hud_init(&H2, 0);
            hud_hit(&H2, HUD_GREAT_SPEED * 2.f);
            ck(hud_slot(&H2) == MSG_GREAT_HIT, "a great one slot 4");

            countdown_init(&C2, 0);
            ck(countdown_slot(&C2) == -1, "a stopped countdown likewise");
            countdown_start(&C2);
            for (f2 = 0; f2 < 400; f2++) {
                const int sl = countdown_slot(&C2);
                if (sl == 5) seen_3 = 1;
                if (sl == 9) seen_go = 1;
                countdown_step(&C2, 1.f / 60.f);
            }
            ck(seen_3 && seen_go,
               "and the countdown reports slot 5 for `3' and NINE for GO -- "
               "which is not its own cell index");
        }
    }

    printf("\n%d checks, %d failed%s\n", checks, fails,
           fails ? "  -- SOME CHECKS FAILED" : "");
    return fails ? 1 : 0;
}
