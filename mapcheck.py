#!/usr/bin/env python3
"""
mapcheck.py -- a probe, not a test: how far the recovered world->minimap
transform puts a known road point from the road as PAINTED.

WHY IT EXISTS. `gen_hud_data.py` asserts everything about that transform that can
be asserted from the tables alone -- that the affine is uniform, unrotated,
unmirrored, and lands every race start inside the art -- and all of that passes
while the arrow can still sit tens of metres off the painted ribbon, because none
of it compares the two shipped things against EACH OTHER. This does:

  * the checkpoint markers out of each packed `<track>.vsc` are on the road. The
    notes establish that independently (`cp_N` is within 1.58 m of the driven
    `.aip` laps on all ten tracks -- see traps.md on the checkpoint polyline), so
    they are the best cheap stand-in for "where the road is" in WORLD space.
  * the ribbon in `trackmap_<n>.csi` is the road as PAINTED. A saturated-colour
    mask traces it; the mask over-reports (beach_1's sand and beach_2's dunes
    both trip it), which makes every number here optimistic.

So the printed distance is a LOWER BOUND on the transform's error, in metres, via
each track's own recovered scale.

THIS IS A PRINTED NUMBER AND NOT AN ASSERTION, deliberately. traps.md's rule for
an invariant that cannot be cleanly asserted -- "print the number instead and
watch it across a repack" -- applies exactly: the mask is approximate, so a
threshold here would be a threshold on the mask. Watch the numbers; a change in
them is a change in the transform.

  python3 mapcheck.py [--assets DIR] [--textures DIR] [--exe PATH]
"""

import argparse
import math
import os
import re
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import csi2png
import vsc_check
import gen_hud_data

IMAGE_BASE = 0x400000
MAP_WORLD_VA = 0x56C378
MAP_UV_VA = 0x56C468
TRACKS = ["beach_1", "beach_2", "beach_3", "beach_4", "country_1",
          "country_2", "country_3", "country_4", "urban_1", "urban_2"]


def affine(P, Q):
    """out = A0 + B (A1-A0) + C (A2-A0), with (B, C) solving the world triangle.

    Algebraically what FUN_004b8500 computes with 0x410430 -- read instruction by
    instruction, not assumed -- and what race_ui.c's solve_map computes."""
    (x0, z0), (x1, z1), (x2, z2) = P
    det = (x1 - x0) * (z2 - z0) - (x2 - x0) * (z1 - z0)
    if abs(det) < 1e-9:
        return None
    out = []
    for k in range(2):
        q0, q1, q2 = Q[0][k], Q[1][k], Q[2][k]
        a = ((q1 - q0) * (z2 - z0) - (q2 - q0) * (z1 - z0)) / det
        b = ((x1 - x0) * (q2 - q0) - (x2 - x0) * (q1 - q0)) / det
        out.append((a, b, q0 - a * x0 - b * z0))
    return out


def ribbon(path):
    """The painted road: saturated, opaque pixels. Over-reports on pale sand."""
    d = open(path, "rb").read()
    h = csi2png.parse_header(d)
    px = np.frombuffer(bytes(csi2png.decode_level(d, h.data_off, h.width,
                                                 h.height, h)),
                       dtype=np.uint8).reshape(h.height, h.width, 4).astype(int)
    a = px[:, :, 3]
    mx = px[:, :, :3].max(axis=2)
    mn = px[:, :, :3].min(axis=2)
    m = (a > 80) & (mx > 100) & ((mx - mn) > 50)
    ys, xs = np.nonzero(m)
    return xs.astype(float), ys.astype(float), h.width, h.height


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--assets", default="/mnt/c/tools/rccars_vita/assets")
    ap.add_argument("--textures",
                    default="/mnt/c/tools/rccars_extracted/Textures.1")
    ap.add_argument("--exe", default="/mnt/c/Games/RC Cars/RCCars.exe")
    ap.add_argument("--champ",
                    default="/mnt/c/tools/rccars_extracted/Scripts/"
                            "championship.ini")
    a = ap.parse_args()

    b = open(a.exe, "rb").read()

    def fl(va, n):
        return struct.unpack_from("<%df" % n, b, va - IMAGE_BASE)

    print("How far the recovered transform puts a cp_N marker from the painted")
    print("ribbon, in metres. A LOWER BOUND -- the ribbon mask over-reports.\n")
    # THE ENGINE'S LEVEL ORDER, not this port's. Everything indexed by a level --
    # trackmap_%i and both calibration tables -- is in the engine's, which is
    # Scripts/championship.ini's TrackN order. Indexing them by the port's order
    # is the bug this probe was originally built to measure the "residual" of:
    # eight of the ten tracks were drawing another track's map through another
    # track's transform, and the two that were not (beach_1, beach_3) are the two
    # that sit at their own index.
    order, disp = gen_hud_data.engine_levels(a.champ)
    lvl = [order.index(nm) for nm in TRACKS]

    print("%-11s %-12s %-9s  n  median   p90    max    m across the art"
          % ("track", "map", "engine"))
    meds = []
    for t, nm in enumerate(TRACKS):
        vsc = os.path.join(a.assets, nm + ".vsc")
        csi = os.path.join(a.textures, "trackmap_%d.csi" % (lvl[t] + 1))
        if not (os.path.exists(vsc) and os.path.exists(csi)):
            print("%-11s (missing %s)" % (nm, vsc if not os.path.exists(vsc)
                                          else csi))
            continue
        W = fl(MAP_WORLD_VA + 24 * lvl[t], 6)
        U = fl(MAP_UV_VA + 24 * lvl[t], 6)
        m = affine([(W[2 * i], W[2 * i + 1]) for i in range(3)],
                   [(U[2 * i], U[2 * i + 1]) for i in range(3)])
        (aa, bb, cc), (ee, ff, gg) = m
        scale = math.hypot(aa, ee)          # units of u per metre
        xs, ys, wpx, hpx = ribbon(csi)
        s = vsc_check.load(vsc)
        cps = [(x, z) for (n_, x, y, z, yaw) in s["markers"]
               if re.fullmatch(r"cp_\d+", n_)]
        if not cps:
            print("%-11s (no cp_N markers -- packed without --markers?)" % nm)
            continue
        ds = []
        for (x, z) in cps:
            u = (aa * x + bb * z + cc) * wpx
            v = (ee * x + ff * z + gg) * hpx
            ds.append(np.hypot(xs - u, ys - v).min() / wpx / scale)
        ds = np.array(ds)
        meds.append(float(np.median(ds)))
        print("%-11s trackmap_%-4d %-9s %2d  %6.1f %6.1f %6.1f   %5.1f"
              % (nm, lvl[t] + 1, disp[lvl[t]], len(ds), np.median(ds),
                 np.percentile(ds, 90), ds.max(), 1.0 / scale))
    if meds:
        print("\nmedian of the per-track medians: %.1f m" % np.median(meds))
        print("best track %.1f m, worst %.1f m" % (min(meds), max(meds)))


if __name__ == "__main__":
    main()
