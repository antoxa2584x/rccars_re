/* wetcheck -- the water path against the REAL ten collision grids.
 *
 * rb_test's part 6b drives carSurfaceDrag through a synthetic water plane, which
 * pins the model. This pins the DATA and the plumbing either side of it: that
 * pack_col.py's COL3 water grid loads, that col_water_at reads it back, that the
 * ten tracks' race starts are dry, and that a car driven into a real river
 * actually slows down.
 *
 * The distinction matters here for the reason it always has in this port -- a
 * synthetic plane found the parked-shake bug the real track could not, and the
 * real grids found the pier-edge spawn the plane could not. Neither replaces the
 * other.
 *
 *   gcc -I../rccars_vita -O2 -fno-fast-math -ffp-contract=off wetcheck.c \
 *       ../rccars_vita/col.c ../rccars_vita/rb.c ../rccars_vita/contact.c \
 *       ../rccars_vita/collide.c ../rccars_vita/rbcar.c -lm -o wetcheck
 */
#include "col.h"
#include "rbcar.h"
#include "rb_data.h"
#include "tracks.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails;
static void ck(int ok, const char *what)
{
    if (!ok) { printf("  FAIL: %s\n", what); fails++; }
}

#define ASSETS "/mnt/c/tools/rccars_vita/assets/"

/* A COVERED DECK OVER WATER, found by shape alone -- a drivable floor standing
 * clear of a water cell with more geometry above it. Used by the pier section
 * below to locate beach_1's over-water buildings WITHOUT consulting the material
 * byte, so that asserting the material afterwards means something.
 *
 * The "more geometry above" clause is what separates a deck from a roof. A
 * section through either of beach_1's buildings runs lower_part 1.0-2.0, floor
 * 1.2-2.2, wood_wight 3.5-4.8, then the roof, and the car can only ever reach the
 * `floor`. Ask for the topmost floor instead and every sample lands on the roof.
 */
static int covered_deck(const col_t *col, float x, float z,
                        float *deck_y, float *water_y)
{
    float wy = 0.0f, top, deck, n0, n1, n2;

    if (!col_water_at(col, x, z, &wy))
        return 0;
    if (!col_ground_at(col, x, z, 1e9f, &top, &n0, &n1, &n2))
        return 0;
    if (!col_ground_at(col, x, z, top - 0.5f, &deck, &n0, &n1, &n2))
        return 0;
    if (deck < wy + 1.0f || n1 < 0.7f)
        return 0;
    if (deck_y) *deck_y = deck;
    if (water_y) *water_y = wy;
    return 1;
}

/* ... and one the CAR FITS ON. A 0.25 m search grid happily finds a plank edge
 * one sample wide; parking a 0.42 m car on it slid the car off into the sea,
 * which reads as the water gate failing when it is the fixture that is wrong. */
static int deck_car_fits(const col_t *col, float x, float z, float deck)
{
    int dx, dz;

    for (dz = -1; dz <= 1; dz++)
        for (dx = -1; dx <= 1; dx++) {
            float d = 0.0f;
            if (!covered_deck(col, x + 0.5f * dx, z + 0.5f * dz, &d, NULL))
                return 0;
            if (fabsf(d - deck) > 0.15f)
                return 0;
        }
    return 1;
}

