#!/usr/bin/env python3
"""
gen_prop_data.py -- the knockable props, out of the game's own data.

Emits rccars_vita/prop_data.h: the 13 prop models, their rigid-body parameters,
their collision proxies, and the 129 placements across the ten tracks.

Three sources, none of them invented:

  RCCarsDB/stone.sb     each prop is a MOD_MODEL whose MOD_SCRIPT carries its
                        physics AS READABLE TEXT -- geometry type, mass,
                        coeffHookNorm, coeffFrict, shiftCenterMassOY, and on
                        exactly one object ExtForces { fWindBlow = 1 }. The type
                        numbers even carry name comments (// sphere, // bottle).
                        No disassembly needed for any of it.

  RCCars.exe            the collision proxy per geometry type. Each type has its
                        own loader around 0x4fa2e0-0x4fb280 reading `radius` and
                        `shift*` from a section named after the type, through the
                        INT getter at 0x404770, then applying the linear remap
                        this project has met before (FUN_0040BA70, inlined here
                        by the optimiser):

                            v = clamp((raw - inLo) * scale + outLo, outLo, outHi)

                        This script reads those float immediates straight out of
                        .data rather than trusting the .ini's own declared range,
                        because the two DISAGREE -- see BUCKET below.

  RCCarsDB/<track>.sb   MOD_INSTANCE nodes under the track's "Stones" folder,
                        whose 0x4072 string names the model and whose 0x540B is
                        the placement. Same node type and same fields as the race
                        start that gen_tracks.py already reads.

BUCKET IS THE EXCEPTION AND IT IS NOT A SLIDER. Its eight keys go through the
FLOAT getter at 0x404950, not the int one, so no remap is applied at all -- and
no BUCKET.crs ships, so the loader leaves the target holding whatever the image
had. That is the same shape as SpeedAngMaxREL in physLoadTires (see CLAUDE.md).
The .ini's own `Mask float / FValues 0.01 1.00` is the only statement of what the
numbers mean anywhere in the data, so that is what is used, and it is marked in
the output as the one prop constant that is not read back from a loader.

Usage:  python3 gen_prop_data.py [--check]
"""
import argparse
import math
import os
import re
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sb2obj as S

GAME = "/mnt/c/Games/RC Cars"
EXE = os.path.join(GAME, "RCCars.exe")
DB = os.path.join(GAME, "RCCarsDB")
SETTINGS = "/mnt/c/tools/rccars_extracted/Settings"
OUT = "/mnt/c/tools/rccars_vita/prop_data.h"

IMAGE_BASE = 0x400000

# The two config getters. Which one a key goes through decides whether a slider
# conversion happens at all, so they are named rather than numbered at the use.
GET_INT = 0x404770
GET_FLOAT = 0x404950

# The proxy loaders live in one run; bounding the disassembly keeps the scan
# honest rather than letting it wander into neighbouring functions. The upper
# bound is carUpdateSuspension, the next named thing -- SHELL's last key is read
# at 0x4fb273 and its arithmetic runs past 0x4fb280, so a tighter bound silently
# drops one sphere offset rather than failing.
PROXY_LO, PROXY_HI = 0x4FA280, 0x4FB340

TRACKS = ["beach_1", "beach_2", "beach_3", "beach_4", "country_1",
          "country_2", "country_3", "country_4", "urban_1", "urban_2"]

# stone.sb's own comments, kept as the enum so the header reads like the data.
GEOM_NAMES = {1: "SPHERE", 2: "CYLINDER", 3: "CONUS", 4: "BOTTLE",
              5: "BUCKET", 6: "SHELL", 7: "KETTLE", 8: "BALLON"}


# ----------------------------------------------------------------- the exe ---

class Image:
    def __init__(self, path):
        self.b = open(path, "rb").read()

    def f32(self, va):
        return struct.unpack_from("<f", self.b, va - IMAGE_BASE)[0]

    def cstr(self, va):
        off = va - IMAGE_BASE
        return self.b[off:self.b.index(b"\0", off)].decode("latin-1")


