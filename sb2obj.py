#!/usr/bin/env python3
"""
Export geometry from CREAT Studios .sb scene databases to Wavefront OBJ.

.sb reuses the TIOX chunk framing (see FORMAT_NOTES.md). Chunk 'end' fields are
ABSOLUTE file offsets at every nesting level, including inside face records.

Scene layout:
    0x3801                      file marker
    0x4802  string              "CREAT Studio Scene Project 6.0"
    0x9A00 * N                  type registry: chunk count field = type 4CC
                                (byte-reversed: HSEM->MESH, RTAM->MATR, ...),
                                child 0x4803 string = "MOD_MESH" etc.
    0x9200 [type 4CC]           scene nodes, nested to form the scene graph

Inside a MOD_MESH node:
    0x4003  string              node name
    0x7411 [n]  n * vec3        vertex positions
    0x7413 [n]  n * vec2        UVs (per face-vertex; indexing not yet mapped)
    0x7414 [2]  2 * vec3        bounding box (min, max)
    0x540B [9] / 0x5438 [9]     3x3 matrices
    0x0617 [n]                  n face records, each a 0x8218 blob

Face record 0x8218 -- a u32 lead value, then chunks:
    0x3419 [3]                  per-face info
    0x3020                      packed ARGB colour
    0x7027       vec3           face normal
    0x063A -> 0x023B ->
        0x303D                  material 4CC
        0x343F [3]  3 * u32     TRIANGLE vertex indices

Usage:
    python3 sb2obj.py <file.sb> [out.obj] [--limit N]
"""

import argparse
import glob
import os
import re
import struct

import csi2png


def mtl_name(ix, name):
    """OBJ material names must not contain whitespace."""
    return f"{ix:03d}_{re.sub(r'[^A-Za-z0-9_.-]', '_', name)}"


def rd_chunk(b, p):
    cid = struct.unpack_from("<H", b, p)[0]
    end = struct.unpack_from("<I", b, p + 2)[0]
    n = p + 6
    cnt = None
    if (cid & 0x400) or (cid & 0xF000) == 0x9000:
        cnt = struct.unpack_from("<I", b, n)[0]
        n += 4
    return cid, end, cnt, n


def read_registry(b):
    """Type 4CC -> MOD_ name. Ends at the first scene node."""
    reg, p = {}, 0
    while p < len(b):
        cid, end, cnt, body = rd_chunk(b, p)
        if end <= p or end > len(b):
            break
        if cid == 0x9A00 and cnt is not None:
            c2, e2, _, b2 = rd_chunk(b, body)
            reg[cnt] = b[b2:e2].split(b"\0")[0].decode("latin-1")
        elif cid == 0x9200:
            break
        p = end
    return reg, p


def children(b, p, end):
    out = []
    while p < end:
        cid, cend, cnt, body = rd_chunk(b, p)
        if cend <= p or cend > end:
            break
        out.append((cid, cnt, body, cend))
        p = cend
    return out


def walk_all(b, p, end, depth, acc, maxdepth=16):
    """Flatten every chunk in a subtree."""
    while p < end:
        cid, cend, cnt, body = rd_chunk(b, p)
        if cend <= p or cend > end:
            return
        acc.append((cid, cnt, body, cend))
        if (cid & 0xF000) in (0x0000, 0x5000, 0x9000) and body < cend and depth < maxdepth:
            walk_all(b, body, cend, depth + 1, acc, maxdepth)
        p = cend


TAG_BASE_UV = 0x42415331   # '1SAB' little-endian -> "BAS1", the base UV set
TAG_LIGHTMAP = 0x4C4D5F31  # '1_ML' little-endian -> "LM_1", the lightmap set
                           # beach_1: 51,384 faces carry BAS1 and 28,171 also
                           # carry LM_1, which indexes two 512x512 atlases
                           # embedded in the .sb itself
TAG_BASE2_UV = 0x42415332  # '2SAB' -> "BAS2", a SECOND base texture
TAG_ALPHA_UV = 0x414C5031  # '1PLA' -> "ALP1", the MASK that blends the two

