#!/usr/bin/env python3
"""
Generate vis_data.h -- the constants behind the Vita port's visual subsystems:
the car shadow, the animated water, and the checkpoint markers.

Same shape as gen_rb_data.py: read the game's own Settings/*.ini slider
definitions and Settings/*.crs runtime overrides, apply the conversion each
loader actually performs, and emit a header. Nothing here is invented -- every
conversion below is cited to the function it was read out of.

Conversions, by loader:

  FUN_004f9aa0   "CarRender" / "CarLight" / "CarEnvMap"
      ShadowSize<n>   raw * 0.01                (metres, per CAR 1..3)
      ShadowShift<n>  raw * 0.01 - 0.5          (metres along the body +Z)
      ShadowDensity   raw                       (0..255 alpha)
      TextureSizePow2 1 << raw                  (the shadow render target)
      FrameSkipNmb    raw                       (regenerate every n+1 frames)
      AngleAzimuth    raw                       (degrees)
      AngleElevation  raw                       (degrees)

  FUN_005323a0   "car_lmshd"      alphavel raw/100, deltaalpha raw/100,
                                  maxdist raw/1 -- three cfgGetInt-over-a-
                                  divisor calls through FUN_00532410, plus
                                  Splines/light_car.bspl loaded by name. This is
                                  the CAR'S OWN LIGHTMAP SHADOW: see
                                  FUN_00531ff0, which samples the level lightmap
                                  under the four wheels and chases the result
  FUN_004f98e0   "LOD"            DistLOD1SwitchOn / Delta12 / Delta23, each
                                  remapLinear(raw, 1..100 -> 20..120) * 0.1
                                  metres and ACCUMULATED, so the three distances
                                  are 2.00, 4.30 and 6.60 m. carVisualBody
                                  (0x005077e0) fades the car's own lighting to
                                  flat between the first two
  FUN_0052b370   "anim_cp"        every key raw * 0.01
  FUN_0051cef0   "WaterLOD_Coast" HeightOn/Off (raw-100)*0.01, ScrollVel raw*0.01,
                                  Alpha* raw/255, ActiveRad raw
  FUN_00523a10   "WaterLOD_Stream" ScrollVel raw*0.01, Noise raw*0.004,
                                  PoolNoiseLen raw*0.002, PoolNoiseFreq raw*0.01

One block is NOT a config section: the stream family's per-kind flags and vertex
alpha are a static table in RCCars.exe, read here off the image so the header
cannot drift from it. See read_surface_table.

Usage: python3 gen_vis_data.py [--dir SETTINGS] [--level beach_1] [-o OUT]
  FUN_00525c10   "WaterLOD_Wave"  TimeLong/TimeShort/Len raw*0.01, Angle raw
  FUN_0052a550   "Water_Wave_New" every key raw / 100  (via FUN_00532410,
                                  which is literally `cfgGetInt(...) / scale`)
  FUN_00521540   the WaterLOD surface section, PER TRACK
                                  (WLOD_<track>_more1 -> WSURF[]):
                                  angle/angle2 raw*10 (degrees),
                                  speed/speed2/length2 raw*0.01,
                                  period raw*0.1, period2 raw*0.1,
                                  amp/amp2/magnetOffset/magnetRadius raw*0.01
                                  (clamped at 1.0), offset raw*0.01-0.5,
                                  posX/posZ raw-500, angleFoam raw (degrees)
  FUN_005240c0   evaluates it -- the sea's vertical displacement, which the
                 notes said for a year was not transcribed. See WSURF_HEADER.

A key absent from the .crs keeps the .ini default -- that is exactly what
cfgGetInt does, and it is load-bearing here: carRender.crs ships without
ShadowSize/ShadowShift at all, so the three sizes come from carRender.ini.
"""

import argparse
import math
import os
import re
import struct

import bspl2csv
from dump_physics import read_crs

CARS = (1, 2, 3)

GAME = "/mnt/c/Games/RC Cars"
EXE = os.path.join(GAME, "RCCars.exe")
IMAGE_BASE = 0x400000

# The stream family's node table. Three 0x3c-byte entries, each opening with the
# inline needle the scanner at 0x523760 hands to strstr, followed by nine dwords.
# The count is a dword of its own just in front of the table.
SURF_TABLE = 0x575710
SURF_COUNT = 0x575708
SURF_STRIDE = 0x3C


# --------------------------------------------------------------- ini / crs

def read_ini(path):
    """-> {key: (min, max, default)}. Keys with `Mask spline` have no values."""
    out = {}
    key = None
    for line in open(path, "r", errors="replace"):
        line = line.strip()
        m = re.match(r'^Key\s+"(.*)"$', line)
        if m:
            key = m.group(1)
            continue
        m = re.match(r"^Values\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)$", line)
        if m and key:
            out[key] = tuple(int(x) for x in m.groups())
            key = None
    return out


