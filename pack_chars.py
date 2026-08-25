#!/usr/bin/env python3
"""
pack_chars.py -- one track's people, animals and road cars into <track>.chr.

The 59 characters are placed by MOD_INSTANCE nodes in each track's own .sb and
their geometry lives in three shared databases, so nothing about them reaches
the packed track. This writes the models a given track actually needs, plus the
recorded paths that track's instances name.

WHY PER TRACK AND NOT ONE SHARED SCENE. props.vsc packs all 13 props once
because the app loads it once and every track uses some of them for 2.7 MB. The
characters are ten times that: the Man alone is 112,560 animation keys and the
people 1,500-1,900 triangles each. A track needs between two and eight models,
so packing per track keeps ONE track's characters resident instead of thirteen
models' worth -- worst case beach_3 at 8,010 triangles and 2.2 MB of keys
against a shared file that would carry every model on every track.

TWO POSES PER PART, and CHR2 exists because the second one was missing. `trs`
is the node's own 0x540B, the pose it was SAVED in and the one a part with no
clip channel holds; `bind` is its 0x5438, THE SKIN'S BIND POSE, and `rest_inv`
is the inverse of the composed BIND chain. They are different poses -- the
people's mesh is a T-pose their saved pose is not -- and binding the skin to the
saved pose is what drew every skinned model stuck in its bind layout. See
sbchar.py's header. `bind` is shipped rather than dropped because it is the only
thing a harness can rebuild `rest_inv` from and check it against: posed from
`bind`, every part's draw matrix must come out the identity, which is the check
that ties the two halves of a part record together. Posed from `trs` it must
NOT, and the old CHR1 harness asserting that it did is what let this ship.

bmin/bmax are measured on the model POSED IN ITS SAVED POSE, not on the packed
(bind-pose) vertices, because the bind pose is not the shape of the character:
the Man's packed box is 1.57 m across (arms out) against 0.55 m posed, and the
Dog's is 1.06 m across and 0.25 m deep against 0.42 x 1.06 posed. The box feeds
the draw cull and char.c's stride anchor, and both want the animal.

CHR2 is its own format rather than another VSC version, for two reasons that are
both about scene.c: a character rig is up to 89 parts against CARANI_MAX_PARTS's
56, and four of the thirteen models are SKINNED, which needs a per-vertex bone
list and a blend that scene_draw has no place for. Textures are packed in the
VSC encoding exactly, and read back through scene.c's own uploader, so quality,
the 565 byte order and the mip rule cannot drift apart between the two formats.

    "CHR2"
    u32   n_tex, n_models, n_paths
    tex   [u16 len, name, u16 w, u16 h, u8 fmt, u8 mips, levels...]  as VSC
    model [u16 len, name,
           u32 n_parts, n_batches, n_clips, n_slot, n_var,
           f32 bmin[3], bmax[3], f32 yaw_off,
           parts   [u16 len, name, i32 parent, f32 trs[9], f32 bind[9],
                    f32 rest_inv[16]]
           var     [n_var][n_slot] u32 texture index
           batches [u32 slot, part, nverts, nidx, skinned,
                    verts f32 x,y,z,u,v,
                    skin  (u8 n, n * (u16 part, f32 w)) per vertex,
                    idx   u16]
           clips   [u16 len, name, f32 duration, u32 frames,
                    u32 n_pose, [u16 part, f32 trs[9]],
                    u32 n_chan, [u16 part, u16 comp, u32 nkeys, [f32 t, f32 v]]]]
    path  [u16 len, name, u32 kind, u32 n, f32 duration,
           n * (f32 t, f32 pos[3], f32 fwd[3], f32 up[3], f32 state)]

Usage:
    python3 pack_chars.py beach_1 /mnt/c/tools/rccars_vita/assets/beach_1.chr
"""
import argparse
import math
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sb2obj as S
import pack_vsc as P
import sbchar
import gen_char_data as G

MAX_VERTS = 65000
PROFILES = "/mnt/c/tools/rccars_extracted/Profiles"