def disasm(path, lo, hi):
    """(va, mnemonic, operands) for one address range, via objdump."""
    out = subprocess.run(
        ["objdump", "-d", "-M", "intel",
         "--start-address=0x%x" % lo, "--stop-address=0x%x" % hi, path],
        capture_output=True, text=True).stdout
    ev = []
    for line in out.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*(\S+)\s*(.*)", line)
        if m:
            ev.append((int(m.group(1), 16), m.group(3), m.group(4).strip()))
    return ev


def ds_operand(arg):
    m = re.search(r"ds:0x([0-9a-f]+)", arg or "")
    return int(m.group(1), 16) if m else None


def read_proxy_loaders(img):
    """
    section -> {key: (out_lo, out_hi, scale, in_lo)} for the int-getter keys, and
    section -> {key: None} for float-getter keys (no conversion is applied).

    The window for a key's arithmetic is bounded by the NEXT call, which is what
    makes the section/key/constant association sound. Scanning loosely across
    call boundaries mixes one type's shift range into another's -- it produced a
    BALLON range on a SHELL key on the first attempt here.
    """
    ev = disasm(EXE, PROXY_LO, PROXY_HI)
    call_ix = [i for i, (_, op, _) in enumerate(ev) if op == "call"]
    out = {}
    pushes = []
    for i, (va, op, arg) in enumerate(ev):
        if op == "push" and re.fullmatch(r"0x5[0-9a-f]{5}", arg or ""):
            pushes.append(int(arg, 16))
            continue
        if op != "call":
            continue
        target = int(arg, 16) if re.fullmatch(r"0x[0-9a-f]+", arg or "") else None
        if len(pushes) >= 2 and target in (GET_INT, GET_FLOAT):
            key, section = img.cstr(pushes[-2]), img.cstr(pushes[-1])
            sec = out.setdefault(section, {})
            if target == GET_FLOAT:
                sec.setdefault(key, None)
            else:
                nxt = next((j for j in call_ix if j > i), len(ev))
                sec.setdefault(key, remap_after(img, ev[i + 1:nxt]))
        pushes = []
    return out


def remap_after(img, window):
    """
    Pick the inlined FUN_0040BA70 out of one key's window:

        fsub  inLo
        fmul  scale
        fadd  outLo      (or fsub, when outLo is negative)

    The two branch arms above it load the clamped ends; they are recomputed from
    outLo and scale instead of being read, so a misparsed branch cannot silently
    become a range.
    """
    in_lo = scale = out_lo = None
    for _, op, arg in window:
        a = ds_operand(arg)
        if a is None:
            continue
        if op == "fsub" and in_lo is None:
            in_lo = img.f32(a)
        elif op == "fmul" and in_lo is not None and scale is None:
            scale = img.f32(a)
        elif in_lo is not None and scale is not None:
            if op == "fadd":
                out_lo = img.f32(a)
                break
            if op == "fsub":
                out_lo = -img.f32(a)
                break
    if scale is None or out_lo is None:
        return None
    return (out_lo, out_lo + 99.0 * scale, scale, in_lo)


# ---------------------------------------------------------------- the .ini ---

def read_ini(name):
    """Slider defaults: Key "x" / Values <lo> <hi> <default>, plus FValues."""
    path = os.path.join(SETTINGS, name)
    if not os.path.exists(path):
        return {}
    keys, cur = {}, None
    for line in open(path, errors="replace"):
        line = line.strip()
        m = re.match(r'Key\s+"([^"]+)"', line)
        if m:
            cur = m.group(1)
            continue
        m = re.match(r"Values\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)", line)
        if m and cur:
            keys[cur] = {"raw": int(m.group(3)),
                         "lo": int(m.group(1)), "hi": int(m.group(2))}
            continue
        m = re.match(r"FValues\s+([-\d.]+)\s+([-\d.]+)", line)
        if m and cur and cur in keys:
            keys[cur]["frange"] = (float(m.group(1)), float(m.group(2)))
    return keys


