#!/usr/bin/env python3
"""
sbchar.py -- read one animated CHARACTER out of a .sb database.

The 59 people, animals and road cars the tracks place live in three databases of
their own -- RCCarsDB/AIChars.sb (Seagull, Crab, Spider, Dog, Vulture),
people.sb (Man, Guard, Woman, RepairMan) and Roadcar.sb (RoadCar, Truck,
Trailer, Btr, Hammer) -- one MOD_MODEL subtree each. This module turns one of
those subtrees into (parts, meshes, clips, textures); pack_chars.py writes them
and gen_char_data.py reads the placements that point at them.

WHAT A CHARACTER IS, and it is two things at once:

  - a node hierarchy of MOD_MESH nodes. Some carry geometry and some are pure
    joints. Every node carries TWO poses and they are different things:
      * 0x540B, translation, SCALE, rotation in degrees (sb2obj.node_trs), is
        the pose the file was SAVED in -- what a model with no clip playing
        stands in, and what a clip's own 0x5410 replaces per part;
      * 0x5438, NINE FLOATS IN THE 0x5410 ORDER (translation, rotation in
        DEGREES, scale), is THE SKIN'S BIND POSE -- the pose the mesh vertices
        were authored in and the one `inv(rest)` has to undo.
    They are not the same pose. Composed, 0x540B puts the Man's hands at
    x +-0.25 with his arms DOWN and 0x5438 puts them at x +-0.62 straight OUT,
    which is the T-pose his mesh is modelled in (1.58 m across against his own
    1.66 m of height). Binding the skin to 0x540B therefore drew every skinned
    model stuck in its bind layout -- the people with their arms out, the
    Seagull with its wings permanently spread, the Dog stretched sideways --
    with only the joints that a clip animates moving inside it.
    0x5438's THIRD triple is not usable as scale: it reads 1.0 on the people's
    ROOT where 0x540B says 0.04 and 25.0 on the Dog's where 0x540B says 1.0, so
    scale is taken from 0x540B and only translation and rotation from 0x5438.
    (It cancels for rendering either way -- a uniform root scale appears once in
    inv(rest[bone]) and once in rest[mesh] -- but taking 0x5438's would ship the
    packed vertices 25x out of metres.)
    The tree composes exactly the way a car's rig does, in both poses.
  - one MOD_ANIMATION per (node, clip). Each carries 0x3408 -- whose FIRST DWORD
    IS THE CLIP INDEX, read rather than assumed, because the Woman has a node
    with six animations against five sequences and position would misfile every
    clip after it -- a 0x5410 rest pose in T,R,S order (NOT the T,S,R that
    0x540B uses), and a MOD_SPLINE per animated component.

A SPLINE'S KEYS ARE CHUNKS, not a packed record array. The 0x865D blob walks as
a run of 0x805B chunks exactly like a .bspl point list, and there are TWO sizes:
18 bytes (u32 type=1, f32 time, f32 value) and 26 (type=0, time, value, and two
tangents). Reading the blob as a fixed 18-byte stride is what made the Crab's
99-key HIT channel look corrupt -- it is 26-byte keys, 2574 = 99 * 26, with
perfectly ordinary values. Walk the chunks and neither size needs knowing.

The tangents are deliberately dropped. Every clip in the shipped data is one key
per frame at the sequence's own 30 Hz, so the curve between two keys is a single
frame long and linear interpolation and a Hermite segment differ by less than the
30 Hz sampling already costs.

SKINNING IS REAL AND IT IS IN 0x062A. The people and the Dog put their whole
body in one or two meshes hung off a node that never moves, with the skeleton
beside it -- so the parts alone would draw a statue. 0x062A is a per-vertex list
of 0x322B chunks, each `u32 n_influences` then 0x342C (n bone NODE IDS) and
0x542D (n weights, in PERCENT). The bone id is the first dword of a node's own
0x3408, which is why parts carry it. Meshes without a 0x062A (the Crab, the
Spider, the Seagull, the road cars, the Guard's gun) are rigidly parented and
draw under one matrix, which is the cheaper path and the one the car rig uses.

Usage:
    import sbchar
    m = sbchar.read_model("<db>.sb", "Dog")
"""
import math
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sb2obj as S