# Path decimation. The recorded people are sampled at 70-150 Hz by a recorder
# that wrote a frame whenever the game drew one, which is far finer than a 0.59
# m/s walk needs: a kept sample every 1/20 s is 3 cm of travel, against a person
# the size of a shoebox in a world where the car is 0.42 m long. A sample is
# kept anyway when the heading turns or the recorded state changes, so nothing
# that a consumer can see is smoothed away.
PATH_MIN_DT = 1.0 / 20.0
PATH_MAX_TURN_COS = math.cos(2.0 * math.pi / 180.0)


# ------------------------------------------------------------------- paths ---

def read_person_path(path):
    """Profiles/<track>_<who>.dat -- the recorded walk.

    u32 n, f32 duration, u32 n again, u16 32 (the record size), u16 unknown;
    then n records of eight floats: time, position, a unit heading whose y is
    always exactly 0, and an integer-valued state.

    THE STATE COLUMN IS NOT DECODED. It takes the values 0,1,2,5,6,7,8 across
    the eight shipped files, which is more than any people model has sequences
    (the Man has six), so it is not a clip index -- it is the recorder's own
    behaviour state. It is carried through so a consumer that learns what it
    means does not need a repack; char.c picks its clip from the recorded SPEED
    instead, which is a property of the path rather than of the recorder."""
    d = open(path, "rb").read()
    n, dur, n2 = struct.unpack_from("<IfI", d, 0)
    rec, _unk = struct.unpack_from("<HH", d, 12)
    if rec != 32 or n != n2 or 16 + n * 32 != len(d):
        raise SystemExit("%s: not a person profile (n=%d n2=%d rec=%d size=%d)"
                         % (path, n, n2, rec, len(d)))
    out = []
    for i in range(n):
        t, x, y, z, fx, fy, fz, st = struct.unpack_from("<8f", d, 16 + i * 32)
        out.append((t, (x, y, z), (fx, fy, fz), (0.0, 1.0, 0.0), st))
    return out, dur


def read_gpf(path):
    """Profiles/*.gpf -- the road cars' and the vultures' recorded drive.

    u8[2] version, char[16] owner name ("ROADCAR" / "ZEROCAM"), 6 zero bytes,
    u32 n, u32 m (floats per record), u32 n*m; then ONE float that is the total
    duration, then n records of m floats, then n pairs of (f32 time, u32 float
    offset = i*m).

    THE LEADING DURATION FLOAT IS THE WHOLE TRICK. Reading the records straight
    off the header shifts every one of them by a float and makes field 0 look
    like a per-record scalar with a wild first entry; with it consumed, m=9
    records read as position, a unit forward, and a unit UP that is exactly
    perpendicular to the forward in all 9,849 vulture samples, and the file
    length closes to the byte on both record counts.

    m = 8 (road cars) has no up vector: fields 6 and 7 are a pair whose first is
    clamped to +-0.2 in every shipped file and whose meaning is not recovered.
    Those cars are on the ground, so char.c builds their frame from the forward
    and world up.

    Returns (rows, duration, owner). THE OWNER NAME IS LOAD-BEARING -- it is what
    says whose frame the recording is in, and the two owners want opposite
    treatment: see roadcar_centre_y below for ROADCAR and char.c's
    CHR_VULTURE_SHIFTY for ZEROCAM."""
    d = open(path, "rb").read()
    n, m, tot = struct.unpack_from("<3I", d, 0x18)
    if tot != n * m or len(d) != 40 + n * m * 4 + n * 8:
        raise SystemExit("%s: not a .gpf (n=%d m=%d tot=%d size=%d)"
                         % (path, n, m, tot, len(d)))
    dur = struct.unpack_from("<f", d, 36)[0]
    base = 40
    tail = base + n * m * 4
    out = []
    for i in range(n):
        r = struct.unpack_from("<%df" % m, d, base + i * m * 4)
        t, off = struct.unpack_from("<fI", d, tail + i * 8)
        if off != i * m:
            raise SystemExit("%s: time index %d names float %d, not %d"
                             % (path, i, off, i * m))
        up = tuple(r[6:9]) if m >= 9 else (0.0, 1.0, 0.0)
        out.append((t, tuple(r[0:3]), tuple(r[3:6]), up, 0.0))
    owner = d[2:18].split(b"\0")[0].decode("latin-1")
    return out, dur, owner


