#!/usr/bin/env python3
"""
gen_char_data.py -- char_data.h: the 59 characters, where they stand, and every
behaviour constant the engine loads for them.

Two halves, both read out of shipped data:

THE PLACEMENTS come from each track's own .sb. A character is a MOD_INSTANCE
whose 0x4072 names a model in AIChars.sb / people.sb / Roadcar.sb -- the same
node type a prop uses, which is why gen_prop_data.py has to check the enclosing
FOLDER to tell them apart. What makes the characters richer than the props is
that every instance carries its own brief:

    MOD_MARKER   a named point, with a heading:
                 Seagull  flightDir            where it flies off to
                 Crab     seahole1, landhole1  the two holes it runs between
                 Spider   hole1, hole2         ditto
                 Dog      resetPlace           where it goes back to
    MOD_VOLUME   a named prism: 0x80D4 is ten floats, a y range and then FOUR
                 (x, z) corners in traversal order -- a rotated box, and every
                 shipped one is a parallelogram to the millimetre.
                 Seagull  visibilityVolume, inhabitVolume
                 Crab     visibilityVolume
                 Spider   visibilityVolume
                 Dog      attackVolume
                 Guard    visibilityVolume, guardVolume
    MOD_SCRIPT   0x807E holds READABLE TEXT in the engine's own config syntax:
                 Section Keys "Path" { name(string), clother(int) }
                 Section Keys "Model" { body(int), box(int) }
                 `name` is the recorded path -- Profiles/<name>.dat for a
                 person; `clother`, `body` and `box` pick a texture variant.

THE CONSTANTS come from the loaders in the exe, not from the .ini defaults. Nine
config files feed nine loaders and every one of them was found the standard way:
the key string's address, the `push` of it, then the arithmetic between the
int getter at 0x404770 and the `fstp` that stores the result. All but two are a
flat multiply:

    dog        0x519db0   ConeAngle raw degrees; the other five x0.01
    seagull    0x5133a0   VertAngleMax raw degrees; the other five x0.01
    spider     0x515af0   CdtRad x0.01; CdtShiftY x0.01 - 1.0; the two
                          HeightFloor* x0.001
    vulture    0x51b530   FlyVel, Scale x0.01; ShiftY (raw - 100) x 0.1
    People     0x5120e0   speedForw is a CLAMPED REMAP, not a multiply --
                          raw <= 1 gives 0, raw > 100 gives 2, else
                          (raw - 1) * 0.020202
    peop_sole  0x50fcb0   all four x0.01
    guard      0x51aa90   all four x0.01
    roadcar    0x517650   TowerShift (raw - 200) x 0.01, TowerShootT x0.1,
                          the rest x0.01

The shipped VALUE is the .crs where one exists and the .ini default where it
does not -- spider, vulture and roadcar ship no .crs at all, exactly as
BUCKET.ini does for the props.

Usage:
    python3 gen_char_data.py --check
    python3 gen_char_data.py /mnt/c/tools/rccars_vita/char_data.h
"""
import argparse
import math
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sb2obj as S
import sbchar
from dump_physics import read_crs

EXE = "/mnt/c/Games/RC Cars/RCCars.exe"
DB = "/mnt/c/Games/RC Cars/RCCarsDB"
SETTINGS = "/mnt/c/tools/rccars_extracted/Settings"
PROFILES = "/mnt/c/tools/rccars_extracted/Profiles"

TRACKS = ["beach_1", "beach_2", "beach_3", "beach_4", "country_1", "country_2",
          "country_3", "country_4", "urban_1", "urban_2"]

# Everything a track can instance. The folder a MOD_INSTANCE sits in is not a
# reliable discriminator -- beach_1 files them under Seagulls / Crabs / Peoples,
# beach_4 under "AI Characters", urban_2 directly under the track node -- so the
# model NAME is what separates a character from a prop.
MODELS = ["Dog", "Seagull", "Crab", "Spider", "Vulture",
          "Man", "Woman", "RepairMan", "Guard",
          "Truck", "Trailer", "Hammer", "Btr"]

# Which behaviour each model runs. The port's own grouping: the data says what a
# character HAS (a flightDir, two holes, a guardVolume, a recorded path) and
# these are the five shapes those brief sheets come in.
KIND = {
    "Dog": "DOG", "Seagull": "SEAGULL", "Crab": "BURROW", "Spider": "BURROW",
    "Vulture": "PATH", "Man": "WALKER", "Woman": "WALKER",
    "RepairMan": "WALKER", "Guard": "GUARD", "Truck": "PATH",
    "Trailer": "PATH", "Hammer": "PATH", "Btr": "PATH",
}


# ------------------------------------------------------- the collision proxy ---
#
# THE CHARACTERS ARE SOLID IN THE RETAIL GAME AND THE PORT DROVE THROUGH THEM.
# Not a guess and not a design decision: the engine keeps two registries and the
# game fills both at startup out of one function, 0x534d00.
#
#   0x533130(fourcc, provider)   the collision PROXY of one object kind, up to 10
#       $CAR 0x4ef9e0   STON 0x50dc50   PEOP 0x511e30   CRAB 0x513840
#       SPDR 0x514cb0   RDCR 0x516480   DOG$ 0x518820   GUAM 0x51a350
#
#   0x533170(a, b, filter, resolver)   which kinds collide, up to 20
#       $CAR/$CAR  $CAR/STON  STON/STON  $CAR/PEOP  $CAR/GUAM
#       $CAR/CRAB  $CAR/SPDR  $CAR/RDCR  STON/RDCR  $CAR/DOG$
#
# So the car collides with the people, the guards, the crabs, the spiders, the
# road cars and the dogs; the road cars knock the props over too. The seagull and
# the vulture appear in NEITHER registry -- they fly, and they are the two models
# this table therefore leaves unsolid.
#
# A provider is provider(obj, int *out_n, sphere out[]) with sphere = 4 floats
# (x, y, z, radius), the same shape rb_gather_spheres writes for the car, and the
# 4CCs are big-endian in the same convention the animation channels' 0x3057 uses.
# The resolvers end at 0x4f0730 -> rb_coll_resolve (0x4f0750) with the CAR as the
# only body, so the car is pushed out and the character is not moved: it is the
# static side, exactly as the world is.
#
# Every radius below is the immediate 0x3e4ccccd = 0.2f in the provider itself,
# and the stack step is the global at 0x5544f0 = 0.2 along the world up vector
# at 0x55e9a0 = (0, 1, 0).
#
#   PEOP  0x511e30  n=3  r 0.2  at the position, +0.2, +0.4 in world Y
#   GUAM  0x51a350  n=2  r 0.2  at the position, +0.2
#   CRAB  0x513840  n=1  r 0.2  at the position, y - 0.18 (0x554b14)
#   SPDR  0x514cb0  n=1  r CdtRad 0.23, y + CdtShiftY -0.24 -- the ONLY proxy
#                        whose numbers come out of a settings file, and the
#                        reason gen_char_data's own CdtShiftY was wrong by a
#                        metre until this was read (see read_conversion)
#   DOG$  0x518820  n=5  r 0.2  at the WORLD POSITIONS of five named bones,
#                        cached at 0x518cb5..0x518d9d: LHAND1, RHAND1, LFOOT2,
#                        RFOOT2, HEAD -- the four paws and the head
#   RDCR  0x516480  up to 8 spheres from a per-instance table of local offsets
#                        (state+0x264, stride 0xc) and radii (state+0x2c4,
#                        stride 4) turned by the object's own matrix. WHERE THAT
#                        TABLE IS FILLED FROM IS NOT RECOVERED -- it is written
#                        through base-plus-index arithmetic, and no road-car node
#                        name, .ini or .sb chunk carries it. So the road cars get
#                        a proxy of the PORT'S below, and it is the one entry
#                        here that is not the engine's own numbers.
#
# The layer is stepped once per FRAME with dt (0x4fc685), not per physics
# substep, over the objects within 10.0 m of the car (the immediate at 0x53343b),
# so there is no carSubstepContact bisection behind it and no depenetration in
# it. That is the original, and it is why CHR_SOLID_RANGE is 10 m.
PROXY_R = 0.2                 # 0x3e4ccccd, in every provider
PROXY_STEP = 0.2              # 0x5544f0, along (0,1,0) at 0x55e9a0
PROXY_CRAB_DOWN = 0.18        # 0x554b14
PROXY_SOLID_RANGE = 10.0      # 0x41200000 at 0x53343b
PROXY_MAX = 8                 # RDCR's slot count, the largest of the six

