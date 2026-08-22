#!/usr/bin/env python3
"""Build a tuning-global -> config-key map by walking the loader disassembly.

Loaders follow a fixed idiom:
    mov  esi, <array base>
    push <"KeyName">           ; string VA in .rdata
    call 0x404770              ; cfgGetInt(3, section, key, &out)
    ...conversion...
    fstp DWORD PTR [esi+<off>] ; or ds:<abs>

so pairing the most recent key push with each float store recovers the layout.
"""
import re
import struct
import subprocess
import sys

EXE = "/mnt/c/Games/RC Cars/RCCars.exe"


def pe(d):
    p = struct.unpack_from("<I", d, 0x3C)[0]
    nsec = struct.unpack_from("<H", d, p + 6)[0]
    optsz = struct.unpack_from("<H", d, p + 20)[0]
    imgbase = struct.unpack_from("<I", d, p + 24 + 28)[0]
    off = p + 24 + optsz
    secs = []
    for i in range(nsec):
        e = off + 40 * i
        vs, va, rs, ro = struct.unpack_from("<IIII", d, e + 8)
        secs.append((imgbase + va, vs, ro, rs))
    return secs


def cstr(d, secs, va, maxn=64):
    for sva, vs, ro, rs in secs:
        if sva <= va < sva + max(vs, rs):
            o = ro + (va - sva)
            end = d.find(b"\0", o, o + maxn)
            if end < 0:
                return None
            s = d[o:end]
            if s and all(32 <= c < 127 for c in s):
                return s.decode()
    return None


PUSH_IMM = re.compile(r"push\s+0x([0-9a-f]+)$")
MOV_BASE = re.compile(r"mov\s+(e[a-z][a-z]),0x([0-9a-f]{7,8})$")
ST_ABS = re.compile(r"fstp?\s+DWORD PTR ds:0x([0-9a-f]+)$")
ST_REG = re.compile(r"fstp?\s+DWORD PTR \[(e[a-z][a-z])(?:([+-])0x([0-9a-f]+))?\]$")


def main():
    lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
    d = open(EXE, "rb").read()
    secs = pe(d)

    for spec in sys.argv[3:]:
        start = int(spec, 16)
        # size from functions.csv
        size = None
        for line in open("/mnt/c/tools/rccars_re/RCCars.exe.functions.csv"):
            f = line.split(",")
            if len(f) > 2 and f[0].lower() == "%08x" % start:
                size = int(f[2])
                break
        if size is None:
            size = 3200
        out = subprocess.run(
            ["objdump", "-d", "-M", "intel",
             "--start-address=0x%x" % start,
             "--stop-address=0x%x" % (start + size), EXE],
            capture_output=True, text=True).stdout

        print("### %08x" % start)
        bases = {}
        key = None
        for line in out.splitlines():
            if ":\t" not in line:
                continue
            txt = line.split("\t")[-1].strip()
            txt = re.sub(r"\s+", " ", txt)

            m = MOV_BASE.match(txt)
            if m:
                v = int(m.group(2), 16)
                bases[m.group(1)] = v
                if lo <= v < hi:
                    print("    base %-4s = %08x" % (m.group(1), v))
                continue
            m = PUSH_IMM.match(txt)
            if m:
                s = cstr(d, secs, int(m.group(1), 16))
                if s and re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", s):
                    key = s
                continue
            m = ST_ABS.match(txt)
            if m:
                a = int(m.group(1), 16)
                if lo <= a < hi:
                    print("    %08x  <- %s" % (a, key))
                continue
            m = ST_REG.match(txt)
            if m:
                reg, sign, off = m.group(1), m.group(2), m.group(3)
                if reg in bases:
                    o = int(off, 16) if off else 0
                    if sign == "-":
                        o = -o
                    print("    [%s%+d] = %08x  <- %s"
                          % (reg, o, bases[reg] + o, key))


if __name__ == "__main__":
    main()