def decimate(rows):
    """Drop samples the eye cannot separate -- see PATH_MIN_DT."""
    if len(rows) < 3:
        return rows
    out = [rows[0]]
    for r in rows[1:-1]:
        p = out[-1]
        if r[0] - p[0] >= PATH_MIN_DT:
            out.append(r)
            continue
        if r[4] != p[4]:
            out.append(r)
            continue
        dot = sum(r[2][k] * p[2][k] for k in range(3))
        if dot < PATH_MAX_TURN_COS:
            out.append(r)
    out.append(rows[-1])
    return out


_ROADCAR_CENTRE = None


def roadcar_centre_y():
    """How far above its own origin the ROADCAR model's mid-height is.

    A ROADCAR .gpf RECORDS THE VEHICLE'S CENTRE, NOT ITS ORIGIN, and every road
    car in the port floated by exactly that. Measured against the shipped
    collision grids, every sample of all four road-car recordings sits 0.70 m
    above the surface under it -- rdc_beach_2_forw/back and rdc_urban_1_forw at a
    flat 0.70 with a 0.64 minimum, rdc_beach_3_* at 0.72, rdc_beach_4_forw at
    0.69 where it has a grid under it at all -- while every one of the eight
    people's `.dat` recordings sits at 0.00 (`chrfloat` part 1 prints the table).
    A constant on four different terrains is not terrain.

    What it is: the RoadCar model's own posed box is y -0.002 .. 1.434, so its
    mid-height is 0.716 above its origin, which is at its wheels. That is the
    0.70, to within the sampling -- and the RoadCar is exactly the model no track
    instances and whose name the recordings carry in their own header. So the
    recorded y is the recorder's centre, and it is subtracted here, once, from
    the recording rather than per model at draw: all six of beach_2's vehicles
    share these two paths, and taking each model's own half-height instead would
    sink the 2.18 m Trailer 0.37 m into the road the 1.44 m Truck drives on.

    Measured off the model rather than typed so that a repack cannot disagree
    with it. ZEROCAM recordings -- the vultures' -- are untouched: they are a
    camera path and their offset is vulture.ini's own recovered ShiftY.
    """
    global _ROADCAR_CENTRE
    if _ROADCAR_CENTRE is None:
        vs = sbchar.posed_verts(sbchar.read_model("RoadCar"))
        lo = min(v[1] for v in vs)
        hi = max(v[1] for v in vs)
        _ROADCAR_CENTRE = (lo + hi) * 0.5
    return _ROADCAR_CENTRE


def load_path(name):
    """Resolve one script `name` to its file. People name a .dat outright; road
    cars name a key like `rdc_beach2` that no file matches, so the pair the
    track actually ships (`rdc_<track>_forw` / `_back`) is resolved by the
    caller and passed in already spelled."""
    dat = os.path.join(PROFILES, name + ".dat")
    gpf = os.path.join(PROFILES, name + ".gpf")
    if os.path.exists(dat):
        rows, dur = read_person_path(dat)
        return 0, decimate(rows), dur
    if os.path.exists(gpf):
        rows, dur, owner = read_gpf(gpf)
        if owner.upper().startswith("ROADCAR"):
            dy = roadcar_centre_y()
            rows = [(t, (p[0], p[1] - dy, p[2]), f, u, st)
                    for t, p, f, u, st in rows]
            # Printed because it is the one thing about a path that this changes,
            # and because the OWNER is what gates it: a build log naming a vlt_*
            # recording on this line is the gate gone. chartest part 13's own
            # ZEROCAM check cannot see it -- 0.7 m off a bird 22 m up is not
            # observable, so that mutant is equivalent in the game and visible
            # only here.
            print("   %s: -%.3f m, %s's own recorded centre" % (name, dy, owner))
        return 1, decimate(rows), dur
    return None


# ------------------------------------------------------------------ models ---

def mat4_from(M, T):
    """A (3x3, translation) pair as a COLUMN-MAJOR 4x4, the layout
    glMultMatrixf wants and the one rb.c and carani.c already use."""
    return [M[0][0], M[1][0], M[2][0], 0.0,
            M[0][1], M[1][1], M[2][1], 0.0,
            M[0][2], M[1][2], M[2][2], 0.0,
            T[0],    T[1],    T[2],    1.0]


