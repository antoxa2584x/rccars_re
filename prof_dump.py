#!/usr/bin/env python3
"""
Reader for CarProfiles/*.dat -- the recorded driving profiles the AI opponents
and the ghost car replay.

Same chunk container as everything else in this game (FORMAT_NOTES.md), with one
quirk: the root 0x2250 header is followed by a **u16 version** (1) before the
first child, where the pack, .crs, .bspl, snd.dat and chn.dat carry a float.

    0x2250 root
      u16 version
      0x0260  header
        0x2061 u16  the CYCLE START sample index -- where the lap loop rejoins
        0x2069 u16  car: 0 Overkill, 1 Buggy, 2 Hummer
        0x4074 str  "Time path FULL: %f"        (the recorder's own log lines)
        0x4075 str  "Time path CYCLE: %f"
        0x4076 str  "Distance FULL: %f"
        0x4077 str  "Distance CYCLE: %f"
        0x2070 u16  boost upgrade level
        0x2071 u16  resonator upgrade level
        0x2072 u16  tires upgrade level
        0x2066 u16  36   sizeof the 0x0263 record
        0x2067 u16  136  sizeof a 0x0264 sample
        0x2068 u16  12   sizeof a 0x0265 event
      0x0263  the recorder's end state, 36 bytes in 129 of the 152 shipped
              profiles and 40 in the other 23 -- one extra dword at index 5,
              shifting everything after it. Only the first two fields are the
              same in both, and they are the only two read here: dword 0 is the
              cursor the recorder stopped at (which FUN_005023e0 discards, since
              it zeroes the cursor and the time after every load) and dword 1
              its clock. The rest holds a duplicate count, the event count and
              two heap POINTERS written out verbatim.
      0x0264  count x 136-byte samples
      0x0265  n x 12-byte events

A sample is a float timestamp followed by the ENTIRE 32-float rigid-body ODE
state (PHYSICS.md "The model"), plus one trailing float:

    +0x00  time                       seconds since the recording started
    +0x04  state[0..2]   position of the centre of mass
    +0x10  state[3..6]   orientation quaternion (w,x,y,z)
    +0x20  state[7..9]   linear momentum P
    +0x2c  state[10..12] angular momentum L
    +0x38  state[13..18] per-wheel suspension length, 6 corners
    +0x50  state[19..24] per-wheel length delta
    +0x68  state[25]     the steer angle in DEGREES (phys+0x5c6c)
    +0x6c  state[26..31] per-wheel extra compression
    +0x84  one trailing float, 0 in every shipped profile

Four independent things pin that layout, and none of them is the table itself:

  - state[25] runs +/-29.5 on an Overkill profile against its recovered steering
    lock of 29.29 degrees, and +/-35 where the AI driver commands it (0x4fddd0
    clamps to 35);
  - state[26..29] hold a constant 0.0014182 with [30] and [31] zero, which is
    `radius * 0.02` = len_extra on a FOUR-wheel car (FUN_004fbe60);
  - state[13..16] sit at 0.097 at rest against a recovered `len_free - sag` of
    0.218 - 0.119 = 0.099 m;
  - the engine's own accessor agrees. FUN_00501cf0 reads the position from
    +0x04/+0x08/+0x0c and the momentum from +0x20/+0x24/+0x28 of a `idx * 0x88`
    stride, which is this table's first and third rows.

**Only position and momentum are ever read back.** FUN_00501cf0 exposes those
two; FUN_004fd740 copies all 32 dwords and then hands out the position and
`P * invmass`. Everything else in the record is recorder residue -- which is why
pack_ai.py keeps the wheel lengths and the steer angle anyway: the port has a
car rig to animate and the numbers are sitting right there.

Usage:
    python3 prof_dump.py <file.dat> [--samples N] [--events]
"""

import argparse
import struct


