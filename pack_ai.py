#!/usr/bin/env python3
"""
Pack the AI opponents' recorded paths into one assets/<track>.aip per track.

An RC Cars opponent replays a profile a human (or, for some of these, the AI
itself -- see below) recorded: CarProfiles/<level>/<car><n>.dat, decoded by
prof_dump.py. The engine reads only the position and the linear momentum out of
each sample, walks the polyline at a commanded speed, and interpolates the pose
between the two samples the cursor straddles. ai.c does the same.

    char[4] "AIP1"
    u32     n                    opponents, up to AI_MAX_OPPONENTS
    u32     flags                0
    n x 100-byte opponent records (below)
    the sample blocks, in record order

    opponent record
      char[16] name              the driver, for the HUD
      char[32] path              the profile it came from, for the log
      u16 car                    0 Overkill, 1 Buggy, 2 Hummer
      u16 ref                    1..7 into AI_PLAYERS
      u16 races                  the EASY/NORMAL/HARD mask
      u16 boost, reson, tires    upgrade levels 0..3
      u16 pad, pad
      u32 n_samples
      u32 cycle_start            where the lap loop rejoins -- NOT 0
      u32 offset                 byte offset of this opponent's sample block
      f32 path_len               polyline length in metres
      f32 duration               the recorded time span in seconds
      f32 body_dy                the body-frame lift already APPLIED to every
                                 sample in this block, metres -- provenance
                                 only, see "the body frame" below
      f32 start[3]               sample 0's position, after the lift

    sample, 36 bytes
      f32 x, y, z                position of the centre of mass
      i16 qw, qx, qy, qz         orientation * 32767
      i16 px, py, pz             linear momentum P * 2000
      u8  susp[6]                suspension length * (255 / SUSP_FULL)
      i16 steer                  degrees * 100
      u16 dt                     seconds * 10000 since the PREVIOUS sample

The five fields kept are exactly the five something reads. Position and momentum
are what the engine itself takes out of a sample (FUN_00501cf0 reads +0x04 and
+0x20 and nothing else); the suspension lengths and the steer angle are what the
port's car rig needs to animate, and they are free because they are already
there; `dt` is the denominator of FUN_00502ea0's teleport guard. **The angular
momentum L is deliberately NOT packed**: nothing in the replay path reads the
body's angular velocity, so it would cost 6 bytes a sample for a future physics
path that does not exist yet. When one does, that is an AIP2.

Nothing is decimated. 36 bytes a sample over the 50 primary profiles is 7.7 MB
across the ten tracks and at most 1.2 MB in one of them, against a 35 MB .vsc,
so there is no size pressure worth trading exactness for -- and an exact polyline
means the path length, the corner geometry and the lap time are the recording's
own, with nothing to justify.

The quantisation is sized off the shipped data, measured over all 151 readable
profiles: suspension reaches 0.2506 m (u8 over 0..0.3 m is 1.18 mm), steer
reaches exactly 35.00 degrees (i16 at 0.01 degrees), |P| reaches 13.332 kg m/s
and no single axis 8.85 (i16 at 1/2000 spans +-16.38) and the longest
inter-sample gap is 0.830 s (u16 at 0.1 ms).

**Steer reaching exactly 35.00 is not a coincidence and it is worth knowing.**
That is `FUN_004fddd0`'s own clamp -- the AI's steering controller -- so some of
these profiles were recorded with the AI at the wheel rather than a person. It
also confirms, from the other end, that sample float 25 IS the steer angle:
nothing else in the record has a reason to stop dead on that number.

**The body frame, and why every sample is lifted.** A recorded position is the
ENGINE's centre of mass, and the port's is STILL not quite in the same place, so
a recorded position fed straight into rb_car_set_state puts the car at the wrong
height. The correction is a translation along the body's own Y axis (X and Z of
both origins are the model origin's), so it is applied as
`pos += body_dy * row1(R(q))`, which stays right when the car is on its side.

`body_dy` is MEASURED, not derived. At sample 0 every profile is a parked car at
the track's race start, and the port's own resting relation is exact there -- the
resting wheel centre is `mount_y - (len_free - sag)` in body space and it sits one
wheel radius above the surface, so a resting com is `REST_COM_Y` up. So
`body_dy = (ground + rest_com_y) - com_y` at sample 0, taken as the median over
all 50 primaries of that car:

    Overkill  -0.0278 m   (21 profiles, spread 11.6 mm)
    Buggy     -0.0544 m   (15 profiles, spread  8.7 mm)
    Hummer    -0.0056 m   (14 profiles, spread  7.9 mm)

The spread is the ground query sampling a mesh with centimetre relief under the
COM's own XZ, not disagreement about the offset.

**These are the residual AFTER CenterMassOY, and they are the one place the
recovered com height does not fully check out.** The engine's com sits com_oy
above the model origin and the port's now does too, so on the reasoning alone
these should all be zero. What they measure instead is that the recorded resting
com is 27.8 mm higher than the port puts the Overkill's, 54.4 mm higher than the
Buggy's, and 5.6 mm higher than the Hummer's. Before CenterMassOY the same numbers
read +28.9 / -25.1 / +33.5, i.e. the change nearly closes the Hummer (33.5 -> 5.6),
leaves the Overkill where it was and makes the Buggy worse.

The missing input is where the ENGINE's wheel mounts are. It reads them out of a
table in its own object (FUN_004f10d0 copies from +0x5d44 / +0xadd4 / +0xca94) and
the port derives them from the mesh as `wheel_y + len_free - sag`; a table that is
not exactly that lands exactly here. So this is not evidence against com_oy, it is
a second unknown in the same equation -- and the measurement stands on its own
either way, because the check below re-derives the resting drop after the lift and
requires it to land on REST_COM_Y, which is the property that matters.

A separate confirmation falls out of the same samples: the recorded suspension
lengths at rest are 0.0971 / 0.1455 / 0.0967 m against the port's own recovered
`len_free - sag` of 0.0984 / 0.1467 / 0.0984 -- within 1.7 mm on all three. These
recordings were made on the physics this port transcribes.

Four checks run before anything is written, and each is a measurement rather
than an assertion about intent:

  - the profile's own car index must equal the roster's. The two are independent
    copies (the .dat header's 0x2069 against ailayouts.ini's AI<n>Car) and they
    agree on all 50.
  - **the path must lie ON the track, and that is what PROVES the level-name
    mapping.** Every 10th sample is probed against the packed .col grid; the
    share with no surface under them at all is the number that matters. All 50
    primaries score 0% against the track championship.ini's comments name, and
    surf/over1 scored against the other nine grids gives 51.7% to 99.3% -- two
    orders of magnitude clear, and independent of any comment. The MEDIAN drop
    is only printed: it is a car under load with 20 cm of suspension travel, so
    it says nothing exact.
  - after the lift, sample 0's drop to the surface must be REST_COM_Y -- the
    resting com height the port's own mount_y implies, derived from rb_data.h
    rather than typed in so a change to the body frame cannot leave it stale.
    That is the one place the geometry is pinned, and it is what makes the lift
    a measurement with a check rather than a fudge.
  - the lap must CLOSE: the last sample sits where sample `cycle_start` does,
    because that is where FUN_00503880 sends the cursor when the path runs out.
    A gap there would teleport the car once a lap. Measured over the 50
    primaries it is 0.00 to 0.21 m on a 400-575 m lap -- which is also the
    third independent confirmation that header chunk 0x2061 IS the cycle start
    index: an arbitrary index would not close the loop.
  - sample 0's distance and heading from the track's race start are reported
    under a deliberately loose bound. They are NOT the mapping proof and cannot
    be: the marker is the PLAYER's grid slot and the opponents line up around
    it, 0.2 m away on urban_2 and 12 m away on beach_4, at up to 55 degrees of
    heading difference on country_3. What the bound catches is a gross error,
    not a starting grid.

Usage:
    python3 pack_ai.py [--track beach_1] [--out DIR] [--profiles DIR]
                       [--col DIR] [--check-only]
"""

