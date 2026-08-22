#!/usr/bin/env python3
"""
Generate fx_data.h -- the constants behind the port's three missing effects
(wheel dust, exhaust smoke, tyre marks) and the car body's env-map glance.

Same shape as gen_vis_data.py: read the game's own Settings/*.ini slider
definitions with Settings/*.crs runtime values layered on, apply the conversion
the loader actually performs, and emit a header. Nothing is invented; every
conversion is cited to the function it came out of.

All four particle/trace loaders go through ONE helper, FUN_00532410:

    float cfgGetScaled(handle, section, key, _, scale)
    {
        if (!cfgGetInt(3, section, key, &raw)) return 0.0f;   /* NOTE: 0, not
                                                                the default */
        return raw / scale;
    }

so every value below is `raw / scale` and the scale is a literal float pushed at
the call site. `scanned` by rccars_re/gen_fx_data.py's own table, which was read
off the disassembly rather than guessed -- a negative scale is a sign flip as
well (car_dustx GravityY divides by -100).

  FUN_0052ea60   "car_dustx"       the wheel dust system
  FUN_005308f0   "exhausted_gas"   the exhaust smoke system
  FUN_005300e0   "car_trace"       the tyre marks
  FUN_0052ed30   "surf_<kind>"     per-surface dust colour, intensity and life
  FUN_004f9aa0   "CarEnvMap"       the body glance alphas, stored as raw bytes

Curves are the .spl text format: a segment count, then four control points per
cubic Bezier segment, each segment starting where the last ended. They are
sampled into the port's existing piecewise-linear rb_curve, which is what
gen_rb_data.py already does with the .bspl curves.

  dust_int.spl    km/h -> dust particles per second (percent)
  dust_speed.spl  km/h -> percent of the wheel velocity a dust particle keeps
  eg_int.spl      km/h -> exhaust particles per second (percent)

Usage: python3 gen_fx_data.py [--dir SETTINGS] [--spl SPLINES] [-o OUT]
"""

import argparse
import os
import re

from dump_physics import read_crs
from gen_vis_data import read_ini

# ---------------------------------------------------------------------------
# key -> scale, read off the push immediates at each FUN_00532410 call site.
# ---------------------------------------------------------------------------

DUSTX = [
    ("GravityY",         -100.0, "GRAVITY_Y",      "m/s^2 on the dust, +y"),
    ("DX",                 10.0, "DX",             "system +0xa0; not recovered"),
    ("DY",                 10.0, "DY",             "system +0xac; not recovered"),
    ("TwistSpeed",        100.0, "TWIST",          "sprite spin, turns/s"),
    ("ScaleX",            100.0, "SCALE_X",        "sprite half-width"),
    ("ScaleXDisp",        100.0, "SCALE_X_DISP",   "+- random on ScaleX"),
    ("ScaleY",            100.0, "SCALE_Y",        "sprite half-height"),
    ("ScaleYDisp",        100.0, "SCALE_Y_DISP",   "+- random on ScaleY"),
    ("DynamicScale",       10.0, "DYN_SCALE",      "growth over life, spinning"),
    ("DynamicScaleForw",   10.0, "DYN_SCALE_FORW", "growth over life, rolling"),
    ("SpeedAtt",          100.0, "SPEED_ATT",      "velocity decay per second"),
    ("WheelSpinThresh",   100.0, "SPIN_THRESH",    "slip above which it is spin"),
    ("WheelSpinScale",    100.0, "SPIN_SCALE",     "rate multiplier when spinning"),
    ("WheelSpinSpeed",    100.0, "SPIN_SPEED",     "throw-back speed per unit slip"),
    ("ShiftX",            100.0, "SHIFT_X",        "front pair emit shift, inboard"),
    ("ZIgnoreRad",         10.0, "ZIGNORE_RAD",    "hide a particle nearer than this"),
]