# kind: COLUMN spheres stacked in world +Y from the instance position, BONES
# spheres at named joints, HULL the port's own for the road cars, NONE unsolid.
PROXY = {
    "Man":       ("COLUMN", 3, PROXY_R, 0.0, PROXY_STEP, None, "PEOP 0x511e30"),
    "Woman":     ("COLUMN", 3, PROXY_R, 0.0, PROXY_STEP, None, "PEOP 0x511e30"),
    "RepairMan": ("COLUMN", 3, PROXY_R, 0.0, PROXY_STEP, None, "PEOP 0x511e30"),
    "Guard":     ("COLUMN", 2, PROXY_R, 0.0, PROXY_STEP, None, "GUAM 0x51a350"),
    "Crab":      ("COLUMN", 1, PROXY_R, -PROXY_CRAB_DOWN, 0.0, None,
                  "CRAB 0x513840"),
    "Spider":    ("COLUMN", 1, None, None, 0.0, None, "SPDR 0x514cb0"),
    "Dog":       ("BONES", 5, PROXY_R, 0.0, 0.0,
                  ["LHAND1", "RHAND1", "LFOOT2", "RFOOT2", "HEAD"],
                  "DOG$ 0x518820"),
    "Truck":     ("HULL", 3, None, 0.0, 0.0, None, "RDCR 0x516480, the port's"),
    "Trailer":   ("HULL", 3, None, 0.0, 0.0, None, "RDCR 0x516480, the port's"),
    "Hammer":    ("HULL", 3, None, 0.0, 0.0, None, "RDCR 0x516480, the port's"),
    "Btr":       ("HULL", 3, None, 0.0, 0.0, None, "RDCR 0x516480, the port's"),
    "Seagull":   ("NONE", 0, 0.0, 0.0, 0.0, None, "in neither registry"),
    "Vulture":   ("NONE", 0, 0.0, 0.0, 0.0, None, "in neither registry"),
}


# --------------------------------------------------------------- the image ---

class Image(object):
    def __init__(self, path):
        self.d = open(path, "rb").read()
        d = self.d
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        optsz = struct.unpack_from("<H", d, pe + 20)[0]
        self.base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
        self.secs = []
        off = pe + 24 + optsz
        for i in range(nsec):
            e = off + 40 * i
            vs, va, rs, ro = struct.unpack_from("<IIII", d, e + 8)
            self.secs.append((self.base + va, vs, ro, rs))

    def off(self, va):
        for sva, vs, ro, rs in self.secs:
            if sva <= va < sva + max(vs, rs):
                return ro + (va - sva)
        return None

    def va(self, off):
        for sva, vs, ro, rs in self.secs:
            if ro <= off < ro + rs:
                return sva + (off - ro)
        return None

    def f32(self, va):
        o = self.off(va)
        if o is None or o + 4 > len(self.d):
            return None
        return struct.unpack_from("<f", self.d, o)[0]

    def strings(self, s):
        pat = s.encode() + b"\0"
        out, o = [], 0
        while True:
            o = self.d.find(pat, o)
            if o < 0:
                break
            va = self.va(o)
            if va and va > 0x550000:
                out.append(va)
            o += 1
        return out

    def pushes(self, va):
        pat = struct.pack("<BI", 0x68, va)
        out, o = [], 0
        while True:
            o = self.d.find(pat, o)
            if o < 0:
                break
            r = self.va(o)
            if r and 0x401000 <= r < 0x570000:
                out.append(r)
            o += 1
        return out


def read_conversion(img, site, window=0x60):
    """The arithmetic between a key's `push` and the `fstp` that stores it.

    Scans forward from the push for the x87 forms that take an absolute
    address -- d8 25 fsub, d8 05 fadd, d8 0d fmul, d9 1d fstp -- and stops at
    the first fstp. Returns (ops, dest) where ops is the list of
    ("mul"|"add", value) IN THE ORDER THE INSTRUCTIONS RUN, or None if there is
    no fstp in the window.

    THE ORDER IS THE POINT, and getting it wrong cost a metre on a 7 cm animal.
    This used to fold every op into one (scale, offset) pair and report
    `(raw + offset) * scale`, which is only the arithmetic when the fsub comes
    BEFORE the fmul. The spider's CdtShiftY is the other order -- 0x515b8c is
    `fmul 0.01` then `fsub 1.0`, i.e. raw*0.01 - 1.0 -- so raw 76 was published
    as +0.75 where the engine computes -0.24, and the spider's collision sphere
    (the only thing that reads it) sat a metre above the spider. The vulture's
    ShiftY and the roadcar's TowerShift ARE offset-first and are unchanged.

    Done by byte pattern rather than by disassembler on purpose: the whole
    shape is four two-byte opcodes with an absolute32 each, and it is the same
    shape scale_consts.py already recovers for the physics loaders."""
    o = img.off(site)
    if o is None:
        return None
    d = img.d
    ops, dest = [], None
    i = o
    end = o + window
    while i < end - 6:
        op = d[i:i + 2]
        if op in (b"\xd8\x25", b"\xd8\x05", b"\xd8\x0d", b"\xd9\x1d"):
            va = struct.unpack_from("<I", d, i + 2)[0]
            if img.off(va) is None:
                i += 1
                continue
            v = img.f32(va)
            if op == b"\xd9\x1d":
                dest = va
                break
            if op == b"\xd8\x25":
                ops.append(("add", -v))
            elif op == b"\xd8\x05":
                ops.append(("add", v))
            else:
                ops.append(("mul", v))
            i += 6
            continue
        i += 1
    if dest is None:
        return None
    return ops, dest


def apply_ops(raw, ops):
    """Replay read_conversion's ops in order. Single precision at every step,
    the way the x87 store to a float global leaves it."""
    v = float(raw)
    for kind, k in ops:
        v = v * k if kind == "mul" else v + k
        v = struct.unpack("<f", struct.pack("<f", v))[0]
    return v


