#!/usr/bin/env python3
"""
Measure each car's rig geometry out of RCCarsDB/Car.sb.

The physics needs numbers the .crs files do not carry: where the wheels attach,
how big the body is. The original takes them from the mesh (carSetupWheelMounts
0x004f10d0 reads the model, then carSetupSprings offsets by springs[8..10]).
This reads the same source.

The LOD3 subtree is the one to use: its wheel nodes carry plain unrotated
translations, so no hierarchy composition is needed.

Usage: python3 car_geom.py [Car.sb]
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sb2obj as S

SB = "/mnt/c/Games/RC Cars/RCCarsDB/Car.sb"
CARS = ("Overkill", "Buggy", "Hummer")
WHEELS = ("WHEEL_FRONT_LEFT", "WHEEL_FRONT_RIGHT",
          "WHEEL_REAR_LEFT", "WHEEL_REAR_RIGHT")


def scan(path):
    b = open(path, "rb").read()
    rows = []

    def walk(off, end, path_, acc):
        p = off
        while p < end - 5:
            cid, cend, cnt, body = S.rd_chunk(b, p)
            if cend <= p or cend > end:
                break
            if cid == 0x9200:
                nm = S.node_name(b, body, cend) or "?"
                trs = S.node_trs(b, body, cend)
                t = trs[0] if trs else (0.0, 0.0, 0.0)
                w = (acc[0] + t[0], acc[1] + t[1], acc[2] + t[2])
                rows.append((path_ + "/" + nm, t, w))
                walk(body, cend, path_ + "/" + nm, w)
            p = cend

    p = 0
    while p < len(b) - 5:
        cid, cend, cnt, body = S.rd_chunk(b, p)
        if cid == 0x9200:
            walk(p, len(b), "", (0.0, 0.0, 0.0))
            break
        p = cend
    return rows


def composed(path=SB):
    """Per-car rig geometry with the node hierarchy properly COMPOSED.

    `scan` above sums translations and ignores parent rotation, which is only
    valid where the chain happens to be unrotated -- true of the Overkill's LOD3
    wheels and false everywhere else, which is why gen_rb_data.py had to hand the
    Buggy and Hummer the Overkill's numbers. That blocker is gone: the 0x540B
    Euler order in sb2obj is fixed (it built Rx*Ry*Rz where the engine's own
    modpTRSCalcLTM builds Rz*Ry*Rx), so the whole tree composes and every car's
    wheels come out mirror-symmetric.

    Returns {car_name: {...}} with, in metres:
        half_track   mean |x| over the wheel-centre nodes
        half_base    mean |z| over the wheels NOT on the centre line -- the
                     Hummer's middle axle sits at z = 0 and would drag the mean
                     down by a third
        wheel_y      mean wheel-centre height above the model origin
        extent       ENVIR_CAR_BODY's (x, y, z) size
        box          ENVIR_CAR_BODY's (lo, hi) corners, MODEL space
        wheels       {node name: (x, y, z)}

    `extent` is a SIZE and says nothing about where the shell sits; `box` is the
    same measurement before the subtraction. The difference is load-bearing and
    was a real bug: gen_rb_data.py used extent/2 as half-extents ABOUT THE BODY
    ORIGIN to keep the collision proxy "inside the shell it stands for", but the
    Overkill's shell spans y +0.068 .. +0.180 and the Hummer's +0.071 .. +0.190
    -- both wholly above the origin, so that clamp box does not overlap the
    shell in Y at all. Anything reasoning about where the bodywork IS wants
    `box`; only the inertia tensor wants `extent`.

    These INCLUDE the uniform 1.05 scale the car node chains carry, because that
    scale is part of the transform placing the mesh -- it is the size the car is
    actually drawn at. Only the top-level subtree is measured; LOD1/2/3 are
    reduced copies and pack_vsc.py flattens the top level.
    """
    b = open(path, "rb").read()
    reg, start = S.read_registry(b)
    hsem = next((k for k, v in reg.items() if v == "MOD_MESH"), None)
    out = {}

    def rec(p, end, depth, car, pM, pT, in_lod):
        while p < end:
            cid, cend, cnt, body = S.rd_chunk(b, p)
            if cend <= p or cend > end:
                return
            if cid == 0x9200:
                nm = S.node_name(b, body, cend) or "?"
                M, T = S.compose(pM, pT, S.node_trs(b, body, cend))
                mine = car
                if mine is None and nm in CARS:
                    mine = nm
                    out.setdefault(nm, {"wheels": {}, "extent": None,
                                    "box": None})
                lod = in_lod or ("LOD" in nm.upper())
                if mine and not lod and cnt == hsem:
                    _, vs, _, _ = S.parse_mesh(b, body, cend)
                    if vs:
                        base = nm[:-5] if nm.endswith("_body") else nm
                        u = base.upper()
                        if u.startswith("WHEEL_"):
                            out[mine]["wheels"].setdefault(u, T)
                        elif u == "ENVIR_CAR_BODY" and out[mine]["extent"] is None:
                            w = [(T[0] + M[0][0]*x + M[0][1]*y + M[0][2]*z,
                                  T[1] + M[1][0]*x + M[1][1]*y + M[1][2]*z,
                                  T[2] + M[2][0]*x + M[2][1]*y + M[2][2]*z)
                                 for (x, y, z) in vs]
                            lo = [min(q[i] for q in w) for i in range(3)]
                            hi = [max(q[i] for q in w) for i in range(3)]
                            out[mine]["extent"] = tuple(hi[i] - lo[i]
                                                        for i in range(3))
                            out[mine]["box"] = (tuple(lo), tuple(hi))
                if body < cend and depth < 24:
                    rec(body, cend, depth + 1, mine, M, T, lod)
            p = cend

    rec(start, len(b), 0, None, S.IDENT3, (0.0, 0.0, 0.0), False)

    for d in out.values():
        ws = d["wheels"]
        if not ws:
            continue
        d["half_track"] = sum(abs(t[0]) for t in ws.values()) / len(ws)
        off = [t for t in ws.values() if abs(t[2]) > 1e-3]
        d["half_base"] = (sum(abs(t[2]) for t in off) / len(off)) if off else 0.0
        d["wheel_y"] = sum(t[1] for t in ws.values()) / len(ws)
    return out


def mesh_sizes(objpath):
    """Body and wheel extents from the OBJ dump (parts are in local space)."""
    out, cur = {}, None
    if not os.path.exists(objpath):
        return out
    for line in open(objpath):
        if line.startswith("o "):
            cur = line[2:].strip()
            out.setdefault(cur, [])
        elif line.startswith("v ") and cur is not None:
            p = line.split()
            out[cur].append((float(p[1]), float(p[2]), float(p[3])))
    sizes = {}
    for k, vs in out.items():
        if not vs or k in sizes:
            continue
        xs = [v[0] for v in vs]; ys = [v[1] for v in vs]; zs = [v[2] for v in vs]
        sizes[k] = (max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs))
    return sizes


def main():
    rows = scan(sys.argv[1] if len(sys.argv) > 1 else SB)
    sizes = mesh_sizes("/mnt/c/tools/rccars_obj/Car.obj")

    for car in CARS:
        # the LOD3 subtree carries unrotated wheel translations
        pick = {}
        body_y = None
        for path, t, w in rows:
            if ("/" + car + "/") not in path or "LOD3" not in path:
                continue
            nm = path.split("/")[-1]
            if nm in WHEELS and nm not in pick:
                pick[nm] = t
            if nm == "ENVIR_CAR_BODY" and body_y is None:
                body_y = t[1]
        if len(pick) < 4:
            print("/* %s: LOD3 wheels not found */" % car)
            continue

        half_track = sum(abs(pick[k][0]) for k in WHEELS) / 4.0
        half_base = sum(abs(pick[k][2]) for k in WHEELS) / 4.0
        wheel_y = sum(pick[k][1] for k in WHEELS) / 4.0
        bs = sizes.get("ENVIR_CAR_BODY", (0.14, 0.106, 0.422))

        print("%-9s half_track=%.4f half_base=%.4f wheel_y=%.4f body_y=%.4f "
              "body_size=(%.3f %.3f %.3f)  wheel_below_body=%.4f"
              % (car, half_track, half_base, wheel_y,
                 body_y if body_y is not None else 0.0,
                 bs[0], bs[1], bs[2],
                 (body_y - wheel_y) if body_y is not None else 0.0))


if __name__ == "__main__":
    main()