class Section:
    """One settings group: .ini defaults with .crs runtime values layered on."""

    def __init__(self, sdir, base):
        self.name = base
        self.vals = {}
        ini = os.path.join(sdir, base + ".ini")
        crs = os.path.join(sdir, base + ".crs")
        if os.path.exists(ini):
            for k, (_lo, _hi, d) in read_ini(ini).items():
                self.vals[k] = d
        if os.path.exists(crs):
            for k, (_lo, _hi, v) in read_crs(crs).items():
                self.vals[k] = v

    def raw(self, key, default=None):
        if key in self.vals:
            return self.vals[key]
        # the engine's key lookup is case-insensitive on these sections
        for k, v in self.vals.items():
            if k.lower() == key.lower():
                return v
        if default is None:
            raise KeyError(f"{self.name}: no key {key!r}")
        return default


# ------------------------------------------------------------------ output

def read_surface_table(path=EXE):
    """
    The engine's stream-family node table -> [{needle, scroll, noise, sound,
    alpha}], read off the image.

    Every field below is cited to the instruction that consumes it, because none
    of this is a config key and there is no loader to read it back from:

      +0x00  the needle, inline. strstr(node->name, needle) at 0x52377b, first
             hit wins, and the index it hits at is stored on the surface.
      +0x20  scroll?      0x5234c9: if set, every UV's V advances by
                          dt * ScrollVel each frame.
      +0x24  noise?       0x523903 allocates a 256-entry noise object and a copy
                          of the rest UVs; 0x523555 then jitters each U by
                          noise * PoolNoiseLen. Set for `pool` ALONE -- a pool
                          shimmers in place where a stream flows.
      +0x28  (set on all three, no consumer separated)
      +0x2c  0x523439: contributes the surface to a list the caller collects.
      +0x30  sound?       0x5239d2: if set, look up a 'SNCH' node named
                          `water_fall` and attach it. `wtr_fall` alone.
      +0x34  ALPHA, a byte. 0x523866 writes `0xffffff | (alpha << 24)` into
             every one of the surface's vertex colours -- i.e. white at a flat
             opacity, and THIS is what makes one of these surfaces water rather
             than a slab of texture. The port had 0.55 for all of them, which is
             the waterfall's value used as a guess for all three.
      +0x38  (clear on all three)

    The needles are asserted rather than trusted: a wrong address would
    otherwise emit three plausible alphas read out of the middle of something
    else.
    """
    b = open(path, "rb").read()

    def dw(va):
        return struct.unpack_from("<I", b, va - IMAGE_BASE)[0]

    n = dw(SURF_COUNT)
    if n != 3:
        raise SystemExit(f"surface table count is {n}, not 3 -- wrong address?")
    out = []
    for i in range(n):
        base = SURF_TABLE + i * SURF_STRIDE
        off = base - IMAGE_BASE
        needle = b[off:off + 0x18].split(b"\0")[0].decode("latin-1")
        out.append({"needle": needle,
                    "scroll": dw(base + 0x20),
                    "noise": dw(base + 0x24),
                    "sound": dw(base + 0x30),
                    "alpha": dw(base + 0x34) & 0xFF})
    want = ["stream", "wtr_fall", "pool"]
    got = [e["needle"] for e in out]
    if got != want:
        raise SystemExit(f"surface table reads {got}, expected {want}")
    return out


# The ten tracks, in tracks.h's order -- WSURF[] is indexed by the same number
# load_track() uses, and each row carries its own track name so a test can pin
# that the two lists have not drifted apart.
TRACKS = ("beach_1", "beach_2", "beach_3", "beach_4", "country_1",
          "country_2", "country_3", "country_4", "urban_1", "urban_2")