# ------------------------------------------------------------- the stone.sb --

def read_sound(b, reg, body, end):
    """The model's own MOD_SNDCHANNEL: (wav name, 0x50E0, 0x50E1).

    Every one of the thirteen carries exactly one, holding a MOD_SOUND that names
    the wav -- the same structure gen_tracks.py reads the per-track ambient bed
    out of, and the reason the port has to invent nothing about which object makes
    which noise:

        Ball, Ball_CB -> ball      Ball_CS -> ball2     Conus, Shell -> conus
        Cola, Tonic, Ballon -> pepsi                    Klubok -> klubok
        Whiskey1, Whiskey2 -> whiskey                   Vedro, Kettle -> vedro

    Note that Shell borrows the traffic cone's sound and Kettle the bucket's, and
    that the channel NAMES do not all match the model names (Whiskey2's channel is
    `Whiskey3`, Ball_CS's is `Ball2`) -- so this reads the sound from inside each
    model's own subtree rather than matching names up against chn.dat.

    The radii come back as authored, and 0x50E0 > 0x50E1 on all thirteen (30.0
    against 8.0). That is not a mistake to correct here: under the port's distance
    law (mix_pan -- "rmin: full volume inside, rmax: silent outside") the pair
    means full volume out to 8 m and silence past it, which is a well-defined
    answer for an object this small. Passed through as authored.
    """
    for cid, cnt, cb, ce in S.children(b, body, end):
        if cid != 0x9200:
            continue
        if reg.get(cnt) == "MOD_SNDCHANNEL":
            rmin = rmax = 0.0
            name = None
            for c2, n2, b2, e2 in S.children(b, cb, ce):
                if c2 == 0x50E0 and e2 - b2 >= 4:
                    rmin = struct.unpack_from("<f", b, b2)[0]
                elif c2 == 0x50E1 and e2 - b2 >= 4:
                    rmax = struct.unpack_from("<f", b, b2)[0]
                elif c2 == 0x9200 and reg.get(n2) == "MOD_SOUND":
                    name = S.node_name(b, b2, e2)
            if name:
                return name, rmin, rmax
        got = read_sound(b, reg, cb, ce)
        if got:
            return got
    return None


def read_models():
    """
    [(name, {type, mass, coeffHookNorm, coeffFrict, shiftCenterMassOY, wind,
             sound, snd_rmin, snd_rmax})]

    The MOD_SCRIPT text blocks and the MOD_MODEL nodes are both in file order and
    there are exactly as many of one as the other, which is what pairs them. The
    pairing is ASSERTED on the type comments: a script that says "// bottle" must
    land on a model whose own name is a bottle. Get that wrong and every prop
    gets someone else's mass.
    """
    b = open(os.path.join(DB, "stone.sb"), "rb").read()
    reg, start = S.read_registry(b)

    models = []
    scripts = []

    def walk(p, end, depth):
        while p < end:
            cid, cend, cnt, body = S.rd_chunk(b, p)
            if cend <= p or cend > end:
                return
            if cid == 0x9200 and cnt is not None:
                t = reg.get(cnt)
                nm = S.node_name(b, body, cend)
                if t == "MOD_MODEL":
                    models.append((body, cend, nm))
                walk(body, cend, depth + 1)
            p = cend

    walk(start, len(b), 0)
    for m in re.finditer(rb"Section Keys \"Geom\"(.{0,600}?)End\s*$", b,
                         re.S | re.M):
        pass
    # The blocks are plain text in the blob; find them by their opening line.
    for m in re.finditer(rb'Section Keys "Geom"', b):
        chunk = b[m.start():m.start() + 700].split(b"\0")[0].decode("latin-1")
        d = {}
        for k, v in re.findall(r"(\w+)\((?:int|float)\)\s*=\s*(-?[\d.]+)", chunk):
            d[k] = float(v)
        cm = re.search(r"type\(int\)\s*=\s*(\d+)\s*//\s*(\w+)", chunk)
        if cm:
            d["type"] = int(cm.group(1))
            d["comment"] = cm.group(2)
        d["wind"] = 1.0 if "fWindBlow" in chunk else 0.0
        scripts.append((m.start(), d))

    models.sort()
    scripts.sort()
    if len(models) != len(scripts):
        raise SystemExit("stone.sb: %d models but %d script blocks -- the file "
                         "order pairing this relies on is not safe"
                         % (len(models), len(scripts)))
    out = []
    for (mbody, mend, nm), (_, d) in zip(models, scripts):
        snd = read_sound(b, reg, mbody, mend)
        if not snd:
            # Every shipped model has one. A missing channel is a parse failure,
            # not a silent prop -- fail loudly rather than shipping one object
            # that makes no noise for no stated reason.
            raise SystemExit("stone.sb: model %r has no MOD_SNDCHANNEL" % nm)
        d["sound"], d["snd_rmin"], d["snd_rmax"] = snd
        out.append((nm, d))
    return out


