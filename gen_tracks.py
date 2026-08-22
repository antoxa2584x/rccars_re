#!/usr/bin/env python3
"""
Emit `tracks.h` for the Vita app: one row per track with its race start.

Every track .sb carries a MOD_INSTANCE named `Player` under a `Players` folder.
Its 0x540B is the grid position and heading, and its 0x4072 string names the car
that starts there ("Car1" = Overkill, "Car2" = Buggy, "Car3" = Hummer). That is
the game's own race start, so the port no longer has to invent a spawn.

THE YAW GOES ACROSS UNCHANGED, and that is worth stating because three separate
bugs in this port came from assuming a convention instead of checking one.
sb2obj composes node rotations as column-vector matrices with

    rotY(phi) = ((cos, 0, sin), (0, 1, 0), (-sin, 0, cos))

so a node's local +Z lands on world (sin phi, 0, cos phi). rbcar_init builds its
pose quaternion as (cos(yaw/2), 0, sin(yaw/2), 0), and rb_quat_to_matrix turns
that into a row-major row-vector matrix whose row 2 -- the body's +Z, which is
the car's nose -- is also (sin yaw, 0, cos yaw). Same mapping, so
`yaw_deg = the .sb Euler Y value` with no negation and no half turn.

The marker's own Y is kept only as a ceiling for the runtime ground probe: it
tells col_ground_at which surface is meant when geometry is stacked (a pier over
sand), without pinning the car to a height the collision grid disagrees with.

Usage: python3 gen_tracks.py <RCCarsDB dir> <out tracks.h> [--assets DIR]
"""

import argparse
import os
import struct
import sys

import sb2obj

# Display order = the game's own progression: four beaches, four country, two urban.
TRACKS = [
    ("beach_1",   "Beach 1"),
    ("beach_2",   "Beach 2"),
    ("beach_3",   "Beach 3"),
    ("beach_4",   "Beach 4"),
    ("country_1", "Country 1"),
    ("country_2", "Country 2"),
    ("country_3", "Country 3"),
    ("country_4", "Country 4"),
    ("urban_1",   "Urban 1"),
    ("urban_2",   "Urban 2"),
]

CAR_NAMES = {"Car1": 0, "Car2": 1, "Car3": 2}


def node_name(b, body, end):
    for cid, cnt, cb, ce in sb2obj.children(b, body, end):
        if cid == 0x4003:
            return b[cb:ce].split(b"\0")[0].decode("latin-1")
    return None


def node_trs(b, body, end):
    """The 0x540B triple: (translation, scale, rotation-in-degrees)."""
    for cid, cnt, cb, ce in sb2obj.children(b, body, end):
        if cid == 0x540B and ce - cb >= 36:
            f = struct.unpack_from("<9f", b, cb)
            return (f[0:3], f[3:6], f[6:9])
    return None


def node_str(b, body, end, want):
    for cid, cnt, cb, ce in sb2obj.children(b, body, end):
        if cid == want:
            return b[cb:ce].split(b"\0")[0].decode("latin-1")
    return None


def find_player(path):
    """Return (x, y, z, yaw_deg, car_index) for the track's race start."""
    b = open(path, "rb").read()
    reg, start = sb2obj.read_registry(b)
    found = []

    def walk(pos, end, depth, pM, pT):
        while pos < end:
            cid, cend, cnt, body = sb2obj.rd_chunk(b, pos)
            if cend <= pos or cend > end:
                return
            if cid == 0x9200:
                t = reg.get(cnt, "?")
                nm = node_name(b, body, cend)
                # compose this node's local transform onto its parent's
                M, T = sb2obj.compose(pM, pT, node_trs(b, body, cend))
                if t == "MOD_INSTANCE" and nm == "Player":
                    trs = node_trs(b, body, cend)
                    car = node_str(b, body, cend, 0x4072) or "Car1"
                    # Parents are folders and have been composed into T; the
                    # heading is the node's own Euler Y (parents are unrotated,
                    # which is asserted below).
                    yaw = trs[2][1] if trs else 0.0
                    found.append((T, yaw, car, pM))
                if depth < 12:
                    walk(body, cend, depth + 1, M, T)
            pos = cend

    walk(start, len(b), 0, sb2obj.IDENT3, (0.0, 0.0, 0.0))
    if not found:
        return None
    T, yaw, car, pM = found[0]

    # If a parent ever rotates, the plain Euler Y is not the world heading and
    # this needs a proper matrix-to-yaw. Catch it rather than ship a wrong angle.
    off = max(abs(pM[i][j] - (1.0 if i == j else 0.0))
              for i in range(3) for j in range(3))
    if off > 1e-4:
        raise SystemExit(f"{os.path.basename(path)}: Player has a rotated parent "
                         f"(off-identity by {off:.4f}) -- yaw needs composing")

    yaw = ((yaw % 360.0) + 360.0) % 360.0
    if yaw > 180.0:
        yaw -= 360.0
    return (T[0], T[1], T[2], yaw, CAR_NAMES.get(car, 0), car, len(found))


