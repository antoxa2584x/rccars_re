/*
 * chartest.c -- the tracks' people, animals and road cars, against the REAL
 * packed .chr files, the REAL collision grids and the REAL char_data.h.
 *
 * Built the way vis_test is: char.c compiled against testgl/, the RECORDING
 * stand-in for vitaGL, so a check can read back what actually went through the
 * pipe. The stub here goes further than vis_test's in one place that matters --
 * it keeps a REAL MATRIX STACK over glPushMatrix / glTranslatef / glRotatef /
 * glScalef / glMultMatrixf, because a character is placed entirely through
 * those calls and a stub that discarded them could see the vertices and not
 * where they went. That is the same lesson glTexImage2D, glMultMatrixf and
 * glBlendFunc each taught this project once already.
 *
 * Build (see BUILD.md):
 *   gcc -I. -Itestgl -O2 ../rccars_re/chartest.c char.c scene.c col.c \
 *       carani.c rb.c contact.c collide.c rlog.c -lm -o chartest
 */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

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

/* ------------------------------------------------------------- the GL stub -- */

#define MAXV 400000
#define MAXD 4000

typedef struct {
    int first, count;
    GLuint tex;
    float m[16];                    /* the modelview AT THE DRAW */
    /*
     * THE CLIENT POINTER AND HOW MUCH OF IT THIS DRAW COVERS, kept because on
     * hardware that is what the GPU is handed: the custom vitaGL is built with
     * DRAW_SPEEDHACK=2 and stops copying a draw's vertices into a mapped temp
     * once they exceed 32 KB (ffp.c:1553) -- past that line GXM reads the app's
     * own memory at flush, not at the draw. So two draws in one frame that share
     * a buffer are one bug however right the vertices looked when they were
     * written, and a recorder that only copies them cannot see it. Part 14.
     */
    const void *vp;
    size_t vbytes;
    int stride;
} drawrec;

static struct {
    float pos[MAXV][3];
    float uv[MAXV][2];
    int n_verts;
    drawrec d[MAXD];
    int n_draws;
    int overflow;
} cap;

static const float *vp, *tp;
static int vstride, tstride;
static int cs_vertex, cs_tex, tex2d_on, alpha_on;
static GLuint bound_tex;

static float mstack[32][16];
static int msp;

static void m_id(float *m)
{
    memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void m_mul(float *r, const float *a, const float *b)
{
    int i, j, k;
    float t[16];
    for (j = 0; j < 4; j++)
        for (i = 0; i < 4; i++) {
            float s = 0.0f;
            for (k = 0; k < 4; k++) s += a[k * 4 + i] * b[j * 4 + k];
            t[j * 4 + i] = s;
        }
    memcpy(r, t, sizeof t);
}

static void mv_apply(const float *m) { m_mul(mstack[msp], mstack[msp], m); }

void glPushMatrix(void)
{
    if (msp < 31) { memcpy(mstack[msp + 1], mstack[msp], 64); msp++; }
}
void glPopMatrix(void) { if (msp > 0) msp--; }
void glMultMatrixf(const float *m) { mv_apply(m); }
void glLoadIdentity(void) { m_id(mstack[msp]); }
void glLoadMatrixf(const float *m) { memcpy(mstack[msp], m, 64); }
void glMatrixMode(GLenum mode) { (void)mode; }

void glTranslatef(float x, float y, float z)
{
    float m[16];
    m_id(m);
    m[12] = x; m[13] = y; m[14] = z;
    mv_apply(m);
}

void glScalef(float x, float y, float z)
{
    float m[16];
    m_id(m);
    m[0] = x; m[5] = y; m[10] = z;
    mv_apply(m);
}

void glRotatef(float a, float x, float y, float z)
{
    float m[16], c, s, l;
    l = sqrtf(x * x + y * y + z * z);
    if (l < 1e-9f) return;
    x /= l; y /= l; z /= l;
    c = cosf(a * 3.14159265358979f / 180.0f);
    s = sinf(a * 3.14159265358979f / 180.0f);
    m_id(m);
    m[0] = x * x * (1 - c) + c;
    m[1] = y * x * (1 - c) + z * s;
    m[2] = x * z * (1 - c) - y * s;
    m[4] = x * y * (1 - c) - z * s;
    m[5] = y * y * (1 - c) + c;
    m[6] = y * z * (1 - c) + x * s;
    m[8] = x * z * (1 - c) + y * s;
    m[9] = y * z * (1 - c) - x * s;
    m[10] = z * z * (1 - c) + c;
    mv_apply(m);
}

void glGenTextures(GLsizei n, GLuint *out)
{
    static GLuint next = 1;
    GLsizei i;
    for (i = 0; i < n; i++) out[i] = next++;
}
void glDeleteTextures(GLsizei n, const GLuint *ids) { (void)n; (void)ids; }
void glBindTexture(GLenum t, GLuint id) { (void)t; bound_tex = id; }
void glTexImage2D(GLenum t, GLint lvl, GLint ifmt, GLsizei w, GLsizei h,
                  GLint b, GLenum f, GLenum ty, const void *px)
{ (void)t; (void)lvl; (void)ifmt; (void)w; (void)h; (void)b; (void)f;
  (void)ty; (void)px; }
void glTexParameteri(GLenum t, GLenum p, GLint v) { (void)t; (void)p; (void)v; }
void glGenerateMipmap(GLenum t) { (void)t; }
void glEnable(GLenum c)
{ if (c == GL_TEXTURE_2D) tex2d_on = 1;
  if (c == GL_ALPHA_TEST) alpha_on = 1; }
void glDisable(GLenum c)
{ if (c == GL_TEXTURE_2D) tex2d_on = 0;
  if (c == GL_ALPHA_TEST) alpha_on = 0; }
void glEnableClientState(GLenum c)
{ if (c == GL_VERTEX_ARRAY) cs_vertex = 1;
  if (c == GL_TEXTURE_COORD_ARRAY) cs_tex = 1; }
void glDisableClientState(GLenum c)
{ if (c == GL_VERTEX_ARRAY) cs_vertex = 0;
  if (c == GL_TEXTURE_COORD_ARRAY) cs_tex = 0; }
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void *p)
{ (void)size; (void)type; vp = (const float *)p; vstride = stride / 4; }
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *p)
{ (void)size; (void)type; tp = (const float *)p; tstride = stride / 4; }
void glColorPointer(GLint size, GLenum type, GLsizei stride, const void *p)
{ (void)size; (void)type; (void)stride; (void)p; }
void glBlendFunc(GLenum a, GLenum b) { (void)a; (void)b; }
void glPolygonOffset(GLfloat f, GLfloat u) { (void)f; (void)u; }
void glDepthMask(GLboolean b) { (void)b; }
/* scene.c's transition bands set this; nothing here draws one, so a no-op is
   the honest stub -- vis_test part 15 is where it is recorded and checked. */
void glDepthFunc(GLenum f) { (void)f; }
void glColor4f(float r, float g, float b, float a)
{ (void)r; (void)g; (void)b; (void)a; }
void glTexEnvi(GLenum a, GLenum b, GLint c) { (void)a; (void)b; (void)c; }
void glGetFloatv(GLenum p, GLfloat *v) { (void)p; m_id(v); }
GLboolean glIsEnabled(GLenum c) { return c == GL_TEXTURE_2D ? tex2d_on : 0; }
void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{ (void)mode; (void)first; (void)count; }
void glActiveTexture(GLenum t) { (void)t; }
void glClientActiveTexture(GLenum t) { (void)t; }
void glGenBuffers(GLsizei n, GLuint *b) { GLsizei i; for (i = 0; i < n; i++) b[i] = 0; }
void glBindBuffer(GLenum t, GLuint b) { (void)t; (void)b; }
void glBufferData(GLenum t, GLsizeiptr s, const void *d, GLenum u)
{ (void)t; (void)s; (void)d; (void)u; }
void glDeleteBuffers(GLsizei n, const GLuint *b) { (void)n; (void)b; }

/*
 * Record a draw: every referenced vertex, put through the CURRENT MODELVIEW, so
 * a check reads world space rather than model space.
 */
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *idxp)
{
    const unsigned short *idx = (const unsigned short *)idxp;
    GLsizei i;
    drawrec *d;
    (void)mode; (void)type;
    if (cap.n_draws >= MAXD || !vp) { cap.overflow = 1; return; }
    d = &cap.d[cap.n_draws++];
    d->first = cap.n_verts;
    d->count = 0;
    d->tex = bound_tex;
    d->vp = vp;
    d->stride = vstride;
    d->vbytes = 0;
    memcpy(d->m, mstack[msp], 64);
    {   /* what GXM would be handed: the top index reached, times the stride */
        GLsizei q;
        unsigned int top = 0;
        for (q = 0; q < count; q++)
            if ((unsigned)idx[q] + 1u > top) top = (unsigned)idx[q] + 1u;
        d->vbytes = (size_t)top * (size_t)vstride * sizeof(float);
    }
    for (i = 0; i < count; i++) {
        const float *v = vp + (size_t)idx[i] * vstride;
        float *o;
        if (cap.n_verts >= MAXV) { cap.overflow = 1; break; }
        o = cap.pos[cap.n_verts];
        o[0] = d->m[0] * v[0] + d->m[4] * v[1] + d->m[8] * v[2] + d->m[12];
        o[1] = d->m[1] * v[0] + d->m[5] * v[1] + d->m[9] * v[2] + d->m[13];
        o[2] = d->m[2] * v[0] + d->m[6] * v[1] + d->m[10] * v[2] + d->m[14];
        if (tp) {
            const float *t = tp + (size_t)idx[i] * tstride;
            cap.uv[cap.n_verts][0] = t[0];
            cap.uv[cap.n_verts][1] = t[1];
        }
        cap.n_verts++;
        d->count++;
    }
}

static void cap_reset(void)
{
    memset(&cap, 0, sizeof cap);
    msp = 0;
    m_id(mstack[0]);
    vp = tp = NULL;
}

/* ----------------------------------------------------------- test plumbing -- */

static int fails, checks;

static void ck(int cond, const char *what, const char *fmt, ...)
{
    va_list ap;
    checks++;
    if (cond) printf("  ok   %-52s ", what);
    else { fails++; printf("  FAIL %-52s ", what); }
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
}

static int near(float a, float b, float tol) { return fabsf(a - b) <= tol; }

static const chr_track_t *track_of(const char *name)
{
    unsigned int i;
    for (i = 0; i < CHR_N_TRACKS; i++)
        if (!strcmp(CHR_TRACKS[i].track, name))
            return &CHR_TRACKS[i];
    return NULL;
}

static int find_inst(const chr_t *c, const char *model)
{
    unsigned int i;
    for (i = 0; i < c->n_inst; i++)
        if (!strcmp(c->inst[i].place->model, model))
            return (int)i;
    return -1;
}

static int cmp_float(const void *a, const void *b)
{
    float x = *(const float *)a, y = *(const float *)b;
    return x < y ? -1 : (x > y ? 1 : 0);
}

static float dist2(const float *a, const float *b)
{
    float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return dx * dx + dy * dy + dz * dz;
}

/* ============================================================= part 1: data = */

static void part1(void)
{
    unsigned int t;
    int total = 0, models = 0, skinned = 0, clips = 0;

    printf("\n== 1. the ten packed .chr files, against char_data.h\n");
    for (t = 0; t < NTRACK; t++) {
        char path[256];
        chr_t c;
        const chr_track_t *ct = track_of(TRACK[t]);
        unsigned int i;
        int all_bound = 1, all_paths = 1;

        snprintf(path, sizeof path, ASSETS "%s.chr", TRACK[t]);
        if (!char_init(&c, path, TRACK[t], NULL)) {
            ck(0, TRACK[t], "char_init failed");
            continue;
        }
        ck(ct && c.n_inst == ct->n, TRACK[t],
           "%u instances placed of %u in char_data.h, %u models, %u paths",
           c.n_inst, ct ? ct->n : 0, c.n_models, c.n_paths);
        total += (int)c.n_inst;
        models += (int)c.n_models;
        for (i = 0; i < c.n_models; i++) {
            const chr_model_t *m = &c.model[i];
            unsigned int b;
            if (!m->n_parts || !m->n_batches) all_bound = 0;
            if (m->n_parts > CHR_MAX_PARTS) all_bound = 0;
            for (b = 0; b < m->n_batches; b++)
                if (m->batch[b].skinned) skinned++;
            clips += (int)m->n_clips;
        }
        for (i = 0; i < c.n_inst; i++)
            if (c.inst[i].place->path && c.inst[i].path < 0)
                all_paths = 0;
        ck(all_bound, "  models are within CHR_MAX_PARTS and non-empty", "");
        ck(all_paths, "  every instance that names a path found it", "");
        char_free(&c);
    }
    ck(total == CHR_N_PLACEMENTS, "all 59 characters place",
       "%d of %d", total, CHR_N_PLACEMENTS);
    ck(skinned > 0 && clips > 0, "skins and clips reached the runtime",
       "%d skinned batches, %d clips over %d model loads", skinned, clips,
       models);

    /*
     * EVERY LOADED VERTEX'S WEIGHTS SUM TO ONE.
     *
     * pack_chars.py normalises them over ALL influences, so a runtime cap below
     * what the data uses drops weight and pulls those vertices toward the model
     * origin -- char.c renormalises what it keeps for exactly that reason. At
     * the shipped CHR_MAX_INF of 5 nothing is ever dropped (the Dog and the
     * Seagull have four vertices each at five influences and everything else
     * stops at three), so this check is quiet by construction. It is here for
     * the cap, not for the data: built with CHR_MAX_INF at 2 and the
     * renormalise removed, it is the only check in the file that fails.
     */
    {
        unsigned int t2;
        float worst = 0.0f;
        int counted = 0;
        for (t2 = 0; t2 < NTRACK; t2++) {
            char path[256];
            chr_t c;
            unsigned int i;
            snprintf(path, sizeof path, ASSETS "%s.chr", TRACK[t2]);
            if (!char_init(&c, path, TRACK[t2], NULL)) continue;
            for (i = 0; i < c.n_models; i++) {
                unsigned int b;
                for (b = 0; b < c.model[i].n_batches; b++) {
                    const chr_batch_t *bt = &c.model[i].batch[b];
                    unsigned int v;
                    if (!bt->skinned) continue;
                    for (v = 0; v < bt->nverts; v++) {
                        float sum = 0.0f;
                        unsigned int q;
                        for (q = 0; q < bt->inf_n[v]; q++)
                            sum += bt->inf_w[v * CHR_MAX_INF + q];
                        if (!bt->inf_n[v]) continue;
                        counted++;
                        if (fabsf(sum - 1.0f) > worst)
                            worst = fabsf(sum - 1.0f);
                    }
                }
            }
            char_free(&c);
        }
        ck(counted > 1000 && worst < 1e-4f,
           "every skinned vertex's weights sum to one",
           "worst %.5f over %d vertices, cap %d", worst, counted,
           CHR_MAX_INF);
    }
}