int main(void)
{
    int t, n_with_water = 0, n_drownable = 0;
    /* How much of each grid is wet, and whether the start is. */
    puts("-- COL3 water grids, and the ten race starts --");
    for (t = 0; t < N_TRACKS; t++) {
        char path[256];
        col_t col;
        float wy = 0.0f, gy = 0.0f, n0, n1, n2;
        unsigned int cell, wet = 0, ncell;
        int has_ground, has_water;

        snprintf(path, sizeof(path), ASSETS "%s.col", TRACKS[t].base);
        if (!col_load(path, &col)) {
            printf("  FAIL: %s did not load\n", path);
            fails++;
            continue;
        }
        ck(col.water_y != NULL, "the grid carries a COL3 water layer");
        ncell = col.nx * col.nz;
        if (col.water_y)
            for (cell = 0; cell < ncell; cell++)
                if (col.water_y[cell] > COL_NO_WATER) wet++;
        if (wet) n_with_water++;

        /* col_water_at must agree with the raw layer, cell for cell and value for
           value. Counting only the cells that ARE wet leaves both the sentinel
           test and the cell indexing untested -- a mutant that reported water
           everywhere, and one that swapped x for z, both survived until this was
           here. */
        if (col.water_y) {
            unsigned int said = 0, wrong = 0;
            for (cell = 0; cell < ncell; cell++) {
                float x = col.minx + ((float)(cell % col.nx) + 0.5f) * col.cell;
                float z = col.minz + ((float)(cell / col.nx) + 0.5f) * col.cell;
                float got = 0.0f;
                int r = col_water_at(&col, x, z, &got);
                if (r) said++;
                if (r != (col.water_y[cell] > COL_NO_WATER)) wrong++;
                else if (r && got != col.water_y[cell]) wrong++;
            }
            ck(said == wet && wrong == 0,
               "col_water_at agrees with the stored layer in every cell");
        }

        /* Off the grid there is no water.
         *
         * The interesting case is not a wild coordinate -- that reads unmapped
         * memory and passes or fails by luck, which is why the first version of
         * this check let a mutant with no bounds test survive. It is ONE CELL past
         * the right edge, where the flat index `cz*nx + cx` is still inside the
         * array but names a cell one ROW further on. Take any wet cell (row, col)
         * with row >= 1 and query column `col + nx` of row `row - 1`: unguarded,
         * `cz*nx + cx` lands exactly on the wet cell and reports water on the far
         * side of the map. */
        if (col.water_y && wet) {
            float off = 0.0f;
            unsigned int cc;
            int found = 0, wrapped = 0;
            for (cc = col.nx; cc < ncell; cc++) {      /* skip row 0 */
                if (col.water_y[cc] <= COL_NO_WATER) continue;
                found = 1;
                if (col_water_at(&col,
                        col.minx + ((float)(cc % col.nx + col.nx) + 0.5f) * col.cell,
                        col.minz + ((float)(cc / col.nx - 1) + 0.5f) * col.cell,
                        &off))
                    wrapped = 1;
                break;
            }
            ck(found, "the track has a wet cell to wrap onto");
            ck(!wrapped, "a query off the right edge does not wrap onto the next row");

            /* Water does not stand on a hillside.
             *
             * pack_col.py used to register a water face over its whole XZ
             * BOUNDING BOX. The sea ships as broad LOD tiles whose triangles run
             * to 68 m across, so a long diagonal one flooded a fat rectangle of
             * the beach behind it, and the car got water drag on a dry sand ramp
             * with the sea nowhere near -- reported from the game, and 14 to 27%
             * of every wet grid.
             *
             * Nothing here could see it. The starts were all dry, the ford still
             * forded, and no assertion on the finished grid separated a wrongly
             * flooded cell from a genuinely deep one, which is the reason this
             * file gives for NOT checking the water grid directly.
             *
             * There is one thing that separates them: the TERRAIN. A cell the sea
             * really covers has its bed under the surface. A cell the box rule
             * invented has the beach standing proud of it -- metres proud, up the
             * side of a ramp. So the check is bound to the car rather than to any
             * constant in the packer: no wet cell may have its own ground more
             * than a CAR LENGTH above the water it is supposed to be under. At
             * the shoreline the fuzz is half a cell of a gentle slope; a flooded
             * ramp is nothing like it. */
            {
                float worst = 0.0f; unsigned int cbad = 0, cw = 0, cc2;
                float wx = 0.f, wz = 0.f;
                for (cc2 = 0; cc2 < ncell; cc2++) {
                    float g, w0, x, z, nn0, nn1, nn2;
                    if (col.water_y[cc2] <= COL_NO_WATER) continue;
                    cw++;
                    w0 = col.water_y[cc2];
                    x = col.minx + ((float)(cc2 % col.nx) + 0.5f) * col.cell;
                    z = col.minz + ((float)(cc2 / col.nx) + 0.5f) * col.cell;
                    /* Is there a bed at or just under the surface? The ceiling
                       matters: probing the whole column returns the highest
                       thing there, which over a river is the BRIDGE, and the
                       first version of this check failed every track on piers
                       and roofs 9 to 21 m up. */
                    if (col_ground_at(&col, x, z, w0 + 0.42f, &g, &nn0, &nn1,
                                      &nn2))
                        continue;                       /* bed found: genuine */
                    /* Nothing under the surface. Two innocent readings and one
                       guilty one, and the NORMAL separates them: open sea past
                       the end of the seabed has no terrain at all, a sea cliff
                       or an overhanging mountain has terrain that is not a
                       floor, and a flooded beach has a drivable surface standing
                       proud of its own water. Only the third is the bug -- and
                       it is the only one the car can be driving on when the
                       drag fires, which is how it was reported. */
                    if (!col_ground_at(&col, x, z, 1.0e30f, &g, &nn0, &nn1,
                                       &nn2))
                        continue;                       /* no terrain at all */
                    if (nn1 < 0.5f)
                        continue;                       /* a cliff, not a floor */
                    cbad++;
                    if (g - w0 > worst) { worst = g - w0; wx = x; wz = z; }
                }
                /* PRINTED, NOT ASSERTED, and the distinction is the point.
                   pack_col.py's own docstring already says no assertion on the
                   finished grid can separate a wrongly flooded cell from a
                   genuinely deep one, and three attempts here confirmed it: the
                   first version failed every track on BRIDGE DECKS (probing the
                   whole column returns the pier, not the bed), the second on sea
                   CLIFFS, and this one still counts the pier decks that stand
                   legitimately over water -- beach_1's worst is a jetty 9.3 m
                   up. Harmless, because the drag compares the wheel centre with
                   the water and a deck that high is nowhere near it.
                   So this is a number to watch across a repack, in the same
                   class as the rejected-vertical-face count, and the assertion
                   that actually guards the packing is on the packer's side:
                   pack_col.py prints how many cell samples fell inside a water
                   face's box and outside the face. */
                printf("  %-10s %u/%u wet cells carry a drivable floor above "
                       "their own water, worst %+.2f m at (%.1f, %.1f) -- "
                       "piers and jetties, watch it across a repack\n",
                       TRACKS[t].base, cbad, cw, worst, wx, wz);
            }
        }

        has_ground = col_ground_at(&col, TRACKS[t].x, TRACKS[t].z,
                                   TRACKS[t].y + 0.25f, &gy, &n0, &n1, &n2);
        has_water = col_water_at(&col, TRACKS[t].x, TRACKS[t].z, &wy);

        printf("  %-10s %5u/%-5u cells wet  start: ground %+7.3f",
               TRACKS[t].base, wet, ncell, gy);
        if (has_water) {
            /* The wheel centre sits one radius above the ground; the drag needs
               30% of the radius submerged, i.e. the ground more than 0.3r below
               the surface. Report the margin in those terms. */
            float r = RB_CARS[TRACKS[t].car].tune.cdt_rad_wheel;
            float submerged = r - ((gy + r) - wy);
            printf("  water %+7.3f  submerged %+.4f m (%.0f%% of r, ramp starts "
                   "at 30%%)", wy, submerged, 100.0f * submerged / r);
            ck(submerged < 0.35f * r,
               "the race start is not deep enough in water to matter");
        } else {
            printf("  no water in the start cell");
        }
        printf("\n");
        ck(has_ground, "the race start has ground under it");
        col_free(&col);
    }
    /* main.c respawns a car whose centre of mass sinks DROWN_DEPTH below the
     * surface. That constant is the port's -- the original uses per-track reset
     * volumes (the `car_reset` scripts at 0x004f27a0) that the port has no data
     * for -- so it gets a measurement rather than an argument.
     *
     * Two things have to hold. Deep water must exist SOMEWHERE, or driving into
     * the sea would just strand the car -- not on every track, because country_1's
     * seven `pool` meshes are puddles 8 cm deep and nothing more. And the cell the
     * player starts in, with its neighbours, must be nowhere near the threshold.
     *
     * The radius is 2 m, one grid cell: "where the car is", not "where it can get
     * to". Ten metres fails on beach_1 and SHOULD -- the start is on a beach and
     * the sea 10 m away is 0.68 m deep. Driving into the sea is meant to reset
     * you; that is the whole point of the rule. */
    puts("\n-- the drown threshold (main.c DROWN_DEPTH = 0.5 m) --");
    for (t = 0; t < N_TRACKS; t++) {
        char path[256];
        col_t col;
        float r, near_worst = 0.0f, far_worst = 0.0f, worst = 0.0f;
        unsigned int ix, iz, deep = 0, bedded = 0;

        snprintf(path, sizeof(path), ASSETS "%s.col", TRACKS[t].base);
        if (!col_load(path, &col)) continue;
        if (!col.water_y) { col_free(&col); continue; }
        r = RB_CARS[TRACKS[t].car].tune.cdt_rad_wheel;

        for (iz = 0; iz < col.nz; iz++) {
            for (ix = 0; ix < col.nx; ix++) {
                float x = col.minx + ((float)ix + 0.5f) * col.cell;
                float z = col.minz + ((float)iz + 0.5f) * col.cell;
                float wy, gy, n0, n1, n2, sink, dx, dz;
                if (!col_water_at(&col, x, z, &wy)) continue;
                if (!col_ground_at(&col, x, z, wy + 8.0f, &gy, &n0, &n1, &n2))
                    continue;                   /* no bed: open water */
                bedded++;
                /* how far under the surface a car resting here would sit */
                sink = wy - (gy + r);
                if (sink > 0.5f) deep++;
                if (sink > worst) worst = sink;
                dx = x - TRACKS[t].x; dz = z - TRACKS[t].z;
                if (dx*dx + dz*dz < 4.0f && sink > near_worst)
                    near_worst = sink;          /* the start cell and neighbours */
                if (dx*dx + dz*dz < 100.0f && sink > far_worst)
                    far_worst = sink;           /* printed for context only */
            }
        }
        if (!bedded) { col_free(&col); continue; }
        printf("  %-10s %4u/%-4u bedded cells over 0.5 m under  deepest %+.2f m"
               "   start: %+.2f m within 2 m, %+.2f m within 10\n",
               TRACKS[t].base, deep, bedded, worst, near_worst, far_worst);
        if (deep) n_drownable++;
        ck(near_worst < 0.4f,
           "nothing near the drown depth in the race start's own cell");
        col_free(&col);
    }

    printf("\n  %d of %d tracks have water, %d of them deep enough to drown in\n",
           n_with_water, N_TRACKS, n_drownable);
    ck(n_drownable >= 4, "several tracks have water a car can drown in");
    ck(n_with_water >= 4, "at least four tracks carry water");
    ck(n_with_water < N_TRACKS, "and not all of them do");

    /* beach_1's stream: the car must ford it, and be slowed doing so.
     *
     * The stream surface runs y[-0.37, -0.20] and its bed 2 to 13 cm below that
     * (see CLAUDE.md), so a wheel in it is genuinely submerged. Find a wet cell
     * whose ground is well under the surface, put the car there, and drive. */
    {
        col_t col;
        float best_sub = 0.0f, bx = 0.0f, bz = 0.0f, bg = 0.0f, bw = 0.0f;
        unsigned int ix, iz;
        float r;

        puts("\n-- fording beach_1's river --");
        if (!col_load(ASSETS "beach_1.col", &col)) {
            printf("  FAIL: beach_1.col did not load\n");
            return ++fails, 1;
        }
        r = RB_CARS[0].tune.cdt_rad_wheel;

        for (iz = 0; iz < col.nz; iz++) {
            for (ix = 0; ix < col.nx; ix++) {
                float x = col.minx + ((float)ix + 0.5f) * col.cell;
                float z = col.minz + ((float)iz + 0.5f) * col.cell;
                float wy, gy, n0, n1, n2, sub;
                int d, walled = 1;
                if (!col_water_at(&col, x, z, &wy)) continue;
                if (!col_ground_at(&col, x, z, wy + 0.5f, &gy, &n0, &n1, &n2))
                    continue;                       /* open sea: no bed here */
                /* A ford, not the edge of the ocean. Require a bed under all four
                   neighbours too: without this the search picks the shallowest
                   sea cell it can find, the car drives straight out into deep
                   water, falls, and the run reports the 20 m/s momentum cap
                   rather than anything about drag. */
                for (d = 0; d < 4; d++) {
                    float ox = x + ((d == 0) ? col.cell : (d == 1) ? -col.cell : 0.f);
                    float oz = z + ((d == 2) ? col.cell : (d == 3) ? -col.cell : 0.f);
                    float oy, m0, m1, m2;
                    if (!col_ground_at(&col, ox, oz, wy + 0.5f, &oy, &m0, &m1, &m2))
                        { walled = 0; break; }
                }
                if (!walled) continue;
                sub = r - ((gy + r) - wy);          /* how deep the wheel sits */
                /* want a real ford, not the open ocean: shallower than a wheel */
                if (sub > best_sub && sub < r) {
                    best_sub = sub; bx = x; bz = z; bg = gy; bw = wy;
                }
            }
        }
        printf("  deepest fordable cell: (%.2f, %.2f) bed %+.3f water %+.3f "
               "-> wheel %.0f%% submerged\n",
               bx, bz, bg, bw, 100.0f * best_sub / r);
        ck(best_sub > 0.3f * r,
           "beach_1 has water deep enough to reach the drag ramp");

        /* What the drag actually is, on this cell, through the real grid. This
           is the measurement that isolates it: a driving comparison here does
           not, because the ford is a depression in the bed and the terrain
           dominates whatever the water does (measured: 0.26 m/s with water
           against 0.09 without, i.e. the wrong way round and both of them
           meaningless). */
        {
            rb_car c;
            float pts[RB_MAX_FORCES][3], f[RB_MAX_FORCES][3];
            float mag = 0.0f;
            double want;
            int i, n = 0;

            rbcar_init(&c, 0, col_rb_world(&col), bx, bg, bz, 0.0f);
            for (i = 0; i < 30; i++)
                rbcar_step(&c, 0.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
            printf("  parked in it: in_water %d%d%d%d  gaps %+.4f %+.4f\n",
                   c.hit[0].in_water, c.hit[1].in_water,
                   c.hit[2].in_water, c.hit[3].in_water,
                   c.hit[0].water_gap, c.hit[2].water_gap);
            ck(c.hit[0].in_water && c.hit[1].in_water
               && c.hit[2].in_water && c.hit[3].in_water,
               "all four wheels find the river through the real grid");

            /* And the gap each wheel got is its OWN centre height above the water
               height at its OWN position. Checking only in_water leaves the
               probe's whole arithmetic untested: a flipped sign, a probe answered
               at the world origin, and an x/z swap in the lookup all survived
               until this was here -- the first of them by making the drag
               STRONGER, which the bracket below happily accepted. */
            {
                int w, ok = 1;
                float worst = 0.0f;
                for (w = 0; w < c.nwheels; w++) {
                    float ctr[3], wy2 = 0.0f;
                    rb_wheel_frame(&c, w, 0, ctr, NULL, NULL, NULL);
                    if (!col_water_at(&col, ctr[0], ctr[2], &wy2)) { ok = 0; break; }
                    float d = c.hit[w].water_gap - (ctr[1] - wy2);
                    if (fabsf(d) > worst) worst = fabsf(d);
                    /* 1 mm. The recorded gap comes from the last SUBSTEP's
                       gather and the recomputation from the pose at the end of
                       the frame, so a fraction of a millimetre of drift is
                       expected (measured 0.014 to 0.33 mm on a parked car). It is
                       two orders of magnitude tighter than anything a wrong probe
                       does: a flipped sign doubles the gap, ~70 mm here. */
                    if (fabsf(d) > 1e-3f) ok = 0;
                }
                printf("  per-wheel gap vs recomputed: worst %.4f mm\n",
                       worst * 1000.0f);
                ck(ok, "each wheel's gap is its own centre above the water under it");
            }

            c.body.v[0] = 0.0f; c.body.v[1] = 0.0f; c.body.v[2] = 4.0f;
            c.body.P[0] = 0.0f; c.body.P[1] = 0.0f;
            c.body.P[2] = 4.0f * c.body.mass;
            rb_surface_drag(&c, &n, pts, f);
            for (i = 0; i < n; i++)
                mag += sqrtf(f[i][0]*f[i][0] + f[i][1]*f[i][1] + f[i][2]*f[i][2]);
            want = 4.0 * (double)c.tune.coeff_water * 16.0;   /* 4 wheels, v^2=16 */
            printf("  drag at 4 m/s: %.3f N over %d wheels (full coeffWater "
                   "would be %.3f N), %.2f m/s^2 on a %.2f kg car\n",
                   mag, n, want, mag / c.body.mass, c.body.mass);
            ck(n == c.nwheels, "every wheel contributes a drag force");
            /* 86% submerged is past the 0.9r saturation on some wheels and on the
               ramp for others, so bracket rather than pin. Both bounds are real:
               below 40% of the full coefficient the water would not be felt, and
               above it there is nothing left to be. */
            ck(mag > 0.4 * want, "the river drags hard enough to feel");
            ck(mag <= want * 1.02, "and never harder than the full coefficient");

            /* And what that costs a coasting car. A QUARTER of a second, and
               speed rather than distance travelled: the ford is a depression in
               the bed, so over any longer window the car reaches the far bank and
               the terrain decides the answer -- measured, 1.134 m against 1.153 m
               over a second, which says nothing about water at all. The
               end-to-end "water halves the top speed" figure belongs on flat
               ground, and lives in rb_test part 6b. */
            {
                float v_wet = 0.0f, v_dry = 0.0f;
                float *saved = col.water_y;
                int k;
                for (k = 0; k < 2; k++) {
                    col.water_y = k ? NULL : saved;
                    rbcar_init(&c, 0, col_rb_world(&col), bx, bg, bz, 0.0f);
                    for (i = 0; i < 30; i++)
                        rbcar_step(&c, 0.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
                    c.body.v[2] = 4.0f;
                    c.body.P[2] = 4.0f * c.body.mass;
                    for (i = 0; i < 15; i++)
                        rbcar_step(&c, 0.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
                    if (k) v_dry = rbcar_speed(&c);
                    else   v_wet = rbcar_speed(&c);
                }
                col.water_y = saved;
                printf("  coasting 0.25 s from 4 m/s: %.3f m/s left in the "
                       "water, %.3f m/s without it (lost %.3f against %.3f)\n",
                       v_wet, v_dry, 4.0f - v_wet, 4.0f - v_dry);
                /* Bound to the DRAG LAW'S OWN CLOSED FORM, and there is no fitted
                   constant in it. carSurfaceDrag is `coeffWater * v^2` per wheel,
                   so a car coasting under nothing else integrates exactly:

                       1/v = 1/v0 + (share * nwheels * coeffWater / m) * t

                   where `share` is how much of the full coefficient this cell's
                   depth actually delivers -- `mag / want`, measured a few lines
                   up rather than assumed. Everything here comes from the
                   recovered coefficient and this fixture's own measurements.

                   Two-sided, and both sides are derived. The floor is half the
                   predicted loss: the water must deliver at least that much of
                   what its own force law says. The ceiling is the prediction plus
                   `4 - v_dry`, which is precisely what this cell's terrain costs
                   a car with no water in it -- measured in the same loop, so the
                   band widens or narrows with the contamination instead of
                   guessing at it.

                   IT USED TO BE `4 - v_wet > 2 * (4 - v_dry)`, i.e. "the water at
                   least doubles what a coasting car loses", and that denominator
                   is the trap: it is this depression's terrain, not the car. It
                   moved 24% (0.475 -> 0.587 m/s) under a change to the CENTRE OF
                   MASS, which has nothing to do with water, and took the check
                   red with it -- while the wet run it was judging had just come to
                   match the closed form above to 6 mm/s, against 376 mm/s before.
                   The bound went red on the build that got MORE accurate. */
                {
                    double share = (want > 0.0) ? (double)mag / want : 1.0;
                    double pred = 4.0 - 1.0 / (1.0 / 4.0
                                  + share * (double)c.nwheels
                                    * (double)c.tune.coeff_water
                                    / (double)c.body.mass * 0.25);
                    double lost = 4.0 - v_wet, slack = 4.0 - v_dry;
                    printf("      the drag law's own closed form wants %.3f m/s "
                           "lost (%.0f%% of the coefficient); terrain on this "
                           "cell costs %.3f\n", pred, 100.0 * share, slack);
                    ck(lost > 0.5 * pred && lost < pred + slack,
                       "coasting matches the drag law through the real grid");
                }
            }
        }
        col_free(&col);
    }

    /* ON THE PIER. The counterpart to the ford, and the case the ford cannot
     * speak about: a solid floor standing OVER a water cell.
     *
     * beach_1 has two buildings out over the sea, and driving on either one used
     * to report water. rb_world.water answers per COLUMN -- a per-cell surface
     * height is all it has to answer from -- so the probe says "water here" two
     * metres up, and until in_water was gated on the wheel actually reaching the
     * surface the car drove the whole deck wet: car_surf_water instead of
     * car_surf_wood, no dust, no tyre marks.
     *
     * THE DECK IS FOUND GEOMETRICALLY, with no reference to the material byte,
     * and only then is the material asserted. That ordering is the point: a
     * search for "the biggest WOOD floor" and then a check that it is wood
     * asserts nothing at all, and this project has shipped six of those. What
     * locates it here is the shape -- a drivable floor, standing clear of a water
     * cell, with more geometry above it -- which is a description of a covered
     * deck and cannot be moved by anything in pack_col.py's keyword lists.
     *
     * The vertical structure is why the ceiling matters. A section through either
     * building runs `lower_part` 1.0-2.0, `floor` 1.2-2.2, `wood_wight` 3.5-4.8,
     * then the roof -- `tile`, which is bamboo, or `tin`, which is corrugated
     * sheet. Take the topmost floor and you are standing on the roof: measured
     * 90.6% "asphalt" on pier 1 and 98.6% "wetsand" on pier 2, both of them the
     * roof rather than anything the car can reach. */
    {
        /* pack_col.py's SURF_NAMES, for the printout. Index 7 is wood. */
        static const char *const SURF_LABEL[9] = {
            "asphalt", "sand", "wetsand", "gravel", "stone", "grass", "metal",
            "wood", "water"
        };
        col_t col;
        float bx = 0.0f, bz = 0.0f, bdeck = 0.0f, bwater = 0.0f;
        float best = 1e30f, x, z;
        unsigned int hist[9], n_deck = 0;
        int i;

        puts("\n-- parked on beach_1's pier, over the sea --");
        if (!col_load(ASSETS "beach_1.col", &col)) {
            printf("  FAIL: beach_1.col did not load\n");
            return ++fails, 1;
        }
        memset(hist, 0, sizeof(hist));

        for (z = col.minz; z < col.minz + col.nz * col.cell; z += 0.25f) {
            for (x = col.minx; x < col.minx + col.nx * col.cell; x += 0.25f) {
                float wy = 0.0f, deck = 0.0f;
                int mat;
                if (!covered_deck(&col, x, z, &deck, &wy))
                    continue;
                mat = col_material_at(&col, x, deck, z);
                if (mat < 0 || mat > 8)
                    continue;
                hist[mat]++;
                n_deck++;
                /* The LOWEST deck the car FITS ON is where it gets parked, not the
                   highest: the gate under test is "is the wheel within a radius of
                   the surface", so the sample that comes nearest to tripping it is
                   the interesting one. The highest deck is 5 m up and would pass a
                   gate that was out by a factor of fifty. */
                if (deck < best && deck_car_fits(&col, x, z, deck)) {
                    best = deck; bx = x; bz = z; bdeck = deck; bwater = wy;
                }
            }
        }
        printf("  %u covered-deck samples over water; materials:", n_deck);
        for (i = 0; i < 9; i++)
            if (hist[i])
                printf("  %s %.0f%%", SURF_LABEL[i], 100.0 * hist[i] / n_deck);
        printf("\n");
        ck(n_deck > 500, "beach_1 has a covered deck standing over the sea");
        /* The report was that this deck does not sound like wood. It is made of
           planks -- `floor` and `lower_part`, both photographs of weathered
           decking -- and both used to land in the asphalt list by name. */
        ck(n_deck > 0 && hist[7] > n_deck / 2,
           "and the deck the car can reach is WOOD, not asphalt");

        if (n_deck > 500) {
            rb_car c;
            int wet = 0, mat;

            mat = col_material_at(&col, bx, bdeck, bz);
            printf("  parking at (%.2f, %.2f) deck %+.3f over water %+.3f, "
                   "material %s\n", bx, bz, bdeck, bwater, SURF_LABEL[mat]);
            rbcar_init(&c, 0, col_rb_world(&col), bx, bdeck, bz, 0.0f);
            for (i = 0; i < 60; i++)
                rbcar_step(&c, 0.0f, 0.0f, 0.0f, 0, 1.0f / 60.0f);
            for (i = 0; i < c.nwheels; i++)
                if (c.hit[i].in_water) wet++;
            printf("  parked on it: y %+.3f, in_water %d%d%d%d, gap %+.3f "
                   "(wheel radius %.3f)\n", c.body.x[1],
                   c.hit[0].in_water, c.hit[1].in_water,
                   c.hit[2].in_water, c.hit[3].in_water,
                   c.hit[0].water_gap, c.wheel[0].radius);
            ck(c.body.x[1] > bwater + 1.0f, "the car is still up on the deck");
            ck(wet == 0, "no wheel on the deck reports being in the water");
            ck(c.hit[0].water_gap > c.wheel[0].radius,
               "and the gap it did measure is more than a wheel radius");
        }
        col_free(&col);
    }

    /* ---- COL4: the engine's own surface class -------------------------
       Two things, and neither may be asserted against the constant that set it.

       First that all ten grids carry the array at all, with a real spread of
       classes -- an array of zeroes loads and queries perfectly and puts the
       tyre marks back on one flat strength, silently.

       Second, and this is the one with a mechanism behind it: a DECAL must defer
       to the floor beneath it. `wt_wetsand`, the sand/wet-sand transition strip,
       carries class 0 -- "no opinion" -- and is authored about a centimetre
       above the sand it modulates, so a query that took the TOPMOST face would
       answer 0 there and lay no mark on any transition in the game.
       FUN_00534fc0 takes the minimum over the POSITIVE classes instead, and this
       finds a place that can tell the two apart by SHAPE rather than by
       coordinates: a class-0 triangle with a classified one just underneath. */
    {
        int t, ten_ok = 1, spread_ok = 1, probes = 0, deferred = 0;
        puts("\n-- COL4: the engine's surface class, and decals deferring --");
        for (t = 0; t < N_TRACKS; t++) {
            col_t col;
            char path[256];
            unsigned int i, seen = 0, nz = 0;
            int mask = 0;
            snprintf(path, sizeof(path), ASSETS "%s.col", TRACKS[t].base);
            if (!col_load(path, &col))
                continue;
            if (!col.eng_surf) {
                ten_ok = 0;
                printf("  %-10s NO engine surface array\n", TRACKS[t].base);
                col_free(&col);
                continue;
            }
            for (i = 0; i < col.ntris; i++) {
                if (col.eng_surf[i]) {
                    nz++;
                    if (col.eng_surf[i] < 31)
                        mask |= 1 << col.eng_surf[i];
                }
                seen++;
            }
            /* at least four distinct classes, and at least a third of the
               triangles saying something -- the measured range over the ten is
               60.5% to 89.8% classified */
            {
                int nk = 0, b2;
                for (b2 = 1; b2 < 31; b2++) if (mask & (1 << b2)) nk++;
                if (nk < 4 || nz * 3 < seen) {
                    spread_ok = 0;
                    printf("  %-10s only %d classes, %.1f%% classified\n",
                           TRACKS[t].base, nk, 100.0 * nz / (seen ? seen : 1));
                }
            }
            /* the decal rule, located by shape */
            for (i = 0; i < col.ntris && probes < 40; i++) {
                const float *tr = &col.tris[(size_t)i * 9];
                float cx, cy, cz;
                unsigned int j;
                int under = 0;
                if (col.eng_surf[i])
                    continue;                       /* want a class-0 face */
                cx = (tr[0] + tr[3] + tr[6]) / 3.f;
                cy = (tr[1] + tr[4] + tr[7]) / 3.f;
                cz = (tr[2] + tr[5] + tr[8]) / 3.f;
                /* is there a CLASSIFIED face within 5 cm below its centroid? */
                for (j = 0; j < col.ntris; j++) {
                    const float *o = &col.tris[(size_t)j * 9];
                    float oy = (o[1] + o[4] + o[7]) / 3.f;
                    float ox = (o[0] + o[3] + o[6]) / 3.f;
                    float oz = (o[2] + o[5] + o[8]) / 3.f;
                    if (!col.eng_surf[j]) continue;
                    if (oy > cy || cy - oy > 0.05f) continue;
                    if (ox - cx > 0.3f || cx - ox > 0.3f) continue;
                    if (oz - cz > 0.3f || cz - oz > 0.3f) continue;
                    under = 1;
                    break;
                }
                if (!under)
                    continue;
                probes++;
                if (col_surface_at(&col, cx, cy, cz) > 0)
                    deferred++;
            }
            col_free(&col);
        }
        ck(ten_ok, "every one of the ten grids carries an engine class array");
        ck(spread_ok, "and each has a real spread of classes, not one value");
        printf("  decal probes: %d class-0 faces with a classified face under "
               "them, %d resolved to the floor\n", probes, deferred);
        ck(probes > 0, "the ten grids do contain a decal stacked on a floor");
        /* Not all of them: a probe centroid can sit outside the lower face in
           XZ. What must not happen is the query answering 0 across the board,
           which is what topmost-face-wins does. */
        ck(deferred * 2 >= probes,
           "a decal defers to the surface under it rather than reporting 0");
    }

    /* A grid whose material array is TRUNCATED. col_load drops the materials and
       must then also drop the water, because the water sits behind them in the
       file and the read cursor is already in the wrong place -- anything taken
       from there would be garbage presented as data rather than absent data. */
    {
        col_t col;
        FILE *f = fopen(ASSETS "beach_1.col", "rb");
        puts("\n-- a truncated grid drops what it cannot trust --");
        ck(f != NULL, "beach_1.col opens");
        if (f) {
            long sz;
            unsigned char *buf;
            fseek(f, 0, SEEK_END); sz = ftell(f); fseek(f, 0, SEEK_SET);
            buf = malloc((size_t)sz);
            if (buf && fread(buf, 1, (size_t)sz, f) == (size_t)sz) {
                /* Lop off the tail back into the middle of the materials.
                   The tail is [surf ntris][water ncell*4][eng_surf ntris]
                   [light ntris], so every array behind `surf` has to be counted
                   here -- and it is READ OFF THE HEADER rather than written down,
                   because it has now gone stale twice: once when COL4 appended
                   eng_surf and again when COL5 appended `light`, and both times
                   the cut stopped short of `surf` entirely, left it intact, and
                   the fixture reported that the code had stopped dropping it.
                   A fixture with the file's own dimensions in it cannot go stale
                   the third time. */
                const unsigned int nx = *(const unsigned int *)(buf + 16);
                const unsigned int nz = *(const unsigned int *)(buf + 20);
                const unsigned int nt = *(const unsigned int *)(buf + 24);
                const int v5 = memcmp(buf, "COL5", 4) == 0;
                const int v4 = v5 || memcmp(buf, "COL4", 4) == 0;
                const int v3 = v4 || memcmp(buf, "COL3", 4) == 0;
                long tail = 0;
                FILE *o;
                if (v5) tail += (long)nt;                  /* light    */
                if (v4) tail += (long)nt;                  /* eng_surf */
                if (v3) tail += (long)nx * nz * 4;         /* water    */
                /* HALF of `surf`, not all of it and half again. The three lines
                   above already carry every array BEHIND surf, so `nt + nt/2`
                   removed surf entirely and then cut 17,838 bytes into the GRID
                   REFERENCE ARRAY in front of it -- a different state from the
                   one this fixture is named for, and one col_load used to accept,
                   leaving 4,459 references uninitialised and then indexing
                   tris[] with them. Which is the third time this arithmetic has
                   been wrong in the same place: twice by counting one array too
                   FEW when the format grew, and once by counting one too MANY
                   while fixing that. The cut is into surf's own bytes now, and
                   the check below covers the idx case on purpose. */
                tail += (long)nt / 2;                      /* into `surf` */
                printf("  %.4s, %u tris, %u x %u cells -> cutting %ld bytes\n",
                       (const char *)buf, nt, nx, nz, tail);
                o = fopen("/tmp/wetcheck_trunc.col", "wb");
                if (o) {
                    fwrite(buf, 1, (size_t)(sz - tail), o);
                    fclose(o);
                }
                if (col_load("/tmp/wetcheck_trunc.col", &col)) {
                    printf("  truncated: %u tris, materials %s, water %s, "
                           "engine class %s\n",
                           col.ntris, col.surf ? "PRESENT" : "absent",
                           col.water_y ? "PRESENT" : "absent",
                           col.eng_surf ? "PRESENT" : "absent");
                    printf("             lightmap %s\n",
                           col.light ? "PRESENT" : "absent");
                    ck(col.surf == NULL, "a short material array is dropped");
                    ck(col.water_y == NULL,
                       "and the water behind it is dropped with it");
                    ck(col.eng_surf == NULL,
                       "and so is the engine surface class behind that");
                    ck(col.light == NULL,
                       "and COL5's lightmap byte, which is behind that again");
                    ck(col.ntris > 1000, "while the collision triangles survive");
                    col_free(&col);
                } else {
                    ck(0, "a truncated grid still loads its triangles");
                }

                /* AND THE ARRAYS IT CANNOT RUN WITHOUT ARE A REFUSAL, NOT A
                   DEGRADATION. Everything above is behind `surf` and is optional:
                   a missing material, water, class or lightmap array is a duller
                   car or a drier track. The triangles, the cell offsets and the
                   grid references are not optional -- the queries index tris[]
                   BY idx[], so a short idx array is an out-of-bounds read in the
                   hottest loop in the port rather than a lost feature. Cut into
                   idx and the whole grid has to be rejected. */
                {
                    long cut = tail + (long)nt;   /* the rest of surf, into idx */
                    o = fopen("/tmp/wetcheck_noidx.col", "wb");
                    if (o) {
                        fwrite(buf, 1, (size_t)(sz - cut), o);
                        fclose(o);
                    }
                    ck(col_load("/tmp/wetcheck_noidx.col", &col) == 0,
                       "but a grid cut into its REFERENCES is refused outright");
                    col_free(&col);
                }
            }
            free(buf);
            fclose(f);
        }
    }

    /* A COL2 grid, i.e. every .col packed before this existed, must still load
       and simply report no water. The port has to survive stale assets. */
    {
        col_t col;
        FILE *f;
        long sz;
        unsigned char *buf;
        puts("\n-- an older grid still loads --");
        f = fopen(ASSETS "beach_1.col", "rb");
        ck(f != NULL, "beach_1.col opens");
        if (f) {
            fseek(f, 0, SEEK_END); sz = ftell(f); fseek(f, 0, SEEK_SET);
            buf = malloc((size_t)sz);
            if (buf && fread(buf, 1, (size_t)sz, f) == (size_t)sz) {
                FILE *o;
                memcpy(buf, "COL2", 4);         /* same bytes, older magic */
                o = fopen("/tmp/wetcheck_col2.col", "wb");
                if (o) { fwrite(buf, 1, (size_t)sz, o); fclose(o); }
                if (col_load("/tmp/wetcheck_col2.col", &col)) {
                    float wy;
                    printf("  COL2 reload: %u tris, water layer %s\n",
                           col.ntris, col.water_y ? "PRESENT" : "absent");
                    ck(col.water_y == NULL, "a COL2 grid carries no water layer");
                    ck(!col_water_at(&col, 0.0f, 0.0f, &wy),
                       "and col_water_at reports none");
                    ck(col.ntris > 1000, "while its collision triangles still load");
                    col_free(&col);
                } else {
                    ck(0, "a COL2 grid still loads");
                }
            }
            free(buf);
            fclose(f);
        }
    }

    printf("\n%s\n", fails ? "SOME CHECKS FAILED" : "all checks passed");
    return fails != 0;
}