# BAS2 and ALP1 are the game's surface-transition blend, and they are not a
# guess: FUN_0046fc20 is a 4CC -> layer-type map the loader calls per UV layer
# (_mod2ConvertFace, 0x00435ed0, whose error string names texInfoType), and it
# answers BAS1..BAS4 -> 0..3, LM_1 -> 0x10, DET1 -> 0x100 and ALP1..ALP4 ->
# 0x1000..0x1003. modpFaceIsTextureBase (0x00432280) accepts BAS1..BAS9 for the
# same reason. So a face may carry several base textures and an alpha per base,
# and the ten tracks use exactly two of each:
#
#   BAS1 + BAS2 + ALP1 + LM_1   the terrain transition -- two ground textures
#                               lerped by the `alpha` mask, 3,608 faces over
#                               nine tracks (country_1 has none)
#   BAS1 + ALP1                 the coast foam band -- `coast_base` masked by
#                               `coast`, 601 faces on the four beaches and
#                               country_3, and NO lightmap
#
# Which side the mask selects was measured, not assumed. ALP1's v is exactly 0
# or 1 at 98% of transition vertices (the band is one quad wide and the mask a
# vertical ramp), and where a transition face shares a vertex with a face
# painted in ONE texture, v is 1.0 on 3,174 of 3,188 BAS1 neighbours and 0.0 on
# 3,146 of 3,160 BAS2 ones -- 0.2% the other way. Both masks are black in their
# first row and white in their last, so mask 1 = BAS1 and mask 0 = BAS2:
#
#     colour = mask * BAS1 + (1 - mask) * BAS2
#
# DET1 appears in no track file, so the `detailer*` textures are ordinary base
# maps here.


def parse_mesh(b, body, end):
    """
    Return (name, verts, uvs, faces) for one MOD_MESH node's own geometry.
    faces is a list of (vidx3, uvidx3|None, texid, lmidx3|None, lmtexid,
    uvidx3_2|None, texid2, aidx3|None, atexid) -- nine elements: the base layer,
    the lightmap layer, then the SECOND base and the ALPHA mask that blends the
    two. See TAG_BASE2_UV.

    Vertex indices come from 0x3419 -- NOT from 0x343F. Each 0x023B layer under
    0x063A carries its own 0x343F, and every one of them indexes the UV array
    (base and lightmap sets), which is why treating any of them as positions
    overruns on meshes where the UV count exceeds the vertex count.
    """
    name, verts, uvs, faces = None, [], [], []
    faces_span = None
    for cid, cnt, cb, ce in children(b, body, end):
        if cid == 0x4003:
            name = b[cb:ce].split(b"\0")[0].decode("latin-1")
        elif cid == 0x7411 and cnt:
            verts += [struct.unpack_from("<3f", b, cb + i * 12) for i in range(cnt)]
        elif cid == 0x7413 and cnt:
            uvs += [struct.unpack_from("<2f", b, cb + i * 8) for i in range(cnt)]
        elif cid == 0x0617:
            faces_span = (cb, ce)

    if faces_span:
        cb, ce = faces_span
        for rid, rcnt, rb, re_ in children(b, cb, ce):
            vidx = uvidx = tidx = lmidx = lmtidx = None
            uvidx2 = tidx2 = aidx = atidx = None
            for c2, n2, b2, e2 in children(b, rb + 4, re_):   # skip u32 lead
                if c2 == 0x3419 and e2 - b2 >= 12:
                    vidx = struct.unpack_from("<3I", b, b2)
                elif c2 == 0x063A:
                    first = first_t = None
                    for c3, n3, b3, e3 in children(b, b2, e2):   # 0x023B layers
                        tag = ind = tex = None
                        for c4, n4, b4, e4 in children(b, b3, e3):
                            if c4 == 0x303C and e4 - b4 >= 4:
                                tex = struct.unpack_from("<I", b, b4)[0]
                            elif c4 == 0x303D and e4 - b4 >= 4:
                                tag = struct.unpack_from("<I", b, b4)[0]
                            elif c4 == 0x343F and e4 - b4 >= 12:
                                ind = struct.unpack_from("<3I", b, b4)
                        if ind is None:
                            continue
                        if first is None:
                            first, first_t = ind, tex
                        # Do NOT break on BAS1: the lightmap layer is a second
                        # 0x023B under the same 0x063A and used to be discarded
                        # here, which is why the port rendered unlit.
                        if tag == TAG_BASE_UV:
                            uvidx, tidx = ind, tex
                        elif tag == TAG_LIGHTMAP:
                            lmidx, lmtidx = ind, tex
                        elif tag == TAG_BASE2_UV:
                            uvidx2, tidx2 = ind, tex
                        elif tag == TAG_ALPHA_UV:
                            aidx, atidx = ind, tex
                    if uvidx is None:
                        uvidx, tidx = first, first_t
            if vidx:
                faces.append((vidx, uvidx, tidx, lmidx, lmtidx,
                              uvidx2, tidx2, aidx, atidx))
    return name, verts, uvs, faces


