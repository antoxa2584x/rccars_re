/* WHAT THE CHECKPOINT GROUND PROBE ACTUALLY FINDS, per checkpoint, per track.
 *
 * cp_init ceilings col_ground_at at the marker's own y PLUS 5 m, and
 * col_ground_at returns the HIGHEST surface not above the ceiling. A tunnel or a
 * bridge deck less than 5 m over the racing line is therefore inside the ceiling
 * and wins -- so cp_t.ground becomes the ROOF, which puts both the respawn and
 * the animated marker up there.
 *
 * place_car already uses 1 m for exactly this reason (beach_2's grid is under an
 * overpass: ground 3.19, roof 10.08). So: print what each ceiling gives.
 */
#include "checkpoint.h"
#include "scene.h"
#include "col.h"
#include "tracks.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    int t, bad5 = 0, bad1 = 0, tot = 0;
    printf("%-11s cp  marker_y   ceil+5m   ceil+1m   ceil+0.6m   verdict\n",
           "track");
    for (t = 0; t < N_TRACKS; t++) {
        char p[160];
        scene_t sc; col_t col; checkpoints_t cps;
        int k;
        snprintf(p, sizeof p, "assets/%s.vsc", TRACKS[t].base);
        memset(&sc, 0, sizeof sc);
        if (!scene_load(p, &sc)) continue;
        snprintf(p, sizeof p, "assets/%s.col", TRACKS[t].base);
        memset(&col, 0, sizeof col);
        if (!col_load(p, &col)) { scene_release(&sc); continue; }
        cp_init(&cps, &sc, &col);
        for (k = 0; k < cps.n; k++) {
            const float *m = cps.cp[k].p[0];
            float g5 = m[1], g1 = m[1], g06 = m[1], nx, ny, nz;
            col_ground_at(&col, m[0], m[2], m[1] + 5.0f, &g5, &nx, &ny, &nz);
            col_ground_at(&col, m[0], m[2], m[1] + 1.0f, &g1, &nx, &ny, &nz);
            col_ground_at(&col, m[0], m[2], m[1] + 0.6f, &g06, &nx, &ny, &nz);
            tot++;
            if (g5 - g1 > 0.5f) bad5++;
            if (g1 - g06 > 0.5f) bad1++;
            printf("%-11s %2d  %8.2f  %8.2f  %8.2f  %8.2f    %s\n",
                   TRACKS[t].base, k + 1, m[1], g5, g1, g06,
                   (g5 - g1 > 0.5f) ? "ROOF at +5" :
                   (g1 - g06 > 0.5f) ? "roof at +1" : "");
        }
        scene_release(&sc);
        col_free(&col);
    }
    printf("\n%d checkpoints: %d pick a ROOF at the shipped +5 m ceiling, "
           "%d would at +1 m\n", tot, bad5, bad1);
    return 0;
}
/* glBlendFunc, glColor4f, glDepthMask and glDepthFunc all come from
   glstub_host.c -- one definition site, or the link sees them twice. */
void glDrawArrays(unsigned m,int f,int c){(void)m;(void)f;(void)c;}
