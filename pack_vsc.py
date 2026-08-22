#!/usr/bin/env python3
"""
Pack an .sb scene into a compact runtime scene file (.vsc) for the Vita app.

OBJ is a text format with separate position/UV indices; neither is useful at
runtime. This de-indexes into unique (position, uv) pairs, merges every mesh
that shares a texture into one draw batch, and stores textures as raw pixels so
the app can upload them straight to the GPU with no decode step.

Layout (all little-endian, which matches both the host and the Vita's ARM):

    char[4] "VSC9". Older files still LOAD: 3 is textures+batches, 4 adds the
            car's rig, 5 adds markers and the shadow radius, 6 adds LIGHTMAPS --
            a second UV per vertex and a lightmap texture per batch -- 7 adds
            the ENV-MAP class per batch, with vertex normals for the batches that
            have one, 8 adds named models sharing one file (props.vsc), and 9
            adds the SURFACE BLEND: a second base texture, a mask, and the two
            extra UV sets they need.
    u32     texture count
    u32     batch count
    u32     part count                                        -- VSC4 and up
    u32     marker count                                      -- VSC5 only
    float   shadow radius                                     -- VSC5 only
    u32     model count                                       -- VSC8 and up
            0 from this packer, which writes no model table (pack_props.py does);
            the FIELD is in the header from 8 on and the loader reads it.
            the world half-extent the '__shadow' texture covers, or 0 if there
            is none. See bake_shadow for why this is not just ShadowSize.
    -- textures, in order --
      u16 name length, char[] name (no terminator)
      u16 width, u16 height
      u8  format   0 = RGB565 (2 bytes/px), 1 = RGBA8888 (4 bytes/px)
      u8  mip level count
      u8[] pixels for each level in turn, level n sized (w>>n) x (h>>n)

    RGB565 is packed RED in the high bits -- standard 565. vitaGL's
    GL_UNSIGNED_SHORT_5_6_5 path memcpy's the pixels straight into GXM's
    U5U6U5_RGB, which the SDK also spells R5G6B5. See csi_texture for the
    hardware evidence and for why the old BGR packing looked right in Vita3K.
    -- parts, in order --                                     -- VSC4 only
      u16 name length, char[] name (no terminator)
      i32 parent index (-1 for the root, which is always part 0)
      float[16] rest       node -> model, at rest
      float[16] rest_inv   its inverse
      float[16] local      rest relative to the parent part
    -- markers, in order --                                   -- VSC5 only
      u16 name length, char[] name (no terminator)
      float x, y, z        world position, hierarchy already composed
      float yaw            the node's Euler Y, degrees, wrapped to [-180, 180)
    -- batches, in order --
      u32 texture index (0xFFFFFFFF = untextured)
      u32 flags        see BATCH_* below
      u32 part index                                          -- VSC4 and up
      u32 lightmap texture index (0xFFFFFFFF = unlit)         -- VSC6 and up
      u32 env class  0 none, 1 body, 2 GRE1, 3 GRE2, 4 upgrades -- VSC7 only
      u32 model index                                         -- VSC8 and up
      u32 SECOND base texture index (0xFFFFFFFF = none)       -- VSC9 and up
      u32 blend MASK texture index  (0xFFFFFFFF = none)       -- VSC9 and up
      u32 vertex count, u32 index count
      vertex[] { float x,y,z; float u,v; float lu,lv; }   28 bytes  -- VSC6 up
                { float x,y,z; float u,v; }               20 bytes  -- older
      float[3 * vertex count] normals   -- VSC7, and only when env class != 0
      float[4 * vertex count] u2,v2,mu,mv  -- VSC9, and only on a BLEND batch:
                the second base texture's UVs and the mask's. A side array for
                the same reason the normals are one -- 2.5% of the world's
                triangles blend, and widening every vertex by 16 bytes to say so
                would cost more memory than the whole feature.
      u16[]    indices

The lightmap is the game's own: the .sb carries a second UV layer tagged LM_1
alongside the base BAS1, and two 512x512 atlases embedded as 0x8232 blobs. 54%
of beach_1's faces are lit by them. sb2obj used to read the BAS1 layer and stop,
which is why the port rendered with no level lighting at all.

Batch flags name what a surface IS, so the app can animate it the way the game
does. The engine knows this from the object's name in the scene, and so do we --
except the sky, which is keyed on its TEXTURE because the level authors did not
name the dome consistently. See SKY_TEX_RE for what that cost:

      bit 0  sky          draw camera-locked, depth write off
      bit 1  water        the WaterLOD surface tiles (`sea`, *_LODn)
      bit 2  coast        the foam band along the shoreline (coast1..7)
      bit 3  stream       the stream surface
      bit 4  waterfall    wtr_fall
      bit 5  modulate     a signed detail map -- MULTIPLIED onto the ground
                          rather than drawn as a surface. See signed_detail_map;
                          this is the sand/wet-sand transition strip.
      bit 6  transp       the artists tagged this mesh TRANSP<n> -- draw it
                          ALPHA-BLENDED, not alpha-tested. See TRANSP_RE.
      bit 7  pool         a standing puddle -- the third entry of the engine's
                          own stream-family table. See SURFACE_NEEDLES.
      bit 8  blend        the SURFACE TRANSITION: two base textures lerped by a
                          mask texture, all three named by the face's own UV
                          layers. See BATCH_BLEND.

Water tiles ship at FOUR subdivision levels of the same patch -- polySurface
3415/3418/3420 are LOD1/2/3 of one tile, sharing an XZ bbox to the centimetre.
The engine picks one by distance; a flattened scene draws all four coincident at
y = 0, which is 4x overdraw and, once the surface is displaced by a wave, four
interpenetrating copies. --water-lod keeps only the most subdivided tile of each
bbox group, which is both faster and the only version that can be animated.

Batches are split so no batch exceeds 65535 vertices, keeping indices u16 --
GXM handles u16 indices natively and it halves index bandwidth.

--rig keeps the car's articulated nodes addressable after the flattening. Vertices
stay baked in model space, so an unanimated part draws exactly as it did before;
the app articulates a part by drawing it under `rest_inv * animated_world`. The
node names are the game's own -- carCreateAniProc1 (0x00504100) looks up the same
list out of the table at 0x00572fcc -- so the runtime rig and the original's agree
node for node. All matrices are row-major row-vector 4x4, the engine's layout,
which is also what glMultMatrixf wants.

Usage: python3 pack_vsc.py <file.sb> <out.vsc> [--pngdir DIR] [--csidir DIR]
"""

import argparse
import glob
import math
import os
import re
import struct

import csi2png
import sb2obj

MAX_VERTS = 65535

BATCH_SKY = 1
BATCH_WATER = 2
BATCH_COAST = 4
BATCH_STREAM = 8
BATCH_FALL = 16
BATCH_MODULATE = 32
BATCH_TRANSP = 64
BATCH_POOL = 128
BATCH_BLEND = 256

# How a mesh's name maps onto a surface kind -- and this is the ENGINE'S OWN
# RULE, read out of it rather than guessed: a case-sensitive SUBSTRING test, not
# a prefix and not an anchored pattern.
#
# The stream family is a static table of three 0x3c-byte entries at 0x575710
# (its count, 3, at 0x575708), each beginning with an inline needle -- `stream`,
# `wtr_fall`, `pool`. The scanner at 0x523760 walks it per node calling strstr
# (0x548560) on the node's own name at +0x18 and takes the first hit; `coast` is
# scanned exactly the same way by the callback at 0x51c950. Every flag and
# constant those entries carry is decoded in gen_vis_data.py's POOL/STREAM/FALL
# block -- including the vertex alpha, which is what makes a pool water.
#
# This used to be three `^`-anchored regexes, and the anchor was wrong on FOUR of
# the ten tracks -- 12 meshes that the engine animates and the port drew as
# opaque ground:
#
#   * country_1's `pool1`..`pool7`, 229 tris -- seven standing puddles across the
#     whole track, and the reported bug. `pool` was not in the port's rule at
#     all, so country_1 packed with NO water batch of any kind.
#   * beach_4's `pool1`/`pool2`, 102 tris, same.
#   * country_3's `NOSHDW_LM0_wtr_fall1`/`_4`, 64 tris -- both of that track's
#     waterfalls, missed on the exporter's own prefix tags alone.
#   * beach_2's `LM0_NOSHDW_coast1`, 30 tris -- the ONLY coast mesh on the track,
#     drawn as a grey band of opaque sand.
#
# Substring matching adds exactly those 12 and nothing else: over the ten tracks
# every name containing one of the four needles is one of these surfaces, and
# every mesh carrying one of the water module's own textures (`sea`,
# `coast_base`, `stream`) now ends up flagged -- which is the invariant
# vsc_check.py holds this to, since it is a property of the DATA and not a
# restatement of the rule.
SURFACE_NEEDLES = (("coast", BATCH_COAST),
                   ("stream", BATCH_STREAM),
                   ("wtr_fall", BATCH_FALL),
                   ("pool", BATCH_POOL))
