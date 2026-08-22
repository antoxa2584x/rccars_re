/* Host test for carparts.c against the REAL packed cars.
 *
 * Compiles scene.c against the testgl stub tree, loads assets/car<n>.vsc, and
 * checks that exactly one exhaust is visible at each booster level and that the
 * wheels take the right tyre texture. The bug this guards is the one that was
 * shipping: all four UPGRADES groups drawn at once.
 *
 *   gcc -I<vita> -I<vita>/testgl -O2 carparts_test.c \
 *       <vita>/carparts.c <vita>/scene.c <vita>/carani.c \
 *       <vita>/rb.c <vita>/contact.c <vita>/collide.c -lm -o carparts_test
 */
#include "carparts.h"
#include "scene.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Minimal GL stand-ins. carparts.c touches no GL at all -- it only edits the
   batch fields scene_draw would read -- so these exist purely to let scene.c
   link. vis_test.c has a far richer recorder; this test does not need one. */
static GLuint next_tex = 1;
void glGenTextures(GLsizei n, GLuint *out) { GLsizei i; for (i = 0; i < n; i++) out[i] = next_tex++; }
void glDeleteTextures(GLsizei n, const GLuint *ids) { (void)n; (void)ids; }
void glBindTexture(GLenum t, GLuint id) { (void)t; (void)id; }
void glTexImage2D(GLenum a, GLint b, GLint c, GLsizei d, GLsizei e, GLint f,
                  GLenum g, GLenum h, const void *p)
{ (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)p; }
void glTexParameteri(GLenum a, GLenum b, GLint c) { (void)a;(void)b;(void)c; }
/* Same story as the lightmap pair below: scene.c's one-call mip generation and
   its frustum culling added these, and without them this harness stops LINKING
   and the stale binary answers instead. Nothing here asserts on either. */
void glGenerateMipmap(GLenum t) { (void)t; }
void glGetFloatv(GLenum pname, GLfloat *out)
{
    int i;
    (void)pname;
    for (i = 0; i < 16; i++) out[i] = (i % 5) ? 0.f : 1.f;
}
void glVertexPointer(GLint a, GLenum b, GLsizei c, const void *d) { (void)a;(void)b;(void)c;(void)d; }
void glTexCoordPointer(GLint a, GLenum b, GLsizei c, const void *d) { (void)a;(void)b;(void)c;(void)d; }
void glDrawElements(GLenum a, GLsizei b, GLenum c, const void *d) { (void)a;(void)b;(void)c;(void)d; }
/* scene.c's vertex buffers. Real ids, so scene_load takes the SAME branch here
   as on hardware and the batches end up buffered rather than quietly falling
   back to client pointers; the offsets then reach the pointer calls above, which
   this harness discards anyway -- what it checks is nidx and the batch flags.
   vis_test.c is where the buffer contents are read back. */
static GLuint next_buf_id = 1;
void glGenBuffers(GLsizei n, GLuint *out)
{ GLsizei i; for (i = 0; i < n; i++) out[i] = next_buf_id++; }
void glBindBuffer(GLenum t, GLuint id) { (void)t; (void)id; }
void glBufferData(GLenum t, GLsizeiptr s, const void *d, GLenum u)
{ (void)t;(void)s;(void)d;(void)u; }
void glDeleteBuffers(GLsizei n, const GLuint *ids) { (void)n; (void)ids; }
void glPushMatrix(void) {}
void glPopMatrix(void) {}
void glMultMatrixf(const GLfloat *m) { (void)m; }
/* The lightmap pass added these to scene.c (the atlas goes on texture unit 1 and
   modulates). Without them this harness stopped LINKING, and since a failed link
   leaves the previous binary in place, `./carparts_test` went on answering out of
   a stale build -- the exact trap BUILD.md warns about. vis_test.c records the
   active unit because it asserts on it; nothing here does, so these only need to
   resolve. glcap_unit_* are declared by testgl/vitaGL.h and must exist too. */
