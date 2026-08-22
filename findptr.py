#!/usr/bin/env python3
"""Locate every 4-byte occurrence of the given VAs anywhere in the image,
reporting section and, for .text hits, the enclosing function."""
import bisect
import csv
import os
import struct
import sys

EXE = "/mnt/c/Games/RC Cars/RCCars.exe"
# The Ghidra exports sit next to this script -- they are gitignored, so this
# resolves against the script and not the caller's working directory.
HERE = os.path.dirname(os.path.abspath(__file__))
CSVP = os.path.join(HERE, "RCCars.exe.functions.csv")


def sections(d):
    p = struct.unpack_from("<I", d, 0x3C)[0]
    nsec = struct.unpack_from("<H", d, p + 6)[0]
    optsz = struct.unpack_from("<H", d, p + 20)[0]
    imgbase = struct.unpack_from("<I", d, p + 24 + 28)[0]
    off = p + 24 + optsz
    out = []
    for i in range(nsec):
        e = off + 40 * i
        name = d[e:e + 8].rstrip(b"\0").decode("latin-1")
        vs, va, rs, ro = struct.unpack_from("<IIII", d, e + 8)
        out.append((name, imgbase + va, vs, ro, rs))
    return out


def main():
    targets = {int(a, 16) for a in sys.argv[1:]}
    d = open(EXE, "rb").read()
    fns = []
    with open(CSVP, newline="") as f:
        for r in csv.DictReader(f):
            try:
                fns.append((int(r["address"], 16), int(r["size"]), r["name"]))
            except ValueError:
                pass
    fns.sort()
    starts = [a for a, _, _ in fns]

    for name, sva, vs, ro, rs in sections(d):
        n = min(vs, rs)
        for i in range(n - 3):
            v = struct.unpack_from("<I", d, ro + i)[0]
            if v in targets:
                va = sva + i
                extra = ""
                if name == ".text":
                    j = bisect.bisect_right(starts, va) - 1
                    if j >= 0 and fns[j][0] <= va < fns[j][0] + fns[j][1]:
                        extra = "  in %08x/%s" % (fns[j][0], fns[j][2])
                print("target %08x  at %-8s %08x (off %06x)%s"
                      % (v, name, va, ro + i, extra))


if __name__ == "__main__":
    main()