# The textures the WaterLOD module owns. Nothing but an animated surface may
# carry one -- see vsc_check.py.
WATER_TEXTURES = ("sea", "coast_base", "stream")
WATER_LOD_RE = re.compile(r"_LOD(\d)$", re.I)
WATER_TEX = "sea"

# The sky dome is picked by its TEXTURE, not by its node name. Every one of the
# ten domes carries a texture named sky_up / sky_up_c / sky_up_c2 / sky_up_u and
# no other mesh in any track references one, so this is exact in both directions.
#
# It used to be the mesh-name regex `^pCylinder`, which is the name Maya gives
# every cylinder primitive, and the dome is only sometimes one of them. That got
# it wrong on 8 of the 10 tracks, both ways round:
#
#   * 34 ordinary meshes, 1,868 triangles, were drawn camera-locked with depth
#     writes off -- i.e. they vanished from where they belong. beach_1's two
#     column shafts (pCylinder1/4, textured `column`) left their bases and
#     capitals hanging in mid-air near cp_4; beach_3's two round walls near the
#     start (pCylinder3/4, 618 tris of wall_2/wall_3) disappeared outright; and
#     the `kooler` barrels went missing from country_4 (7), urban_1 (10) and
#     urban_2 (11). Both of those were reported from play.
#   * five domes are not named pCylinder<n>. beach_2's is NC_pCylinder6, which
#     matched only because the old code used search() rather than match(); the
#     other four -- beach_3, country_1, country_2, urban_2 -- are NOSHDW_LM0_NC
#     and packed with NO sky batch, so the dome wrote depth and moved with the
#     world. envmap_init, which takes the car's reflection off the first
#     BATCH_SKY batch, then found either nothing (country_1, country_2, urban_2)
#     or a WALL (beach_2's grating/wall_t2, beach_3's wall_2/wall_3).
SKY_TEX_RE = re.compile(r"^sky", re.I)

# ---------------------------------------------------------------------------
# THE SURFACE TRANSITION -- two base textures and a mask, all three in the data.
#
# Reported as sand meeting wet sand at a hard line where the original is smooth.
# The port had no mechanism for it and these notes said the original's was not
# recovered; it is in every track file, and sb2obj was reading past it.
#
# A face's UV layers are tagged, and the engine maps the tags to layer TYPES in
# FUN_0046fc20 -- BAS1..BAS4 -> 0..3, LM_1 -> 0x10, DET1 -> 0x100,
# ALP1..ALP4 -> 0x1000..3 -- called per layer by _mod2ConvertFace (0x00435ed0,
# named by its own error string "can't convert texInfoType"). Nine of the ten
# tracks carry faces with BAS1 + BAS2 + ALP1 + LM_1: two ground textures and the
# mask that blends them, plus the lightmap. 3,608 faces, and country_1 has none.
#
#     colour = mask * BAS1 + (1 - mask) * BAS2
#
# The polarity is measured, not assumed -- see TAG_BASE2_UV in sb2obj.py for the
# 3,174-against-14 vote that fixes which side is which.
#
# WHAT THIS IS WORTH, measured as the length of edge where two different ground
# textures meet, split by whether either side blends (`soft`) or not (`hard`):
# on beach_1 the sand pairs the old note measured as 404.5 m of hard line --
# sand_halfdry|sand_wet 279.1 m and Sand_trample|sand_halfdry 118.3 m -- are now
# 100% soft, 0.0 m hard. Band width is a median 1.10 m there and 0.52-1.71 m
# over the ten tracks, i.e. one quad wide, which is why the mask's v is 0 or 1
# at 98% of transition vertices and the whole ramp lives inside one triangle.
#
# What stays hard is hard in the ORIGINAL too, and it is a different technique:
# the artists' painted transition strips, textures literally named `trasition`,
# `transition_mine_1`, `transition_sand`, `rock_05re_grass`. Those carry the
# blend in the image and join their neighbours at a plain seam -- beach_1's
# mont_grass|montains 454.9 m, country_4's rock_05re_c4|rock_05re_grass_c4
# 1,193 m. Nothing in the data asks for those to be blended, and the port draws
# them as authored.
#
# The mask arrives as an RGB image with no alpha channel (`alpha`, 128x64, a
# vertical black-to-white ramp with an organic edge, tiling along the band), so
# the packer moves its luminance INTO the alpha channel -- see MASK_TEXTURES and
# luma_to_alpha. The runtime needs it there because a fixed-function combiner
# can take alpha from a texture and colour from another unit, and cannot take
# alpha from a texture's RGB.
#
# NOT wired up here: the coast foam band, which is the second family the same
# layers describe (BAS1 `coast_base` + ALP1 `coast`, 601 faces on the four
# beaches and country_3, no BAS2 and no lightmap). water.c already fades that
# band with the recovered WaterLOD_Coast height ramp on the vertex alpha and
# scrolls its base UVs; whether the authored mask multiplies that or replaces it
# is a question about the WATER model, with its own evidence to find, so the
# packer leaves those faces exactly as they were. See docs/render-world.md.

# The artists' own render-state tag, and the one place a MESH NAME is trusted.
#
# Level meshes carry exporter-time prefixes -- NOSHDW, LM0, NC, TRANSP<n><A|B>,
# ACP<n> -- e.g. `NOSHDW_LM0_NC_TRANSPB_ACP4`. None of those strings appears in
# RCCars.exe or CRSGame20.dll, so the engine does not parse them at runtime: they
# told the MAYA EXPORTER what to bake. That makes them a statement of intent, and
# for transparency it is the only statement available, because the flag the
# exporter wrote has not been located in the .sb.
#
# Trusting a name here and not for the sky is a real distinction, not a
# convenience: `pCylinder` is what Maya calls every cylinder it makes and says
# nothing about the object, while `TRANSP255B` is a word an artist typed to mean
# "draw this transparent". It is also cross-checkable, and it checks out --
# every TRANSP-tagged texture in the ten tracks is one of the ground icons
# (icon_chp_1/3, icon_start, icon_arrow_1/2, icon_pacific, icon_shc_1), glass,
# lamp1, foliage (flora_7, list_test), SAW, SAIL_G or beton_cleen1, and 168 of
# the 202 tagged mesh/texture pairs carry a real alpha channel.
#
# The number is an alpha level (255, 225, 50 -- `TRANSP50A_NC_parus` is a sail at
# a fifth opacity) and the A/B suffix a blend variant; neither is decoded, and
# the port uses the tag only as a boolean. Note `fence` and `wood_red_trellis`
# are NOT tagged, so this is not the answer to their missing cut-out either.
TRANSP_RE = re.compile(r"(?:^|_)TRANSP\d*[AB]?(?:_|$)", re.I)

# The second artist signal, and it is needed: the mesh tag alone MISSES painted
# markings. Cross-checking the tag against the texture name caught `icon_way_1`,
# `icon_shc_1`, `icon_arrow_1` and `icon_arrow_2` batches sitting on meshes
# nobody tagged -- the same inconsistency as the sky domes and country_4's
# lightmapped transition strips. Every `icon_*` texture in the ten tracks is a
# marking painted on the track (start line, checkpoint numerals, direction
# arrows), and every one of them carries a broad smooth alpha ramp: 18% to 36% of
# texels strictly between transparent and opaque, 242-256 distinct alpha values,
# and no single plateau over 2.2% -- i.e. nothing like the antialiased 1-px
# border that makes a cut-out. So both signals are used, and vsc_check.py holds
# them to a PIXEL property rather than to either name.
ICON_TEX_RE = re.compile(r"^icon_", re.I)

# Markers the app wants at runtime: the checkpoint spine (cp_N and its refining
# points cp_N_M), the shore-wave spawn points, and SUN_AF -- the sun.
#
# SUN_AF is the engine's own name for it, twice over: FUN_00478bf0 registers the
# subsystem as "SUN_AF" (0x004a29a2 pushes the string) and FUN_00478fd0 then asks
# modFindName for a node of type 'MARK' (0x4d41524b) with that same name, taking
# its translation as the sun's world position. So the marker and the subsystem
# share one string, and there is nothing to guess.
#
# EIGHT of the ten tracks have one. urban_1 and urban_2 do NOT, and that is the
# original's own answer for those two: no node, so FUN_00478fd0 leaves the
# subsystem disabled and draws no sun at all.
MARKER_RE = re.compile(r"^(cp_\d+(_\d+)?|water_wave_\d+|SUN_AF)$", re.I)

