#!/usr/bin/env python3
"""
Generate ai_data.h -- everything the AI opponents need that is not a recorded
path. The paths themselves are packed by pack_ai.py.

RC Cars' opponents are not a driving AI in the usual sense. Each one replays a
profile a human recorded (CarProfiles/<level>/*.dat, see prof_dump.py) and the
only thing the game decides at runtime is HOW FAST to walk along it. That single
number is what this header carries the inputs for.

Sources, each with the loader it was read out of:

  Scripts/ailayouts.ini        the roster. FUN_004a9290 reads section
                               "AIRace%i" for CoeffCommonOpponents and then,
                               per opponent, AI%iRef / AI%iCar / AI%iBoost /
                               AI%iReson / AI%iTires / AI%iPath. AI%iRaces is
                               read on the menu side. The seven drivers are
                               "AIPlayer%i" (Car, Skin, Face).

  Scripts/aiCommonCoeffs.ini   FUN_004f74f0 reads easy/normal/hard/ultra out of
                               section "CommonCoeffs" into four globals, and
                               FUN_004f11b0 selects one of them by the value
                               FUN_004a4c80 returns -- but ONLY when the game
                               mode (FUN_004a4ca0) is 1, the championship.

  Scripts/aiShiftsFuncWaitAccel.ini
                               FUN_004f74f0 reads eqSHIFTS/rearShifts and
                               eqSHIFTS/frontShifts as float[10]. Only
                               frontShifts is ever read back: FUN_004fd5e0
                               subtracts frontShifts[gap] from the distance
                               difference when the player is two or more
                               checkpoints ahead. rearShifts is dead in the
                               retail build -- kept, and marked, because the
                               commented-out line above it in the .ini shows
                               what it was for.

  Splines/FuncWaitAccel<n>.bspl and <n>_A
                               FUN_004f74f0 caches ten splines; FUN_004fd6b0
                               picks one by the opponent's index 0..4, taking
                               the plain family when the mode is 5 and the _A
                               family otherwise. Each maps a signed distance in
                               metres to a speed multiplier.

  Settings/OpponentBehav.ini   FUN_004f74f0 reads nine keys through the float
                               getter at 0x404950. OpponentBehav.crs ships only
                               the four LEGACY keys (coeffAccel, coeffWait,
                               distAccel, distWait) and none of these nine, so
                               every one of them takes its .ini default -- which
                               is what cfgGetInt does, the same fallback
                               carRender.crs relies on for ShadowSize.

  Settings/AIEmu.ini           FUN_004a1440. DistCam is raw * 0.1 metres; the
                               twelve Prob* keys belong to the multiplayer
                               "GMAIEMU" path that repositions off-camera AI
                               cars, which this port has no equivalent of. Read
                               and emitted anyway, because a value in a header
                               is cheaper to find later than a value in a .ini.

  Scripts/championship.ini     the level names. The AIRace<n> sections are
                               numbered in championship order, and each Track<n>
                               section carries the port's own level name in its
                               comment -- "// SURF (Beach_1)". That comment is
                               NOT what this script trusts: it seeds the mapping
                               and pack_ai.py then proves it by measuring each
                               profile's first sample against the track's own
                               race start.

Two of the nine OpponentBehav keys are stored PRE-SUMMED. FUN_004f74f0 does

    deltaFromEquilAccelMIN += deltaFromEquilAccelMAX
    deltaFromEquilWaitMIN  += deltaFromEquilWaitMAX

so the ".._MIN" global holds the far end of a band whose near end is ".._MAX",
not an independent value. Emitted summed, as the engine holds them.

**None of those nine reaches a shipped behaviour.** FUN_004fd6b0 copies the
whole 33-dword block into a stack table and then overwrites the only three
entries FUN_00471a70 reads -- table[0], [1] and [2] -- with 1.0, 1.0 and 10.0,
which makes its ramp evaluate to exactly 1.0 for any input. What survives is the
spline multiply. So the rubber band is the spline and nothing else, and these
constants are emitted for provenance: if a future pass finds the ramp being fed
real values somewhere else, the numbers are already here.

Usage: python3 gen_ai_data.py [--root DIR] [-o OUT]
"""

import argparse
import os
import re

import bspl2csv