import argparse
import math
import os
import re
import struct
import sys

from prof_dump import Profile

SAMPLE = 36
RECORD = 100
SUSP_FULL = 0.3          # the u8 range for a suspension length, metres
STEER_SCALE = 100.0      # i16 counts per degree
MOM_SCALE = 2000.0       # i16 counts per kg m/s
DT_SCALE = 10000.0       # u16 counts per second
MASS = 2.0               # every car is 2.0 kg (rb_data.h); P / MASS is speed

# How far a probe looks for the surface under a sample. A car in the air on a
# jump is legitimately far off the ground, so this only bounds the search.
GROUND_REACH = 3.0
PROBE_STRIDE = 10

# How high a RESTING centre of mass sits above the surface in the port's body
# frame, per car. That is what the lift below is calibrated against, and it used
# to be simply the wheel radius, because gen_rb_data.py parked the com on the
# wheel-centre plane. CenterMassOY moved it, so it is derived from rb_data.h
# rather than typed in:
#
#     rest_com_y = radius - (mount_y - (len_free - sag))
#
# -- the resting wheel centre is `mount_y - (len_free - sag)` in body space, and
# it sits one radius above the ground. Reading it out of the header means a
# future change to the body frame cannot leave this stale, which is exactly what
# happened the first time. 0.0151 / 0.0198 / 0.0200 m as things stand, against
# radii of 0.0718 / 0.0491 / 0.0591.
REST_COM_Y = (0.0, 0.0, 0.0)


