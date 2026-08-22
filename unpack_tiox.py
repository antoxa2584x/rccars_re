#!/usr/bin/env python3
"""
Extractor for CREAT Studios "TIOX" pack containers (RC Cars, engine SOURCE51).

Format derived from the shipped reader/writer API:
  tioPackLoadRoot / tioPackFindFile / tiohbReadChunk  (CRSGame20.dll)

Chunk header:
    u16 id
    u32 end                  absolute file offset one past the chunk
    u32 count                present ONLY if (id & 0x400) or (id & 0xf000) == 0x9000

The id's high nibble is a type tag: 0x3xxx = dword, 0x4xxx = string,
0x0xxx / 0x5xxx = container.

Usage:
    python3 unpack_tiox.py list    <pack>
    python3 unpack_tiox.py extract <pack> <outdir> [--limit N] [--filter SUBSTR]
"""

import argparse
import os
import struct
import sys

# --- chunk ids -------------------------------------------------------------
CH_ROOT        = 0x5210
CH_COMMENT     = 0x4011
CH_BASEPATH    = 0x4014
CH_POOLSIZE    = 0x3012
CH_FILELIST    = 0x0613   # count-bearing
CH_ENTRY       = 0x0220
CH_ENTRY_NAME  = 0x4021
CH_ENTRY_V1    = 0x3022
CH_ENTRY_V2    = 0x3023
CH_ENTRY_V3    = 0x3024
CH_DATADESC    = 0x0630   # count-bearing; wraps the whole blob region to EOF
CH_BLOB        = 0x1431   # count-bearing; one per stored file, count = payload size
BLOB_HDR       = 10       # u16 id + u32 end + u32 count


class Reader:
    def __init__(self, buf):
        self.b = buf
        self.p = 0

    def tell(self):
        return self.p

    def seek(self, off):
        self.p = off

    def u16(self):
        v = struct.unpack_from("<H", self.b, self.p)[0]
        self.p += 2
        return v

    def u32(self):
        v = struct.unpack_from("<I", self.b, self.p)[0]
        self.p += 4
        return v

    def f32(self):
        v = struct.unpack_from("<f", self.b, self.p)[0]
        self.p += 4
        return v

    def cstr(self):
        e = self.b.index(b"\0", self.p)
        s = self.b[self.p:e].decode("latin-1")
        self.p = e + 1
        return s

    def chunk(self):
        """Returns (id, end, count|None). Mirrors tiohbReadChunk."""
        cid = self.u16()
        end = self.u32()
        cnt = None
        if (cid & 0x400) or (cid & 0xF000) == 0x9000:
            cnt = self.u32()
        return cid, end, cnt


class Entry:
    __slots__ = ("name", "v1", "v2", "v3")

    def __init__(self):
        self.name = None
        self.v1 = self.v2 = self.v3 = 0

    def __repr__(self):
        return f"<{self.name} v1={self.v1} v2={self.v2} v3={self.v3}>"


def parse_entry(r, entry_end):
    """Parse one 0x0220 entry body (mirrors the per-entry chunk loop)."""
    e = Entry()
    while r.tell() < entry_end:
        cid, end, _ = r.chunk()
        if cid == CH_ENTRY_NAME:
            e.name = r.cstr()
        elif cid == CH_ENTRY_V1:
            e.v1 = r.u32()
        elif cid == CH_ENTRY_V2:
            e.v2 = r.u32()
        elif cid == CH_ENTRY_V3:
            e.v3 = r.u32()
        else:
            pass  # unknown -> skip, as the engine does
        r.seek(end)
    return e


def parse_pack(buf, verbose=True):
    r = Reader(buf)

    cid, root_end, _ = r.chunk()
    if cid != CH_ROOT:
        raise SystemExit(f"not a TIOX pack: root chunk id 0x{cid:04X} != 0x{CH_ROOT:04X}")
    version = r.f32()

    comment = basepath = ""
    pool_size = None
    entries = []
    datadesc = None

    while r.tell() < root_end:
        cid, end, cnt = r.chunk()
        if cid == CH_COMMENT:
            comment = r.cstr()
        elif cid == CH_BASEPATH:
            basepath = r.cstr()
        elif cid == CH_POOLSIZE:
            pool_size = r.u32()
        elif cid == CH_FILELIST:
            declared = cnt
            while r.tell() < end:
                ecid, eend, _ = r.chunk()
                if ecid != CH_ENTRY:
                    break
                entries.append(parse_entry(r, eend))
                r.seek(eend)
            if verbose and declared is not None and declared != len(entries):
                print(f"  ! file list declared {declared} entries, parsed {len(entries)}")
        elif cid == CH_DATADESC:
            datadesc = (cid, end, cnt)
        r.seek(end)

    return {
        "version": version,
        "comment": comment,
        "basepath": basepath,
        "pool_size": pool_size,
        "entries": entries,
        "root_end": root_end,
        "datadesc": datadesc,
    }