/* ========================================================= part 2: sampling = */

static void part2(void)
{
    float k3[6] = { 0.0f, 10.0f, 1.0f, 20.0f, 2.0f, 40.0f };
    float k1[2] = { 0.5f, 7.0f };
    chr_chan_t c3, c1, c0;

    printf("\n== 2. chr_sample\n");
    c3.part = c3.comp = 0; c3.n = 3; c3.k = k3;
    c1.part = c1.comp = 0; c1.n = 1; c1.k = k1;
    c0.part = c0.comp = 0; c0.n = 0; c0.k = NULL;

    ck(near(chr_sample(&c3, -5.0f), 10.0f, 1e-5f), "clamps below the first key",
       "%.3f", chr_sample(&c3, -5.0f));
    ck(near(chr_sample(&c3, 99.0f), 40.0f, 1e-5f), "clamps above the last key",
       "%.3f", chr_sample(&c3, 99.0f));
    /* A LOPSIDED u, not the midpoint: an inverted interpolation fraction is
       invisible at 0.5, which is the trap this project has hit before. */
    ck(near(chr_sample(&c3, 1.25f), 25.0f, 1e-4f), "lerps at u = 0.25",
       "%.4f, want 25", chr_sample(&c3, 1.25f));
    ck(near(chr_sample(&c3, 1.75f), 35.0f, 1e-4f), "lerps at u = 0.75",
       "%.4f, want 35", chr_sample(&c3, 1.75f));
    ck(near(chr_sample(&c1, -3.0f), 7.0f, 1e-5f)
       && near(chr_sample(&c1, 90.0f), 7.0f, 1e-5f),
       "one key is a constant", "%.3f", chr_sample(&c1, 90.0f));
    ck(chr_sample(&c0, 1.0f) == 0.0f, "no keys is zero, not a read", "");
    ck(chr_sample(NULL, 1.0f) == 0.0f, "a null channel is zero", "");
}

/* ========================================================== part 3: volumes = */

static void part3(void)
{
    chr_volume_t v, cw, absent;
    unsigned int t;
    int inside_own = 0, have_sight = 0;

    printf("\n== 3. MOD_VOLUME -- the y range and the four corners\n");
    memset(&v, 0, sizeof v);
    v.ylo = -1.0f; v.yhi = 2.0f;
    v.qx[0] = 0; v.qz[0] = 0;
    v.qx[1] = 4; v.qz[1] = 0;
    v.qx[2] = 4; v.qz[2] = 2;
    v.qx[3] = 0; v.qz[3] = 2;
    cw = v;                                     /* the other winding */
    cw.qx[1] = 0; cw.qz[1] = 2;
    cw.qx[2] = 4; cw.qz[2] = 2;
    cw.qx[3] = 4; cw.qz[3] = 0;
    memset(&absent, 0, sizeof absent);

    ck(chr_in_volume(&v, 2.0f, 0.0f, 1.0f), "a point inside is inside", "");
    ck(!chr_in_volume(&v, 5.0f, 0.0f, 1.0f), "outside in x is outside", "");
    ck(!chr_in_volume(&v, 2.0f, 0.0f, 3.0f), "outside in z is outside", "");
    ck(!chr_in_volume(&v, 2.0f, 9.0f, 1.0f), "above the y range is outside", "");
    ck(!chr_in_volume(&v, 2.0f, -9.0f, 1.0f), "below the y range is outside", "");
    ck(chr_in_volume(&cw, 2.0f, 0.0f, 1.0f), "the other winding still works",
       "");
    ck(!chr_in_volume(&absent, 0.0f, 0.0f, 0.0f),
       "an absent volume contains nothing", "");

    /*
     * AND THE DECODE ITSELF, against the shipped data: a character's own
     * visibilityVolume has to contain the character. That is a statement about
     * the ten floats being a y range and four (x, z) corners in that order, and
     * it is not something the runtime can arrange for itself -- read the pairs
     * the other way round and the quads collapse.
     */
    for (t = 0; t < CHR_N_TRACKS; t++) {
        unsigned int i;
        for (i = 0; i < CHR_TRACKS[t].n; i++) {
            const chr_place_t *p = &CHR_TRACKS[t].place[i];
            if (!(p->have & CHR_VOL_SIGHT)) continue;
            have_sight++;
            if (chr_in_volume(&p->sight, p->x, p->y, p->z))
                inside_own++;
        }
    }
    ck(have_sight >= 20 && inside_own == have_sight,
       "every visibilityVolume contains its own character",
       "%d of %d", inside_own, have_sight);
}

/* ============================================================ part 4: pose == */

