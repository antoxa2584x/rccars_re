#!/usr/bin/env python3
"""
Decode CREAT Studios .bspl spline files (RC Cars).

A .bspl is a miniature scene file using the same TIOX chunk framing as .sb:
a 0x3801 marker, a 0x4802 comment string, optionally the full 0x9A00 type
registry, then one or more 0x9200 nodes tagged 'NLPS' (reversed SPLN =
MOD_SPLINE).

Inside a spline node:

    0x3408 [5]   index
    0x305D       point count
    0x865D [n]   container; n * 0x805B, each 12 bytes = 3 floats

The x component is 0 throughout RC Cars' splines, so these are 2D curves
carried in (y, z): y is the input (e.g. speed), z the output (e.g.
acceleration). splAccelBase1 runs z 9.55 -> 0 as y goes 0 -> 25, i.e.
acceleration falling off to zero at top speed.

The sibling .spl text format holds the same idea as a cubic Bezier chain:
a segment count, then 4 control points per segment, each segment starting
where the previous ended.

Usage: python3 bspl2csv.py <dir-or-file> [--csv OUTDIR]
"""

import argparse
import glob
import os
import struct

CH_MARKER = 0x3801
CH_POINTS = 0x865D
CH_POINT = 0x805B
CH_COUNT = 0x305D
TAG_SPLINE = 0x53504C4E          # 'NLPS' little-endian -> "SPLN"


def rd_chunk(b, p):
    cid = struct.unpack_from("<H", b, p)[0]
    end = struct.unpack_from("<I", b, p + 2)[0]
    n = p + 6
    cnt = None
    if (cid & 0x400) or (cid & 0xF000) == 0x9000:
        cnt = struct.unpack_from("<I", b, n)[0]
        n += 4
    return cid, end, cnt, n


def find_all(b, p, end, target, depth=0, out=None):
    if out is None:
        out = []
    while p < end:
        cid, ce, cnt, bd = rd_chunk(b, p)
        if ce <= p or ce > len(b):
            return out
        if cid == target:
            out.append((bd, ce, cnt))
        elif (cid & 0xF000) in (0x0000, 0x5000, 0x9000) and bd < ce and depth < 8:
            find_all(b, bd, ce, target, depth + 1, out)
        p = ce
    return out


def read_points(b, bd, ce):
    pts = []
    p = bd
    while p < ce:
        cid, e2, n2, b2 = rd_chunk(b, p)
        if e2 <= p or e2 > len(b):
            break
        if cid == CH_POINT and e2 - b2 >= 12:
            pts.append(struct.unpack_from("<3f", b, b2))
        p = e2
    return pts


def decode(path):
    b = open(path, "rb").read()
    curves = []
    for bd, ce, cnt in find_all(b, 0, len(b), CH_POINTS):
        pts = read_points(b, bd, ce)
        if pts:
            curves.append(pts)
    return curves


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("--csv", default=None)
    a = ap.parse_args()

    files = ([a.src] if os.path.isfile(a.src)
             else sorted(glob.glob(os.path.join(a.src, "*.bspl"))))
    if a.csv:
        os.makedirs(a.csv, exist_ok=True)

    for f in files:
        curves = decode(f)
        name = os.path.basename(f)
        if not curves:
            print(f"{name:<26} (no spline points found)")
            continue
        for i, pts in enumerate(curves):
            flat = "" if len(curves) == 1 else f" curve{i}"
            ys = [p[1] for p in pts]
            zs = [p[2] for p in pts]
            print(f"{name:<26}{flat} {len(pts):>3} pts   "
                  f"in [{min(ys):8.3f},{max(ys):8.3f}]  out [{min(zs):8.3f},{max(zs):8.3f}]")
            if a.csv:
                stem = os.path.splitext(name)[0] + (f"_{i}" if len(curves) > 1 else "")
                with open(os.path.join(a.csv, stem + ".csv"), "w") as fh:
                    fh.write("in,out\n")
                    for p in pts:
                        fh.write(f"{p[1]:.6f},{p[2]:.6f}\n")


if __name__ == "__main__":
    main()