def ops_formula(ops):
    """The arithmetic as a string, for the header comment."""
    out = "raw"
    for i, (kind, k) in enumerate(ops):
        if i:
            out = "(%s)" % out
        if kind == "mul":
            out = "%s * %g" % (out, k)
        elif k < 0:
            out = "%s - %g" % (out, -k)
        else:
            out = "%s + %g" % (out, k)
    return out


# ---------------------------------------------------------------- the .ini ---

def read_ini(name):
    """Slider files: Key "X" / Values lo hi default."""
    p = os.path.join(SETTINGS, name)
    out, key = {}, None
    if not os.path.exists(p):
        return out
    for line in open(p, "r", errors="replace"):
        line = line.strip()
        m = re.match(r'Key\s+"([^"]+)"', line)
        if m:
            key = m.group(1)
        elif key and line.startswith("Values"):
            q = line.split()
            if len(q) >= 4:
                out[key] = (int(q[1]), int(q[2]), int(q[3]))
            key = None
    return out


def shipped(ini_name, crs_name):
    """raw value per key: the .crs where one ships, the .ini default otherwise."""
    ini = read_ini(ini_name)
    crs = {}
    p = os.path.join(SETTINGS, crs_name)
    if os.path.exists(p):
        try:
            crs = read_crs(p)
        except Exception:
            crs = {}
    out = {}
    for k, (lo, hi, dflt) in ini.items():
        out[k] = (crs[k][2] if k in crs else dflt, k in crs)
    return out


# One entry per config file: (ini, crs, keys in emission order). The angles are
# the two keys whose loader applies no scale at all; they are listed here only
# so the check output can say so -- the conversion itself is read from the exe.
GROUPS = [
    ("dog", "dog.ini", "dog.crs",
     ["ConeAngle", "RadNear", "RadFar_Near", "RadFar_Far", "RadReach",
      "TimeAttack", "AniSpeed"]),
    ("seagull", "seagull.ini", "seagull.crs",
     ["SpeedRun", "SpeedFlight", "FlightHeight", "VertAngleMax", "TimeIdle",
      "TimeTurn"]),
    ("spider", "spider.ini", "spider.crs",
     ["CdtRad", "CdtShiftY", "HeightFloor", "HeightFloorDead"]),
    ("vulture", "vulture.ini", "vulture.crs", ["FlyVel", "Scale", "ShiftY"]),
    ("people", "People.ini", "People.crs", ["speedForw"]),
    ("sole", "peop_sole.ini", "peop_sole.crs",
     ["TimeDelay", "ShiftX", "ShiftY", "Size"]),
    ("guard", "guard.ini", "guard.crs",
     ["TimeFollow", "TimeAttack", "TimeFollowEx", "ExVolumeDelta"]),
    ("roadcar", "roadcar.ini", "roadcar.crs",
     ["DeltaAlpha", "ShadowY", "ShadowR", "TowerVel", "TowerShift",
      "TowerShootT"]),
]

# ---------------------------------------------------------------------------
# BURST_MNG -- the gunfire, and its loader is a DIFFERENT GETTER.
#
# The guard shoots by spawning a 'BRMN' object (0x51aa81 pushes the 4CC), and
# every constant that object runs on comes out of Settings/burst_mng.ini through
# the FLOAT getter at 0x532410, whose fifth argument is a DIVISOR -- value is
# raw / div, not raw * scale -- so read_conversion's cfgGetInt-then-fmul pattern
# cannot see any of them. The call shape is fixed and regular:
#
#     push <div:f32> ; push 1 ; push <key> ; push "BURST_MNG" ; push obj ; call
#
# so the divisor's own push sits exactly 7 bytes before the key's, and the
# destination global is the `fstp ds:` (or `mov ds:,eax` for the int) that
# follows the call. Read that way rather than transcribed, so a wrong address
# reads a wrong number instead of agreeing with a comment.
BURST_SECTION = "BURST_MNG"
BURST_GETTER = 0x532410
BURST_KEYS = ["NShoots", "TimeShoot", "Offset", "Radius", "CollisionLen"]


def read_burst(img):
    """-> {key: (value, raw, div, site_va, dest_va, from_crs)}"""
    raws = shipped("burst_mng.ini", "burst_mng.crs")
    sect = set(img.strings(BURST_SECTION))
    out = {}
    for k in BURST_KEYS:
        if k not in raws:
            out[k] = None
            continue
        raw, from_crs = raws[k]
        got = None
        for sva in img.strings(k):
            for site in img.pushes(sva):
                o = img.off(site)
                if o is None or o < 7:
                    continue
                d = img.d
                # push <section> must follow the key's own push, or this is one
                # of the four other config files that also have a `Radius`.
                if d[o + 5] != 0x68:
                    continue
                if struct.unpack_from("<I", d, o + 6)[0] not in sect:
                    continue
                if d[o - 7] != 0x68 or d[o - 2] != 0x6A:
                    continue
                div = struct.unpack_from("<f", d, o - 6)[0]
                dest = None
                for j in range(o + 12, o + 40):
                    if d[j] == 0xD9 and d[j + 1] == 0x1D:      # fstp ds:
                        dest = struct.unpack_from("<I", d, j + 2)[0]
                        break
                    if d[j] == 0xA3:                            # mov ds:,eax
                        dest = struct.unpack_from("<I", d, j + 1)[0]
                        break
                if div:
                    got = (raw / div, raw, div, site, dest, from_crs)
                break
            if got:
                break
        out[k] = got
    return out


# ---------------------------------------------------------------------------
# THE ATTENTION MACHINE -- one shared watcher, four creatures, and its numbers
# are ARGUMENTS rather than config.
#
# 0x535720 allocates a watcher over two volumes and 0x5357e0 steps it, returning
# 0 nothing / 1 dropped / 2 tracking / 3 ATTACK. The two delays and the attack
# count are pushed as immediates at each creation site, so there is no key name
# to grep for and no .ini to read: the site is the record.
#
#     param_3  the dwell before the FIRST attack        (w[0x33])
#     param_4  the dwell between later ones            (w[0x34])
#     param_6  how many attacks before it drops the target and re-acquires
#
# param_7 (the guard's 45.0) is stored at w[0x61] and nothing in the four
# watcher functions reads it -- searched by scanning .text for every dword in
# w's own range, the same way the guard's four dead .ini keys were found.
WATCHERS = [
    ("guard", 0x51A670, "Guard_Man: visibilityVolume + guardVolume"),
    ("tower", 0x517F10, "the Btr's tower, roadcar.ini's Tower* keys"),
    ("dog", 0x518B80, "the Dog: attackVolume"),
    ("seagull", 0x513B40, "the Seagull, whose ground machine does not use it"),
]