def scope_textures(b, body, end, glb):
    """
    MOD_GLBTEXTURE nodes declared as DIRECT children of one node.
      0x4003 name, 0x3408 index, 0x4034 original source path,
      0x8232 embedded CSI image (lightmaps, which have no source file)

    Texture indices are scoped to the declaring subtree, not global: Car.sb
    declares 101 textures using only 9 distinct indices. Track files happen to
    use globally unique indices, which makes a flat table look correct there
    and silently collapse everywhere else.
    """
    out = {}
    for cid, cnt, cb, ce in children(b, body, end):
        if cid != 0x9200 or cnt != glb:
            continue
        nm, ix, emb, words = None, None, None, ()
        for c2, n2, b2, e2 in children(b, cb, ce):
            if c2 == 0x4003:
                nm = b[b2:e2].split(b"\0")[0].decode("latin-1")
            elif c2 == 0x3408 and e2 - b2 >= 4:
                # The record is FIVE dwords, not one: index, 0, a format/type
                # word (0x4001 almost everywhere), then two flag words. Word 4
                # is the SURFACE CLASS -- see pack_col.eng_surface_class, which
                # is FUN_00535060's own priority chain over its bits.
                words = struct.unpack_from("<%dI" % ((e2 - b2) // 4), b, b2)
                ix = words[0]
            elif c2 == 0x8232:
                emb = (b2, e2)
        if ix is None:
            ix = len(out)
        out[ix] = {"name": nm or f"tex{ix}", "embedded": emb, "words": words}
    return out


def node_name(b, body, end):
    for cid, cnt, cb, ce in children(b, body, end):
        if cid == 0x4003:
            return b[cb:ce].split(b"\0")[0].decode("latin-1")
    return None


IDENT3 = ((1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0))


def node_trs(b, body, end):
    """
    Local transform from chunk 0x540B: NINE floats that are three vec3s --
    translation, scale, and rotation in DEGREES (not a 3x3 matrix; the middle
    triple is exactly (1,1,1) on unrotated nodes, which is the giveaway).

    The rotation is applied X, then Y, then Z -- see _rot3, which is read
    straight off the engine's own builder rather than inferred.
    """
    for cid, cnt, cb, ce in children(b, body, end):
        if cid == 0x540B and ce - cb >= 36:
            v = struct.unpack_from("<9f", b, cb)
            return v[0:3], v[3:6], v[6:9]
    return None


def _mul3(A, B):
    return tuple(tuple(sum(A[i][k] * B[k][j] for k in range(3)) for j in range(3))
                 for i in range(3))


def _rot3(rx, ry, rz):
    """The 0x540B Euler triple as a COLUMN-vector rotation, R = Rz * Ry * Rx --
    i.e. X is applied first and Z last.

    Read off the engine's own builder, `FUN_004089b0` (called by
    `modpTRSCalcLTM 0x00434730` with the node's +0x90/+0x9c/+0xa8 triples and
    the local matrix at +0xb4). It loads the identity, then calls, EACH WITH
    MODE 1 -- pre-multiply, m = X * m:

        0x0040c880 translate    0x0040cb30 rotZ    0x0040cab0 rotY
        0x0040ca30 rotX         0x0040c9a0 scale

    in that order, so the row-vector local matrix is S * Rx * Ry * Rz * T.
    Transposing into this function's column-vector convention leaves the
    rotation as Rz * Ry * Rx.

    This used to build Rx * Ry * Rz, the reverse, which is the same matrix only
    when two of the three angles are zero -- true of almost every node in a
    track, and false for the cars' suspension. It left the Overkill's four LEFT
    spring nodes aimed up into the body instead of down at the axle, and it is
    why the Buggy and Hummer wheel mounts could not be composed out of the mesh
    at all. Three independent confirmations that this order is the right one:

      - it is what the disassembly above does;
      - it makes the eight SPRING_* nodes EXACTLY mirror-symmetric across x
        (the old order: 20 mm of asymmetry), which the art must be, since the
        wheels already came out mirrored to 1e-7;
      - it puts each spring's aim point, `origin + SpringLength * local_Z`,
        on the axle it belongs to -- SPRING_FRONT_2's lands at y = 0.0567,
        which is FRONT_AXLE's own origin. The old order aimed four of the
        eight at y = +0.185, above the roof.
    """
    import math
    R = IDENT3
    for ax, a in (("z", rz), ("y", ry), ("x", rx)):
        c, s = math.cos(a * math.pi / 180.0), math.sin(a * math.pi / 180.0)
        M = (((1, 0, 0), (0, c, -s), (0, s, c)) if ax == "x" else
             ((c, 0, s), (0, 1, 0), (-s, 0, c)) if ax == "y" else
             ((c, -s, 0), (s, c, 0), (0, 0, 1)))
        R = _mul3(R, M)
    return R


def compose(pM, pT, trs):
    """world = parent ∘ local, with local(v) = t + R*S*v"""
    if not trs:
        return pM, pT
    t, s, r = trs
    R = _rot3(r[0], r[1], r[2])
    RS = tuple(tuple(R[i][j] * s[j] for j in range(3)) for i in range(3))
    M = _mul3(pM, RS)
    T = tuple(pT[i] + sum(pM[i][k] * t[k] for k in range(3)) for i in range(3))
    return M, T


def collect(b, reg, start, subtree=None, part_names=None):
    """
    Walk the scene graph, yielding each mesh's geometry with face texture
    indices already resolved to texture NAMES through the enclosing scope
    chain (innermost declaration wins).

    subtree: if given, only meshes at or below a node with that name are
    returned -- e.g. "Car1" pulls one car out of Car.sb's shared database.
    Texture scopes are still accumulated from the root so declarations made
    above the subtree remain visible.

    Returns (meshes, textures) where meshes is a list of
    (name, verts, uvs, faces) with faces the NINE-element tuple parse_mesh
    builds, texture indices resolved to names:
    (vidx3, uvidx3, texname, lmidx3, lmname, uvidx3_2, texname2, aidx3, aname).
    textures maps texname -> {"embedded": (off,end)|None}.

    part_names: if given, a set of node names that stay ADDRESSABLE after the
    hierarchy is flattened -- the car's rig, the nodes the game's own animation
    procs look up by name (carCreateAniProc1 0x00504100 reads them out of the
    table at 0x00572fcc). Meshes keep their baked model-space vertices, but each
    one also records which part it belongs to, and the parts record enough of
    the hierarchy to re-articulate it at runtime. The return becomes a 3-tuple
    (meshes, textures, parts) with a 5th element on every mesh, its part index:

        meshes[i] = (name, verts, uvs, faces, part_index)
        parts[j]  = (name, parent_index, M, T)

    where (M, T) is the node's REST transform in the same space as the baked
    vertices, so `rest_inv * animated` is the correction to apply to them.
    Part 0 is always the implicit root and is the identity.
    """
    hsem = next((k for k, v in reg.items() if v == "MOD_MESH"), None)
    glb = next((k for k, v in reg.items() if v == "MOD_GLBTEXTURE"), None)
    out, seen = [], {}
    want = set(part_names or ())
    # part 0 is the implicit root: everything not under a named node hangs here
    parts = [("__root__", -1, IDENT3, (0.0, 0.0, 0.0))]

    def remember(t):
        """Record a texture, preferring the declaration that CARRIES the image.

        The same name is declared in more than one scope, and only one of them
        has the embedded 0x8232 blob. setdefault keeps whichever came first,
        which on beach_4 was the empty one -- so its two lightmap atlases
        resolved to nothing and the whole track packed unlit while beach_1,
        which happens to declare them the other way round, packed fine."""
        old = seen.get(t["name"])
        if old is None or (t["embedded"] and not old["embedded"]):
            seen[t["name"]] = t

    def resolve(ix, scopes):
        for sc in reversed(scopes):
            if ix in sc:
                return sc[ix]
        return None

    def rec(p, end, depth, scopes, inside, pM, pT, part):
        while p < end:
            cid, cend, cnt, body = rd_chunk(b, p)
            if cend <= p or cend > end:
                return
            if cid == 0x9200:
                local = scope_textures(b, body, cend, glb)
                sc = scopes + [local] if local else scopes
                # Lightmaps are referenced only by the LM_1 UV layer, which OBJ
                # cannot express, so they never enter the material table via
                # faces. Keep them anyway -- the embedded image is real data.
                for t in local.values():
                    if t["embedded"]:
                        remember(t)
                nname = node_name(b, body, cend)
                here = inside
                if subtree and not inside and nname == subtree:
                    here = True
                # node transforms compose down the tree; ignoring them leaves a
                # hierarchical rig (a car) scattered in pieces around the origin
                M, T = compose(pM, pT, node_trs(b, body, cend))
                mypart = part
                if here and nname in want:
                    mypart = len(parts)
                    parts.append((nname, part, M, T))
                if cnt == hsem and here:
                    nm, vs, uv, fs = parse_mesh(b, body, cend)
                    vs = [(T[0] + M[0][0]*x + M[0][1]*y + M[0][2]*z,
                           T[1] + M[1][0]*x + M[1][1]*y + M[1][2]*z,
                           T[2] + M[2][0]*x + M[2][1]*y + M[2][2]*z)
                          for (x, y, z) in vs]
                    if vs and fs:
                        named = []
                        for (vidx, uvidx, tidx, lmidx, lmtidx,
                             uvidx2, tidx2, aidx, atidx) in fs:
                            t = resolve(tidx, sc) if tidx is not None else None
                            if t:
                                remember(t)
                            lm = resolve(lmtidx, sc) if lmtidx is not None else None
                            if lm:
                                remember(lm)
                            # The second base and its mask resolve through the
                            # same scope chain as the first. Both are remembered:
                            # the mask is referenced ONLY by an ALP1 layer, so a
                            # collector that skips it packs a scene with a blend
                            # and nothing to blend by.
                            t2 = resolve(tidx2, sc) if tidx2 is not None else None
                            if t2:
                                remember(t2)
                            am = resolve(atidx, sc) if atidx is not None else None
                            if am:
                                remember(am)
                            named.append((vidx, uvidx, t["name"] if t else None,
                                          lmidx, lm["name"] if lm else None,
                                          uvidx2, t2["name"] if t2 else None,
                                          aidx, am["name"] if am else None))
                        out.append((nm or f"mesh_{p}", vs, uv, named, mypart))
                if body < cend and depth < 24:
                    rec(body, cend, depth + 1, sc, here, M, T, mypart)
            p = cend

    root = scope_textures(b, start, len(b), glb)
    rec(start, len(b), 0, [root] if root else [], subtree is None,
        IDENT3, (0.0, 0.0, 0.0), 0)
    if part_names is None:
        return [m[:4] for m in out], seen
    return out, seen, parts


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("out", nargs="?")
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--pngdir", default="/mnt/c/tools/rccars_png",
                    help="converted CSI textures, for map_Kd resolution")
    a = ap.parse_args()

    b = open(a.src, "rb").read()
    reg, start = read_registry(b)
    meshes, textures = collect(b, reg, start)
    if a.limit:
        meshes = meshes[: a.limit]

    nv = sum(len(m[1]) for m in meshes)
    nu = sum(len(m[2]) for m in meshes)
    nt = sum(len(m[3]) for m in meshes)
    print(f"{os.path.basename(a.src)}: {len(meshes)} meshes, {nv:,} verts, "
          f"{nu:,} uvs, {nt:,} tris")

    allv = [v for m in meshes for v in m[1]]
    if allv:
        for i, ax in enumerate("xyz"):
            lo = min(v[i] for v in allv)
            hi = max(v[i] for v in allv)
            print(f"   {ax}: [{lo:10.2f}, {hi:10.2f}]")

    out = a.out or os.path.splitext(a.src)[0] + ".obj"
    outdir = os.path.dirname(os.path.abspath(out))
    os.makedirs(outdir, exist_ok=True)
    mtl = os.path.splitext(out)[0] + ".mtl"

    print(f"   textures used: {len(textures)}")

    # resolve each texture name to a converted PNG
    png_index = {}
    for p in glob.glob(os.path.join(a.pngdir, "**", "*.png"), recursive=True):
        png_index.setdefault(os.path.splitext(os.path.basename(p))[0].lower(), p)

    # embedded images (lightmaps) are raw CSI blobs -> convert them out
    embdir = os.path.join(outdir, "embedded")
    resolved, emb_n, unresolved = {}, 0, []
    for nm, t in sorted(textures.items()):
        if t["embedded"]:
            s0, s1 = t["embedded"]
            os.makedirs(embdir, exist_ok=True)
            raw = os.path.join(embdir, f"{nm}.csi")
            with open(raw, "wb") as fh:
                fh.write(b[s0:s1])
            try:
                csi2png.convert(raw, embdir)
                resolved[nm] = f"embedded/{nm}.png"
                emb_n += 1
            except Exception as e:
                unresolved.append(f"{nm} (embedded: {e})")
        else:
            hit = png_index.get(nm.lower())
            if hit:
                resolved[nm] = os.path.relpath(hit, outdir).replace("\\", "/")
            else:
                unresolved.append(nm)
    print(f"   textures resolved: {len(resolved)}/{len(textures)} "
          f"({emb_n} embedded converted)")
    if unresolved:
        print(f"   unresolved: {unresolved[:5]}")

    with open(mtl, "w") as f:
        f.write(f"# materials for {os.path.basename(a.src)}\n")
        for i, nm in enumerate(sorted(textures)):
            f.write(f"\nnewmtl {mtl_name(i, nm)}\n")
            f.write("Kd 1.000 1.000 1.000\nd 1.000\nillum 1\n")
            if nm in resolved:
                f.write(f"map_Kd {resolved[nm]}\n")
    mname = {nm: mtl_name(i, nm) for i, nm in enumerate(sorted(textures))}

    bad = 0
    with open(out, "w") as f:
        f.write(f"# exported from {os.path.basename(a.src)} by sb2obj.py\n")
        f.write(f"mtllib {os.path.basename(mtl)}\n")
        vbase = tbase = 1
        for nm, vs, uv, fs in meshes:
            f.write(f"o {nm}\n")
            for v in vs:
                f.write("v %.6f %.6f %.6f\n" % v)
            for t in uv:
                f.write("vt %.6f %.6f\n" % t)          # OBJ V axis matches as-is
            cur = object()
            # group faces by texture so viewers get one draw call per material
            for (vidx, uvidx, tidx, _lmidx, _lmt,
                 _uv2, _t2, _aidx, _at) in sorted(
                    fs, key=lambda r: (r[2] is None, r[2] or "")):
                if max(vidx) >= len(vs):
                    bad += 1
                    continue
                if tidx != cur:
                    cur = tidx
                    if tidx in mname:
                        f.write(f"usemtl {mname[tidx]}\n")
                if uvidx and max(uvidx) < len(uv):
                    f.write("f %d/%d %d/%d %d/%d\n" % (
                        vidx[0] + vbase, uvidx[0] + tbase,
                        vidx[1] + vbase, uvidx[1] + tbase,
                        vidx[2] + vbase, uvidx[2] + tbase))
                else:
                    f.write(f"f {vidx[0]+vbase} {vidx[1]+vbase} {vidx[2]+vbase}\n")
            vbase += len(vs)
            tbase += len(uv)
    print(f"wrote {out} + {os.path.basename(mtl)}  ({bad} out-of-range tris skipped)")


if __name__ == "__main__":
    main()