/* The furthest any drawn vertex gets from the instance's own origin. */
static float pose_reach(chr_t *c, unsigned int inst)
{
    const chr_model_t *m = &c->model[c->inst[inst].model];
    unsigned int b, v;
    float worst = 0.0f;
    chr_pose(c, inst);
    for (b = 0; b < m->n_batches; b++) {
        const chr_batch_t *bt = &m->batch[b];
        for (v = 0; v < bt->nverts; v++) {
            const float *src = &bt->verts[v * 5];
            float p[3] = { 0, 0, 0 };
            if (bt->skinned) {
                unsigned int q, n = bt->inf_n[v];
                for (q = 0; q < n; q++) {
                    unsigned int bp = bt->inf_part[v * CHR_MAX_INF + q];
                    float w = bt->inf_w[v * CHR_MAX_INF + q];
                    const float *mm = c->draw[bp];
                    p[0] += w * (mm[0] * src[0] + mm[4] * src[1]
                                 + mm[8] * src[2] + mm[12]);
                    p[1] += w * (mm[1] * src[0] + mm[5] * src[1]
                                 + mm[9] * src[2] + mm[13]);
                    p[2] += w * (mm[2] * src[0] + mm[6] * src[1]
                                 + mm[10] * src[2] + mm[14]);
                }
                if (!n) { p[0] = src[0]; p[1] = src[1]; p[2] = src[2]; }
            } else {
                const float *mm = c->draw[bt->part];
                p[0] = mm[0] * src[0] + mm[4] * src[1] + mm[8] * src[2] + mm[12];
                p[1] = mm[1] * src[0] + mm[5] * src[1] + mm[9] * src[2] + mm[13];
                p[2] = mm[2] * src[0] + mm[6] * src[1] + mm[10] * src[2] + mm[14];
            }
            {
                float d = sqrtf(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
                if (d > worst) worst = d;
            }
        }
    }
    return worst;
}

static void part4(void)
{
    chr_t c;
    unsigned int i;

    printf("\n== 4. the pose: the skeleton, the skin, and the bind pose\n");
    if (!char_init(&c, ASSETS "beach_3.chr", "beach_3", NULL)) {
        ck(0, "beach_3.chr", "will not load");
        return;
    }
    for (i = 0; i < c.n_models; i++) {
        const chr_model_t *m = &c.model[i];
        int inst = find_inst(&c, m->name);
        float model_r = 0.0f, worst = 0.0f, worst_t = 0.0f;
        unsigned int a;
        int cl;
        if (inst < 0) continue;
        for (a = 0; a < 3; a++) {
            float lo = fabsf(m->bmin[a]), hi = fabsf(m->bmax[a]);
            if (lo > model_r) model_r = lo;
            if (hi > model_r) model_r = hi;
        }
        /*
         * THE SKIN IS BOUND TO THE RIGHT BONES, and this is what says so.
         *
         * Every clip, sampled across its whole length: no vertex may leave a
         * ball twice the model's own bind-pose reach. One wrong bone index puts
         * a limb at another joint's transform and throws vertices metres away;
         * so does reading the 0x5410 rest pose in the 0x540B T,S,R order, which
         * swaps a rotation in DEGREES for a scale.
         */
        for (cl = 0; cl < (int)m->n_clips; cl++) {
            int k;
            c.inst[inst].clip = cl;
            for (k = 0; k <= 12; k++) {
                float r;
                c.inst[inst].clip_t = m->clip[cl].duration * (float)k / 12.0f;
                r = pose_reach(&c, (unsigned)inst);
                if (r > worst) { worst = r; worst_t = c.inst[inst].clip_t; }
            }
        }
        if (!m->n_clips) {
            c.inst[inst].clip = -1;
            worst = pose_reach(&c, (unsigned)inst);
        }
        ck(worst > 0.01f && worst < model_r * 2.0f + 0.5f, m->name,
           "reach %.3f m over %u clips against a bind reach of %.3f (t=%.2f)",
           worst, m->n_clips, model_r, worst_t);
    }

    /*
     * THE SKIN MOVES, AND IT DOES NOT TEAR.
     *
     * The reach bound above is a bound: it passes on a skin that ignores every
     * influence but the first, and on one that puts every vertex on part 0. Two
     * checks that cannot:
     *
     *   MOVES -- over its own clip a skinned vertex has to leave the bind pose
     *   by something worth animating. A skin bound entirely to the root part
     *   draws a statue, and a statue passes every bound.
     *
     *   IS A BLEND -- for a good share of the blended vertices, the full
     *   weighted sum has to differ from what the DOMINANT influence alone
     *   would give. That is the property "take only the first influence"
     *   removes, and it is a statement about the packed weights rather than
     *   about any particular pose. Part 9 then reads the drawn vertices back
     *   through the recorder and holds char.c's own blend to this one.
     *
     *   A tear bound was tried here first and is not a check: the Man's worst
     *   pair of near neighbours on different bones is two vertices 2.9 cm apart
     *   on DIFFERENT FINGERS, which are supposed to separate. Near in space is
     *   not the same surface.
     */
    for (i = 0; i < c.n_models; i++) {
        const chr_model_t *m = &c.model[i];
        int inst = find_inst(&c, m->name);
        unsigned int b;
        float spread = 0.0f;
        int multi = 0, differ = 0;
        if (inst < 0 || !m->n_clips) continue;
        c.inst[inst].clip = 0;
        c.inst[inst].clip_t = m->clip[0].duration * 0.37f;
        chr_pose(&c, (unsigned)inst);
        for (b = 0; b < m->n_batches; b++) {
            const chr_batch_t *bt = &m->batch[b];
            unsigned int v;
            if (!bt->skinned) continue;
            for (v = 0; v < bt->nverts; v++) {
                const float *sv = &bt->verts[v * 5];
                float full[3] = { 0, 0, 0 }, dom[3];
                unsigned int q, bp0;
                float d;
                if (bt->inf_n[v] < 2) continue;
                multi++;
                for (q = 0; q < bt->inf_n[v]; q++) {
                    unsigned int bp = bt->inf_part[v * CHR_MAX_INF + q];
                    float wt = bt->inf_w[v * CHR_MAX_INF + q];
                    const float *mm = c.draw[bp];
                    full[0] += wt * (mm[0] * sv[0] + mm[4] * sv[1]
                                     + mm[8] * sv[2] + mm[12]);
                    full[1] += wt * (mm[1] * sv[0] + mm[5] * sv[1]
                                     + mm[9] * sv[2] + mm[13]);
                    full[2] += wt * (mm[2] * sv[0] + mm[6] * sv[1]
                                     + mm[10] * sv[2] + mm[14]);
                }
                bp0 = bt->inf_part[v * CHR_MAX_INF];
                {
                    const float *mm = c.draw[bp0];
                    dom[0] = mm[0] * sv[0] + mm[4] * sv[1] + mm[8] * sv[2]
                             + mm[12];
                    dom[1] = mm[1] * sv[0] + mm[5] * sv[1] + mm[9] * sv[2]
                             + mm[13];
                    dom[2] = mm[2] * sv[0] + mm[6] * sv[1] + mm[10] * sv[2]
                             + mm[14];
                }
                d = sqrtf((full[0] - dom[0]) * (full[0] - dom[0])
                          + (full[1] - dom[1]) * (full[1] - dom[1])
                          + (full[2] - dom[2]) * (full[2] - dom[2]));
                if (d > 0.002f) differ++;
                if (d > spread) spread = d;
            }
        }
        if (!multi) continue;
        ck(spread > 0.002f, "  the blend has something to do",
           "%s: %d blended vertices, worst blend-vs-dominant %.3f m",
           m->name, multi, spread);
        ck(differ * 4 >= multi, "  and the blend is not the dominant bone",
           "%s: %d of %d differ by more than 2 mm, worst %.3f m",
           m->name, differ, multi, spread);
    }

    /*
     * AND THE BIND POSE IS THE IDENTITY -- POSED FROM `bind`, NOT FROM `trs`.
     *
     * rest_inv is the inverse of the composed 0x5438 BIND chain, so composing
     * the model out of each part's own bind transform must give the identity on
     * every part. That is the one check that ties the two halves of a packed
     * part record together, and it fails on either being wrong.
     *
     * THE OLD FORM OF THIS CHECK ASSERTED THE BUG. It posed from `trs`, the
     * node's SAVED 0x540B pose, and demanded the identity there -- which is only
     * true if the skin is bound to the saved pose, which is exactly the mistake:
     * the people's mesh is a T-pose (1.57 m across against 1.66 m tall) that
     * their saved pose is not, so every skinned model drew stuck in its bind
     * layout with only the animated joints moving inside it. So the second half
     * below is as load-bearing as the first: from `trs` the draw matrices must
     * NOT be the identity on a model whose two poses differ, or `bind` has been
     * packed as a copy of `trs` and the check has gone hollow again.
     */
    {
        int worst_part = -1, trs_differs = 0, differ_models = 0;
        float worst = 0.0f;
        for (i = 0; i < c.n_models; i++) {
            const chr_model_t *m = &c.model[i];
            int inst = find_inst(&c, m->name);
            unsigned int p, e;
            int poses_differ = 0;
            float trs_worst = 0.0f;
            if (inst < 0) continue;
            for (p = 0; p < m->n_parts; p++)
                for (e = 0; e < 9; e++)
                    if (fabsf(m->part[p].bind[e] - m->part[p].trs[e]) > 1e-4f)
                        poses_differ = 1;

            chr_pose_bind(&c, (unsigned)inst);
            for (p = 0; p < m->n_parts && p < CHR_MAX_PARTS; p++) {
                float id[16];
                m_id(id);
                for (e = 0; e < 16; e++) {
                    float d = fabsf(c.draw[p][e] - id[e]);
                    if (d > worst) { worst = d; worst_part = (int)p; }
                }
            }

            if (!poses_differ) continue;
            differ_models++;
            c.inst[inst].clip = -1;             /* no clip: the saved pose */
            chr_pose(&c, (unsigned)inst);
            for (p = 0; p < m->n_parts && p < CHR_MAX_PARTS; p++) {
                float id[16];
                m_id(id);
                for (e = 0; e < 16; e++) {
                    float d = fabsf(c.draw[p][e] - id[e]);
                    if (d > trs_worst) trs_worst = d;
                }
            }
            if (trs_worst > 1e-2f) trs_differs++;
        }
        ck(worst < 1e-3f, "posed from bind, draw[] is the identity everywhere",
           "worst element %.2e on part %d", worst, worst_part);
        ck(differ_models > 0 && trs_differs == differ_models,
           "  and posed from trs it is NOT (bind is a real second pose)",
           "%d of %d models whose poses differ move", trs_differs,
           differ_models);
    }
    char_free(&c);
}

/* ================================================== part 5: which way it faces */

static void part5(void)
{
    unsigned int t;
    int checked = 0, ok_toes = 0, axis_ok = 0, models = 0, turned = 0,
        turned_ok = 0;

    printf("\n== 5. the forward axis, and the yaw offset derived from it\n");
    for (t = 0; t < NTRACK; t++) {
        char path[256];
        chr_t c;
        unsigned int i;
        snprintf(path, sizeof path, ASSETS "%s.chr", TRACK[t]);
        if (!char_init(&c, path, TRACK[t], NULL)) continue;
        for (i = 0; i < c.n_models; i++) {
            const chr_model_t *m = &c.model[i];
            int inst = find_inst(&c, m->name);
            unsigned int p;
            int foot = -1;
            models++;
            /* an axis-aligned offset, or the model is drawn at an angle to
               every heading it is ever given */
            if (near(m->yaw_off, 0.0f, 1e-3f)
                || near(fabsf(m->yaw_off), 90.0f, 1e-3f)
                || near(fabsf(m->yaw_off), 180.0f, 1e-3f))
                axis_ok++;
            if (fabsf(m->yaw_off) > 1e-3f) {
                turned++;
                /* the models that are turned are the CLIPLESS ones that are
                   longer across X than Z -- the road cars, whose headlamps put
                   their front at -X */
                if (!m->n_clips
                    && (m->bmax[0] - m->bmin[0]) > (m->bmax[2] - m->bmin[2]))
                    turned_ok++;
            }
            if (inst < 0) continue;
            for (p = 0; p < m->n_parts && p < CHR_MAX_PARTS; p++) {
                const char *n = m->part[p].name;
                if (strstr(n, "FOOT") && strstr(n, "eff")
                    && m->part[p].parent >= 0) { foot = (int)p; break; }
            }
            if (foot < 0) continue;
            /*
             * THE WITNESS, IN MODEL SPACE. The toe's LOCAL translation is not
             * the thing -- the ankle it hangs off is rotated, so a local +Z can
             * be a model -X. Posing with no clip composes the nodes' own 0x540B
             * chain, which is the bind pose, and the toe has to come out
             * forward of its own ankle there.
             */
            c.inst[inst].clip = -1;
            chr_pose(&c, (unsigned)inst);
            checked++;
            if (c.world[foot][14] > c.world[m->part[foot].parent][14]
                && near(m->yaw_off, 0.0f, 1e-3f))
                ok_toes++;
        }
        char_free(&c);
    }
    ck(models > 0 && axis_ok == models,
       "every yaw_off is axis aligned", "%d of %d", axis_ok, models);
    ck(checked >= 5 && ok_toes == checked,
       "a model with feet keeps yaw_off 0 and its toes at +Z",
       "%d of %d", ok_toes, checked);
    ck(turned >= 3 && turned_ok == turned,
       "the turned models are the clipless ones long in X",
       "%d of %d", turned_ok, turned);

    /*
     * AND IT HAS TO REACH THE DRAW. Everything above reads yaw_off out of the
     * loaded model; nothing so far says char_draw applies it. Drawn at yaw 0, a
     * road car -- which is 4.5 m along its own X and 2 m along its own Z -- has
     * to come out LONGER IN WORLD Z than in world X, and it only does if the
     * offset is in the matrix.
     */
    {
        chr_t c;
        if (char_init(&c, ASSETS "urban_1.chr", "urban_1", NULL)) {
            int hi = find_inst(&c, "Hammer");
            if (hi >= 0) {
                unsigned int k;
                float lo[3], up[3];
                int v;
                /* char_draw decides for itself who is in range, so the others
                   are moved out of it rather than un-flagged. */
                for (k = 0; k < c.n_inst; k++) {
                    c.inst[k].x = 10000.0f + 100.0f * (float)k;
                    c.inst[k].y = 0.0f;
                    c.inst[k].z = 10000.0f;
                }
                c.inst[hi].yaw = 0.0f;
                c.inst[hi].x = c.inst[hi].y = c.inst[hi].z = 0.0f;
                c.inst[hi].pitch = c.inst[hi].roll = 0.0f;
                cap_reset();
                {
                    float at[3];
                    at[0] = c.inst[hi].x; at[1] = c.inst[hi].y;
                    at[2] = c.inst[hi].z;
                    char_draw(&c, at);
                }
                lo[0] = lo[1] = lo[2] = 1e9f;
                up[0] = up[1] = up[2] = -1e9f;
                for (v = 0; v < cap.n_verts; v++) {
                    int a;
                    for (a = 0; a < 3; a++) {
                        if (cap.pos[v][a] < lo[a]) lo[a] = cap.pos[v][a];
                        if (cap.pos[v][a] > up[a]) up[a] = cap.pos[v][a];
                    }
                }
                ck(cap.n_verts > 100 && (up[2] - lo[2]) > (up[0] - lo[0]) * 1.5f,
                   "  and drawn at yaw 0 a road car lies along Z",
                   "world extents x %.2f z %.2f over %d vertices",
                   up[0] - lo[0], up[2] - lo[2], cap.n_verts);
            }
            char_free(&c);
        }
    }
}

/* ============================================================= part 6: dog === */

/*
 * char_step measures CHR_STEP_DIST from the CAR, so a fixture that wants a
 * character to keep running while the car is "gone" has to move the car out of
 * the character's own sight rather than out of the county -- past 60 m the
 * instance is not stepped at all and the check is measuring the cull.
 */
static void step_n(chr_t *c, const float car[3], float speed, int n, float dt)
{
    float fwd[3] = { 0.0f, 0.0f, 1.0f };
    int i;
    for (i = 0; i < n; i++)
        char_step(c, dt, car, fwd, speed);
}

static void part6(void)
{
    chr_t c;
    int di;
    chr_inst_t *d;
    float car[3];
    float home[3];
    float behind[3], front[3];

    printf("\n== 6. the Dog: the cone, the chase, the reach, the way home\n");
    if (!char_init(&c, ASSETS "country_2.chr", "country_2", NULL)) {
        ck(0, "country_2.chr", "will not load");
        return;
    }
    di = find_inst(&c, "Dog");
    if (di < 0) { ck(0, "country_2 has a Dog", ""); char_free(&c); return; }
    d = &c.inst[di];
    home[0] = d->x; home[1] = d->y; home[2] = d->z;

    /* far away: nothing happens */
    car[0] = home[0] + 40.0f; car[1] = home[1]; car[2] = home[2];
    step_n(&c, car, 5.0f, 30, 1.0f / 60.0f);
    ck(d->state == CHR_ST_IDLE && near(d->x, home[0], 1e-3f),
       "40 m away it does not move", "state %d", d->state);
    /*
     * AND PAST CHR_STEP_DIST ITS MACHINE IS NOT STEPPED AT ALL -- while a
     * REPLAYER'S CLOCK STILL IS. Two halves of one rule, and they have to be
     * asserted together: CHR_STEP_DIST bounds the invented state machines,
     * because those are what a character does ABOUT the car, and it must not
     * bound a recorded path, because that is where a character IS at time t.
     *
     * Culling replayers too is what froze beach_1's Man and Woman one frame
     * after the start, on the first sample of a 469 s and a 201 s walk, for the
     * whole race -- reported as two missing people. country_2 has both kinds in
     * one file: a Dog and four Vultures on two recorded .gpf paths.
     *
     * The clip time is what says whether a machine ran; path_t is what says
     * whether a replay did.
     */
    {
        float t0, p0 = -1.0f;
        unsigned int k, replayers = 0;
        int vi = -1;
        char_reset(&c);
        for (k = 0; k < c.n_inst; k++) {
            unsigned int kk = c.inst[k].place->kind;
            if (kk == CHR_KIND_PATH || kk == CHR_KIND_WALKER) {
                replayers++;
                if (vi < 0 && c.inst[k].path >= 0) vi = (int)k;
            }
        }
        car[0] = home[0] + CHR_STEP_DIST * 3.0f;
        car[1] = home[1];
        car[2] = home[2];
        step_n(&c, car, 5.0f, 2, 1.0f / 60.0f);
        t0 = d->clip_t;
        if (vi >= 0) p0 = c.inst[vi].path_t;
        step_n(&c, car, 5.0f, 60, 1.0f / 60.0f);
        ck(c.n_stepped == replayers && near(d->clip_t, t0, 1e-6f),
           "  past CHR_STEP_DIST no MACHINE is stepped",
           "%u stepped of %u replayers, the Dog's clip clock %.4f -> %.4f",
           c.n_stepped, replayers, t0, d->clip_t);
        ck(vi >= 0 && c.inst[vi].path_t > p0 + 0.9f,
           "  and a REPLAYER's clock runs anyway",
           "%s's path_t %.3f -> %.3f over 1.0 s",
           vi >= 0 ? c.inst[vi].place->name : "-", p0,
           vi >= 0 ? c.inst[vi].path_t : -1.0f);
        char_reset(&c);
    }

    /*
     * THE CONE IS LOAD-BEARING and this is the pair that says so: a car at
     * RadFar_Far * 0.7 BEHIND the dog must not be seen, and the same car at
     * RadNear * 0.8 behind MUST be -- the near radius is all round and the far
     * one is not. A version with no cone test passes the first line and fails
     * this one; a version with no near radius fails the second.
     */
    char_reset(&c);
    d->yaw = 0.0f;                              /* facing +Z */
    behind[0] = d->x;
    behind[1] = d->y;
    behind[2] = d->z - CHR_DOG_RADFAR_FAR * 0.7f;
    step_n(&c, behind, 5.0f, 2, 1.0f / 60.0f);
    ck(d->state == CHR_ST_IDLE, "2.1 m directly behind is outside the cone",
       "state %d seen %d", d->state, d->seen);

    char_reset(&c);
    d->yaw = 0.0f;
    behind[2] = d->z - CHR_DOG_RADNEAR * 0.8f;
    step_n(&c, behind, 5.0f, 2, 1.0f / 60.0f);
    ck(d->seen, "0.28 m directly behind is inside RadNear",
       "state %d seen %d", d->state, d->seen);

    /* in front and inside RadFar_Far: it comes */
    char_reset(&c);
    d->yaw = 0.0f;
    front[0] = d->x;
    front[1] = d->y;
    front[2] = d->z + CHR_DOG_RADFAR_FAR * 0.8f;
    step_n(&c, front, 5.0f, 4, 1.0f / 60.0f);
    ck(d->state == CHR_ST_GO, "in front and inside RadFar_Far it gives chase",
       "state %d", d->state);
    {
        float before = fabsf(d->z - front[2]);
        step_n(&c, front, 5.0f, 30, 1.0f / 60.0f);
        ck(fabsf(d->z - front[2]) < before, "and it closes the distance",
           "%.3f -> %.3f m", before, fabsf(d->z - front[2]));
    }
    step_n(&c, front, 5.0f, 240, 1.0f / 60.0f);
    ck(d->state == CHR_ST_ACT, "within RadReach it attacks",
       "state %d, %.3f m out",
       d->state, sqrtf((d->x - front[0]) * (d->x - front[0])
                       + (d->z - front[2]) * (d->z - front[2])));
    /*
     * HOW LONG THE ATTACK LASTS, measured from the moment it starts rather than
     * asked for at a fixed offset. With the car still under its nose the dog
     * goes straight back to attacking, so the state is ACT again a frame later
     * -- the quantity that means anything is the RUN LENGTH, and it has to come
     * out as the recovered TimeAttack rather than as anything else.
     */
    {
        int frames = 0, guard = 0, barks = 0;
        float fwd[3] = { 0, 0, 1 };
        /* Run out the attack that is already in progress, then catch the NEXT
           one from its first frame -- a run length measured from the middle of
           a run is not a run length. */
        while (d->state == CHR_ST_ACT && guard++ < 60 * 20)
            char_step(&c, 1.0f / 60.0f, front, fwd, 5.0f);
        while (d->state != CHR_ST_ACT && guard++ < 60 * 20) {
            char_step(&c, 1.0f / 60.0f, front, fwd, 5.0f);
            if (d->event == CHR_EV_BARK) barks++;
        }
        while (d->state == CHR_ST_ACT && frames < 60 * 20) {
            char_step(&c, 1.0f / 60.0f, front, fwd, 5.0f);
            if (d->event == CHR_EV_BARK) barks++;
            frames++;
        }
        ck(near(frames / 60.0f, CHR_DOG_TIMEATTACK, 0.1f),
           "  and it lasts the recovered TimeAttack",
           "%.2f s against %.2f", frames / 60.0f, CHR_DOG_TIMEATTACK);
        /*
         * AND THE BARK IS AN EDGE. char.c raises a cue and main.c turns it into
         * a sound; over one whole attack there is exactly one, not one per
         * frame -- prop_t.hit's rule, and the same failure it exists to stop.
         */
        ck(barks == 1, "  and it raises exactly one bark for that attack",
           "%d over %.2f s", barks, frames / 60.0f);
    }

    /*
     * The car goes away -- 20 m, which is far outside RadFar_Far's 3 m and well
     * inside CHR_STEP_DIST's 60, so the dog is still being stepped and really
     * does have to decide to go home. Two hundred metres would only prove that
     * an unstepped instance does not move.
     */
    car[0] = home[0] + 20.0f;
    car[1] = home[1];
    car[2] = home[2] + 20.0f;
    step_n(&c, car, 0.0f, 60 * 60, 1.0f / 60.0f);
    ck(d->state == CHR_ST_IDLE, "with the car gone it settles", "state %d",
       d->state);
    {
        float p[3] = { d->x, d->y, d->z };
        const chr_place_t *pl = d->place;
        float want[3];
        want[0] = (pl->have & CHR_MARK_A) ? pl->mark_a.x : pl->x;
        want[1] = p[1];
        want[2] = (pl->have & CHR_MARK_A) ? pl->mark_a.z : pl->z;
        ck(dist2(p, want) < 1.0f, "  back at its resetPlace",
           "%.2f m away", sqrtf(dist2(p, want)));
    }
    char_free(&c);
}

/* ========================================================= part 7: the rest == */

static void part7(void)
{
    chr_t c;
    int si, bi;
    printf("\n== 7. the Seagull and the Crab\n");
    if (!char_init(&c, ASSETS "beach_1.chr", "beach_1", NULL)) {
        ck(0, "beach_1.chr", "will not load");
        return;
    }
    si = find_inst(&c, "Seagull");
    bi = find_inst(&c, "Crab");

    if (si >= 0) {
        chr_inst_t *s = &c.inst[si];
        const chr_place_t *pl = s->place;
        float y0 = s->y;
        float car[3];
        /* the car well outside the visibility volume */
        car[0] = pl->x + 300.0f; car[1] = pl->y; car[2] = pl->z + 300.0f;
        step_n(&c, car, 5.0f, 60 * 5, 1.0f / 60.0f);
        ck(s->state != CHR_ST_FLY, "undisturbed it stays on the ground",
           "state %d", s->state);
        ck((pl->have & CHR_VOL_HOME)
               ? chr_in_volume(&pl->home, s->x, pl->home.ylo + 0.01f, s->z)
               : 1,
           "  and inside its own inhabitVolume", "at %.1f, %.1f", s->x, s->z);

        /* now put the car in the middle of the visibility volume */
        car[0] = (pl->sight.qx[0] + pl->sight.qx[2]) * 0.5f;
        car[2] = (pl->sight.qz[0] + pl->sight.qz[2]) * 0.5f;
        car[1] = (pl->sight.ylo + pl->sight.yhi) * 0.5f;
        ck(chr_in_volume(&pl->sight, car[0], car[1], car[2]),
           "  the fixture's car really is in the volume", "");
        step_n(&c, car, 5.0f, 60 * 8, 1.0f / 60.0f);
        ck(s->state == CHR_ST_FLY, "  startled, it takes off", "state %d",
           s->state);
        ck(s->y > y0 + 0.2f, "  and gains height toward FlightHeight",
           "%.2f -> %.2f m (FlightHeight %.2f)", y0, s->y,
           CHR_SEAGULL_FLIGHTHEIGHT);
    } else {
        ck(0, "beach_1 has a Seagull", "");
    }

    if (bi >= 0) {
        chr_inst_t *b = &c.inst[bi];
        const chr_place_t *pl = b->place;
        float car[3], a[3], bmk[3], p[3];
        a[0] = pl->mark_a.x; a[1] = pl->mark_a.y; a[2] = pl->mark_a.z;
        bmk[0] = pl->mark_b.x; bmk[1] = pl->mark_b.y; bmk[2] = pl->mark_b.z;
        ck((pl->have & CHR_MARK_A) && (pl->have & CHR_MARK_B),
           "the Crab has both holes", "");
        car[0] = (pl->sight.qx[0] + pl->sight.qx[2]) * 0.5f;
        car[2] = (pl->sight.qz[0] + pl->sight.qz[2]) * 0.5f;
        car[1] = (pl->sight.ylo + pl->sight.yhi) * 0.5f;
        {
            /*
             * A crab that shuttles between two holes is somewhere in the middle
             * most of the time, so where it ENDS says nothing. What says
             * something is how close it got to each: it has to REACH both.
             */
            float near_a = 1e9f, near_b = 1e9f, moved = 0.0f;
            float px = b->x, pz = b->z;
            int k;
            for (k = 0; k < 60 * 60; k++) {
                float d;
                step_n(&c, car, 5.0f, 1, 1.0f / 60.0f);
                p[0] = b->x; p[1] = a[1]; p[2] = b->z;
                d = dist2(p, a);
                if (d < near_a) near_a = d;
                p[1] = bmk[1];
                d = dist2(p, bmk);
                if (d < near_b) near_b = d;
                moved += fabsf(b->x - px) + fabsf(b->z - pz);
                px = b->x; pz = b->z;
            }
            ck(moved > 1.0f, "  seen, it runs", "%.1f m of travel", moved);
            ck(near_a < 0.25f && near_b < 0.25f, "  and it reaches BOTH holes",
               "%.2f m from seahole1, %.2f from landhole1",
               sqrtf(near_a), sqrtf(near_b));
        }
    } else {
        ck(0, "beach_1 has a Crab", "");
    }
    char_free(&c);
}

/* ======================================================= part 8: the replays = */

static void part8(void)
{
    chr_t c;
    int wi;
    printf("\n== 8. the recorded walks and drives\n");
    if (!char_init(&c, ASSETS "beach_1.chr", "beach_1", NULL)) {
        ck(0, "beach_1.chr", "will not load");
        return;
    }
    wi = find_inst(&c, "Man");
    if (wi < 0) { ck(0, "beach_1 has a Man", ""); char_free(&c); return; }
    {
        chr_inst_t *w = &c.inst[wi];
        const chr_path_t *pa = &c.path[w->path];
        float car[3];
        unsigned int k;
        float worst = 0.0f;
        int steps = 0;
        float travelled = 0.0f;
        float px = w->x, pz = w->z;

        ck(pa->n > 100 && pa->duration > 10.0f, "the walk is packed",
           "%u samples over %.1f s", pa->n, pa->duration);

        /*
         * REPLAY FIDELITY: at every packed sample's own timestamp the walker
         * must be AT that sample. This is the check that a decimated path and a
         * cursor that walks forward cannot fake -- an off-by-one in the cursor,
         * or reading the .dat's eight floats in the wrong order, moves every
         * one of these.
         */
        for (k = 1; k + 1 < pa->n; k += (pa->n / 200) + 1) {
            float want[3], got[3];
            w->path_t = pa->t[k];
            w->cursor = 0;
            /* The car rides along beside the walker at a stop -- inside
               CHR_STEP_DIST so the replay runs, and at zero speed so the
               run-over test cannot fire. */
            car[0] = w->x + 5.0f; car[1] = w->y; car[2] = w->z;
            char_step(&c, 1e-6f, car, car, 0.0f);
            want[0] = pa->p[k * 3]; want[1] = pa->p[k * 3 + 1];
            want[2] = pa->p[k * 3 + 2];
            got[0] = w->x; got[1] = w->y; got[2] = w->z;
            if (dist2(want, got) > worst) worst = dist2(want, got);
            steps++;
        }
        ck(steps > 50 && worst < 0.01f * 0.01f,
           "  the replay is on the recorded polyline",
           "worst %.4f m over %d samples", sqrtf(worst), steps);

        /* it goes somewhere, and it comes back round */
        char_reset(&c);
        for (k = 0; k < 60 * 60; k++) {
            car[0] = w->x + 5.0f; car[1] = w->y; car[2] = w->z;
            char_step(&c, 1.0f / 60.0f, car, car, 0.0f);
            travelled += sqrtf((w->x - px) * (w->x - px)
                               + (w->z - pz) * (w->z - pz));
            px = w->x; pz = w->z;
        }
        ck(travelled > 5.0f, "  a minute of it covers ground", "%.1f m",
           travelled);
        /*
         * THE WRAP, AND THE CURSOR WITH IT. Driving to the end leaves the
         * cursor at the last sample; wrapping puts the clock back at the
         * beginning, and a cursor that only ever walks FORWARD then reports the
         * last segment for the rest of the lap. So the check is not that the
         * clock wrapped -- it is that the POSITION after the wrap is the
         * position the recording has at that time.
         */
        w->path_t = pa->duration - 0.01f;
        car[0] = w->x + 5.0f; car[1] = w->y; car[2] = w->z;
        char_step(&c, 0.5f, car, car, 0.0f);
        ck(w->path_t < pa->duration * 0.5f, "  and it wraps rather than stopping",
           "t = %.3f of %.1f", w->path_t, pa->duration);
        {
            float want[3], got[3];
            unsigned int j = 0;
            while (j + 1 < pa->n && pa->t[j + 1] < w->path_t) j++;
            want[0] = pa->p[j * 3]; want[1] = pa->p[j * 3 + 1];
            want[2] = pa->p[j * 3 + 2];
            got[0] = w->x; got[1] = w->y; got[2] = w->z;
            ck(sqrtf(dist2(want, got)) < 0.5f,
               "  and it is back at the start of the recording",
               "%.2f m from sample %u", sqrtf(dist2(want, got)), j);
        }
    }
    char_free(&c);

    /* the road cars, whose instances all share one dummy pivot */
    if (char_init(&c, ASSETS "beach_2.chr", "beach_2", NULL)) {
        unsigned int i;
        int moved = 0, apart = 0, road = 0;
        float car[3];
        float px[CHR_MAX_INST], pz[CHR_MAX_INST];
        int k;
        for (i = 0; i < c.n_inst; i++) { px[i] = c.inst[i].x; pz[i] = c.inst[i].z; }
        /* The eye rides with the first road car: everything here is a replay,
           and a replay that is culled is a replay that does not run. */
        /* The car keeps up with the leading road car; a convoy 500 m away is
           a convoy that is not stepped. */
        for (k = 0; k < 60 * 5; k++) {
            unsigned int q;
            car[0] = car[1] = car[2] = 0.0f;
            for (q = 0; q < c.n_inst; q++)
                if (c.inst[q].place->kind == CHR_KIND_PATH) {
                    car[0] = c.inst[q].x; car[1] = c.inst[q].y;
                    car[2] = c.inst[q].z;
                    break;
                }
            char_step(&c, 1.0f / 60.0f, car, car, 0.0f);
        }
        for (i = 0; i < c.n_inst; i++) {
            if (c.inst[i].place->kind != CHR_KIND_PATH) continue;
            road++;
            if (fabsf(c.inst[i].x - px[i]) + fabsf(c.inst[i].z - pz[i]) > 1.0f)
                moved++;
        }
        ck(road >= 6 && moved == road, "every road car leaves the dummy pivot",
           "%d of %d", moved, road);
        /*
         * AND THEY ARE NOT ALL IN THE SAME PLACE. The six instances share one
         * authored position and one authored yaw -- the .sb's pivot for them is
         * a dummy -- so without the per-instance phase the whole convoy would
         * be one truck drawn six times.
         */
        {
            /*
             * SEPARATED ON THE SAME PATH. Counting every pair is not a check:
             * three of the six are on `forw` and three on `back`, which are two
             * different roads, so fifteen of the pairs are apart whatever the
             * phase does. What the per-instance phase is for is the cars
             * SHARING a path, and with it dropped those sit on top of each
             * other.
             */
            unsigned int a, b;
            int same = 0;
            for (a = 0; a < c.n_inst; a++) {
                if (c.inst[a].place->kind != CHR_KIND_PATH) continue;
                for (b = a + 1; b < c.n_inst; b++) {
                    float d;
                    if (c.inst[b].place->kind != CHR_KIND_PATH) continue;
                    if (c.inst[a].path != c.inst[b].path) continue;
                    same++;
                    d = fabsf(c.inst[a].x - c.inst[b].x)
                        + fabsf(c.inst[a].z - c.inst[b].z);
                    if (d > 1.0f) apart++;
                }
            }
            ck(same >= 3 && apart == same,
               "  and the ones sharing a path are spread along it",
               "%d of %d same-path pairs apart", apart, same);
        }
        char_free(&c);
    }
}

/* ============================================================ part 9: draw === */

static void part9(void)
{
    chr_t c;
    float car[3], eye[3];
    int i, near_draws, far_draws;

    printf("\n== 9. the draw, through a real matrix stack\n");
    if (!char_init(&c, ASSETS "country_1.chr", "country_1", NULL)) {
        ck(0, "country_1.chr", "will not load");
        return;
    }
    /* eye on top of the first instance */
    /* Eye and car both on the first instance, at a standstill: everything is
       stepped and everything near is drawn, which is the state the two range
       checks below then move ONE of. */
    eye[0] = c.inst[0].x; eye[1] = c.inst[0].y; eye[2] = c.inst[0].z;
    car[0] = eye[0]; car[1] = eye[1]; car[2] = eye[2];
    step_n(&c, car, 0.0f, 2, 1.0f / 60.0f);

    cap_reset();
    char_draw(&c, eye);
    near_draws = cap.n_draws;
    ck(near_draws > 0 && cap.n_verts > 0, "a character near the eye draws",
       "%d draws, %d vertices, %u instances drawn", cap.n_draws, cap.n_verts,
       c.n_drawn);
    /*
     * WHAT IT LEAVES BEHIND, and this check used to assert the BUG.
     *
     * GL_VERTEX_ARRAY and GL_TEXTURE_COORD_ARRAY are APP-WIDE state: main.c
     * enables both once before the frame loop and every other module relies on
     * it. char_draw enabled them for itself and disabled them on the way out,
     * which left them off for everything drawn after it -- the water, the
     * effects, the sun, the checkpoint arrows and the whole of ui.c. The world
     * and the menu stopped drawing while the characters carried on, and the
     * check here read `!cs_vertex && !cs_tex` and called it correct.
     *
     * So: driven from BOTH entry states, and asserting RESTORED rather than any
     * particular value -- a version that hardcodes either one cannot pass both
     * halves. Same shape as vis_test's ck_state_restored, and the same lesson:
     * a recorder sees state DURING a draw and is blind to what a call leaves
     * behind unless something asks.
     */
    {
        int i2, ok_all = 1;
        for (i2 = 0; i2 < 2; i2++) {
            int want = i2;
            cs_vertex = cs_tex = want;
            alpha_on = want;
            char_draw(&c, eye);
            if (cs_vertex != want || cs_tex != want || alpha_on != 1)
                ok_all = 0;
        }
        ck(ok_all, "  the client arrays are left exactly as they were found",
           "vertex %d tex %d alpha %d after entering with 1/1/1",
           cs_vertex, cs_tex, alpha_on);
    }
    ck(msp == 0, "  and the matrix stack is balanced", "depth %d", msp);

    /*
     * WHERE THE VERTICES LANDED. Every one has to be within the model's own
     * reach of the instance it belongs to -- which is the check that the two
     * matrices a character is drawn under (the instance placement and the
     * animated part transform) are composed in the right order and the right
     * way round. Swapping them, or dropping the instance translate, puts the
     * whole character at the world origin.
     */
    {
        float worst = 1e9f, worst_near = 0.0f;
        int v;
        for (v = 0; v < cap.n_verts; v++) {
            float best = 1e9f;
            unsigned int k;
            for (k = 0; k < c.n_inst; k++) {
                float p[3];
                float d;
                p[0] = c.inst[k].x; p[1] = c.inst[k].y; p[2] = c.inst[k].z;
                d = dist2(cap.pos[v], p);
                if (d < best) best = d;
            }
            if (best > worst_near) worst_near = best;
        }
        (void)worst;
        ck(worst_near < 25.0f, "  every vertex is near an instance",
           "worst %.2f m", sqrtf(worst_near));
    }

    /*
     * WHAT WENT DOWN THE PIPE IS WHAT THE SKIN SAYS.
     *
     * Everything above reads the pose; this reads the VERTICES char.c actually
     * submitted and holds them to a blend computed here, out of the packed
     * weights and c->draw. Independent of char.c's own skinning loop, which is
     * the only thing that can catch a blend that quietly drops its tail
     * influences or sends every vertex through one bone.
     */
    {
        unsigned int k;
        float worst = 0.0f;
        int compared = 0;
        for (k = 0; k < c.n_inst; k++) {
            const chr_model_t *m;
            unsigned int b;
            int d;
            if (!c.inst[k].drawn) continue;
            m = &c.model[c.inst[k].model];
            chr_pose(&c, k);
            for (b = 0; b < m->n_batches; b++) {
                const chr_batch_t *bt = &m->batch[b];
                if (!bt->skinned || !bt->nidx) continue;
                /* find the recorded draw with this batch's triangle count */
                for (d = 0; d < cap.n_draws; d++) {
                    unsigned int q;
                    if ((unsigned)cap.d[d].count != bt->nidx) continue;
                    for (q = 0; q < bt->nidx; q += 11) {
                        unsigned short vi = bt->idx[q];
                        const float *sv = &bt->verts[vi * 5];
                        float mo[3] = { 0, 0, 0 }, wo[3];
                        const float *mm2 = cap.d[d].m;
                        unsigned int e;
                        float dd;
                        for (e = 0; e < bt->inf_n[vi]; e++) {
                            unsigned int bp = bt->inf_part[vi * CHR_MAX_INF + e];
                            float wt = bt->inf_w[vi * CHR_MAX_INF + e];
                            const float *mm = c.draw[bp];
                            mo[0] += wt * (mm[0] * sv[0] + mm[4] * sv[1]
                                           + mm[8] * sv[2] + mm[12]);
                            mo[1] += wt * (mm[1] * sv[0] + mm[5] * sv[1]
                                           + mm[9] * sv[2] + mm[13]);
                            mo[2] += wt * (mm[2] * sv[0] + mm[6] * sv[1]
                                           + mm[10] * sv[2] + mm[14]);
                        }
                        wo[0] = mm2[0] * mo[0] + mm2[4] * mo[1]
                                + mm2[8] * mo[2] + mm2[12];
                        wo[1] = mm2[1] * mo[0] + mm2[5] * mo[1]
                                + mm2[9] * mo[2] + mm2[13];
                        wo[2] = mm2[2] * mo[0] + mm2[6] * mo[1]
                                + mm2[10] * mo[2] + mm2[14];
                        dd = sqrtf(dist2(wo, cap.pos[cap.d[d].first + q]));
                        if (dd > worst) worst = dd;
                        compared++;
                    }
                    break;
                }
            }
        }
        ck(compared > 20 && worst < 1e-3f,
           "  the skinned vertices submitted ARE the blend",
           "worst %.5f m over %d sampled vertices", worst, compared);
    }

    /*
     * THE DRAW RANGE IS THE EYE'S, AND THE STEP RANGE IS THE CAR'S. Moving the
     * EYE alone past CHR_DRAW_DIST has to stop the draw and leave the stepping
     * exactly where it was -- which is the whole reason the two are separate:
     * the free camera can be a hundred metres from a car that a dog is still
     * chasing.
     */
    {
        unsigned int stepped_before = c.n_stepped;
        float fwd[3] = { 0, 0, 1 };
        eye[0] += CHR_DRAW_DIST * 3.0f;
        for (i = 0; i < 2; i++)
            char_step(&c, 1.0f / 60.0f, car, fwd, 0.0f);
        cap_reset();
        char_draw(&c, eye);
        far_draws = cap.n_draws;
        ck(far_draws == 0, "past CHR_DRAW_DIST from the EYE nothing is drawn",
           "%d draws at %.0f m", far_draws, CHR_DRAW_DIST * 3.0f);
        ck(c.n_stepped == stepped_before && c.n_stepped > 0,
           "  and moving the eye does not stop the stepping",
           "%u stepped, was %u", c.n_stepped, stepped_before);
    }
    char_free(&c);
}

/* ========================================================== part 10: run over */

static void part10(void)
{
    chr_t c;
    int wi, hurt_entries = 0, hurt_events = 0, i;
    printf("\n== 10. being run over is an EDGE\n");
    if (!char_init(&c, ASSETS "beach_1.chr", "beach_1", NULL)) {
        ck(0, "beach_1.chr", "will not load");
        return;
    }
    wi = find_inst(&c, "Woman");
    if (wi < 0) { ck(0, "beach_1 has a Woman", ""); char_free(&c); return; }
    {
        chr_inst_t *w = &c.inst[wi];
        float car[3], fwd[3] = { 0, 0, 1 };
        int was = w->state;
        /* park the car on top of her for four seconds */
        for (i = 0; i < 60 * 4; i++) {
            car[0] = w->x; car[1] = w->y; car[2] = w->z;
            char_step(&c, 1.0f / 60.0f, car, fwd, 3.0f);
            if (w->state == CHR_ST_HURT && was != CHR_ST_HURT)
                hurt_entries++;
            if (w->event == CHR_EV_HURT) hurt_events++;
            was = w->state;
        }
        ck(hurt_entries == 1, "held under the car it reacts ONCE",
           "%d entries over 4 s", hurt_entries);
        ck(hurt_events == hurt_entries,
           "  and one CHR_EV_HURT cue with it",
           "%d cues against %d entries", hurt_events, hurt_entries);
        /*
         * AND IT KEEPS QUIET AFTER THE CLIP ENDS. The state guard alone is
         * enough while the Hurt clip is playing -- it runs 4.5 s -- so a window
         * that short cannot see the cooldown at all. Twelve seconds can: once
         * the clip is over, only CHR_HIT_COOL stops a car still parked on top
         * from setting it off again immediately.
         */
        for (; i < 60 * 12; i++) {
            car[0] = w->x; car[1] = w->y; car[2] = w->z;
            char_step(&c, 1.0f / 60.0f, car, fwd, 3.0f);
            if (w->state == CHR_ST_HURT && was != CHR_ST_HURT)
                hurt_entries++;
            was = w->state;
        }
        ck(hurt_entries <= 2, "  and the cooldown holds past the clip",
           "%d entries over 12 s, clip is %.1f s and CHR_HIT_COOL is %.1f",
           hurt_entries,
           c.model[w->model].clip[w->clip >= 0 ? w->clip : 0].duration,
           CHR_HIT_COOL);

        /* and a car crawling past does not count as a knock */
        char_reset(&c);
        hurt_entries = 0;
        was = c.inst[wi].state;
        for (i = 0; i < 60 * 4; i++) {
            car[0] = w->x; car[1] = w->y; car[2] = w->z;
            char_step(&c, 1.0f / 60.0f, car, fwd,
                      CHR_HIT_MIN_SPEED * 0.5f);
            if (w->state == CHR_ST_HURT && was != CHR_ST_HURT)
                hurt_entries++;
            was = w->state;
        }
        ck(hurt_entries == 0, "  below CHR_HIT_MIN_SPEED it is not a knock",
           "%d entries", hurt_entries);
    }
    char_free(&c);
}

/* ================================================= part 11: on the ground === */

/*
 * THE AUTHORED Y IS A PIVOT THAT FLOATS, and this is the check the props did
 * not have.
 *
 * prop_init had to drop the 129 props to the surface for exactly this reason --
 * they hang a median 0.47 m in the air otherwise -- and that bug SHIPPED,
 * because the harness asked whether a prop had SUNK and never whether it was
 * RESTING. So: against the real grids, every character that stands on the
 * ground has to be ON it, and the check is two-sided.
 *
 * The WALKERS and the path followers are both excluded, for one reason: their y
 * comes out of the recording on their first step, not out of the pivot. The six
 * road cars share one dummy pivot that is not on the ground at all, and
 * beach_1's Man and Woman share one that has no collision under it -- which is
 * fine, because neither is ever drawn there.
 */
static void part11(void)
{
    unsigned int t;
    int placed = 0, on = 0, roofed = 0;
    float worst = 0.0f;
    const char *worst_who = "-";

    printf("\n== 11. dropped to the real collision grids\n");
    for (t = 0; t < NTRACK; t++) {
        char path[256];
        chr_t c;
        col_t col;
        unsigned int i;
        int lifted = 0;

        snprintf(path, sizeof path, ASSETS "%s.col", TRACK[t]);
        memset(&col, 0, sizeof col);
        if (!col_load(path, &col)) {
            ck(0, TRACK[t], "no %s", path);
            continue;
        }
        snprintf(path, sizeof path, ASSETS "%s.chr", TRACK[t]);
        if (!char_init(&c, path, TRACK[t], &col)) {
            col_free(&col);
            continue;
        }
        for (i = 0; i < c.n_inst; i++) {
            const chr_inst_t *in = &c.inst[i];
            float g = 0.0f, nx, ny, nz;
            if (in->place->kind == CHR_KIND_PATH
                || in->place->kind == CHR_KIND_WALKER) continue;
            placed++;
            if (fabsf(in->y - in->place->y) > 0.02f) lifted++;
            /* The same two probes drop() uses -- a check that asks a different
               question from the code cannot say whether the code was right. */
            if (!col_ground_at(&col, in->x, in->z, in->y + CHR_DROP_CEIL, &g,
                               &nx, &ny, &nz)
                && !col_ground_at(&col, in->x, in->z, in->y + CHR_DROP_LIFT,
                                  &g, &nx, &ny, &nz)) {
                printf("       (no surface at all under %s on %s)\n",
                       in->place->name, TRACK[t]);
                continue;
            }
            if (fabsf(in->y - g) > worst) {
                worst = fabsf(in->y - g);
                worst_who = in->place->name;
            }
            if (fabsf(in->y - g) < 0.02f) on++;
            {
                float tight = 0.0f, loose = 0.0f;
                int a = col_ground_at(&col, in->x, in->z,
                                      in->place->y + CHR_DROP_CEIL, &tight,
                                      &nx, &ny, &nz);
                int b = col_ground_at(&col, in->x, in->z,
                                      in->place->y + CHR_DROP_LIFT, &loose,
                                      &nx, &ny, &nz);
                if (a && b && fabsf(tight - loose) > 0.02f) roofed++;
            }
        }
        ck(1, TRACK[t], "%u characters, %d moved off the authored pivot",
           c.n_inst, lifted);
        char_free(&c);
        col_free(&col);
    }
    ck(placed > 25 && on == placed,
       "every standing character rests on the surface under it",
       "%d of %d, worst %.4f m (%s)", on, placed, worst, worst_who);
    /*
     * REPORTED, NOT ASSERTED, and deliberately: this is how many standing
     * characters have SOMETHING between the tight ceiling and the generous one,
     * i.e. how many the tight probe is actually protecting. On the shipped ten
     * it is zero -- so a build that used the generous ceiling for both probes
     * would behave identically here, and the tight one is a guard against the
     * case beach_2's overpass put the CAR in rather than one the characters are
     * in. A number to watch across a repack, in the manner of the rejected
     * vertical-face count, not a check to satisfy.
     */
    printf("       %d of %d standing characters have geometry between the two "
           "ceilings (%.1f m and %.1f m)\n", roofed, placed, CHR_DROP_CEIL,
           CHR_DROP_LIFT);
}

/* ============================== part 12: the characters are SOLID ============ */

/*
 * THE CAR COLLIDES WITH THEM, and this is the half that was missing.
 *
 * Both registries are transcribed in char_data.h's CHR_PROXY (see there for the
 * addresses). What is checkable here:
 *
 *   - the six kinds the engine registers a car pair for HAVE a proxy and the two
 *     flyers do not. That is a statement about the shipped table, and it fails if
 *     a model is added to the table by hand or dropped out of it;
 *   - a proxy is where the character IS -- every sphere within its own drawn
 *     geometry, which is what makes the numbers 0.2 m rather than arbitrary;
 *   - the Dog's five spheres MOVE WITH ITS LEGS, because they are bones and not
 *     a column. A column proxy passes every other check in this part;
 *   - a car driven at a character is STOPPED by it, and one driven nowhere near
 *     one is bit-identical with the contact call in and out. The no-op half is
 *     what makes the first half mean anything.
 */

static float dist3(const float a[3], const float b[3])
{
    float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

static void part12(void)
{
    chr_t c;
    col_t col;
    unsigned int i;
    int bad = 0;

    printf("\n== 12. the characters are SOLID to the car\n");

    /* --- the table itself, against the engine's own two registries ---------- */
    {
        static const char *SOLID[] = { "Man", "Woman", "RepairMan", "Guard",
                                       "Crab", "Spider", "Dog", "Truck",
                                       "Trailer", "Hammer", "Btr", NULL };
        static const char *FLYER[] = { "Seagull", "Vulture", NULL };
        int k, found = 0, unsolid = 0;
        for (k = 0; SOLID[k]; k++) {
            int j;
            for (j = 0; j < CHR_N_PROXY; j++)
                if (!strcmp(CHR_PROXY[j].model, SOLID[k])) {
                    if (CHR_PROXY[j].n > 0 && CHR_PROXY[j].kind != CHR_PX_NONE)
                        found++;
                    break;
                }
        }
        for (k = 0; FLYER[k]; k++) {
            int j;
            for (j = 0; j < CHR_N_PROXY; j++)
                if (!strcmp(CHR_PROXY[j].model, FLYER[k])) {
                    if (!CHR_PROXY[j].n && CHR_PROXY[j].kind == CHR_PX_NONE)
                        unsolid++;
                    break;
                }
        }
        ck(found == 11, "every kind the engine pairs with $CAR has a proxy",
           "%d of 11", found);
        ck(unsolid == 2, "  and the Seagull and the Vulture have none",
           "%d of 2 -- they are in neither registry", unsolid);
    }

    /* --- the recovered numbers, not a second copy of the table -------------- */
    {
        int j, peop = 0, guam = 0, crab = 0, spdr = 0, dog = 0;
        for (j = 0; j < CHR_N_PROXY; j++) {
            const chr_proxy_t *p = &CHR_PROXY[j];
            if (!strcmp(p->model, "Man") || !strcmp(p->model, "Woman")
                || !strcmp(p->model, "RepairMan"))
                peop += (p->n == 3 && near(p->radius, 0.2f, 1e-4f)
                         && near(p->shift_y, 0.0f, 1e-4f)
                         && near(p->step_y, 0.2f, 1e-4f));
            if (!strcmp(p->model, "Guard"))
                guam = (p->n == 2 && near(p->radius, 0.2f, 1e-4f)
                        && near(p->step_y, 0.2f, 1e-4f));
            if (!strcmp(p->model, "Crab"))
                crab = (p->n == 1 && near(p->radius, 0.2f, 1e-4f)
                        && near(p->shift_y, -0.18f, 1e-4f));
            if (!strcmp(p->model, "Spider"))
                spdr = (p->n == 1
                        && near(p->radius, CHR_SPIDER_CDTRAD, 1e-6f)
                        && near(p->shift_y, CHR_SPIDER_CDTSHIFTY, 1e-6f));
            if (!strcmp(p->model, "Dog"))
                dog = (p->n == 5 && p->kind == CHR_PX_BONES);
        }
        ck(peop == 3 && guam && crab, "the recovered stacks are what shipped",
           "PEOP 3x0.2 at 0.2, GUAM 2x0.2, CRAB 1x0.2 at -0.18");
        ck(spdr, "  and the Spider's comes out of spider.ini",
           "CdtRad %.3f, CdtShiftY %.3f -- BELOW its pivot, as the Crab's is",
           CHR_SPIDER_CDTRAD, CHR_SPIDER_CDTSHIFTY);
        ck(dog, "  and the Dog's five are BONES", "");
    }

    /* --- a proxy is where the character is --------------------------------- */
    {
        unsigned int t;
        int checked = 0;
        float worst = 0.0f;
        char worst_who[CHR_NAME] = "-";
        for (t = 0; t < NTRACK; t++) {
            char path[256];
            snprintf(path, sizeof path, ASSETS "%s.chr", TRACK[t]);
            if (!char_init(&c, path, TRACK[t], NULL)) continue;
            for (i = 0; i < c.n_inst; i++) {
                float sp[CHR_PROXY_MAX][4], p[3];
                int n = char_proxy(&c, i, sp), k;
                const chr_model_t *m = &c.model[c.inst[i].model];
                float reach;
                if (n <= 0) continue;
                checked++;
                /* Every sphere CENTRE has to be within the instance's own drawn
                   reach of it -- the model radius, scaled, plus a sphere. A proxy
                   built in the wrong space (model instead of world, or the yaw
                   dropped) lands metres away and fails this. */
                if (!char_pos(&c, i, p)) continue;
                reach = m->radius * c.inst[i].scale + 0.5f;
                for (k = 0; k < n; k++) {
                    float ctr[3] = { sp[k][0], sp[k][1], sp[k][2] };
                    float d = dist3(ctr, p);
                    if (d > worst) {
                        worst = d;
                        /* copied, not aliased: char_free below frees c.model */
                        snprintf(worst_who, sizeof worst_who, "%s", m->name);
                    }
                    if (!(d <= reach && sp[k][3] > 0.0f))
                        bad++;
                }
            }
            char_free(&c);
        }
        ck(checked > 25 && bad == 0,
           "every sphere of every solid character is on the character",
           "%d instances, worst centre %.2f m from the pivot (%s)",
           checked, worst, worst_who);
    }

    /* --- the Dog's proxy is its LEGS --------------------------------------- */
    {
        int di;
        if (char_init(&c, ASSETS "country_1.chr", "country_1", NULL)) {
            di = find_inst(&c, "Dog");
            if (di < 0) {
                ck(0, "country_1 has a Dog", "");
            } else {
                chr_inst_t *d = &c.inst[di];
                const chr_model_t *m = &c.model[d->model];
                float a[CHR_PROXY_MAX][4], b[CHR_PROXY_MAX][4];
                int na, nb, k, moved = 0;
                float spread_a = 0.0f;
                int run = chr_clip_index(m, "run");
                d->clip = run;
                d->clip_t = 0.0f;
                na = char_proxy(&c, (unsigned)di, a);
                d->clip_t = m->clip[run].duration * 0.5f;
                nb = char_proxy(&c, (unsigned)di, b);
                for (k = 0; k < na && k < nb; k++) {
                    float ca[3] = { a[k][0], a[k][1], a[k][2] };
                    float cb[3] = { b[k][0], b[k][1], b[k][2] };
                    if (dist3(ca, cb) > 0.02f) moved++;
                    if (k) {
                        float c0[3] = { a[0][0], a[0][1], a[0][2] };
                        float d2 = dist3(ca, c0);
                        if (d2 > spread_a) spread_a = d2;
                    }
                }
                ck(na == 5 && nb == 5, "the Dog gathers five spheres",
                   "%d then %d", na, nb);
                ck(moved >= 3, "  and half a run cycle MOVES them",
                   "%d of %d moved more than 2 cm -- a column proxy moves none",
                   moved, na);
                ck(spread_a > 0.4f, "  and they are spread along the dog",
                   "%.2f m from the first to the furthest", spread_a);
            }
            char_free(&c);
        }
    }

    /* --- the contact: a no-op far away, a wall up close -------------------- */
    {
        int wi;
        memset(&col, 0, sizeof col);
        if (!col_load(ASSETS "beach_1.col", &col)) {
            ck(0, "beach_1.col", "will not load");
            return;
        }
        if (!char_init(&c, ASSETS "beach_1.chr", "beach_1", &col)) {
            ck(0, "beach_1.chr", "will not load");
            col_free(&col);
            return;
        }
        wi = find_inst(&c, "Woman");
        if (wi < 0) {
            ck(0, "beach_1 has a Woman", "");
            char_free(&c);
            col_free(&col);
            return;
        }

        /* THE NO-OP. Two cars driven identically from the race start, one with
           the contact call in the loop and one without. beach_1's characters are
           tens of metres from the start, so the two must agree BIT FOR BIT --
           that is what says this cannot perturb the handling model when nothing
           is touching. */
        {
            rb_car a, b;
            int step, touched = 0;
            rbcar_init(&a, 0, col_rb_world(&col), TRACKS[0].x, TRACKS[0].y,
                       TRACKS[0].z, TRACKS[0].yaw);
            rbcar_init(&b, 0, col_rb_world(&col), TRACKS[0].x, TRACKS[0].y,
                       TRACKS[0].z, TRACKS[0].yaw);
            for (step = 0; step < 300; step++) {
                rbcar_step(&a, 1.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
                rbcar_step(&b, 1.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
                touched += char_car_solid(&c, &a, NULL);
            }
            ck(touched == 0 && !memcmp(&a.body.x, &b.body.x, sizeof a.body.x)
               && !memcmp(&a.body.P, &b.body.P, sizeof a.body.P)
               && !memcmp(&a.body.L, &b.body.L, sizeof a.body.L),
               "five seconds nowhere near a character is a byte-identical no-op",
               "%d contacts, |dx| %.2e", touched,
               (double)dist3(a.body.x, b.body.x));
        }

        /*
         * THE WALL. Drive from the track's own race start -- known-good ground,
         * which is why the character is moved to the CAR rather than the car to
         * the character: beach_1's Woman is a path replayer standing on her
         * authored pivot, and col_ground_at finds nothing at all three metres to
         * her west, so a car started there falls out of the world at 20 m/s and
         * "it did not reach her" passes for the wrong reason. That is what the
         * first version of this fixture did.
         *
         * Against a control run with her proxy suppressed, the car must travel
         * measurably LESS far and must have been touching.
         */
        {
            rb_car a, b;
            int step, touched = 0, hit_steps = 0;
            float start[3], da, db;
            chr_inst_t *w = &c.inst[wi];
            const chr_proxy_t *save = c.model[w->model].proxy;
            float wx, wy, wz;

            rbcar_init(&a, 0, col_rb_world(&col), TRACKS[0].x, TRACKS[0].y,
                       TRACKS[0].z, TRACKS[0].yaw);
            rbcar_init(&b, 0, col_rb_world(&col), TRACKS[0].x, TRACKS[0].y,
                       TRACKS[0].z, TRACKS[0].yaw);
            for (step = 0; step < 60; step++) {          /* settle on its springs */
                rbcar_step(&a, 0.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
                rbcar_step(&b, 0.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
            }
            {   /* four metres straight ahead, on the ground the car is on */
                const float *cm = rbcar_matrix(&a);
                float g = 0.0f, nx, ny, nz;
                wx = cm[12] + cm[8] * 4.0f;
                wz = cm[14] + cm[10] * 4.0f;
                wy = col_ground_at(&col, wx, wz, cm[13] + 1.0f, &g, &nx, &ny,
                                   &nz) ? g : cm[13];
            }
            start[0] = a.body.x[0]; start[1] = a.body.x[1];
            start[2] = a.body.x[2];
            for (step = 0; step < 240; step++) {
                int n;
                /* Hold her still: the walkers replay a recorded path, and a
                   Woman who has walked away is not a wall to measure. */
                w->x = wx; w->y = wy; w->z = wz;
                rbcar_step(&a, 1.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
                n = char_car_solid(&c, &a, NULL);
                touched += n;
                if (n) hit_steps++;
                /* the control, with nothing solid at all */
                c.model[w->model].proxy = NULL;
                rbcar_step(&b, 1.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
                char_car_solid(&c, &b, NULL);
                c.model[w->model].proxy = save;
            }
            da = dist3(a.body.x, start);
            db = dist3(b.body.x, start);
            ck(touched > 0, "driven at a Woman the car touches her",
               "%d contacts over %d steps", touched, hit_steps);
            ck(da < db - 0.5f, "  and it does not go through her",
               "%.2f m against an unsolid control's %.2f m", da, db);
        }

        /*
         * AND THE FLYERS ARE NOT WALLS. The same drive, at a Seagull instead --
         * which the engine's registries leave out of both -- and it must be the
         * no-op the table says. Placed the same way as the Woman above and for
         * the same reason: the character comes to the car's own good ground.
         */
        {
            rb_car a, b;
            int step, touched = 0, si = find_inst(&c, "Seagull");
            if (si < 0) {
                ck(0, "beach_1 has a Seagull", "");
            } else {
                chr_inst_t *g = &c.inst[si];
                float gx, gy, gz;
                /* And EVERYTHING ELSE out of the way. The block above left the
                   Woman parked four metres in front of the start, so without
                   this the car hits her and the Seagull is never tested -- which
                   showed up as 148 contacts against a bird with no proxy. */
                for (i = 0; i < c.n_inst; i++)
                    if ((int)i != si) {
                        c.inst[i].x += 1000.0f;
                        c.inst[i].z += 1000.0f;
                    }
                rbcar_init(&a, 0, col_rb_world(&col), TRACKS[0].x, TRACKS[0].y,
                           TRACKS[0].z, TRACKS[0].yaw);
                rbcar_init(&b, 0, col_rb_world(&col), TRACKS[0].x, TRACKS[0].y,
                           TRACKS[0].z, TRACKS[0].yaw);
                for (step = 0; step < 60; step++) {
                    rbcar_step(&a, 0.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
                    rbcar_step(&b, 0.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
                }
                {
                    const float *cm = rbcar_matrix(&a);
                    float gr = 0.0f, nx, ny, nz;
                    gx = cm[12] + cm[8] * 4.0f;
                    gz = cm[14] + cm[10] * 4.0f;
                    gy = col_ground_at(&col, gx, gz, cm[13] + 1.0f, &gr, &nx,
                                       &ny, &nz) ? gr : cm[13];
                }
                for (step = 0; step < 240; step++) {
                    g->x = gx; g->y = gy; g->z = gz;
                    rbcar_step(&a, 1.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
                    rbcar_step(&b, 1.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
                    touched += char_car_solid(&c, &a, NULL);
                }
                ck(touched == 0
                   && near(dist3(a.body.x, b.body.x), 0.0f, 1e-4f),
                   "and a Seagull driven straight over is not a wall",
                   "%d contacts, |dx| %.2e m", touched,
                   (double)dist3(a.body.x, b.body.x));
            }
        }
        char_free(&c);
        col_free(&col);
    }
}


/* ======================================= part 13: replayers and their sound == */

/*
 * WHERE A PATH REPLAYER IS BEFORE ANYTHING STEPS IT, WHAT A RECORDED ROAD-CAR
 * SAMPLE SITS ABOVE, AND WHAT A MODEL SOUNDS LIKE. Three reported bugs, one part.
 *
 * Nothing here existed while all three shipped, and part 11 says why the first
 * two could not be seen: it excludes every PATH and WALKER instance, on the
 * grounds that "the road cars share one dummy pivot that is not on the ground at
 * all ... which is fine, because neither is ever drawn there". That was an
 * assumption about the app, not a measurement of it, and it was wrong -- the app
 * drew them exactly there.
 */
static void part13(void)
{
    unsigned int t;
    int off_path = 0, replayers = 0;
    float worst_off = 0.0f;
    char worst_who[CHR_NAME * 2] = "-";

    printf("\n== 13. the replayers, their recorded height, and their voices\n");

    /* --- a replayer starts ON ITS PATH, not on the authored pivot ----------- */
    for (t = 0; t < NTRACK; t++) {
        char path[256];
        chr_t c;
        col_t col;
        unsigned int i;
        memset(&col, 0, sizeof col);
        snprintf(path, sizeof path, ASSETS "%s.col", TRACK[t]);
        col_load(path, &col);
        snprintf(path, sizeof path, ASSETS "%s.chr", TRACK[t]);
        if (!char_init(&c, path, TRACK[t], &col)) { col_free(&col); continue; }
        for (i = 0; i < c.n_inst; i++) {
            const chr_inst_t *in = &c.inst[i];
            const chr_path_t *pa;
            unsigned int j;
            float near2 = 1e18f;
            if (in->path < 0) continue;
            replayers++;
            pa = &c.path[in->path];
            /* To the POLYLINE, not to the nearest sample: the reset lands the
               instance at its own phase, which is BETWEEN two samples, and a
               road car's are up to half a metre apart at 9 m/s. Off-path is off
               the line, and pre-fix it was 89.6 m off it. */
            for (j = 0; j + 1 < pa->n; j++) {
                const float *a = &pa->p[j * 3], *b = &pa->p[(j + 1) * 3];
                float ex = b[0] - a[0], ey = b[1] - a[1], ez = b[2] - a[2];
                float wx = in->x - a[0], wy = in->y - a[1], wz = in->z - a[2];
                float ee = ex * ex + ey * ey + ez * ez;
                float u = ee > 1e-12f ? (wx * ex + wy * ey + wz * ez) / ee : 0.0f;
                float d2;
                if (u < 0.0f) u = 0.0f;
                if (u > 1.0f) u = 1.0f;
                wx -= ex * u; wy -= ey * u; wz -= ez * u;
                d2 = wx * wx + wy * wy + wz * wz;
                if (d2 < near2) near2 = d2;
            }
            if (sqrtf(near2) > 0.05f) off_path++;
            if (sqrtf(near2) > worst_off) {
                worst_off = sqrtf(near2);
                snprintf(worst_who, sizeof worst_who, "%s on %s",
                         in->place->name, TRACK[t]);
            }
        }
        char_free(&c);
        col_free(&col);
    }
    ck(replayers >= 20 && off_path == 0,
       "every replayer is on its own path from the reset",
       "%d of %d off it, worst %.2f m (%s)", off_path, replayers, worst_off,
       worst_who);

    /*
     * AND NOT UNDER THE SEA. beach_1's two people begin their recordings
     * submerged -- the Man 1.28 m under the surface for his first 30 s of 469,
     * the Woman 1.01 m under for her first 60 s of 201, and 1,713 of her 3,603
     * samples in all -- and the water is drawn after the characters, so at a race
     * start there was nothing there to see. Reported as two missing people on the
     * first map. char_reset winds the phase on to the first sample the recording
     * itself has out of the water and standing on the grid; the flyers, whose
     * paths never satisfy the second half, keep their own phase.
     */
    {
        int wet = 0, checked = 0;
        float deepest = 0.0f;
        char who[CHR_NAME * 2] = "-";
        for (t = 0; t < NTRACK; t++) {
            char path[256];
            chr_t c;
            col_t col;
            unsigned int i;
            memset(&col, 0, sizeof col);
            snprintf(path, sizeof path, ASSETS "%s.col", TRACK[t]);
            if (!col_load(path, &col)) continue;
            snprintf(path, sizeof path, ASSETS "%s.chr", TRACK[t]);
            if (!char_init(&c, path, TRACK[t], &col)) { col_free(&col); continue; }
            for (i = 0; i < c.n_inst; i++) {
                const chr_inst_t *in = &c.inst[i];
                float w = 0.0f;
                if (in->path < 0) continue;
                /* A flyer is allowed to be over water -- it is over everything. */
                if (!strcmp(in->place->model, "Vulture")) continue;
                checked++;
                if (col_water_at(&col, in->x, in->z, &w) && in->y < w) {
                    wet++;
                    if (w - in->y > deepest) {
                        deepest = w - in->y;
                        snprintf(who, sizeof who, "%s on %s", in->place->name,
                                 TRACK[t]);
                    }
                }
            }
            char_free(&c);
            col_free(&col);
        }
        ck(checked >= 15 && wet == 0,
           "  and none of them starts under the water",
           "%d of %d submerged, deepest %.2f m (%s)", wet, checked, deepest,
           who);
    }

    /*
     * --- and a ROADCAR recording is on the road -----------------------------
     *
     * A .gpf whose header names ROADCAR records the vehicle's CENTRE, so
     * pack_chars.py takes the RoadCar model's own 0.716 m mid-height off every
     * sample. Unpacked, every one of the four recordings floats a flat 0.70 m --
     * one constant over four different terrains -- which is what put six trucks
     * in the air. Measured against the surface UNDER each sample: a ceiling of
     * y + 8 finds beach_2's overpass instead of its road.
     *
     * The people's .dat recordings must NOT have moved (they were already on the
     * ground) and the vultures' ZEROCAM .gpf must not either -- it is a camera
     * path, and vulture.ini's own ShiftY is what lowers the bird off it.
     */
    {
        int rdc_paths = 0, dat_paths = 0, zcam = 0, bad_rdc = 0, bad_dat = 0;
        float worst_rdc = 0.0f, worst_dat = 0.0f, zlow = 1e9f;
        for (t = 0; t < NTRACK; t++) {
            char path[256];
            chr_t c;
            col_t col;
            unsigned int i;
            memset(&col, 0, sizeof col);
            snprintf(path, sizeof path, ASSETS "%s.col", TRACK[t]);
            if (!col_load(path, &col)) continue;
            snprintf(path, sizeof path, ASSETS "%s.chr", TRACK[t]);
            if (!char_init(&c, path, TRACK[t], &col)) { col_free(&col); continue; }
            for (i = 0; i < c.n_paths; i++) {
                const chr_path_t *pa = &c.path[i];
                unsigned int j, got = 0;
                float med;
                int is_vulture = !strncmp(pa->name, "vlt_", 4);
                float *dy = malloc((size_t)(pa->n ? pa->n : 1) * sizeof *dy);
                for (j = 0; j < pa->n; j++) {
                    float x = pa->p[j * 3], y = pa->p[j * 3 + 1];
                    float z = pa->p[j * 3 + 2];
                    float g, nx, ny, nz;
                    if (!col_ground_at(&col, x, z, y + 0.05f, &g, &nx, &ny, &nz))
                        continue;
                    dy[got++] = y - g;
                }
                if (!got) {                     /* a bird, or a road with no grid */
                    if (is_vulture) zcam++;
                    free(dy);
                    continue;
                }
                /* THE MEDIAN, not the mean. beach_3's road crosses its own
                   bridge and beach_4's runs where the grid has no faces under a
                   quarter of it, so a handful of samples sit metres above
                   whatever is beneath them; the offset this is measuring is a
                   constant on every sample, and the median is what sees it.
                   Pre-fix every one of the six came out at 0.70. */
                qsort(dy, got, sizeof *dy, cmp_float);
                med = dy[got / 2];
                free(dy);
                if (is_vulture) {
                    zcam++;
                    if (med < zlow) zlow = med;
                    continue;
                }
                if (pa->kind == 1) {            /* a ROADCAR recording */
                    rdc_paths++;
                    if (fabsf(med) > worst_rdc) worst_rdc = fabsf(med);
                    if (fabsf(med) > 0.15f) bad_rdc++;
                } else {                        /* a person's walk */
                    dat_paths++;
                    if (fabsf(med) > worst_dat) worst_dat = fabsf(med);
                    if (fabsf(med) > 0.05f) bad_dat++;
                }
            }
            char_free(&c);
            col_free(&col);
        }
        ck(rdc_paths >= 5 && bad_rdc == 0,
           "every road-car recording is ON the road",
           "%d of %d off it, worst median %.3f m above the surface under it",
           bad_rdc, rdc_paths, worst_rdc);
        ck(dat_paths >= 8 && bad_dat == 0,
           "  and the people's own recordings did not move",
           "%d of %d off the ground, worst median %.3f m", bad_dat, dat_paths,
           worst_dat);
        ck(zcam >= 2 && zlow > 5.0f,
           "  and the vultures' ZEROCAM path is untouched",
           "%d paths, lowest median %.1f m up -- it is a camera path", zcam,
           zlow > 1e8f ? -1.0f : zlow);
    }

    /* --- what a model sounds like is its own MOD_SNDCHANNEL ----------------- */
    {
        static const struct { const char *model, *wav; } WANT[] = {
            { "Dog", "dog_attack" }, { "Seagull", "seagull_vzliot" },
            { "Man", "man_voice" }, { "Woman", "woman_voice" },
            { "RepairMan", "man_voice" }, { "Guard", "man_voice" },
            { "Crab", NULL }, { "Spider", NULL }, { "Vulture", NULL },
            { "Truck", NULL }, { "Trailer", NULL }, { "Hammer", NULL },
            { "Btr", NULL }, { NULL, NULL }
        };
        int k, right = 0, want = 0, silent = 0, checked = 0;
        for (k = 0; WANT[k].model; k++) {
            int j;
            want++;
            for (j = 0; j < CHR_N_SND; j++) {
                if (strcmp(CHR_SND[j].model, WANT[k].model)) continue;
                if (!WANT[k].wav) right += (CHR_SND[j].wav == NULL);
                else right += (CHR_SND[j].wav
                               && !strcmp(CHR_SND[j].wav, WANT[k].wav));
                break;
            }
        }
        ck(right == want, "every model's voice is the one its own subtree names",
           "%d of %d, and seven of the thirteen name none", right, want);

        /*
         * AND THAT IS WHAT REACHES THE CALLER. The bug was in the mapping and
         * not in the table: main.c raised a voice for every run-over event and
         * picked it as "a Woman sounds like a woman, everything else sounds like
         * a man" -- so a squashed CRAB, whose model has no channel at all, cried
         * out in a man's voice. chr_model_wav is what it asks now, so this walks
         * every instance of every track through the real accessor.
         */
        for (t = 0; t < NTRACK; t++) {
            char path[256];
            chr_t c;
            unsigned int i;
            snprintf(path, sizeof path, ASSETS "%s.chr", TRACK[t]);
            if (!char_init(&c, path, TRACK[t], NULL)) continue;
            for (i = 0; i < c.n_inst; i++) {
                const char *wav = chr_model_wav(&c, i);
                const char *mdl = c.inst[i].place->model;
                int quiet = !strcmp(mdl, "Crab") || !strcmp(mdl, "Spider")
                            || !strcmp(mdl, "Vulture") || !strcmp(mdl, "Truck")
                            || !strcmp(mdl, "Trailer")
                            || !strcmp(mdl, "Hammer") || !strcmp(mdl, "Btr");
                checked++;
                if (quiet) {
                    silent += (wav == NULL);
                } else {
                    silent += (wav != NULL);
                }
            }
            char_free(&c);
        }
        ck(checked > 50 && silent == checked,
           "  and chr_model_wav says so for all 59 placements",
           "%d of %d -- the Crab and the Spider are silent", silent, checked);
    }

    /*
     * --- and the burrowers can be touched at all ---------------------------
     *
     * The recovered CRAB shift (0.18 m down, 0x554b14) and SPDR CdtShiftY
     * (0.24 m down) put both animals' only sphere entirely under the surface
     * they stand on, where a car on top of that surface cannot reach it: driven
     * straight at a Crab the car recorded 0 contacts and did not slow at all.
     * char_proxy clamps the centre to the pivot plane -- the PEOP and GUAM
     * convention -- and keeps the recovered radius. So the gathered sphere sits
     * AT the pivot, and the drive below is part 12's, run at a Crab.
     */
    {
        col_t col;
        chr_t c;
        int ci;
        memset(&col, 0, sizeof col);
        if (!col_load(ASSETS "beach_1.col", &col)) {
            ck(0, "beach_1.col", "will not load");
            return;
        }
        if (!char_init(&c, ASSETS "beach_1.chr", "beach_1", &col)) {
            ck(0, "beach_1.chr", "will not load");
            col_free(&col);
            return;
        }
        ci = find_inst(&c, "Crab");
        if (ci < 0) {
            ck(0, "beach_1 has a Crab", "");
        } else {
            float sp[CHR_PROXY_MAX][4];
            int n = char_proxy(&c, (unsigned)ci, sp);
            ck(n == 1 && near(sp[0][1], c.inst[ci].y, 1e-4f)
               && near(sp[0][3], 0.2f, 1e-4f),
               "the Crab's sphere is AT its feet, not under the sand",
               "%d sphere, centre %.3f against a pivot of %.3f, r %.3f", n,
               n ? sp[0][1] : 0.0f, c.inst[ci].y, n ? sp[0][3] : 0.0f);
        }
        {
            rb_car a, b;
            int step, touched = 0, i2;
            float start[3], wx, wy, wz, da, db;
            if (ci >= 0) {
                chr_inst_t *w = &c.inst[ci];
                const chr_proxy_t *save = c.model[w->model].proxy;
                for (i2 = 0; i2 < (int)c.n_inst; i2++)
                    if (i2 != ci) {
                        c.inst[i2].x += 1000.0f;
                        c.inst[i2].z += 1000.0f;
                    }
                rbcar_init(&a, 0, col_rb_world(&col), TRACKS[0].x, TRACKS[0].y,
                           TRACKS[0].z, TRACKS[0].yaw);
                rbcar_init(&b, 0, col_rb_world(&col), TRACKS[0].x, TRACKS[0].y,
                           TRACKS[0].z, TRACKS[0].yaw);
                for (step = 0; step < 60; step++) {
                    rbcar_step(&a, 0.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
                    rbcar_step(&b, 0.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
                }
                {
                    const float *cm = rbcar_matrix(&a);
                    float g = 0.0f, nx, ny, nz;
                    wx = cm[12] + cm[8] * 4.0f;
                    wz = cm[14] + cm[10] * 4.0f;
                    wy = col_ground_at(&col, wx, wz, cm[13] + 1.0f, &g, &nx,
                                       &ny, &nz) ? g : cm[13];
                }
                memcpy(start, a.body.x, sizeof start);
                for (step = 0; step < 240; step++) {
                    w->x = wx; w->y = wy; w->z = wz;
                    rbcar_step(&a, 1.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
                    touched += char_car_solid(&c, &a, NULL);
                    c.model[w->model].proxy = NULL;
                    rbcar_step(&b, 1.f, 0.f, 0.f, 0, RBCAR_TICK_DT);
                    char_car_solid(&c, &b, NULL);
                    c.model[w->model].proxy = save;
                }
                da = dist3(a.body.x, start);
                db = dist3(b.body.x, start);
                ck(touched > 0 && da < db - 0.5f,
                   "  and a car driven at a Crab feels it",
                   "%d contacts, %.2f m against an unsolid control's %.2f m",
                   touched, da, db);
                /* AND IT IS NOT A WALL. A crab is 0.16 m of animal: the car is
                   slowed and gets past, where a Woman stops it at 3.64 m. */
                ck(da > 5.0f, "  and is not stopped by one",
                   "%.2f m travelled, against 3.64 m at a Woman", da);
            }
        }
        char_free(&c);
        col_free(&col);
    }
}


/* ============================ part 14: the skinning arena on hardware ======= */

/*
 * TWO SKINNED DRAWS IN ONE FRAME MUST NOT SHARE VERTEX MEMORY, AND A FRAME MUST
 * NOT REUSE THE PREVIOUS ONE'S.
 *
 * This is not a style rule, it is the hardware's. The custom vitaGL is built
 * with DRAW_SPEEDHACK=2, so a draw whose vertex data exceeds
 * SAFE_DRAW_SIZE_THRESHOLD (0x8000 = 32 KB) is NOT copied into a mapped temp
 * buffer -- GXM is handed the app's own pointer and reads it when the scene
 * flushes (source/ffp.c:1553, 1580). At 20 bytes a vertex that line falls at
 * 1,638 vertices, and it cuts straight through this cast: the Woman's two
 * batches (1798, 1930), the Dog's (2028), the RepairMan's first (1668) and the
 * Guard's second (1950) are over it; the Man's (1572, 1564), the RepairMan's
 * second (1364), the Guard's first (1386) and every Seagull batch are under.
 *
 * char.c filled ONE shared scratch and drew from it per batch, so on hardware
 * every over-the-line draw read whatever the last fill had left there and the
 * Woman, the Dog, the RepairMan and the Guard drew as exploding spikes -- while
 * the Man and the Seagull, both under the line and therefore copied, were
 * perfect. Reported as "some broken npcs", and no check in this file could see
 * it: the recorder copies the vertices at the draw, which is exactly what the
 * hardware stopped doing.
 *
 * So the assertions are about the POINTERS, not the vertices: disjoint within a
 * frame, and rotated between frames (char.c keeps CHR_SKIN_RINGS arenas, one per
 * display buffer, which is vitaGL's own discipline for its circular pool).
 */
/* Is this the address of a skinned fill? The ring is one allocation per frame
   slot, so "inside one of them" is the test -- and a draw that is NOT inside one
   is a rigid batch drawing out of the packed .chr, which nothing rewrites. */
static int in_arena(const chr_t *c, const void *p)
{
    unsigned int r;
    const char *q = (const char *)p;
    if (!q) return 0;
    for (r = 0; r < (unsigned)CHR_SKIN_RINGS; r++) {
        const char *a = (const char *)c->skin[r];
        if (a && q >= a && q < a + (size_t)c->skin_cap * 5 * sizeof(float))
            return 1;
    }
    return 0;
}

static void part14(void)
{
    chr_t c;
    col_t col;
    unsigned int t;
    int overlaps = 0, skinned_draws = 0, over_line = 0;
    const char *worst = "-";

    printf("\n== 14. the skinning arena: one slice per draw, one arena per frame\n");

    for (t = 0; t < NTRACK; t++) {
        char path[256];
        unsigned int i;
        int a, b;
        memset(&col, 0, sizeof col);
        snprintf(path, sizeof path, ASSETS "%s.col", TRACK[t]);
        col_load(path, &col);
        snprintf(path, sizeof path, ASSETS "%s.chr", TRACK[t]);
        if (!char_init(&c, path, TRACK[t], &col)) { col_free(&col); continue; }
        /* Everything at once: the eye at one instance and every other instance
           moved ON TOP of it, so one frame draws every skinned model this track
           has. Two draws cannot collide if only one of them happens. */
        for (i = 1; i < c.n_inst; i++) {
            c.inst[i].x = c.inst[0].x;
            c.inst[i].y = c.inst[0].y;
            c.inst[i].z = c.inst[0].z;
        }
        {
            float eye[3] = { c.inst[0].x, c.inst[0].y, c.inst[0].z };
            cap_reset();
            char_draw(&c, eye);
        }
        /*
         * Only the SKINNED fills are the hazard, and the way to know one is that
         * its pointer lands inside the skin ring. A rigid batch draws straight
         * out of the .chr, so two instances of one Truck legitimately hand over
         * the same address -- nothing ever writes it -- and counting those as
         * overlaps is what the first version of this check did.
         */
        for (a = 0; a < cap.n_draws; a++) {
            if (!in_arena(&c, cap.d[a].vp)) continue;
            skinned_draws++;
            if (cap.d[a].vbytes > 0x8000u) over_line++;
            for (b = a + 1; b < cap.n_draws; b++) {
                const char *pa = (const char *)cap.d[a].vp;
                const char *pb = (const char *)cap.d[b].vp;
                if (!in_arena(&c, cap.d[b].vp)) continue;
                if (pa == pb
                    || (pa < pb ? pa + cap.d[a].vbytes > pb
                                : pb + cap.d[b].vbytes > pa)) {
                    overlaps++;
                    worst = TRACK[t];
                }
            }
        }
        char_free(&c);
        col_free(&col);
    }
    ck(skinned_draws > 20 && over_line > 0,
       "the cast really does cross vitaGL's 32 KB line",
       "%d draws over it of %d, so the client pointer IS handed to GXM",
       over_line, skinned_draws);
    ck(overlaps == 0, "  and no two draws in one frame share vertex memory",
       "%d overlapping pair(s)%s%s", overlaps,
       overlaps ? ", worst on " : "", overlaps ? worst : "");

    /* --- and the arena rotates between frames ------------------------------ */
    {
        char path[256];
        const void *f[CHR_SKIN_RINGS + 1];
        int k, distinct = 1, repeated = 0;
        memset(&col, 0, sizeof col);
        snprintf(path, sizeof path, ASSETS "beach_1.col");
        col_load(path, &col);
        if (char_init(&c, ASSETS "beach_1.chr", "beach_1", &col)) {
            unsigned int i;
            float eye[3];
            for (i = 1; i < c.n_inst; i++) {
                c.inst[i].x = c.inst[0].x;
                c.inst[i].y = c.inst[0].y;
                c.inst[i].z = c.inst[0].z;
            }
            eye[0] = c.inst[0].x; eye[1] = c.inst[0].y; eye[2] = c.inst[0].z;
            for (k = 0; k < CHR_SKIN_RINGS + 1; k++) {
                int a;
                f[k] = NULL;
                cap_reset();
                char_draw(&c, eye);
                for (a = 0; a < cap.n_draws; a++)
                    if (cap.d[a].vbytes > 0x8000u) { f[k] = cap.d[a].vp; break; }
            }
            for (k = 1; k < CHR_SKIN_RINGS; k++)
                if (f[k] && f[k] != f[0]) distinct++;
            /* and it comes back round: frame CHR_SKIN_RINGS reuses frame 0's */
            repeated = (f[CHR_SKIN_RINGS] && f[CHR_SKIN_RINGS] == f[0]);
            ck(f[0] && distinct == CHR_SKIN_RINGS && repeated,
               "  and the arena rotates over CHR_SKIN_RINGS frames",
               "%d distinct in %d frames, and frame %d is back on frame 0's",
               distinct, CHR_SKIN_RINGS, CHR_SKIN_RINGS);
            char_free(&c);
        }
        col_free(&col);
    }
}

/* ================================================================== main ==== */

int main(int argc, char **argv)
{
    if (argc > 1 && chdir(argv[1]) != 0)
        fprintf(stderr, "chdir %s failed\n", argv[1]);
    printf("chartest -- the tracks' people, animals and road cars\n");
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
    part11();
    part12();
    part13();
    part14();
    printf("\n%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