# --------------------------------------------------------- the placements ----

def read_placements(track):
    """MOD_INSTANCE under a Stones folder: (name, model, pos, scale, rot_deg)."""
    b = open(os.path.join(DB, track + ".sb"), "rb").read()
    reg, start = S.read_registry(b)
    rows = []

    def walk(p, end, trail):
        while p < end:
            cid, cend, cnt, body = S.rd_chunk(b, p)
            if cend <= p or cend > end:
                return
            if cid == 0x9200 and cnt is not None:
                t = reg.get(cnt)
                nm = S.node_name(b, body, cend)
                if t == "MOD_INSTANCE":
                    ref = None
                    for c2, _, cb2, ce2 in S.children(b, body, cend):
                        if c2 == 0x4072:
                            ref = b[cb2:ce2].split(b"\0")[0].decode("latin-1")
                    trs = S.node_trs(b, body, cend)
                    # The folder is what separates a prop from a character; both
                    # are MOD_INSTANCE and both name an external database.
                    in_stones = any((x or "").lower() == "stones" for x in trail)
                    if ref and trs and in_stones:
                        rows.append((nm, ref, trs[0], trs[1], trs[2]))
                walk(body, cend, trail + [nm])
            p = cend

    walk(start, len(b), [])
    return rows


# ------------------------------------------------------------------ output ---

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true",
                    help="print what was recovered and write nothing")
    args = ap.parse_args()

    img = Image(EXE)
    proxies = read_proxy_loaders(img)
    models = read_models()

    # geometry type -> the section its loader reads
    SECTION_OF = {2: "CYLINDER", 4: "BOTTLE", 5: "BUCKET", 6: "SHELL",
                  7: "KETTLE", 8: "BALLON"}
    INI_OF = {"CYLINDER": "CYLINDER.ini", "BOTTLE": "BOTTLE.ini",
              "BUCKET": "BUCKET.ini", "SHELL": "SHELL.ini",
              "KETTLE": "kettle.ini", "BALLON": "BALLON.ini"}

    resolved = {}
    for gt, sec in SECTION_OF.items():
        ini = read_ini(INI_OF[sec])
        conv = proxies.get(sec, {})
        vals = {}
        for key, spec in ini.items():
            raw = spec["raw"]
            rm = conv.get(key)
            if rm is not None:
                out_lo, out_hi, scale, in_lo = rm
                v = (raw - in_lo) * scale + out_lo
                v = max(out_lo, min(out_hi, v))
                vals[key] = (v, "loader")
            elif "frange" in spec:
                lo, hi = spec["frange"]
                v = lo + (raw - spec["lo"]) * (hi - lo) / float(
                    spec["hi"] - spec["lo"])
                vals[key] = (v, "ini-float")
            else:
                vals[key] = (None, "unresolved")
        resolved[gt] = (sec, vals)

    placements = {t: read_placements(t) for t in TRACKS}

    if args.check:
        print("proxy sections found in the exe: %s"
              % ", ".join(sorted(proxies)))
        for gt in sorted(resolved):
            sec, vals = resolved[gt]
            print("\n[%d %s] section %s" % (gt, GEOM_NAMES[gt], sec))
            for k, (v, src) in sorted(vals.items()):
                print("    %-14s %-10s %s" % (
                    k, ("%.4f" % v) if v is not None else "?", src))
        print("\nmodels:")
        for nm, d in models:
            print("    %-10s type %d %-9s mass %-5s hook %-4s frict %-4s "
                  "comY %-5s wind %d"
                  % (nm, d.get("type", -1), "(%s)" % d.get("comment", "?"),
                     d.get("mass"), d.get("coeffHookNorm"), d.get("coeffFrict"),
                     d.get("shiftCenterMassOY"), int(d.get("wind", 0))))
        n = sum(len(v) for v in placements.values())
        print("\nplacements: %d across %d tracks" % (n, len(placements)))
        for t in TRACKS:
            print("    %-11s %d" % (t, len(placements[t])))
        return

    write_header(resolved, models, placements)
    print("wrote %s" % OUT)


