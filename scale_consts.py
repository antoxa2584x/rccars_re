#!/usr/bin/env python3
"""
Recover the scale constants the physics loaders apply to slider integers.

Each loader reads a config key as an integer and converts it with

    value = (raw - BASE) * SCALE + OFFSET

where BASE / SCALE / OFFSET are float globals in .rdata. Some keys have no
SCALE (identity) and/or no OFFSET. This parses the decompiled loaders
(physics.c) for those expressions and resolves each global to its float value
by reading the PE directly.

Usage: python3 scale_consts.py [--exe PATH] [--src physics.c]
"""

import argparse
import re
import struct


def pe_sections(d):
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    optsz = struct.unpack_from("<H", d, pe + 20)[0]
    imgbase = struct.unpack_from("<I", d, pe + 24 + 28)[0]
    off = pe + 24 + optsz
    secs = []
    for i in range(nsec):
        e = off + 40 * i
        name = d[e:e + 8].rstrip(b"\0").decode("latin-1")
        vs, va, rs, ro = struct.unpack_from("<IIII", d, e + 8)
        secs.append((name, imgbase + va, vs, ro, rs))
    return secs


def va_to_off(secs, va):
    for name, sva, vs, ro, rs in secs:
        if sva <= va < sva + max(vs, rs):
            return ro + (va - sva)
    return None