def find_ambient(path):
    """The track's ambient sound bed, from its own MOD_SNDCHANNEL node.

    Every shipped track carries exactly one non-positional sound channel, named
    `ground_noise_snd`, holding a MOD_SOUND whose 0x4003 names the wav: noise1
    on the beaches, noise2/3/4 inland. Its 0x50E0/0x50E1 radii are both 0, which
    is what marks it global -- the only other channels in any track (country_4's
    two transformers, urban_1's one, urban_2's radar) carry rmax 3.0.

    Those positional ones are NOT returned. Their parent is a MOD_VOLUME whose
    extent is not in the sound subtree, so there is no position to play them at;
    see sfx.h.
    """
    b = open(path, "rb").read()
    reg, start = sb2obj.read_registry(b)
    found = []

    def walk(pos, end, depth):
        while pos < end:
            cid, cend, cnt, body = sb2obj.rd_chunk(b, pos)
            if cend <= pos or cend > end:
                return
            if cid == 0x9200:
                if reg.get(cnt) == "MOD_SNDCHANNEL":
                    rmax = 0.0
                    snd = None
                    for c2, n2, b2, e2 in sb2obj.children(b, body, cend):
                        if c2 == 0x50E1 and e2 - b2 >= 4:
                            rmax = struct.unpack_from("<f", b, b2)[0]
                        elif c2 == 0x9200 and reg.get(n2) == "MOD_SOUND":
                            snd = node_name(b, b2, e2)
                    if snd and rmax == 0.0:
                        found.append(snd)
                if depth < 12:
                    walk(body, cend, depth + 1)
            pos = cend

    walk(start, len(b), 0)
    return found[0] if found else ""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("db", help="RCCarsDB directory")
    ap.add_argument("out", help="output tracks.h")
    ap.add_argument("--assets", default=None,
                    help="only emit tracks whose .vsc and .col exist here")
    a = ap.parse_args()

    rows = []
    for base, title in TRACKS:
        sb = os.path.join(a.db, base + ".sb")
        if not os.path.exists(sb):
            print(f"  {base}: no .sb, skipped")
            continue
        if a.assets:
            vsc = os.path.join(a.assets, base + ".vsc")
            col = os.path.join(a.assets, base + ".col")
            if not (os.path.exists(vsc) and os.path.exists(col)):
                print(f"  {base}: not packed, skipped")
                continue
        r = find_player(sb)
        if not r:
            print(f"  {base}: NO Player instance, skipped")
            continue
        x, y, z, yaw, car, carname, n = r
        amb = find_ambient(sb)
        rows.append((base, title, x, y, z, yaw, car, amb))
        extra = f"  ({n} Player nodes!)" if n != 1 else ""
        print(f"  {base:10s} start ({x:8.3f},{y:7.3f},{z:8.3f})  yaw {yaw:8.2f}  "
              f"{carname}  ambient={amb or '-'}{extra}")

    if not rows:
        raise SystemExit("no tracks emitted")

    with open(a.out, "w") as f:
        f.write("/* Generated by rccars_re/gen_tracks.py -- do not edit.\n"
                " *\n"
                " * The race start of each track, read from its own Players/Player\n"
                " * instance. `yaw` is the .sb Euler Y and goes straight into\n"
                " * rbcar_init: both conventions map local +Z to (sin y, 0, cos y).\n"
                " * `y` is the marker height, used only as the ceiling for the\n"
                " * runtime ground probe so a pier is not mistaken for the sand\n"
                " * under it.\n"
                " *\n"
                " * `ambient` is the track's own sound bed, read from its single\n"
                " * global MOD_SNDCHANNEL (`ground_noise_snd`). sfx.c loops it\n"
                " * non-positionally, which is what its zero radii mean. */\n"
                "#ifndef TRACKS_H\n#define TRACKS_H\n\n"
                "typedef struct {\n"
                "    const char *name;      /* menu label */\n"
                "    const char *base;      /* assets/<base>.vsc and .col */\n"
                "    float x, y, z;         /* race start, world space */\n"
                "    float yaw;             /* degrees, straight into rbcar_init */\n"
                "    int   car;             /* 0 Overkill, 1 Buggy, 2 Hummer */\n"
                "    const char *ambient;   /* bank sound for the ambient bed */\n"
                "} track_info;\n\n"
                f"#define N_TRACKS {len(rows)}\n\n"
                "static const track_info TRACKS[N_TRACKS] = {\n")
        for base, title, x, y, z, yaw, car, amb in rows:
            f.write(f'    {{ "{title}", "{base}", '
                    f"{x:.5f}f, {y:.5f}f, {z:.5f}f, {yaw:.5f}f, {car}, "
                    f'"{amb}" }},\n')
        f.write("};\n\n#endif\n")
    print(f"wrote {a.out} with {len(rows)} tracks")


if __name__ == "__main__":
    main()