def read_watcher(img, site, window=0x180):
    """The (first delay, repeat delay, max attacks) pushed at a 0x535720 call.

    Walks forward from the function entry collecting `push imm32` / `push imm8`
    in order and stops at the first call to 0x535720; the argument list is that
    run reversed, and cdecl puts param_1 last. Only the shape matters -- three
    of the seven arguments are stack pointers this does not have to resolve."""
    d, o = img.d, img.off(site)
    if o is None:
        return None
    pushes = []
    for j in range(o, o + window):
        if d[j] == 0xE8:
            tgt = img.va(j) + 5 + struct.unpack_from("<i", d, j + 1)[0]
            if tgt == 0x535720:
                args = list(reversed(pushes))
                if len(args) < 4:
                    return None
                # args[0] is param_1 only when every argument was an immediate;
                # here the pointers are pushed from registers and do not appear,
                # so the immediates arrive in argument order: delay1, delay2,
                # count, and (the guard only) the unread param_7.
                f1 = struct.unpack("<f", struct.pack("<I", args[0]))[0]
                f2 = struct.unpack("<f", struct.pack("<I", args[1]))[0]
                return (f1, f2, args[2])
            continue
        if d[j] == 0x68:
            pushes.append(struct.unpack_from("<I", d, j + 1)[0])
        elif d[j] == 0x6A:
            pushes.append(d[j + 1])
    return None


# ---------------------------------------------------------------------------
# THE REACTION -- what a person DOES about a car, and every threshold in it is
# an immediate in .text rather than a key in a config file.
#
# The whole feature was in the shipped data and these notes said the characters
# only react to being run over: people.sb gives every person a `Pendal` (a KICK)
# and a `Brosok` (a THROW) clip, the exe's own clip table at 0x573c68 is
# [Stand, Walk, Brosok, Pendal, Hurt] in animation-state order, and 0x510f40 is
# the decision. There is no slider for any of it, so each constant is emitted
# here as the FLOAT AT THE ADDRESS THE INSTRUCTION READS, with the instruction's
# own address beside it: a wrong site emits a wrong number instead of quietly
# agreeing with a comment, which is the rule the physics loaders already follow.
#
# (macro, .data address of the float, the instruction that reads it, note)
REACT = [
    ("CHR_REACT_DY",        0x554688, 0x510F89,
     "the car has to be within this much of the person's own height"),
    ("CHR_REACT_NEAR",      0x5544A0, 0x511098,
     "inside this it always reacts, whichever way it is facing"),
    ("CHR_REACT_THROW_P",   0x5544F0, 0x5110DA,
     "rand01 at or under this tries the THROW; above it, the kick"),
    ("CHR_REACT_CONE",      0x5547BC, 0x511163,
     "degrees off the person's forward, for the running kick"),
    ("CHR_REACT_FAR",       0x5543F0, 0x511174,
     "and how far away that one reaches"),
    ("CHR_REACT_CAR_SPEED", 0x554B08, 0x5111B0,
     "and how fast the car has to be coming -- 15.0 km/h exactly"),
    ("CHR_REACT_TURN_RATE", 0x000000, 0x5114CD,
     "degrees a second onto the car, pushed as an immediate"),
    ("CHR_REACT_WINDUP",    0x554384, 0x5114F9,
     "seconds facing the car before the swing starts"),
    ("CHR_REACT_KICK_LOST", 0x554518, 0x511555,
     "the car got away: abandon the kick past this"),
    ("CHR_REACT_KICK_T0",   0x554AAC, 0x5115C4,
     "seconds into the 3.0 s Pendal clip when the sole connects"),
    ("CHR_REACT_GRAB_LOST", 0x554B0C, 0x511290,
     "the same for the throw, which reaches further"),
    ("CHR_REACT_GRAB_T0",   0x554688, 0x5112EB,
     "seconds into the 4.7 s Brosok clip when the car leaves the ground"),
    ("CHR_REACT_LET_GO_T",  0x5543C8, 0x510DD7,
     "and when it leaves the hand"),
    ("CHR_REACT_LIFT_UP",   0x554B04, 0x510EC3,
     "how far above the person the clearance sphere sits"),
    ("CHR_REACT_KICK_AWAY", 0x5543F0, 0x51160E,
     "m/s away from the person, horizontally"),
    ("CHR_REACT_KICK_UP",   0x5547E4, 0x511605,
     "and m/s straight up -- the kick pops the car"),
    ("CHR_REACT_THROW_FWD", 0x5547E4, 0x510DFE,
     "m/s along the person's forward, past the car's own 6.91 top speed"),
    ("CHR_REACT_THROW_UP",  0x5543F0, 0x510E0A,
     "and m/s up -- the throw is the flatter of the two"),
    ("CHR_BULLET_AWAY",     0x000000, 0x5085C8,
     "a round that lands: m/s away from where it hit"),
    ("CHR_BULLET_UP",       0x000000, 0x5085C3,
     "and m/s up"),
    ("CHR_BULLET_LEAD",     0x554384, 0x52DC8E,
     "Offset is scaled by this and the target's speed IN KM/H, to lead it"),
    ("CHR_BULLET_REACH",    0x554824, 0x52DD0E,
     "the ray runs this many times the distance to the aim point"),
    ("CHR_BULLET_MIN",      0x554490, 0x52DD14,
     "or this far, whichever is longer"),
    ("CHR_GULL_STARTLE",    0x000000, 0x512D0E,
     "a car, a person or a dog inside this and the gull goes up"),
    ("CHR_GULL_ARRIVE",     0x000000, 0x512CE2,
     "and how near its wander target counts as arrived"),
    ("CHR_GUARD_MUZZLE",    0x554824, 0x51AA6C,
     "the burst is spawned this far above the guard's own pivot"),
]

# The immediates above that are PUSHED rather than loaded from .data: the value
# is the push's own operand, so the address column is the instruction and there
# is no separate float to read.
REACT_PUSHED = {
    0x5114CD: 4, 0x51160E: 4, 0x511605: 4,
    0x5085C8: 4, 0x5085C3: 4, 0x512D0E: 4,
}

# And one is neither: MSVC stores a float straight into the object with
# `mov dword ptr [reg+disp32], imm32`, so the operand is six bytes in.
REACT_STORED = {0x512CE2: 6}


def read_react(img):
    """-> [(macro, value, data_va, site_va, note)]

    A .data entry is read at its address; a pushed one is read out of the
    instruction itself, which is why REACT_PUSHED exists -- MSVC emits a float
    argument as `push imm32` and there is no constant pool entry to point at."""
    out = []
    for macro, dva, site, note in REACT:
        if site in REACT_STORED:
            o = img.off(site)
            v = (None if o is None
                 else struct.unpack_from("<f", img.d, o + REACT_STORED[site])[0])
            out.append((macro, v, 0, site, note))
            continue
        if site in REACT_PUSHED:
            o = img.off(site)
            if o is None or img.d[o] != 0x68:
                out.append((macro, None, dva, site, note))
                continue
            v = struct.unpack_from("<f", img.d, o + 1)[0]
            out.append((macro, v, 0, site, note))
            continue
        v = img.f32(dva)
        out.append((macro, v, dva, site, note))
    return out


# The one key whose loader is a clamped remap rather than a multiply, and the
# three immediates it uses. Written out with its address so a mutant has to
# break two copies -- the same rule the harnesses follow for a recovered value.
SPEEDFORW_SITE = 0x5120E0