WSURF_HEADER = """
/* ---- the sea surface: FUN_00521540 loads it, FUN_005240c0 evaluates it -----
 *
 * Per surface vertex, with the projections FUN_0051c000 precomputes at load:
 *
 *     phi1 = (t*speed + x*cos(angle) + z*sin(angle)) * period
 *     phi2 =  t*period2 + |(x,z) - (posX,posZ)| / length2
 *     damp = clamp01(depth / magnetRadius)
 *     y    = restY + offset + (1 - damp)*magnetOffset
 *                  + damp * ( amp*(0.25 + 0.75*sin phi1) + amp2*sin phi2 )
 *
 * So `period` is a SPATIAL frequency in rad/m -- wavelength 2*pi/period -- and
 * the time term rides on speed*period, which leaves speed as the phase speed in
 * m/s. Wave two is RADIAL about (posX, posZ) with wavelength 2*pi*length2.
 * `speed2` and `angle2` are loaded and FUN_005240c0 never reads them; angle2's
 * projection is precomputed all the same, so both are carried here.
 *
 * `amp` and `amp2` are HEIGHTS, in metres, and both are read: the key string at
 * 0x5756e0 is "amp" and the loader puts it in param_2[8], "amp2" at 0x5756c0
 * goes to param_2[9]. The 1/amp2 stored beside it is not a sign that amp2 is a
 * divisor -- it is used at 0x51c71d to normalise the coast foam's signal back
 * to unit amplitude. The amp term is deliberately asymmetric: 0.25 + 0.75*sin
 * spans -0.5..+1.0, so crests stand twice as far above the mean as troughs fall
 * below it, which is what a swell does.
 *
 * The steepness these produce is the check that the units are read right: every
 * track lands between 2 and 9 per cent, and real swell breaks past about 14.
 * Reading `period` as a time instead put two of the five past breaking.
 *
 * magnetOffset/magnetRadius are the shallow-water block. Both conversions are
 * unobvious and neither is raw*0.01: magnetOffset is raw*0.01 - 0.5, so the
 * shipped raw 50 is exactly ZERO on every track and there is no shore lift at
 * all; magnetRadius is raw*0.05, so the shipped raw 51 is 2.55 m of DEPTH over
 * which the swell fades to flat -- five times the 0.5 m the port had guessed.
 *
 * NOT here, and each for its own reason:
 *   texScaleX/Z  the world-to-UV rate the engine tessellates its own grid with
 *                (raw*0.1). The port ships authored tiles that carry their own
 *                UVs, so there is nothing for it to scale.
 *   alphaPow     conversion known -- raw*0.2 - 10, stored as -1/that, so the
 *                shipped raw 60 is -0.5 -- but the code that consumes it is not
 *                transcribed, and -0.5 is not an exponent water.c could use.
 *                The port's own depth ramp keeps its own constant, in water.h.
 *
 * The section is PER TRACK (WLOD_<track>_more1); the ones that ship one are the
 * four beaches and country_3, and beach_2 also ships a .crs override. A track
 * without one falls back to WaterLOD, whose amp and amp2 are both 0 -- and
 * every track that falls back has no sea surface at all. ---- */

#define WSURF_N_TRACKS 10

typedef struct {
    const char *track;         /* tracks.h's base name, for the index check */
    const char *section;       /* the config section this row was read from */
    float angle_deg, speed, period;              /* wave 1, directional */
    float angle2_deg, speed2, period2, length2;  /* wave 2, radial */
    float amp, amp2;                             /* metres */
    float offset, magnet_offset, magnet_radius;  /* metres */
    float pos_x, pos_z;                          /* the radial wave's centre */
    float tex_rad_min, tex_rad_max;              /* UV, the orbit's radius */
    float tex_speed_min, tex_speed_max;          /* degrees per second */
    float alpha_min, alpha_max;                  /* 0..1 */
} wsurf_t;

"""


def wsurf_section(sdir, track):
    """The config section the WaterLOD surface reads for `track`. Per level,
    with the same two fallbacks the engine's own lookup has."""
    sec = Section(sdir, f"WLOD_{track}_more1")
    if not sec.vals:
        sec = Section(sdir, f"wlod_{track}_more1")
    if not sec.vals:
        sec = Section(sdir, "WaterLOD")
    return sec


def wsurf_values(sec):
    """FUN_00521540's conversions, in ONE place.

    pack_col.py bakes offset/magnetOffset/magnetRadius into the .col water grid
    and water.c draws the surface with the copies of them in vis_data.h, so the
    two have to be the same arithmetic and not two typings of it. Four of these
    are not raw*0.01 and three of those four were got wrong once already."""
    r = sec.raw
    return {
        "angle_deg":     min(r("angle") * 10, 360.0),
        "speed":         min(r("speed") * 0.01, 10.0),
        "period":        min(r("period") * 0.1, 10.0),
        "angle2_deg":    min(r("angle2") * 10, 360.0),
        "speed2":        min(r("speed2") * 0.01, 10.0),
        "period2":       min(r("period2") * 0.1, 100.0),
        "length2":       min(r("length2", 161) * 0.01, 10.0),
        "amp":           min(r("amp") * 0.01, 1.0),
        "amp2":          min(r("amp2") * 0.01, 1.0),
        "offset":        r("offset") * 0.01 - 0.5,
        "magnet_offset": r("magnetOffset") * 0.01 - 0.5,
        "magnet_radius": r("magnetRadius") * 0.05,
        "pos_x":         float(r("posX", 500) - 500),
        "pos_z":         float(r("posZ", 500) - 500),
        "tex_rad_min":   r("texRadMIn") * 0.01,
        "tex_rad_max":   r("texRadMax") * 0.01,
        "tex_speed_min": r("texSpeedMin") * 3.6,
        "tex_speed_max": r("texSpeedMax") * 3.6,
        "alpha_min":     r("alphaMin") / 255.0,
        "alpha_max":     r("alphaMax") / 255.0,
    }