def invert_rigid_scaled(M, T):
    """Inverse of v -> T + M v for a general 3x3 M. The parts carry scale as
    well as rotation (the Crab's legs are scaled), so this cannot take the
    transpose shortcut."""
    a, b, c = M[0]
    d, e, f = M[1]
    g, h, i = M[2]
    det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g)
    if abs(det) < 1e-20:
        det = 1e-20
    inv = ((( e * i - f * h) / det, -(b * i - c * h) / det,  (b * f - c * e) / det),
           (-(d * i - f * g) / det,  (a * i - c * g) / det, -(a * f - c * d) / det),
           (( d * h - e * g) / det, -(a * h - b * g) / det,  (a * e - b * d) / det))
    it = tuple(-sum(inv[r][k] * T[k] for k in range(3)) for r in range(3))
    return inv, it


def variant_sets(model):
    """Texture sets an instance can wear, set 0 being what the faces reference.

    Two rules, each read off the data it applies to and neither guessed:

      - the Man declares man_1new, man_2new, man1_1, man2_2 and the Dog declares
        dog_1, dog, and those two models are EXACTLY the two whose instances
        carry a `clother` key. So the declarations come in blocks the size of
        the used set, and clother indexes the block.
      - the Truck declares trailer_txt, trailer_txt_01, trailer_txt_02 and the
        Trailer treiler_box_01, treiler_box_02, and those instances carry `body`
        and `box`. So a numeric suffix is the variant, and one that has none
        gains one.

    A slot with no alternate keeps set 0's texture at every index, which is what
    the Truck's treiler_fara headlamp does."""
    used = []
    for m in model["meshes"]:
        for _v, _u, t in m["faces"]:
            if t and t not in used:
                used.append(t)
    decl = model["glb_order"]
    sets = [list(used)]

    # block rule
    blocks = [d for d in decl if d not in used]
    k = len(used)
    if k and len(blocks) >= k and len(blocks) % k == 0:
        for j in range(0, len(blocks), k):
            sets.append(blocks[j:j + k])
            if len(sets) > 4:
                break

    # suffix rule, only where it produces names the model actually declares
    def bump(name, v):
        if v == 0:
            return name
        if len(name) > 3 and name[-3] == "_" and name[-2:].isdigit():
            return "%s_%02d" % (name[:-3], int(name[-2:]) + v)
        return "%s_%02d" % (name, v)

    for v in range(1, 4):
        alt = [bump(t, v) for t in used]
        if any(a != t and a in decl for a, t in zip(alt, used)):
            sets.append([a if a in decl else t for a, t in zip(alt, used)])
    return used, sets