DB = os.environ.get("RCCARS_DB", "/mnt/c/Games/RC Cars/RCCarsDB")

# Which database each model lives in. RoadCar is listed for completeness; no
# track instances it, and its two textures are not in the .csi tree either.
SRC = {
    "Seagull": "AIChars.sb", "Crab": "AIChars.sb", "Spider": "AIChars.sb",
    "Dog": "AIChars.sb", "Vulture": "AIChars.sb",
    "Man": "people.sb", "Guard": "people.sb", "Woman": "people.sb",
    "RepairMan": "people.sb",
    "RoadCar": "Roadcar.sb", "Truck": "Roadcar.sb", "Trailer": "Roadcar.sb",
    "Btr": "Roadcar.sb", "Hammer": "Roadcar.sb",
}

# 0x3057 is a BIG-ENDIAN 4CC and self-documenting. The index is into the nine
# floats of a TRS triple in the 0x5410 order: T, R (degrees), S.
CHAN = {"TRNX": 0, "TRNY": 1, "TRNZ": 2,
        "ROTX": 3, "ROTY": 4, "ROTZ": 5,
        "SCLX": 6, "SCLY": 7, "SCLZ": 8}
CHAN_NAME = {v: k for k, v in CHAN.items()}

# 'TREV' appears twice in the whole shipped set, both on the Seagull, and what
# it does is not recovered. Counted and dropped rather than guessed at.
CHAN_IGNORED = ("TREV",)


def _open(model):
    path = os.path.join(DB, SRC[model])
    b = open(path, "rb").read()
    reg, start = S.read_registry(b)
    return b, reg, start


def _find(b, p, end, name):
    while p < end:
        cid, cend, cnt, body = S.rd_chunk(b, p)
        if cend <= p or cend > end:
            return None
        if cid == 0x9200:
            if S.node_name(b, body, cend) == name:
                return (body, cend)
            r = _find(b, body, cend, name)
            if r:
                return r
        p = cend
    return None


def _u32(b, body, end, cid):
    for c, cnt, cb, ce in S.children(b, body, end):
        if c == cid and ce - cb >= 4:
            return struct.unpack_from("<I", b, cb)[0]
    return None


def _f32(b, body, end, cid):
    for c, cnt, cb, ce in S.children(b, body, end):
        if c == cid and ce - cb >= 4:
            return struct.unpack_from("<f", b, cb)[0]
    return None


def node_id(b, body, end):
    """The node's own id -- first dword of its 0x3408. What 0x062A's bone
    indices name, so it is the only thing that ties a skin to a skeleton."""
    for c, cnt, cb, ce in S.children(b, body, end):
        if c == 0x3408 and ce - cb >= 4:
            return struct.unpack_from("<I", b, cb)[0]
    return None


def bind_trs(b, body, end, trs):
    """0x5438 -> the node's local BIND transform in node_trs's (t, s, r) shape.

    Nine floats in the 0x5410 order -- translation, rotation in DEGREES, scale.
    THE SCALE IS NOT USABLE (see the module header): it reads 1.0 on the
    people's ROOT where 0x540B says 0.04 and 25.0 on the Dog's where 0x540B
    says 1.0, so the node's own 0x540B scale is kept and only translation and
    rotation come from here.

    Every node of every shipped model carries one -- 44/44 on the Seagull,
    71/71 on the Man, 76/76 on the Guard -- so a missing chunk is a parse
    failure rather than a model without a bind pose; it falls back to 0x540B,
    which is what the packer did for everything before this was found.
    """
    for cid, cnt, cb, ce in S.children(b, body, end):
        if cid == 0x5438 and ce - cb >= 36:
            v = struct.unpack_from("<9f", b, cb)
            return v[0:3], trs[1], v[3:6]
    return trs


def read_keys(b, body, end):
    """One 0x865D blob -> [(time, value)], walking it as 0x805B chunks."""
    out = []
    p = body
    while p < end:
        cid, cend, cnt, cb = S.rd_chunk(b, p)
        if cid != 0x805B or cend <= p or cend > end:
            raise ValueError("key blob at %d is not a 0x805B run" % body)
        if cb + 12 <= cend:
            _ty, t, v = struct.unpack_from("<Iff", b, cb)
            out.append((t, v))
        p = cend
    return out


