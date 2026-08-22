#!/usr/bin/env python3
"""
gen_hud_data.py -- the in-race HUD's data, out of the shipped game.

Writes rccars_vita/hud_data.h. NOTHING in here is invented: every number comes
either out of RCCars.exe's .data or out of a Settings/*.ini the game ships, and
each one is emitted with the address or the key it came from.

WHAT IS WHERE, and this is the whole point of the file
-----------------------------------------------------
  * `Settings/map.ini`      mapPosX_0 / mapPosY_0, the minimap's screen corner,
                            in RAW pixels of the 800x600 frame the game was
                            authored for (`Values 0 800 620` / `0 600 79` -- the
                            slider's own range IS the screen, and neither key
                            carries `Mask float`, so no conversion applies).
  * `Settings/cockpit.ini`  the place badge, the LAP word, the lap number and the
                            speed/boost indicator's angular sweep. These DO carry
                            `Mask float`, so each is remapped -- see cvt().
  * `Settings/cockpit_time.ini`  the two clocks' Y and the lap-time blink.
  * `Settings/smash20.ini` / `smash26.ini`   the font's metrics.
  * `RCCars.exe` 0x56c378   ten rows of three WORLD (x, z) reference points.
  * `RCCars.exe` 0x56c468   ten rows of the three MAP (u, v) points they land on.
                            Together: the per-track world->minimap affine.
                            FUN_004a9840 copies row `track` of both, 24 bytes
                            each, beside loading `trackmap_<track+1>`.
  * `RCCars.exe` 0x56db70   the minimap panel's own outline, 8 (u, v) pairs,
                            walked at 0x4b79ea and drawn as three quads.
  * `Language/English/Smash26.dat`  the font's character ORDER, 90 bytes of
                            ASCII, row-major over a 10 x 9 atlas.
  * `Language/English/Smash26.csi`  the atlas, whose per-glyph ink extents are
                            measured here so the text is not drawn monospaced.

THE AFFINE IS CHECKED, not assumed. Three (x,z)->(u,v) pairs determine an affine
exactly, and a painted top-down map of a world has to come out CONFORMAL -- a
uniform scale and a rotation, with no shear and no mirror. It does, on all ten
tracks: the two axis scales agree to within 11%, the off-diagonal cosine is under
0.07, the determinant is positive, and every track's own race start lands between
0.15 and 0.90 of the image. A mis-read of the pairing (x0,z0),(x1,z1),(x2,z2) as
(x0,x1,x2),(z0,z1,z2) fails every one of those.

AND THE ROTATION IS REAL, WHICH THIS DOCSTRING USED TO DENY. It said "no
rotation", and asserted only the three properties above -- every one of which a
pure rotation satisfies. The artists turned each painting to fit the panel: world
+x comes out at +145.7 degrees on beach_1, +14.6 on beach_4, +89.1 on country_4
and -129.4 on urban_2, printed per track in the emitted table below. race_ui.c's
marker heading was derived from the untested half of that sentence and pointed
the wrong way on all ten. See traps.md.

BUT ALL THREE ARE THE TWO TABLES AGAINST EACH OTHER, and they pass while the arrow
sits metres off the painted road. The check that compares two DIFFERENT shipped
artifacts is `mapcheck.py`: the `cp_N` markers out of each packed .vsc against the
ribbon as painted in trackmap_<n>. It measures 0.3 m on beach_1 and 0.7 on
beach_3 -- which is what says this reading of the tables is right -- and 3 to 19 m
on the other eight. Not an axis, an order or a flip: all 96 such combinations were
scored against it and none is clean. See known-issues.md, and run mapcheck.py
after touching anything here.
"""

import argparse
import math
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import csi2png

IMAGE_BASE = 0x400000

MAP_WORLD_VA = 0x56C378     # 10 x 3 x (x, z), metres
MAP_UV_VA    = 0x56C468     # 10 x 3 x (u, v), fractions of the 512x512 art
MAP_OUTLINE_VA = 0x56DB70   # 8 x (u, v), the panel's own shape
MAP_OUTLINE_N  = 8
# ... and the three immediates the outline is put through at 0x4b79ea:
#     (u * 167.0 - 83.5) * (1/512)   =   (u - 0.5) * 167/512
MAP_OUTLINE_SPAN_VA = 0x55483C   # 167.0
MAP_OUTLINE_HALF_VA = 0x554838   # 83.5
MAP_OUTLINE_NORM_VA = 0x554834   # 0.001953125 = 1/512

N_TRACKS = 10

# ---------------------------------------------------------------- the arrow
# THE DIRECTION ARROW ("DirectArrow") and the WRONG WAY banner. Both live in
# the race module, both are recovered whole, and every number below is either a
# slider in Settings/arrow.ini, geometry in RCCarsDB/cockpit.sb, or one of the
# exe's own immediates -- read here rather than typed, so a mis-read address
# cannot ship a plausible number.
ARW_INI = "arrow"                # Settings/arrow.ini, the slider group's name

# FUN_004b1360, the arrow's configure-from-ini: the four immediates its eight
# slider reads are put through.
ARW_MILLI_VA = 0x5544D8          # 0.001   NearPlane
ARW_CENTI_VA = 0x5544CC          # 0.01    CX / CY / Len / pos_y / pos_z
ARW_HALF_VA  = 0x554384          # 0.5     the viewport is CX,CY +/- Len/2
ARW_POS_BIAS_VA = 0x5547F8       # 200.0   pos_y / pos_z are biased by it
ARW_DIR_BIAS_VA = 0x55448C       # 90.0    and `dir` by this
ARW_DEG2RAD_VA  = 0x5543A4       # pi/180

# FUN_004e8e50, the per-racer record's reset: the two slew rates, as the
# immediates of the two `mov` that write them.
ARW_SLEW_VA = 0x4E8E7A           # 90.0 deg/s, record +0x00
ARW_CHASE_VA = 0x4E8E74          # 1.0 /s,     record +0x04
ARW_SNAP_MUL_VA = 0x554490       # 10.0, the multiplier while a snap is latched
ARW_ANG_EPS_VA  = 0x554390       # 1.0 deg, the snap's exit test

# FUN_004e8f00, the updater: the wrong-way timer and the blink.
ARW_WRONG_CAP_VA   = 0x554928    # 12.0 s, the timer's ceiling
ARW_WRONG_DECAY_VA = 0x5543F4    # 3.0, how many times faster it falls
ARW_WRONG_ON_VA    = 0x554498    # 6.0 s, the threshold the banner comes up at
ARW_WRONG_NEAR_VA  = 0x55472C    # 8.0 m of the last checkpoint suppresses it
ARW_BLINK_NEAR_VA  = 0x554730    # 5.0 m -- and the field it tests is IDENTIFIED
ARW_MIN_INIT_VA    = 0x4EA9C8    # 2e6, the sentinel the running minimum starts at
ARW_BESIDE_IN_VA   = 0x5543F0    # 4.0 m, the `cp_beside' cue's inner radius
ARW_CUE_CP_VA      = 0x4EA9B3    # 0x25d, the cue fired when a checkpoint is passed
ARW_CUE_BESIDE_VA  = 0x4EB605    # 0x25e, and when the car leaves one it reached
ARW_BLINK_SHOW_VA  = 0x554460    # 0.25 s, the half of the cycle the arrow is on
ARW_BLINK_CYCLE_VA = 0x4E9276    # 0.5 s, the cycle it restarts at
ARW_STEADY_VA      = 0x4E9258    # -1.0, the value that means "no blink"
ARW_TINT_FULL_VA   = 0x5547F4    # 512.0, progress -> the two colour channels

# FUN_004b11e0's slot tables, and the wrong way is slot 2.
MSG_SLOT_TEX_VA  = 0x56D2D0      # texture index per slot
MSG_SLOT_SIZE_VA = 0x56D278      # (w, h) as screen fractions
MSG_SLOT_UV_VA   = 0x56D328      # (u0, v0, u1, v1)
MSG_SLOT_PRIO_VA = 0x56D2FC      # the poster keeps the higher of the two
MSG_WRONG_SLOT   = 2
MSG_N_SLOTS      = 11            # 0..10; the twelfth row is uninitialised .data
MSG_TEX_NAMES = ("msg_low_signal", "msg_pause", "msg_wrong_way",
                 "msg_hits", "msg_321_s_f", "msg_bestlap")
MSG_LIFE_STEP_VA = 0x4AF28E      # the retire loop: life -= dt, drop at < -hold
MSG_N_PLAYERS_VA = 0x4AF2C4      # its own bound, 0xc = three dwords
MSG_GROW_FLOOR_VA = 0x5544A0     # 0.8 s, the floor on the grow-in's own length
# ... and the four arguments 0x4b024e pushes at the poster.
MSG_WRONG_LIFE_VA = 0x4B0256     # 1.5 s
MSG_WRONG_ARG4_VA = 0x4B0251     # 4.0, unread by the drawer
MSG_WRONG_SND_VA  = 0x4B0312     # 0x27c, the cue the first frame of each post

# RCCarsDB/cockpit.sb: the two chevrons' own top faces. The nodes RCCars.exe
# looks up by name at 0x4af44c / 0x4af465 -- and the two it fails on in
# "can't locate extru9 and extru9_3 on cockpit arrow".
ARW_SB = "RCCarsDB/cockpit.sb"
ARW_NODES = ("extru9", "extru9_3")


# Race starts, for the affine's own check. Straight out of gen_tracks.py's
# output -- the Players/Player instance of each track, x and z.
STARTS = [
    (-9.03305, -55.57899), (67.64058, -6.55663), (-36.69868, -51.98118),
    (43.66217, -15.96085), (21.99630, -40.57381), (1.73054, 12.94670),
    (33.88171, -32.51354), (-14.59037, 64.12112), (-70.97053, -24.32026),
    (-2.70602, -1.08644),
]
TRACK_NAMES = ["beach_1", "beach_2", "beach_3", "beach_4", "country_1",
               "country_2", "country_3", "country_4", "urban_1", "urban_2"]