# Textures no geometry references but the app draws from code: the shore-wave
# sprite, the two three-frame checkpoint arrows (green = next, red = later), and
# the sun.
#
# The sun's five are FUN_00478f60's own list, in its own order -- it stores them
# at +0x174 (the disc) and +0x218..+0x228 (the four lens ghosts and the shine),
# and note lens_flare_3 appears TWICE, as ghosts 1 and 3. `line_flare` is NOT in
# it: that texture belongs to the four-entry table at 0x005705d8, which this
# subsystem never reads.
EXTRA_TEX = ("water_wave", "water_wave_alpha",
             "cp_ar_2_f1", "cp_ar_2_f2", "cp_ar_2_f3",
             "cp_ar_3_f1", "cp_ar_3_f2", "cp_ar_3_f3",
             "sun_disc", "lens_flare_1", "lens_flare_2", "lens_flare_3",
             "shine")

SHADOW_TEX_NAME = "__shadow"

# The car rig, in the game's own words. Each car has its own animation proc and
# its own pointer table of node names, and these three lists are those tables
# read out of the image verbatim:
#
#   carCreateAniProc1 0x00504100 -> table 0x00572fcc, 16 names  (Car1, Overkill)
#   carCreateAniProc2 0x00505050 -> table 0x005731b8, 24 names  (Car2, Buggy)
#   carCreateAniProc3 0x00505ed0 -> table 0x005733f0, 23 names  (Car3, Hummer)
#
# carAniProc1 (0x00504820) drives the first set: rotY(steer) on the supports,
# rotZ(spin) on the wheels, rotZ(pitch) + rotX(roll) on the axles, aim-and-stretch
# on the springs. carAniProc2 (0x00505780) drives a double wishbone -- rotX on the
# UP and DOWN arms and the opposite rotX on the upright that carries the wheel --
# and steers the WHEEL nodes themselves, the Buggy having no separate knuckle.
# carAniProc3 (0x005068e0) is proc1 plus a third axle that TRANSLATES rather than
# pivots, plus middle wheels that steer at 0.3x. See carani.c.
#
# A name that a given car does not have simply finds nothing; the three tables
# overlap on the wheels, so one flat list covers all three cars.
#
# Two spellings for the Buggy's lower spring halves: the engine's table says
# Spring1_1 and the mesh node is Spring1_1_GRE1 (the env-map class suffix, which
# the Hummer's do not carry). Both are listed -- carani_bind takes whichever the
# model actually has.
CAR_RIG = (("FRONT_AXLE", "REAR_AXLE", "MIDDLE_AXLE",
            "AXLE_FRONT_LEFT_SUPPORT", "AXLE_FRONT_RIGHT_SUPPORT",
            "WHEEL_FRONT_LEFT", "WHEEL_FRONT_RIGHT",
            "WHEEL_REAR_LEFT", "WHEEL_REAR_RIGHT",
            "WHEEL_MIDDLE_LEFT", "WHEEL_MIDDLE_RIGHT",
            "SPRING_REAR_1", "SPRING_REAR_2", "SPRING_REAR_3", "SPRING_REAR_4",
            "SPRING_FRONT_1", "SPRING_FRONT_2", "SPRING_FRONT_3", "SPRING_FRONT_4")
           + tuple("AXLE_%s_%s_%s" % (row, side, part)
                   for row in ("FRONT", "REAR")
                   for side in ("LEFT", "RIGHT")
                   for part in ("UP", "DOWN", "WHEEL"))
           + tuple("Spring%d_%d%s" % (n, half, suffix)
                   for n in range(1, 7) for half in (1, 2)
                   for suffix in ("", "_GRE1")))

# ---------------------------------------------------------------------------
# The env-map glance.
#
# The engine does not overlay anything extra on the car: it flags the geometry
# it already has. physCreateSubsystem (0x004fcce0) looks the body up by name --
# "ENVIR_CAR_BODY_body", falling back to "ENVIR_CAR_BODY" -- and writes
# `0xffffff | EnvMapBody << 24` into its colour, i.e. white at that alpha;
# FUN_00507130 then hands carVisualHelperB the four alphas EnvMapBody,
# EnvMapGRE1, EnvMapGRE2 and EnvMapUpgrades together, and carVisualHelperA
# applies them across the instance. So the classes are node-name classes, and
# the suffixes are all over Car.sb: GLASSES_GRE1, FARI_GRE2 (headlights),
# BAMP_*_GRE2 (bumpers), Spring<n>_1_GRE1 (chrome springs).
#
# Both spellings occur -- ENVIR_CAR_BODY_zer_GER1 and ENVIR_CAR_BODY_dno_GRE2
# sit in the same subtree -- so match either.
ENV_NONE, ENV_BODY, ENV_GRE1, ENV_GRE2, ENV_UPGRADES = 0, 1, 2, 3, 4

ENVIR_RE = re.compile(r"^ENVIR_CAR_BODY", re.I)
GRE1_RE = re.compile(r"_G[RE][RE]1(_|$)", re.I)
GRE2_RE = re.compile(r"_G[RE][RE]2(_|$)", re.I)
# The bolt-on exhaust, keyed the way carparts.py keys it: off the texture name's
# trailing level rather than a per-car node table.
UPGRADE_TEX_RE = re.compile(r"turbo_\d$", re.I)


def env_class(name, texnames):
    """Which CarEnvMap alpha a mesh is drawn with, or ENV_NONE."""
    if name:
        if GRE1_RE.search(name):
            return ENV_GRE1
        if GRE2_RE.search(name):
            return ENV_GRE2
        if ENVIR_RE.match(name):
            return ENV_BODY
    if any(t and UPGRADE_TEX_RE.search(t) for t in texnames):
        return ENV_UPGRADES
    return ENV_NONE


def vertex_normals(verts, idx):
    """Area-weighted vertex normals for one batch, in its own model space.

    The env-mapped pass needs a normal per vertex and the .vsc has never carried
    one. Only env batches get them, so a track packs byte-for-byte the size it
    always did; the car pays 12 bytes on a few hundred vertices.
    """
    n = [[0.0, 0.0, 0.0] for _ in verts]
    for t in range(0, len(idx) - 2, 3):
        a, b, c = idx[t], idx[t + 1], idx[t + 2]
        pa, pb, pc = verts[a], verts[b], verts[c]
        e1 = (pb[0] - pa[0], pb[1] - pa[1], pb[2] - pa[2])
        e2 = (pc[0] - pa[0], pc[1] - pa[1], pc[2] - pa[2])
        cx = e1[1] * e2[2] - e1[2] * e2[1]
        cy = e1[2] * e2[0] - e1[0] * e2[2]
        cz = e1[0] * e2[1] - e1[1] * e2[0]
        for v in (a, b, c):
            n[v][0] += cx
            n[v][1] += cy
            n[v][2] += cz
    out = []
    for v in n:
        L = math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])
        if L < 1e-12:
            out.append((0.0, 1.0, 0.0))
        else:
            out.append((v[0] / L, v[1] / L, v[2] / L))
    return out


