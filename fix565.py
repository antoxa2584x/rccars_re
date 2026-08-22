#!/usr/bin/env python3
"""
fix565.py -- swap R and B in every RGB565 texture of a packed .vsc, in place.

One-shot migration for assets packed before pack_vsc.py's channel order was
corrected. The port used to write BLUE in the high bits; vitaGL hands
GL_UNSIGNED_SHORT_5_6_5 to GXM's U5U6U5_RGB, which is RED-high (the SDK spells
the same enum R5G6B5), so on real hardware every 565 texture came out with red
and blue exchanged -- blue sand, gold water. See pack_vsc.csi_texture.

This is a pure permutation of bytes already in the file, so running it is
equivalent to repacking with the fixed packer -- and takes seconds rather than
rebuilding 478 MB from the .sb databases.

Running it twice would undo it, so it has to know which order a file is already
in. It does NOT guess that from the pixels. The first version of this tool did:
it scored each file's mean(high 5-bit field) - mean(low 5-bit field) on the
theory that a real texture set is warmer than it is cool. Nine tracks agreed and
`car1.vsc` did not, because the Overkill's paint is genuinely blue -- so the
statistic was measuring the content it was supposed to be controlling for, which
is the same trap FORMAT_NOTES keeps recording. A heuristic that resonates with
its own subject is not a test.

Instead it re-derives the answer from the SOURCE: decode the original .csi for
one of the scene's 565 textures, pack it both ways, and see which one the bytes
in the .vsc actually equal. That is exact, content-independent, and cannot drift
from the packer because it calls the packer's own decoder. `warmth` is still
reported, as information only.

Usage:
    python3 fix565.py /mnt/c/tools/rccars_vita/assets/*.vsc
    python3 fix565.py --check  <files>     report only, change nothing
"""
import argparse
import os
import struct
import sys

import numpy as np

import csi2png
import pack_vsc


def texture_spans(buf):
    """Locate every RGB565 payload in a .vsc, and validate the whole file parses.

    Returns (version, [(offset, n_pixels), ...], [(name, w, h, fmt, mips), ...]).
    Raises ValueError if the stream does not land exactly on EOF, which is the
    only real guard that the offsets below are the right ones.
    """
    o = 0

    def u8():
        nonlocal o
        v = buf[o]
        o += 1
        return v

    def u16():
        nonlocal o
        v = struct.unpack_from("<H", buf, o)[0]
        o += 2
        return v

    def u32():
        nonlocal o
        v = struct.unpack_from("<I", buf, o)[0]
        o += 4
        return v

    def skip(n):
        nonlocal o
        o += n

    magic = bytes(buf[0:4])
    if magic[:3] != b"VSC" or not (0x33 <= magic[3] <= 0x39):
        raise ValueError(f"bad magic {magic!r}")
    ver = magic[3] - 0x30
    skip(4)

    n_tex, n_batches = u32(), u32()
    n_parts = u32() if ver >= 4 else 0
    n_markers = 0
    if ver >= 5:
        n_markers = u32()
        skip(4)                      # shadow radius
    n_models = u32() if ver >= 8 else 0

    for _ in range(n_models):        # VSC8: the model name table
        nlen = u16()
        skip(nlen)

    spans, info = [], []
    for _ in range(n_tex):
        nlen = u16()
        name = bytes(buf[o:o + nlen]).decode("latin1")
        skip(nlen)
        w, h, fmt, mips = u16(), u16(), u8(), u8()
        bpp = 4 if fmt == 1 else 2
        info.append((name, w, h, fmt, mips))
        for lvl in range(mips):
            lw, lh = max(1, w >> lvl), max(1, h >> lvl)
            if fmt == 0:
                spans.append((o, lw * lh))
            skip(lw * lh * bpp)

    for _ in range(n_parts):         # VSC4: name, parent, 3 x mat4
        nlen = u16()
        skip(nlen + 4 + 192)
    for _ in range(n_markers):       # VSC5: name, x, y, z, yaw
        nlen = u16()
        skip(nlen + 16)

    vsz = 28 if ver >= 6 else 20
    for _ in range(n_batches):
        u32()                        # tex
        flags = u32()
        if ver >= 4:
            skip(4)                  # part
        if ver >= 6:
            skip(4)                  # lm_tex
        env = 0
        if ver >= 7:
            env = u32()
        if ver >= 8:
            skip(4)                  # model index
        blend = flags & 256          # BATCH_BLEND
        if ver >= 9:
            skip(8)                  # the second base texture and the mask
        nv, ni = u32(), u32()
        skip(vsz * nv)
        if ver >= 7 and env:
            skip(12 * nv)            # vertex normals, env-mapped batches only
        if ver >= 9 and blend:
            skip(16 * nv)            # the blend's two UV sets, blend batches only
        skip(2 * ni)

    if o != len(buf):
        raise ValueError(f"parse ended at {o}, file is {len(buf)} bytes")
    return ver, spans, info