def engine_levels(champ_path):
    """
    -> [track name] in the ENGINE's own level order, and their display names.

    THE ENGINE DOES NOT NUMBER THE TRACKS THE WAY THIS PORT DOES, and everything
    indexed by a level -- `trackmap_%i` and both calibration tables -- is in the
    engine's order. `Scripts/championship.ini` states it in its own comments, one
    per section:

        // SURF (Beach_1)      Section Keys "Track1"
        // FISHERS (Country_3) Section Keys "Track2"
        ...

    so the order is read out of the shipped data rather than guessed -- and then
    CHECKED against a geometric fit, which is the only reason to believe a comment:
    for each track, the map whose painted ribbon its own cp_N markers land on. The
    two agree on all ten, at 0.1 to 0.3 m against 1.2 to 29 m for every other
    pairing. The AI profile directories are a third witness: beach_1's are
    `surf/`, beach_2's `Camping/`, beach_3's `Fort/`, urban_2's `AAD/`.

    Two of the ten -- beach_1 and beach_3 -- happen to sit at their own index,
    which is exactly why the identity looked right for a while: they were the two
    that came out sub-metre while the other eight drew another track's map through
    another track's transform.
    """
    order, disp = [], []
    pend = None
    for line in open(champ_path, "r", errors="replace"):
        m = re.match(r"^//\s*([A-Z][A-Z0-9 _]*?)\s*\(([A-Za-z0-9_]+)\)\s*$",
                     line.strip())
        if m:
            pend = (m.group(1).strip(), m.group(2).strip().lower())
            continue
        m = re.match(r'^Section\s+Keys\s+"Track(\d+)"', line.strip())
        if m and pend:
            n = int(m.group(1))
            if n != len(order) + 1:
                raise SystemExit("championship.ini: Track%d out of order" % n)
            disp.append(pend[0])
            order.append(pend[1])
            pend = None
    if sorted(order) != sorted(TRACK_NAMES):
        raise SystemExit("championship.ini names %r, not the ten tracks" % order)
    return order, disp


# ------------------------------------------------------------------- the ini

def read_ini(path):
    """
    -> {key: (raw, slider_max, flo, fhi or None)}.

    The engine's slider file: a `Values lo hi default` line per key, optionally
    followed by `Mask float` and `FValues flo fhi`. A key with no float mask is
    used RAW; one with a mask is remapped, and NOT every scalar here is raw*0.01
    (see traps.md) -- which is why the mask is carried through rather than
    applied at read time.
    """
    out, key, vals = {}, None, None
    for line in open(path, "r", errors="replace"):
        s = line.strip()
        m = re.match(r'^Key\s+"(.*)"$', s)
        if m:
            if key and vals:
                out[key] = vals
            key, vals = m.group(1), None
            continue
        # smash26.ini's letSpace carries a whole comma-separated tail the artist
        # left behind; only the first number of the triple is the value.
        m = re.match(r"^Values\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)", s)
        if m and key:
            lo, hi, d = (int(x) for x in m.groups())
            vals = [d, hi - lo, None, None]
            continue
        m = re.match(r"^FValues\s+(-?[\d.]+)\s+(-?[\d.]+)", s)
        if m and key and vals:
            vals[2], vals[3] = float(m.group(1)), float(m.group(2))
    if key and vals:
        out[key] = vals
    return out


def cvt(ini, key):
    """
    The key's value in the units the engine uses it in.

    `flo + raw * (fhi - flo) / slider_range` for a float-masked key, and the raw
    integer otherwise. The remap is not taken on faith: smash26.ini's author left
    all twelve of that file's converted values inline as a comment on its first
    Values line (`30, -8, 0, 6, 25, 28, 5, 5, 5, 5, 25, 25`), and this expression
    reproduces all twelve.
    """
    raw, span, flo, fhi = ini[key]
    if flo is None:
        return float(raw)
    if span == 0:
        return flo
    return flo + raw * (fhi - flo) / span


# --------------------------------------------------------------- the affine

def solve_affine(P, Q):
    """(u, v) = A (x, z) + b from three point pairs. -> (a, b, c, d, e, f)."""
    def det3(m):
        return (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
                - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
                + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]))

    M = [[P[i][0], P[i][1], 1.0] for i in range(3)]
    D = det3(M)
    if abs(D) < 1e-9:
        raise SystemExit("the three reference points are collinear")
    out = []
    for k in range(2):
        rhs = [Q[i][k] for i in range(3)]
        for c in range(3):
            T = [row[:] for row in M]
            for i in range(3):
                T[i][c] = rhs[i]
            out.append(det3(T) / D)
    return out


def check_affine(t, P, Q):
    a, b, c, d, e, f = solve_affine(P, Q)
    sx, sy = math.hypot(a, d), math.hypot(b, e)
    cos = (a * b + d * e) / (sx * sy)
    x, z = STARTS[t]
    u, v = a * x + b * z + c, d * x + e * z + f
    name = TRACK_NAMES[t]
    if not (0.5 < sx / sy < 2.0):
        raise SystemExit(f"{name}: map axes disagree ({sx:.5f} vs {sy:.5f})")
    if abs(cos) > 0.12:
        raise SystemExit(f"{name}: map axes are not orthogonal (cos {cos:+.3f})")
    if a * e - b * d <= 0:
        raise SystemExit(f"{name}: map transform is mirrored")
    if not (0.05 < u < 0.95 and 0.05 < v < 0.95):
        raise SystemExit(f"{name}: race start maps off the art ({u:.3f},{v:.3f})")
    # where world +x ends up on the panel: u right, v down, 0 = up, cw positive
    rot = math.degrees(math.atan2(a, -d))
    return sx, sy, cos, u, v, rot


# ----------------------------------------------------------------- the font

def font_glyphs(csi_path, dat_path, cols, rows):
    """
    -> (chars, [(left, right)] per glyph in cell fractions, atlas w, h).

    The atlas is a plain `cols` x `rows` grid -- confirmed rather than assumed:
    the empty bands in its alpha straddle exactly 256/10 and 256/9 -- and the
    .dat gives the row-major order. Per glyph the INK columns are measured,
    because the art is left-aligned in its cell with a ragged right edge and
    drawing it monospaced spaces every number out.

    The .csi is decoded in memory through csi2png's own two primitives; nothing
    here goes near a PNG or PIL.
    """
    chars = open(dat_path, "rb").read().rstrip(b"\0")
    if len(chars) != cols * rows:
        raise SystemExit("%s: %d chars, not %d"
                         % (dat_path, len(chars), cols * rows))

    d = open(csi_path, "rb").read()
    hdr = csi2png.parse_header(d)
    W, H = hdr.width, hdr.height
    rgba = csi2png.decode_level(d, hdr.data_off, W, H, hdr)
    if len(rgba) < W * H * 4:
        raise SystemExit("%s: short pixel data" % csi_path)

    def a(x, y):
        return rgba[(y * W + x) * 4 + 3]

    cw, chh = W / cols, H / rows
    out = []
    for i in range(len(chars)):
        r, c = divmod(i, cols)
        x0, x1 = int(round(c * cw)), int(round((c + 1) * cw))
        y0, y1 = int(round(r * chh)), int(round((r + 1) * chh))
        lo, hi = None, None
        for x in range(x0, x1):
            if any(a(x, y) > 8 for y in range(y0, y1)):
                if lo is None:
                    lo = x
                hi = x + 1
        if lo is None:                  # a blank cell: it gets the space run
            lo, hi = x0, x0
        out.append(((lo - x0) / cw, (hi - x0) / cw))
    return chars, out, W, H



def csi_rgba(path):
    """-> (w, h, rgba bytes), through csi2png's own two primitives."""
    d = open(path, "rb").read()
    hdr = csi2png.parse_header(d)
    return hdr.width, hdr.height, csi2png.decode_level(
        d, hdr.data_off, hdr.width, hdr.height, hdr)


def fit_ring(path, want_red):
    """
    The dial ring's centre, radius band and angular span, in the texture's own
    0..1, measured off the shipped art.

    `cockpit_sp1` carries the dial: a dark translucent disc, a silver rim, and a
    GOLD tube running the whole sweep. `cockpit_sp2` carries the same ring in two
    tones, RED over the sweep and grey elsewhere. Neither is a config file, so
    the geometry is measured rather than read -- and the measurement is its own
    check: the two arcs and cockpit.ini's indicatorAngMin/Max are three
    independent statements of one sweep, and they agree to four degrees.

    A Kasa least-squares circle over the ring's own pixels gives the centre; the
    radius band and the angular span then fall out of it. Angles are degrees,
    zero straight UP and positive clockwise -- the convention cockpit.ini's two
    are in.
    """
    w, h, px = csi_rgba(path)
    if w != h:
        raise SystemExit("%s is %dx%d, not square" % (path, w, h))
    pts = []
    for y in range(h):
        for x in range(w):
            o = (y * w + x) * 4
            R, G, B, A = px[o], px[o + 1], px[o + 2], px[o + 3]
            if A < 60:
                continue
            red = R > 110 and R > G * 1.6 and R > B * 1.6
            gold = R > 140 and G > 90 and B < 120 and R > B * 1.5
            if (red if want_red else gold):
                pts.append((x, y))
    if len(pts) < 200:
        raise SystemExit("%s: only %d ring pixels" % (path, len(pts)))
    # Kasa fit: x^2 + y^2 = 2 cx x + 2 cy y + (r^2 - cx^2 - cy^2)
    n = len(pts)
    Sx = Sy = Sxx = Syy = Sxy = Sb = Sbx = Sby = 0.0
    for x, y in pts:
        b = x * x + y * y
        Sx += x; Sy += y; Sxx += x * x; Syy += y * y; Sxy += x * y
        Sb += b; Sbx += b * x; Sby += b * y
    A = [[Sxx, Sxy, Sx], [Sxy, Syy, Sy], [Sx, Sy, float(n)]]
    rhs = [Sbx, Sby, Sb]
    # Gaussian elimination, 3x3
    for i in range(3):
        p_ = max(range(i, 3), key=lambda k: abs(A[k][i]))
        A[i], A[p_] = A[p_], A[i]
        rhs[i], rhs[p_] = rhs[p_], rhs[i]
        for k in range(i + 1, 3):
            f = A[k][i] / A[i][i]
            for j in range(i, 3):
                A[k][j] -= f * A[i][j]
            rhs[k] -= f * rhs[i]
    sol = [0.0] * 3
    for i in (2, 1, 0):
        sol[i] = (rhs[i] - sum(A[i][j] * sol[j] for j in range(i + 1, 3))) / A[i][i]
    cx, cy = sol[0] / 2.0, sol[1] / 2.0

    rs, angs = [], []
    for x, y in pts:
        dx, dy = x - cx, y - cy
        rs.append(math.hypot(dx, dy))
        a = math.degrees(math.atan2(dx, -dy))
        angs.append(a)
    rs.sort(); angs.sort()
    # 0.5th/99.5th percentile, so one stray texel does not set a bound
    def pc(v, q):
        return v[min(len(v) - 1, max(0, int(q * (len(v) - 1))))]
    return dict(cu=cx / w, cv=cy / h,
                r0=pc(rs, 0.005) / w, r1=pc(rs, 0.995) / w,
                a0=pc(angs, 0.005), a1=pc(angs, 0.995))


