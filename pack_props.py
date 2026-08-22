#!/usr/bin/env python3
"""
pack_props.py -- the 13 knockable props into one props.vsc (VSC8).

The props live in RCCarsDB/stone.sb, one MOD_MODEL subtree each, and they are
shared by all ten tracks -- so they pack ONCE into a scene of their own rather
than into every track. 1,748 triangles and 13 textures for the lot, which is
nothing against a track's 48,000.

VSC8 adds two things to VSC7, both of them only meaningful in a file like this
one:

    header    u32 n_models,  then per model  u16 namelen + name
    batch     u32 model      (which model this batch belongs to)

A track or a car packs n_models = 0 and writes no per-batch field, so nothing
about the existing assets changes and scene.c keeps loading VSC3..VSC7 exactly
as before.

WHY NAMES AND NOT JUST ORDER. prop_data.h is generated from the same stone.sb by
gen_prop_data.py and comes out in the same order, so index N would agree here by
construction -- right up until one of the two scripts changes how it walks the
file, at which point every prop silently gets someone else's mass and collision
shape and nothing fails. prop.c binds by name and logs what it could not
resolve, which is the same rule carani_bind follows for the car rig.

This file writes geometry and textures only. Every physics number, including the
collision proxies and the fit for the two geometry types the engine ships no
section for, is gen_prop_data.py's job and arrives in prop_data.h. The one thing
the two share is model_xz_centre, imported rather than copied, because the drawn
mesh and the collision spheres have to agree about where a prop's origin is.

Usage:
    python3 pack_props.py /mnt/c/tools/rccars_vita/assets/props.vsc
"""
import argparse
import math
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sb2obj as S
import pack_vsc as P
# One definition of the re-basing, shared with the proxy fit. The drawn mesh and
# the collision spheres have to agree about where a prop's origin is, and two
# copies of that rule would eventually not.
from gen_prop_data import model_xz_centre

STONE_SB = "/mnt/c/Games/RC Cars/RCCarsDB/stone.sb"

# stone.sb's own MOD_MODEL order. gen_prop_data.py walks the same file and emits
# PROP_MODELS in this order; prop.c still binds by name.
MODELS = ["Ball", "Cola", "Whiskey1", "Whiskey2", "Tonic", "Conus", "Klubok",
          "Vedro", "Shell", "Ball_CS", "Ball_CB", "Kettle", "Ballon"]

