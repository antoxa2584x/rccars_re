#!/usr/bin/env python3
"""
CSI texture -> PNG converter (CREAT Studios SOURCE51 engine, RC Cars).

Format recovered from the engine's own validator (FUN_004452C0, reached via
csiLoadHeader) and channel decoder (csiGetARGBFormat) in RCCars.exe.

Header is a fixed 160 bytes:
    0x00  u32  magic 'MISC'          (reversed 'CSIM', engine's 4CC convention)
    0x04  f32  version
    0x08  u32  header size (160)
    0x0C  u32  data offset (>= 160)
    0x10  u32  width
    0x14  u32  height
    0x18  u32  bytes per pixel
    0x1C  u32  mipmap count (1..10)
    0x20  u32  channel count
    0x24  u8[] channel ids, one ASCII char each: 'A' 'R' 'G' 'B' 'I'
    0x2C  u16[] per-channel bit range, stride 2, little-endian:
              high byte = end bit, low byte = start bit
              bit count = hi - lo + 1;  0xFFFF means channel absent

Pixel data follows at the data offset: mip 0 first, then each successive
half-size level, each level width*height*bpp bytes.

Usage:
    python3 csi2png.py <file.csi|dir> [outdir] [--mips]
"""

import argparse
import glob
import os
import struct
import sys
import zlib

HDR_SIZE = 160
MAGIC = 0x4353494D          # 'MISC' little-endian


class CsiHeader:
    __slots__ = ("width", "height", "bpp", "mips", "channels", "data_off")

    def __repr__(self):
        ch = "".join(c for c, _, _ in self.channels)
        return f"{self.width}x{self.height} {self.bpp}bpp {ch} mips={self.mips}"


def parse_header(d):
    if len(d) < HDR_SIZE:
        raise ValueError("short file")
    magic, _ver, hsz, doff, w, h, bpp, mips, nch = struct.unpack_from("<IfIIIIIII", d, 0)
    if magic != MAGIC:
        raise ValueError(f"bad magic {magic:#x}")
    if hsz != HDR_SIZE:
        raise ValueError(f"unexpected header size {hsz}")
    if not (1 <= mips <= 10) or not w or not h or not bpp:
        raise ValueError("header failed engine validation")

    hdr = CsiHeader()
    hdr.width, hdr.height, hdr.bpp, hdr.mips = w, h, bpp, mips
    hdr.data_off = doff
    hdr.channels = []
    for i in range(nch):
        cid = d[0x24 + i]
        rng = struct.unpack_from("<H", d, 0x2C + i * 2)[0]
        if rng == 0xFFFF:
            continue
        lo, hi = rng & 0xFF, rng >> 8
        hdr.channels.append((chr(cid), lo, hi - lo + 1))
    return hdr


def decode_level(data, off, w, h, hdr):
    """Unpack one mip level into RGBA8 bytes."""
    bpp = hdr.bpp
    out = bytearray(w * h * 4)
    # default opaque; overwritten if an alpha channel exists
    for i in range(3, len(out), 4):
        out[i] = 255
    lut = []
    for cid, lo, nbits in hdr.channels:
        maxv = (1 << nbits) - 1
        idx = {"R": 0, "G": 1, "B": 2, "A": 3}.get(cid)
        lut.append((cid, lo, maxv, idx))
    for p in range(w * h):
        px = int.from_bytes(data[off + p * bpp: off + (p + 1) * bpp], "little")
        o = p * 4
        for cid, lo, maxv, idx in lut:
            v = (px >> lo) & maxv
            v = (v * 255 + maxv // 2) // maxv if maxv else 0
            if cid == "I":                      # intensity -> grey
                out[o] = out[o + 1] = out[o + 2] = v
            else:
                out[o + idx] = v
    return bytes(out)


def write_png(path, w, h, rgba):
    def chunk(tag, payload):
        c = struct.pack(">I", len(payload)) + tag + payload
        return c + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)

    raw = bytearray()
    stride = w * 4
    for y in range(h):
        raw.append(0)                            # filter: none
        raw += rgba[y * stride:(y + 1) * stride]
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(bytes(raw), 6))
           + chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(png)


def convert(src, outdir, want_mips=False):
    d = open(src, "rb").read()
    hdr = parse_header(d)
    base = os.path.splitext(os.path.basename(src))[0]
    os.makedirs(outdir, exist_ok=True)

    off = hdr.data_off
    written = []
    for lvl in range(hdr.mips if want_mips else 1):
        w = max(1, hdr.width >> lvl)
        h = max(1, hdr.height >> lvl)
        need = w * h * hdr.bpp
        if off + need > len(d):
            break
        rgba = decode_level(d, off, w, h, hdr)
        name = f"{base}.png" if lvl == 0 else f"{base}.mip{lvl}.png"
        dst = os.path.join(outdir, name)
        write_png(dst, w, h, rgba)
        written.append(dst)
        off += need
    return hdr, written


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("outdir", nargs="?", default="png_out")
    ap.add_argument("--mips", action="store_true", help="also emit mip levels")
    a = ap.parse_args()

    srcs = ([a.src] if os.path.isfile(a.src)
            else sorted(glob.glob(os.path.join(a.src, "**", "*.csi"), recursive=True)))
    if not srcs:
        sys.exit("no .csi files found")

    ok = fail = 0
    errs = []
    for s in srcs:
        try:
            rel = os.path.relpath(os.path.dirname(s), a.src) if os.path.isdir(a.src) else "."
            convert(s, os.path.join(a.outdir, rel) if rel != "." else a.outdir, a.mips)
            ok += 1
        except Exception as e:
            fail += 1
            errs.append(f"{os.path.basename(s)}: {e}")
    print(f"converted {ok}/{len(srcs)}   failed {fail}")
    for e in errs[:10]:
        print("   ", e)


if __name__ == "__main__":
    main()