def read_rest_com_y(path):
    src = open(path, encoding="latin-1").read()
    out = []
    for blk in re.findall(r'\{\s*"(?:Overkill|Buggy|Hummer)",'
                          r'(.*?/\*\s*radius\s*\*/[^\n]*)', src, re.S):
        def f(key):
            m = re.search(r"/\*\s*%s\s*\*/\s*(-?[0-9.]+)f" % key, blk)
            if not m:
                raise SystemExit("pack_ai: no %s in rb_data.h" % key)
            return float(m.group(1))
        out.append(f("radius") - (f("mount_y") - (f("len_free") - f("sag"))))
    if len(out) != 3:
        raise SystemExit("pack_ai: expected 3 cars in rb_data.h, got %d"
                         % len(out))
    return tuple(out)


# ------------------------------------------------------- ai_data.h / tracks.h

def read_ai_data(path):
    """Parse the generated ai_data.h back for the roster. Reading the header
    rather than re-parsing ailayouts.ini keeps the two packers honest: if
    gen_ai_data.py and this disagree, they disagree about one file."""
    txt = open(path).read()
    body = txt[txt.index("AI_RACES[AI_N_RACES] = {"):]
    races = []
    cur = None
    for line in body.splitlines():
        s = line.strip()
        m = re.match(r'\{ "([^"]+)", "([^"]+)", (\d+), ([\d.]+)f, (\d+), \{', s)
        if m:
            cur = {"track": m.group(1), "dir": m.group(2),
                   "race": int(m.group(3)), "coeff": float(m.group(4)),
                   "ops": []}
            races.append(cur)
            continue
        m = re.match(r'\{ (\d+), (\d+), (\d+), (\d+), (\d+), (\d+), "([^"]+)" \},', s)
        if m and cur is not None:
            g = m.groups()
            cur["ops"].append({"ref": int(g[0]), "car": int(g[1]),
                               "races": int(g[2]), "boost": int(g[3]),
                               "reson": int(g[4]), "tires": int(g[5]),
                               "path": g[6]})
        if s == "};" and races:
            break
    names = re.findall(r'\{ "(\w+)", \d+, \d+, "[^"]+\.tga" \},', txt)
    return races, names