def speedforw(raw):
    """0x51211e..0x512179: raw <= 1 -> 0, raw > 100 -> 2, else
    (raw - 1) * 0.020202020183205605."""
    if raw <= 1.0:
        return 0.0
    if raw > 100.0:
        return 2.0
    return (raw - 1.0) * 0.020202020183205605


def resolve_constants(img):
    """-> {group: {key: (value, raw, ops, dest_va, from_crs)}}

    `ops` is read_conversion's ordered op list, or None for the one key whose
    loader is a clamped remap rather than a chain of multiplies and adds."""
    out = {}
    for grp, ini, crs, keys in GROUPS:
        raws = shipped(ini, crs)
        vals = {}
        for k in keys:
            if k not in raws:
                vals[k] = None
                continue
            raw, from_crs = raws[k]
            if grp == "people" and k == "speedForw":
                vals[k] = (speedforw(raw), raw, None, SPEEDFORW_SITE, from_crs)
                continue
            conv = None
            for sva in img.strings(k):
                for site in img.pushes(sva):
                    c = read_conversion(img, site)
                    if not c:
                        continue
                    # Several groups share a key name (TimeAttack is dog's and
                    # guard's, ShiftY is the vulture's and the sole's). Keep the
                    # site whose destination global is nearest the group's other
                    # keys -- resolved below by taking the run with the most
                    # keys in one 0x100 window.
                    conv = conv or []
                    conv.append((site, c[0], c[1]))
            vals[k] = conv
        # pick the coherent set: the destination globals of one loader are
        # consecutive dwords, so the right candidate per key is the one whose
        # dest clusters with the others.
        cands = {k: v for k, v in vals.items() if isinstance(v, list) and v}
        best, bestn = None, -1
        for k, lst in cands.items():
            for c in lst:
                lo = c[2] - 0x80
                n = 0
                for k2, l2 in cands.items():
                    if any(lo <= c2[2] < lo + 0x100 for c2 in l2):
                        n += 1
                if n > bestn:
                    bestn, best = n, c[2] - 0x80
        got = {}
        for k in keys:
            v = vals.get(k)
            if v is None:
                got[k] = None
            elif isinstance(v, tuple):
                got[k] = v
            else:
                pick = None
                for c in v:
                    if best is not None and best <= c[2] < best + 0x100:
                        pick = c
                        break
                pick = pick or (v[0] if v else None)
                if pick is None:
                    got[k] = None
                else:
                    site, ops, dest = pick
                    raw, from_crs = raws[k]
                    got[k] = (apply_ops(raw, ops), raw, ops, dest, from_crs)
        out[grp] = got
    return out


# ------------------------------------------------------------ the .sb brief ---

def parse_script(txt):
    """The engine's own config syntax, as plain text inside 0x807E."""
    out = {}
    for m in re.finditer(r'(\w+)\s*\(\s*(string|int|float)\s*\)\s*=\s*'
                         r'(?:"([^"]*)"|([-\d.]+))', txt):
        key, ty, sv, nv = m.groups()
        out[key] = sv if ty == "string" else (
            int(float(nv)) if ty == "int" else float(nv))
    return out


def read_volume(b, body, end):
    """MOD_VOLUME 0x80D4: ten floats -- y low, y high, then four (x, z)."""
    for c, cnt, cb, ce in S.children(b, body, end):
        if c == 0x80D4 and ce - cb >= 40:
            v = struct.unpack_from("<10f", b, cb)
            return {"ylo": v[0], "yhi": v[1],
                    "quad": [(v[2], v[3]), (v[4], v[5]),
                             (v[6], v[7]), (v[8], v[9])]}
    return None


def read_placements(track):
    b = open(os.path.join(DB, track + ".sb"), "rb").read()
    reg, start = S.read_registry(b)
    rows = []

    def walk(p, end, trail):
        while p < end:
            cid, cend, cnt, body = S.rd_chunk(b, p)
            if cend <= p or cend > end:
                return
            if cid == 0x9200 and cnt is not None:
                ty = reg.get(cnt)
                nm = S.node_name(b, body, cend)
                if ty == "MOD_INSTANCE":
                    ref = None
                    for c2, n2, cb2, ce2 in S.children(b, body, cend):
                        if c2 == 0x4072:
                            ref = b[cb2:ce2].split(b"\0")[0].decode("latin-1")
                    if ref in MODELS:
                        trs = S.node_trs(b, body, cend) or (
                            (0, 0, 0), (1, 1, 1), (0, 0, 0))
                        row = {"track": track, "name": nm or ref, "model": ref,
                               "kind": KIND[ref], "pos": trs[0],
                               "scale": trs[1][0], "yaw": trs[2][1],
                               "markers": {}, "volumes": {}, "script": {},
                               "paths": []}
                        for c2, n2, cb2, ce2 in S.children(b, body, cend):
                            if c2 != 0x9200:
                                continue
                            sty = reg.get(n2)
                            snm = S.node_name(b, cb2, ce2) or ""
                            if sty == "MOD_MARKER":
                                mt = S.node_trs(b, cb2, ce2)
                                if mt:
                                    row["markers"][snm] = (mt[0], mt[2][1])
                            elif sty == "MOD_VOLUME":
                                v = read_volume(b, cb2, ce2)
                                if v:
                                    row["volumes"][snm] = v
                            elif sty == "MOD_SCRIPT":
                                for c3, n3, b3, e3 in S.children(b, cb2, ce2):
                                    if c3 == 0x807E and e3 - b3 > 4:
                                        row["script"].update(parse_script(
                                            b[b3 + 4:e3].decode("latin-1")))
                        rows.append(row)
                walk(body, cend, trail + [nm])
            p = cend

    walk(start, len(b), [])
    assign_paths(track, rows)
    return rows


