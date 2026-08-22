#!/usr/bin/env python3
"""
pack_snd.py -- build the Vita port's sound bank from the game's own audio.

Two things come out of here:

  assets/sound.sbk   every SFX the game ships, resampled, with the per-sound
                     volume the engine's own snd.dat assigns it
  assets/music/*     the MP3 playlist, copied verbatim, plus music.idx

The bank is an INDEX + concatenated PCM, and the app reads entries out of it on
demand rather than loading all of it. That matters: the shipped WAVs total 66.8
MB of 44.1 kHz PCM and only a fraction is live at once (one car's motor set, one
track's ambient bed, the common cues).

WHERE THE DATA COMES FROM
-------------------------
`Sound/snd.dat` is the engine's sound prototype table -- the same universal
chunk container as everything else in this game (see FORMAT_NOTES.md):

    0x5250 root, float version, then one 0x0260 per sound:
        0x4061  name
        0x3063  volume, 0..100
        0x3064  always 1 in the retail data

149 entries. The name maps to `Sound/<name>.wav` CASE-INSENSITIVELY -- the table
says `ArrowFocus`, the file is `arrowfocus.wav`, and getting that wrong loses 15
of the 117 files. Eleven names have no file at all (the carAI_* variants,
EnterDlg, ExitDlg, motorAI_decel3); those are the engine's own
"SOUNDMANAGER: findsoundpath() failed" path and are simply skipped.

Two files are never named in snd.dat and are packed anyway because the port
wants them: car_surf_grass and water_stream.

`Sound/chn.dat` is the channel prototype table, same container, 46 entries. The
channels are what the exe refers to by name (the literal "car_Surfaces" is in
the image). They carry no routing in the .dat -- the binding is per-scene, in
the .sb's MOD_SNDCHANNEL nodes.

MUSIC
-----
`Autoexec.gm` is the game's own playlist, and it splits the soundtrack in two:

    TrackMP map "Dead_TrackNN.mp3" cycle 1     7 tracks, 3.7-5.7 MB
    TrackMP map "Mast_TrackNN.mp3" cycle 0    11 tracks, 1.7-2.6 MB

`cycle` is carried through to music.idx and the port uses group 1 as the racing
cycle and group 0 in the menu. All 18 are 44.1 kHz stereo 128 kbit MP3 and ship
as-is; audio.c decodes them with minimp3 on the streaming thread. Re-encoding
was considered and rejected -- MP3 at 128k is already smaller than any PCM or
ADPCM form that would not audibly cost something.

Usage:
    python3 pack_snd.py --out /mnt/c/tools/rccars_vita/assets
    python3 pack_snd.py --out ... --rate 44100      (full-rate bank, 2x size)
    python3 pack_snd.py --out ... --no-music        (bank only)
"""

import argparse
import os
import re
import shutil
import struct
import sys

import numpy as np

GAME = "/mnt/c/Games/RC Cars"
SOUND_DIR = "/mnt/c/tools/rccars_extracted/Sound"

# Packed even though snd.dat never names them: the port drives surface loops off
# its own surface classification, which has a grass case, and the stream needs a
# loop distinct from the ocean's.
EXTRA = ["car_surf_grass", "water_stream"]

SBK_MAGIC = b"SBK1"
SBK_NAME = 32                       # must match audio.h's SBK_NAME


# ---------------------------------------------------------------- chunk reader

def rd_chunk(b, p):
    """The universal CREAT chunk header. See FORMAT_NOTES.md."""
    cid = struct.unpack_from("<H", b, p)[0]
    end = struct.unpack_from("<I", b, p + 2)[0]
    n = p + 6
    if (cid & 0x400) or (cid & 0xF000) == 0x9000:
        n += 4
    return cid, end, n


def read_proto_table(path):
    """snd.dat / chn.dat -> [(name, volume)].

    Root 0x5250, then a float version (the same trap the pack, .crs and .bspl
    set -- the version sits between the root header and the first child), then
    one 0x0260 container per entry.
    """
    b = open(path, "rb").read()
    cid, end, _ = rd_chunk(b, 0)
    end = min(end, len(b))
    out = []
    p = 10                                          # 6-byte header + f32 version
    while p < end:
        cid, cend, body = rd_chunk(b, p)
        if cend <= p or cend > len(b):
            break
        if cid == 0x0260:
            name, vol = None, 100
            q = body
            while q < cend:
                c2, e2, b2 = rd_chunk(b, q)
                if e2 <= q:
                    break
                if c2 == 0x4061:
                    name = b[b2:e2].split(b"\0")[0].decode("latin-1")
                elif c2 == 0x3063:
                    vol = struct.unpack_from("<I", b, b2)[0]
                q = e2
            if name:
                out.append((name, vol))
        p = cend
    return out


# ------------------------------------------------------------------ wav reader