def read_tracks(path):
    """tracks.h -> {base: (x, y, z, yaw)}."""
    out = {}
    for m in re.finditer(r'\{ "[^"]*", "(\w+)", (-?[\d.]+)f, (-?[\d.]+)f, '
                         r'(-?[\d.]+)f, (-?[\d.]+)f,', open(path).read()):
        out[m.group(1)] = tuple(float(m.group(i)) for i in range(2, 6))
    return out


# ------------------------------------------------------------- the .col grid

class Col:
    """Just enough of a .col reader to answer "what is the surface height under
    this point". pack_col.py writes the format; col.c is the real reader."""

    def __init__(self, path):
        b = open(path, "rb").read()
        magic = b[:4].decode("latin1")
        if magic not in ("COL1", "COL2", "COL3", "COL4"):
            raise ValueError("%s: magic %r" % (path, magic))
        ver = int(magic[3])
        p = 4
        self.min_x, self.min_z, self.cell, self.nx, self.nz = \
            struct.unpack_from("<fffII", b, p)
        p += 20
        ntris, = struct.unpack_from("<I", b, p)
        p += 4
        if ver >= 2:
            p += 4                                  # default_surf
        self.tris = struct.unpack_from("<%df" % (ntris * 9), b, p)
        p += ntris * 36
        ncell = self.nx * self.nz
        self.start = struct.unpack_from("<%dI" % (ncell + 1), b, p)
        p += (ncell + 1) * 4
        nidx = self.start[ncell]
        self.idx = struct.unpack_from("<%dI" % nidx, b, p)
        self.ntris = ntris

    def ground(self, x, y, z, reach=GROUND_REACH):
        """The highest surface at or below y + 0.05 within `reach`, or None.
        Same intent as col_ground_at: a downward query, so a triangle's winding
        does not matter."""
        cx = int((x - self.min_x) / self.cell)
        cz = int((z - self.min_z) / self.cell)
        if not (0 <= cx < self.nx and 0 <= cz < self.nz):
            return None
        c = cz * self.nx + cx
        best = None
        t = self.tris
        for k in range(self.start[c], self.start[c + 1]):
            o = self.idx[k] * 9
            ax, ay, az = t[o], t[o + 1], t[o + 2]
            bx, by, bz = t[o + 3], t[o + 4], t[o + 5]
            gx, gy, gz = t[o + 6], t[o + 7], t[o + 8]
            # barycentric containment in XZ
            d = (bz - gz) * (ax - gx) + (gx - bx) * (az - gz)
            if abs(d) < 1e-12:
                continue
            u = ((bz - gz) * (x - gx) + (gx - bx) * (z - gz)) / d
            v = ((gz - az) * (x - gx) + (ax - gx) * (z - gz)) / d
            if u < -1e-4 or v < -1e-4 or u + v > 1 + 1e-4:
                continue
            hy = u * ay + v * by + (1 - u - v) * gy
            if hy <= y + 0.05 and hy >= y - reach:
                if best is None or hy > best:
                    best = hy
        return best


# ------------------------------------------------------------------ packing

def q16(v):
    n = int(round(v * 32767.0))
    return max(-32767, min(32767, n))


def body_up(q):
    """Row 1 of the body-to-world matrix, i.e. the car's own up axis in world
    space. Same expression rb_quat_to_matrix builds m[4..6] from, so the lift
    lands where the port's own matrix would put it."""
    w, x, y, z = q
    n = w * w + x * x + y * y + z * z
    if n < 1e-6:
        return (0.0, 1.0, 0.0)
    s = 2.0 / n
    return (s * y * x - s * z * w,
            1.0 - (s * z * z + s * x * x),
            s * z * y + s * x * w)