# NOT part of carAniProc1's list -- the antenna is simulated separately, by the
# chain in Settings/Antenna_Car<n>.crs (4 points, 0.25 m, stiffness/damping/
# windFriction). It is named here only so it lands in its own PART, and
# therefore its own batch: the app has to reach its vertices to bend it, and a
# batch merges every mesh sharing a texture, which would otherwise glue the
# antenna to the engine block it shares `overkill_dviglo` with.
#
# `booster_<n>_end` is here for a different reason again: nothing animates it,
# but the exhaust smoke has to come out of the pipe, and the pipe's position is
# in the model rather than in any config. FUN_005303c0 gets it from a per-car
# table at phys+0xe454 and transforms it by the car matrix (FUN_0050bcc0); the
# port reads the node's own rest transform instead, which is the same point
# without a table nobody has recovered. Naming it makes it addressable after the
# flattening -- see fx_set_pipe.
# `booster_<n>` is here for a THIRD reason: it is what tells the app which
# exhaust is which. The four upgrade groups are numbered by the LEVEL they
# belong to, and the TEXTURE SUFFIX IS A DIFFERENT NUMBERING ENTIRELY --
#
#     level    Overkill           Buggy      Hummer
#       1      overkill_turbo_1   turbo_1    hum_turbo_3
#       2      overkill_turbo_3   turbo_2    hum_turbo_4
#       3      overkill_turbo_4   turbo_3    hum_turbo_2
#       4      overkill_turbo_2   turbo_4    hum_turbo_1
#
# -- so keying the level off the texture name is right only for the Buggy, and
# on the other two it draws somebody else's exhaust. The engine never uses the
# texture for this: FUN_0050c163 walks the level index over three tables of NODE
# names at 0x00573914, 0x00573934 and 0x00573924, holding `booster_<n>`,
# `compressor_<n>` and `booster_<n>_end`. Naming `booster_<n>` here puts each
# exhaust in its own part, so the app reads the level off the part name and
# never looks at a texture -- see carparts_bind.
#
# It has to be `booster_<n>` and NOT the group node above it: that group is
# `UPGRADES<n>` on the Overkill, `UPGRADE<n>` on the Buggy (nested inside a node
# itself called UPGRADES1) and `Upgrades<n>` on the Hummer's LODs. `booster_<n>`
# is the one name all three spell the same way, and it is the engine's own key.
# `compressor_<n>` is named for the same reason and is NOT optional: it is a
# SIBLING of booster_<n> under the group, not a child, so naming only booster_<n>
# leaves a quarter of each exhaust behind on __root__ -- unattributed, and
# therefore drawn at every level at once. That is the same "four exhausts stacked
# on top of each other" this whole file exists to stop, just smaller. The engine
# walks both tables per level for exactly this reason.
CAR_PARTS_EXTRA = ("ANTENNA",
                   "booster_1", "booster_2", "booster_3", "booster_4",
                   "compressor_1", "compressor_2", "compressor_3", "compressor_4",
                   "booster_1_end", "booster_2_end",
                   "booster_3_end", "booster_4_end")


def mat_from_mt(M, T):
    """sb2obj's (3x3, translation) pair -- which is column-vector, v' = T + M*v --
    as the engine's row-major ROW-vector 4x4, v' = v*A. A's 3x3 block is M
    transposed and the translation lives in row 3."""
    A = [0.0] * 16
    for i in range(3):
        for k in range(3):
            A[k * 4 + i] = M[i][k]
        A[12 + i] = T[i]
    A[15] = 1.0
    return A


def mat_inverse_affine(A):
    """Inverse of a row-vector affine 4x4. The rig carries a uniform 1.05 scale,
    so a transpose is not enough -- this is the general 3x3 adjugate."""
    m = [[A[r * 4 + c] for c in range(3)] for r in range(3)]
    det = (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
           - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
           + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]))
    if abs(det) < 1e-20:
        raise ValueError("singular node transform")
    d = 1.0 / det
    inv = [[0.0] * 3 for _ in range(3)]
    for r in range(3):
        for c in range(3):
            r1, r2 = [x for x in range(3) if x != r]
            c1, c2 = [x for x in range(3) if x != c]
            cof = m[r1][c1] * m[r2][c2] - m[r1][c2] * m[r2][c1]
            if (r + c) & 1:
                cof = -cof
            inv[c][r] = cof * d          # adjugate = transpose of cofactors
    out = [0.0] * 16
    for r in range(3):
        for c in range(3):
            out[r * 4 + c] = inv[r][c]
    for c in range(3):
        out[12 + c] = -sum(A[12 + k] * inv[k][c] for k in range(3))
    out[15] = 1.0
    return out


def mat_mul(A, B):
    """A*B, row-vector convention: (v*A)*B == v*(A*B)."""
    return [sum(A[r * 4 + k] * B[k * 4 + c] for k in range(4))
            for r in range(4) for c in range(4)]


def index_csi(root, prefer=None):
    """
    name -> path of the HIGHEST-RESOLUTION .csi with that name.

    The pack ships three complete texture sets -- Textures.1, Textures.2 and
    Textures.3 -- which are the game's quality levels, at 512, 256 and 128 px
    for the same image. `RenderQual` in Video picks one at runtime.

    This used to be a dict comprehension over a recursive glob, so the LAST
    match won: `sand_wet` came out of Textures.3 at 128 px and `tire3_3` at
    64 px. Every texture in the port was whichever set happened to sort last,
    which is why the car read as blurry and the sand as mush. Pick by size, and
    do not trust directory order for it.
    """
    best = {}
    pat = "Textures.%s" % prefer if prefer else None
    for p in glob.glob(os.path.join(root, "**", "*.csi"), recursive=True):
        if pat and pat not in p.replace("\\", "/"):
            continue
        key = os.path.splitext(os.path.basename(p))[0].lower()
        try:
            h = csi2png.parse_header(open(p, "rb").read(256))
            area = h.width * h.height
        except Exception:
            area = 0
        if key not in best or area > best[key][0]:
            best[key] = (area, p)
    return {k: v[1] for k, v in best.items()}


def surface_kind(name, textures):
    """The BATCH_* bit a mesh's name (and texture) earns it, or 0.

    The needle test is the engine's -- strstr over the whole name, first hit
    wins, in the exe table's own order. See SURFACE_NEEDLES.
    """
    if not name:
        return 0
    for needle, bit in SURFACE_NEEDLES:
        if needle in name:
            return bit
    if WATER_TEX in textures and WATER_LOD_RE.search(name):
        return BATCH_WATER
    return 0


def signed_detail_map(w, h, fmt, level0):
    """
    Is this texture a SIGNED DETAIL MAP -- near-greyscale, with a border of
    neutral 128? If so it is not a surface, it is a MULTIPLY decal.

    The engine draws such a thing with D3D8 blend mode 5 (SRCBLEND =
    D3DBLEND_DESTCOLOR, DESTBLEND = D3DBLEND_SRCCOLOR), i.e.
    dst' = 2 * src * dst. A texel of exactly 128 leaves the ground untouched,
    below 128 darkens it and above 128 lightens it, so the decal fades to
    nothing at its own edge with no alpha channel anywhere in sight. The tyre
    mark t_halfdry_tire2_<n> is the known member of the family -- its flat
    border is exactly 128 and its tread runs 96..140 -- and the whole reasoning
    is under "A tyre mark is not a sprite" in CLAUDE.md.

    Decided from the PIXELS, the same way BATCH_ALPHA_KEYED is, and for the same
    reason: the name is not evidence. `wt_wetsand` is the one world texture in
    the game that qualifies, and it is the sand/wet-sand TRANSITION strip that
    every track lays along the boundary -- 151 meshes and 9,198 triangles across
    the ten. Drawn opaque, as the port used to, it is a flat grey band with two
    hard edges, which is exactly the "sharp transition" that got reported.

    Three things corroborate it beyond the pixels: the strips are authored 5-21 mm
    ABOVE the terrain (median 1 cm, every one of beach_1's 776 vertices over a
    sand/path floor), they carry NO lightmap at all (0 of 728 faces), and their
    surface class in the engine's own data is 0 -- "no opinion", which is how
    FUN_00534fc0's min-over-positive lets a decal defer to the floor beneath it.

    Nothing here corrects for RGB565 quantisation, which lifts a neutral 128 to
    about 132 and so brightens the whole strip by ~3%. That is what the packed
    tyre marks have always done -- same textures, same path -- so the two stay
    consistent; making it exact means forcing these to RGBA8888.
    """
    if not level0:
        return False
    n = w * h
    if n < 4 or (w < 3 or h < 3):
        return False

    if fmt == 1:                                  # RGBA8888
        def rgb(i):
            return level0[i*4], level0[i*4+1], level0[i*4+2]
    else:                                         # RGB565, red high
        def rgb(i):
            v = level0[i*2] | (level0[i*2+1] << 8)
            r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
            return (r*255)//31, (g*255)//63, (b*255)//31

    # greyscale: every texel's channels within a whisker of each other. 565
    # quantisation alone spreads them by a few counts, so this cannot be exact.
    for i in range(n):
        r, g, b = rgb(i)
        if max(r, g, b) - min(r, g, b) > 12:
            return False

    # and the border is neutral: that is what makes it fade out rather than
    # stamp an edge.
    border = ([i for i in range(w)] + [(h-1)*w + i for i in range(w)]
              + [y*w for y in range(h)] + [y*w + w - 1 for y in range(h)])
    near = sum(1 for i in border if abs(rgb(i)[0] - 128) <= 6)
    return near * 2 >= len(border)


def sky_kind(texnames, sky_re):
    """BATCH_SKY if any of a mesh's textures names the sky dome, else 0."""
    if not sky_re:
        return 0
    return BATCH_SKY if any(t and sky_re.search(t) for t in texnames) else 0