def fl(x):
    """A C float literal that is always a float: %g renders 1.0 as "1", and
    "1f" is an integer with a bad suffix, not a float."""
    t = repr(float(x))
    if "e" in t or "E" in t:
        return t + "f"
    if "." not in t:
        t += ".0"
    return t + "f"


def write_header(resolved, models, placements):
    model_verts = {nm: model_vertices(nm) for nm, _ in models}
    name_of = {nm: i for i, (nm, _) in enumerate(models)}
    w = []
    a = w.append
    a("/*")
    a(" * prop_data.h -- GENERATED by rccars_re/gen_prop_data.py. Do not edit.")
    a(" *")
    a(" * The 13 knockable props, their physics, their collision proxies and the")
    a(" * 129 placements across the ten tracks. Every number here is read back")
    a(" * from the game: the physics out of stone.sb's own MOD_SCRIPT text, the")
    a(" * proxy dimensions out of the per-type loaders at 0x4fa2e0-0x4fb280, the")
    a(" * placements out of each track's Stones folder. See the generator for")
    a(" * which source each field came from and for the one exception (BUCKET).")
    a(" */")
    a("#ifndef PROP_DATA_H")
    a("#define PROP_DATA_H")
    a("")
    for gt in sorted(GEOM_NAMES):
        a("#define PROP_GEOM_%-9s %d" % (GEOM_NAMES[gt], gt))
    a("")
    a("/* At most this many spheres in a prop's collision proxy: the bucket has")
    a("   three tiers and nothing else has more than three shifts. */")
    a("#define PROP_MAX_SPHERES 3")
    a("")
    a("typedef struct {")
    a("    const char *name;")
    a("    int   geom;                 /* PROP_GEOM_* */")
    a("    float mass;                 /* kg, stone.sb */")
    a("    float restitution;          /* coeffHookNorm */")
    a("    float friction;             /* coeffFrict */")
    a("    float com_y;                /* shiftCenterMassOY, metres */")
    a("    int   wind;                 /* ExtForces fWindBlow -- the tumbleweed */")
    a("    int   n_spheres;")
    a("    float sphere[PROP_MAX_SPHERES][4];   /* x, y, z, radius */")
    a("    /* The sound this object makes when the car hits it, and the two radii")
    a("       it is heard over -- straight out of the model's own MOD_SNDCHANNEL")
    a("       in stone.sb (0x4003 for the wav, 0x50E0 / 0x50E1 for the radii).")
    a("       Seven wavs cover the thirteen models: Shell shares the cone's")
    a("       and Kettle the bucket's. See read_sound in gen_prop_data.py for why")
    a("       rmin comes out LARGER than rmax on every one of them, and why that")
    a("       is passed through rather than 'fixed'. */")
    a("    const char *sound;")
    a("    float snd_rmin, snd_rmax;")
    a("} prop_model_t;")
    a("")
    a("static const prop_model_t PROP_MODELS[] = {")
    for nm, d in models:
        gt = int(d.get("type", 1))
        spheres = proxy_spheres(gt, resolved, model_verts.get(nm))
        a("    { \"%s\", PROP_GEOM_%s, %s, %s, %s, %s, %d, %d, {"
          % (nm, GEOM_NAMES.get(gt, "SPHERE"), fl(d.get("mass", 0.5)),
             fl(d.get("coeffHookNorm", 0.3)), fl(d.get("coeffFrict", 1.0)),
             fl(d.get("shiftCenterMassOY", 0.0)), int(d.get("wind", 0)),
             len(spheres)))
        for s in spheres:
            a("        { %s, %s, %s, %s }," % tuple(fl(x) for x in s))
        for _ in range(len(spheres), 3):
            a("        { 0.f, 0.f, 0.f, 0.f },")
        a("      }, \"%s\", %s, %s },"
          % (d["sound"], fl(d["snd_rmin"]), fl(d["snd_rmax"])))
    a("};")
    a("")
    a("/* The distinct wavs the thirteen models name, so sfx.c can page the set")
    a("   without walking PROP_MODELS looking for duplicates. */")
    sounds = sorted({d["sound"] for _, d in models})
    a("#define PROP_N_SOUNDS %d" % len(sounds))
    a("static const char *const PROP_SOUNDS[PROP_N_SOUNDS] = {")
    a("    " + ", ".join('"%s"' % s for s in sounds))
    a("};")
    a("#define PROP_N_MODELS %d" % len(models))
    a("")
    a("typedef struct {")
    a("    int   model;                /* index into PROP_MODELS */")
    a("    float pos[3];")
    a("    float yaw;                  /* degrees, about world up */")
    a("} prop_place_t;")
    a("")
    for t in TRACKS:
        rows = placements[t]
        a("static const prop_place_t PROP_PLACE_%s[] = {" % t)
        for nm, ref, pos, scl, rot in rows:
            mi = name_of.get(ref, -1)
            a("    { %2d, { %s, %s, %s }, %s },   /* %s (%s) */"
              % (mi, fl(pos[0]), fl(pos[1]), fl(pos[2]), fl(rot[1]), nm, ref))
        if not rows:
            a("    { -1, { 0.f, 0.f, 0.f }, 0.f },")
        a("};")
    a("")
    a("typedef struct { const prop_place_t *p; int n; } prop_track_t;")
    a("static const prop_track_t PROP_TRACKS[] = {")
    for t in TRACKS:
        n = len(placements[t])
        a("    { PROP_PLACE_%s, %d }," % (t, n))
    a("};")
    a("")
    a("#endif /* PROP_DATA_H */")
    open(OUT, "w").write("\n".join(w) + "\n")