def drop_collision_boxes(m):
    """Remove the road cars' authored COLLISION BOXES from what gets drawn.

    Reported four times as "the npc cars are white boxes" and once as black
    ones, and it is the geometry, not the draw path: each of the four road-car
    models carries FIVE box meshes that the retail engine does not render, and
    the port rendered all five on top of the vehicle.

    What they are, measured (posed, against the mesh each one sits beside):

      Hammer  Box00        5.27 x 1.42 x 2.37   body hammer_01_body 5.02 x 1.50 x 2.37
              Box01..04_body 0.97 cubes         one on each wheel obj17*, same centre
      Truck   cube1        4.49 x 1.03 x 1.90   body polySurface4_body 4.35 x 1.04 x 2.00
              cube2..5_body 0.59 x 0.59 x 0.22  one on each wheel obj28..31, same centre
      Trailer cube1        5.50 x 1.77 x 2.04   the whole tractor plus camper
              cube2..5_body                     one on each wheel obj24..27, same centre
      Btr     cube1_body   2.87 x 2.04 x 6.89   body Line38_body 3.00 x 1.58 x 6.68
              cube2*       4 boxes              one per PAIR of the eight road wheels

    THE DATA SAYS SO TWICE, so neither half is guessed:

      - every one of them is 8 vertices and 12 triangles with a UV set of ONE
        DISTINCT COORDINATE for the whole mesh -- (0,0) on the Truck and the
        Trailer's cube1, (0, 0.998) on the Hammer, (0,1) on the Trailer's wheel
        boxes. A mesh with one UV has no mapping; drawn, every face samples a
        single texel, which is exactly the flat white and flat black boxes that
        were reported. hummer_txt's is (0,0,0), so the Hammer was five black
        boxes; the Truck's five have no texture at all and inherited slot 0,
        treiler_fara, whose (0,0) is (176,201,211) -- the white ones;
      - the Truck's five and the Trailer's cube1 carry texture id 0xFFFFFFFF
        outright, the same "no texture" the .vsc packer already refuses to draw
        (BATCH_NO_TEXTURE). build_model used to send those faces to slot 0.

    ACROSS THE WHOLE CAST the predicate names 20 meshes and every one of them is
    one of these: 5 in each of the four road-car models, 0 in the nine people
    and animals, 0 in RoadCar. No drawn mesh anywhere has a one-point UV set --
    which is what makes this a rule and not a name list. Asserted below, so a
    repack that finds a sixth box or one in a person fails instead of shipping.

    Called before variant_sets and posed_verts so the slot table and bmin/bmax
    are measured on the same geometry that is drawn. The Hammer's box was the
    first mesh naming hummer_txt, so its slot ORDER changes with this; the sets
    are regenerated in the same pass and there is one of them.

    NOT applied to RoadCar: it is a single mesh with no box in it, so the 0.70 m
    ROADCAR path centre roadcar_centre_y() measures is untouched.

    These five boxes are also the only per-vehicle collision volumes the data
    carries, and RDCR's provider reads up to eight -- see dynamic-layer.md,
    "the road cars' collision proxy", which still derives its spheres from the
    model box rather than from these.
    """
    keep, boxes = [], []
    for mesh in m["meshes"]:
        one_uv = len(set(mesh["uvs"])) <= 1
        is_box = len(mesh["verts"]) == 8 and len(mesh["faces"]) == 12
        if one_uv and is_box:
            boxes.append(mesh["name"])
        else:
            if one_uv:
                raise SystemExit("%s/%s: one UV for the whole mesh but %d verts "
                                 "/ %d tris -- not a box, and drawing it would "
                                 "sample one texel" % (m["name"], mesh["name"],
                                                       len(mesh["verts"]),
                                                       len(mesh["faces"])))
            keep.append(mesh)
    if boxes and len(boxes) != 5:
        raise SystemExit("%s: %d collision boxes, expected 5 (one body, four "
                         "wheel groups): %s" % (m["name"], len(boxes), boxes))
    if boxes and m["name"] not in ("Hammer", "Truck", "Trailer", "Btr"):
        raise SystemExit("%s: a collision box in a model that is not a road "
                         "car: %s" % (m["name"], boxes))
    m["meshes"] = keep
    return boxes


def build_model(name, csi, texroot, alpha):
    m = sbchar.read_model(name)
    boxes = drop_collision_boxes(m)
    if boxes:
        # Printed because it is the one thing about a road car this
        # removes, and a build log with the line missing is the drop gone.
        print("   %s: %d collision boxes not drawn (%s)"
              % (name, len(boxes), ", ".join(boxes)))
    slots, sets = variant_sets(m)
    slot_of = {t: i for i, t in enumerate(slots)}

    # Batches: grouped by (texture slot, part) so a rigid batch draws under one
    # matrix. A SKINNED mesh is its own batch whatever its part, because the
    # part is only where its bind pose hangs and every vertex moves separately.
    groups = {}
    dropped = 0
    for mi, mesh in enumerate(m["meshes"]):
        vs, uv, faces = mesh["verts"], mesh["uvs"], mesh["faces"]
        skin = mesh["skin"]
        for vidx, uvidx, tname in faces:
            if max(vidx) >= len(vs):
                dropped += 1
                continue
            slot = slot_of.get(tname, 0 if slots else 0)
            key = (slot, mesh["part"], mi if skin else -1)
            g = groups.setdefault(key, [{"vmap": {}, "verts": [], "idx": [],
                                         "skin": [], "part": mesh["part"],
                                         "slot": slot,
                                         "skinned": skin is not None}])
            cur = g[-1]
            if len(cur["verts"]) + 3 > MAX_VERTS:
                cur = {"vmap": {}, "verts": [], "idx": [], "skin": [],
                       "part": mesh["part"], "slot": slot,
                       "skinned": skin is not None}
                g.append(cur)
            for k in range(3):
                vi = vidx[k]
                ti = uvidx[k] if (uvidx and uvidx[k] < len(uv)) else None
                vkey = (mi, vi, ti)
                s = cur["vmap"].get(vkey)
                if s is None:
                    s = len(cur["verts"])
                    cur["vmap"][vkey] = s
                    px, py, pz = vs[vi]
                    tu, tv = uv[ti] if ti is not None else (0.0, 0.0)
                    cur["verts"].append((px, py, pz, tu, tv))
                    cur["skin"].append(skin[vi] if skin else None)
                cur["idx"].append(s)

    batches = []
    for key in sorted(groups):
        for g in groups[key]:
            if g["idx"]:
                batches.append(g)

    # NOT the packed vertices' box -- those are the BIND pose, which is a T-pose
    # for the people and a splayed layout for the Dog. The box is measured on the
    # model posed in its own saved 0x540B pose, which is the animal. See header.
    allv = sbchar.posed_verts(m) or [v[:3] for g in batches for v in g["verts"]]
    if allv:
        bmin = [min(v[i] for v in allv) for i in range(3)]
        bmax = [max(v[i] for v in allv) for i in range(3)]
    else:
        bmin = bmax = [0.0, 0.0, 0.0]

    return {"m": m, "slots": slots, "sets": sets, "batches": batches,
            "bmin": bmin, "bmax": bmax, "dropped": dropped,
            "yaw_off": sbchar.yaw_offset(m),
            "fwd_witness": sbchar.model_forward(m)[2]}