def read_wav(path):
    """-> (rate, channels, list-of-frames-as-bytes) for 16-bit PCM only.

    Walks the RIFF chunk list rather than assuming a 44-byte header: a couple of
    these files carry a LIST/INFO block before `data`, and slicing at 44 would
    decode that as audio.
    """
    b = open(path, "rb").read()
    if b[:4] != b"RIFF" or b[8:12] != b"WAVE":
        raise ValueError("%s: not a RIFF/WAVE" % path)
    p = 12
    fmt = None
    data = None
    while p + 8 <= len(b):
        tag = b[p:p + 4]
        sz = struct.unpack_from("<I", b, p + 4)[0]
        body = p + 8
        if tag == b"fmt ":
            fmt = struct.unpack_from("<HHIIHH", b, body)
        elif tag == b"data":
            data = b[body:body + sz]
        p = body + sz + (sz & 1)                    # RIFF chunks are word-aligned
    if fmt is None or data is None:
        raise ValueError("%s: missing fmt or data" % path)
    afmt, ch, rate, _br, _ba, bits = fmt
    if afmt != 1 or bits != 16:
        raise ValueError("%s: not 16-bit PCM (fmt=%d bits=%d)" % (path, afmt, bits))
    return rate, ch, data


def to_mono_s16(data, ch):
    """Interleaved s16 -> mono float32, averaging channels.

    Float from here on: the downmix and the resample both need headroom, and
    rounding twice through int16 costs a bit on the nine stereo files.
    """
    a = np.frombuffer(data[:len(data) - (len(data) % (2 * ch))], dtype="<i2")
    if ch == 1:
        return a.astype(np.float32)
    return a.reshape(-1, ch).astype(np.float32).mean(axis=1)


def resample(a, src, dst):
    """Linear resample of a mono float array. Identity when the rates match.

    Linear is the right call here and not a compromise: the material is 44.1k
    going to 22.05k, an exact 2:1, and everything above 11 kHz in this set is
    tape hiss. The mixer interpolates the same way at playback, so a fancier
    kernel here would be filtered by a linear one downstream anyway.

    The pre-filter is NOT optional. Decimating 2:1 with no low-pass folds
    everything above the new Nyquist back down as aliasing, and this set is full
    of broadband material -- tyre noise, the engine loops, the water beds -- that
    turns into a metallic whistle without it.
    """
    if len(a) == 0 or src == dst:
        return a
    if dst < src:
        a = lowpass(a, 0.5 * dst / src)
    n = int(len(a) * dst / src)
    pos = np.arange(n, dtype=np.float64) * (src / dst)
    j = pos.astype(np.int64)
    f = (pos - j).astype(np.float32)
    j = np.clip(j, 0, len(a) - 1)
    j1 = np.clip(j + 1, 0, len(a) - 1)
    return a[j] + (a[j1] - a[j]) * f


def lowpass(a, cutoff):
    """Zero-phase windowed-sinc low-pass, cutoff in cycles/sample (0..0.5)."""
    taps = 63
    m = np.arange(taps) - (taps - 1) / 2.0
    h = 2 * cutoff * np.sinc(2 * cutoff * m)
    h *= np.hamming(taps)
    h /= h.sum()
    return np.convolve(a, h.astype(np.float32), mode="same")


def to_s16(a):
    return np.clip(np.rint(a), -32768, 32767).astype("<i2")


def fit_peak(a):
    """Scale a file down if the resampler overshot the rails. -> (a, gain).

    Several of these masters are already at full scale, and a windowed-sinc
    low-pass rings past the peak it started from: 44 of the 117 files come out
    over 32767, the worst (car_cdt_car) at 1.16x with 0.59% of its samples
    outside the range. Hard-clipping those is an audible crackle on exactly the
    sounds you notice -- impacts and the engine loops.

    Scaling costs at most 1.2 dB on the hottest file and nothing at all on the
    other 73, and it keeps the shape of the waveform. The alternative, leaving
    headroom on everything up front, would quieten the whole bank to fix 37% of
    it.
    """
    if len(a) == 0:
        return a, 1.0
    peak = float(np.abs(a).max())
    if peak <= 32767.0:
        return a, 1.0
    g = 32767.0 / peak
    return a * g, g


# --------------------------------------------------------------- bank building

def find_wav(name, files_lc):
    """snd.dat names are not the on-disk case. Match case-insensitively."""
    return files_lc.get(name.lower())


