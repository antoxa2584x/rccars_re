#!/usr/bin/env python3
"""
What geometry does a track's .sb hold near a point, and what happens to it in
pack_vsc.py?

Two reported bugs need this: beach_1's columns floating near the last
checkpoint, and beach_3's missing round wall near the start. Both are questions
about a MESH -- its authored position, and whether the packer keeps it, drops
it, or keeps it with no texture (which scene_draw skips).

Usage:
    python3 nearby.py <track.sb> --at X Z [--rad R] [--sort dist|tris]
    python3 nearby.py <track.sb> --name REGEX
    python3 nearby.py <track.sb> --markers REGEX
"""

import argparse
import math
import re
import sys

import sb2obj
import pack_vsc


def load(path):
    b = open(path, "rb").read()
    reg, start = sb2obj.read_registry(b)
    meshes, textures = sb2obj.collect(b, reg, start)
    return b, reg, start, meshes, textures


def mesh_info(mesh_id, mesh, coarse):
    nm, vs, uv, faces = mesh[:4]
    xs = [v[0] for v in vs]
    ys = [v[1] for v in vs]
    zs = [v[2] for v in vs]
    tex = sorted({f[2] for f in faces if f[2]})
    untex = sum(1 for f in faces if f[2] is None)
    bad = sum(1 for f in faces if max(f[0]) >= len(vs))
    fate = []
    if mesh_id in coarse:
        fate.append("DROPPED(coarse water LOD)")
    if untex:
        fate.append("NO_TEXTURE x%d" % untex)
    if bad:
        fate.append("BAD_INDEX x%d" % bad)
    kind = pack_vsc.surface_kind(nm, {f[2] for f in faces})
    if kind:
        names = [n for n, bit in (("water", 2), ("coast", 4), ("stream", 8),
                                  ("fall", 16)) if kind & bit]
        fate.append("surface=" + ",".join(names))
    return dict(id=mesh_id, name=nm, nv=len(vs), nf=len(faces), tex=tex,
                cx=(min(xs) + max(xs)) / 2, cy=(min(ys) + max(ys)) / 2,
                cz=(min(zs) + max(zs)) / 2,
                x0=min(xs), x1=max(xs), y0=min(ys), y1=max(ys),
                z0=min(zs), z1=max(zs), fate=fate)


def show(i):
    f = ("  " + " ".join(i["fate"])) if i["fate"] else ""
    print("  [%4d] %-28s %5d tris  x %8.2f..%8.2f  y %7.2f..%7.2f  "
          "z %8.2f..%8.2f  tex=%s%s"
          % (i["id"], (i["name"] or "?")[:28], i["nf"], i["x0"], i["x1"],
             i["y0"], i["y1"], i["z0"], i["z1"],
             ",".join(i["tex"])[:40] or "-", f))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("--at", nargs=2, type=float, metavar=("X", "Z"))
    ap.add_argument("--rad", type=float, default=20.0)
    ap.add_argument("--name")
    ap.add_argument("--markers")
    ap.add_argument("--sort", default="dist", choices=("dist", "tris", "id"))
    ap.add_argument("--limit", type=int, default=80)
    a = ap.parse_args()

    b, reg, start, meshes, textures = load(a.src)
    print("%s: %d meshes, %d textures" % (a.src, len(meshes), len(textures)))

    if a.markers:
        mk = pack_vsc.collect_markers(b, reg, start, re.compile(a.markers))
        print("markers matching %r: %d" % (a.markers, len(mk)))
        for nm, T, yaw in sorted(mk):
            print("  %-20s  (%9.3f %8.3f %9.3f)  yaw %8.2f"
                  % (nm, T[0], T[1], T[2], yaw))
        return

    coarse = pack_vsc.pick_water_lods(meshes)
    infos = [mesh_info(i, m, coarse) for i, m in enumerate(meshes)]

    if a.name:
        rx = re.compile(a.name, re.I)
        sel = [i for i in infos if i["name"] and rx.search(i["name"])]
        print("meshes matching %r: %d" % (a.name, len(sel)))
    elif a.at:
        x, z = a.at
        sel = []
        for i in infos:
            # distance from the point to the mesh's XZ box, so a big terrain
            # tile the point sits on reads as distance 0
            dx = max(i["x0"] - x, 0.0, x - i["x1"])
            dz = max(i["z0"] - z, 0.0, z - i["z1"])
            d = math.hypot(dx, dz)
            if d <= a.rad:
                i["d"] = d
                sel.append(i)
        print("meshes within %.1f m of (%.2f, %.2f): %d" % (a.rad, x, z, len(sel)))
        if a.sort == "dist":
            sel.sort(key=lambda i: i["d"])
    else:
        sel = infos
    if a.sort == "tris":
        sel.sort(key=lambda i: -i["nf"])
    elif a.sort == "id":
        sel.sort(key=lambda i: i["id"])
    for i in sel[:a.limit]:
        show(i)
    if len(sel) > a.limit:
        print("  ... %d more" % (len(sel) - a.limit))


if __name__ == "__main__":
    main()