EG = [
    ("GravityY",     -100.0, "GRAVITY_Y",     "m/s^2 on the smoke, +y"),
    ("DX",             10.0, "DX",            "system +0xa0; not recovered"),
    ("DY",             10.0, "DY",            "system +0xac; not recovered"),
    ("TwistSpeed",    100.0, "TWIST",         "sprite spin, turns/s"),
    ("ScaleX",        100.0, "SCALE_X",       "sprite half-width"),
    ("ScaleXDisp",    100.0, "SCALE_X_DISP",  "+- random on ScaleX"),
    ("ScaleY",        100.0, "SCALE_Y",       "sprite half-height"),
    ("ScaleYDisp",    100.0, "SCALE_Y_DISP",  "+- random on ScaleY"),
    ("DynamicScale",   10.0, "DYN_SCALE",     "growth over life"),
    ("Speed",         100.0, "SPEED",         "ejection speed along the pipe"),
    ("SpeedAtt",      100.0, "SPEED_ATT",     "velocity decay per second"),
    ("TimeLife",      100.0, "LIFE",          "seconds"),
    ("TimeLifeDisp",  100.0, "LIFE_DISP",     "+- random on TimeLife"),
    ("ExplodeTime",   100.0, "EXPLODE_TIME",  "backfire hold, seconds"),
    ("ExplodeColor",  100.0, "EXPLODE_COLOR", "backfire grey floor, 0..1"),
    ("ExplodeAlpha",  100.0, "EXPLODE_ALPHA", "unused by the port"),
    ("WheelSpin",     100.0, "SPIN_THRESH",   "slip above which the pipe puffs"),
]

TRACE = [
    ("timeLife",      1.0, "LIFE",        "seconds a mark survives"),
    ("maxLen",      100.0, "MAX_LEN",     "metres between samples before a break"),
    ("scaleCoeff",  100.0, "SCALE_COEFF", "texture repeats per metre"),
]

SURFACES = [
    (0, "surf_default",  "DEFAULT"),
    (1, "surf_sand",     "SAND"),
    (2, "surf_wetsand",  "WETSAND"),
    (3, "surf_dunesand", "DUNESAND"),
    (5, "surf_gravel",   "STONEROAD"),
]

SURF_KEYS = [
    ("DustX_IntScale",     100.0),
    ("DustX_ColorR",         1.0),
    ("DustX_ColorG",         1.0),
    ("DustX_ColorB",         1.0),
    ("DustX_ColorA",         1.0),
    ("DustX_TimeLife",     100.0),
    ("DustX_TimeLifeDisp", 100.0),
]

ENVMAP = [
    ("EnvMapBody",     "BODY",     "the painted shell"),
    ("EnvMapGRE1",     "GRE1",     "*_GRE1 / *_GER1 -- glass and chrome"),
    ("EnvMapGRE2",     "GRE2",     "*_GRE2 / *_GER2 -- lamps, bumpers, underside"),
    ("EnvMapUpgrades", "UPGRADES", "the bolt-on exhaust"),
]


# ---------------------------------------------------------------------------


class Section:
    """One settings group: .ini defaults with .crs runtime values layered on.

    Deliberately keeps EVERY value a duplicated key was given, because
    car_trace.ini declares `maxLen` twice with different ranges and the second
    one is load-bearing -- see maxheight() below.
    """

    def __init__(self, sdir, base):
        self.name = base
        self.vals = {}
        self.ini_all = {}
        ini = os.path.join(sdir, base + ".ini")
        crs = os.path.join(sdir, base + ".crs")
        if os.path.exists(ini):
            key = None
            for line in open(ini, "r", errors="replace"):
                line = line.strip()
                m = re.match(r'^Key\s+"(.*)"$', line)
                if m:
                    key = m.group(1)
                    continue
                m = re.match(r"^Values\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)$", line)
                if m and key:
                    lo, hi, d = (int(x) for x in m.groups())
                    self.ini_all.setdefault(key, []).append((lo, hi, d))
                    key = None
            for k, v in self.ini_all.items():
                self.vals[k] = v[0][2]          # the FIRST declaration wins
        if os.path.exists(crs):
            for k, (_lo, _hi, v) in read_crs(crs).items():
                self.vals[k] = v

    def raw(self, key):
        if key in self.vals:
            return self.vals[key]
        for k, v in self.vals.items():
            if k.lower() == key.lower():
                return v
        # FUN_00532410 returns 0.0 when cfgGetInt fails -- not the .ini default,
        # because there is no .ini default to find.
        return None