def pick_water_lods(meshes):
    """
    Indices of the water tiles to DROP: every subdivision level of a patch bar
    the finest. Tiles of one patch share an XZ bounding box exactly (they are
    the same surface, retessellated), so the box is the group key.
    """
    groups = {}
    for i, mesh in enumerate(meshes):
        nm, vs = mesh[0], mesh[1]
        m = WATER_LOD_RE.search(nm or "")
        if not m or not vs:
            continue
        tex = {f[2] for f in mesh[3]}
        if WATER_TEX not in tex:
            continue
        key = (round(min(v[0] for v in vs), 2), round(max(v[0] for v in vs), 2),
               round(min(v[2] for v in vs), 2), round(max(v[2] for v in vs), 2))
        groups.setdefault(key, []).append((int(m.group(1)), i))
    drop = set()
    for g in groups.values():
        g.sort()
        for _lvl, i in g[:-1]:
            drop.add(i)
    return drop


def collect_markers(b, reg, start, pattern):
    """
    Every MOD_MARKER node whose name matches, with its world position and the
    world direction of its local +Z as a yaw.

    Markers carry no geometry -- a MARK node is a 0x540B transform and a name --
    so this is a separate walk from collect(). The checkpoint spine (cp_N,
    cp_N_M) and the shore-wave spawn points are both stored this way.
    """
    mark = next((k for k, v in reg.items() if v == "MOD_MARKER"), None)
    out = []

    def rec(p, end, depth, pM, pT):
        while p < end:
            cid, cend, cnt, body = sb2obj.rd_chunk(b, p)
            if cend <= p or cend > end:
                return
            if cid == 0x9200:
                nm = sb2obj.node_name(b, body, cend)
                M, T = sb2obj.compose(pM, pT, sb2obj.node_trs(b, body, cend))
                if cnt == mark and nm and pattern.match(nm):
                    # local +Z in world space: M's third column
                    yaw = math.degrees(math.atan2(M[0][2], M[2][2]))
                    out.append((nm, T, yaw))
                if body < cend and depth < 24:
                    rec(body, cend, depth + 1, M, T)
            p = cend

    rec(start, len(b), 0, sb2obj.IDENT3, (0.0, 0.0, 0.0))
    return out


def shadow_fit_radius(meshes, margin=1.45):
    """
    The world half-extent a shadow texture has to cover for THIS car.

    ShadowSize is not it. shdAddSource's `radius` is the source's influence
    radius -- how far out shdRender marks receiving faces -- and the texture
    carries its own scale in the 4x4 matrix FUN_005072f0 builds, which is not
    recovered. Taking ShadowSize for the texture extent happens to work for the
    Overkill (0.36 against a 0.258 half-length) and the Hummer (0.42 against
    0.294) and fails for the Buggy, whose 0.298 half-length does not fit inside
    0.29: the silhouette runs off the edge, and CLAMP_TO_EDGE then smears that
    edge across the whole receiver.

    So: fit the texture to the car, and let shadow.c take the larger of this and
    ShadowSize as the radius it both gathers and projects over.

    The margin is not slack, it is the PENUMBRA's room. bake_shadow blurs by
    radius_frac of the texture edge, twice, which spreads the silhouette by
    about 2 * radius_frac * size pixels each way; without room for that the soft
    edge runs off the texture and CLAMP_TO_EDGE smears it across the receiver --
    the same failure as too small a radius, arrived at from the other direction.
    At the defaults (0.06, twice) the silhouette needs to sit inside 128/1.33
    pixels of centre, so 1.45 leaves a little over.
    """
    hx = hz = 0.0
    for mesh in meshes:
        for x, _y, z in mesh[1]:
            if abs(x) > hx:
                hx = abs(x)
            if abs(z) > hz:
                hz = abs(z)
    return max(hx, hz) * margin


def bake_shadow(meshes, size, radius, blur=2, radius_frac=0.06):
    """
    The car's silhouette seen from above, as an RGBA texture: black, with alpha
    = coverage. This is what the engine projects.

    FUN_005072f0 hands shdAddSource a texture at +0x50 and a texture matrix; the
    engine fills that texture by rendering the car top-down into a
    TextureSizePow2 target every FrameSkipNmb+1 frames. The port bakes it once
    instead -- same silhouette, no render target -- which is the one divergence
    here: a car standing on its nose keeps the shadow of a level one.

    The mapping matches shadow.c exactly: u = 0.5 + bx/(2R), v = 0.5 + bz/(2R)
    with (bx, bz) the offset from the shadow centre in BODY space.
    """
    cov = [0.0] * (size * size)
    scale = size / (2.0 * radius)
    half = size * 0.5
    for mesh in meshes:
        vs, faces = mesh[1], mesh[3]
        for vidx, *_rest in faces:
            if max(vidx) >= len(vs):
                continue
            pts = [(half + vs[i][0] * scale, half + vs[i][2] * scale) for i in vidx]
            x0 = max(0, int(min(p[0] for p in pts)))
            x1 = min(size - 1, int(max(p[0] for p in pts)) + 1)
            y0 = max(0, int(min(p[1] for p in pts)))
            y1 = min(size - 1, int(max(p[1] for p in pts)) + 1)
            if x1 < x0 or y1 < y0:
                continue
            (ax, ay), (bx, by), (cx, cy) = pts
            den = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy)
            if abs(den) < 1e-12:
                continue
            inv = 1.0 / den
            for py in range(y0, y1 + 1):
                fy = py + 0.5
                row = py * size
                for px in range(x0, x1 + 1):
                    fx = px + 0.5
                    w0 = ((by - cy) * (fx - cx) + (cx - bx) * (fy - cy)) * inv
                    if w0 < 0.0:
                        continue
                    w1 = ((cy - ay) * (fx - cx) + (ax - cx) * (fy - cy)) * inv
                    if w1 < 0.0 or w0 + w1 > 1.0:
                        continue
                    cov[row + px] = 1.0

    # Soften the edge. The engine filters its shadow target (FilterSpeed); a
    # hard-edged silhouette reads as a cut-out, which is exactly how the first
    # build looked on screen.
    #
    # The radius has to be a FRACTION OF THE TEXTURE, not a fixed pixel count. A
    # 3-tap blur run twice is about 4 mm of softness once 256 px is stretched
    # over 0.57 m -- invisible. `radius_frac` of the edge, run twice, gives a
    # penumbra of a few centimetres, which is what the retail shadow has.
    r = max(1, int(size * radius_frac))
    for _ in range(blur):
        for axis in (0, 1):
            tmp = [0.0] * (size * size)
            for a in range(size):
                # running sum over the window, so cost is O(size^2), not
                # O(size^2 * r) -- at r = 16 that is the difference between
                # instant and a minute
                acc = 0.0
                for k in range(-r, r + 1):
                    i = 0 if k < 0 else (size - 1 if k >= size else k)
                    acc += cov[a * size + i] if axis == 0 else cov[i * size + a]
                inv = 1.0 / (2 * r + 1)
                for b in range(size):
                    if axis == 0:
                        tmp[a * size + b] = acc * inv
                    else:
                        tmp[b * size + a] = acc * inv
                    lo = b - r
                    hi = b + r + 1
                    lo = 0 if lo < 0 else (size - 1 if lo >= size else lo)
                    hi = 0 if hi < 0 else (size - 1 if hi >= size else hi)
                    acc += (cov[a * size + hi] if axis == 0 else cov[hi * size + a])
                    acc -= (cov[a * size + lo] if axis == 0 else cov[lo * size + a])
            cov = tmp

    px = bytearray(size * size * 4)
    for i, c in enumerate(cov):
        a = int(c * 255.0 + 0.5)
        px[i * 4 + 3] = 255 if a > 255 else (0 if a < 0 else a)
    return size, size, 1, [bytes(px)]


def load_texture(path, want_alpha, mask=False):
    """Decode a .csi FILE. See load_texture_data."""
    return load_texture_data(open(path, "rb").read(), want_alpha, mask)