def read_skin(b, body, end):
    """0x062A -> [[(node_id, weight), ...]] per vertex, weights normalised.

    The file stores percentages that sum to 100 to within a rounding error;
    they are renormalised here so the runtime can blend without a divide, and
    a vertex whose weights sum to nothing is given entirely to its first bone
    rather than collapsing to the origin."""
    out = []
    p = body
    while p < end:
        cid, cend, cnt, cb = S.rd_chunk(b, p)
        if cend <= p or cend > end:
            break
        if cid == 0x322B:
            n = struct.unpack_from("<I", b, cb)[0]
            bones, wts = [], []
            for c2, n2, b2, e2 in S.children(b, cb + 4, cend):
                if c2 == 0x342C:
                    bones = list(struct.unpack_from("<%dI" % n, b, b2))
                elif c2 == 0x542D:
                    wts = list(struct.unpack_from("<%df" % n, b, b2))
            if bones and len(wts) == len(bones):
                tot = sum(wts)
                if tot > 1e-6:
                    wts = [w / tot for w in wts]
                else:
                    wts = [1.0] + [0.0] * (len(wts) - 1)
                out.append(list(zip(bones, wts)))
            else:
                out.append([])
        p = cend
    return out


def read_sequences(b, reg, body, end):
    """The MOD_SEQUENCE children of a MOD_MODEL, in file order = clip index."""
    seq_t = next((k for k, v in reg.items() if v == "MOD_SEQUENCE"), None)
    out = []
    for c, cnt, cb, ce in S.children(b, body, end):
        if c == 0x9200 and cnt == seq_t:
            out.append({
                "name": S.node_name(b, cb, ce) or "clip%d" % len(out),
                "rate": _u32(b, cb, ce, 0x3050) or 30,
                "frames": _u32(b, cb, ce, 0x3051) or 0,
                "speed": _f32(b, cb, ce, 0x5052),
            })
    return out


def read_sound(b, reg, body, end):
    """The model's own MOD_SNDCHANNEL, if it has one: (wav, rmin, rmax).
    Same shape gen_prop_data.py reads a prop's knock out of."""
    ch_t = next((k for k, v in reg.items() if v == "MOD_SNDCHANNEL"), None)
    snd_t = next((k for k, v in reg.items() if v == "MOD_SOUND"), None)
    for c, cnt, cb, ce in S.children(b, body, end):
        if c == 0x9200 and cnt == ch_t:
            rmin = _f32(b, cb, ce, 0x50E0) or 0.0
            rmax = _f32(b, cb, ce, 0x50E1) or 0.0
            for c2, n2, b2, e2 in S.children(b, cb, ce):
                if c2 == 0x9200 and n2 == snd_t:
                    return (S.node_name(b, b2, e2), rmin, rmax)
            return (S.node_name(b, cb, ce), rmin, rmax)
    return None