def collapse(keys):
    """A channel whose value never moves is one key. Better than half of every
    clip is exactly that -- a joint that is animated in one clip still carries a
    full-length flat channel in the others -- and the runtime's sampler already
    handles a single key as a constant."""
    if len(keys) <= 1:
        return keys
    v0 = keys[0][1]
    for _t, v in keys:
        if abs(v - v0) > 1e-6:
            return keys
    return [keys[0]]


# ------------------------------------------------------------------- write ---

def wstr(f, s):
    b = s.encode("latin-1")
    f.write(struct.pack("<H", len(b)))
    f.write(b)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("track")
    ap.add_argument("out")
    ap.add_argument("--texroot", default="/mnt/c/tools/rccars_extracted")
    ap.add_argument("--texset", default="1")
    ap.add_argument("--alpha", action="store_true", default=True)
    a = ap.parse_args()

    placements = G.read_placements(a.track)
    if not placements:
        print("%s: no characters" % a.track)
        return

    want = []
    for p in placements:
        if p["model"] not in want:
            want.append(p["model"])
    csi = P.index_csi(a.texroot, prefer=a.texset)

    built = [build_model(n, csi, a.texroot, a.alpha) for n in want]

    # texture table: every name any variant set of any model names
    names = []
    for b in built:
        for s in b["sets"]:
            for t in s:
                if t not in names:
                    names.append(t)
    tex_blobs, tex_index, missing = [], {}, []
    for n in names:
        blob = None
        for b in built:
            rec = b["m"]["textures"].get(n)
            if rec and rec.get("embedded"):
                blob = rec["embedded"]
                src = b
                break
        p = csi.get(n.lower())
        try:
            if blob:
                raw = open(sbchar.os.path.join(
                    sbchar.DB, sbchar.SRC[src["m"]["name"]]), "rb").read()
                w, h, fmt, levels = P.load_texture_data(raw[blob[0]:blob[1]],
                                                        a.alpha)
            elif p:
                w, h, fmt, levels = P.load_texture(p, a.alpha)
            else:
                missing.append(n)
                continue
        except Exception:
            missing.append(n)
            continue
        if not levels:
            missing.append(n)
            continue
        tex_index[n] = len(tex_blobs)
        tex_blobs.append((n, w, h, fmt, levels))

    # paths this track's instances name
    paths, path_index = [], {}
    for p in placements:
        for nm in p["paths"]:
            if nm in path_index:
                continue
            got = load_path(nm)
            if got is None:
                print("   path %r not found" % nm)
                continue
            kind, rows, dur = got
            path_index[nm] = len(paths)
            paths.append((nm, kind, rows, dur))

    with open(a.out, "wb") as f:
        f.write(b"CHR2")
        f.write(struct.pack("<III", len(tex_blobs), len(built), len(paths)))
        for n, w, h, fmt, levels in tex_blobs:
            wstr(f, n)
            f.write(struct.pack("<HHBB", w, h, fmt, len(levels)))
            for px in levels:
                f.write(px)

        for b in built:
            m = b["m"]
            wstr(f, m["name"])
            f.write(struct.pack("<IIIII", len(m["parts"]), len(b["batches"]),
                                len(m["clips"]), len(b["slots"]),
                                len(b["sets"])))
            f.write(struct.pack("<6f", *(b["bmin"] + b["bmax"])))
            f.write(struct.pack("<f", b["yaw_off"]))
            for pt in m["parts"]:
                wstr(f, pt["name"])
                f.write(struct.pack("<i", pt["parent"]))
                t, s, r = pt["trs"]
                f.write(struct.pack("<9f", *(list(t) + list(s) + list(r))))
                t, s, r = pt["bind"]
                f.write(struct.pack("<9f", *(list(t) + list(s) + list(r))))
                M, T = pt["rest"]
                iM, iT = invert_rigid_scaled(M, T)
                f.write(struct.pack("<16f", *mat4_from(iM, iT)))
            for st in b["sets"]:
                for t in st:
                    f.write(struct.pack("<I", tex_index.get(t, 0xFFFFFFFF)))
            for g in b["batches"]:
                f.write(struct.pack("<IIIII", g["slot"], g["part"],
                                    len(g["verts"]), len(g["idx"]),
                                    1 if g["skinned"] else 0))
                for v in g["verts"]:
                    f.write(struct.pack("<5f", *v))
                if g["skinned"]:
                    for infl in g["skin"]:
                        infl = infl or []
                        f.write(struct.pack("<B", len(infl)))
                        for pi, w in infl:
                            f.write(struct.pack("<Hf", pi, w))
                f.write(struct.pack("<%dH" % len(g["idx"]), *g["idx"]))
            for c in m["clips"]:
                wstr(f, c["name"])
                f.write(struct.pack("<fI", c["duration"], c["frames"]))
                f.write(struct.pack("<I", len(c["pose"])))
                for pi in sorted(c["pose"]):
                    t, r, s = c["pose"][pi]
                    f.write(struct.pack("<H", pi))
                    f.write(struct.pack("<9f", *(list(t) + list(r) + list(s))))
                chans = [(pi, comp, collapse(k)) for pi, comp, k in c["chan"]]
                f.write(struct.pack("<I", len(chans)))
                for pi, comp, keys in chans:
                    f.write(struct.pack("<HHI", pi, comp, len(keys)))
                    for t, v in keys:
                        f.write(struct.pack("<ff", t, v))

        for nm, kind, rows, dur in paths:
            wstr(f, nm)
            f.write(struct.pack("<IIf", kind, len(rows), dur))
            for t, p, fw, up, st in rows:
                f.write(struct.pack("<11f", t, p[0], p[1], p[2],
                                    fw[0], fw[1], fw[2],
                                    up[0], up[1], up[2], st))

    tris = sum(len(g["idx"]) for b in built for g in b["batches"]) // 3
    nkeys = sum(len(collapse(k)) for b in built for c in b["m"]["clips"]
                for _p, _c, k in c["chan"])
    nsk = sum(1 for b in built for g in b["batches"] if g["skinned"])
    print("%s: CHR2  %d models, %d batches (%d skinned), %d tris, %d textures, "
          "%d keys, %d paths, %.2f MB"
          % (os.path.basename(a.out), len(built),
             sum(len(b["batches"]) for b in built), nsk, tris, len(tex_blobs),
             nkeys, len(paths), os.path.getsize(a.out) / 1e6))
    for b in built:
        print("   %-10s parts=%-3d batches=%-3d clips=%d yaw_off=%+6.1f  (%s)"
              % (b["m"]["name"], len(b["m"]["parts"]), len(b["batches"]),
                 len(b["m"]["clips"]), b["yaw_off"], b["fwd_witness"]))
    if missing:
        print("   MISSING textures: %s" % ", ".join(missing))
    drop = sum(b["dropped"] for b in built)
    if drop:
        print("   %d faces dropped (index past the vertex array)" % drop)


if __name__ == "__main__":
    main()