def assign_paths(track, rows):
    """Which recorded path each instance follows.

    A person names its own file outright (`beach_1_man` -> that .dat). The road
    cars name a KEY (`rdc_beach2`) that matches no file, and the files that do
    ship are the pair `rdc_<track>_forw` / `_back` -- the two directions of the
    same road, byte-identical on urban_1 where the road is a loop and reversed
    ends on beach_2 where it is not. The vultures name nothing and country_2
    ships `vlt_country_2_1` / `_2`.

    HOW THE INSTANCES SHARE THEM IS THE PORT'S. Six road cars over two paths and
    four vultures over two: they are dealt alternately and given a start phase
    spread over the path's own duration, so a track does not run its whole
    convoy nose to tail. A Trailer takes the same path and phase as the Truck of
    the same index, lagged -- the shipped `body`/`box` keys pair Trailer<n> with
    Truck<n> and they are drawn as a unit."""
    def have(nm):
        return (os.path.exists(os.path.join(PROFILES, nm + ".dat")) or
                os.path.exists(os.path.join(PROFILES, nm + ".gpf")))

    road = [n for n in ("rdc_%s_forw" % track, "rdc_%s_back" % track)
            if have(n)]
    vult = [n for n in ("vlt_%s_1" % track, "vlt_%s_2" % track) if have(n)]

    nroad = nvult = 0
    for r in rows:
        nm = r["script"].get("name")
        if r["kind"] == "WALKER":
            if nm and have(nm):
                r["paths"] = [nm]
                r["path_phase"] = 0.0
        elif r["model"] in ("Truck", "Trailer", "Hammer", "Btr"):
            if road:
                r["paths"] = [road[nroad % len(road)]]
                r["path_phase"] = (nroad // len(road)) * 0.37
                nroad += 1
        elif r["model"] == "Vulture":
            if vult:
                r["paths"] = [vult[nvult % len(vult)]]
                r["path_phase"] = (nvult // len(vult)) * 0.5
                nvult += 1
    for r in rows:
        r.setdefault("path_phase", 0.0)


# ------------------------------------------------------------------ output ---

def cname(s):
    return re.sub(r"[^A-Za-z0-9_]", "_", s)


def fl(v):
    """A C float literal.

    Two things: "%gf" % 45 is `45f`, which does not compile; and the value is
    rounded to float32 FIRST, because the engine does its multiply in float32
    and 300 * 0.01f is exactly 3.0f there while the same product in double is
    2.9999999329. Printing the double would put a difference into the header
    that the original does not have."""
    v = struct.unpack("<f", struct.pack("<f", float(v)))[0]
    s = "%.9g" % v
    if "." not in s and "e" not in s and "E" not in s and "inf" not in s:
        s += ".0"
    return s + "f"


def emit(f, consts, placements, burst, watch, react):
    f.write("""/*
 * char_data.h -- the 59 people, animals and road cars the tracks place, and
 * every behaviour constant the engine loads for them.
 *
 * GENERATED by rccars_re/gen_char_data.py -- do not edit by hand.
 *
 * The placements, their markers, their volumes and their script keys come from
 * each track's own .sb; the constants come from the loaders in RCCars.exe,
 * whose addresses are quoted per group below. Geometry, animation and the
 * recorded paths are NOT here -- they are packed per track into <track>.chr by
 * pack_chars.py, and char.c binds the two together by MODEL NAME.
 */
#ifndef CHAR_DATA_H
#define CHAR_DATA_H

""")
    for grp, ini, crs, keys in GROUPS:
        got = consts[grp]
        f.write("/* %s -- %s%s */\n"
                % (grp, ini, "" if not os.path.exists(os.path.join(SETTINGS, crs))
                   else " + " + crs))
        for k in keys:
            v = got.get(k)
            if not v:
                f.write("/* %-16s UNRESOLVED */\n" % k)
                continue
            val, raw, ops, dest, from_crs = v
            if ops is None:
                how = "clamped remap at 0x%06x" % dest
            else:
                how = "%s -> 0x%06x" % (ops_formula(ops), dest)
            f.write("#define CHR_%s_%-18s %-14s /* raw %d %s, %s */\n"
                    % (grp.upper(), k.upper(), fl(val), raw,
                       "crs" if from_crs else "ini", how))
        f.write("\n")

    # --------------------------------------------------- the burst manager ---
    f.write("""/*
 * BURST_MNG -- the gunfire. burst_mng.ini + burst_mng.crs, read through the
 * FLOAT getter at 0x532410 (value = raw / div, and it does NOT clamp), so
 * `Radius` ships at 200 against its own .ini range of 0..100 and means 2.0 m.
 *
 * A guard whose watcher says ATTACK spawns a 'BRMN' object at 0x51aa81, one
 * metre and a half above its own pivot, and that object fires NShoots rounds
 * TimeShoot apart. Each round is a ray from the muzzle to the target car's
 * position led by `Offset` x its speed IN KM/H x 0.5, plus a sweep that walks
 * from +Radius to -Radius along one random unit vector across the burst -- so a
 * burst sprays and only the middle of it is on the car.
 */
""")
    for k in BURST_KEYS:
        v = burst.get(k)
        if not v:
            f.write("/* %-16s UNRESOLVED */\n" % k)
            continue
        val, raw, div, site, dest, from_crs = v
        f.write("#define CHR_BURST_%-14s %-14s /* raw %d %s, / %g at 0x%06x -> 0x%06x */\n"
                % (k.upper(), fl(val), raw, "crs" if from_crs else "ini",
                   div, site, dest or 0))
    f.write("""
/* The whole burst lasts (NShoots - 1) * TimeShoot, derived at 0x52dff0. */
#define CHR_BURST_LEN            %s

/* AND `CollisionLen` IS A DEAD KEY. 0x52dfd4 stores it at 0x165be58 and nothing
 * in the image reads that dword -- searched by scanning every dword in .text
 * inside 0x165be3c..0x165be60, which finds all six of its neighbours' readers.
 * The ray's own length is 1.5x the distance to the aim point or 10 m, whichever
 * is longer, and both of those are immediates in 0x52dc50. Same shape as the
 * guard's four .ini keys below: shipped, converted, and never consumed.
 */

""" % fl((burst["NShoots"][0] - 1) * burst["TimeShoot"][0]
         if burst.get("NShoots") and burst.get("TimeShoot") else 0.0))

    # ------------------------------------------------ the attention machine ---
    f.write("""/*
 * THE ATTENTION MACHINE -- 0x535720 builds it, 0x5357e0 steps it, and four
 * creatures share it. It holds a target, two volumes and a dwell clock:
 *
 *   nothing is in the ACQUIRE volume            -> 0, the creature is idle
 *   the target has left the HOLD volume         -> 1, dropped
 *   the dwell has not run out yet               -> 2, tracking (turn to face)
 *   it has                                      -> 3, ATTACK
 *
 * Acquire takes the NEAREST '$CAR' in the acquire volume with the distance of
 * the last car it attacked DOUBLED, so it works round a field rather than
 * fixating. `first` is the dwell before the first attack and `again` the dwell
 * between later ones; after `count` attacks it drops the target and re-acquires.
 * Every one of those is an immediate at the creation site, quoted per row.
 */
typedef struct {
    float first, again;
    int   count;
} chr_watch_t;

""")
    for name, site, note in WATCHERS:
        w = watch.get(name)
        if not w:
            f.write("/* %-8s UNRESOLVED */\n" % name)
            continue
        f.write("#define CHR_WATCH_%-10s { %-14s %-14s %d } /* 0x%06x -- %s */\n"
                % (name.upper(), fl(w[0]) + ",", fl(w[1]) + ",", w[2], site, note))
    f.write("\n")

    # ------------------------------------------------------- the reaction ---
    f.write("""/*
 * THE REACTION -- a person KICKS a car, or picks it up and THROWS it, and a
 * guard shoots at it. All of it was in the shipped data while these notes said
 * the layer only noticed being run over: people.sb gives the Man, the Woman and
 * the RepairMan a `Pendal` (Russian for a kick) and a `Brosok` (a throw), the
 * Guard a `kick` and a `shoot`, and the exe's own clip tables put them in
 * animation-state order --
 *
 *   people  0x573c68  [ Stand, Walk, Brosok, Pendal, Hurt ]
 *   guard   0x574bec  [ kick, shoot, stand ]
 *   seagull 0x573e84  [ start_flight, flight_idle, look_around, walk, flight ]
 *
 * -- selected from a flag word by 0x50f780 (0x100 throw, 0x20 kick, 0x10 hurt,
 * 0x3 walk, else stand), 0x51ae10 (0x10 kick, 0x20 shoot, else stand) and
 * 0x5137c0 (1 look_around, 2 walk, 4 flight, else start_flight). The decision
 * itself is 0x510f40, the kick 0x511470, the grab 0x5111f0 and the carry and
 * release 0x510b90.
 *
 * None of it is sliderable, so every threshold below is emitted as the float at
 * the address the instruction reads, with the instruction beside it -- a wrong
 * site emits a wrong number rather than agreeing with a comment.
 */
""")
    for macro, val, dva, site, note in react:
        if val is None:
            f.write("/* %-22s UNRESOLVED at 0x%06x */\n" % (macro, site))
            continue
        where = ("imm at 0x%06x" % site if not dva
                 else "0x%06x read at 0x%06x" % (dva, site))
        f.write("#define %-22s %-14s /* %s\n%*s   %s */\n"
                % (macro, fl(val), where, 24, "", note))
    f.write("\n")

    # ------------------------------------------------ the collision proxy ---
    f.write("""/*
 * THE COLLISION PROXY -- one per model, and the characters ARE solid.
 *
 * The retail engine registers a sphere-set provider per object 4CC and a
 * resolver per PAIR of them, both at 0x534d00:
 *
 *   proxies (0x533130)  $CAR STON PEOP CRAB SPDR RDCR DOG$ GUAM
 *   pairs   (0x533170)  $CAR/$CAR $CAR/STON STON/STON $CAR/PEOP $CAR/GUAM
 *                       $CAR/CRAB $CAR/SPDR $CAR/RDCR STON/RDCR $CAR/DOG$
 *
 * So the car collides with the people, the guards, the crabs, the spiders, the
 * road cars and the dogs. The SEAGULL AND THE VULTURE ARE IN NEITHER REGISTRY --
 * they fly -- and they are the two models below with no spheres.
 *
 * A provider writes (x, y, z, radius) quads, the same shape rb_gather_spheres
 * writes for the car, and every resolver ends at rb_coll_resolve with the car as
 * the ONLY body: the car is pushed out and the character does not move. It is
 * the static side, exactly as the world is.
 *
 * The whole layer is stepped once per FRAME with dt (0x4fc685) over the objects
 * within CHR_SOLID_RANGE of the car, so there is no carSubstepContact bisection
 * behind it and no depenetration in it. That is the original.
 */
typedef enum {
    CHR_PX_NONE,     /* the flyers: in neither of the engine's registries */
    CHR_PX_COLUMN,   /* n spheres stacked in WORLD +Y from the instance position */
    CHR_PX_BONES,    /* n spheres at named joints, at their posed world positions */
    CHR_PX_HULL      /* THE PORT'S -- see the road cars below */
} chr_proxy_kind_t;

#define CHR_PROXY_MAX     %d    /* RDCR's slot count, the largest of the six */
#define CHR_PROXY_BONES   5    /* the Dog's, the only BONES proxy */
#define CHR_SOLID_RANGE   %s  /* 0x41200000 at 0x53343b */

typedef struct {
    const char *model;
    unsigned char kind;                    /* chr_proxy_kind_t */
    unsigned char n;                       /* spheres, 0 for the flyers */
    float radius;                          /* every sphere, metres */
    float shift_y;                         /* the first sphere off the pivot */
    float step_y;                          /* between spheres, world up */
    const char *bone[CHR_PROXY_BONES];     /* BONES only */
} chr_proxy_t;

static const chr_proxy_t CHR_PROXY[] = {
""" % (PROXY_MAX, fl(PROXY_SOLID_RANGE)))
    for mdl in MODELS:
        kind, n, r, sh, st, bones, prov = PROXY[mdl]
        if mdl == "Spider":
            rs = "CHR_SPIDER_CDTRAD"
            shs = "CHR_SPIDER_CDTSHIFTY"
        else:
            rs = fl(r or 0.0)
            shs = fl(sh or 0.0)
        bl = ", ".join(['"%s"' % b for b in (bones or [])]
                       + ["NULL"] * (5 - len(bones or [])))
        f.write("    { %-12s CHR_PX_%-9s %d, %-20s %-22s %-14s { %s } },"
                "  /* %s */\n"
                % ('"%s",' % mdl, kind + ",", n, rs + ",", shs + ",",
                   fl(st) + ",", bl, prov))
    f.write("};\n\n#define CHR_N_PROXY  %d\n\n" % len(MODELS))

    # -------------------------------------------- each model's own sound ---
    #
    # SIX OF THE THIRTEEN CARRY A MOD_SNDCHANNEL and the other seven make no
    # noise. Read out of the model's own subtree by sbchar.read_sound -- the same
    # structure a prop's knock comes out of -- rather than assigned by kind: the
    # Crab and the Spider have no channel at all, and "everything that is not a
    # Woman sounds like a man" is what made a squashed crab cry out in a man's
    # voice.
    sounds = []
    for mdl in MODELS:
        try:
            snd = sbchar.read_model(mdl)["sound"]
        except Exception:
            snd = None
        sounds.append((mdl, snd))
    f.write("""/*
 * EACH MODEL'S OWN SOUND, out of its own MOD_SNDCHANNEL -- six of the thirteen
 * carry one and the other seven are silent. rmin > rmax on every one of them, as
 * the props' 30/8 is, and both are passed through as authored.
 *
 * This is data and not a rule: the Crab and the Spider have no channel, so a car
 * that runs one over raises no cue of its own. char.c hands the wav name up and
 * the caller decides what it sounds like, the same split menu.c keeps from
 * sfx.c.
 */
typedef struct {
    const char *model;
    const char *wav;                /* NULL: the model has no channel */
    float rmin, rmax;
} chr_snd_t;

static const chr_snd_t CHR_SND[] = {
""")
    for mdl, snd in sounds:
        if snd:
            wav, rmin, rmax = snd
            f.write('    { %-12s %-18s %-8s %-8s },\n'
                    % ('"%s",' % mdl, '"%s",' % wav, fl(rmin) + ",", fl(rmax)))
        else:
            f.write('    { %-12s %-18s %-8s %-8s },  /* no MOD_SNDCHANNEL */\n'
                    % ('"%s",' % mdl, "NULL,", "0.0f,", "0.0f"))
    f.write("};\n\n#define CHR_N_SND  %d\n\n" % len(MODELS))

    f.write("""/* Behaviour families. Each names what the instance's own brief supports. */
typedef enum {
    CHR_KIND_DOG,      /* a vision cone, a chase, an attackVolume, a resetPlace */
    CHR_KIND_SEAGULL,  /* walks its inhabitVolume, takes off along flightDir */
    CHR_KIND_BURROW,   /* Crab and Spider: runs between two named holes */
    CHR_KIND_WALKER,   /* Man, Woman, RepairMan: replays a recorded walk */
    CHR_KIND_GUARD,    /* stands, turns to follow the car inside guardVolume */
    CHR_KIND_PATH,     /* road cars and vultures: replays a .gpf */
    CHR_KIND_COUNT
} chr_kind_t;

/* A named point out of the instance's own subtree. Absent markers have
   have_flag clear -- a Dog with no resetPlace goes back to where it spawned. */
typedef struct {
    float x, y, z, yaw;
} chr_marker_t;

/* MOD_VOLUME: a y range and four (x, z) corners in traversal order. Convex, and
   every shipped one is a parallelogram, but chr_in_volume tests it as a general
   convex quad so nothing depends on that. */
typedef struct {
    float ylo, yhi;
    float qx[4], qz[4];
} chr_volume_t;

#define CHR_MARK_NONE   0u
#define CHR_MARK_A      1u   /* flightDir / seahole1 / hole1 / resetPlace */
#define CHR_MARK_B      2u   /* landhole1 / hole2 */
#define CHR_VOL_SIGHT   4u   /* visibilityVolume */
#define CHR_VOL_HOME    8u   /* inhabitVolume / guardVolume */
#define CHR_VOL_ATTACK 16u   /* attackVolume */

typedef struct {
    const char *name;       /* the instance's own node name -- documentation */
    const char *model;      /* the MOD_MODEL in the .chr */
    unsigned char kind;     /* chr_kind_t */
    unsigned char variant;  /* clother / body -- which texture set to wear */
    unsigned int  have;     /* CHR_MARK_* | CHR_VOL_* */
    float x, y, z;          /* the authored pivot; char_init drops it to ground */
    float yaw;              /* degrees, unwrapped in the file */
    float scale;
    const char *path;       /* recorded path name in the .chr, or NULL */
    float phase;            /* 0..1 of the path's duration -- THE PORT'S */
    chr_marker_t mark_a, mark_b;
    chr_volume_t sight, home, attack;
} chr_place_t;

""")
    total = 0
    for t in TRACKS:
        rows = placements[t]
        total += len(rows)
        f.write("static const chr_place_t CHR_PLACE_%s[] = {\n" % t.upper())
        for r in rows:
            have = []
            ma = mb = None
            for key, bit in (("flightDir", "A"), ("seahole1", "A"),
                             ("hole1", "A"), ("resetPlace", "A"),
                             ("landhole1", "B"), ("hole2", "B")):
                if key in r["markers"]:
                    if bit == "A" and ma is None:
                        ma = r["markers"][key]
                        have.append("CHR_MARK_A")
                    elif bit == "B" and mb is None:
                        mb = r["markers"][key]
                        have.append("CHR_MARK_B")
            sight = r["volumes"].get("visibilityVolume")
            home = r["volumes"].get("inhabitVolume") or r["volumes"].get("guardVolume")
            att = r["volumes"].get("attackVolume")
            if sight:
                have.append("CHR_VOL_SIGHT")
            if home:
                have.append("CHR_VOL_HOME")
            if att:
                have.append("CHR_VOL_ATTACK")

            def mk(m):
                if not m:
                    return "{0,0,0,0}"
                (x, y, z), yaw = m
                return "{%s,%s,%s,%s}" % (fl(x), fl(y), fl(z), fl(yaw))

            def vol(v):
                if not v:
                    return "{0,0,{0,0,0,0},{0,0,0,0}}"
                return "{%s,%s,{%s},{%s}}" % (
                    fl(v["ylo"]), fl(v["yhi"]),
                    ",".join(fl(q[0]) for q in v["quad"]),
                    ",".join(fl(q[1]) for q in v["quad"]))

            var = r["script"].get("clother")
            if var is None:
                var = r["script"].get("body")
            var = int(var or 0)
            path = r["paths"][0] if r["paths"] else None
            f.write('    { "%s", "%s", CHR_KIND_%s, %d, %s,\n'
                    '      %s, %s, %s, %s, %s, %s, %s,\n'
                    '      %s, %s,\n      %s,\n      %s,\n      %s },\n'
                    % (r["name"], r["model"], r["kind"], var,
                       " | ".join(have) or "CHR_MARK_NONE",
                       fl(r["pos"][0]), fl(r["pos"][1]), fl(r["pos"][2]),
                       fl(r["yaw"]), fl(r["scale"]),
                       '"%s"' % path if path else "0",
                       fl(r["path_phase"]), mk(ma), mk(mb),
                       vol(sight), vol(home), vol(att)))
        f.write("};\n\n")

    f.write("typedef struct {\n"
            "    const char *track;\n"
            "    const chr_place_t *place;\n"
            "    unsigned int n;\n"
            "} chr_track_t;\n\n")
    f.write("static const chr_track_t CHR_TRACKS[] = {\n")
    for t in TRACKS:
        n = len(placements[t])
        f.write('    { "%s", CHR_PLACE_%s, %d },\n' % (t, t.upper(), n))
    f.write("};\n")
    f.write("#define CHR_N_TRACKS %d\n" % len(TRACKS))
    f.write("#define CHR_N_PLACEMENTS %d\n\n" % total)
    f.write("#endif /* CHAR_DATA_H */\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out", nargs="?")
    ap.add_argument("--check", action="store_true")
    a = ap.parse_args()

    img = Image(EXE)
    consts = resolve_constants(img)
    burst = read_burst(img)
    watch = {n: read_watcher(img, site) for n, site, _ in WATCHERS}
    react = read_react(img)
    placements = {t: read_placements(t) for t in TRACKS}

    if a.check or not a.out:
        for grp, ini, crs, keys in GROUPS:
            print("== %s (%s)" % (grp, ini))
            for k in keys:
                v = consts[grp].get(k)
                if not v:
                    print("   %-16s UNRESOLVED" % k)
                    continue
                val, raw, ops, dest, fc = v
                print("   %-16s = %-12g raw %-5d %-4s  %s"
                      % (k, val, raw, "crs" if fc else "ini",
                         "clamped remap" if ops is None else
                         "%s -> %s" % (ops_formula(ops), hex(dest))))
        print("== burst_mng (BURST_MNG, the float getter at 0x532410)")
        for k in BURST_KEYS:
            v = burst.get(k)
            print("   %-16s %s" % (k, "UNRESOLVED" if not v else
                  "= %-10g raw %-5d %-4s / %g at %s -> %s"
                  % (v[0], v[1], "crs" if v[5] else "ini", v[2], hex(v[3]),
                     hex(v[4] or 0))))
        print("== watchers (0x535720)")
        for nm, site, note in WATCHERS:
            print("   %-8s %-24s %s" % (nm, watch.get(nm), note))
        print("== the reaction (immediates)")
        for macro, val, dva, site, note in react:
            print("   %-22s %-12s %s @ %s"
                  % (macro, "UNRESOLVED" if val is None else "%g" % val,
                     hex(dva), hex(site)))
        n = 0
        for t in TRACKS:
            rows = placements[t]
            n += len(rows)
            print("== %-10s %d characters" % (t, len(rows)))
            for r in rows:
                print("   %-12s %-22s pos=(%.2f %.2f %.2f) yaw=%.1f scale=%.2f "
                      "mark=%s vol=%s script=%s path=%s"
                      % (r["model"], r["name"], r["pos"][0], r["pos"][1],
                         r["pos"][2], r["yaw"], r["scale"],
                         sorted(r["markers"]), sorted(r["volumes"]),
                         r["script"], r["paths"]))
        print("TOTAL %d characters" % n)
        if not a.out:
            return

    with open(a.out, "w") as f:
        emit(f, consts, placements, burst, watch, react)
    print("wrote %s" % a.out)


if __name__ == "__main__":
    main()
