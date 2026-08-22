#!/usr/bin/env python3
"""
Extract RC Cars vehicle physics parameters.

Two sources, per car (1 = Overkill, 2 = Buggy, 3 = Hummer):

  Settings/<group><n>.ini   slider DEFINITIONS -- key, integer range, default,
                            and an optional "FValues lo hi" giving the float
                            range the slider maps into
  Settings/<group><n>.crs   the same keys as TIOX chunks with the RUNTIME value

The .crs strings are obfuscated with XOR 0x25 (e.g. 'NuJV' -> 'kPos').
Chunks inside each 0x0220 entry:
    0x4021 name   0x3023 min   0x3024 max   0x3025 value

Where the .ini supplies FValues, the physical quantity is
    lo + (value - min) / (max - min) * (hi - lo)
otherwise the raw integer is the value.

Usage: python3 dump_physics.py [--dir SETTINGS] [--car N]
"""

import argparse
import os
import re
import struct

try:
    from scales_table import SCALES
except ImportError:
    SCALES = {}

GROUPS = ["engine", "tires", "resistance", "springs", "steering", "cdt"]
CARS = {1: "Overkill", 2: "Buggy", 3: "Hummer"}
XOR = 0x25


def rd_chunk(b, p):
    cid = struct.unpack_from("<H", b, p)[0]
    end = struct.unpack_from("<I", b, p + 2)[0]
    n = p + 6
    cnt = None
    if (cid & 0x400) or (cid & 0xF000) == 0x9000:
        cnt = struct.unpack_from("<I", b, n)[0]
        n += 4
    return cid, end, cnt, n


def deobf(raw):
    return "".join(chr(c ^ XOR) for c in raw)


def read_crs(path):
    """-> {name: (min, max, value)}"""
    b = open(path, "rb").read()
    cid, end, cnt, body = rd_chunk(b, 0)
    out = {}
    p = body + 4                      # skip the float version, as in the pack
    while p < end:
        c, ce, n, bd = rd_chunk(b, p)
        if ce <= p or ce > len(b):
            break
        if c == 0x0220:
            name = None
            vals = {}
            q = bd
            while q < ce:
                c2, ce2, n2, bd2 = rd_chunk(b, q)
                if ce2 <= q or ce2 > len(b):
                    break
                if c2 == 0x4021:
                    name = deobf(b[bd2:ce2].split(b"\0")[0])
                elif c2 in (0x3023, 0x3024, 0x3025) and ce2 - bd2 >= 4:
                    vals[c2] = struct.unpack_from("<i", b, bd2)[0]
                q = ce2
            if name and 0x3025 in vals:
                out[name] = (vals.get(0x3023, 0), vals.get(0x3024, 0), vals[0x3025])
        p = ce
    return out


def read_ini(path):
    """-> {key: (flo, fhi) or None}"""
    out, key = {}, None
    if not os.path.exists(path):
        return out
    for line in open(path, "r", errors="replace"):
        line = line.strip()
        m = re.match(r'Key\s+"([^"]+)"', line)
        if m:
            key = m.group(1)
            out.setdefault(key, None)
        elif key and line.startswith("FValues"):
            p = line.split()
            if len(p) >= 3:
                out[key] = (float(p[1]), float(p[2]))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", default="/mnt/c/tools/rccars_extracted/Settings")
    ap.add_argument("--car", type=int, default=0)
    a = ap.parse_args()

    for n, label in CARS.items():
        if a.car and n != a.car:
            continue
        print(f"\n================ Car {n}: {label} ================")
        for g in GROUPS:
            crs = os.path.join(a.dir, f"{g}{n}.crs")
            ini = os.path.join(a.dir, f"{g}{n}.ini")
            if not os.path.exists(crs):
                continue
            params = read_crs(crs)
            franges = read_ini(ini)
            if not params:
                continue
            print(f"\n  [{g}]")
            for name, (lo, hi, val) in params.items():
                # prefer the conversion recovered from the loader code; it is
                # what the game actually applies. FValues in the .ini only
                # covers some keys and is a UI hint.
                sc = SCALES.get(name)
                if sc:
                    b, k, o = sc
                    print(f"    {name:<28} {val:>6}  ({lo}..{hi})  = {(val - b) * k + o:.5f}")
                    continue
                fr = franges.get(name)
                if fr and hi != lo:
                    phys = fr[0] + (val - lo) / (hi - lo) * (fr[1] - fr[0])
                    print(f"    {name:<28} {val:>6}  ({lo}..{hi})  ~ {phys:.5f}  (ini FValues)")
                else:
                    print(f"    {name:<28} {val:>6}  ({lo}..{hi})  (raw)")


if __name__ == "__main__":
    main()
