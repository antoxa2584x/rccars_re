/* THE PLACING'S TWO INPUTS, driven on every real track: is each monotonic, and
 * does each cover a spine length per lap?
 *
 * Both halves matter and the second is the one that caught three wrong fixes in a
 * row: a progress measure that FREEZES is perfectly monotonic and completely
 * useless, and every "0 backward steps" result was vacuous until forward travel
 * was measured beside it.
 *
 * The player is driven along the first opponent's own recording, so both sides
 * cover the same ground and their numbers should track each other.
 */
#include "checkpoint.h"
#include "scene.h"
#include "col.h"
#include "ai.h"
#include "tracks.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static int sp(void *c,float x,float y,float z,float h,float *d,int *k)
{ return cp_spine_dist_near((const checkpoints_t*)c,x,y,z,h,d,k); }
static int lp(void *c,float x,float y,float z,float *o)
{ return cp_lap_progress((const checkpoints_t*)c,x,y,z,o); }

int main(void)
{
    int t;
    printf("%-11s  player: back  worst  laps  |  opponents: back  laps\n","track");
    for (t = 0; t < N_TRACKS; t++) {
        char p[160]; scene_t sc; col_t col; checkpoints_t cps; ai_t ai; ai_track tr;
        int i, k, pback = 0, oback = 0;
        float pworst = 0.f;
        float pprev = -1e9f, pfwd = 0.f, ofwd = 0.f, oprev[AI_MAX_OPPONENTS];
        const ai_car *pc;

        snprintf(p,sizeof p,"assets/%s.vsc",TRACKS[t].base);
        memset(&sc,0,sizeof sc); if (!scene_load(p,&sc)) continue;
        snprintf(p,sizeof p,"assets/%s.col",TRACKS[t].base);
        memset(&col,0,sizeof col); col_load(p,&col);
        cp_init(&cps,&sc,&col);
        /* cp_init only enables the progression when the ARROW textures loaded,
           and a host stub has none -- so cp_step would do nothing and the
           player would look frozen. The real app has them. */
        cps.enabled = 1;
        memset(&ai,0,sizeof ai);
        if (!ai_init(&ai,t,"assets",col_rb_world(&col),1,0)) {
            scene_release(&sc); col_free(&col); continue; }
        tr.ctx=&cps; tr.spine=sp; tr.lap_progress=lp; tr.spine_len=cps.spine_len;
        for (k=0;k<AI_MAX_OPPONENTS;k++) oprev[k]=-1e9f;
        pc = &ai.car[0];
        for (i = 0; i < 12000; i++) {
            const int si = pc->n > 0 ? i % pc->n : 0;
            cp_step(&cps, pc->s[si].p[0], pc->s[si].p[1], pc->s[si].p[2],
                    1.f/60.f);
            ai_step(&ai,&tr,pc->s[si].p[0],pc->s[si].p[1],pc->s[si].p[2],
                    cps.lap, 1.f/60.f);
            if (pprev > -1e8f) {
                float d = ai.player_dist - pprev;
                if (d > 0.f) pfwd += d; else if (d < -0.5f) { pback++; if (-d > pworst) pworst = -d; }
            }
            pprev = ai.player_dist;
            for (k = 0; k < ai.n; k++) {
                if (oprev[k] > -1e8f) {
                    float d = ai.car[k].spine_dist - oprev[k];
                    if (d > 0.f && k == 0) ofwd += d;
                    else if (d < -0.5f) oback++;
                }
                oprev[k] = ai.car[k].spine_dist;
            }
        }
        printf("%-11s  %6d %7.1fm %6.2f  |  %8d  %6.2f\n", TRACKS[t].base,
               pback, pworst, pfwd/cps.spine_len, oback, ofwd/cps.spine_len);
        ai_free(&ai); scene_release(&sc); col_free(&col);
    }
    return 0;
}
/* glBlendFunc, glColor4f, glDepthMask and glDepthFunc all come from
   glstub_host.c -- one definition site, or the link sees them twice. */
void glDrawArrays(unsigned m,int f,int c){(void)m;(void)f;(void)c;}
