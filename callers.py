#!/usr/bin/env python3
"""Find every direct caller (E8 rel32 / E9 jmp) of the given target VAs,
attributed to the enclosing function."""
import bisect
import csv
import struct
import sys


def pe_sections(d):
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
    exe = "/mnt/c/Games/RC Cars/RCCars.exe"
    csvp = "/mnt/c/tools/rccars_re/RCCars.exe.functions.csv"
    targets = {int(a, 16) for a in sys.argv[1:]}

    d = open(exe, "rb").read()
    secs = pe_sections(d)
    fns = []
    with open(csvp, newline="") as f:
        for r in csv.DictReader(f):
            try:
                fns.append((int(r["address"], 16), int(r["size"]), r["name"]))
            except ValueError:
                pass
    fns.sort()
    starts = [a for a, _, _ in fns]

    hits = {}
    for name, sva, vs, ro, rs in secs:
        if name != ".text":
            continue
        n = min(vs, rs)
        for i in range(n - 4):
            if d[ro + i] not in (0xE8, 0xE9):
                continue
            rel = struct.unpack_from("<i", d, ro + i + 1)[0]
            va = sva + i
            tgt = (va + 5 + rel) & 0xFFFFFFFF
            if tgt in targets:
                j = bisect.bisect_right(starts, va) - 1
                fname = "<none>"
                fa = 0
                if j >= 0 and fns[j][0] <= va < fns[j][0] + fns[j][1]:
                    fa, _, fname = fns[j]
                hits.setdefault(tgt, {}).setdefault((fa, fname), 0)
                hits[tgt][(fa, fname)] += 1

    for t in sorted(hits):
        print("=== target %08x (%d callers) ===" % (t, len(hits[t])))
        for (fa, fname), c in sorted(hits[t].items()):
            print("    %08x %-24s x%d" % (fa, fname, c))


if __name__ == "__main__":
    main()