def fit_proxy(verts, n_spheres):
    """
    THE PORT'S, and the only fitted number in this header -- everything else is
    read back from a loader or from stone.sb's own text.

    Geometry types 1 (sphere) and 3 (conus) have no Settings section and no
    loader, so the engine must be sizing them from something else; whatever that
    is has not been recovered. Ball, Ball_CS, Ball_CB, Klubok and Conus are 25 of
    the 129 placements, so "leave them without a proxy" is not an option.

    The fit is made from the shipped mesh rather than typed in: slice the
    vertices into n height bands and give each band the sphere that encloses it.
    One band is the enclosing sphere, which is exact for the four spherical
    props. Two give a traffic cone a fat base and a thin top, which is the whole
    reason not to hand a cone a single sphere.
    """
    if not verts:
        return []
    ys = [v[1] for v in verts]
    lo, hi = min(ys), max(ys)
    if hi - lo < 1e-6:
        hi = lo + 1e-6
    # The axis is the mesh's own, NOT x = z = 0. Conus and Klubok are modelled
    # well off their origin -- measuring the radius from the origin called the
    # traffic cone 5.6 m wide, against a real 0.33 m bounding box.
    cx, cz = model_xz_centre(verts)
    out = []
    for i in range(n_spheres):
        y0 = lo + (hi - lo) * i / n_spheres
        y1 = lo + (hi - lo) * (i + 1) / n_spheres
        band = [v for v in verts if y0 - 1e-6 <= v[1] <= y1 + 1e-6]
        if not band:
            continue
        cy = 0.5 * (y0 + y1)
        # reach the corners of the band, not just its waist
        r = max(math.sqrt((v[0] - cx) ** 2 + (v[2] - cz) ** 2 + (v[1] - cy) ** 2)
                for v in band)
        out.append((0.0, cy, 0.0, r))
    return out


