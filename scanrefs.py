#!/usr/bin/env python3
"""Scan .text for 32-bit absolute addresses landing in a VA range and attribute
each hit to its enclosing function (from Ghidra's functions.csv export).

This catches every immediate / mod-rm displacement regardless of whether
Ghidra materialised a reference for it -- which matters for base+index
accesses into per-car tuning arrays.
"""
import bisect
import csv
import struct
import sys


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


def load_funcs(path):
    fns = []
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            try:
                a = int(r["address"], 16)
                s = int(r["size"])
            except (ValueError, KeyError):
                continue
            fns.append((a, s, r["name"]))
    fns.sort()
    return fns


def main():
    lo = int(sys.argv[1], 16)
    hi = int(sys.argv[2], 16)
    exe = sys.argv[3] if len(sys.argv) > 3 else "/mnt/c/Games/RC Cars/RCCars.exe"
    csvp = sys.argv[4] if len(sys.argv) > 4 else "/mnt/c/tools/rccars_re/RCCars.exe.functions.csv"

    d = open(exe, "rb").read()
    secs = pe_sections(d)
    fns = load_funcs(csvp)
    starts = [a for a, _, _ in fns]

    text = [s for s in secs if s[0] == ".text"]
    if not text:
        text = [secs[0]]

    hits = {}   # func -> set of target VAs
    for name, sva, vs, ro, rs in text:
        n = min(vs, rs)
        for i in range(n - 3):
            v = struct.unpack_from("<I", d, ro + i)[0]
            if lo <= v < hi:
                va = sva + i
                j = bisect.bisect_right(starts, va) - 1
                if j >= 0:
                    fa, fs, fname = fns[j]
                    if fa <= va < fa + fs:
                        hits.setdefault((fa, fname), set()).add(v)
                        continue
                hits.setdefault((0, "<no function>"), set()).add(v)

    for (fa, fname), tgts in sorted(hits.items()):
        print("%08x %-28s %3d refs: %s" % (
            fa, fname, len(tgts),
            " ".join("%08x" % t for t in sorted(tgts))[:400]))


if __name__ == "__main__":
    main()