def warmth(buf, spans):
    """mean(high 5-bit field) - mean(low 5-bit field) over every 565 texel."""
    hi_sum = lo_sum = 0.0
    n = 0
    for off, npx in spans:
        v = np.frombuffer(buf, dtype="<u2", count=npx,
                          offset=off).astype(np.uint32)
        hi_sum += float((v >> 11).sum())
        lo_sum += float((v & 0x1F).sum())
        n += npx
    if not n:
        return 0.0
    return (hi_sum - lo_sum) / n * (255.0 / 31.0)


def probe_order(buf, spans, info, csidir):
    """Which channel order is this file's 565 data in? Decided against the source.

    Returns ('red', name) / ('blue', name), or (None, reason) when no texture in
    the scene could be matched to a .csi -- the embedded lightmap atlases and the
    baked '__shadow' are generated, so they have no source to compare with.
    """
    idx = pack_vsc.index_csi(csidir)
    # spans are in file order and cover only fmt == 0 textures, level by level;
    # walk the two lists together so a name lines up with its own level 0.
    si = 0
    for name, w, h, fmt, mips in info:
        if fmt != 0:
            continue
        off, npx = spans[si]
        si += mips
        src = idx.get(name.lower())
        if not src:
            continue
        try:
            d = open(src, "rb").read()
            hd = csi2png.parse_header(d)
            if hd.width != w or hd.height != h:
                continue
            o = hd.data_off
            if o < csi2png.HDR_SIZE or o + w * h * hd.bpp > len(d):
                o = csi2png.HDR_SIZE
            rgba = csi2png.decode_level(d, o, w, h, hd)
        except Exception:
            continue

        px = np.frombuffer(buf, dtype="<u2", count=npx, offset=off)
        a = np.frombuffer(rgba, dtype=np.uint8, count=w * h * 4).reshape(-1, 4)
        r = (a[:, 0] >> 3).astype(np.uint16)
        g = (a[:, 1] >> 2).astype(np.uint16)
        b = (a[:, 2] >> 3).astype(np.uint16)
        as_red = (r << 11) | (g << 5) | b
        as_blue = (b << 11) | (g << 5) | r
        if np.array_equal(px, as_red):
            return "red", name
        if np.array_equal(px, as_blue):
            return "blue", name
    return None, "no texture in this scene has a matching .csi source"


def swap(buf, spans):
    for off, npx in spans:
        v = np.frombuffer(buf, dtype="<u2", count=npx, offset=off)
        # (b<<11)|g|(r) <-> (r<<11)|g|(b): exchange the two 5-bit fields.
        v[:] = ((v & 0x001F) << 11) | (v & 0x07E0) | (v >> 11)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--check", action="store_true",
                    help="report the channel order, change nothing")
    ap.add_argument("--force", action="store_true",
                    help="swap even when the source says the file is already "
                         "red-high, or when no source could be found")
    ap.add_argument("--csidir", default="/mnt/c/tools/rccars_extracted",
                    help="where the original .csi textures live, for the "
                         "channel-order probe")
    args = ap.parse_args()

    bad = 0
    for path in args.files:
        with open(path, "rb") as f:
            buf = bytearray(f.read())
        try:
            ver, spans, info = texture_spans(buf)
        except ValueError as e:
            print(f"{path}: SKIPPED -- {e}")
            bad += 1
            continue

        n565 = sum(1 for _, _, _, fmt, _ in info if fmt == 0)
        w = warmth(buf, spans)
        order, why = probe_order(buf, spans, info, args.csidir)
        verdict = {"red":  "red-high, already correct",
                   "blue": "BLUE-high, needs fixing",
                   None:   "UNKNOWN"}[order]
        print(f"{path}: VSC{ver}  {n565}/{len(info)} textures are 565, "
              f"{len(spans)} mip levels, warmth {w:+.1f} (info only)")
        print(f"    source probe: {verdict}"
              + (f" -- matched against '{why}'" if order else f" -- {why}"))

        if args.check:
            continue
        if order == "red" and not args.force:
            print("    left alone (--force to swap anyway)")
            continue
        if order is None and not args.force:
            print("    NOT touched: cannot tell which order it is in. Pass "
                  "--csidir, or --force if you are sure.")
            bad += 1
            continue

        swap(buf, spans)
        with open(path, "wb") as f:
            f.write(buf)
        # Re-read from disk rather than trusting the buffer: this is the only
        # check that what landed on the card is what we meant to write.
        with open(path, "rb") as f:
            back = bytearray(f.read())
        _, spans2, info2 = texture_spans(back)
        order2, _ = probe_order(back, spans2, info2, args.csidir)
        ok = (order2 == "red")
        print(f"    swapped, re-read from disk -> "
              f"{'OK, red-high' if ok else 'STILL WRONG (%s)' % order2}")
        if not ok:
            bad += 1

    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