def scaled(sec, key, scale):
    raw = sec.raw(key)
    v = 0.0 if raw is None else raw / scale
    return 0.0 if v == 0.0 else v          # kill a signed zero from a -ve scale


def maxheight(trace):
    """car_trace's fourth key, and the one honest judgement call in this file.

    FUN_005300e0 reads `maxHeight` with a scale of 1000, and there is no
    `maxHeight` anywhere: not in car_trace.ini, not in any .crs, and no
    car_trace.crs exists at all. Read literally the engine gets 0.0.

    But car_trace.ini declares `maxLen` TWICE -- once over 0..200 defaulting to
    100, once over 0..1000 defaulting to 90 -- and 1000 is exactly the scale
    maxHeight is read with. The second entry is the misnamed one, so the
    intended value is 90/1000 = 0.09 m.

    It matters because maxHeight is what lets a straight trail fit in a 32-slot
    ring: FUN_0052f700 REWRITES the head segment instead of allocating a new one
    when the new sample is within maxHeight of the line through the last two
    (FUN_0052fb60 returning 0). At maxHeight 0 nothing is ever collinear enough,
    every sample allocates, and the ring holds about a second of driving. So
    reading it literally does not reproduce the original either -- it disables a
    mechanism the original clearly has.
    """
    dups = trace.ini_all.get("maxLen", [])
    for lo, hi, d in dups:
        if hi == 1000:
            return d / 1000.0, d
    return 0.0, None


def read_spl(path):
    """One .spl: a segment count, then 4 control points per cubic segment.

    -> [(x, y)] * 4 per segment.
    """
    nums = []
    for line in open(path, "r", errors="replace"):
        parts = line.split()
        if len(parts) == 2:
            try:
                nums.append((float(parts[0]), float(parts[1])))
            except ValueError:
                pass
    n = 0
    with open(path, "r", errors="replace") as f:
        for line in f:
            line = line.strip()
            if line:
                n = int(float(line))
                break
    segs = []
    for i in range(n):
        seg = nums[i * 4:i * 4 + 4]
        if len(seg) == 4:
            segs.append(seg)
    return segs


def sample_spl(segs, per_seg=8):
    """Bezier chain -> a monotonic-in-x polyline for rb_curve_eval.

    rb_curve_eval walks the points expecting `in` to increase, so anything that
    doubles back in x is dropped. These three curves have non-monotonic CONTROL
    points but monotonic curves, so nothing is normally dropped; the count is
    reported so a curve that does double back cannot go by unnoticed.
    """
    pts, dropped = [], 0
    for si, seg in enumerate(segs):
        (x0, y0), (x1, y1), (x2, y2), (x3, y3) = seg
        first = 0 if si == 0 else 1
        for k in range(first, per_seg + 1):
            t = k / float(per_seg)
            mt = 1.0 - t
            b0 = mt * mt * mt
            b1 = 3.0 * mt * mt * t
            b2 = 3.0 * mt * t * t
            b3 = t * t * t
            x = b0 * x0 + b1 * x1 + b2 * x2 + b3 * x3
            y = b0 * y0 + b1 * y1 + b2 * y2 + b3 * y3
            if pts and x <= pts[-1][0]:
                dropped += 1
                continue
            pts.append((x, y))
    return pts, dropped


# ------------------------------------------------------------------ output

def emit(f, name, value, comment):
    if isinstance(value, float):
        f.write(f"#define {name:<26} {value:.6f}f   /* {comment} */\n")
    else:
        f.write(f"#define {name:<26} {value}   /* {comment} */\n")


