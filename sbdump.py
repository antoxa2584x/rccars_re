#!/usr/bin/env python3
"""
Structure dumper for CREAT Studios .sb scene databases (RC Cars, SOURCE51).

.sb reuses the same chunk framing as the TIOX pack (see FORMAT_NOTES.md):
    u16 id
    u32 end                absolute offset, one past the chunk
    u32 count              only if (id & 0x400) or (id & 0xF000) == 0x9000

Type tag lives in the id's high nibble:
    0x1xxx blob   0x3xxx dword   0x4xxx string
    0x9xxx / 0x0xxx / 0x5xxx container

Usage: python3 sbdump.py <file.sb> [--depth N] [--max N] [--hist]
"""

import argparse
import collections
import struct


def rd_chunk(b, p):
    cid = struct.unpack_from("<H", b, p)[0]
    end = struct.unpack_from("<I", b, p + 2)[0]
    n = p + 6
    cnt = None
    if (cid & 0x400) or (cid & 0xF000) == 0x9000:
        cnt = struct.unpack_from("<I", b, n)[0]
        n += 4
    return cid, end, cnt, n


def fourcc(v):
    s = struct.pack("<I", v)
    return s.decode("latin-1") if all(32 <= c < 127 for c in s) else None


def preview(b, cid, body, end):
    """Render a chunk's payload according to its type nibble."""
    n = end - body
    if n <= 0:
        return ""
    if (cid & 0xF000) == 0x4000:                      # string
        s = b[body:end].split(b"\0")[0]
        return " " + repr(s.decode("latin-1"))
    if (cid & 0xF000) == 0x3000 and n >= 4:           # dword
        return f" = {struct.unpack_from('<I', b, body)[0]}"
    if n <= 16:
        return " " + b[body:end].hex(" ")
    return f" [{n} bytes]"


def walk(b, p, end, depth, out, maxdepth, budget):
    while p < end and budget[0] > 0:
        if p + 6 > len(b):
            return
        cid, cend, cnt, body = rd_chunk(b, p)
        if cend <= p or cend > len(b):
            out.append(("  " * depth) + f"!! bad chunk 0x{cid:04X} end={cend} at {p}")
            return
        budget[0] -= 1
        fc = fourcc(cnt) if cnt is not None else None
        tag = f" cnt={cnt}" + (f" '{fc}'" if fc else "") if cnt is not None else ""
        out.append(("  " * depth) + f"0x{cid:04X} @{p} end={cend}{tag}{preview(b, cid, body, cend)}")
        # containers: recurse when the payload itself parses as chunks
        if (cid & 0xF000) in (0x9000, 0x0000, 0x5000) and depth < maxdepth and body < cend:
            walk(b, body, cend, depth + 1, out, maxdepth, budget)
        p = cend


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("path")
    ap.add_argument("--depth", type=int, default=4)
    ap.add_argument("--max", type=int, default=300)
    ap.add_argument("--hist", action="store_true", help="chunk-id histogram over whole file")
    a = ap.parse_args()

    b = open(a.path, "rb").read()
    print(f"{a.path}  {len(b):,} bytes")

    if a.hist:
        hist = collections.Counter()
        stack = [(0, len(b))]
        seen = 0
        while stack:
            p, end = stack.pop()
            while p < end and p + 6 <= len(b):
                cid, cend, cnt, body = rd_chunk(b, p)
                if cend <= p or cend > len(b):
                    break
                hist[cid] += 1
                seen += 1
                if (cid & 0xF000) in (0x9000, 0x0000, 0x5000) and body < cend:
                    stack.append((body, cend))
                p = cend
        print(f"chunks walked: {seen:,}\n")
        for cid, n in hist.most_common(30):
            print(f"  0x{cid:04X}  {n:>8}")
        return

    out = []
    walk(b, 0, len(b), 0, out, a.depth, [a.max])
    print("\n".join(out))


if __name__ == "__main__":
    main()