def read_model(model, texture_scope=True):
    """
    -> dict(name, parts, meshes, clips, textures, sound)

      parts[i]  = dict(name, parent, node_id, trs=(t,s,r), bind=(t,s,r),
                       rest=(M,T))
                  trs is 0x540B, the saved pose; bind is 0x5438, the skin's;
                  rest is the composed BIND chain, which is what inv(rest)
                  undoes and what the mesh vertices are baked through.
                  Part 0 is the model's own root node.
      meshes[i] = dict(name, part, verts (MODEL space, BIND pose), uvs,
                       faces [(vidx3, uvidx3, texname)], skin or None)
      clips[i]  = dict(name, rate, frames, duration,
                       chan=[(part, comp, [(t, v), ...])],
                       pose={part: (t, r, s)})   -- the clip's own rest pose
      textures  = {name: {"embedded": (off,end)|None}}
    """
    b, reg, start = _open(model)
    anim_t = next(k for k, v in reg.items() if v == "MOD_ANIMATION")
    spln_t = next(k for k, v in reg.items() if v == "MOD_SPLINE")
    mesh_t = next(k for k, v in reg.items() if v == "MOD_MESH")
    glb_t = next((k for k, v in reg.items() if v == "MOD_GLBTEXTURE"), None)

    sub = _find(b, start, len(b), model)
    if not sub:
        raise SystemExit("%s: no MOD_MODEL named %r" % (SRC[model], model))
    mbody, mend = sub

    seqs = read_sequences(b, reg, mbody, mend)
    parts, meshes = [], []
    by_node_id = {}
    clip_chan = [[] for _ in seqs]
    clip_pose = [{} for _ in seqs]
    ignored_chan = 0

    # Texture scopes accumulate from the file root the way sb2obj.collect does,
    # so a declaration made above the model stays visible.
    seen = {}

    def remember(t):
        old = seen.get(t["name"])
        if old is None or (t["embedded"] and not old["embedded"]):
            seen[t["name"]] = t

    def resolve(ix, scopes):
        for sc in reversed(scopes):
            if ix in sc:
                return sc[ix]
        return None

    def walk(p, end, depth, scopes, pM, pT, parent):
        nonlocal ignored_chan
        while p < end:
            cid, cend, cnt, body = S.rd_chunk(b, p)
            if cend <= p or cend > end:
                return
            if cid == 0x9200 and cnt == mesh_t:
                nm = S.node_name(b, body, cend) or "node%d" % p
                local = S.scope_textures(b, body, cend, glb_t)
                sc = scopes + [local] if local else scopes
                for t in local.values():
                    if t["embedded"]:
                        remember(t)
                trs = S.node_trs(b, body, cend) or ((0, 0, 0), (1, 1, 1), (0, 0, 0))
                bind = bind_trs(b, body, cend, trs)
                M, T = S.compose(pM, pT, bind)
                pi = len(parts)
                nid = node_id(b, body, cend)
                parts.append({"name": nm, "parent": parent, "node_id": nid,
                              "trs": trs, "bind": bind, "rest": (M, T)})
                if nid is not None:
                    by_node_id.setdefault(nid, pi)

                nmm, vs, uv, fs = S.parse_mesh(b, body, cend)
                if vs and fs:
                    named = []
                    for (vidx, uvidx, tidx, lmidx, lmtidx,
                         _uv2, _t2, _aidx, _at) in fs:
                        t = resolve(tidx, sc) if tidx is not None else None
                        if t:
                            remember(t)
                        named.append((vidx, uvidx, t["name"] if t else None))
                    world = [(T[0] + M[0][0] * x + M[0][1] * y + M[0][2] * z,
                              T[1] + M[1][0] * x + M[1][1] * y + M[1][2] * z,
                              T[2] + M[2][0] * x + M[2][1] * y + M[2][2] * z)
                             for (x, y, z) in vs]
                    skin = None
                    for c2, n2, b2, e2 in S.children(b, body, cend):
                        if c2 == 0x062A:
                            skin = read_skin(b, b2, e2)
                    if skin is not None and len(skin) != len(vs):
                        # A skin that does not cover the mesh is a parse error,
                        # not a mesh to draw wrong: fail loudly.
                        raise SystemExit("%s/%s: 0x062A has %d entries for %d "
                                         "vertices" % (model, nm, len(skin),
                                                       len(vs)))
                    meshes.append({"name": nm, "part": pi, "verts": world,
                                   "uvs": uv, "faces": named, "skin": skin})

                # the node's animations
                for c2, n2, b2, e2 in S.children(b, body, cend):
                    if c2 != 0x9200 or n2 != anim_t:
                        continue
                    ci = _u32(b, b2, e2, 0x3408)
                    if ci is None or ci >= len(seqs):
                        continue
                    for c3, n3, b3, e3 in S.children(b, b2, e2):
                        if c3 == 0x5410 and e3 - b3 >= 36:
                            v = struct.unpack_from("<9f", b, b3)
                            clip_pose[ci][pi] = (v[0:3], v[3:6], v[6:9])
                        elif c3 == 0x9200 and n3 == spln_t:
                            comp, keys = None, None
                            for c4, n4, b4, e4 in S.children(b, b3, e3):
                                if c4 == 0x3057 and e4 - b4 >= 4:
                                    fcc = struct.pack(
                                        ">I", struct.unpack_from("<I", b, b4)[0])
                                    comp = CHAN.get(fcc.decode("latin-1"))
                                    if comp is None:
                                        ignored_chan += 1
                                elif c4 == 0x865D:
                                    keys = read_keys(b, b4, e4)
                            if comp is not None and keys:
                                clip_chan[ci].append((pi, comp, keys))
                walk(body, cend, depth + 1, sc, M, T, pi)
            elif cid == 0x9200:
                walk(body, cend, depth + 1, scopes, pM, pT, parent)
            p = cend

    root_scope = S.scope_textures(b, start, len(b), glb_t)
    model_scope = S.scope_textures(b, mbody, mend, glb_t)
    scopes = [x for x in (root_scope, model_scope) if x]
    for sc in scopes:
        for t in sc.values():
            if t["embedded"]:
                remember(t)
    walk(mbody, mend, 0, scopes, S.IDENT3, (0.0, 0.0, 0.0), -1)

    # A skin names bones by NODE ID; convert once, here, so nothing downstream
    # has to carry the id table. A bone the model does not contain is dropped
    # and the vertex renormalised -- it would otherwise index part 0 and drag
    # the whole limb to the root.
    dropped_bones = 0
    for m in meshes:
        if not m["skin"]:
            continue
        conv = []
        for infl in m["skin"]:
            got = []
            for nid, w in infl:
                pi = by_node_id.get(nid)
                if pi is None:
                    dropped_bones += 1
                    continue
                got.append((pi, w))
            tot = sum(w for _, w in got)
            if got and tot > 1e-6:
                got = [(p, w / tot) for p, w in got]
            else:
                got = [(m["part"], 1.0)]
            conv.append(got)
        m["skin"] = conv

    clips = []
    for i, s in enumerate(seqs):
        rate = s["rate"] or 30
        dur = s["frames"] / float(rate) if s["frames"] else 0.0
        if not dur and clip_chan[i]:
            dur = max(k[-1][0] for _, _, k in clip_chan[i])
        clips.append({"name": s["name"], "rate": rate, "frames": s["frames"],
                      "speed": s["speed"], "duration": dur,
                      "chan": clip_chan[i], "pose": clip_pose[i]})

    # The model's own MOD_GLBTEXTURE declarations, IN FILE ORDER. That order is
    # what the `clother` script key indexes: the Man declares man_1new, man_2new,
    # man1_1, man2_2 and the Dog declares dog_1, dog, and those two models are
    # exactly the two whose instances carry a `clother` -- so clother k selects
    # the k-th block of however many textures the faces actually reference.
    glb = []
    for c, cnt, cb, ce in S.children(b, mbody, mend):
        if c == 0x9200 and cnt == glb_t:
            nm = S.node_name(b, cb, ce)
            if nm and nm not in glb:
                glb.append(nm)

    return {"name": model, "parts": parts, "meshes": meshes, "clips": clips,
            "textures": seen, "glb_order": glb,
            "sound": read_sound(b, reg, mbody, mend),
            "ignored_chan": ignored_chan, "dropped_bones": dropped_bones}