def wsurf_row(track, sec):
    """One WSURF[] initialiser, out of wsurf_values."""
    v = wsurf_values(sec)
    amp, amp2 = v["amp"], v["amp2"]
    per = v["period"]
    lam1 = (2.0 * math.pi / per) if per else 0.0
    lam2 = 2.0 * math.pi * v["length2"]
    body = "\n".join((
        '    { "%s", "%s",' % (track, sec.name),
        "      %.6ff, %.6ff, %.6ff," % (v["angle_deg"], v["speed"], per),
        "      %.6ff, %.6ff, %.6ff, %.6ff," % (
            v["angle2_deg"], v["speed2"], v["period2"], v["length2"]),
        "      %.6ff, %.6ff," % (amp, amp2),
        "      %.6ff, %.6ff, %.6ff," % (v["offset"], v["magnet_offset"],
                                        v["magnet_radius"]),
        "      %.6ff, %.6ff," % (v["pos_x"], v["pos_z"]),
        "      %.6ff, %.6ff," % (v["tex_rad_min"], v["tex_rad_max"]),
        "      %.6ff, %.6ff," % (v["tex_speed_min"], v["tex_speed_max"]),
        "      %.6ff, %.6ff }," % (v["alpha_min"], v["alpha_max"]),
        "      /* swell %.3f m peak to peak; wavelengths %.1f m and %.1f m,"
        " steepness %.3f and %.3f */" % (
            1.5 * amp + 2.0 * amp2, lam1, lam2,
            (1.5 * amp / lam1) if lam1 else 0.0,
            (2.0 * amp2 / lam2) if lam2 else 0.0),
    ))
    return body + "\n"


def lod_dist(raw):
    """LOD.ini's own conversion, read at 0x004f98f9 and again twice below it.

    The idiom is the engine's remapLinear over the slider's declared range:
    raw 1..100 maps onto 20..120, clamped at both ends, and the result is
    scaled by 0.1 -- so 1 gives 2.0 m and 100 gives 12.0 m. The three keys are
    CUMULATIVE (each is stored as itself plus the one before), which is what
    makes them switch distances rather than three independent numbers.
    """
    if raw < 1:
        v = 20.0
    elif raw > 100:
        v = 120.0
    else:
        v = (raw - 1.0) * (100.0 / 99.0) + 20.0
    return v * 0.1


def light_curve(spldir):
    """Splines/light_car.bspl -- the lightmap-to-car-brightness curve.

    FUN_00531ff0 evaluates it at `mean lightmap RGB * 255` and divides the
    answer by 255 again, so both axes are 0..255 and the file's own numbers are
    the ones to emit. Two control points, which under the engine's own evaluator
    (0x0040f830, clamped piecewise linear -- rb_curve_eval in this port) is a
    line between them and a clamp outside.
    """
    curves = bspl2csv.decode(os.path.join(spldir, "light_car.bspl"))
    if not curves or len(curves[0]) < 2:
        raise SystemExit("light_car.bspl: no curve")
    return [(p[1], p[2]) for p in curves[0]]