def build_bank(sound_dir, rate, verbose):
    protos = read_proto_table(os.path.join(sound_dir, "snd.dat"))
    files_lc = {}
    for f in os.listdir(sound_dir):
        stem, ext = os.path.splitext(f)
        if ext.lower() == ".wav":
            files_lc[stem.lower()] = os.path.join(sound_dir, f)

    wanted = []                                     # [(bankname, volume, path)]
    seen = set()
    missing = []
    for name, vol in protos:
        key = name.lower()
        if key in seen:
            continue                                # snd.dat lists a few twice
        path = find_wav(name, files_lc)
        if path is None:
            missing.append(name)
            continue
        seen.add(key)
        wanted.append((key, vol, path))
    for name in EXTRA:
        if name.lower() in seen:
            continue
        path = find_wav(name, files_lc)
        if path is None:
            missing.append(name)
            continue
        seen.add(name.lower())
        wanted.append((name.lower(), 100, path))

    wanted.sort(key=lambda e: e[0])                 # sorted => app can bsearch

    entries = []
    blobs = []
    total_src = 0
    n_scaled = 0
    for name, vol, path in wanted:
        if len(name) >= SBK_NAME:
            raise ValueError("name too long for the bank index: %r" % name)
        src_rate, ch, data = read_wav(path)
        total_src += len(data)
        a, gain = fit_peak(resample(to_mono_s16(data, ch), src_rate, rate))
        if gain < 1.0:
            n_scaled += 1
        a = to_s16(a)
        blobs.append(a.tobytes())
        entries.append((name, vol, len(a)))
        if verbose:
            print("  %-24s %6.2f s  vol=%3d %s%s" %
                  (name, len(a) / rate, vol,
                   "stereo->mono " if ch > 1 else "",
                   "-%.2f dB" % (-20 * np.log10(gain)) if gain < 1.0 else ""))

    hdr = struct.pack("<4sIII", SBK_MAGIC, rate, len(entries), 0)
    idx_size = len(entries) * (SBK_NAME + 4 + 4 + 4)
    base = len(hdr) + idx_size
    idx = b""
    off = base
    for (name, vol, ns), blob in zip(entries, blobs):
        idx += name.encode("ascii").ljust(SBK_NAME, b"\0")
        idx += struct.pack("<III", off, ns, vol)
        off += len(blob)
    return hdr + idx + b"".join(blobs), entries, missing, total_src, n_scaled


# ----------------------------------------------------------------------- music

def read_playlist(autoexec):
    """Autoexec.gm -> [(filename, cycle_group)] in the game's own order."""
    out = []
    pat = re.compile(r'^\s*TrackMP\s+map\s+"([^"]+)"\s+cycle\s+(\d+)', re.I)
    for line in open(autoexec, "r", errors="replace"):
        m = pat.match(line)
        if m:
            out.append((m.group(1), int(m.group(2))))
    return out


def pack_music(tracks_dir, autoexec, outdir, verbose):
    pl = read_playlist(autoexec)
    if not pl:
        raise ValueError("no TrackMP entries in %s" % autoexec)
    mdir = os.path.join(outdir, "music")
    os.makedirs(mdir, exist_ok=True)
    kept = []
    for fn, cyc in pl:
        src = os.path.join(tracks_dir, fn)
        if not os.path.exists(src):
            print("  MISSING %s" % fn, file=sys.stderr)
            continue
        dst = os.path.join(mdir, fn)
        if not (os.path.exists(dst) and os.path.getsize(dst) == os.path.getsize(src)):
            shutil.copyfile(src, dst)
        kept.append((fn, cyc))
        if verbose:
            print("  %-20s cycle=%d  %5.1f MB" % (fn, cyc, os.path.getsize(src) / 1048576))
    with open(os.path.join(mdir, "music.idx"), "w", newline="\n") as f:
        f.write("# generated by pack_snd.py from Autoexec.gm -- name<TAB>cycle\n")
        for fn, cyc in kept:
            f.write("%s\t%d\n" % (fn, cyc))
    return kept


# ------------------------------------------------------------------------ main

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", required=True, help="the port's assets/ directory")
    ap.add_argument("--sound-dir", default=SOUND_DIR)
    ap.add_argument("--game-dir", default=GAME)
    ap.add_argument("--rate", type=int, default=22050,
                    help="bank sample rate (default 22050; 44100 doubles the size)")
    ap.add_argument("--no-music", action="store_true")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)

    print("bank: reading %s" % args.sound_dir)
    blob, entries, missing, total_src, n_scaled = build_bank(
        args.sound_dir, args.rate, args.verbose)
    path = os.path.join(args.out, "sound.sbk")
    with open(path, "wb") as f:
        f.write(blob)
    print("  %d sounds, %.1f s, %.1f MB at %d Hz (from %.1f MB of 44.1k source)"
          % (len(entries), sum(e[2] for e in entries) / args.rate,
             len(blob) / 1048576, args.rate, total_src / 1048576))
    if n_scaled:
        print("  %d scaled down to stop the resampler's overshoot clipping "
              "(see fit_peak)" % n_scaled)
    if missing:
        print("  %d snd.dat names with no wav (the engine's findsoundpath() "
              "failure path): %s" % (len(missing), ", ".join(sorted(missing))))
    print("  -> %s" % path)

    if not args.no_music:
        print("music: reading %s" % os.path.join(args.game_dir, "Autoexec.gm"))
        kept = pack_music(os.path.join(args.game_dir, "Tracks"),
                          os.path.join(args.game_dir, "Autoexec.gm"),
                          args.out, args.verbose)
        n1 = sum(1 for _, c in kept if c)
        print("  %d tracks (%d cycling, %d menu) -> %s"
              % (len(kept), n1, len(kept) - n1, os.path.join(args.out, "music")))


if __name__ == "__main__":
    main()