MAX_VERTS = 65000


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--src", default=STONE_SB)
    ap.add_argument("--texroot", default="/mnt/c/tools/rccars_extracted")
    ap.add_argument("--texroot2", default="",
                    help="a SECOND .csi root, searched after --texroot and "
                         "winning on a name they both carry. The engine's own "
                         "font lives outside the pack -- Language/English/ holds "
                         "Smash20.csi and Smash26.csi, which is why this port "
                         "spent years recording that the game shipped no "
                         "reusable glyph atlas -- and --texset does not apply to "
                         "it, because that directory is not one of the three "
                         "quality sets.")
    ap.add_argument("--texset", default="1",
                    help="1 = 512px, 2 = 256, 3 = 128. Pick by SIZE, never by "
                         "directory order -- see index_csi.")
    ap.add_argument("--alpha", action="store_true", default=True)
    ap.add_argument("--extra-tex", default="",
                    help="comma-separated textures to pack even though no "
                         "geometry references them, for anything the app binds "
                         "by NAME at runtime. The same option pack_vsc.py has, "
                         "and here for a reason of its own: this is the app's "
                         "one shared LOAD-ONCE scene -- built before the first "
                         "track and never reloaded -- so a texture that belongs "
                         "to no track and no car belongs in it rather than in "
                         "all ten tracks. That is what `msg_hits` is: the game's "
                         "own on-screen HIT banner (message slots 3 and 4), "
                         "which hud.c binds by name.")
    a = ap.parse_args()

    b = open(a.src, "rb").read()
    reg, start = S.read_registry(b)
    csi = P.index_csi(a.texroot, prefer=a.texset)
    if a.texroot2:
        # No `prefer`: the second root is not one of the three quality sets, so
        # filtering it by `Textures.<n>` would drop everything in it.
        csi.update(P.index_csi(a.texroot2))

    groups = {}
    proxies = {}
    model_names = []
    dropped = 0

    for mi, model in enumerate(MODELS):
        meshes, textures = S.collect(b, reg, start, subtree=model)
        if not meshes:
            raise SystemExit("stone.sb: no geometry under %r" % model)
        model_names.append(model)

        allv = [v for m in meshes for v in m[1]]
        proxies[model] = allv
        # Conus is modelled 5.5 m off its own origin; every other prop is on it.
        # See model_xz_centre for how that was settled against the collision
        # grids. Y is deliberately left alone -- it is what rests a prop on the
        # ground rather than half through it.
        ox, oz = model_xz_centre(allv)

        for mesh_id, mesh in enumerate(meshes):
            nm, vs, uv, faces = mesh[:4]
            for (vidx, uvidx, tname, lmidx, lmname,
                 _uv2, _tname2, _aidx, _aname) in faces:
                if max(vidx) >= len(vs):
                    dropped += 1
                    continue
                # model joins the grouping key: a batch is drawn under ONE
                # prop's matrix, so two models can never share one.
                g = groups.setdefault((tname, mi),
                                      [{"vmap": {}, "verts": [], "idx": []}])
                cur = g[-1]
                if len(cur["verts"]) + 3 > MAX_VERTS:
                    cur = {"vmap": {}, "verts": [], "idx": []}
                    g.append(cur)
                for k in range(3):
                    vi = vidx[k]
                    ti = uvidx[k] if (uvidx and uvidx[k] < len(uv)) else None
                    key = (mesh_id, vi, ti)
                    slot = cur["vmap"].get(key)
                    if slot is None:
                        slot = len(cur["verts"])
                        cur["vmap"][key] = slot
                        px, py, pz = vs[vi]
                        px -= ox
                        pz -= oz
                        tu, tv = uv[ti] if ti is not None else (0.0, 0.0)
                        cur["verts"].append((px, py, pz, tu, tv, 0.0, 0.0))
                    cur["idx"].append(slot)

        # textures declared inside this model's scope have to be collected as we
        # go: S.collect is called per subtree, so the last call's table is not
        # the whole set.
        for n, rec in textures.items():
            TEXTURES.setdefault(n, rec)

    names = sorted({t for (t, _m) in groups if t})
    # Textures no batch references, resolved out of the .csi tree like any other.
    # The scene format carries its texture table independently of its batches, so
    # one with nothing pointing at it costs exactly its own bytes and scene_tex
    # finds it by name.
    names += [n for n in a.extra_tex.split(",") if n and n not in names]
    tex_blobs, tex_index, missing = [], {}, []
    for n in names:
        blob = TEXTURES.get(n, {}).get("embedded")
        p = csi.get(n.lower())
        if not blob and not p:
            missing.append(n)
            continue
        try:
            if blob:
                w, h, fmt, levels = P.load_texture_data(b[blob[0]:blob[1]], a.alpha)
            else:
                w, h, fmt, levels = P.load_texture(p, a.alpha)
        except Exception:
            missing.append(n)
            continue
        if not levels:
            missing.append(n)
            continue
        tex_index[n] = len(tex_blobs)
        tex_blobs.append((n, w, h, fmt, levels))

    batches = []
    for (tname, mi), gl in groups.items():
        ti = tex_index.get(tname, 0xFFFFFFFF)
        for g in gl:
            if g["idx"]:
                batches.append((ti, 0, 0, 0xFFFFFFFF, 0, None,
                                g["verts"], g["idx"], mi))

    with open(a.out, "wb") as f:
        f.write(b"VSC8")
        f.write(struct.pack("<II", len(tex_blobs), len(batches)))
        f.write(struct.pack("<I", 0))               # parts (no rig)
        f.write(struct.pack("<If", 0, 0.0))         # markers, shadow radius
        f.write(struct.pack("<I", len(model_names)))
        for nm in model_names:
            nb = nm.encode("latin-1")
            f.write(struct.pack("<H", len(nb)))
            f.write(nb)
        for n, w, h, fmt, levels in tex_blobs:
            nb = n.encode("latin-1")
            f.write(struct.pack("<H", len(nb)))
            f.write(nb)
            f.write(struct.pack("<HHBB", w, h, fmt, len(levels)))
            for px in levels:
                f.write(px)
        for ti, flags, part, li, env, nrm, verts, idx, mi in batches:
            f.write(struct.pack("<III", ti, flags, part))
            f.write(struct.pack("<I", li))          # lightmap: none
            f.write(struct.pack("<I", env))         # env class: none
            f.write(struct.pack("<I", mi))          # VSC8: the model
            f.write(struct.pack("<II", len(verts), len(idx)))
            for v in verts:
                f.write(struct.pack("<7f", *v))
            f.write(struct.pack("<%dH" % len(idx), *idx))

    nv = sum(len(x[6]) for x in batches)
    ni = sum(len(x[7]) for x in batches)
    print("%s: VSC8  %d models, %d batches, %d verts, %d tris, %d textures"
          % (os.path.basename(a.out), len(model_names), len(batches), nv,
             ni // 3, len(tex_blobs)))
    if dropped:
        print("   %d faces dropped (index past the vertex array)" % dropped)
    if missing:
        print("   MISSING textures: %s" % ", ".join(missing))


TEXTURES = {}

if __name__ == "__main__":
    main()