# ------------------------------------------------------- which way it faces ---

def model_forward(model):
    """
    (fx, fz, witness) -- the model's own forward direction in model space.

    NOT read off the bounding box, and that is the whole point: the four skinned
    models are authored in a spread bind pose, so the Dog's mesh is 1.06 m along
    X and 0.25 m along Z and reads as though the dog faced sideways. Its
    SKELETON says otherwise -- over the run cycle the paws swing 0.519 m along Z
    against 0.067 m along X -- and the skeleton is what the animation moves.

    Two rules, each with its own witness, and every shipped model satisfies one:

      - a model with an ANIMATED SKELETON faces +Z. Checked, not assumed: the
        foot end effectors sit 0.039-0.052 m along +Z of their own ankles on all
        six models that have them, and every HEAD node sits at positive Z (the
        Dog's at +0.594, the Seagull's at +0.100). Two independent witnesses,
        agreeing, and it is also the convention rbcar_init and gen_tracks.py use
        for the car.
      - a model with NO clips is a vehicle, and its headlamps say which end is
        the front: the `fara` meshes (Russian for headlight) sit at the extreme
        -X of the Truck, the Trailer and the Hammer, with the mirrors and the
        windscreen just inboard of them. So those face -X, and the axis is taken
        from where the lamps are rather than from the length of the box.

    A model that satisfies neither -- the Vulture, which has no clips and no
    lamps -- keeps +Z and says so, since nothing in its data distinguishes the
    two ends of a bird with its wings along X.
    """
    if model["clips"]:
        # the witness, so a model that stops satisfying it is caught
        best = None
        for i, p in enumerate(model["parts"]):
            nm = p["name"].upper()
            if nm.endswith("EFF") and "FOOT" in nm and p["parent"] >= 0:
                dz = p["rest"][1][2] - model["parts"][p["parent"]]["rest"][1][2]
                if best is None or abs(dz) > abs(best):
                    best = dz
        if best is None:
            for p in model["parts"]:
                if "HEAD" in p["name"].upper():
                    best = p["rest"][1][2]
                    break
        if best is not None and best < 0.0:
            return 0.0, -1.0, "toes/head at -Z"
        return 0.0, 1.0, "toes/head at +Z" if best is not None else "no witness"

    lamps = [m for m in model["meshes"]
             if any((t or "").lower().startswith("fara")
                    or "fara" in (m["name"] or "").lower()
                    for _v, _u, t in m["faces"]) or "fara" in m["name"].lower()]
    if lamps:
        allv = [v for m in model["meshes"] for v in m["verts"]]
        cx = sum(v[0] for v in allv) / len(allv)
        cz = sum(v[2] for v in allv) / len(allv)
        lv = [v for m in lamps for v in m["verts"]]
        dx = sum(v[0] for v in lv) / len(lv) - cx
        dz = sum(v[2] for v in lv) / len(lv) - cz
        if abs(dx) >= abs(dz):
            return (1.0 if dx > 0 else -1.0), 0.0, "headlamps at %sX" % (
                "+" if dx > 0 else "-")
        return 0.0, (1.0 if dz > 0 else -1.0), "headlamps at %sZ" % (
            "+" if dz > 0 else "-")
    return 0.0, 1.0, "no witness -- default +Z"


