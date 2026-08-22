#!/usr/bin/env python3
"""Build a direct-call graph from raw E8/E9 rel32 scanning and answer
structural queries: callers-of, callees-of, and cluster roots.

Usage:
  cg.py callers <va> [depth]
  cg.py callees <va> [depth]
  cg.py roots   <lo> <hi>      # funcs in [lo,hi) called from outside it
"""
import bisect
import csv
import os
import struct
import sys
from collections import defaultdict

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


def build():
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
    byaddr = {a: (s, n) for a, s, n in fns}
    valid = set(starts)

    callees = defaultdict(set)
    callers = defaultdict(set)
    for name, sva, vs, ro, rs in sections(d):
        if name != ".text":
            continue
        n = min(vs, rs)
        for i in range(n - 4):
            if d[ro + i] != 0xE8:
                continue
            rel = struct.unpack_from("<i", d, ro + i + 1)[0]
            va = sva + i
            tgt = (va + 5 + rel) & 0xFFFFFFFF
            if tgt not in valid:
                continue
            j = bisect.bisect_right(starts, va) - 1
            if j < 0 or not (fns[j][0] <= va < fns[j][0] + fns[j][1]):
                continue
            src = fns[j][0]
            if src == tgt:
                continue
            callees[src].add(tgt)
            callers[tgt].add(src)
    return byaddr, callees, callers


def nm(byaddr, a):
    return byaddr.get(a, (0, "?"))[1]


def sz(byaddr, a):
    return byaddr.get(a, (0, "?"))[0]


def walk(byaddr, graph, root, depth, seen=None, ind=0):
    if seen is None:
        seen = set()
    if root in seen or depth < 0:
        return
    seen.add(root)
    print("%s%08x %-22s size=%d" % ("  " * ind, root, nm(byaddr, root),
                                    sz(byaddr, root)))
    for t in sorted(graph.get(root, ())):
        walk(byaddr, graph, t, depth - 1, seen, ind + 1)


def main():
    byaddr, callees, callers = build()
    cmd = sys.argv[1]
    if cmd == "roots":
        lo, hi = int(sys.argv[2], 16), int(sys.argv[3], 16)
        for a in sorted(byaddr):
            if not (lo <= a < hi):
                continue
            outside = [c for c in callers.get(a, ()) if not (lo <= c < hi)]
            if outside:
                print("%08x %-22s size=%-5d <- %s" % (
                    a, nm(byaddr, a), sz(byaddr, a),
                    " ".join("%08x/%s" % (c, nm(byaddr, c)) for c in sorted(outside))))
    elif cmd == "callers":
        walk(byaddr, callers, int(sys.argv[2], 16),
             int(sys.argv[3]) if len(sys.argv) > 3 else 2)
    elif cmd == "callees":
        walk(byaddr, callees, int(sys.argv[2], 16),
             int(sys.argv[3]) if len(sys.argv) > 3 else 2)


if __name__ == "__main__":
    main()