def _q(s):
    return "'" + s.replace("'", "'\\''") + "'"


# ---------------------------------------------------------------- the arrow

def arrow_chevrons(sb_path):
    """
    -> [(z0, z1, name)], (x0, x1), (u0, u1), (v0, v1), y  for the DirectArrow.

    The two chevrons out of RCCarsDB/cockpit.sb. Each is SIX vertices and four
    triangles, flat in the XZ plane at one y, and its UV is an exact AFFINE
    function of (x, z) -- a planar projection -- which is asserted here and is
    what lets race_ui.c draw each chevron as ONE quad with a UV rect instead of
    a triangle list. The V is carved by coc_str_2's own alpha, and the rect
    comes out exactly that chevron's ink box in the art.

    The mesh is the authority on which way the arrow points: -z is away from the
    arrow's own camera, so the apex being the most-negative z is what says the
    chevron points forward.
    """
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "sb2obj", os.path.join(os.path.dirname(os.path.abspath(__file__)),
                               "sb2obj.py"))
    sb = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(sb)

    b = open(sb_path, "rb").read()
    reg, start = sb.read_registry(b)
    meshes, _tex = sb.collect(b, reg, start)
    byname = {nm: (v, uv, f) for nm, v, uv, f in meshes}

    out, box = [], None
    for nm in ARW_NODES:
        if nm not in byname:
            raise SystemExit("%s: no node %r" % (sb_path, nm))
        verts, uvs, faces = byname[nm]
        if len(verts) != 6 or len(faces) != 4:
            raise SystemExit("%s: %s is %d verts / %d tris, not 6 / 4"
                             % (sb_path, nm, len(verts), len(faces)))
        ys = {round(v[1], 5) for v in verts}
        if len(ys) != 1:
            raise SystemExit("%s: %s is not flat in y: %r" % (sb_path, nm, ys))
        # vertex -> uv, through the faces' parallel index triples
        pair = {}
        for f in faces:
            for vi, ti in zip(f[0], f[1]):
                if pair.setdefault(vi, ti) != ti:
                    raise SystemExit("%s: %s vertex %d has two uvs"
                                     % (sb_path, nm, vi))
        if len(pair) != 6:
            raise SystemExit("%s: %s uses %d of its 6 verts"
                             % (sb_path, nm, len(pair)))
        # u = a x + b, v = c z + d, least squares over the six, then CHECKED --
        # a mesh whose uv is not planar cannot be drawn as one quad.
        P = [(verts[vi][0], verts[vi][2], uvs[ti][0], uvs[ti][1])
             for vi, ti in sorted(pair.items())]
        a, bb = _fit1([(x, u) for x, _z, u, _v in P], "%s u(x)" % nm)
        c, dd = _fit1([(z, v) for _x, z, _u, v in P], "%s v(z)" % nm)
        for x, z, u, v in P:
            if abs(a * x + bb - u) > 4e-3 or abs(c * z + dd - v) > 4e-3:
                raise SystemExit("%s: %s uv is not affine in (x, z) at "
                                 "(%.4f, %.4f)" % (sb_path, nm, x, z))
        xs = sorted({round(v[0], 5) for v in verts})
        zs = sorted({round(v[2], 5) for v in verts})
        apex = min(verts, key=lambda v: v[2])
        if abs(apex[0]) > 1e-4:
            raise SystemExit("%s: %s's furthest point is not on its centre line"
                             % (sb_path, nm))
        # x and u are the SAME on both chevrons; z and the v intercept are not,
        # since the second one sits 0.3 further along z. What has to match is
        # the u fit, the v SLOPE, and the v range each one's own z maps to --
        # the two really do sample one rect, which is why one UV rect serves
        # both and why this is worth asserting rather than assuming.
        this = (xs[0], xs[-1], a, bb, c,
                c * zs[0] + dd, c * zs[-1] + dd, round(ys.pop(), 5))
        if box is None:
            box = this
        elif (this[:2] != box[:2]
              or max(abs(t - o) for t, o in zip(this[2:], box[2:])) > 4e-3):
            raise SystemExit("%s: %s is not the same chevron as %s"
                             % (sb_path, nm, ARW_NODES[0]))
        out.append((zs[0], zs[-1], nm))

    x0, x1, a, bb, c, v0, v1, y = box
    span = out[0][1] - out[0][0]
    for lo, hi, nm in out[1:]:
        if abs((hi - lo) - span) > 1e-4:
            raise SystemExit("%s: %s is a different depth" % (sb_path, nm))
    # the uv rect the quad samples, in the art's own 0..1
    return out, (x0, x1), (out[0][0], out[0][1]), (a * x0 + bb, a * x1 + bb,
                                                   v0, v1), y


def _fit1(pts, what):
    """Least-squares y = a x + b, and a degenerate x is an error not a NaN."""
    n = len(pts)
    sx = sum(p[0] for p in pts)
    sy = sum(p[1] for p in pts)
    sxx = sum(p[0] * p[0] for p in pts)
    sxy = sum(p[0] * p[1] for p in pts)
    den = n * sxx - sx * sx
    if abs(den) < 1e-12:
        raise SystemExit("%s: degenerate fit" % what)
    a = (n * sxy - sx * sy) / den
    return a, (sy - a * sx) / n