def pack_samples(pr, dy):
    out = bytearray()
    prev_t = pr.time(0)
    for i in range(pr.n):
        x, y, z = pr.pos(i)
        qw, qx, qy, qz = pr.quat(i)
        # The recorder writes q normalised (carSetState normalises on the way
        # in), but renormalise anyway so the i16 rounding cannot drift the norm.
        nrm = math.sqrt(qw * qw + qx * qx + qy * qy + qz * qz)
        if nrm < 1e-6:
            qw, qx, qy, qz, nrm = 1.0, 0.0, 0.0, 0.0, 1.0
        if dy:
            ux, uy, uz = body_up((qw, qx, qy, qz))
            x += dy * ux
            y += dy * uy
            z += dy * uz
        s = pr.susp(i)
        t = pr.time(i)
        out += struct.pack("<3f4h3h6BhH",
                           x, y, z,
                           q16(qw / nrm), q16(qx / nrm), q16(qy / nrm),
                           q16(qz / nrm),
                           *[max(-32767, min(32767, int(round(v * MOM_SCALE))))
                             for v in pr.mom(i)],
                           *[max(0, min(255, int(round(v * 255.0 / SUSP_FULL))))
                             for v in s],
                           max(-32767, min(32767,
                                           int(round(pr.steer(i) * STEER_SCALE)))),
                           max(0, min(65535,
                                      int(round((t - prev_t) * DT_SCALE)))))
        prev_t = t
    assert len(out) == pr.n * SAMPLE
    return bytes(out)


def resolve_profile(root, authored):
    """`surf/over1` -> the real path. The roster spells directories in three
    different cases (surf, Fort, AAD) and the filesystem has one, so this
    matches case-insensitively -- the same fix snd.dat's sound names needed."""
    d, f = authored.replace("\\", "/").split("/")
    dirs = {x.lower(): x for x in os.listdir(root)}
    if d.lower() not in dirs:
        raise FileNotFoundError("%s: no such profile directory" % d)
    real = os.path.join(root, dirs[d.lower()])
    files = {x.lower(): x for x in os.listdir(real)}
    want = (f + ".dat").lower()
    if want not in files:
        raise FileNotFoundError("%s/%s.dat" % (d, f))
    return os.path.join(real, files[want])