def emit(f, name, value, comment):
    if isinstance(value, float):
        f.write(f"#define {name:<26} {value:.6f}f   /* {comment} */\n")
    else:
        f.write(f"#define {name:<26} {value}   /* {comment} */\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", default="/mnt/c/tools/rccars_extracted/Settings")
    ap.add_argument("--level", default="beach_1")
    ap.add_argument("--splines",
                    default="/mnt/c/tools/rccars_extracted/Splines")
    ap.add_argument("-o", "--out", default="/mnt/c/tools/rccars_vita/vis_data.h")
    a = ap.parse_args()

    car_render = Section(a.dir, "carRender")
    car_light = Section(a.dir, "carLight")
    car_lmshd = Section(a.dir, "car_lmshd")
    # LOD.crs OVERRIDES the .ini here and the difference matters: Delta12 ships
    # as 60 in the .ini (7.96 m) and 4 in the .crs (2.30 m), which is what puts
    # the car's lighting fade at 4.30 m instead of 9.96 and makes this feature
    # almost entirely about the car the camera is following.
    lod = Section(a.dir, "LOD")
    anim_cp = Section(a.dir, "anim_cp")
    coast = Section(a.dir, "waterLOD_coast")
    stream = Section(a.dir, "waterLOD_stream")
    wave_lod = Section(a.dir, "waterLOD_wave")
    wave_new = Section(a.dir, "water_wave_new")
    # the per-level water surface section; beach_1 ships only the .ini
    surf = Section(a.dir, f"WLOD_{a.level}_more1")
    if not surf.vals:
        surf = Section(a.dir, f"wlod_{a.level}_more1")
    if not surf.vals:
        surf = Section(a.dir, "WaterLOD")

    with open(a.out, "w") as f:
        f.write(f"""/*
 * vis_data.h -- constants for the port's visual subsystems, generated by
 * rccars_re/gen_vis_data.py from the game's own slider and config files.
 *
 * DO NOT EDIT. Regenerate with:
 *     python3 rccars_re/gen_vis_data.py
 *
 * Every conversion is the one the matching loader performs; see the script's
 * docstring for the function each came from. The sea surface is per track --
 * see WSURF[] below.
 */

#ifndef VIS_DATA_H
#define VIS_DATA_H

""")

        # ---------------------------------------------------------- shadow
        f.write("/* ---- car shadow: FUN_004f9aa0 (CarRender), used by FUN_005072f0 ---- */\n\n")
        sizes = [car_render.raw(f"ShadowSize{n}") * 0.01 for n in CARS]
        shifts = [car_render.raw(f"ShadowShift{n}") * 0.01 - 0.5 for n in CARS]
        f.write("/* radius of the projected shadow, metres, per car (1 Overkill,\n"
                "   2 Buggy, 3 Hummer). shdAddSource's `radius` argument. */\n")
        f.write("#define SHADOW_SIZE   { %s }\n" % ", ".join(f"{v:.6f}f" for v in sizes))
        f.write("/* offset of the shadow centre along the body's +Z (forward). */\n")
        f.write("#define SHADOW_SHIFT  { %s }\n\n" % ", ".join(f"{v:.6f}f" for v in shifts))
        emit(f, "SHADOW_DENSITY", car_render.raw("ShadowDensity"),
             "0..255, the shadow's alpha")
        emit(f, "SHADOW_TEX_SIZE", 1 << car_render.raw("TextureSizePow2"),
             "TextureSizePow2, the render-target edge")
        emit(f, "SHADOW_FRAME_SKIP", car_render.raw("FrameSkipNmb"),
             "regenerate the shadow texture every n+1 frames")
        emit(f, "SHADOW_FILTER_SPEED", car_render.raw("FilterSpeed"), "FilterSpeed")
        emit(f, "LIGHT_AZIMUTH_DEG", float(car_light.raw("AngleAzimuth")),
             "CarLight AngleAzimuth")
        emit(f, "LIGHT_ELEVATION_DEG", float(car_light.raw("AngleElevation")),
             "CarLight AngleElevation")
        emit(f, "LIGHT_AMBIENT", car_light.raw("AmbientI"), "CarLight AmbientI")
        emit(f, "LIGHT_DIRECT", car_light.raw("DirectI"), "CarLight DirectI")

        # ---- the car's own lighting, and the lightmap shadow that drives it.
        #
        # carVisualBody 0x005077e0: one DIRECTIONAL light and one ambient, both
        # GREY -- the six CarLight R/G/B keys are declared in the .ini, are all
        # 255, and the loader at 0x004f9aa0 does not read them, so there is
        # nothing to emit. The two intensities are BYTES scaled by the car's
        # light level, clamped at 255 and greyed:
        #
        #     ambient = min(255, AmbientI * level)      -> D3DRS_AMBIENT
        #     diffuse = min(255, DirectI  * level)      -> the light's colour
        #
        # 138 and 128 out of 255, so a face square to the sun renders at 1.04x
        # its texture and a face facing away at 0.54x -- the whole range the
        # highlight has to work in, and the reason the car reads as plastic.
        f.write("\n/* ---- the car's own light: carVisualBody 0x005077e0, "
                "and the LOD fade ---- */\n\n")
        near = lod_dist(lod.raw("DistLOD1SwitchOn"))
        far = near + lod_dist(lod.raw("Delta12"))
        emit(f, "LIGHT_FADE_NEAR", near,
             "LOD DistLOD1SwitchOn: full lighting closer than this, metres")
        emit(f, "LIGHT_FADE_FAR", far,
             "+ Delta12: flat (ambient 255, no light) beyond this")

        # ---- car_lmshd: the level's lightmap, sampled under the wheels.
        # FUN_00531ff0 averages FUN_004572c0's answer -- the mean of the
        # lightmap texel's RGB at the contact point -- over the wheels that
        # have one, runs it through light_car.bspl, and CHASES that with a
        # dead band. FUN_004572c0 is gated on Video/VIDEO_LightMaps and the
        # whole step on Video/VIDEO_Light.
        f.write("\n/* ---- car_lmshd: the lightmap under the wheels "
                "(FUN_00531ff0 / FUN_005323a0) ---- */\n\n")
        emit(f, "LIGHT_LM_ALPHAVEL", car_lmshd.raw("alphavel") / 100.0,
             "car_lmshd alphavel/100: light level chase, per second")
        emit(f, "LIGHT_LM_DELTA", car_lmshd.raw("deltaalpha") / 100.0,
             "deltaalpha/100: dead band, below which nothing moves")
        emit(f, "LIGHT_LM_MAXDIST", float(car_lmshd.raw("maxdist")),
             "maxdist: no camera within this of the car -> level 1.0, metres")
        pts = light_curve(a.splines)
        f.write("/* Splines/light_car.bspl: lightmap brightness (0..255) -> "
                "the\n"
                "   car's light level (0..255). Clamped piecewise linear, the\n"
                "   engine's own 0x0040f830 -- so %.3f at or below %.1f and\n"
                "   %.3f at or above %.1f, a line between. Which is why a tunnel\n"
                "   darkens the car by a bit over a third and no more. */\n"
                % (pts[0][1] / 255.0, pts[0][0],
                   pts[-1][1] / 255.0, pts[-1][0]))
        f.write("#define LIGHT_LM_CURVE_N %d\n" % len(pts))
        f.write("#define LIGHT_LM_CURVE { %s }\n"
                % ", ".join("{%.6ff, %.6ff}" % (i, o) for i, o in pts))

        # ------------------------------------------------------ checkpoints
        f.write("\n/* ---- checkpoint markers: FUN_0052b370 (anim_cp), drawn by FUN_0052abc0 ---- */\n\n")
        emit(f, "CP_SIZE", anim_cp.raw("Size") * 0.01,
             "half-extent of the billboard quad, metres")
        emit(f, "CP_SHIFT", anim_cp.raw("Shift") * 0.01,
             "lift above the checkpoint centroid, metres")
        emit(f, "CP_MIN_DIST", anim_cp.raw("MinDist") * 0.01,
             "fully opaque closer than this")
        emit(f, "CP_MAX_DIST", anim_cp.raw("MaxDist") * 0.01,
             "invisible beyond this")
        emit(f, "CP_BLINK_DELTA", anim_cp.raw("BlinkDelta") * 0.01,
             "seconds per animation frame (3 frames)")

        # ------------------------------------------------------------ water
        f.write("\n/* ---- coast foam band: FUN_0051cef0 (WaterLOD_Coast) ---- */\n\n")
        emit(f, "COAST_HEIGHT_ON", (coast.raw("HeightOn") - 100) * 0.01, "metres")
        emit(f, "COAST_HEIGHT_OFF", (coast.raw("HeightOff") - 100) * 0.01, "metres")
        emit(f, "COAST_SCROLL_VEL", coast.raw("ScrollVel") * 0.01, "UV per second")
        emit(f, "COAST_ALPHA_MIN", coast.raw("AlphaMin") / 255.0, "0..1")
        emit(f, "COAST_ALPHA_MAX", coast.raw("AlphaMax") / 255.0, "0..1")
        emit(f, "COAST_ACTIVE_RAD", float(coast.raw("ActiveRad")), "metres")

        f.write("\n/* ---- stream: FUN_00523a10 (WaterLOD_Stream) ---- */\n\n")
        emit(f, "STREAM_SCROLL_VEL", stream.raw("ScrollVel") * 0.01, "UV per second")
        emit(f, "STREAM_NOISE", stream.raw("Noise") * 0.004, "")
        emit(f, "STREAM_POOL_NOISE_LEN", stream.raw("PoolNoiseLen", 5) * 0.002, "")
        emit(f, "STREAM_POOL_NOISE_FREQ", stream.raw("PoolNoiseFreq", 155) * 0.01, "")

        # ---- the stream family's own opacity, off the exe's table ----------
        f.write("""
/* ---- stream / waterfall / pool: the node table at 0x%x -------------
 *
 * NOT a config section -- a static table of three entries in the exe, scanned
 * per node by 0x523760. read_surface_table cites the instruction behind every
 * field; what matters here is +0x34, which 0x523866 writes into EVERY vertex
 * colour of the matched surface as `0xffffff | (alpha << 24)`. White at a flat
 * opacity: that single byte is the whole of what makes one of these three a
 * body of water rather than a textured slab, and the three do not agree.
 *
 * The port used to have one number, 0.55, for all of them, marked "NOT
 * recovered" -- which turns out to be the WATERFALL'S value, and it was never
 * applied to a pool at all because pools were not classified. */

""" % SURF_TABLE)
        for e in read_surface_table():
            key = {"stream": "STREAM", "wtr_fall": "FALL", "pool": "POOL"}[e["needle"]]
            emit(f, key + "_VERTEX_ALPHA", e["alpha"] / 255.0,
                 "`%s`, raw %d/255 at +0x34" % (e["needle"], e["alpha"]))
        f.write("\n/* Does this kind scroll its UVs (+0x20)? A pool does not: its\n"
                "   entry sets the NOISE flag at +0x24 instead, so it shimmers in\n"
                "   place. The noise field itself is the one part of the entry the\n"
                "   port does not transcribe -- see water_draw. */\n")
        for e in read_surface_table():
            key = {"stream": "STREAM", "wtr_fall": "FALL", "pool": "POOL"}[e["needle"]]
            emit(f, key + "_SCROLLS", e["scroll"], "+0x20")

        f.write("\n/* ---- shore wave spawner: FUN_00525c10 (WaterLOD_Wave) ---- */\n\n")
        emit(f, "WAVE_TIME_LONG", wave_lod.raw("TimeLong") * 0.01,
             "seconds between long-lived waves")
        emit(f, "WAVE_TIME_SHORT", wave_lod.raw("TimeShort") * 0.01,
             "seconds between short-lived waves")
        emit(f, "WAVE_SPAWN_ANGLE", float(wave_lod.raw("Angle")),
             "degrees of spread around the marker's facing")
        emit(f, "WAVE_SPAWN_LEN", wave_lod.raw("Len") * 0.01,
             "metres the wave travels")
        emit(f, "WAVE_ACTIVE_RAD", float(wave_lod.raw("ActiveRad")), "metres")

        f.write("\n/* ---- shore wave sprite: FUN_0052a550 (Water_Wave_New), drawn by FUN_0052a030.\n"
                "   Every key goes through FUN_00532410, which is `cfgGetInt(...) / 100`. ---- */\n\n")
        emit(f, "WAVE_LIFE_LONG", wave_new.raw("TimeLifeLong") / 100.0, "seconds")
        emit(f, "WAVE_LIFE_SHORT", wave_new.raw("TimeLifeShort") / 100.0, "seconds")
        emit(f, "WAVE_LIFE_SHORT_DISP", wave_new.raw("TimeLifeShortDisp") / 100.0,
             "seconds of random spread on the short life")
        emit(f, "WAVE_SPEED", wave_new.raw("WaveSpeed", 145) / 100.0, "m/s")
        emit(f, "WAVE_LEN", wave_new.raw("Len") / 100.0,
             "half-length of the quad along the crest")
        emit(f, "WAVE_HEIGHT", wave_new.raw("Height") / 100.0, "crest height scale")
        emit(f, "WAVE_DHEIGHT", wave_new.raw("DHeight") / 100.0 - 1.0,
             "DHeight-1: the vertical lift applied to the whole quad")
        emit(f, "WAVE_ANIM_SPEED", wave_new.raw("AnimSpeed") / 100.0, "UV per second")
        emit(f, "WAVE_ANIM_WAIT", wave_new.raw("AnimWait") / 100.0, "seconds")
        emit(f, "WAVE_DEC_TIME", wave_new.raw("DecTime") / 100.0,
             "fraction of life spent fading out")
        emit(f, "WAVE_INC_TIME", wave_new.raw("IncTime") / 100.0,
             "fraction of life spent fading in")

        f.write(WSURF_HEADER)
        f.write("static const wsurf_t WSURF[WSURF_N_TRACKS] = {\n")
        for t in TRACKS:
            f.write(wsurf_row(t, wsurf_section(a.dir, t)))
        f.write("};\n\n")

        # ------------------------------------------------------- antenna
        f.write("\n/* ---- the whip antenna: FUN_00500230 (Antenna_Car<n>).\n"
                "   A chain of nPoints masses, chainLength long in TOTAL -- the loader\n"
                "   divides by nPoints-1 to get the segment length. Every scalar is\n"
                "   raw*0.01 clamped at 10, which is why stiffness's raw 2000 lands on\n"
                "   10.0 and not 20.0. ---- */\n\n")
        for n in CARS:
            ant = Section(a.dir, f"Antenna_Car{n}")
            if not ant.vals:
                continue
            pts = ant.raw("nPoints")
            clen = min(ant.raw("chainLength") * 0.01, 1.0)
            f.write(f"/* car {n} */\n")
            emit(f, f"ANT{n}_POINTS", pts, "chain masses")
            emit(f, f"ANT{n}_LENGTH", clen, "metres, total")
            emit(f, f"ANT{n}_SEG", clen / max(pts - 1, 1), "metres per segment")
            emit(f, f"ANT{n}_MASS", min(ant.raw("mass") * 0.01, 10.0), "")
            emit(f, f"ANT{n}_STIFFNESS", min(ant.raw("stiffness") * 0.01, 10.0),
                 "raw 2000 clamps to 10")
            emit(f, f"ANT{n}_DAMPING", min(ant.raw("damping") * 0.01, 10.0), "")
            emit(f, f"ANT{n}_WIND", min(ant.raw("windFriction") * 0.01, 10.0), "")
            emit(f, f"ANT{n}_VOLUME", min(ant.raw("volume") * 0.01, 10.0), "")
            f.write("\n")

        # ---- the sun and its lens flare ------------------------------------
        #
        # FUN_00479b70 reads every one of these out of Settings/LENS_FLARE
        # through FUN_00479e80(load, section, key, 1, scale), which is raw/scale
        # -- so the colours (scale 1.0) come out as 0..255 bytes and everything
        # else (scale 100.0) as a fraction. Dist1 and Dist2 are NEGATED at load
        # (0x00479c3a, 0x00479c5d), which is what puts two of the four lens
        # ghosts on the far side of the screen centre from the sun.
        lens = Section(a.dir, "LENS_FLARE")
        sky = Section(a.dir, "Sky")
        if lens.vals:
            f.write("\n/* ---- the sun and its lens flare: FUN_00479b70 loads\n"
                    "   Settings/LENS_FLARE, FUN_00478fd0 places it, FUN_00479070\n"
                    "   builds the disc, FUN_00479250 the ghost chain and\n"
                    "   FUN_004797c0 draws all five sprites.\n"
                    "\n"
                    "   The sun's WORLD POSITION is not here -- it is the SUN_AF\n"
                    "   marker in each track's own .sb, which pack_vsc.py exports.\n"
                    "   Eight tracks have one; urban_1 and urban_2 do not, and the\n"
                    "   original draws no sun on those two. ---- */\n\n")
            emit(f, "SUN_SIZE", lens.raw("SizeSun") / 100.0,
                 "SizeSun, a fraction of the viewport half-width")
            emit(f, "SUN_COLOR_R", lens.raw("ColorRSun"), "ColorRSun, 0..255")
            emit(f, "SUN_COLOR_G", lens.raw("ColorGSun"), "ColorGSun")
            emit(f, "SUN_COLOR_B", lens.raw("ColorBSun"), "ColorBSun")
            emit(f, "SUN_RAY_PERIOD", lens.raw("TimeShootRay") / 100.0,
                 "TimeShootRay: seconds between line-of-sight casts")
            emit(f, "SUN_FADE_TIME", lens.raw("FadeTime") / 100.0,
                 "FadeTime: seconds to fade the flare in or out")
            emit(f, "SUN_CAM_OFFSET", lens.raw("CamOffset") / 100.0,
                 "CamOffset: added to the camera's focal distance")
            f.write("\n/* The four lens GHOSTS, in FUN_00479250's own order: a point\n"
                    "   `Dist` metres from the view centre along the line to the sun,\n"
                    "   at the camera's focal depth. Negative is the far side of the\n"
                    "   centre from the sun -- which is what a lens ghost does. */\n")
            for i in (1, 2, 3, 4):
                v = lens.raw("Dist%d" % i) / 100.0
                emit(f, "SUN_DIST%d" % i, -v if i <= 2 else v,
                     "Dist%d%s" % (i, ", negated at 0x00479c%s"
                                   % ("3a" if i == 1 else "5d") if i <= 2 else ""))
            f.write("\n/* The five SPRITE sizes and colours. Sprite 5 sits on the sun\n"
                    "   itself and is the `shine`; 2 and 4 are left white by the\n"
                    "   loader (0x00479ce6 onward writes 0xff into those slots), which\n"
                    "   is why only 1 and 3 have keys. */\n")
            for i in (1, 2, 3, 4, 5):
                emit(f, "SUN_FLARE_SIZE%d" % i, lens.raw("Size%d" % i) / 100.0,
                     "Size%d" % i)
            for i in (1, 2, 3, 4, 5):
                if i in (1, 3):
                    r, g, bl = (lens.raw("ColorR%d" % i), lens.raw("ColorG%d" % i),
                                lens.raw("ColorB%d" % i))
                    why = "ColorR%d/G%d/B%d" % (i, i, i)
                else:
                    r = g = bl = 255
                    why = "not a key: the loader writes 0xff"
                f.write("#define SUN_FLARE_RGB%d           %d, %d, %d   /* %s */\n"
                        % (i, r, g, bl, why))
        if sky.vals:
            # Sky.crs carries exactly one key. It is NOT read by the sun code and
            # nothing in the port uses it yet; emitted for provenance.
            f.write("\n/* Settings/Sky.crs, one key, and no consumer recovered. */\n")
            emit(f, "SKY_HOR_ANGLE_RAW", sky.raw("horAngle"),
                 "horAngle, raw 0..100 -- conversion and consumer unknown")

        f.write("\n#endif /* VIS_DATA_H */\n")

    print(f"wrote {a.out}")
    print(f"  shadow size  {['%.3f' % v for v in sizes]}  shift {['%.3f' % v for v in shifts]}"
          f"  density {car_render.raw('ShadowDensity')}")
    print(f"  checkpoint   size {anim_cp.raw('Size') * 0.01:.2f} m  shift "
          f"{anim_cp.raw('Shift') * 0.01:.2f} m  blink {anim_cp.raw('BlinkDelta') * 0.01:.2f} s")
    print(f"  water        coast scroll {coast.raw('ScrollVel') * 0.01:.2f}, "
          f"stream scroll {stream.raw('ScrollVel') * 0.01:.2f}")
    for t in TRACKS:
        sec = wsurf_section(a.dir, t)
        v = wsurf_values(sec)
        amp, amp2, per = v["amp"], v["amp2"], v["period"]
        lam1 = 2.0 * math.pi / per if per else 0.0
        lam2 = 2.0 * math.pi * v["length2"]
        # Steepness per TRAIN, height over wavelength. Real swell lives under
        # about 1/7; past that a wave breaks and cannot stand. Printed because
        # it is the one number that says whether `period` has been read as the
        # spatial frequency it is -- under the old reading (wavelength =
        # speed*period) two of these five tracks came out past breaking.
        s1 = (1.5 * amp / lam1) if lam1 else 0.0
        s2 = (2.0 * amp2 / lam2) if lam2 else 0.0
        print(f"    {t:<10} {sec.name:<22} swell {1.5 * amp + 2.0 * amp2:.3f} m p-p"
              f"   lambda {lam1:5.1f} / {lam2:4.1f} m"
              f"   steep {s1:.3f} / {s2:.3f}"
              f"   offset {v['offset']:+.2f} m")


if __name__ == "__main__":
    main()