# ---------------------------------------------------------------- the output

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out", nargs="?",
                    default="/mnt/c/tools/rccars_vita/hud_data.h")
    ap.add_argument("--exe", default="/mnt/c/Games/RC Cars/RCCars.exe")
    ap.add_argument("--settings",
                    default="/mnt/c/tools/rccars_extracted/Settings")
    ap.add_argument("--lang", default="/mnt/c/Games/RC Cars/Language/English")
    ap.add_argument("--textures",
                    default="/mnt/c/tools/rccars_extracted/Textures.1")
    ap.add_argument("--cockpit",
                    default="/mnt/c/Games/RC Cars/RCCarsDB/cockpit.sb",
                    help="the scene the DirectArrow's two chevrons are in")
    ap.add_argument("--champ",
                    default="/mnt/c/tools/rccars_extracted/Scripts/"
                            "championship.ini")
    ap.add_argument("--print-maps", action="store_true",
                    help="print `<track> <trackmap number>` per line and exit -- "
                         "what build.sh packs each track's map from, so the "
                         "packer and the header cannot name two different maps.")
    a = ap.parse_args()

    if a.print_maps:
        order, _d = engine_levels(a.champ)
        for nm in TRACK_NAMES:
            print("%s %d" % (nm, order.index(nm) + 1))
        return

    b = open(a.exe, "rb").read()

    def f32(va):
        return struct.unpack_from("<f", b, va - IMAGE_BASE)[0]

    def floats(va, n):
        return struct.unpack_from("<%df" % n, b, va - IMAGE_BASE)

    # -- the three immediates the panel outline is put through, asserted rather
    #    than assumed: a wrong address gives a plausible-looking scale.
    span, half, norm = (f32(MAP_OUTLINE_SPAN_VA), f32(MAP_OUTLINE_HALF_VA),
                        f32(MAP_OUTLINE_NORM_VA))
    if abs(span - 167.0) > 1e-3 or abs(half - span / 2) > 1e-3:
        raise SystemExit("map outline scale is not (span, span/2): %r" % (
            (span, half),))
    if abs(norm - 1.0 / 512.0) > 1e-9:
        raise SystemExit("map outline normaliser is not 1/512: %r" % norm)

    outline = [floats(MAP_OUTLINE_VA + 8 * i, 2) for i in range(MAP_OUTLINE_N)]
    for u, v in outline:
        if not (0.0 <= u <= 1.0 and 0.0 <= v <= 1.0):
            raise SystemExit("map outline point out of 0..1: %r" % ((u, v),))

    # THE ENGINE'S LEVEL ORDER, and the map number each of this port's tracks
    # therefore uses. Everything below is emitted in the PORT's order (tracks.h's,
    # which is what race_ui.c and main.c index by), with each row lifted from the
    # engine row that track really is.
    engine_order, engine_disp = engine_levels(
        os.path.join(os.path.dirname(a.settings.rstrip("/")), "Scripts",
                     "championship.ini")
        if os.path.exists(os.path.join(
            os.path.dirname(a.settings.rstrip("/")), "Scripts",
            "championship.ini")) else a.champ)
    # port track index -> engine level index
    lvl = [engine_order.index(nm) for nm in TRACK_NAMES]

    calib, report = [], []
    for t in range(N_TRACKS):
        w = floats(MAP_WORLD_VA + 24 * lvl[t], 6)
        q = floats(MAP_UV_VA + 24 * lvl[t], 6)
        P = [(w[0], w[1]), (w[2], w[3]), (w[4], w[5])]
        Q = [(q[0], q[1]), (q[2], q[3]), (q[4], q[5])]
        for u, v in Q:
            if not (0.0 <= u <= 1.0 and 0.0 <= v <= 1.0):
                raise SystemExit("track %d: uv out of 0..1" % t)
        report.append(check_affine(t, P, Q))
        calib.append((P, Q))

    dial = fit_ring(os.path.join(a.textures, "cockpit_sp1.csi"), False)
    arc = fit_ring(os.path.join(a.textures, "cockpit_sp2.csi"), True)

    m = read_ini(os.path.join(a.settings, "map.ini"))
    ck = read_ini(os.path.join(a.settings, "cockpit.ini"))
    ct = read_ini(os.path.join(a.settings, "cockpit_time.ini"))
    ar = read_ini(os.path.join(a.settings, "arrow.ini"))

    # THE ARROW'S CAMERA, out of arrow.ini through FUN_004b1360's own four
    # immediates. Read, not typed: NOT every slider here is raw * 0.01 (pos_y
    # and pos_z are biased by 200 first and `dir` by 90) and traps.md says to
    # read the conversion per key.
    milli, centi, half = f32(ARW_MILLI_VA), f32(ARW_CENTI_VA), f32(ARW_HALF_VA)
    pos_bias, dir_bias = f32(ARW_POS_BIAS_VA), f32(ARW_DIR_BIAS_VA)
    if (abs(milli - 1e-3) > 1e-9 or abs(centi - 1e-2) > 1e-9
            or abs(half - 0.5) > 1e-9):
        raise SystemExit("arrow: the ini scales are not (0.001, 0.01, 0.5): %r"
                         % ((milli, centi, half),))
    if abs(f32(ARW_DEG2RAD_VA) - math.pi / 180.0) > 1e-9:
        raise SystemExit("arrow: 0x%x is not pi/180" % ARW_DEG2RAD_VA)
    arw = {
        "near":  cvt(ar, "NearPlane") * milli,
        "fov":   cvt(ar, "AngleVert"),
        "cx":    cvt(ar, "CX") * centi,
        "cy":    cvt(ar, "CY") * centi,
        "len":   cvt(ar, "Len") * centi,
        "pos_y": (cvt(ar, "pos_y") - pos_bias) * centi,
        "pos_z": (cvt(ar, "pos_z") - pos_bias) * centi,
        "dir":   cvt(ar, "dir") - dir_bias,
    }
    # The viewport is a SQUARE centred on (CX, CY): FUN_004b1360 builds the rect
    # as (CX - Len/2, CY - Len/2, Len, Len) and hands it to the camera. Both
    # sides of the screen have to contain it, or the recovered pair is not a
    # centre.
    for k in ("cx", "cy"):
        if not (arw["len"] * half < arw[k] < 1.0 - arw["len"] * half):
            raise SystemExit("arrow: %s puts the viewport off screen" % k)
    if not (0.0 < arw["fov"] < 180.0):
        raise SystemExit("arrow: AngleVert is %r" % arw["fov"])
    # The camera has to be able to SEE the arrow: it sits at the origin, and the
    # ray from the camera along -(0, cos dir, sin dir) has to cross y = 0 within
    # the chevrons' own footprint. This is the check that says `dir` is an
    # elevation and not a heading.
    _cd = math.cos(math.radians(arw["dir"]))
    _sd = math.sin(math.radians(arw["dir"]))
    if _cd <= 1e-6:
        raise SystemExit("arrow: dir does not look down at all")
    _hit = arw["pos_z"] - _sd * (arw["pos_y"] / _cd)
    if abs(_hit) > 0.5:
        raise SystemExit("arrow: the camera's aim crosses y=0 at z=%.3f, "
                         "nowhere near the arrow" % _hit)

    chev, chev_x, chev_z, chev_uv, chev_y = arrow_chevrons(a.cockpit)

    # THE WRONG WAY BANNER is message slot 2, and the tables say so rather than
    # the name: slot 2's texture index picks the third of the six the exe loads
    # at 0x4af195, whose order is msg_low_signal, msg_pause, msg_wrong_way,
    # msg_hits, msg_321_s_f, msg_bestlap.
    def u32(va):
        return struct.unpack_from("<I", b, va - IMAGE_BASE)[0]
    if u32(MSG_SLOT_TEX_VA + 4 * MSG_WRONG_SLOT) != MSG_WRONG_SLOT:
        raise SystemExit("message slot %d is not on texture %d"
                         % (MSG_WRONG_SLOT, MSG_WRONG_SLOT))
    msg_w, msg_h = floats(MSG_SLOT_SIZE_VA + 8 * MSG_WRONG_SLOT, 2)
    msg_uv = floats(MSG_SLOT_UV_VA + 16 * MSG_WRONG_SLOT, 4)
    if msg_uv != (0.0, 0.0, 1.0, 1.0):
        raise SystemExit("the wrong-way slot is a sub-rect, not the whole "
                         "texture: %r" % (msg_uv,))
    if not (0.0 < msg_w <= 1.0 and 0.0 < msg_h <= 1.0):
        raise SystemExit("the wrong-way banner is %r of the screen"
                         % ((msg_w, msg_h),))
    # THE WHOLE ELEVEN-SLOT TABLE, not just the one slot dirarrow.c wanted. Every
    # row has to name one of the six textures the exe loads and sit inside its
    # own texture, or one of the four addresses is wrong.
    msg_slots = []
    for sl in range(MSG_N_SLOTS):
        ti = u32(MSG_SLOT_TEX_VA + 4 * sl)
        sw, sh = floats(MSG_SLOT_SIZE_VA + 8 * sl, 2)
        u = floats(MSG_SLOT_UV_VA + 16 * sl, 4)
        pr = u32(MSG_SLOT_PRIO_VA + 4 * sl)
        if not (0 <= ti < len(MSG_TEX_NAMES)):
            raise SystemExit("message slot %d names texture %d" % (sl, ti))
        if not (0.0 < sw <= 1.0 and 0.0 < sh <= 1.0):
            raise SystemExit("message slot %d is %r of the screen" % (sl, (sw, sh)))
        if not (0.0 <= u[0] < u[2] <= 1.0 and 0.0 <= u[1] < u[3] <= 1.0):
            raise SystemExit("message slot %d's uv rect is %r" % (sl, u))
        if pr > 100:
            raise SystemExit("message slot %d's priority is %d" % (sl, pr))
        msg_slots.append((ti, sw, sh, u, pr))
    # The five slots over msg_321_s_f must PARTITION that atlas rather than
    # overlap -- countdown.c's own check, kept here so the table is held to it
    # before anything reads it.
    cd = [r for r in msg_slots if MSG_TEX_NAMES[r[0]] == "msg_321_s_f"]
    if len(cd) != 5:
        raise SystemExit("msg_321_s_f carries %d slots, not 5" % len(cd))
    if abs(sum((r[3][2] - r[3][0]) * (r[3][3] - r[3][1]) for r in cd) - 1.0) > 0.02:
        raise SystemExit("the five countdown cells do not tile their atlas")

    msg_life = f32(MSG_WRONG_LIFE_VA)
    msg_grow = f32(MSG_GROW_FLOOR_VA)
    msg_snd = u32(MSG_WRONG_SND_VA)
    if not (0.1 < msg_life < 10.0):
        raise SystemExit("the wrong-way banner's life is %r" % msg_life)

    # The updater's own numbers. Bounds only -- their MEANING is in the header's
    # comment and in dirarrow.h; what a check can do here is refuse a mis-read
    # address, and every one of these has a sign and a rough size it must have.
    arw_slew = f32(ARW_SLEW_VA)
    arw_chase = f32(ARW_CHASE_VA)
    arw_snap = f32(ARW_SNAP_MUL_VA)
    arw_eps = f32(ARW_ANG_EPS_VA)
    wr_cap = f32(ARW_WRONG_CAP_VA)
    wr_dec = f32(ARW_WRONG_DECAY_VA)
    wr_on = f32(ARW_WRONG_ON_VA)
    wr_near = f32(ARW_WRONG_NEAR_VA)
    bl_near = f32(ARW_BLINK_NEAR_VA)
    bl_show = f32(ARW_BLINK_SHOW_VA)
    bl_cycle = f32(ARW_BLINK_CYCLE_VA)
    bl_min0 = f32(ARW_MIN_INIT_VA)
    bl_in = f32(ARW_BESIDE_IN_VA)
    cue_cp = u32(ARW_CUE_CP_VA)
    cue_beside = u32(ARW_CUE_BESIDE_VA)
    # The blink reads a RUNNING MINIMUM, so its sentinel has to be far outside
    # any track and its inner radius inside the outer one, or the pair is not a
    # hysteresis and one of the two addresses is wrong.
    if not (bl_min0 > 1e5 and 0.0 < bl_in < bl_near):
        raise SystemExit("arrow: the beside pair is %r / %r / %r"
                         % (bl_min0, bl_in, bl_near))
    if not (cue_cp + 1 == cue_beside):
        raise SystemExit("arrow: the two checkpoint cues are not adjacent ids: "
                         "%#x / %#x" % (cue_cp, cue_beside))
    steady = f32(ARW_STEADY_VA)
    tint_full = f32(ARW_TINT_FULL_VA)
    if not (0.0 < arw_slew < 3600.0 and 0.0 < arw_chase < 100.0
            and arw_snap > 1.0 and 0.0 < arw_eps < 90.0):
        raise SystemExit("arrow: the slew block is %r"
                         % ((arw_slew, arw_chase, arw_snap, arw_eps),))
    if not (0.0 < wr_on < wr_cap and wr_dec > 1.0 and wr_near > 0.0
            and bl_near > 0.0):
        raise SystemExit("arrow: the wrong-way block is %r"
                         % ((wr_on, wr_cap, wr_dec, wr_near, bl_near),))
    # The blink is HALF ON: the arrow draws while the countdown is at or under
    # bl_show, and the countdown restarts at bl_cycle. Anything else is not a
    # blink and would mean one of the two addresses is wrong.
    if abs(bl_cycle - 2.0 * bl_show) > 1e-6 or steady >= 0.0:
        raise SystemExit("arrow: the blink is %r / %r / %r"
                         % (bl_show, bl_cycle, steady))
    if abs(tint_full - 512.0) > 1e-6:
        raise SystemExit("arrow: the tint's full scale is %r" % tint_full)

    fonts = {}
    for nm, ini_name in (("SMALL", "smash20.ini"), ("BIG", "smash26.ini")):
        base = "Smash20" if nm == "SMALL" else "Smash26"
        fi = read_ini(os.path.join(a.settings, ini_name))
        chars, ink, W, H = font_glyphs(
            os.path.join(a.lang, base + ".csi"),
            os.path.join(a.lang, base + ".dat"), 10, 9)
        fonts[nm] = (base, fi, chars, ink, W, H)

    # the two fonts must agree on the glyph SET, or one char lookup cannot serve
    # both -- which is what the header emits.
    if fonts["SMALL"][2] != fonts["BIG"][2]:
        raise SystemExit("Smash20 and Smash26 disagree on their character order")
    if fonts["SMALL"][4:] != fonts["BIG"][4:]:
        raise SystemExit("Smash20 and Smash26 are not the same atlas size")
    if fonts["BIG"][4] != fonts["BIG"][5]:
        raise SystemExit("the font atlas is not square")
    chars = fonts["BIG"][2]

    o = []
    w = o.append
    w("/* Generated by rccars_re/gen_hud_data.py -- do not edit.")
    w(" *")
    w(" * The in-race HUD's data, every number out of the shipped game: the")
    w(" * minimap's per-track world->art transform and its panel shape out of")
    w(" * RCCars.exe's .data, the screen layout out of the shipped Settings sliders, and the")
    w(" * engine's own Smash font out of Language/English. See the generator's")
    w(" * docstring for the address or the key behind each block. */")
    w("#ifndef HUD_DATA_H")
    w("#define HUD_DATA_H")
    w("")
    w("/* THE FRAME EVERY PIXEL POSITION BELOW IS IN. RC Cars is a 2003 PC game")
    w("   and its HUD was authored at 800x600: map.ini's two sliders run 0..800")
    w("   and 0..600 exactly, and cockpit.ini's float ranges are -100..800 and")
    w("   -100..600. race_ui.c scales UNIFORMLY off the height and anchors each")
    w("   group to the edge it was authored against, so 4:3 art is not stretched")
    w("   4/3 wider on a 16:9 panel -- the same rule hud.h states for the")
    w("   message layer. */")
    w("#define HUD_REF_W 800.f")
    w("#define HUD_REF_H 600.f")
    w("")
    w("/* ------------------------------------------------------- the minimap */")
    w("")
    w("/* map.ini mapPosX_0 / mapPosY_0, raw pixels: the panel's top-left corner.")
    w("   The _1 pair (620, 401) is the second player's, for a split screen this")
    w("   port does not have; there is no unsuffixed pair in the file, so _0 is")
    w("   what single-player uses. */")
    w("#define MAP_POS_X %.1ff" % cvt(m, "mapPosX_0"))
    w("#define MAP_POS_Y %.1ff" % cvt(m, "mapPosY_0"))
    w("")
    w("/* The panel's side, in the same pixels. 0x4b79ea puts every outline point")
    w("   through (u * %.1f - %.1f) * 1/512, i.e. it centres u about 0.5 and" % (span, half))
    w("   scales it to a half-extent of %.1f -- so the panel is %.0f px across in" % (half, span))
    w("   the authored frame. Measured off the game's OWN screenshot of its HUD")
    w("   (Textures.1/opt_cock_m_1.csi, the cockpit option preview): the silver")
    w("   frame there is 167 px tall and 182 wide, so the height closes to the")
    w("   pixel and the width is 9% over. The extra width is NOT modelled -- see")
    w("   known-issues.md. */")
    w("#define MAP_SIDE %.1ff" % span)
    w("")
    w("/* The panel's own shape, 0x56db70: eight (u, v) in its own 0..1 box, four")
    w("   rows of two, which 0x4b79ea draws as a strip of three quads -- a")
    w("   rounded rectangle with clipped corners. Emitted as the raw fractions;")
    w("   race_ui.c does the centring and the scale. */")
    w("#define MAP_OUTLINE_N %d" % MAP_OUTLINE_N)
    w("static const float MAP_OUTLINE[MAP_OUTLINE_N][2] = {")
    for u, v in outline:
        w("    { %.6ff, %.6ff }," % (u, v))
    w("};")
    w("")
    w("/* THE WORLD -> ART TRANSFORM, per track: three world (x, z) points and")
    w("   the three art (u, v) they land on. FUN_004a9840 copies row `track` of")
    w("   0x56c378 and of 0x56c468 (24 bytes each) beside loading")
    w("   `trackmap_<track+1>`, and FUN_004b8500 solves the affine over them --")
    w("   after checking the three world points are not coincident. Three pairs")
    w("   determine an affine exactly; race_ui.c solves the same one.")
    w("")
    w("   The generator CHECKS each row rather than trusting the pairing: a")
    w("   painted top-down map has to come out CONFORMAL -- one scale, one")
    w("   rotation, no shear, no mirror -- and it does on all ten. **The rotation")
    w("   is real and is per track**: the artists turned each painting to fit the")
    w("   panel, so a marker's heading has to go through the transform and cannot")
    w("   be derived from an axis convention (race_ui_map_heading). Per track:")
    for t in range(N_TRACKS):
        sx, sy, cos, u, v, rot = report[t]
        w("     %-10s trackmap_%-2d %-8s %5.1f x %5.1f m  cos %+.3f  "
          "+x at %+7.1f deg  start (%.3f, %.3f)"
          % (TRACK_NAMES[t], lvl[t] + 1, engine_disp[lvl[t]], 1 / sx, 1 / sy,
             cos, rot, u, v))
    w("")
    w("   THAT IS STILL THESE TWO TABLES AGAINST EACH OTHER. Held against a")
    w("   DIFFERENT shipped artifact -- the cp_N markers in each packed .vsc")
    w("   against the ribbon as painted -- rccars_re/mapcheck.py measures 0.3 m")
    w("   of error on beach_1 and 0.7 on beach_3, and 3 to 19 m on the other")
    w("   eight. The arithmetic below IS the engine's (FUN_004b8500 + 0x410430,")
    w("   read instruction by instruction); the residual is unexplained and it is")
    w("   in known-issues.md. Run mapcheck.py after touching any of this. */")
    w("typedef struct {")
    w("    float world[3][2];   /* x, z in metres */")
    w("    float art[3][2];     /* u, v in the 512x512 trackmap */")
    w("} map_calib;")
    w("")
    w("/* THE MAP NUMBER each of this port's tracks uses, and the engine's own")
    w("   name for it. The rows above are in THIS port's order (tracks.h's) with")
    w("   each one lifted from the engine level that track really is, so nothing")
    w("   downstream has to know the engine numbers a level differently -- but")
    w("   main.c still has to ASK for `trackmap_<n>` by the engine's number,")
    w("   because that is what the file is called. Read out of")
    w("   Scripts/championship.ini's own section comments and checked against a")
    w("   geometric fit; see engine_levels(). */")
    for t in range(N_TRACKS):
        w("#define MAP_TRACKMAP_%-10s %2d   /* %s */"
          % (TRACK_NAMES[t].upper(), lvl[t] + 1, engine_disp[lvl[t]]))
    w("static const int MAP_TRACKMAP[%d] = { %s };"
      % (N_TRACKS, ", ".join(str(lvl[t] + 1) for t in range(N_TRACKS))))
    w("")
    w("/* And the engine's own display names, which are not `Beach 1' at all. */")
    w("static const char *const MAP_TRACK_NAME[%d] = {" % N_TRACKS)
    for t in range(N_TRACKS):
        w('    "%s",%s/* %s */' % (engine_disp[lvl[t]],
                                   " " * max(1, 12 - len(engine_disp[lvl[t]])),
                                   TRACK_NAMES[t]))
    w("};")
    w("")
    w("#define MAP_N_TRACKS %d" % N_TRACKS)
    w("static const map_calib MAP_CALIB[MAP_N_TRACKS] = {")
    for t in range(N_TRACKS):
        P, Q = calib[t]
        w("    /* %s -- trackmap_%d, %s */"
          % (TRACK_NAMES[t], lvl[t] + 1, engine_disp[lvl[t]]))
        w("    { { { %11.5ff, %11.5ff }, { %11.5ff, %11.5ff }, "
          "{ %11.5ff, %11.5ff } }," % (P[0][0], P[0][1], P[1][0], P[1][1],
                                       P[2][0], P[2][1]))
        w("      { { %10.7ff, %10.7ff }, { %10.7ff, %10.7ff }, "
          "{ %10.7ff, %10.7ff } } }," % (Q[0][0], Q[0][1], Q[1][0], Q[1][1],
                                         Q[2][0], Q[2][1]))
    w("};")
    w("")
    w("/* ---------------------------------------- the badges and the clocks */")
    w("")
    w("/* cockpit.ini. The unsuffixed keys are single-player's; the _0 / _1 pairs")
    w("   are the split screen's two halves. Each is the top-left of a 128x128")
    w("   quad drawn 1:1 -- which is what makes the negative Y right rather than")
    w("   wrong: place1..6 and lap carry 30-odd px of transparent margin above")
    w("   their word art, so the badge's INK lands on screen with the quad's own")
    w("   top edge above it. */")
    for nm, key in (("HUD_PLACE_X", "placePosX"), ("HUD_PLACE_Y", "placePosY"),
                    ("HUD_LAP_X", "lapPosX"), ("HUD_LAP_Y", "lapPosY"),
                    ("HUD_LAPNUM_X", "lapNumPosX"),
                    ("HUD_LAPNUM_Y", "lapNumPosY")):
        w("#define %-16s %8.3ff   /* %s */" % (nm, cvt(ck, key), key))
    w("")
    w("/* The badge art is 128x128 with the word inside it -- place1..place6 and")
    w("   lap, all six places and the word, one texture each. */")
    w("#define HUD_BADGE_SIDE 128.f")
    w("")
    w("/* cockpit_time.ini. Two clocks, one above the other, both centred across")
    w("   the screen: the race total and the current lap. cockpit.ini has a")
    w("   timeLapPosY of its own (%g) and cockpit_time.ini overrides it; the file"
      % cvt(ck, "timeLapPosY"))
    w("   named for the feature wins. */")
    w("#define HUD_TIME_TOTAL_Y %8.3ff   /* timeTotalPosY */" % cvt(ct, "timeTotalPosY"))
    w("#define HUD_TIME_LAP_Y   %8.3ff   /* timeLapPosY */" % cvt(ct, "timeLapPosY"))
    w("")
    w("/* timeLapBlinkInSec: how long a just-completed lap time is held blinking")
    w("   before the live one takes the line back. */")
    w("#define HUD_LAP_BLINK %8.3ff" % cvt(ct, "timeLapBlinkInSec"))
    w("")
    w("/* ------------------------------------------------------- the gauges */")
    w("")
    w("/* cockpit.ini's `indicator` group: the sweep the needle runs through,")
    w("   degrees, zero straight up and positive clockwise. -34 is up-and-left")
    w("   and +127 is down-and-right, a 161 degree arc through the top -- which")
    w("   is exactly the arc the game's own HUD screenshot shows on both gauges")
    w("   (opt_cock_all.csi: grey, then gold over the top, then red down the")
    w("   right). indicatorShift is the offset from the gauge's own centre that")
    w("   the sweep is struck about. */")
    w("#define HUD_IND_ANG_MIN %8.3ff   /* indicatorAngMin */" % cvt(ck, "indicatorAngMin"))
    w("#define HUD_IND_ANG_MAX %8.3ff   /* indicatorAngMax */" % cvt(ck, "indicatorAngMax"))
    w("#define HUD_IND_SHIFT_X %8.3ff   /* indicatorShiftX */" % cvt(ck, "indicatorShiftX"))
    w("#define HUD_IND_SHIFT_Y %8.3ff   /* indicatorShiftY */" % cvt(ck, "indicatorShiftY"))
    w("")
    w("")
    w("/* THE DIAL'S OWN GEOMETRY, measured off cockpit_sp1 and cockpit_sp2")
    w("   rather than read, because no config file carries it. cockpit_sp1 is the")
    w("   dial -- a dark translucent disc, a silver rim and a GOLD tube running")
    w("   the whole sweep; cockpit_sp2 is the same ring in two tones, RED over")
    w("   the sweep and grey elsewhere. So the port draws sp1 whole as the track")
    w("   and reveals a SECTOR of sp2 up to the value: red filling along gold.")
    w("")
    w("   THE MEASUREMENT IS ITS OWN CHECK. sp1's gold spans %+.1f..%+.1f deg," % (dial["a0"], dial["a1"]))
    w("   sp2's red %+.1f..%+.1f, and cockpit.ini says %+.0f..%+.0f -- three" % (arc["a0"], arc["a1"], cvt(ck, "indicatorAngMin"), cvt(ck, "indicatorAngMax")))
    w("   independent statements of one sweep agreeing to four degrees, which is")
    w("   the tube's own end-cap rounding. The ini's pair is what the port uses;")
    w("   these two are the reason to believe it. Both rings are centred on their")
    w("   texture to within 0.006 of its side. */")
    w("#define HUD_DIAL_CU  %8.5ff   /* cockpit_sp2's fitted centre */" % arc["cu"])
    w("#define HUD_DIAL_CV  %8.5ff" % arc["cv"])
    w("#define HUD_DIAL_R0  %8.5ff   /* the ring's inner and outer radius, */" % min(dial["r0"], arc["r0"]))
    w("#define HUD_DIAL_R1  %8.5ff   /* fractions of the texture's side */" % max(dial["r1"], arc["r1"]))
    w("")
    w("/* WHERE THE TWO DIALS SIT, and this is the one block in this file with")
    w("   no config key behind it: cockpit.ini carries indicatorShiftX/Y and")
    w("   nothing positional, so the dials' screen place is hardcoded in the exe.")
    w("   It is MEASURED instead, off the game's own full-HUD screenshot -- the")
    w("   cockpit option preview Textures.1/opt_cock_all.csi, a 250x191 render of")
    w("   the 800x600 frame, which shows the whole thing: `3rd' top left, the two")
    w("   clocks top centre, `Lap 1/6' top right, `100 Boost' bottom left and")
    w("   `29 mph' bottom right.")
    w("")
    w("   Three numbers, each the most robust landmark for its axis. The ring's")
    w("   outer radius and its centre Y come out of the arc's VERTICAL extent,")
    w("   which runs from its topmost point down to its %+.0f end: 1.602 R, and" % cvt(ck, "indicatorAngMax"))
    w("   415..562 in the shot. The centre X is the dark disc's own midpoint,")
    w("   22..152. race_ui.c derives the quad's side from R and the art's own")
    w("   outer radius above, so the two cannot disagree.")
    w("")
    w("   THE SHOT SHOWS BOTH THE DIAL AND THE MAP PANEL ABOUT A TENTH WIDER THAN")
    w("   TALL against art that is square in both cases (168x147 and 182x167).")
    w("   Something in the original's 2D path is not a uniform scale and what it")
    w("   is has not been recovered; sqrt(4/3) = 1.155 fits the dial to 1.5% and")
    w("   the map to 6%, which is a guess and is not applied. Both are drawn")
    w("   SQUARE. See known-issues.md. */")
    w("#define HUD_DIAL_CX    87.f    /* the boost dial's ring centre */")
    w("#define HUD_DIAL_CY   507.f")
    w("#define HUD_DIAL_R_PX  92.f    /* its outer radius, same pixels */")
    w("")
    w("/* ------------------------------- the direction arrow and WRONG WAY */")
    w("")
    w("/* THE `DirectArrow\' -- the engine\'s own next-corner arrow, and the")
    w("   WRONG WAY banner that shares its updater. Both are recovered whole.")
    w("")
    w("   The arrow is TWO CHEVRONS in a scene of their own, drawn through a")
    w("   camera of their own into a small square viewport at the bottom of the")
    w("   screen. The pieces and where each came from:")
    w("")
    w("     RCCarsDB/cockpit.sb   the geometry -- nodes `extru9\' and")
    w("                           `extru9_3\' under `navigation_arrows_1\', the")
    w("                           two names 0x4af44c looks up and the two the")
    w("                           error string \"can\'t locate extru9 and")
    w("                           extru9_3 on cockpit arrow\" names")
    w("     Textures.1/coc_str_2  the art: a 128x128 grey chevron on alpha")
    w("     Settings/arrow.ini    the camera and its viewport")
    w("     Config.gm Cockpit/DirectArrow   whether it is shown at all (1)")
    w("     RCCars.exe 0x4b1360   the eight sliders and what they are put through")
    w("     RCCars.exe 0x4e8f00   the angle, the tint, the blink and the wrong way")
    w("     RCCars.exe 0x4e94e0   what the drawer asks for, off a 44-byte record")
    w("     RCCars.exe 0x4b0fa0   the draw, and 0x4b024e the banner\'s post */")
    w("")
    w("/* arrow.ini, through FUN_004b1360. NearPlane is raw * %g, the three" % milli)
    w("   fractions raw * %g, pos_y/pos_z (raw - %g) * %g and dir raw - %g." % (centi, pos_bias, centi, dir_bias))
    w("   AngleVert is used RAW, as the camera\'s vertical field of view, with an")
    w("   aspect of exactly 1. */")
    w("#define ARW_NEAR      %8.4ff   /* NearPlane */" % arw["near"])
    w("#define ARW_FOV_Y     %8.3ff   /* AngleVert, degrees */" % arw["fov"])
    w("")
    w("/* WHERE IT SITS: FUN_004b1360 builds the camera\'s viewport as the rect")
    w("   (CX - Len/2, CY - Len/2, Len, Len) in fractions of the screen, so Len")
    w("   is the SIDE of a square centred on (CX, CY) -- %g of the screen at" % arw["len"])
    w("   (%g, %g), i.e. the bottom centre. FUN_00471380 turns that into pixels" % (arw["cx"], arw["cy"]))
    w("   and 0x4b115d clears it before the arrow is drawn into it. */")
    w("#define ARW_CX        %8.4ff   /* CX */" % arw["cx"])
    w("#define ARW_CY        %8.4ff   /* CY */" % arw["cy"])
    w("#define ARW_LEN       %8.4ff   /* Len */" % arw["len"])
    w("")
    w("/* THE CAMERA. It sits at (0, pos_y, pos_z) and its axis is")
    w("   (0, cos dir, sin dir) -- so it looks back along the NEGATIVE of that,")
    w("   down at the chevrons lying in the y = %g plane at the origin. The" % chev_y)
    w("   generator checks that aim: the ray crosses y = 0 at z = %+.3f, which is" % _hit)
    w("   inside the chevrons\' own footprint, and that is what says `dir\' is an")
    w("   elevation rather than a heading.")
    w("")
    w("   dir is measured from the horizon, so the arrow\'s plane is seen at")
    w("   %.1f degrees and a step ALONG z foreshortens on screen by cos dir =" % (90.0 - arw["dir"]))
    w("   %.4f while a step along x does not. dirarrow.c uses exactly that." % _cd)
    w("   */")
    w("#define ARW_CAM_Y     %8.4ff   /* pos_y */" % arw["pos_y"])
    w("#define ARW_CAM_Z     %8.4ff   /* pos_z */" % arw["pos_z"])
    w("#define ARW_CAM_DIR   %8.3ff   /* dir, degrees above the horizon */" % arw["dir"])
    w("")
    w("/* THE TWO CHEVRONS, out of cockpit.sb: six vertices and four triangles")
    w("   each, flat at one y, one behind the other along z. -z is AWAY from the")
    w("   camera, so the apex being the most-negative z is what says they point")
    w("   FORWARD -- the mesh settles it, not the art.")
    w("")
    w("   Each one\'s UV is an exact AFFINE function of (x, z) -- the generator")
    w("   fits it and refuses a mesh where it is not -- so a chevron is ONE quad")
    w("   with a UV rect and the V is carved by coc_str_2\'s own alpha. The rect")
    w("   below comes out the chevron\'s own ink box in the art, which is a free")
    w("   check on the whole reading. Note u runs BACKWARDS against x: the art is")
    w("   sampled mirrored, and the chevron is symmetric so it does not show. */")
    w("#define ARW_CHEV_N %d" % len(chev))
    w("#define ARW_CHEV_X0  %8.4ff" % chev_x[0])
    w("#define ARW_CHEV_X1  %8.4ff" % chev_x[1])
    w("#define ARW_CHEV_U0  %8.5ff   /* at x = ARW_CHEV_X0 */" % chev_uv[0])
    w("#define ARW_CHEV_U1  %8.5ff   /* at x = ARW_CHEV_X1 */" % chev_uv[1])
    w("#define ARW_CHEV_V0  %8.5ff   /* at the APEX, z = ARW_CHEV_Z[i][0] */" % chev_uv[2])
    w("#define ARW_CHEV_V1  %8.5ff   /* at the near edge, z = [i][1] */" % chev_uv[3])
    w("#define ARW_CHEV_Y   %8.4ff" % chev_y)
    w("static const float ARW_CHEV_Z[ARW_CHEV_N][2] = {")
    for lo, hi, nm in chev:
        w("    { %8.4ff, %8.4ff },   /* %s */" % (lo, hi, nm))
    w("};")
    w("#define ARW_CHEV_TEX \"coc_str_2\"")
    w("")
    w("/* THE ANGLE. FUN_00410150 flattens the car\'s forward and the direction")
    w("   to the aim point into XZ, takes acos of their normalised dot and")
    w("   returns DEGREES clamped to 0..180 -- so the engine\'s own target angle")
    w("   is UNSIGNED and its arrow leans the same way for a left turn and a")
    w("   right one. dirarrow.h gives it the sign and says why.")
    w("")
    w("   The shown angle CHASES that target through FUN_0040c580, a wrap-aware")
    w("   move-towards on degrees with a 360 modulus, at ARW_SLEW degrees a")
    w("   second -- times ARW_SNAP_MUL while a snap is latched, which clears when")
    w("   the two are within ARW_ANG_EPS. FUN_0040cc60 then multiplies by pi/180")
    w("   and builds a rotation about Y, which is why the degrees are degrees. */")
    w("#define ARW_SLEW      %8.3ff   /* record +0x00, deg/s */" % arw_slew)
    w("#define ARW_SNAP_MUL  %8.3ff   /* 0x554490 */" % arw_snap)
    w("#define ARW_ANG_EPS   %8.3ff   /* 0x554390, deg */" % arw_eps)
    w("")
    w("/* THE TINT, and it is the arrow\'s second reading. Both chevrons get one")
    w("   D3DCOLOR built at 0x4b10d4 from the SMOOTHED progress p through the")
    w("   current checkpoint-to-checkpoint segment:")
    w("")
    w("       R = min(%g * (1 - p), %d)   G = min(%g * p, %d)   B = 0   A = 255" % (tint_full, 255, tint_full, 235))
    w("")
    w("   so p = 0 is RED, p = 1/2 is YELLOW and p = 1 is GREEN -- red just after")
    w("   a checkpoint, green as the next one comes up. The two ceilings really")
    w("   are different (255 and 235) and are the exe\'s own `cmp\' immediates.")
    w("")
    w("   p chases the raw fraction at ARW_CHASE per second, and the WRONG WAY")
    w("   state forces the raw fraction to 0, which is what turns the arrow red")
    w("   the moment the banner comes up. */")
    w("#define ARW_TINT_FULL %8.1ff   /* 0x5547f4 */" % tint_full)
    w("#define ARW_TINT_R_CAP   %d       /* 0x4b10e5 */" % 255)
    w("#define ARW_TINT_G_CAP   %d       /* 0x4b10fb */" % 235)
    w("#define ARW_CHASE     %8.3ff   /* record +0x04, per second */" % arw_chase)
    w("")
    w("/* THE BLINK. Record +0x10 counts down by dt; when it goes negative it is")
    w("   set to ARW_STEADY, or to ARW_BLINK_CYCLE if the car is going the wrong")
    w("   way or the follower\'s own +0x58 is under ARW_BLINK_NEAR. The arrow is")
    w("   drawn while the counter is at or under ARW_BLINK_SHOW -- which is")
    w("   exactly half of ARW_BLINK_CYCLE, so it is a %g s cycle half on, and a" % bl_cycle)
    w("   negative counter is on for good.")
    w("")
    w("   AND +0x58 IS IDENTIFIED: it is the CLOSEST the car has come to the")
    w("   checkpoint it is heading for -- a running minimum, in metres, in 3D.")
    w("   FUN_004eb550 computes the straight-line distance to checkpoint")
    w("   `record+0x4c\', writes it to +0x54, and folds it into +0x58 with a")
    w("   min(); FUN_004ea8d0 resets +0x58 to ARW_MIN_INIT when the checkpoint")
    w("   advances. This file used to say the field was unrecovered and that the")
    w("   port was reading it as a distance on circumstantial grounds; the")
    w("   reading was right and the DETAIL was not -- it is the minimum and not")
    w("   the current distance, so once the car has been within ARW_BLINK_NEAR")
    w("   the arrow keeps blinking until the checkpoint changes rather than")
    w("   stopping as the car drives away.")
    w("")
    w("   THE SAME FUNCTION CARRIES A CUE THE PORT HAD NEVER RAISED. Once per")
    w("   checkpoint (+0x5c latches it), when the minimum is under")
    w("   ARW_BESIDE_IN and the CURRENT distance is back over ARW_BLINK_NEAR --")
    w("   4 m in, 5 m out -- it fires message-system sound %#x, and the shipped" % cue_beside)
    w("   bank has exactly one name for that: `cp_beside.wav\'. The id next to")
    w("   it, %#x, is fired where a checkpoint is passed and is `cp\'. */" % cue_cp)
    w("#define ARW_BLINK_SHOW  %8.4ff   /* 0x554460 */" % bl_show)
    w("#define ARW_BLINK_CYCLE %8.4ff   /* 0x4e9276 */" % bl_cycle)
    w("#define ARW_STEADY      %8.4ff   /* 0x4e9258 */" % steady)
    w("#define ARW_BLINK_NEAR  %8.4ff   /* 0x554730, metres */" % bl_near)
    w("#define ARW_BESIDE_IN   %8.4ff   /* 0x5543f0, metres */" % bl_in)
    w("#define ARW_MIN_INIT    %8.1ff   /* 0x4ea9c8, the min\'s sentinel */" % bl_min0)
    w("")
    w("/* THE WRONG WAY. dot = (direction to the aim point) . (car forward),")
    w("   both flattened to XZ. The timer rises by dt while dot < 0 and falls by")
    w("   ARW_WRONG_DECAY * dt while dot >= 0, held inside [0, ARW_WRONG_CAP];")
    w("   the banner is up while the timer is over ARW_WRONG_ON -- AND the car is")
    w("   at least ARW_WRONG_NEAR from the checkpoint it last passed, which is")
    w("   what keeps a pass from raising it. */")
    w("#define ARW_WRONG_ON    %8.3ff   /* 0x554498, seconds */" % wr_on)
    w("#define ARW_WRONG_CAP   %8.3ff   /* 0x554928 */" % wr_cap)
    w("#define ARW_WRONG_DECAY %8.3ff   /* 0x5543f4 */" % wr_dec)
    w("#define ARW_WRONG_NEAR  %8.3ff   /* 0x55472c, metres */" % wr_near)
    w("")
    w("/* THE BANNER is message slot %d of the ELEVEN-slot layer msg.h owns --" % MSG_WRONG_SLOT)
    w("   say so rather than the name: slot %d\'s texture index picks the THIRD" % MSG_WRONG_SLOT)
    w("   of the six textures 0x4af195 loads, which is msg_wrong_way. Its size")
    w("   is the pair at 0x56d278 and its UV rect is the WHOLE texture, so unlike")
    w("   msg_hits and msg_321_s_f this one is not an atlas.")
    w("")
    w("   0x4b024e posts it with a life of ARW_MSG_LIFE and animate = 1, every")
    w("   frame the flag is up -- and the poster DROPS a post of the slot already")
    w("   showing (0x4afbde), so the banner runs its life out and starts again")
    w("   rather than being pinned at the start of its own grow-in. That is what")
    w("   makes it PULSE. The grow-in is elapsed / max(life, ARW_MSG_GROW), about")
    w("   the rect\'s own centre, and the colour is a flat white.")
    w("")
    w("   ARW_MSG_SND is the cue 0x4b0311 raises on the first frame of each post,")
    w("   which is the port\'s cp_wrongway.")
    w("")
    w("   AND THE FOURTH ARGUMENT IS A HOLD-OVER, not an unread one -- this file")
    w("   said no drawer read it and no drawer does; the RETIRE step at 0x4af28e")
    w("   does. It runs `life -= dt\' over the three player slots and clears the")
    w("   slot when `life < -hold\', while the drawer only draws while")
    w("   `life > 0\'. So slot %d is SELECTED for %g + %g = %g s and VISIBLE for" % (MSG_WRONG_SLOT, msg_life, f32(MSG_WRONG_ARG4_VA), msg_life + f32(MSG_WRONG_ARG4_VA)))
    w("   the first %g of them: the banner shows, goes away for %g s, and is" % (msg_life, f32(MSG_WRONG_ARG4_VA)))
    w("   posted again -- one flash and one beep every %g s, not every %g." % (msg_life + f32(MSG_WRONG_ARG4_VA), msg_life))
    w("   The countdown\'s four posts pass a hold of 0 (`xor ebp,ebp\' at")
    w("   0x4e1c2a), so those retire the instant their life runs out. */")
    w("#define ARW_MSG_SLOT      %d" % MSG_WRONG_SLOT)
    w("#define ARW_MSG_LIFE  %8.3ff   /* 0x4b0256 */" % msg_life)
    w("#define ARW_MSG_HOLD  %8.3ff   /* 0x4b0251 */" % f32(MSG_WRONG_ARG4_VA))
    w("#define ARW_MSG_ANIM      1       /* 0x4b024e */")
    w("#define ARW_MSG_SND     0x%03x     /* 0x4b0312 */" % msg_snd)
    w("")
    w("/* ------------------------------------- the engine's MESSAGE LAYER */")
    w("")
    w("/* THE WHOLE OF IT, and it is eleven slots over six textures. 0x4af195")
    w("   loads the six by name into a handle table; FUN_004b11e0 draws ONE slot")
    w("   over them per player, taking its size from 0x56d278, its UV rect from")
    w("   0x56d328, its texture index from 0x56d2d0 and -- through the poster")
    w("   FUN_004afbb0 -- its right to the screen from the priority at 0x56d2fc.")
    w("")
    w("   THE POSTER IS THE ARBITRATION, and it is three rules:")
    w("     - a post of the slot ALREADY showing is DROPPED (0x4afbde), which is")
    w("       what makes a message that is re-posted every frame pulse instead of")
    w("       sitting pinned at the start of its own grow-in;")
    w("     - a post is dropped when the showing slot\'s priority is strictly")
    w("       HIGHER (0x4afbfc), so equal priorities replace each other;")
    w("     - otherwise it takes the screen, with its life, its hold-over, its")
    w("       animate flag and a one-shot sound latch.")
    w("")
    w("   The BAND is the player index: 0 the whole screen, 1 the top half, 2 the")
    w("   bottom half, and the drawer\'s own yoff table (0x149dd44) is in the")
    w("   zero-filled part of .data, so the three are exactly (1-h)/2, (0.5-h)/2")
    w("   and (1.5-h)/2. A single player is 0.")
    w("")
    w("   The GROW-IN is `elapsed / max(life, MSG_GROW_FLOOR)\' about the rect\'s")
    w("   own centre (FUN_004b12b0), applied only when animate is 1, and the")
    w("   colour is a flat white at every reading -- the original never fades a")
    w("   message. */")
    w("#define MSG_N_SLOTS  %d" % MSG_N_SLOTS)
    w("#define MSG_N_TEX    %d" % len(MSG_TEX_NAMES))
    w("#define MSG_GROW_FLOOR %6.3ff   /* 0x5544a0 */" % msg_grow)
    w("")
    w("/* The six, in the order 0x4af195 loads them -- which IS the texture index")
    w("   the slot table uses, so nothing downstream needs a second table. */")
    w("static const char *const MSG_TEX_NAME[MSG_N_TEX] = {")
    for nm in MSG_TEX_NAMES:
        w("    \"%s\"," % nm)
    w("};")
    w("")
    w("/* Per slot: which of the six, its size as a fraction of the screen, its UV")
    w("   rect, and its priority. `w\' and `h\' ENCODE THE ART\'S ASPECT against the")
    w("   4:3 frame they were authored in -- msg.c keeps h and derives w from the")
    w("   pair, so a 16:9 panel does not stretch the word art 4/3 wider. */")
    w("typedef struct {")
    w("    int   tex;              /* index into MSG_TEX_NAME */")
    w("    float w, h;             /* fractions of the authored 4:3 screen */")
    w("    float u0, v0, u1, v1;   /* the cell, for the atlases */")
    w("    int   prio;             /* higher wins; equal replaces */")
    w("} msg_slot_def;")
    w("static const msg_slot_def MSG_SLOT[MSG_N_SLOTS] = {")
    for sl, (ti, sw, sh, u, pr) in enumerate(msg_slots):
        w("    { %d, %.5ff, %.5ff, %.4ff, %.4ff, %.4ff, %.4ff, %d },   "
          "/* %2d %s */"
          % (ti, sw, sh, u[0], u[1], u[2], u[3], pr, sl, MSG_TEX_NAMES[ti]))
    w("};")
    w("")
    w("/* The slots this port raises, by name rather than by number. The five over")
    w("   msg_321_s_f are countdown.c\'s and the two over msg_hits are hud.c\'s;")
    w("   those two files still draw their own and are not on msg.c yet. */")
    w("#define MSG_LOW_SIGNAL  0")
    w("#define MSG_PAUSE       1")
    w("#define MSG_WRONG_WAY   2")
    w("#define MSG_HIT         3")
    w("#define MSG_GREAT_HIT   4")
    w("#define MSG_FINISH      8")
    w("#define MSG_BEST_LAP   10")
    w("")
    w("/* ------------------------------------------- the engine's own font */")
    w("")
    w("/* Language/English/Smash20 and Smash26: a 256x256 atlas each, %d glyphs" % len(chars))
    w("   on a 10 x 9 grid, with the character ORDER in the matching .dat. The")
    w("   grid is not assumed -- the empty bands in the atlas alpha straddle")
    w("   exactly 256/10 and 256/9 -- and the two files' .dat are byte-identical,")
    w("   so one lookup serves both sizes.")
    w("")
    w("   THE GAME HAS A FONT AND THIS PORT SPENT YEARS SAYING IT DID NOT. ui.h")
    w("   used to explain that `the game ships no reusable glyph atlas'; the claim")
    w("   was made off Textures.1/2/3 and Interface.sb, and the two atlases are in")
    w("   Language/English/, a directory these notes had never listed. Both that")
    w("   comment and gen_font.py's now say so. See traps.md. */")
    small, big = fonts["SMALL"], fonts["BIG"]
    w("#define SF_COLS 10")
    w("#define SF_ROWS 9")
    w("#define SF_N    %d" % len(chars))
    w("#define SF_ATLAS %d   /* both atlases are square and this size */"
      % fonts["BIG"][4])
    w("")
    w("/* Per glyph, the INK's left and right edge as a fraction of its own cell.")
    w("   Measured off the atlas alpha: the art is left-aligned in the cell with")
    w("   a ragged right edge, so a monospaced advance spaces `1/6` and a clock")
    w("   out into nonsense. The two sizes are measured separately because their")
    w("   art is not a scaled copy. */")
    for nm in ("SMALL", "BIG"):
        base, fi, _c, ink, W, H = fonts[nm]
        w("/* %s */" % base)
        w("static const float SF_INK_%s[SF_N][2] = {" % nm)
        for i in range(0, len(ink), 5):
            row = ink[i:i + 5]
            w("    " + " ".join("{ %.5ff, %.5ff }," % (l, r) for l, r in row)
              + "   /* " + "".join(chr(chars[i + k]) if 32 < chars[i + k] < 127
                                   else "?" for k in range(len(row))) + " */")
        w("};")
    w("")
    w("/* char -> glyph index, or -1. Built here rather than searched at runtime:")
    w("   the order is the .dat's and it is not sorted. */")
    idx = [-1] * 256
    for i, c in enumerate(chars):
        idx[c] = i
    w("static const signed char SF_INDEX[256] = {")
    for i in range(0, 256, 16):
        w("    " + " ".join("%3d," % v for v in idx[i:i + 16]))
    w("};")
    w("")
    for nm in ("SMALL", "BIG"):
        base, fi, _c, _ink, W, H = fonts[nm]
        p = "SF_" + nm
        w("/* %s.ini. letSizeX/Y are the cell as DRAWN, in pixels of the 800x600" % base.lower())
        w("   frame -- 25 x 28 against the atlas's own 25.6 x 28.44, so the art is")
        w("   drawn at very nearly 1:1. letSpace is the tracking added after each")
        w("   glyph's ink, spaceLen the width of a space, lineSpace the extra")
        w("   between lines, letBaseLine the font's nominal size. */")
        w("#define %s_SIZE_X   %8.3ff   /* letSizeX */" % (p, cvt(fi, "letSizeX")))
        w("#define %s_SIZE_Y   %8.3ff   /* letSizeY */" % (p, cvt(fi, "letSizeY")))
        w("#define %s_SPACE    %8.3ff   /* letSpace */" % (p, cvt(fi, "letSpace")))
        w("#define %s_SPACE_LEN %7.3ff   /* spaceLen */" % (p, cvt(fi, "spaceLen")))
        w("#define %s_LINE     %8.3ff   /* lineSpace */" % (p, cvt(fi, "lineSpace")))
        w("#define %s_BASELINE %8.3ff   /* letBaseLine */" % (p, cvt(fi, "letBaseLine")))
        w("#define %s_TEX \"%s\"" % (p, base))
        w("")
    w("#endif /* HUD_DATA_H */")

    open(a.out, "w").write("\n".join(o) + "\n")
    print("wrote %s: %d tracks, %d outline points, %d glyphs"
          % (a.out, N_TRACKS, MAP_OUTLINE_N, len(chars)))


if __name__ == "__main__":
    main()