class Profile:
    """One decoded CarProfiles/*.dat."""

    def __init__(self, path):
        self.path = path
        b = open(path, 'rb').read()
        cid, end, body = _rd(b, 0)
        if cid != 0x2250:
            raise ValueError('%s: root is %#06x, expected 0x2250' % (path, cid))
        if end > len(b):
            raise ValueError('%s: root end %#x past the %d-byte file'
                             % (path, end, len(b)))
        self.version, = struct.unpack_from('<H', b, body)

        hdr, rec, samp, evt = {}, None, b'', b''
        for cid, bo, ce in _chunks(b, body + 2, end):
            if cid == 0x0260:
                for k, kb, ke in _chunks(b, bo, ce):
                    hdr[k] = b[kb:ke]
            elif cid == 0x0263:
                rec = b[bo:ce]
            elif cid == 0x0264:
                samp = b[bo:ce]
            elif cid == 0x0265:
                evt = b[bo:ce]

        def u16(key):
            if key not in hdr:
                raise ValueError('%s: header chunk %#06x missing' % (path, key))
            return struct.unpack_from('<H', hdr[key])[0]

        self.cycle_start = u16(0x2061)
        self.car = u16(0x2069)
        self.boost = u16(0x2070)
        self.reson = u16(0x2071)
        self.tires = u16(0x2072)
        self.size_head = u16(0x2066)
        self.size_sample = u16(0x2067)
        self.size_event = u16(0x2068)
        self.notes = [hdr[k].split(b'\0')[0].decode('latin1')
                      for k in (0x4074, 0x4075, 0x4076, 0x4077) if k in hdr]

        # The strides are in the file, so assert them rather than assuming.
        # The SAMPLE and EVENT strides are 136 and 12 in all 152 shipped
        # profiles; the 0x0263 record's own size is NOT constant -- 129 files
        # carry 36 bytes and 23 carry 40, mixed within a single directory
        # (AAD/start.dat is 40 next to AAD/buggy1.dat's 36). Everything past
        # dword 1 shifts with it, so nothing here reads that far: the sample and
        # event counts come from the block lengths, which are the authority.
        if (self.size_sample, self.size_event) != (136, 12):
            raise ValueError('%s: sample/event strides %d/%d, expected 136/12'
                             % (path, self.size_sample, self.size_event))
        if rec is None or len(rec) != self.size_head:
            raise ValueError('%s: 0x0263 is %s bytes, header says %d'
                             % (path, 'absent' if rec is None else len(rec),
                                self.size_head))
        if len(samp) % 136:
            raise ValueError('%s: sample block %d bytes, not a multiple of 136'
                             % (path, len(samp)))

        self.n = len(samp) // 136
        self.n_events = len(evt) // 12
        self._samp = samp
        self._evt = evt

        # Dword 0 is the cursor the recorder stopped at, which for a recording
        # that ran to the end is the sample count. It agrees in every shipped
        # file, both header sizes, so a disagreement means a truncated block.
        n_rec = struct.unpack_from('<I', rec, 0)[0]
        if n_rec != self.n:
            raise ValueError('%s: end cursor %d against %d samples'
                             % (path, n_rec, self.n))
        # Dword 1 is the recorder's clock when it stopped, which is up to one
        # frame PAST the last sample it wrote (35 of the 152 differ, by 0.013 to
        # 0.020 s and never more). Checked with that slack rather than for
        # equality, so it still catches a block that is not this file's.
        if self.n > 1:
            t_rec = struct.unpack_from('<f', rec, 4)[0]
            slack = 3.0 * self.time(self.n - 1) / (self.n - 1)
            if abs(t_rec - self.time(self.n - 1)) > max(slack, 1e-3):
                raise ValueError('%s: end clock %.4f against last sample %.4f'
                                 % (path, t_rec, self.time(self.n - 1)))

        if not 0 <= self.cycle_start < max(self.n, 1):
            raise ValueError('%s: cycle start %d outside 0..%d'
                             % (path, self.cycle_start, self.n - 1))

    # -- accessors, each naming the byte offset it reads ---------------------

    def time(self, i):
        return struct.unpack_from('<f', self._samp, i * 136)[0]

    def pos(self, i):
        return struct.unpack_from('<3f', self._samp, i * 136 + 0x04)

    def quat(self, i):
        """(w, x, y, z) -- the order carSetState uses."""
        return struct.unpack_from('<4f', self._samp, i * 136 + 0x10)

    def mom(self, i):
        """Linear momentum P. Divide by the car's mass for velocity."""
        return struct.unpack_from('<3f', self._samp, i * 136 + 0x20)

    def angmom(self, i):
        return struct.unpack_from('<3f', self._samp, i * 136 + 0x2c)

    def susp(self, i):
        """The six per-wheel suspension lengths."""
        return struct.unpack_from('<6f', self._samp, i * 136 + 0x38)

    def steer(self, i):
        """The steer angle in DEGREES -- state[25], phys+0x5c6c."""
        return struct.unpack_from('<f', self._samp, i * 136 + 0x68)[0]

    def state(self, i):
        """All 32 floats, ready for rb_car_set_state."""
        return struct.unpack_from('<32f', self._samp, i * 136 + 0x04)

    def event(self, i):
        """(kind, b, time). Neither field's meaning is recovered."""
        return struct.unpack_from('<IIf', self._evt, i * 12)

    def length(self):
        """Path length in metres, summed over the polyline. This is the metric
        FUN_00503440 advances the AI along, so it is the one that matters."""
        tot = 0.0
        p = self.pos(0)
        for i in range(1, self.n):
            q = self.pos(i)
            tot += ((q[0] - p[0]) ** 2 + (q[1] - p[1]) ** 2
                    + (q[2] - p[2]) ** 2) ** 0.5
            p = q
        return tot