def read_f32(d, secs, va):
    o = va_to_off(secs, va)
    if o is None or o + 4 > len(d):
        return None
    return struct.unpack_from("<f", d, o)[0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default="/mnt/c/Games/RC Cars/RCCars.exe")
    ap.add_argument("--src", default="physics.c")
    ap.add_argument("--emit", default=None,
                    help="write a python table of the conversions")
    a = ap.parse_args()

    d = open(a.exe, "rb").read()
    secs = pe_sections(d)
    src = open(a.src, "r", errors="replace").read().splitlines()

    key_re = re.compile(r"FUN_00404770\([^,]+,[^,]+,s_([A-Za-z0-9_]+?)_00[0-9a-f]{6},&(\w+)\)")
    # (var - BASE) * SCALE  [+/- OFFSET]   /  (var - BASE) [+/- OFFSET]
    #
    # THE SIGN IS PART OF THE PATTERN. This used to accept only `+ _DAT_`, so a
    # loader that SUBTRACTS its offset matched as far as the scale and silently
    # reported offset 0 -- and on a signed quantity the offset is exactly what
    # centres the slider. physLoadCdt's CdtDelta{X,Y,Z} and CdtDeltaZ_CENTRAL are
    # `fsub 3.0` (0x4f8680 / 0x4f870e / 0x4f879c / 0x4f882a): (raw-1)*6/99 - 3.0,
    # a -3..+3 range that the loader's x0.1 turns into +-0.3 m. Losing the -3.0
    # put every body collision sphere in 0..+0.6 -- always positive, so the car's
    # proxy sat above AND ahead of it and was never symmetric about anything.
    sc_re = re.compile(
        r"-\s*_DAT_(00[0-9a-f]{6})\)"
        r"(?:\s*\*\s*_DAT_(00[0-9a-f]{6}))?"
        r"(?:\s*([+-])\s*_DAT_(00[0-9a-f]{6}))?")

    # Second shape: a linear-remap helper taking IMMEDIATE float bit patterns.
    #   FUN_0040ba70(outLo, outHi, inLo, inHi, x)
    #     = (x - inLo) * (outHi - outLo) / (inHi - inLo) + outLo
    # (argument order confirmed by decompiling the helper itself)
    #
    # There are TWO of these helpers and they are the same function. 0x004f9010
    # is the physics loader's own copy: same five arguments in the same order,
    # same expression at 0x004f905a, and it clamps at both ends instead of
    # leaving the caller to. Matching only the first one lost angleProcDownMax
    # -- which then recorded as raw, i.e. an angle 3.4x too large, next to an
    # angleProcUpMax that had been converted properly. A key that reads back
    # RAW looks exactly like a key with an identity conversion.
    remap_re = re.compile(
        r"FUN_00(?:40ba70|4f9010)\("
        r"\s*(0x[0-9a-f]+|[-\d.]+)\s*,\s*(0x[0-9a-f]+|[-\d.]+)\s*,"
        r"\s*(0x[0-9a-f]+|[-\d.]+)\s*,\s*(0x[0-9a-f]+|[-\d.]+)\s*,")

    def as_f32(tok):
        if tok.startswith("0x"):
            return struct.unpack("<f", struct.pack("<I", int(tok, 16)))[0]
        return float(tok)

    cache = {}
    def val(hexva):
        if hexva is None:
            return None
        if hexva not in cache:
            cache[hexva] = read_f32(d, secs, int(hexva, 16))
        return cache[hexva]

    table = {}
    print(f"{'key':<26}{'base':>10}{'scale':>14}{'offset':>12}   formula")
    print("-" * 92)
    seen = set()
    for i, line in enumerate(src):
        m = key_re.search(line)
        if not m:
            continue
        key, var = m.group(1), m.group(2)
        # NOTE: do not match on `var` -- the decompiler reads the key into one
        # local (the int) and refers to a different local in the conversion
        # (the float reinterpretation of the same slot), e.g. local_5c vs
        # local_58. Just take the first conversion before the next key.
        expr = rmap = None
        for j in range(i + 1, min(i + 40, len(src))):
            if key_re.search(src[j]):
                break
            r = remap_re.search(src[j])
            if r:
                rmap = r
                break
            s = sc_re.search(src[j])
            if s:
                expr = s
                break
        if key in seen:
            continue
        seen.add(key)
        if rmap:
            oL, oH, iL, iH = (as_f32(rmap.group(k)) for k in (1, 2, 3, 4))
            scale = (oH - oL) / (iH - iL) if iH != iL else 0.0
            table[key] = (iL, scale, oL)
            print(f"{key:<26}{iL:>10.6g}{scale:>14.8g}{oL:>12.6g}"
                  f"   (raw - {iL:g}) * {scale:.8g} + {oL:g}   [remap {iL:g}..{iH:g} -> {oL:g}..{oH:g}]")
            continue
        if not expr:
            print(f"{key:<26}{'-':>10}{'-':>14}{'-':>12}   (no scaling found)")
            continue
        b, s_ = val(expr.group(1)), val(expr.group(2))
        o = val(expr.group(4))
        if o is not None and expr.group(3) == "-":
            o = -o
        bs = f"{b:.6g}" if b is not None else "-"
        ss = f"{s_:.8g}" if s_ is not None else "1 (none)"
        os_ = f"{o:.6g}" if o is not None else "0"
        f = f"(raw - {bs})"
        if s_ is not None:
            f += f" * {ss}"
        if o is not None:
            f += f" {'-' if o < 0 else '+'} {abs(o):.6g}"
        table[key] = (b if b is not None else 0.0,
                      s_ if s_ is not None else 1.0,
                      o if o is not None else 0.0)
        print(f"{key:<26}{bs:>10}{ss:>14}{os_:>12}   {f}")


    if a.emit:
        with open(a.emit, "w") as fh:
            fh.write('"""Slider-integer -> physical value conversions recovered from the\n'
                     'physics loaders. value = (raw - base) * scale + offset\n'
                     'Generated by scale_consts.py -- do not edit by hand.\n"""\n\n')
            fh.write("SCALES = {\n")
            for k in sorted(table):
                b, s_, o = table[k]
                fh.write(f"    {k!r}: ({b!r}, {s_!r}, {o!r}),\n")
            fh.write("}\n")
        print(f"\nwrote {a.emit} with {len(table)} conversions")


if __name__ == "__main__":
    main()