# AIRace<n> in championship order -> the port's own track base name. Seeded from
# championship.ini's own section comments; pack_ai.py proves each one against the
# track's race start before writing anything.
RACE_TRACK = {
    1: "beach_1",     # SURF
    2: "country_3",   # FISHERS
    3: "beach_3",     # FORT
    4: "urban_2",     # AAD
    5: "country_2",   # MINES
    6: "beach_2",     # CAMPING
    7: "urban_1",     # AWACS
    8: "country_1",   # RANCHO
    9: "country_4",   # SILO
    10: "beach_4",    # WAR PATH
}

# The order main.c indexes AI_RACES by, i.e. tracks.h's own order.
TRACK_ORDER = ("beach_1", "beach_2", "beach_3", "beach_4",
               "country_1", "country_2", "country_3", "country_4",
               "urban_1", "urban_2")

MAX_OPPONENTS = 5


# ------------------------------------------------------------------ parsing

def read_script(path):
    """Read a Scripts/*.ini: `#define`s plus `Section Keys "x"` blocks holding
    `key (type) = value` lines. Returns {section: {key: text}}.

    The engine's own reader resolves the #defines; this does the same by plain
    substitution, which is enough for these files (every #define is an
    identifier used as a whole token)."""
    defines, out, sec = {}, {}, None
    for raw in open(path, "r", errors="replace"):
        line = raw.split("//")[0].strip()
        if not line:
            continue
        m = re.match(r"#define\s+(\w+)\s+(.+)$", line)
        if m:
            defines[m.group(1)] = m.group(2).strip()
            continue
        m = re.match(r'Section\s+Keys\s+"([^"]+)"', line, re.I)
        if m:
            sec = m.group(1)
            out.setdefault(sec, {})
            continue
        if re.match(r"End\b", line, re.I):
            sec = None
            continue
        m = re.match(r"(\w+)\s*\(([^)]*)\)\s*=\s*(.+)$", line)
        if m and sec is not None:
            out[sec][m.group(1).lower()] = (m.group(2).strip().lower(),
                                            m.group(3).strip())
    return out, defines


def resolve_int(text, defines):
    """`EASY + NORMAL + HARD` or `BUGGY` or `2` -> int."""
    expr = text
    for k, v in defines.items():
        expr = re.sub(r"\b%s\b" % re.escape(k), v, expr)
    if not re.fullmatch(r"[\d\s+\-*/()]+", expr):
        raise ValueError("cannot resolve %r (-> %r)" % (text, expr))
    return int(eval(expr))  # noqa: S307 -- digits and operators only, checked


def read_slider_ini(path):
    """Settings/*.ini slider definitions -> {key: (lo, hi, default, flo, fhi)}.
    Same shape dump_physics.read_ini uses; duplicated here so this generator
    does not depend on the physics extractor's exact return type."""
    out, key, vals, fvals = {}, None, None, None

    def flush():
        if key and vals:
            out[key] = (vals[0], vals[1], vals[2],
                        fvals[0] if fvals else None,
                        fvals[1] if fvals else None)

    for raw in open(path, "r", errors="replace"):
        line = raw.split("//")[0].strip()
        m = re.match(r'Key\s+"([^"]+)"', line)
        if m:
            flush()
            key, vals, fvals = m.group(1), None, None
            continue
        m = re.match(r"Values\s+(-?[\d.]+)\s+(-?[\d.]+)\s+(-?[\d.]+)", line)
        if m:
            vals = tuple(float(x) for x in m.groups())
            continue
        m = re.match(r"FValues\s+(-?[\d.]+)\s+(-?[\d.]+)", line)
        if m:
            fvals = tuple(float(x) for x in m.groups())
    flush()
    return out


def slider_value(spec):
    """The conversion every slider-with-FValues key goes through, written the
    way physLoadCdt's inline form does it: (raw - inLo) * (outHi - outLo) /
    (inHi - inLo) + outLo. Without FValues the raw integer IS the value."""
    lo, hi, default, flo, fhi = spec
    if flo is None:
        return default
    if hi == lo:
        return flo
    return (default - lo) * (fhi - flo) / (hi - lo) + flo


# ------------------------------------------------------------------- curves

def curve_points(path):
    curves = bspl2csv.decode(path)
    if len(curves) != 1:
        raise ValueError("%s: %d curves, expected 1" % (path, len(curves)))
    # x is always 0 in these files; the curve lives in (y, z) = (in, out).
    pts = [(p[1], p[2]) for p in curves[0]]
    if len(pts) < 2:
        raise ValueError("%s: %d points" % (path, len(pts)))
    for a, b in zip(pts, pts[1:]):
        if b[0] <= a[0]:
            raise ValueError("%s: input column not increasing" % path)
    return pts