def yaw_offset(model):
    """Degrees to pre-rotate the model by so its forward becomes +Z."""
    fx, fz, _w = model_forward(model)
    return -math.degrees(math.atan2(fx, fz))


# ------------------------------------------------------------------ helpers ---

def mat_from_trs(t, r, s):
    """local(v) = t + R*S*v with R = Rz*Ry*Rx, exactly sb2obj.compose's rule.
    Returned as a 3x3 and a translation, the same pair compose() takes."""
    R = S._rot3(r[0], r[1], r[2])
    M = tuple(tuple(R[i][j] * s[j] for j in range(3)) for i in range(3))
    return M, tuple(t)


def clip_local(model, clip, part_index, time):
    """The (t, r, s) a part holds at `time` in `clip` -- the clip's own 0x5410
    rest pose with every channel that has keys overriding its component.

    Only used by the harness side; the runtime does the same thing in C."""
    p = model["parts"][part_index]
    base = clip["pose"].get(part_index)
    if base is None:
        t, s, r = p["trs"]
        v = list(t) + list(r) + list(s)
    else:
        v = list(base[0]) + list(base[1]) + list(base[2])
    for pi, comp, keys in clip["chan"]:
        if pi != part_index:
            continue
        v[comp] = sample(keys, time)
    return tuple(v[0:3]), tuple(v[3:6]), tuple(v[6:9])