GLuint glcap_unit_tex[2];
int    glcap_unit_enabled[2];
void glActiveTexture(GLenum unit) { (void)unit; }
void glClientActiveTexture(GLenum unit) { (void)unit; }
void glTexEnvi(GLenum t, GLenum p, GLint v) { (void)t;(void)p;(void)v; }
void glEnable(GLenum c) { (void)c; }
void glDisable(GLenum c) { (void)c; }
void glEnableClientState(GLenum c) { (void)c; }
void glDisableClientState(GLenum c) { (void)c; }
/* And scene_draw_blend's, which scene.c gained with the transition bands. Same
   rule as the two above: nothing here draws a band, so these only need to
   resolve. vis_test part 15 records them and asserts on them. */
void glBlendFunc(GLenum a, GLenum b) { (void)a; (void)b; }
void glColor4f(float r, float g, float b, float a) { (void)r;(void)g;(void)b;(void)a; }
void glDepthFunc(GLenum f) { (void)f; }
void glDepthMask(GLboolean b) { (void)b; }

static int fails = 0;
static void ck(int ok, const char *what)
{
    if (!ok) { printf("  FAIL: %s\n", what); fails++; }
}

int main(void)
{
    const char *files[3] = {
        "assets/car1.vsc", "assets/car2.vsc", "assets/car3.vsc"
    };
    const char *names[3] = { "Overkill", "Buggy", "Hummer" };
    int c;

    /* BATCH_ALPHA_LOWREF is raised at RUNTIME and every other batch flag comes
       out of the file, so the two bit ranges must not meet. The per-car scan
       below cannot see this on its own -- car batches ship with flags 0, so a
       collision with, say, BATCH_SKY leaves every count in this file correct
       and quietly moves TRACK batches between passes instead. */
    ck((BATCH_ALPHA_LOWREF & (BATCH_SKY | BATCH_ANY_WATER)) == 0,
       "the runtime flag shares no bit with a packed one");

    for (c = 0; c < 3; c++) {
        scene_t s;
        carparts_t p;
        int lvl, i;

        if (!scene_load(files[c], &s)) {
            printf("%s: cannot load %s (run from rccars_vita/)\n",
                   names[c], files[c]);
            fails++;
            continue;
        }
        carparts_bind(&p, &s);
        printf("%s: %d exhausts, %d wheel batches, family '%s', %d tyre levels\n",
               names[c], p.n_boost, p.n_wheel, p.family, p.n_tire);

        ck(p.by_part, "the levels came from the part names, not the texture");
        ck(p.n_boost >= 8, "every exhaust batch found (booster + compressor)");
        ck(p.n_wheel >= 4, "wheel batches found");
        ck(p.n_tire == 4, "all four tyre textures packed");
        for (lvl = 0; lvl < 4; lvl++)
            ck(p.n_at_level[lvl] >= 2,
               "each level has BOTH its halves -- booster and compressor");
        for (i = 0; i < p.n_boost; i++)
            ck(p.boost_batch[i] >= 0 && p.boost_nidx[i] > 0,
               "each exhaust batch has geometry");

        /* Before apply, every exhaust is live -- that IS the bug. */
        {
            int live = 0;
            for (i = 0; i < p.n_boost; i++)
                if (s.batches[p.boost_batch[i]].nidx) live++;
            ck(live == p.n_boost, "as packed, all four exhausts draw (the bug)");
        }

        /* THE LEVEL IS THE RIGHT ONE, judged by something carparts.c never
           reads. carparts_bind keys the level off the `booster_<n>` and
           `compressor_<n>` part names -- the engine's first two upgrade tables
           (0x00573914, 0x00573934). fx.c aims the exhaust smoke from
           `booster_<n>_end`, the THIRD table (0x00573924), and that node is
           modelled at the tip of its own pipe. So level n's geometry has to
           physically contain level n's pipe tip.

           This is what makes the check non-circular: it is about where the
           GEOMETRY is, not about what anything is called. Group the wrong
           meshes under a level and the tip is somewhere else entirely.

           The separation is an order of magnitude on both sides. Correctly
           grouped, the worst tip across the three cars sits 11.6 mm from its
           own level's vertices; with the old texture-suffix rule -- which is
           right only on the Buggy -- the Overkill's levels 3 and 4 were 197.5
           and 193.6 mm out and every Hummer level was over 100. 40 mm is 3.4x
           clear of the worst true case and 2.6x clear of the best false one.

           NOT argmin over the levels: the four exhausts overlap in space, so
           the nearest group to a tip is sometimes a neighbour by a hair (11.6
           against 9.7 mm on the Overkill). Distance to its OWN level is the
           quantity with a gap in it. */
        for (lvl = 0; lvl < 4; lvl++) {
            float tip[3];
            double best = 1e30;
            int k, have_tip = 0;
            char want[32];

            snprintf(want, sizeof(want), "booster_%d_end", lvl + 1);
            for (k = 0; k < s.rig.n; k++) {
                if (strcmp(s.rig.part[k].name, want) == 0) {
                    tip[0] = s.rig.part[k].rest[12];
                    tip[1] = s.rig.part[k].rest[13];
                    tip[2] = s.rig.part[k].rest[14];
                    have_tip = 1;
                    break;
                }
            }
            ck(have_tip, "the level's pipe-tip node is packed");
            if (!have_tip)
                continue;

            for (k = 0; k < p.n_boost; k++) {
                const batch_t *b;
                unsigned int v;
                if (p.boost_level[k] != lvl)
                    continue;
                b = &s.batches[p.boost_batch[k]];
                for (v = 0; v < b->nverts; v++) {
                    double dx = (double)b->verts[v].x - tip[0];
                    double dy = (double)b->verts[v].y - tip[1];
                    double dz = (double)b->verts[v].z - tip[2];
                    double d = dx * dx + dy * dy + dz * dz;
                    if (d < best) best = d;
                }
            }
            best = sqrt(best) * 1000.0;
            if (best > 40.0)
                printf("    level %d: pipe tip is %.1f mm from its own "
                       "exhaust\n", lvl + 1, best);
            ck(best <= 40.0,
               "the level's geometry is the one its own pipe comes out of");
        }

        /* BATCH_ALPHA_LOWREF is set at runtime, in a bit range pack_vsc.py does
           not use. If the two ever met, batches would silently move between
           main.c's two car passes -- so check the packed flags are clear of it
           BEFORE anything raises it. */
        {
            unsigned int k, dirty = 0;
            for (k = 0; k < s.n_batches; k++)
                if (s.batches[k].flags & BATCH_ALPHA_LOWREF) dirty++;
            ck(dirty == 0, "nothing arrives from the packer already low-ref");
        }

        /* The exhaust is the only thing on the car whose texture puts SOLID AREA
           below the world's cut-out threshold: every <prefix>turbo_<n> is
           ARGB4444 with a plateau at alpha nibble 7 (0.467 as a byte, 119), and
           glAlphaFunc(GL_GREATER, 0.5) discards it -- which is why the pipes and
           boosters had holes punched through them. main.c draws these batches in
           a second pass with the reference dropped to 0, keyed on this flag, and
           OPAQUE: blending that pass instead is what made them look
           semi-transparent, see BATCH_ALPHA_LOWREF in scene.h.

           The two passes have to PARTITION the car. A batch in neither draws not
           at all; a batch in both draws twice, which with the alpha test alone
           is merely wasted fill, and would be visible banding again the moment
           anyone puts a blend back.

           ONE apply, at level 0, and this block sits ahead of the level loop on
           purpose. The flag is sticky, so run after that loop the whole set has
           been visible at some point and every level ends up flagged whatever
           the code does -- a mutation that flagged only the CURRENT booster
           survived the entire suite from down there. */
        carparts_apply(&p, &s, 0, 0, 0);
        {
            unsigned int k, body = 0, lowref = 0, both = 0;
            for (k = 0; k < s.n_batches; k++) {
                unsigned int f = s.batches[k].flags;
                int is_boost = 0;
                for (i = 0; i < p.n_boost; i++)
                    if (p.boost_batch[i] == (int)k) is_boost = 1;
                if (!!(f & BATCH_ALPHA_LOWREF) != is_boost) both++;
                if ((f & (BATCH_SKY | BATCH_ALPHA_LOWREF)) == 0) body++;
                if ((f & (BATCH_SKY | BATCH_ALPHA_LOWREF)) == BATCH_ALPHA_LOWREF)
                    lowref++;
            }
            ck(both == 0, "the exhausts, and only the exhausts, are low-ref");
            ck(lowref == (unsigned)p.n_boost,
               "all four exhaust levels are flagged, not just the visible one");
            ck(body + lowref == s.n_batches,
               "and main.c's two car passes partition the batches");
        }

        /* After apply, exactly one, and it is the one asked for. */
        for (lvl = 0; lvl < 4; lvl++) {
            int live = 0, wrong = 0, restored = 0, k;
            carparts_apply(&p, &s, 0, lvl, 0);
            for (k = 0; k < p.n_boost; k++) {
                if (!s.batches[p.boost_batch[k]].nidx)
                    continue;
                live++;
                if (p.boost_level[k] != lvl) wrong++;
                else if (s.batches[p.boost_batch[k]].nidx == p.boost_nidx[k])
                    restored++;
            }
            ck(live == p.n_at_level[lvl],
               "exactly one exhaust visible -- all of it, and nothing else");
            ck(wrong == 0, "the visible exhaust is the one selected");
            ck(restored == p.n_at_level[lvl],
               "the visible one has its full index count back");
        }

        /* Tyres: every wheel batch takes the level's texture, and the four
           levels are four DIFFERENT textures. */
        {
            GLuint seen[4];
            for (lvl = 0; lvl < 4; lvl++) {
                carparts_apply(&p, &s, lvl, 0, 0);
                seen[lvl] = s.batches[p.wheel_batch[0]].gl_tex;
                for (i = 0; i < p.n_wheel; i++)
                    ck(s.batches[p.wheel_batch[i]].gl_tex == p.tire_tex[lvl],
                       "every wheel batch got the level's tyre texture");
            }
            for (lvl = 1; lvl < 4; lvl++)
                ck(seen[lvl] != seen[lvl - 1], "each tyre level is a distinct texture");
        }

        /* ---- the skins ------------------------------------------------------
         *
         * Four paint jobs per car, swapped the way the engine swaps them: a
         * prefix match over the model's texture refs (FUN_005352a0), fed names
         * FUN_0049fc80 sprintf's out of (car+1, skin+1).
         */
        {
            int pg, sk, expect_pages;
            printf("    skins: %d/%d packed, pages '%s' x%d + '%s' x%d\n",
                   p.n_skin, CARPARTS_SKINS,
                   p.skin[0].family, p.skin[0].n_batch,
                   p.skin[1].family, p.skin[1].n_batch);

            /* HOW MANY PAGES, and it is not the same answer for all three cars.
               FUN_0049fc80's switch loads `b` for cars 2 and 3 and writes NULL
               for car 1; the art agrees independently, there being no
               car_bskin1<n> in any texture set. Bound to the car index here on
               purpose -- carparts.c never sees one, it reads the pages off the
               model, so this is a second source rather than a restatement. */
            expect_pages = (c == 0) ? 1 : 2;
            {
                int pages = 0;
                for (pg = 0; pg < CARPARTS_SKIN_PAGES; pg++)
                    if (p.skin[pg].n_batch) pages++;
                ck(pages == expect_pages,
                   "the car has the atlas pages the engine loads for it");
            }
            ck(p.n_skin == CARPARTS_SKINS, "all four skins packed for this car");

            /* The family carries ONE digit of the two -- the car -- and the skin
               is appended. Get that wrong and scene_tex looks up `car_askin213`,
               finds nothing, and every level silently shares one texture. So the
               four have to be four DISTINCT ids, per page, and non-zero. */
            for (pg = 0; pg < CARPARTS_SKIN_PAGES; pg++) {
                if (!p.skin[pg].n_batch)
                    continue;
                ck(strlen(p.skin[pg].family) == 10,
                   "the family is the 9-char prefix plus the car digit");
                for (sk = 0; sk < CARPARTS_SKINS; sk++) {
                    int t;
                    ck(p.skin[pg].tex[sk] != 0, "this skin's atlas is packed");
                    for (t = 0; t < sk; t++)
                        ck(p.skin[pg].tex[sk] != p.skin[pg].tex[t],
                           "each skin is a distinct texture, not one reused");
                }
            }
            /* And the two pages are two different atlases at the same skin. */
            if (p.skin[0].n_batch && p.skin[1].n_batch)
                for (sk = 0; sk < CARPARTS_SKINS; sk++)
                    ck(p.skin[0].tex[sk] != p.skin[1].tex[sk],
                       "the two pages of one skin are two textures");

            /* PAINT IS PAINT, judged by something carparts.c never reads. The
               `env` field comes from pack_vsc.py --envmap, which classifies a
               batch by its ENVIR_CAR_BODY NODE name; carparts.c keys on the
               TEXTURE name. So every batch this file calls paint has to be
               env-mapped -- or be the ANTENNA, whose whip is textured off the
               same atlas and carries no class at all.

               One direction only, and that is the measurement: overkill_dviglo,
               overkill_mashi and mashineriya are env-mapped detail atlases that
               are NOT paint, and the exhausts are ENV_UPGRADES. Asserting the
               converse would fail on the shipped cars. */
            for (pg = 0; pg < CARPARTS_SKIN_PAGES; pg++)
                for (i = 0; i < p.skin[pg].n_batch; i++) {
                    const batch_t *b = &s.batches[p.skin[pg].batch[i]];
                    const char *pn = (b->part < (unsigned)s.rig.n)
                                     ? s.rig.part[b->part].name : "";
                    ck(b->env != ENV_NONE || strcmp(pn, "ANTENNA") == 0,
                       "a painted batch is env-mapped, or it is the antenna");
                    ck(b->env != ENV_UPGRADES,
                       "no exhaust is painted -- those wear their own turbo atlas");
                }

            /* The swap does what it says: every painted batch takes the chosen
               skin's atlas, on both pages. */
            for (sk = 0; sk < CARPARTS_SKINS; sk++) {
                carparts_apply(&p, &s, 0, 0, sk);
                for (pg = 0; pg < CARPARTS_SKIN_PAGES; pg++)
                    for (i = 0; i < p.skin[pg].n_batch; i++)
                        ck(s.batches[p.skin[pg].batch[i]].gl_tex
                               == p.skin[pg].tex[sk],
                           "every painted batch got the chosen skin's atlas");
            }

            /* AND NOTHING ELSE MOVES. "Skins only change the car body skin", so
               the set of batches whose texture differs between two skins must be
               exactly the painted set -- no wheel, no exhaust, no chassis detail.
               Counted over the whole scene rather than over the batches this file
               already knows about, which is the only way a batch it wrongly
               claimed would show up. */
            {
                GLuint *was = malloc(sizeof(GLuint) * s.n_batches);
                unsigned int k;
                int moved = 0, painted = 0;
                carparts_apply(&p, &s, 2, 3, 0);
                for (k = 0; k < s.n_batches; k++) was[k] = s.batches[k].gl_tex;
                carparts_apply(&p, &s, 2, 3, 3);   /* skin only */
                for (k = 0; k < s.n_batches; k++)
                    if (was[k] != s.batches[k].gl_tex) moved++;
                for (pg = 0; pg < CARPARTS_SKIN_PAGES; pg++)
                    painted += p.skin[pg].n_batch;
                ck(moved == painted,
                   "a skin change repaints the painted batches and nothing else");
                ck(moved > 0, "and it does repaint something");

                /* The mirror of it: a TYRE change must not repaint the paint. The
                   two rules are separate strings and could both have been
                   loosened; nothing else in this file would notice. */
                carparts_apply(&p, &s, 0, 3, 1);
                for (k = 0; k < s.n_batches; k++) was[k] = s.batches[k].gl_tex;
                carparts_apply(&p, &s, 3, 3, 1);   /* tyres only */
                moved = 0;
                for (k = 0; k < s.n_batches; k++)
                    if (was[k] != s.batches[k].gl_tex) moved++;
                ck(moved == p.n_wheel,
                   "a tyre change moves the wheels only, not the paint");
                free(was);
            }
        }

        /* Out-of-range levels must clamp, not index off the end. */
        carparts_apply(&p, &s, -5, 99, -7);
        carparts_apply(&p, &s, 99, -5, 99);
        ck(1, "out-of-range levels survive");
    }

    /* An unbound/empty scene must be a no-op, not a crash. A car packed without
       the extra skin atlases lands in the same place from the other direction:
       n_skin has to come out of the SCENE, so a scene with no paint reports 0 and
       the menu row pins itself rather than offering four identical cars. */
    {
        scene_t empty;
        carparts_t p;
        memset(&empty, 0, sizeof(empty));
        carparts_bind(&p, &empty);
        carparts_apply(&p, &empty, 2, 2, 2);
        ck(p.n_boost == 0 && p.n_wheel == 0, "empty scene binds to nothing");
        ck(p.n_skin == 0, "and offers no skins");
    }

    /* A HALF-PACKED CAR, and it is why this fixture is hand-built.
     *
     * All three shipped cars have all four skins on every page they use, so on
     * the real assets "how many are consecutive from 1" and "how many are
     * present at all" are the same number -- 4 -- and a mutant that counted the
     * second survived every check above. n_skin promises the FIRST, because the
     * menu wraps over it and a gap would then be a position that repaints
     * nothing.
     *
     * THE GAP HAS TO BE FOLLOWED BY SOMETHING, or the two rules agree anyway. A
     * car missing skin 3 and stopping there counts 2 either way; what separates
     * them is a car missing skin 3 and HAVING skin 4, which is 2 consecutive and
     * 3 present. The first version of this fixture did not have that and the
     * mutant walked straight through it.
     *
     * The state cannot be reached by loading anything -- no shipped car is
     * half-packed -- so the scene is assembled directly, which is cheap because
     * carparts_bind reads very little of one: the batch table, the texture names
     * and ids, and scene_tex's lookup between them. */
    {
        static char names[7][SCENE_TEX_NAME];
        static GLuint ids[7] = { 41, 42, 43, 44, 45, 46, 47 };
        static batch_t bat[2];
        scene_t s;
        carparts_t p;

        snprintf(names[0], SCENE_TEX_NAME, "car_askin21");   /* the one worn */
        snprintf(names[1], SCENE_TEX_NAME, "car_askin22");
        snprintf(names[2], SCENE_TEX_NAME, "car_askin24");   /* page a has no 3 */
        snprintf(names[3], SCENE_TEX_NAME, "car_bskin21");   /* the one worn */
        snprintf(names[4], SCENE_TEX_NAME, "car_bskin22");
        snprintf(names[5], SCENE_TEX_NAME, "car_bskin23");
        snprintf(names[6], SCENE_TEX_NAME, "car_bskin24");
        memset(bat, 0, sizeof(bat));
        bat[0].tex = 0;                                      /* page a */
        bat[1].tex = 3;                                      /* page b */

        memset(&s, 0, sizeof(s));
        s.n_tex = 7;
        s.tex_names = names;
        s.tex_ids = ids;
        s.n_batches = 2;
        s.batches = bat;

        carparts_bind(&p, &s);
        ck(p.skin[0].n_batch == 1 && p.skin[1].n_batch == 1,
           "the hand-built car has one batch on each page");
        /* Page a has 1, 2, 4; page b has all four. Both pages have 1 and 2, the
           run stops at 3, and 4 is present on both AND MUST NOT BE COUNTED --
           that is the whole difference between the two rules. */
        ck(p.n_skin == 2,
           "n_skin stops at the first missing skin and does not resume past it");
        /* What IS packed still works. Skin 2 is on both pages, so both take it. */
        carparts_apply(&p, &s, 0, 0, 1);
        ck(bat[0].gl_tex == ids[1] && bat[1].gl_tex == ids[4],
           "skin 2 is packed on both pages and both take it");
        /* Skin 3 is on page b only: b repaints, a keeps what it had rather than
           going blank. Reaching past n_skin on purpose -- nothing in the app
           will, but a caller that did must not lose the car's texture. */
        carparts_apply(&p, &s, 0, 0, 2);
        ck(bat[1].gl_tex == ids[5], "the page that has skin 3 takes it");
        ck(bat[0].gl_tex == ids[1],
           "and the page that does not keeps what it had, rather than going blank");

        /* The other half of the fallback: a car packed with NO extra atlases has
           exactly one skin, the paint its model ships wearing. That is what every
           car .vsc in the port reported before build.sh asked for the other
           three, and it must still be a valid answer. */
        s.n_tex = 1;                          /* car_askin21 alone */
        s.n_batches = 1;
        carparts_bind(&p, &s);
        ck(p.n_skin == 1, "a car with no extra atlases still has the one it wears");
        ck(p.skin[1].n_batch == 0, "and no second page");
    }

    printf("\n%s\n", fails ? "SOME CHECKS FAILED" : "all carparts checks passed");
    return fails ? 1 : 0;
}