# ---------------------------------------------------------------- emission

def fmt_pts(pts):
    return ", ".join("{%.6ff,%.6ff}" % p for p in pts)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default="/mnt/c/tools/rccars_extracted")
    ap.add_argument("-o", "--out", default="/mnt/c/tools/rccars_vita/ai_data.h")
    a = ap.parse_args()

    scripts = os.path.join(a.root, "Scripts")
    settings = os.path.join(a.root, "Settings")
    splines = os.path.join(a.root, "Splines")

    lay, defines = read_script(os.path.join(scripts, "ailayouts.ini"))
    common, _ = read_script(os.path.join(scripts, "aiCommonCoeffs.ini"))
    shifts, _ = read_script(os.path.join(scripts, "aiShiftsFuncWaitAccel.ini"))
    behav = read_slider_ini(os.path.join(settings, "OpponentBehav.ini"))
    emu = read_slider_ini(os.path.join(settings, "AIEmu.ini"))

    # -- the seven drivers
    players = []
    for i in range(1, 8):
        sec = lay["AIPlayer%d" % i]
        players.append((i,
                        resolve_int(sec["car"][1], defines),
                        resolve_int(sec["skin"][1], defines),
                        sec["face"][1].strip('"')))

    # -- the ten rosters, in the port's track order
    races = {}
    for n in range(1, 11):
        sec = lay["AIRace%d" % n]
        ops = []
        for k in range(1, MAX_OPPONENTS + 1):
            if ("ai%dpath" % k) not in sec:
                continue
            ops.append({
                "ref": resolve_int(sec["ai%dref" % k][1], defines),
                "car": resolve_int(sec["ai%dcar" % k][1], defines),
                "races": resolve_int(sec["ai%draces" % k][1], defines),
                "boost": resolve_int(sec["ai%dboost" % k][1], defines),
                "reson": resolve_int(sec["ai%dreson" % k][1], defines),
                "tires": resolve_int(sec["ai%dtires" % k][1], defines),
                "path": sec["ai%dpath" % k][1].strip('"').replace("\\", "/"),
            })
        races[RACE_TRACK[n]] = {
            "race": n,
            "dir": sec["defaultpath"][1].strip('"'),
            "coeff": float(sec["coeffcommonopponents"][1]),
            "ops": ops,
        }

    # -- the ten splines
    fwa = [curve_points(os.path.join(splines, "FuncWaitAccel%d.bspl" % i))
           for i in range(1, 6)]
    fwa_a = [curve_points(os.path.join(splines, "FuncWaitAccel%d_A.bspl" % i))
             for i in range(1, 6)]

    def shift_list(key):
        txt = shifts["eqSHIFTS"][key][1]
        v = [float(x) for x in txt.split(",")]
        if len(v) != 10:
            raise ValueError("eqSHIFTS/%s has %d entries, expected 10"
                             % (key, len(v)))
        return v

    rear = shift_list("rearshifts")
    front = shift_list("frontshifts")

    diff = [float(common["CommonCoeffs"][k][1])
            for k in ("easy", "normal", "hard", "ultra")]

    o = []
    w = o.append
    w("/* Generated by rccars_re/gen_ai_data.py -- do not edit.")
    w(" *")
    w(" * The AI opponents' roster and the constants behind the one number the")
    w(" * game decides for them at runtime: how fast to walk their recorded")
    w(" * path. See ai.h for the model and gen_ai_data.py for every source.")
    w(" */")
    w("#ifndef AI_DATA_H")
    w("#define AI_DATA_H")
    w("")
    w('#include "rb.h"')
    w("")
    w("#define AI_MAX_OPPONENTS %d" % MAX_OPPONENTS)
    w("#define AI_N_RACES %d" % len(TRACK_ORDER))
    w("#define AI_N_PLAYERS %d" % len(players))
    w("")
    w("/* ailayouts.ini's own #defines for AI<n>Races. */")
    for k in ("EASY", "NORMAL", "HARD"):
        w("#define AI_RACE_%-7s %s" % (k, defines[k]))
    w("")
    w("/* The seven drivers, ailayouts.ini \"AIPlayer<n>\". `car` is the model")
    w("   they are pictured with in the shop; the model they actually race is")
    w("   the opponent entry's own `car`, and the two DISAGREE (AIPlayer3 DOC")
    w("   is an Overkill and drives a Hummer on Fishers). The profile carries a")
    w("   third copy, and pack_ai.py checks it against the roster. */")
    w("typedef struct { const char *name; int car, skin; const char *face; }"
      " ai_player;")
    w("static const ai_player AI_PLAYERS[AI_N_PLAYERS] = {")
    names = {1: "MCJocker", 2: "MaxXMad", 3: "Doc", 4: "BabyShark",
             5: "Rosy", 6: "Dakilla", 7: "Johny"}
    for i, car, skin, face in players:
        w('    { "%s", %d, %d, "%s" },' % (names[i], car, skin, face))
    w("};")
    w("")
    w("typedef struct {")
    w("    int ref;                /* 1..AI_N_PLAYERS, AI<n>Ref */")
    w("    int car;                /* 0 Overkill, 1 Buggy, 2 Hummer */")
    w("    int races;              /* AI_RACE_* mask, AI<n>Races */")
    w("    int boost, reson, tires;/* upgrade levels 0..3 */")
    w("    const char *path;       /* AI<n>Path, as authored */")
    w("} ai_opponent;")
    w("")
    w("typedef struct {")
    w("    const char *track;      /* the port's own assets/<base> name */")
    w("    const char *dir;        /* the CarProfiles subdirectory */")
    w("    int   race;             /* the AIRace<n> number, for provenance */")
    w("    float coeff_common;     /* CoeffCommonOpponents */")
    w("    int   n;")
    w("    ai_opponent op[AI_MAX_OPPONENTS];")
    w("} ai_race;")
    w("")
    w("/* Indexed the same as tracks.h's TRACKS[]. */")
    w("static const ai_race AI_RACES[AI_N_RACES] = {")
    for t in TRACK_ORDER:
        r = races[t]
        w('    { "%s", "%s", %d, %.6ff, %d, {'
          % (t, r["dir"], r["race"], r["coeff"], len(r["ops"])))
        for op in r["ops"]:
            w('        { %d, %d, %d, %d, %d, %d, "%s" },'
              % (op["ref"], op["car"], op["races"], op["boost"], op["reson"],
                 op["tires"], op["path"]))
        w("    } },")
    w("};")
    w("")
    w("/* Scripts/aiCommonCoeffs.ini, selected by difficulty 0..3 -- and only")
    w("   in the championship (FUN_004f11b0 gates on FUN_004a4ca0() == 1). */")
    w("static const float AI_DIFFICULTY[4] = { %s };"
      % ", ".join("%.6ff" % v for v in diff))
    w("")
    w("/* FUN_004fd4c0 clamps the product of the difficulty and the track")
    w("   coefficient into this band and logs \"CAR: coeff for AI is")
    w("   unacceptable\" when it has to. The shipped data never trips it:")
    w("   the widest product is %.4f. */"
      % max(d * races[t]["coeff"] for d in diff for t in TRACK_ORDER))
    w("#define AI_COEFF_MIN 0.5f")
    w("#define AI_COEFF_MAX 2.0f")
    w("")
    w("/* Splines/FuncWaitAccel<n>.bspl -- signed metres of lead over the")
    w("   player, in, speed multiplier out. Negative means the opponent is")
    w("   behind and the curve returns more than 1. */")
    for i, pts in enumerate(fwa, 1):
        w("static const rb_curve_pt ai_fwa%d[] = { %s };" % (i, fmt_pts(pts)))
    for i, pts in enumerate(fwa_a, 1):
        w("static const rb_curve_pt ai_fwa%d_a[] = { %s };" % (i, fmt_pts(pts)))
    w("")
    w("/* Index 0..4 is the opponent's own slot. FUN_004fd6b0 takes AI_FWA when")
    w("   the game mode is 5 and AI_FWA_A otherwise, so the CHAMPIONSHIP (mode")
    w("   1) races against the _A family. The two differ in both directions and")
    w("   neither is simply harder: _A slows a LEADING opponent much further")
    w("   (0.635 against 0.800 at +45 m on slot 0) and lets a TRAILING one")
    w("   catch up less (1.057 against 1.100 at -40 m). It is a tighter band")
    w("   around the player, which is what a championship with its own")
    w("   difficulty multiplier wants. */")
    w("static const rb_curve AI_FWA[5] = {")
    for i, pts in enumerate(fwa, 1):
        w("    { ai_fwa%d, %d }," % (i, len(pts)))
    w("};")
    w("static const rb_curve AI_FWA_A[5] = {")
    for i, pts in enumerate(fwa_a, 1):
        w("    { ai_fwa%d_a, %d }," % (i, len(pts)))
    w("};")
    w("")
    w("/* Scripts/aiShiftsFuncWaitAccel.ini. FUN_004fd5e0 subtracts")
    w("   AI_FRONT_SHIFTS[gap] from the lead when the PLAYER is `gap`+2")
    w("   checkpoints ahead, gap clamped to 9 -- i.e. a player who is lapping")
    w("   the field stops pulling it along. AI_REAR_SHIFTS is loaded by the")
    w("   engine and never read back; it is here because it is data. */")
    w("static const float AI_FRONT_SHIFTS[10] = { %s };"
      % ", ".join("%.1ff" % v for v in front))
    w("static const float AI_REAR_SHIFTS[10] = { %s };  /* unread */"
      % ", ".join("%.1ff" % v for v in rear))
    w("")
    w("/* Settings/OpponentBehav.ini, through the .ini defaults -- the .crs")
    w("   carries none of these keys. FUN_004fd6b0 overwrites the three table")
    w("   entries FUN_00471a70 actually reads, so none of them reaches a")
    w("   shipped behaviour; see gen_ai_data.py. */")
    for key, macro in (("distEquil", "AI_DIST_EQUIL"),
                       ("coeffAccelMAX", "AI_COEFF_ACCEL_MAX"),
                       ("coeffAccelMIN", "AI_COEFF_ACCEL_MIN"),
                       ("coeffWaitMAX", "AI_COEFF_WAIT_MAX"),
                       ("coeffWaitMIN", "AI_COEFF_WAIT_MIN")):
        w("#define %-22s %.6ff" % (macro, slider_value(behav[key])))
    for lo, hi, macro in (("deltaFromEquilAccelMIN", "deltaFromEquilAccelMAX",
                           "AI_DELTA_ACCEL_FAR"),
                          ("deltaFromEquilWaitMIN", "deltaFromEquilWaitMAX",
                           "AI_DELTA_WAIT_FAR")):
        w("#define %-22s %.6ff  /* MIN + MAX, as FUN_004f74f0 stores it */"
          % (macro, slider_value(behav[lo]) + slider_value(behav[hi])))
    w("#define %-22s %.6ff" % ("AI_DELTA_ACCEL_NEAR",
                               slider_value(behav["deltaFromEquilAccelMAX"])))
    w("#define %-22s %.6ff" % ("AI_DELTA_WAIT_NEAR",
                               slider_value(behav["deltaFromEquilWaitMAX"])))
    w("")
    w("/* Settings/AIEmu.ini via FUN_004a1440: DistCam is raw * 0.1 metres and")
    w("   FUN_004a0f60 calls a point visible when it is inside it -- or inside")
    w("   half of it BEHIND the camera. The Prob* pair belongs to the")
    w("   multiplayer GMAIEMU path, which repositions off-camera AI cars; the")
    w("   port has no equivalent and does not read them. */")
    w("#define AI_DIST_CAM %.6ff" % (emu["DistCam"][2] * 0.1))
    w("static const float AI_PROB_ACCUM[6] = { %s };  /* unread */"
      % ", ".join("%.1ff" % emu["ProbAccum%d" % i][2] for i in range(1, 7)))
    w("static const float AI_PROB_CHECK[6] = { %s };  /* unread */"
      % ", ".join("%.1ff" % emu["ProbCheck%d" % i][2] for i in range(1, 7)))
    w("")
    w("#endif")
    w("")

    text = "\n".join(o)
    with open(a.out, "w") as f:
        f.write(text)
    print("wrote %s (%d bytes)" % (a.out, len(text)))
    for t in TRACK_ORDER:
        r = races[t]
        print("  %-10s AIRace%-2d %-8s coeff %.2f  %d opponents: %s"
              % (t, r["race"], r["dir"], r["coeff"], len(r["ops"]),
                 " ".join(os.path.basename(op["path"]) for op in r["ops"])))


if __name__ == "__main__":
    main()