def gen_keystream(key, n):
    """
    Payload cipher, from the packer's own encoder (FUN_10006BC0 in CRSGame20.dll,
    reached via tioPackStoreFile when entry flags & 0x20).

    XOR each byte with the next key byte. When the FOLLOWING key byte is NUL,
    mutate the whole key with k[i] = k[i]*3 + 1 (mod 256) and restart at 0.
    Mutation can introduce NULs mid-key (e.g. 'U' -> 0x00), which shortens later
    rounds -- that behaviour is load-bearing, not an edge case.
    """
    buf = bytearray(key) + bytearray(32 - len(key))   # NUL-padded, as in the original
    klen = len(key)                                   # strlen taken once, up front
    out = bytearray()
    j = 0
    while len(out) < n:
        out.append(buf[j])
        nxt = buf[j + 1]
        j += 1
        if nxt == 0:
            for t in range(klen):
                buf[t] = (buf[t] * 3 + 1) & 0xFF
            j = 0
    return bytes(out)


def classify(entries, filesize, root_end):
    """
    Decide which of v1/v2/v3 is offset and which is size.
    The offset field must land inside the file and past the directory;
    offset+size must stay in bounds. Contiguity breaks the tie.
    """
    fields = ("v1", "v2", "v3")
    best, best_score = None, -1
    for off_f in fields:
        for size_f in fields:
            if off_f == size_f:
                continue
            score = 0
            for e in entries:
                o, s = getattr(e, off_f), getattr(e, size_f)
                if root_end <= o < filesize and 0 <= s and o + s <= filesize:
                    score += 1
            if score > best_score:
                best_score, best = score, (off_f, size_f)
    return best, best_score


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["list", "extract"])
    ap.add_argument("pack")
    ap.add_argument("outdir", nargs="?")
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--filter", default="")
    a = ap.parse_args()

    buf = open(a.pack, "rb").read()
    info = parse_pack(buf)
    ents = info["entries"]
    filesize = len(buf)

    print(f"pack      : {a.pack}  ({filesize:,} bytes)")
    print(f"version   : {info['version']}")
    print(f"comment   : {info['comment']}")
    print(f"basepath  : {info['basepath']}")
    print(f"dir ends  : 0x{info['root_end']:X}")
    print(f"pool size : {info['pool_size']}")
    print(f"entries   : {len(ents):,}")

    (off_f, size_f), score = classify(ents, filesize, info["root_end"])
    print(f"field map : offset={off_f} size={size_f}  ({score}/{len(ents)} entries in bounds)")

    if a.mode == "list":
        for e in ents[: (a.limit or 20)]:
            print(f"  {getattr(e, off_f):>12} {getattr(e, size_f):>10}  {e.name}")
        return

    if not a.outdir:
        sys.exit("extract needs an outdir")

    longest = max((getattr(e, size_f) for e in ents), default=0)
    keystream = gen_keystream(info["basepath"].encode("latin-1"), longest)
    print(f"keystream : generated {len(keystream):,} bytes from basepath key")

    n = bad = short = 0
    total = 0
    for e in ents:
        if a.filter and a.filter.lower() not in (e.name or "").lower():
            continue
        off, size = getattr(e, off_f), getattr(e, size_f)
        if not (0 <= off and off + BLOB_HDR + size <= filesize):
            bad += 1
            continue
        # Each blob carries its own chunk header; the entry offset points at it.
        bid, bend, bcnt = struct.unpack_from("<HII", buf, off)
        if bid != CH_BLOB or bcnt != size:
            bad += 1
            continue
        data = buf[off + BLOB_HDR: off + BLOB_HDR + size]
        if keystream:
            # Payloads are XORed against a global, file-independent keystream.
            # Only the recovered prefix can be decoded; the tail stays raw.
            m = min(len(data), len(keystream))
            data = bytes(data[i] ^ keystream[i] for i in range(m)) + data[m:]
            if len(data) > len(keystream):
                short += 1
        rel = e.name.replace("\\", os.sep).lstrip(os.sep)
        dst = os.path.join(a.outdir, rel)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        with open(dst, "wb") as fh:
            fh.write(data)
        n += 1
        total += size
        if a.limit and n >= a.limit:
            break

    print(f"extracted : {n:,} files, {total:,} bytes  ({bad} malformed skipped, {short} with encoded tails)")


if __name__ == "__main__":
    main()