def load_texture_data(d, want_alpha, mask=False):
    """
    Decode every mip level of a .csi into (w, h, fmt, [level_bytes, ...]).

    The .csi files already carry full mip chains (up to 10 levels), so there is
    no reason to generate them at runtime -- ship them and let the GPU filter.

    Takes BYTES, not a path, because the lightmap atlases are not files: they
    are 0x8232 blobs embedded in the .sb itself (two 512x512 RGB888 images,
    single level, for beach_1).
    """
    h = csi2png.parse_header(d)
    has_alpha = any(c == "A" for c, _, _ in h.channels)
    fmt = 1 if (has_alpha and want_alpha) else 0
    # A BLEND MASK is carried in the alpha channel whatever the source had.
    #
    # `alpha`, the ten tracks' one transition mask, is a plain 128x64 RGB image:
    # its ramp is in the LUMINANCE and there is no alpha channel at all. The
    # runtime reads a mask through a combiner stage whose COMBINE_ALPHA can take
    # GL_TEXTURE -- i.e. the texture's alpha -- and has no way to route its RGB
    # into alpha, so the move has to happen here. RGB is set to white rather than
    # left as the ramp: the stage passes colour through from the unit below, and
    # a grey there would darken the surface if anything ever modulated it.
    #
    # 8 bits, not 565's 5: the ramp is the whole point of the texture and it is
    # magnified across a band about a metre wide, where 32 levels would band.
    if mask:
        fmt = 1

    # The header's data offset is not usable on an EMBEDDED image: the lightmap
    # atlases carry 0 on beach_1 and 1032 on beach_4, and both blobs are exactly
    # 160 + w*h*bpp bytes, so the pixels start straight after the header in each.
    # It is an offset into whatever the image was cut out of.
    #
    # Trusting it silently cost both ways: 0 decoded 160 bytes of HEADER as the
    # first 53 pixels and rolled the whole atlas, and 1032 overran the blob so
    # the level loop produced nothing and the track packed unlit.
    total = sum(max(1, h.width >> l) * max(1, h.height >> l) * h.bpp
                for l in range(h.mips))
    off = h.data_off
    if off < csi2png.HDR_SIZE or off + total > len(d):
        off = csi2png.HDR_SIZE

    levels = []
    for lvl in range(h.mips):
        w = max(1, h.width >> lvl)
        ht = max(1, h.height >> lvl)
        need = w * ht * h.bpp
        if off + need > len(d):
            break
        rgba = csi2png.decode_level(d, off, w, ht, h)
        if mask:
            out = bytearray(w * ht * 4)
            for i in range(w * ht):
                # Whatever channel the mask was authored in: an RGB ramp
                # (`alpha`) has it in R, and an A+I image (`coast`) has the same
                # value in both, so R is the ramp in either case.
                out[i*4] = out[i*4+1] = out[i*4+2] = 255
                out[i*4+3] = rgba[i*4]
            levels.append(bytes(out))
        elif fmt == 1:
            levels.append(rgba)
        else:
            # RGB565 halves memory for opaque textures, packed RED in the high
            # bits -- plain, standard 565.
            #
            # This used to pack BLUE high, on the belief that vitaGL feeds
            # GL_UNSIGNED_SHORT_5_6_5 to GXM's ..._U5U6U5_BGR. It does not. Read
            # off vitaGL's own source, `glTexImage2D(GL_RGB, GL_RGB,
            # GL_UNSIGNED_SHORT_5_6_5)` takes the `fast_store` path -- the pixels
            # are memcpy'd verbatim -- and selects
            # SCE_GXM_TEXTURE_FORMAT_U5U6U5_RGB (textures.c:797). The _BGR at
            # textures.c:513 is a TRANSFER format on the cube-map path, which
            # this never touches. And the SDK settles what the suffix means:
            # `SCE_GXM_COLOR_FORMAT_R5G6B5 = SCE_GXM_COLOR_FORMAT_U5U6U5_RGB`
            # (gxm.h:343), i.e. red in the high bits.
            #
            # Packing blue high therefore swapped R and B on real hardware:
            # sand_wet arrived as R=112 G=163 B=200 (BLUE sand) and sea as
            # R=135 G=117 B=50 (GOLD water), which is exactly what the Vita
            # showed. Vita3K reads U5U6U5_RGB the other way round and so looked
            # right on the old packing -- the emulator is the one that disagrees
            # with the hardware here.
            out = bytearray(w * ht * 2)
            for i in range(w * ht):
                r, g, b = rgba[i * 4], rgba[i * 4 + 1], rgba[i * 4 + 2]
                struct.pack_into("<H", out, i * 2,
                                 ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
            levels.append(bytes(out))
        off += need
    return h.width, h.height, fmt, levels


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("out")
    ap.add_argument("--csidir", default="/mnt/c/tools/rccars_extracted")
    ap.add_argument("--embdir", default="/mnt/c/tools/rccars_obj/embedded")
    ap.add_argument("--alpha", action="store_true", default=True)
    ap.add_argument("--subtree", default=None,
                    help="only pack meshes under this node (e.g. Car1)")
    ap.add_argument("--sky", default=SKY_TEX_RE.pattern,
                    help="regex over a mesh's TEXTURE names for the sky dome "
                         "(drawn camera-locked). Matching on the node name "
                         "instead is what put beach_1's column shafts and "
                         "beach_3's round walls in the sky -- see SKY_TEX_RE.")
    ap.add_argument("--rig", action="store_true",
                    help="keep the car's rig nodes articulable (writes VSC4)")
    ap.add_argument("--envmap", action="store_true",
                    help="classify the body's env-mapped parts and pack their "
                         "vertex normals, for the glance pass (writes VSC7)")
    ap.add_argument("--markers", action="store_true",
                    help="export MOD_MARKER nodes and tag water surfaces "
                         "(writes VSC5)")
    ap.add_argument("--no-blend", dest="blend", action="store_false",
                    default=True,
                    help="drop the surface-transition blend: pack a transition "
                         "face as its BAS1 texture alone, the way the port did "
                         "before the ALP1 layers were read. The mutant "
                         "vsc_check.py's blend clauses are held against.")
    ap.add_argument("--water-lod", action="store_true", default=True,
                    help="keep only the finest subdivision of each water tile")
    ap.add_argument("--extra-tex", default="",
                    help="comma-separated textures to pack even though no "
                         "geometry references them, for anything the app binds "
                         "by name at runtime -- the upgrade tyre sets, say. "
                         "--markers adds the water/checkpoint sprites to this.")
    ap.add_argument("--texset", default="auto", choices=("auto", "1", "2", "3"),
                    help="which of the pack's three texture quality sets to use. "
                         "auto (default) takes the highest-resolution copy of "
                         "each name; 1/2/3 pin it to that set, which is the knob "
                         "to reach for if texture memory becomes a problem.")
    ap.add_argument("--shadow-tex", type=int, default=0, metavar="SIZE",
                    help="bake the top-down car silhouette at SIZExSIZE as "
                         f"'{SHADOW_TEX_NAME}' (the projected shadow's texture)")
    ap.add_argument("--shadow-radius", type=float, default=0.0,
                    help="world half-extent the shadow texture covers; the "
                         "default fits it to the car (see shadow_fit_radius)")
    a = ap.parse_args()

    b = open(a.src, "rb").read()
    reg, start = sb2obj.read_registry(b)
    if a.rig:
        meshes, textures, rig = sb2obj.collect(b, reg, start, subtree=a.subtree,
                                               part_names=CAR_RIG + CAR_PARTS_EXTRA)
    else:
        meshes, textures = sb2obj.collect(b, reg, start, subtree=a.subtree)
        rig = None

    csi = index_csi(a.csidir, a.texset if a.texset != "auto" else None)
    for p in glob.glob(os.path.join(a.embdir, "*.csi")):
        csi.setdefault(os.path.splitext(os.path.basename(p))[0].lower(), p)

    markers = collect_markers(b, reg, start, MARKER_RE) if a.markers else []
    coarse = pick_water_lods(meshes) if (a.markers and a.water_lod) else set()

    # group geometry by texture, de-indexing (pos, uv) pairs
    groups = {}          # texname -> [ {vmap, verts, idx}, ... ]  (split at 64k)
    dropped = 0
    coarse_tris = 0
    sky_re = re.compile(a.sky, re.I) if a.sky else None
    for mesh_id, mesh in enumerate(meshes):
        nm, vs, uv, faces = mesh[:4]
        if mesh_id in coarse:
            coarse_tris += len(faces)
            continue
        # A batch is drawn under one matrix, so geometry from two different rig
        # parts can never share one -- the part joins the grouping key.
        part = mesh[4] if rig is not None else 0
        tex_here = {f[2] for f in faces}
        flags = sky_kind(tex_here, sky_re)
        if (nm and TRANSP_RE.search(nm)) or any(
                t and ICON_TEX_RE.match(t) for t in tex_here):
            flags |= BATCH_TRANSP
        if a.markers:
            flags |= surface_kind(nm, tex_here)
        env = env_class(nm, tex_here) if a.envmap else ENV_NONE
        for (vidx, uvidx, tname, lmidx, lmname,
             uvidx2, tname2, aidx, aname) in faces:
            if max(vidx) >= len(vs):
                dropped += 1
                continue
            # A transition face is one that names a SECOND base texture AND the
            # mask that blends it in. Both, or neither: an ALP1 layer with no
            # BAS2 is the coast band, which water.c owns and this leaves alone
            # (see THE SURFACE TRANSITION above), and a BAS2 with no mask has
            # nothing to blend by.
            blend = bool(a.blend and tname2 and aname and tname2 != tname)
            fflags = flags | (BATCH_BLEND if blend else 0)
            if not blend:
                tname2 = aname = None
            # A batch is drawn with ONE lightmap bound, so the atlas joins the
            # key alongside the base texture. The env class joins it too: the
            # glance pass draws a whole batch at one alpha. The second base and
            # the mask join it because a batch is drawn with one of each as well.
            g = groups.setdefault((tname, fflags, part, lmname, env,
                                   tname2, aname),
                                  [{"vmap": {}, "verts": [], "bverts": [],
                                    "idx": []}])
            cur = g[-1]
            if len(cur["verts"]) + 3 > MAX_VERTS:
                cur = {"vmap": {}, "verts": [], "bverts": [], "idx": []}
                g.append(cur)
            for k in range(3):
                vi = vidx[k]
                ti = uvidx[k] if (uvidx and uvidx[k] < len(uv)) else None
                li = lmidx[k] if (lmidx and lmidx[k] < len(uv)) else None
                bi = uvidx2[k] if (blend and uvidx2 and uvidx2[k] < len(uv)) else None
                ai = aidx[k] if (blend and aidx and aidx[k] < len(uv)) else None
                # vi/ti are MESH-LOCAL, but a batch merges many meshes -- the
                # mesh id must be part of the key or vertices from different
                # meshes collide into one slot and the geometry shreds. The
                # lightmap index joins it for the same reason: two faces can
                # share a position and a base UV and still light differently.
                # So do the blend's two, or a vertex shared by two transition
                # bands would take the first band's mask ramp into the second.
                key = (mesh_id, vi, ti, li, bi, ai)
                slot = cur["vmap"].get(key)
                if slot is None:
                    slot = len(cur["verts"])
                    cur["vmap"][key] = slot
                    px, py, pz = vs[vi]
                    tu, tv = uv[ti] if ti is not None else (0.0, 0.0)
                    lu, lv = uv[li] if li is not None else (0.0, 0.0)
                    cur["verts"].append((px, py, pz, tu, tv, lu, lv))
                    # Kept in lockstep with `verts`, so one slot indexes both.
                    if blend:
                        bu, bv = uv[bi] if bi is not None else (tu, tv)
                        mu, mv = uv[ai] if ai is not None else (0.0, 1.0)
                        cur["bverts"].append((bu, bv, mu, mv))
                cur["idx"].append(slot)

    names = sorted({t for (t, _fl, _part, _lm, _e, _t2, _a) in groups if t}
                   | {lm for (_t, _fl, _part, lm, _e, _t2, _a) in groups if lm}
                   | {t2 for (_t, _fl, _part, _lm, _e, t2, _a) in groups if t2}
                   | {am for (_t, _fl, _part, _lm, _e, _t2, am) in groups if am})
    # Which of them are blend MASKS. `alpha` is the only one in the ten tracks,
    # and it is an RGB image: its ramp has to be moved into the alpha channel,
    # because the runtime combiner reads a mask's ALPHA and cannot read its RGB.
    MASK_TEXTURES = {am for (_t, _fl, _part, _lm, _e, _t2, am) in groups if am}
    # Textures the app binds by name rather than through a batch: the water and
    # checkpoint sprites on a track, the upgrade tyre sets on a car. Only one
    # tyre level is ever assigned in the mesh, so the other three have to be
    # asked for explicitly or there is nothing to swap to.
    extra = list(EXTRA_TEX) if a.markers else []
    extra += [n for n in a.extra_tex.split(",") if n]
    names += [n for n in extra if n not in names]
    tex_blobs, tex_index, missing_tex = [], {}, []
    for n in names:
        # An embedded image wins over a same-named file: the lightmap atlases
        # only exist inside the .sb.
        blob = textures.get(n, {}).get("embedded")
        p = csi.get(n.lower())
        if not blob and not p:
            missing_tex.append(n)
            continue
        try:
            if blob:
                w, h, fmt, levels = load_texture_data(b[blob[0]:blob[1]], a.alpha,
                                                      mask=n in MASK_TEXTURES)
            else:
                w, h, fmt, levels = load_texture(p, a.alpha,
                                                 mask=n in MASK_TEXTURES)
        except Exception:
            missing_tex.append(n)
            continue
        if not levels:
            missing_tex.append(n)
            continue
        tex_index[n] = len(tex_blobs)
        tex_blobs.append((n, w, h, fmt, levels))

    shadow_radius = 0.0
    if a.shadow_tex:
        shadow_radius = a.shadow_radius or shadow_fit_radius(meshes)
        w, h, fmt, levels = bake_shadow(meshes, a.shadow_tex, shadow_radius)
        tex_index[SHADOW_TEX_NAME] = len(tex_blobs)
        tex_blobs.append((SHADOW_TEX_NAME, w, h, fmt, levels))

    # Which textures are multiply decals rather than surfaces. Decided here, on
    # the DECODED pixels, rather than in the grouping loop above -- the loop runs
    # before any texture is opened, and re-decoding every level a second time to
    # answer one question about level 0 would double the pack time. It costs
    # nothing to OR the bit in below instead: the flag is a pure function of
    # `tname`, which is already part of the group key.
    modulate = {n for n, w, h, fmt, levels in tex_blobs
                if signed_detail_map(w, h, fmt, levels[0])}

    # And which of them are worth BLENDING. The artists' TRANSP tag means "not
    # fully opaque", which covers both a soft painted mark and a hard cut-out:
    # `trees_2` and `trellis` are tagged and are cut-outs, and blending a cut-out
    # stops its border being rejected and haloes every leaf. So the names say
    # "this needs transparency" and the PIXELS decide which kind.
    #
    # A broad alpha ramp is the discriminator, and it separates cleanly here:
    # everything that wants blending runs 17.2% to 52.5% of texels strictly
    # between transparent and opaque (icon_start 35.9, icon_way_1 36.4, flora_7
    # 22.5, glass 17.2, list_test 52.5) and the cut-outs sit at 0.1% (trellis) to
    # 1.3% (trees_2). 5% is an order of magnitude clear of both.
    #
    # A tagged texture with NO alpha channel keeps the flag: SAIL_G and SAW are
    # tagged and carry none, and blending at a uniform alpha of 1 is exactly what
    # drawing them opaque already does, so it costs a pass and changes nothing.
    def blendable(w, h, fmt, level0):
        if fmt != 1:
            return True                       # no alpha channel packed
        n = w * h
        part = sum(1 for i in range(n) if 8 < level0[i * 4 + 3] < 248)
        return part * 100.0 >= 5.0 * n

    blend_ok = {n for n, w, h, fmt, levels in tex_blobs
                if blendable(w, h, fmt, levels[0])}

    batches = []
    for (tname, flags, part, lmname, env, tname2, aname), gl in groups.items():
        ti = tex_index.get(tname, 0xFFFFFFFF)
        li = tex_index.get(lmname, 0xFFFFFFFF)
        t2i = tex_index.get(tname2, 0xFFFFFFFF)
        ami = tex_index.get(aname, 0xFFFFFFFF)
        # A blend needs both of its own textures. If either failed to resolve the
        # bit comes off and the batch draws as plain BAS1 -- the pre-blend
        # appearance, which is wrong but is geometry the player can stand on,
        # where a missing mask would blend against an undefined sampler.
        if (flags & BATCH_BLEND) and (t2i == 0xFFFFFFFF or ami == 0xFFFFFFFF):
            flags &= ~BATCH_BLEND
            t2i = ami = 0xFFFFFFFF
        if (flags & BATCH_TRANSP) and tname not in blend_ok:
            flags &= ~BATCH_TRANSP            # tagged, but it is a cut-out
        if tname in modulate:
            flags |= BATCH_MODULATE
            # THE PORT'S, and a one-track divergence. A modulate decal multiplies
            # the framebuffer, and the ground already in it carries its own
            # lighting -- sampling the atlas again on top of that darkens the
            # strip a second time, so in shade it would draw as a dark band,
            # which is the opposite of the bug it exists to fix.
            #
            # Nine of the ten tracks author these faces with no lightmap at all
            # (0 lit faces); country_4 alone lights all 788 of its. That is the
            # artists being inconsistent, not a mechanism, so the flag decides
            # rather than the authoring. The LM UVs stay in the vertices and are
            # simply not used.
            li = 0xFFFFFFFF
        for g in gl:
            if g["idx"]:
                nrm = vertex_normals(g["verts"], g["idx"]) if env else None
                bvs = g["bverts"] if (flags & BATCH_BLEND) else None
                batches.append((ti, flags, part, li, env, nrm, t2i, ami, bvs,
                                g["verts"], g["idx"]))

    parts = []
    if rig is not None:
        for nm, parent, M, T in rig:
            rest = mat_from_mt(M, T)
            rest_inv = mat_inverse_affine(rest)
            # local = rest relative to the parent part: the matrix the engine
            # snapshots into the ani context (carInitAniProc1 0x00504470 copies
            # node+0xac, the node's LOCAL transform) and pre-multiplies each
            # frame's rotation onto.
            pinv = mat_inverse_affine(mat_from_mt(rig[parent][2], rig[parent][3])) \
                if parent >= 0 else None
            local = mat_mul(rest, pinv) if pinv else list(rest)
            parts.append((nm, parent, rest, rest_inv, local))

    # The version follows the DATA, the way it already does for the env map: a
    # scene with a transition band needs the two extra texture slots and the
    # extra UV sets, and a scene without one (country_1 has none, and no car
    # does) writes the older layout that every existing reader accepts.
    n_blend = sum(1 for bb in batches if bb[1] & BATCH_BLEND)
    ver = 9 if n_blend else (7 if a.envmap else 6)
    with open(a.out, "wb") as f:
        f.write(b"VSC%d" % ver)
        f.write(struct.pack("<II", len(tex_blobs), len(batches)))
        if ver >= 4:
            f.write(struct.pack("<I", len(parts)))
        if ver >= 5:
            f.write(struct.pack("<If", len(markers), shadow_radius))
        # VSC8's model count. This packer writes no model table -- pack_props.py
        # owns that -- but the FIELD is in the header from 8 on, and scene.c reads
        # it for any version that high. Leaving it out made the loader take the
        # first texture's name length as the model count.
        if ver >= 8:
            f.write(struct.pack("<I", 0))
        for n, w, h, fmt, levels in tex_blobs:
            nb = n.encode("latin-1")
            f.write(struct.pack("<H", len(nb))); f.write(nb)
            f.write(struct.pack("<HHBB", w, h, fmt, len(levels)))
            for px in levels:
                f.write(px)
        for nm, parent, rest, rest_inv, local in parts:
            nb = nm.encode("latin-1")
            f.write(struct.pack("<H", len(nb))); f.write(nb)
            f.write(struct.pack("<i", parent))
            f.write(struct.pack("<16f", *rest))
            f.write(struct.pack("<16f", *rest_inv))
            f.write(struct.pack("<16f", *local))
        for nm, T, yaw in markers:
            nb = nm.encode("latin-1")
            f.write(struct.pack("<H", len(nb))); f.write(nb)
            f.write(struct.pack("<4f", T[0], T[1], T[2], yaw))
        for ti, flags, part, li, env, nrm, t2i, ami, bvs, verts, idx in batches:
            f.write(struct.pack("<III", ti, flags, part))
            if ver >= 6:
                f.write(struct.pack("<I", li))
            if ver >= 7:
                f.write(struct.pack("<I", env))
            # VSC8's model index. A track has one model and this packer writes no
            # model table, so it is always 0 -- but the FIELD has to be there or
            # every batch after the first is read at the wrong offset by a reader
            # that has seen a props.vsc. pack_props.py owns the table itself.
            if ver >= 8:
                f.write(struct.pack("<I", 0))
            if ver >= 9:
                f.write(struct.pack("<II", t2i, ami))
            f.write(struct.pack("<II", len(verts), len(idx)))
            for v in verts:
                f.write(struct.pack("<7f", *v))
            if ver >= 7 and env:
                for n in nrm:
                    f.write(struct.pack("<3f", *n))
            # The blend's own UVs, after the normals and before the indices, and
            # only on a blend batch -- same rule and same reason as the normals.
            if ver >= 9 and (flags & BATCH_BLEND):
                for bv in bvs:
                    f.write(struct.pack("<4f", *bv))
            f.write(struct.pack(f"<{len(idx)}H", *idx))

    nv = sum(len(v) for *_, v, _ in batches)
    ni = sum(len(i) for *_, i in batches)
    tex_names_by_ix = {ix: n for n, ix in tex_index.items()}
    nlm = sum(1 for bb in batches if bb[3] != 0xFFFFFFFF)
    tb = sum(sum(len(x) for x in lv) for _, _, _, _, lv in tex_blobs)
    mips = sum(len(lv) for _, _, _, _, lv in tex_blobs)
    kinds = {"sky": BATCH_SKY, "water": BATCH_WATER, "coast": BATCH_COAST,
             "stream": BATCH_STREAM, "fall": BATCH_FALL, "pool": BATCH_POOL,
             "modulate": BATCH_MODULATE, "transp": BATCH_TRANSP,
             "blend": BATCH_BLEND}
    tally = ", ".join(f"{n} {sum(1 for _, fl, *_ in batches if fl & bit)}"
                      for n, bit in kinds.items()
                      if any(fl & bit for _, fl, *_ in batches))
    print(f"{os.path.basename(a.src)} -> {os.path.basename(a.out)}  (VSC{ver})")
    print(f"  textures {len(tex_blobs)}  {mips} mip levels  ({tb/1048576:.1f} MB pixels)"
          + (f"  MISSING {', '.join(missing_tex[:6])}" if missing_tex else ""))
    print(f"  batches  {len(batches)}"
          + (f" ({tally})" if tally else "")
          + f"  verts {nv:,}  tris {ni//3:,}"
          + (f"  ({dropped} bad faces dropped)" if dropped else ""))
    if coarse:
        print(f"  water    dropped {len(coarse)} coarser LOD tiles "
              f"({coarse_tris:,} tris) -- one subdivision per patch kept")
    print(f"  lightmap {nlm} of {len(batches)} batches lit")
    if n_blend:
        btris = sum(len(bb[10]) // 3 for bb in batches if bb[1] & BATCH_BLEND)
        pairs = sorted({(tex_names_by_ix.get(bb[0]), tex_names_by_ix.get(bb[6]))
                        for bb in batches if bb[1] & BATCH_BLEND})
        print(f"  blend    {n_blend} batches, {btris:,} tris over "
              f"{len(pairs)} texture pairs: "
              + ", ".join("%s/%s" % pr for pr in pairs[:6])
              + (" ..." if len(pairs) > 6 else ""))
    elif a.blend and not a.rig:
        print("  blend    none -- only country_1 authors no transition band")
    if a.envmap:
        names_by_env = {ENV_BODY: "body", ENV_GRE1: "GRE1", ENV_GRE2: "GRE2",
                        ENV_UPGRADES: "upgrades"}
        etally = ", ".join(
            f"{nm} {sum(1 for _, _, _, _, e, *_ in batches if e == k)}"
            for k, nm in names_by_env.items()
            if any(e == k for _, _, _, _, e, *_ in batches))
        print(f"  envmap   {sum(1 for _, _, _, _, e, *_ in batches if e)} of "
              f"{len(batches)} batches glance"
              + (f" ({etally})" if etally else " -- NONE MATCHED"))
    if a.markers:
        cps = sum(1 for m in markers if m[0].lower().startswith("cp_"))
        wav = sum(1 for m in markers if m[0].lower().startswith("water_wave"))
        sun = [m for m in markers if m[0].upper() == "SUN_AF"]
        print(f"  markers  {len(markers)} ({cps} checkpoint, {wav} wave spawn, "
              + (("sun at %.1f %.1f %.1f" % sun[0][1]) if sun
                 else "NO SUN -- urban_1 and urban_2 have none, and that is the"
                      " original's answer for them") + ")")
    if a.shadow_tex:
        print(f"  shadow   {a.shadow_tex}x{a.shadow_tex} silhouette baked over "
              f"+/-{shadow_radius:.3f} m as '{SHADOW_TEX_NAME}'"
              + ("" if a.shadow_radius else " (fitted)"))
    if rig is not None:
        # MIDDLE_* is the Hummer's third axle; a four-wheeler simply has none.
        have = {p[0] for p in parts}
        missing = [n for n in CAR_RIG
                   if n not in have and "MIDDLE" not in n]
        print(f"  parts    {len(parts)} (rig)"
              + (f"  MISSING {', '.join(missing)}" if missing else ""))
    print(f"  file     {os.path.getsize(a.out)/1048576:.1f} MB")


if __name__ == "__main__":
    main()