def _rd(b, p):
    cid, end = struct.unpack_from('<HI', b, p)
    return cid, end, p + 6


def _chunks(b, p, end):
    out = []
    while p < end - 5:
        cid, cend, body = _rd(b, p)
        if cend <= p or cend > len(b):
            break
        out.append((cid, body, cend))
        p = cend
    return out


CARS = ('Overkill', 'Buggy', 'Hummer')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('path')
    ap.add_argument('--samples', type=int, default=0,
                    help='print this many samples in full')
    ap.add_argument('--events', action='store_true')
    a = ap.parse_args()

    p = Profile(a.path)
    print('%s  v%d' % (a.path, p.version))
    print('  car        %d (%s)' % (p.car, CARS[p.car] if p.car < 3 else '?'))
    print('  upgrades   boost %d  reson %d  tires %d'
          % (p.boost, p.reson, p.tires))
    print('  samples    %d over %.3f s (%.1f Hz mean)'
          % (p.n, p.time(p.n - 1), (p.n - 1) / max(p.time(p.n - 1), 1e-6)))
    print('  cycle from %d (t = %.3f s)'
          % (p.cycle_start, p.time(p.cycle_start)))
    print('  length     %.2f m' % p.length())
    print('  events     %d' % p.n_events)
    for s in p.notes:
        print('  note       %s' % s)
    print('  start      pos %8.3f %8.3f %8.3f  quat %.4f %.4f %.4f %.4f'
          % (p.pos(0) + p.quat(0)))

    for i in range(min(a.samples, p.n)):
        print('  [%5d] t=%8.3f pos %8.3f %8.3f %8.3f  P %7.3f %7.3f %7.3f'
              '  steer %7.2f  susp %s'
              % ((i, p.time(i)) + p.pos(i) + p.mom(i) + (p.steer(i),)
                 + (' '.join('%.4f' % v for v in p.susp(i)[:4]),)))
    if a.events:
        for i in range(p.n_events):
            print('  event %2d kind=%d b=%d t=%.3f' % ((i,) + p.event(i)))


if __name__ == '__main__':
    main()