def emit_curve(f, name, pts):
    body = ", ".join(f"{{{a:.4f}f,{b:.4f}f}}" for a, b in pts)
    f.write(f"static const rb_curve_pt {name}[] = {{{body}}};\n")
    f.write(f"#define {name.upper()}_N {len(pts)}\n\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", default="/mnt/c/tools/rccars_extracted/Settings")
    ap.add_argument("--spl", default="/mnt/c/tools/rccars_extracted/Splines")
    ap.add_argument("-o", "--out", default="/mnt/c/tools/rccars_vita/fx_data.h")
    a = ap.parse_args()

    dustx = Section(a.dir, "car_dustx")
    eg = Section(a.dir, "exhausted_gas")
    trace = Section(a.dir, "car_trace")
    envmap = Section(a.dir, "carEnvMap")

    f = open(a.out, "w")
    f.write("/* Generated by rccars_re/gen_fx_data.py -- do not edit.\n"
            " *\n"
            " * The wheel dust, the exhaust smoke, the tyre marks and the body\n"
            " * glance, as the game itself configures them. Every value here is\n"
            " * raw/scale through FUN_00532410, with the scale read off the push\n"
            " * immediate at the call site; the loader each group comes from is\n"
            " * named above it. See gen_fx_data.py's docstring.\n"
            " *\n"
            " * All four effects ship ENABLED: RCCars.crs sets VIDEO_DustEff,\n"
            " * VIDEO_ExhaustGas, VIDEO_WheelTrace and VIDEO_EnvMap to 1.\n"
            " */\n\n")
    f.write("#ifndef FX_DATA_H\n#define FX_DATA_H\n\n")
    f.write("#include \"rb.h\"          /* rb_curve_pt */\n\n")

    f.write("/* ---- car_dustx, FUN_0052ea60 -------------------------------- */\n")
    for key, scale, name, note in DUSTX:
        emit(f, "DUSTX_" + name, scaled(dustx, key, scale),
             f"{key} {dustx.raw(key)}/{scale:g} -- {note}")
    f.write("\n/* ScaleX/ScaleY are MULTIPLIERS, not metres. What they multiply is\n"
            "   the particle system's own base sprite size, set by FUN_00477940,\n"
            "   which is not transcribed -- so the port supplies the base (see\n"
            "   fx.c's FX_SPRITE_UNIT) and applies these on top of it.\n"
            "\n"
            "   DynamicScale/DynamicScaleForw are the growth over a particle's life,\n"
            "   and FUN_0052e180 picks between them on the WHEEL INDEX -- < 2 gets\n"
            "   ...Forw, so the front pair's dust grows less than the rear's. */\n")
    f.write("\n/* The sprite, FUN_0046e180(\"dust\"). Both systems share it. */\n")
    f.write('#define FX_DUST_TEX  "dust"\n')

    f.write("\n/* ---- exhausted_gas, FUN_005308f0 ---------------------------- */\n")
    for key, scale, name, note in EG:
        emit(f, "EG_" + name, scaled(eg, key, scale),
             f"{key} {eg.raw(key)}/{scale:g} -- {note}")

    f.write("\n/* ---- car_trace, FUN_005300e0 -------------------------------- */\n")
    for key, scale, name, note in TRACE:
        emit(f, "TRACE_" + name, scaled(trace, key, scale),
             f"{key} {trace.raw(key)}/{scale:g} -- {note}")
    mh, mh_raw = maxheight(trace)
    emit(f, "TRACE_MAX_HEIGHT", mh,
         f"maxHeight -- ABSENT; {mh_raw}/1000 from car_trace.ini's second, "
         f"misnamed maxLen. See gen_fx_data.maxheight()")
    f.write("\n/* The ring, FUN_0052f050: 0x20 slots for wheels 0 and 1, 0x30 for\n"
            "   the rest, halved for a car that is not a player's. */\n")
    emit(f, "TRACE_RING_FRONT", 0x20, "0x20 at 0x0052f0a4")
    emit(f, "TRACE_RING_REAR", 0x30, "0x30 at 0x0052f0ad")
    emit(f, "TRACE_WHEELS", 6, "the loop bound in FUN_0052f050")
    f.write("\n/* The four marks, sprintf(\"t_halfdry_tire2_%d\") in FUN_0052f050. */\n")
    emit(f, "TRACE_TEX_N", 4, "the loop bound in FUN_0052f050")
    f.write('#define TRACE_TEX_FMT  "t_halfdry_tire2_%d"\n')
    f.write("\n/* FUN_0052ff20 nudges each strip 2e-05 further along the depth\n"
            "   range than the last, so overlapping marks do not z-fight. */\n")
    emit(f, "TRACE_DEPTH_STEP", 2e-05, "0x0052ffe8")

    f.write("\n/* ---- surf_<kind>, FUN_0052ed30 ------------------------------ */\n"
            "/* Indexed by the engine's own surface id (FUN_0052ee10's switch):\n"
            "   0 default, 1 sand, 2 wetsand, 3 dunesand, 5 stone road. 4 and\n"
            "   anything else fall to default, whose IntScale is 0 -- so asphalt,\n"
            "   grass, metal and wood raise NO dust, which is the original. */\n")
    f.write("typedef struct {\n"
            "    float int_scale;    /* DustX_IntScale, rate multiplier */\n"
            "    float r, g, b, a;   /* DustX_Color*, 0..255 */\n"
            "    float life, life_disp;  /* DustX_TimeLife(Disp), seconds */\n"
            "} fx_surf_t;\n\n")
    f.write("#define FX_SURF_N 6\n")
    f.write("static const fx_surf_t fx_surf[FX_SURF_N] = {\n")
    tables = {}
    for sid, base, label in SURFACES:
        sec = Section(a.dir, base)
        vals = [scaled(sec, k, s) for k, s in SURF_KEYS]
        tables[sid] = (label, base, vals)
    for sid in range(6):
        if sid in tables:
            label, base, v = tables[sid]
            f.write(f"    /* {sid} {label:9} */ "
                    f"{{ {v[0]:.4f}f, {v[1]:.1f}f, {v[2]:.1f}f, {v[3]:.1f}f, "
                    f"{v[4]:.1f}f, {v[5]:.4f}f, {v[6]:.4f}f }},   /* {base}.crs */\n")
        else:
            f.write(f"    /* {sid} unused    */ "
                    "{ 0.0000f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0000f, 0.0000f },\n")
    f.write("};\n")

    f.write("\n/* The port's own surface classes (pack_col.py / sfx.h SURF_*) mapped\n"
            "   onto the engine's dust ids. sand, wetsand and gravel line up by\n"
            "   name; asphalt, grass, metal, wood and water get the default table\n"
            "   and so raise nothing.\n"
            "\n"
            "   `stone` is the one judgement: the engine's id 5 is spelt\n"
            "   SURF_STONEROAD in surf_gravel.crs, and the port splits that idea in\n"
            "   two -- `gravel` (gravel, cobble, path) and `stone` (rock, cliff,\n"
            "   brick). Both are mapped onto 5. Leaving stone on the default table\n"
            "   would make 17% of the drivable area of these tracks dustless. */\n")
    f.write("#define FX_SURF_MAP { 0, 1, 2, 5, 5, 0, 0, 0, 0 }\n")

    f.write("\n/* ---- CarEnvMap, FUN_004f9aa0 -------------------------------- */\n"
            "/* Stored as raw bytes and used as an alpha: physCreateSubsystem\n"
            "   writes 0xffffff | EnvMapBody<<24 into the ENVIR_CAR_BODY material's\n"
            "   colour, i.e. white at that alpha. */\n")
    for key, name, note in ENVMAP:
        emit(f, "ENVMAP_" + name, int(envmap.raw(key) or 0),
             f"{key} -- {note}")
    f.write("\n/* The env-map source is the texture Car.sb declares as S_SKY: the\n"
            "   ONLY declaration in the whole database with no source path, so the\n"
            "   engine fills it at runtime. The port binds the track's own sky. */\n")

    f.write("\n/* ---- curves ------------------------------------------------- */\n")
    for fname, cname in (("dust_int.spl", "fx_dust_int"),
                         ("dust_speed.spl", "fx_dust_speed"),
                         ("eg_int.spl", "fx_eg_int")):
        path = os.path.join(a.spl, fname)
        segs = read_spl(path)
        pts, dropped = sample_spl(segs)
        f.write(f"/* {fname}: {len(segs)} Bezier segments -> {len(pts)} points"
                f"{', %d dropped as non-monotonic' % dropped if dropped else ''} */\n")
        emit_curve(f, cname, pts)

    f.write("#endif\n")
    f.close()
    print("wrote", a.out)


if __name__ == "__main__":
    main()