def draw_matrices(model, clip=None, t=0.0):
    """The per-part `world * inv(rest)` a BIND-POSE vertex goes through -- the
    same product char.c's chr_pose builds, in the same order, so the harness
    side and the runtime cannot drift.

    With clip None every part holds its own 0x540B saved pose, which is what a
    model with no clip playing stands in.
    """
    world, draw = [], []
    for i, p in enumerate(model["parts"]):
        if clip is None:
            t3, s3, r3 = p["trs"]
        else:
            t3, r3, s3 = clip_local(model, clip, i, t)
        par = p["parent"]
        if 0 <= par < i:
            M, T = S.compose(world[par][0], world[par][1], (t3, s3, r3))
        else:
            M, T = mat_from_trs(t3, r3, s3)
        world.append((M, T))
        draw.append(_mul_rigid(M, T, *_invert(*p["rest"])))
    return draw


def _mul_rigid(aM, aT, bM, bT):
    M = tuple(tuple(sum(aM[i][k] * bM[k][j] for k in range(3)) for j in range(3))
              for i in range(3))
    T = tuple(aT[i] + sum(aM[i][k] * bT[k] for k in range(3)) for i in range(3))
    return (M, T)


def _invert(M, T):
    a, b, c = M[0]
    d, e, f = M[1]
    g, h, i = M[2]
    det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g)
    iM = (((e * i - f * h) / det, -(b * i - c * h) / det, (b * f - c * e) / det),
          (-(d * i - f * g) / det, (a * i - c * g) / det, -(a * f - c * d) / det),
          ((d * h - e * g) / det, -(a * h - b * g) / det, (a * e - b * d) / det))
    iT = tuple(-sum(iM[r][k] * T[k] for k in range(3)) for r in range(3))
    return iM, iT


def posed_verts(model, clip=None, t=0.0):
    """Every mesh's vertices as DRAWN -- skinned through draw_matrices for the
    meshes that carry a 0x062A, rigid through their own part for the rest.

    The bind pose is a T-pose (the people) or a splayed layout (the Dog), so the
    packed vertices' own bounding box is not the shape of the character and must
    not be used as one -- which is what this is for.
    """
    draw = draw_matrices(model, clip, t)
    out = []
    for mesh in model["meshes"]:
        if mesh["skin"]:
            for vi, v in enumerate(mesh["verts"]):
                acc = [0.0, 0.0, 0.0]
                for bp, w in mesh["skin"][vi]:
                    M, T = draw[bp]
                    for k in range(3):
                        acc[k] += (T[k] + sum(M[k][j] * v[j]
                                              for j in range(3))) * w
                out.append(tuple(acc))
        else:
            M, T = draw[mesh["part"]]
            for v in mesh["verts"]:
                out.append(tuple(T[k] + sum(M[k][j] * v[j] for j in range(3))
                                 for k in range(3)))
    return out


def sample(keys, t):
    """Linear between keys, clamped at both ends."""
    if not keys:
        return 0.0
    if t <= keys[0][0]:
        return keys[0][1]
    if t >= keys[-1][0]:
        return keys[-1][1]
    lo, hi = 0, len(keys) - 1
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if keys[mid][0] <= t:
            lo = mid
        else:
            hi = mid
    t0, v0 = keys[lo]
    t1, v1 = keys[hi]
    if t1 - t0 < 1e-9:
        return v1
    u = (t - t0) / (t1 - t0)
    return v0 + (v1 - v0) * u


def main():
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("model", nargs="?", default=None)
    a = ap.parse_args()
    names = [a.model] if a.model else [m for m in SRC if m != "RoadCar"]
    for n in names:
        m = read_model(n)
        skinned = sum(1 for x in m["meshes"] if x["skin"])
        keys = sum(len(k) for c in m["clips"] for _, _, k in c["chan"])
        tris = sum(len(x["faces"]) for x in m["meshes"])
        print("%-10s parts=%-4d meshes=%-3d (skinned %d) tris=%-5d clips=%-2d "
              "keys=%-7d tex=%s%s"
              % (n, len(m["parts"]), len(m["meshes"]), skinned, tris,
                 len(m["clips"]), keys, sorted(m["textures"]),
                 "" if not m["dropped_bones"] else
                 "  DROPPED BONES %d" % m["dropped_bones"]))
        for c in m["clips"]:
            print("      %-20s %5.2fs %3d frames  %3d channels"
                  % (c["name"], c["duration"], c["frames"], len(c["chan"])))


if __name__ == "__main__":
    main()