def model_xz_centre(verts):
    """
    How far a model's geometry sits from its own origin in X and Z.

    Twelve of the thirteen are modelled on the origin. CONUS IS NOT: its mesh
    node carries a translation of (3.880, 0.096, -3.840), so composed straight
    it lands 5.5 m away, and the fitted proxy came out with a 5.6 m radius.

    Whether the engine applies that offset was decided against the collision
    grids rather than guessed, because it decides where fifteen traffic cones
    stand. A cone rests ON the ground, so the right answer is the one that puts
    its base near the terrain under it. Over beach_3's five cones the mean
    |y - ground| is 0.43 m at the instance position and 1.28 m with the offset
    added; beach_2 agrees on 7 of its 10 (the other three sit near structures
    where the topmost collision triangle is not the floor they rest on). So the
    instance position is where the cone goes and the offset is not applied.

    Only X and Z are removed. The Y component is meaningful and consistent
    across all thirteen -- Ball's mesh node sits at y = 0.120, exactly its
    radius, which is what puts the ball ON the ground rather than half through
    it. Removing Y as well would bury every prop to its waist.

    pack_props.py subtracts the same value from the packed vertices, so the
    drawn mesh and the collision proxy cannot drift apart.
    """
    if not verts:
        return (0.0, 0.0)
    return (0.5 * (min(v[0] for v in verts) + max(v[0] for v in verts)),
            0.5 * (min(v[2] for v in verts) + max(v[2] for v in verts)))


def model_vertices(name):
    """Every vertex of one MOD_MODEL subtree, in model space."""
    b = open(os.path.join(DB, "stone.sb"), "rb").read()
    reg, start = S.read_registry(b)
    meshes, _ = S.collect(b, reg, start, subtree=name)
    return [v for m in meshes for v in m[1]]


# how many spheres to fit for a type that has no Settings section
FIT_SPHERES = {1: 1, 3: 2}


def proxy_spheres(gt, resolved, verts=None):
    """The proxy for one geometry type, as (x, y, z, r) triples."""
    if gt not in resolved:
        # sphere and conus have no section of their own -- fit to the mesh
        return fit_proxy(verts or [], FIT_SPHERES.get(gt, 1))
    sec, v = resolved[gt]

    def g(k):
        e = v.get(k)
        return e[0] if e and e[0] is not None else None

    r = g("radius")
    if sec == "BUCKET":
        out = []
        for rk, dy, dx, dz in (("radiusSmall", "dySmall", "dxSmall", "dzSmall"),
                               ("radiusMiddle", "dyMiddle", None, None),
                               ("radiusUp", "dyUp", None, None)):
            rr = g(rk)
            if rr is None:
                continue
            out.append((g(dx) or 0.0, g(dy) or 0.0, g(dz) or 0.0, rr))
        return out
    if sec == "KETTLE":
        out = []
        for yk, xk, zk in (("shiftY1", "shiftX1", "shiftZ1"), ("shiftY2", None, None)):
            y = g(yk)
            if y is None or r is None:
                continue
            out.append((g(xk) or 0.0, y, g(zk) or 0.0, r))
        return out
    out = []
    for k in ("shift0", "shift1", "shift2"):
        y = g(k)
        if y is None or r is None:
            continue
        out.append((0.0, y, 0.0, r))
    return out


if __name__ == "__main__":
    main()