def measure_body_dy(races, prof_root, col_dir, tracks):
    """The per-car body-frame lift, measured at every primary profile's first
    sample: `(ground + radius) - com_y`, median per car. See the module header.

    Always measured over ALL ten tracks, whatever --track asks to pack, so the
    number written into one file is the same number written into every other."""
    grids, by_car = {}, {0: [], 1: [], 2: []}
    for r in races:
        cp = os.path.join(col_dir, r["track"] + ".col")
        if not os.path.exists(cp):
            continue
        grids[r["track"]] = Col(cp)
    for r in races:
        col = grids.get(r["track"])
        if col is None:
            continue
        for op in r["ops"]:
            pr = Profile(resolve_profile(prof_root, op["path"]))
            x, y, z = pr.pos(0)
            g = col.ground(x, y, z)
            if g is None or pr.car > 2:
                continue
            by_car[pr.car].append((g + REST_COM_Y[pr.car]) - y)
    out = {}
    for c in (0, 1, 2):
        v = sorted(by_car[c])
        if not v:
            out[c] = 0.0
            print("  body_dy car %d: NO measurement (no .col?), lift 0" % c)
            continue
        out[c] = v[len(v) // 2]
        print("  body_dy car %d: %+0.4f m  (median of %d, spread %.4f)"
              % (c, out[c], len(v), v[-1] - v[0]))
    return out


def yaw_of(q):
    """Heading in degrees from the quaternion, the way sb2obj's rotY does it:
    local +Z lands on (sin y, 0, cos y)."""
    w, x, y, z = q
    return math.degrees(math.atan2(2.0 * (w * y + x * z),
                                   1.0 - 2.0 * (x * x + y * y)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--track", default=None, help="just this one")
    ap.add_argument("--profiles",
                    default="/mnt/c/tools/rccars_extracted/CarProfiles")
    ap.add_argument("--data", default="/mnt/c/tools/rccars_vita/ai_data.h")
    ap.add_argument("--tracks", default="/mnt/c/tools/rccars_vita/tracks.h")
    ap.add_argument("--col", default="/mnt/c/tools/rccars_vita/assets")
    ap.add_argument("--out", default="/mnt/c/tools/rccars_vita/assets")
    ap.add_argument("--start-tol", type=float, default=25.0,
                    help="metres the first sample may sit from the race start."
                         " A loose bound on a starting GRID -- the measured"
                         " spread is 0.2 to 12.4 m")
    ap.add_argument("--yaw-tol", type=float, default=90.0,
                    help="degrees the first sample's heading may differ by."
                         " Measured spread is 0.4 to 59.8")
    ap.add_argument("--offgrid-tol", type=float, default=5.0,
                    help="percent of the path allowed to have no surface under"
                         " it. THE MAPPING PROOF: right grid 0%%, wrong grids"
                         " 52%% to 99%%")
    ap.add_argument("--loop-tol", type=float, default=0.5,
                    help="metres the lap may fail to close by. Measured worst"
                         " is 0.21 m, median 0.12, over a 400-575 m lap")
    ap.add_argument("--check-only", action="store_true")
    ap.add_argument("--rb-data", default="/mnt/c/tools/rccars_vita/rb_data.h",
                    help="where the resting com height is derived from")
    a = ap.parse_args()

    global REST_COM_Y
    REST_COM_Y = read_rest_com_y(a.rb_data)
    print("resting com above the surface, from %s: %s"
          % (os.path.basename(a.rb_data),
             " ".join("%.4f" % v for v in REST_COM_Y)))

    races, names = read_ai_data(a.data)
    starts = read_tracks(a.tracks)
    if not races:
        sys.exit("could not read AI_RACES out of %s" % a.data)

    print("measuring the body-frame lift over all %d primaries:" % sum(
        len(r["ops"]) for r in races))
    body_dy = measure_body_dy(races, a.profiles, a.col, starts)

    fails = 0
    for r in races:
        if a.track and r["track"] != a.track:
            continue
        col = None
        cp = os.path.join(a.col, r["track"] + ".col")
        if os.path.exists(cp):
            col = Col(cp)
        sx, sy, sz, syaw = starts[r["track"]]

        print("=== %s  (AIRace%d, CarProfiles/%s, coeff %.2f)"
              % (r["track"], r["race"], r["dir"], r["coeff"]))
        recs, blocks, off = [], [], 12 + RECORD * len(r["ops"])
        for slot, op in enumerate(r["ops"]):
            path = resolve_profile(a.profiles, op["path"])
            pr = Profile(path)

            # 1. the profile's own car index against the roster's
            if pr.car != op["car"]:
                print("    FAIL %s: profile says car %d, roster says %d"
                      % (op["path"], pr.car, op["car"]))
                fails += 1
            up = (pr.boost, pr.reson, pr.tires)
            want_up = (op["boost"], op["reson"], op["tires"])
            up_note = "" if up == want_up else "  upgrades %s vs roster %s" % (
                up, want_up)

            dy = body_dy.get(pr.car, 0.0)
            blk = pack_samples(pr, dy)

            # Read the first sample back OUT of the packed block, so what is
            # checked is what ships -- quantisation included.
            p0 = struct.unpack_from("<3f", blk, 0)
            q0 = [v / 32767.0 for v in struct.unpack_from("<4h", blk, 12)]

            # 2. the first sample against the track's own race start -- a loose
            # bound on a starting GRID, not the mapping proof
            d0 = math.hypot(p0[0] - sx, p0[2] - sz)
            dyaw = ((yaw_of(q0) - syaw + 180.0) % 360.0) - 180.0
            bad = d0 > a.start_tol or abs(dyaw) > a.yaw_tol
            if bad:
                print("         FAIL start %.2f m / %+.1f deg exceeds"
                      " %.0f m / %.0f deg" % (d0, dyaw, a.start_tol, a.yaw_tol))

            # 3. the lap must close where the cursor is sent back to
            pl = struct.unpack_from("<3f", blk, (pr.n - 1) * SAMPLE)
            pc = struct.unpack_from("<3f", blk, pr.cycle_start * SAMPLE)
            loop = math.dist(pl, pc)
            if loop > a.loop_tol:
                print("         FAIL lap gap %.2f m from sample %d to %d"
                      % (loop, pr.n - 1, pr.cycle_start))
                bad = True

            # 4. the path against the collision grid, and 5. the rest height
            gtxt = "no .col"
            if col is not None:
                drops, miss = [], 0
                for i in range(0, pr.n, PROBE_STRIDE):
                    x, y, z = struct.unpack_from("<3f", blk, i * SAMPLE)
                    g = col.ground(x, y, z)
                    if g is None:
                        miss += 1
                    else:
                        drops.append(y - g)
                drops.sort()
                n = len(drops)
                med = drops[n // 2] if n else float("nan")
                off_share = 100.0 * miss / max(1, miss + n)
                g0 = col.ground(p0[0], p0[1], p0[2])
                rest = (p0[1] - g0) if g0 is not None else float("nan")
                # 4. the one exact relation: a parked car's COM sits a wheel
                # radius above the surface. Tolerance is the measured spread of
                # the lift itself, doubled -- anything inside that is the ground
                # query sampling relief, anything outside is a wrong lift.
                rest_bad = not (abs(rest - REST_COM_Y[pr.car]) <= 0.024)
                # THE MAPPING PROOF. The right grid gives 0%; the nine wrong
                # ones give 52% to 99%.
                grid_bad = off_share > a.offgrid_tol
                if rest_bad:
                    print("         FAIL rest %.3f m against a %.3f m radius"
                          % (rest, REST_COM_Y[pr.car]))
                if grid_bad:
                    print("         FAIL %.1f%% of the path has no surface"
                          " under it -- wrong track?" % off_share)
                bad = bad or rest_bad or grid_bad
                gtxt = ("rest %.3f m vs radius %.3f;  median drop %.3f m,"
                        " %.1f%% off-grid, lap gap %.2f m"
                        % (rest, REST_COM_Y[pr.car], med, off_share, loop))
            if bad:
                fails += 1

            print("    %s slot %d %-14s car %d  %6d samples  cycle %5d"
                  "  %7.1f m  %5.1f s  lift %+0.4f%s"
                  % ("FAIL" if bad else "ok  ", slot, op["path"], pr.car, pr.n,
                     pr.cycle_start, pr.length(), pr.time(pr.n - 1), dy,
                     up_note))
            print("         start %.2f m / %+.1f deg from the race start;  %s"
                  % (d0, dyaw, gtxt))

            recs.append(struct.pack(
                "<16s32s8H3I3f3f",
                names[op["ref"] - 1].encode()[:15],
                op["path"].encode()[:31],
                pr.car, op["ref"], op["races"], op["boost"], op["reson"],
                op["tires"], 0, 0,
                pr.n, pr.cycle_start, off,
                pr.length(), pr.time(pr.n - 1), dy,
                p0[0], p0[1], p0[2]))
            assert len(recs[-1]) == RECORD, len(recs[-1])
            blocks.append(blk)
            off += len(blk)

        if a.check_only:
            continue
        outp = os.path.join(a.out, r["track"] + ".aip")
        with open(outp, "wb") as f:
            f.write(b"AIP1")
            f.write(struct.pack("<II", len(recs), 0))
            for x in recs:
                f.write(x)
            for x in blocks:
                f.write(x)
        print("    wrote %s (%.0f KB)" % (outp, os.path.getsize(outp) / 1024.0))

    if fails:
        sys.exit("%d check(s) failed" % fails)
    print("all checks passed")


if __name__ == "__main__":
    main()
