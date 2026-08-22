#!/usr/bin/env python3
"""
Build a collision grid (.col) from an .sb track for the Vita app.

The engine ships its own collision data in the MOD_COLLISION node, and its index
layer is now decoded (see FORMAT_NOTES.md): a 156 x 12 x 320 voxel grid whose
128,289 entries reference the scene's own MOD_MESH nodes by index, validated by
storing each mesh's triangle count alongside (483/483 match). So the game
collides against its RENDER geometry -- 52,206 of beach_1's 52,358 triangles --
and there is no separate collision hull to recover. Deriving collision from the
visible geometry, as this does, is therefore equivalent up to the filters below.

Still not decoded: the 2.6 MB byte blob, which appears to hold per-voxel
per-mesh triangle bitmasks. Recovering it would buy a much cheaper broadphase,
not different collision.

A uniform grid over XZ keeps the runtime query cheap: a downward ray only has
to test the triangles registered in one cell, not all 52,000.

Layout (little-endian):

    char[4] "COL3"
    f32 min_x, min_z, cell           grid origin and cell size
    u32 nx, nz                       grid dimensions
    u32 ntris
    u32 default_surf                 fallback material, see SURFACES below
    f32[9] * ntris                   triangle vertices, world space
    u32[nx*nz + 1] cell_start        prefix offsets into tri_idx
    u32[...] tri_idx                 triangle indices per cell
    u8[ntris] surf                   material class per triangle (AUDIO)
    f32[nx*nz] water_y               COL3: water surface height per cell
    u8[ntris] eng_surf               COL4: the ENGINE's surface class per
                                     triangle, 0 = no opinion. See
                                     eng_surface_class -- this is the game's own
                                     FUN_00534fc0 data, and it is what the tyre
                                     marks key their strength on. Kept separate
                                     from `surf`, which drives audio and is a
                                     keyword guess at the same idea.

COL1 is the same file without `default_surf` and the trailing `surf` array;
COL2 is the same file without `water_y`; COL3 is the same file without
`eng_surf`. col.c still loads all of them -- a COL1 grid reports SURF_DEFAULT
everywhere, a COL1 or COL2 grid reports no water anywhere, and a pre-COL4 grid
reports engine class 0 everywhere, which puts the tyre marks back on the flat
strength they had before and costs nothing else.

WATER
-----
Water surfaces are EXCLUDED from the collision triangles (see --exclude-name
below) and their height recorded separately, because that is what they are in the
original: not a floor you can drive on, but not nothing either. carSurfaceDrag
(0x004eeea0) asks the engine's water probe (0x00531b10) how far each wheel sits
below the surface and applies a quadratic drag, ramped in between 30% and 90% of
the wheel radius submerged. The probe answers out of a per-wheel record that the
WaterLOD module fills while animating the surface; the port keeps no such record,
so col_water_at answers from this grid instead.

Per cell: the height of the WATER SURFACE ITSELF at the cell's centre, taken from
whichever UPWARD-FACING water triangle actually covers that centre, or NO_WATER.
Three things about that:

  - **a `sea` face is stored where it is DRAWN, not where it is authored.** The
    WaterLOD surface animator (FUN_005240c0) writes every surface vertex to
    restY + offset + (1-damp)*magnetOffset, and water.c does the same; the grid
    used to store restY and nothing else, so the height the car felt and the
    height it could see disagreed by the whole of `offset` -- 0.37 m on beach_1.
    Measured against the shipped grids that put 234 m2 of beach_1 and 459 m2 of
    beach_4 under water the drawn sea was nowhere near, which is drag and a
    water sound on dry sand. Reported as exactly that.

    Only the `sea` faces. The coast band, the stream, the falls and the pools
    are a different scanner and a different animator (animate_scroll), which
    holds them at rest and lifts them 2 cm as a decal; their authored height IS
    their drawn height to within that, and the river has to stay fordable.

    `offset` and the magnet pair are PER TRACK, out of WLOD_<track>_more1, and
    two of the three do not convert as raw*0.01 -- see gen_vis_data.py. The
    packer reads the same section the header generator does rather than being
    handed a number, so the two cannot drift.

  - upward-facing only, because `wtr_fall` is a vertical sheet. Registered by
    bounding box it would report water metres up, and a car driving past the foot
    of a waterfall on dry land would wade through it. NOT covered by any harness
    check, deliberately: no assertion on the finished grid can tell a wrongly
    flooded cell from a genuinely deep one, because the real sea shelf runs 6 m
    under the surface on beach_3. The count of rejected faces is printed instead
    (32 on beach_1, 0 to 3 elsewhere), so a regression shows in the pack output.
  - **the cell centre must be INSIDE the triangle.** This used to stamp each
    triangle's XZ BOUNDING BOX, and the docstring here used to claim that
    over-covers "by up to one cell". That is only true if a triangle is no bigger
    than a cell, and the sea is not: it ships as broad LOD tiles whose individual
    triangles run to **68 m across** on country_3, 32 to 54 m on the beaches. A
    long diagonal triangle's box covers roughly twice the triangle, so the sea
    flooded a fat rectangle of the beach behind it -- measured over the ten
    tracks, **14 to 27 % more cells than the water actually covers**, up to
    5,544 m2 of dry land on beach_3 and 75 % of country_1's wet cells. Reported
    as the car slowing down on a sand ramp with the sea nowhere near it, which is
    exactly what it was.

    Testing the centre instead can only UNDER-cover, by at most half a cell at
    the shoreline, and that is the harmless direction: the drag is thresholded
    anyway. It needs 30% of a wheel radius submerged, and the wheel centre sits
    one radius above the ground, so it takes

        ground < water_y - 0.3 * radius        (2.15 cm on the Overkill)

    before a single newton appears.

    The height is the triangle's PLANE evaluated at that centre, not the
    triangle's maximum y -- max-y biases every cell up to the highest corner of
    whatever tile reaches it, which on a 68 m triangle is a metre of pure error.

    So the thing to check after changing any of this is not the fuzz, it is that
    no start goes deep. rccars_re/wetcheck.c measures all ten in exactly those
    terms against the packed grids; rb_test part 6b pins the ramp arithmetic.

SURFACES
--------
The game ships a surface sound per material -- car_surf_{sand,wetsand,gravel,
stone,asphalt,grass,metal,wood,water} and a matching car_hit_* set -- and the
engine picks one through its own classifier (0x00534fc0), which is NOT
recovered. What IS available is the thing that classifier must itself have been
keyed on: the texture the artist put on the face. The names are descriptive and
mostly transliterated Russian, so SURF_RE below matches on keyword.

Measured over all ten tracks, weighted by UPWARD-FACING area -- the surfaces a
car can actually be on, which is the only weighting that means anything here:

    gravel 20.7%  stone 17.2%  wetsand 16.9%  asphalt 15.1%  metal 8.3%
    grass 5.2%  wood 4.8%  sand 0.8%      unmatched 10.9%

The unmatched remainder is mostly untextured geometry (12.9% of all faces carry
no texture name at all), and it falls back to `default_surf` -- the track's own
dominant floor material, computed here rather than guessed, which is how beach_1
ends up on wetsand and urban_2 on gravel instead of everything defaulting to one
global choice that is wrong on seven tracks out of ten.

This classification drives AUDIO ONLY. It is deliberately not plumbed into
rb_wheel_contact.surface, whose value 3 means deep sand to rb_surface_drag
(contact.c) and comes from the engine's own classifier; overloading that field
would change the physics.

Usage: python3 pack_col.py <file.sb> <out.col> [--cell 3.0] [--sky REGEX]
"""

import argparse
import collections
import math
import os
import re
import struct

import pack_vsc
from gen_vis_data import wsurf_section, wsurf_values
import sb2obj

# Must match sfx.h's SURF_*. Index 0 is the fallback.
SURF_NAMES = ["asphalt", "sand", "wetsand", "gravel", "stone",
              "grass", "metal", "wood", "water"]

# Ordered: first match wins, so the specific cases come before the general
# ones. `sand_wet` must beat `sand`, and `rock_05re_grass_c4` is grass.
#
# THE NAMES BELOW WERE CHECKED AGAINST THE ARTWORK, and six of them had to move.
# Reported as "on the pier there is no wood sound, there is water sound". Half of
# that was rb_collide's water gate (see collide.c); the other half was here, and
# it took looking at the pictures to find, because every one of the six is a name
# whose ENGLISH reading and whose TEXTURE disagree:
#
#   floor           beach_1/3   weathered grey plank decking -- THE PIER DECK
#   lower_part      beach_1/3   dark planks with timber framing -- its walls
#   beam_wight      beach_1/2/3 white-painted decking, grain and knots
#   pile*           4 tracks    weathered grey planks (pile_round_01 is log grain)
#   prokat          beach_1/2   a beach-rental signboard on white boards
#   floor_met       urban_2     says "met" in its own name
#   floor_2         beach_3     set cobbles, i.e. stone, not asphalt
#
# `floor` and `lower_part` are the two that answer the report: sections through
# both of beach_1's over-water buildings run lower_part 1.0-2.0, floor 1.2-2.2,
# and it is that `floor` the car drives on. What is ABOVE it -- wood_wight at
# 3.5-4.8, then `tile` (bamboo) or `tin` (corrugated sheet) at 4.6-5.3 -- is the
# ceiling and the roof, which is why "the topmost floor over a water cell" is the
# wrong thing to measure here and a vertical section is the right one.
#
# Anchors are load-bearing: unanchored `floor` in the wood list would also claim
# floor_met and floor_2, and `tile` must stay with asphalt so ruberoid_tile does.
SURF_RE = [
    ("grass",   r"grass|flora|trava|gazon|kust|bush|trees|tree_|leaf|list_"),
    ("wetsand", r"sand_wet|sand_halfdry|wet_sand|sand_trample|rubbish_sand"),
    ("gravel",  r"gravel|sheben|shingle|galka|cobble|path|gruntov"),
    ("sand",    r"sand|pesok|dune|bags"),
    ("wood",    r"wood|drova|doska|board|parket|brevn|pier|most|box|yashik|"
                r"crate|stolb|planke|fanera|beam_wight|pile|prokat|"
                r"^floor$|lower_part"),
    ("metal",   r"met_|metal|metall|jaluzi|konteyner|hangar|bunker|truba|fence|"
                r"net$|_net|rail|zhest|profil|tank|barrel|bochk|radiator|grate|"
                r"rust|wire|provod|ferm|balk|shveller|lest|floor_met"),
    ("stone",   r"rock|stone|kamen|montain|mont|granit|brick|kirpi|cliff|gora|"
                r"mine|skala|valun|ruin|^floor_2$"),
    ("asphalt", r"beton|asphalt|asfalt|road|floor|trasition|transition|plita|"
                r"concrete|tile|pavement|dor$|dor_|bordur|border|stairs|step|"
                r"base|basa|baza|column|wall|roof|arc|arm|cleen|cafe|butt_end|"
                r"ang_dor|pol_|potolok|niz|verh|ruberoid"),
]
SURF_RE = [(k, re.compile(v, re.I)) for k, v in SURF_RE]


def classify(tname):
    """Texture name -> surface index, or None if no keyword matches."""
    if not tname:
        return None
    for k, rx in SURF_RE:
        if rx.search(tname):
            return SURF_NAMES.index(k)
    return None


# ---------------------------------------------------------------------------
# The ENGINE's own surface classifier, recovered.
#
# FUN_00534fc0 (158 bytes, never decompiled) walks the faces under a contact,
# calls FUN_00535060 on each face's texture field +0x3c, and keeps the MINIMUM
# class in 1..10, returning 0 if there is none. FUN_00535060 (0x00535060) is a
# priority chain over that field's low bits, read straight off the disassembly:
#
#     test al,0x001 -> 2        test al,0x010 -> 4
#     test al,0x002 -> 1        test al,0x020 -> 5
#     test al,0x004 -> 3        test al,0x080 -> 8
#     test al,0x008 -> 7        test ah,0x001 -> 6   (i.e. 0x100)
#                               and  0x200    -> 9, else 0
#
# 0x040 is never tested, so a texture carrying only that bit reads 0.
#
# The field is the texture declaration's 0x3408 WORD 4, SHIFTED DOWN BY FOUR.
# Four independent things pin that down:
#
#   * the sand names only come out right in this bit order. word4 0x10 is
#     `sand_wet` and lands on class 2 = wetsand; word4 0x20 is `sand_halfdry`
#     and lands on class 1 = sand. Swap the first two tests and the two sands
#     trade places with their own names.
#   * gen_fx_data.py's dust table, derived earlier from which `surf_<kind>.crs`
#     files SHIP, is 0 default, 1 sand, 2 wetsand, 3 dunesand, 5 gravel -- and
#     4, 6, 7, 8 have no file at all, i.e. no dust. Every one lands on the class
#     this chain gives, so "asphalt, grass, metal and wood raise nothing" comes
#     out of the data as well as out of the .crs set.
#   * `Sand_gravel` -> gravel, `grating` -> metal, `floor_met` -> metal: three
#     textures whose own names state the answer.
#   * all six names this port had to reclassify by OPENING THE PNG -- floor,
#     lower_part, beam_wight, pile, pile_round_01, prokat -> wood -- agree with
#     the data exactly, as do floor_met -> metal and floor_2 -> stone. Eight
#     predictions, eight hits, from a rule derived out of machine code.
#
# Class 0 means NO OPINION, and that is why FUN_00534fc0 takes the minimum over
# the POSITIVE classes: a decal stacked on a floor -- `wt_wetsand` carries
# word4 0 -- contributes nothing and the floor underneath decides. Do not map
# class 0 onto a material.
#
# This is NOT the audio/dust material in `surf` above, and the two are kept
# apart deliberately. The keyword list agrees with this data on only 26% of the
# 319 textures and has no opinion at all on 47%, so swapping it in would move a
# great many sounds; that is its own change, with its own listening pass. What
# this array is for is the one thing that has no other source: the tyre mark's
# per-surface strength.
ENG_SURF_NAMES = ["default", "sand", "wetsand", "dunesand", "grass",
                  "gravel", "metal", "wood", "stone", "cls9"]


def eng_surface_class(words):
    """A texture's 0x3408 record -> the engine's surface class, 0 if none."""
    if not words or len(words) < 5:
        return 0
    v = words[4] >> 4
    if v & 0x001:
        return 2
    if v & 0x002:
        return 1
    if v & 0x004:
        return 3
    if v & 0x008:
        return 7
    if v & 0x010:
        return 4
    if v & 0x020:
        return 5
    if v & 0x080:
        return 8
    if v & 0x100:
        return 6
    return 9 if (v & 0x200) else 0


def tri_up_area(t):
    """Area of a triangle if it is a floor, else 0. Floors decide the default."""
    ux, uy, uz = (t[1][i] - t[0][i] for i in range(3))
    vx, vy, vz = (t[2][i] - t[0][i] for i in range(3))
    cx = uy * vz - uz * vy
    cy = uz * vx - ux * vz
    cz = ux * vy - uy * vx
    n = math.sqrt(cx * cx + cy * cy + cz * cz)
    if n < 1e-9 or abs(cy) / n < 0.7:
        return 0.0
    return 0.5 * n


# What col.c writes into a cell with no water. Any sentinel below every real
# terrain height works; this one is chosen so `wheel_y - water_y` cannot come out
# finite-and-small by accident, and so it survives a f32 round trip.
NO_WATER = -1.0e30

# Everything the artists lay ON the ground rather than build out of it: the
# checkpoint icons (pack_vsc.py's BATCH_TRANSP) and the sand transition strips
# (BATCH_MODULATE). Used only to CHECK the finished grid -- what actually drops
# them is --no-collide's ACP tag and --exclude-tex's `^wt_`, two different rules,
# so this list is an independent witness over both.
PAINT_TEX = re.compile(r"^icon_|^wt_", re.I)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("out")
    ap.add_argument("--cell", type=float, default=3.0)
    # THE SKY DOME IS DROPPED FROM COLLISION BY ITS TEXTURE, NOT ITS MESH NAME.
    #
    # This default used to be the mesh-name regex `^pCylinder|^NOSHDW_`, and it is
    # why a reported kerb had no collision at all: this filter DELETES the
    # collision of every mesh it matches, and it matched far more than the dome.
    #
    #   ^pCylinder  is what Maya calls every cylinder primitive. On beach_2 that
    #               is `pCylinder1`, the 148-triangle kerb the gas pumps stand on
    #               -- reported as "curb just has no collision, i can ride in it
    #               from any angle", which is exactly what it was. Also beach_3's
    #               two round walls, beach_1's two column shafts, and the `kooler`
    #               barrels on country_4, urban_1 and urban_2.
    #   ^NOSHDW_    is the artists' "casts no shadow" tag and has nothing to do
    #               with collision. 83 meshes carry it, including a `sand_wet`
    #               ground plane spanning 100 m of beach_2.
    #
    # The identical mistake was in pack_vsc.py's --sky, where it drew those same
    # meshes as camera-locked sky. Fixing that one made them VISIBLE, which is why
    # the missing collision was reported immediately afterwards: the geometry had
    # never been in the grid, but until then there was nothing on screen to drive
    # at. TWO extractors, ONE wrong rule, and only the visible half got noticed.
    #
    # Both packers now key on `^sky` over the mesh's TEXTURE names, and every one
    # of the ten domes carries sky_up / sky_up_c / sky_up_c2 / sky_up_u.
    ap.add_argument("--sky", default=r"^sky")
    # `^wt_` is the sand transition strips, and they are the SAME fault as the
    # checkpoint markings below: pack_vsc.py draws them as BATCH_MODULATE decals
    # because they are 64x256 near-greyscale strips bordered at exactly 128 that
    # multiply the ground, and they are authored 5-21 mm ABOVE it (median 1 cm,
    # every one of beach_1's 776 vertices with a sand face under it). In the
    # collision grid that centimetre is a lip the car climbs at the edge of every
    # strip. Measured: 330 of beach_1's 776 decal vertices sat on a surface the
    # decal raised, mean 10.0 mm, worst 25.3 on country_4 -- and dropping them
    # loses no floor anywhere, because the sand they modulate is right underneath.
    #
    # By TEXTURE and not by mesh name, for two reasons: these meshes are named
    # `obj30` and `polySurface5134`, which is Maya's naming and says nothing (the
    # `pCylinder` lesson), and a per-FACE rule lets a mesh that mixes a strip with
    # real geometry keep the real half. It also costs nothing in COL2/COL4: their
    # engine surface class is 0, "no opinion", which is exactly why
    # col_surface_at's min-over-positive rule already let the sand underneath
    # decide.
    #
    # NOT `Sand_trample`, which reads like the same thing and is not: it is MIXED
    # into the terrain meshes themselves on every track that has it, i.e. it is
    # ground the car drives on, and dropping it would cut real holes.
    ap.add_argument("--exclude-tex", default=r"^sea$|^sky|^wt_",
                    help="drop faces using these textures (the ocean, the sky, "
                         "the sand transition decals) so the car cannot drive "
                         "on them")
    # The ocean is excluded by its `sea` texture, but the STREAM and the coast
    # foam band are not -- and they were landing in the grid as solid floors.
    # Measured on beach_1: the collision surface under every one of the stream's
    # 325 vertices sat at exactly 0.00 m below it, because the collision surface
    # WAS the stream. The car drove on the river.
    #
    # These are the same names pack_vsc.py classifies surfaces by, and the
    # default is BUILT FROM ITS TABLE rather than retyped, so the two cannot
    # drift: the needles are the engine's own (0x575710 for the stream family,
    # 0x51c950 for the coast) and the test is its own too -- a SUBSTRING.
    #
    # The `^...$` anchors this carried until now were wrong the same way
    # pack_vsc.py's were, and here it puts a floor where the engine has water:
    # beach_2's `LM0_NOSHDW_coast1` is the only one of the four missed families
    # that this filter still had to catch on its own, because pool and waterfall
    # meshes are `sea`-textured and --exclude-tex already drops those.
    ap.add_argument("--exclude-name",
                    default="|".join(re.escape(n) for n, _ in
                                     pack_vsc.SURFACE_NEEDLES),
                    help="drop meshes with these names (water surfaces)")
    # Which of the dropped faces are WATER, as opposed to sky. Everything matched
    # by --exclude-name is water by definition; among the textures only `sea` is.
    ap.add_argument("--settings",
                    default="/mnt/c/tools/rccars_extracted/Settings",
                    help="where WLOD_<track>_more1 lives -- the water surface's "
                         "own vertical offset comes out of it, and it must be "
                         "the same directory gen_vis_data.py reads")
    ap.add_argument("--water-tex", default=r"^sea$",
                    help="textures whose faces define the water surface height")
    # PAINT IS NOT A FLOOR. The checkpoint ground markings are decals draped over
    # the terrain a centimetre above it, and with them in the grid the car drives
    # on the PAINT: measured on the shipped beach_1.col, 10 of the 4 markers' 30
    # vertices have the marker itself as the collision surface, and repacking
    # without them puts the ground 10.4 mm lower (worst 20.2). That lip at the
    # edge of a painted line is what "checkpoints on ground have small height
    # collision" is. The engine cannot have had it -- the markers carry the
    # artists' own `NC`, no collision.
    #
    # Keyed on ACP, the exporter tag CLAUDE.md reads as "a checkpoint <n>", and
    # TWO independent signals agree on the family: every ACP mesh in all ten
    # tracks is icon-textured and DRAPED (y-span 0.000-0.545 m), while every
    # icon-textured mesh that is NOT tagged ACP is a standing signboard (arrows,
    # icon_pacific, icon_shc_1: y-span 1.234-4.212 m). Better than 2x clear, so
    # the tag and the shape cannot disagree here. The standing signs are left
    # alone deliberately -- they are a different object, and some are painted on
    # walls that need their own collision.
    #
    # This is DELIBERATELY NOT --exclude-name, which means water: everything it
    # matches is fed to the water-height pass, so putting the markers there would
    # make each of them a puddle. And it is deliberately not mirrored in
    # pack_vsc.py -- unlike the water meshes, these must still be DRAWN.
    ap.add_argument("--no-collide", default=r"(^|_)ACP\d*$",
                    help="drop meshes with these names from collision entirely "
                         "(painted ground markings -- not water, not sky)")
    a = ap.parse_args()

    b = open(a.src, "rb").read()
    reg, start = sb2obj.read_registry(b)
    meshes, textures = sb2obj.collect(b, reg, start)
    # texture NAME -> the engine's own surface class, off its 0x3408 record
    eng_of = {t["name"]: eng_surface_class(t.get("words"))
              for t in textures.values()}

    sky = re.compile(a.sky, re.I) if a.sky else None
    skip_tex = re.compile(a.exclude_tex) if a.exclude_tex else None
    skip_name = re.compile(a.exclude_name, re.I) if a.exclude_name else None
    no_coll = re.compile(a.no_collide, re.I) if a.no_collide else None
    water_tex = re.compile(a.water_tex) if a.water_tex else None
    tris = []
    surf = []                             # parallel to tris; None = unmatched
    esurf = []                            # parallel to tris; the ENGINE's class
    wtris = []                            # the water surfaces, for their height
    wsea = []                             # ... and whether each is a `sea` face
    dropped_tex = dropped_name = dropped_vertical = dropped_flat = 0
    dropped_paint = 0
    paint_span = 0.0
    paint_faces = []
    # the level's own XZ extent, for the underlay test below
    _ax = [v[0] for m in meshes for v in m[1]]
    _az = [v[2] for m in meshes for v in m[1]]
    lvl_dx = (max(_ax) - min(_ax)) if _ax else 1.0
    lvl_dz = (max(_az) - min(_az)) if _az else 1.0
    for nm, vs, uv, faces in meshes:
        if sky and any(t and sky.search(t) for t in {f[2] for f in faces}):
            continue                      # the sky dome, by its TEXTURE
        # ...and the artists' MAP-COVERING UNDERLAY PLANES, by their shape.
        #
        # The old mesh-name rule bundled two intentions into one regex, and the
        # comment said so -- "skydome / map-covering planes". The dome is handled
        # above; this is the other half, and it has to stay: beach_1 carries
        # `NOSHDW_LM0_polySurface2655`, 52 triangles of `rubbish_sand_wet` at a
        # single y = -1.38 spanning 92 x 224 m, i.e. a backdrop fill under the
        # whole level. Let it into the grid and it becomes a floor a metre below
        # the river, which is what wetcheck's fording run then found -- one wheel
        # resting 1.0 m under the surface instead of on the bed.
        #
        # Keyed on the SHAPE, which is what actually makes it an underlay: dead
        # flat, and wider than half the level. That is a property of the thing,
        # unlike `^NOSHDW_`, which is the artists' "casts no shadow" tag and says
        # nothing whatever about collision.
        if len(vs) > 2:
            ys = [v[1] for v in vs]
            if max(ys) - min(ys) < 0.001:
                xs = [v[0] for v in vs]
                zs = [v[2] for v in vs]
                if (max(xs) - min(xs)) > 0.5 * lvl_dx \
                        and (max(zs) - min(zs)) > 0.5 * lvl_dz:
                    dropped_flat += 1
                    continue
        # Paint: no collision and no water either, so it is tested before both.
        # The largest y-span dropped is printed rather than asserted -- it is the
        # one number that would show this rule reaching a standing signboard
        # instead of a ground marking, and the two are 2x apart in the shipped
        # tracks (see --no-collide).
        # Every painted GROUND marking, found by a signal the --no-collide rule
        # does not use: an `icon_*` texture on a mesh that is DRAPED rather than
        # standing. The rule drops meshes by the artists' ACP tag; this list is
        # what the finished grid is checked against below, so a marking the tag
        # misses is still caught. The 1 m cut sits between the two families --
        # drapes reach 0.545 m, the standing signboards start at 1.234 m.
        if vs:
            yy = [v[1] for v in vs]
            if max(yy) - min(yy) < 1.0:
                for vidx, _u, tname, *_rest in faces:
                    if tname and PAINT_TEX.search(tname) and max(vidx) < len(vs):
                        paint_faces.append((vs[vidx[0]], vs[vidx[1]],
                                            vs[vidx[2]]))
        if no_coll and nm and no_coll.search(nm):
            dropped_paint += len(faces)
            if vs:
                yy = [v[1] for v in vs]
                paint_span = max(paint_span, max(yy) - min(yy))
            continue
        is_water_mesh = bool(skip_name and nm and skip_name.search(nm))
        # faces are the NINE-element tuple sb2obj.collect builds -- base layer,
        # lightmap layer, then the second base and the alpha mask that blends the
        # two (see TAG_BASE2_UV there). The base texture is still element 2, and
        # a transition face's SECOND texture is deliberately ignored here: the
        # engine's surface class already comes from COL4 and the blend is a
        # rendering fact, not a driving one.
        for (vidx, _uvidx, tname, _lmidx, _lmname,
             _uv2, _tname2, _aidx, _aname) in faces:
            if max(vidx) >= len(vs):
                continue
            tri = (vs[vidx[0]], vs[vidx[1]], vs[vidx[2]])
            # A water surface is dropped from collision and kept for its height.
            # The two tests are separate on purpose: the ocean is identified by
            # its `sea` TEXTURE and the stream, coast and waterfall by their MESH
            # NAMES, and --exclude-tex also carries the sky, which is not water.
            if is_water_mesh or (water_tex and tname and water_tex.search(tname)):
                if is_water_mesh:
                    dropped_name += 1
                else:
                    dropped_tex += 1
                # Upward-facing only: `wtr_fall` is a vertical sheet and would
                # otherwise flood a wide band of dry ground beside it. The count of
                # what this rejects is printed, because no assertion on the finished
                # grid can separate a flooded cell from a genuinely deep one -- the
                # real sea shelf runs to 6 m under the surface on beach_3.
                if tri_up_area(tri) > 0.0:
                    wtris.append(tri)
                    wsea.append(not is_water_mesh)
                else:
                    dropped_vertical += 1
                continue
            if skip_tex and tname and skip_tex.search(tname):
                dropped_tex += 1
                continue
            tris.append(tri)
            surf.append(classify(tname))
            esurf.append(eng_of.get(tname, 0))

    if not tris:
        raise SystemExit("no collision triangles")

    # The track's default material: whichever class covers the most FLOOR area.
    # Weighting by total area instead picks the backdrop -- beach_1's mountains
    # are a bigger surface than beach_1's beach.
    floor = collections.Counter()
    for t, s in zip(tris, surf):
        if s is not None:
            floor[s] += tri_up_area(t)
    default_surf = floor.most_common(1)[0][0] if floor else 0
    n_unmatched = sum(1 for s in surf if s is None)

    xs = [v[0] for t in tris for v in t]
    zs = [v[2] for t in tris for v in t]
    min_x, max_x = min(xs), max(xs)
    min_z, max_z = min(zs), max(zs)
    cell = a.cell
    nx = max(1, int((max_x - min_x) / cell) + 1)
    nz = max(1, int((max_z - min_z) / cell) + 1)

    # register each triangle in every cell its XZ bounding box touches
    buckets = [[] for _ in range(nx * nz)]
    for i, t in enumerate(tris):
        tx0 = min(v[0] for v in t); tx1 = max(v[0] for v in t)
        tz0 = min(v[2] for v in t); tz1 = max(v[2] for v in t)
        cx0 = int((tx0 - min_x) / cell); cx1 = int((tx1 - min_x) / cell)
        cz0 = int((tz0 - min_z) / cell); cz1 = int((tz1 - min_z) / cell)
        for cz in range(max(0, cz0), min(nz - 1, cz1) + 1):
            row = cz * nx
            for cx in range(max(0, cx0), min(nx - 1, cx1) + 1):
                buckets[row + cx].append(i)

    starts, flat = [], []
    for bkt in buckets:
        starts.append(len(flat))
        flat.extend(bkt)
    starts.append(len(flat))

    # The water surface height, per cell: the surface itself, sampled at the
    # cell's centre, from whichever upward-facing water triangle covers it.
    # The box is only the search window -- the centre has to be INSIDE the
    # triangle, or the sea floods the beach behind it. See WATER at the top.
    #
    # The `sea` faces are stored at their DRAWN height. Read the track's own
    # WaterLOD section for it -- the same Section gen_vis_data.py builds
    # vis_data.h's WSURF[] from, so the packer and the renderer cannot disagree
    # about where the waterline is. A track with no section of its own falls
    # back to WaterLOD, and every track that does has no `sea` face at all.
    lvl = os.path.splitext(os.path.basename(a.src))[0]
    wcfg = wsurf_section(a.settings, lvl)
    # The conversions come from gen_vis_data.wsurf_values, not from three lines
    # retyped here -- two of these three are not raw*0.01 and the packer and the
    # header have to agree to the millimetre or the waterline splits in two.
    wval = wsurf_values(wcfg)
    w_offset = wval["offset"]
    w_magoff = wval["magnet_offset"]
    w_magrad = wval["magnet_radius"]

    def ground_at(px, pz, cx, cz):
        """Highest kept collision surface over (px,pz), or None. Same
        barycentric test as the water pass; used only for the swell's depth."""
        best = None
        for k in range(starts[cz * nx + cx], starts[cz * nx + cx + 1]):
            t = tris[flat[k]]
            d = ((t[1][2] - t[2][2]) * (t[0][0] - t[2][0])
                 + (t[2][0] - t[1][0]) * (t[0][2] - t[2][2]))
            if abs(d) < 1e-12:
                continue
            wa = ((t[1][2] - t[2][2]) * (px - t[2][0])
                  + (t[2][0] - t[1][0]) * (pz - t[2][2])) / d
            wb = ((t[2][2] - t[0][2]) * (px - t[2][0])
                  + (t[0][0] - t[2][0]) * (pz - t[2][2])) / d
            wc = 1.0 - wa - wb
            if wa < 0.0 or wb < 0.0 or wc < 0.0:
                continue
            ty = wa * t[0][1] + wb * t[1][1] + wc * t[2][1]
            if best is None or ty > best:
                best = ty
        return best

    water = [NO_WATER] * (nx * nz)
    n_over = 0                            # cells the old box rule would have added
    n_sea = 0                             # cells whose height came off a `sea` face
    for ti, t in enumerate(wtris):
        (x0, y0, z0), (x1, y1, z1), (x2, y2, z2) = t
        tx0 = min(x0, x1, x2); tx1 = max(x0, x1, x2)
        tz0 = min(z0, z1, z2); tz1 = max(z0, z1, z2)
        det = (z1 - z2) * (x0 - x2) + (x2 - x1) * (z0 - z2)
        if abs(det) < 1e-12:              # degenerate in plan: no footprint
            continue
        cx0 = int((tx0 - min_x) / cell); cx1 = int((tx1 - min_x) / cell)
        cz0 = int((tz0 - min_z) / cell); cz1 = int((tz1 - min_z) / cell)
        for cz in range(max(0, cz0), min(nz - 1, cz1) + 1):
            row = cz * nx
            pz = min_z + (cz + 0.5) * cell
            for cx in range(max(0, cx0), min(nx - 1, cx1) + 1):
                px = min_x + (cx + 0.5) * cell
                # barycentric in the XZ plane; they double as the interpolation
                # weights for the surface height, so the plane comes out free
                wa = ((z1 - z2) * (px - x2) + (x2 - x1) * (pz - z2)) / det
                wb = ((z2 - z0) * (px - x2) + (x0 - x2) * (pz - z2)) / det
                wc = 1.0 - wa - wb
                if wa < 0.0 or wb < 0.0 or wc < 0.0:
                    n_over += 1           # inside the box, outside the water
                    continue
                ty = wa * y0 + wb * y1 + wc * y2
                if wsea[ti]:
                    # Where FUN_005240c0 actually puts this vertex. damp is the
                    # shallow-water ramp, clamp01(depth / magnetRadius), taken
                    # against the REST height exactly as the engine's own
                    # per-vertex precompute is: the surface has to settle to one
                    # height, not chase itself. magnetOffset is 0.00 on every
                    # shipped track, so in practice this is a flat `offset` --
                    # the term is here because the config can set it and because
                    # dropping it would leave the packer and water.c spelling
                    # the same formula two different ways.
                    g = ground_at(px, pz, cx, cz)
                    dmp = 1.0
                    if g is not None and w_magrad > 1e-6:
                        dmp = min(max((ty - g) / w_magrad, 0.0), 1.0)
                    ty += w_offset + (1.0 - dmp) * w_magoff
                    n_sea += 1
                if ty > water[row + cx]:
                    water[row + cx] = ty

    with open(a.out, "wb") as f:
        f.write(b"COL4")
        f.write(struct.pack("<fffII", min_x, min_z, cell, nx, nz))
        f.write(struct.pack("<I", len(tris)))
        f.write(struct.pack("<I", default_surf))
        for t in tris:
            f.write(struct.pack("<9f", *t[0], *t[1], *t[2]))
        f.write(struct.pack(f"<{len(starts)}I", *starts))
        f.write(struct.pack(f"<{len(flat)}I", *flat))
        f.write(bytes(default_surf if s is None else s for s in surf))
        f.write(struct.pack(f"<{len(water)}f", *water))
        # COL4. Not folded into `surf`: that byte is the audio material and this
        # one is the engine's own class, and the two disagree on most textures
        # (26% agreement over the 319 in these tracks). Two bytes a triangle is
        # 40-50 KB a track.
        f.write(bytes(esurf))

    occupied = sum(1 for x in buckets if x)
    n_wet = sum(1 for w in water if w != NO_WATER)
    print(f"{os.path.basename(a.src)} -> {os.path.basename(a.out)}")
    print(f"  dropped {dropped_tex:,} faces by texture ({a.exclude_tex})")
    print(f"  dropped {dropped_name:,} faces by mesh name ({a.exclude_name})")
    print(f"  dropped {dropped_flat} map-covering underlay plane(s) by shape")
    print(f"  dropped {dropped_paint:,} painted-marking faces ({a.no_collide}), "
          f"deepest one draped over {paint_span:.3f} m of relief "
          f"-- over ~1 m would mean this reached a standing sign")
    print(f"  tris {len(tris):,}   grid {nx} x {nz} @ {cell}  ({occupied:,} occupied)")
    print(f"  refs {len(flat):,}  avg {len(flat)/max(1,occupied):.1f} tris/occupied cell")
    print(f"  bounds x[{min_x:.1f},{max_x:.1f}] z[{min_z:.1f},{max_z:.1f}]")
    mix = collections.Counter(SURF_NAMES[s] for s in surf if s is not None)
    print(f"  surfaces default={SURF_NAMES[default_surf]}  "
          f"unmatched {n_unmatched:,} ({100.0*n_unmatched/len(tris):.1f}% -> default)")
    print("    " + "  ".join(f"{k} {v:,}" for k, v in mix.most_common()))
    emix = collections.Counter(ENG_SURF_NAMES[e] for e in esurf)
    print(f"  engine   {sum(1 for e in esurf if e):,} of {len(esurf):,} tris classified"
          f" ({100.0*sum(1 for e in esurf if e)/len(esurf):.1f}%)")
    print("    " + "  ".join(f"{k} {v:,}" for k, v in emix.most_common()))
    if n_wet:
        wy = [w for w in water if w != NO_WATER]
        print(f"  water {len(wtris):,} upward faces -> {n_wet:,} cells "
              f"({100.0*n_wet/(nx*nz):.1f}% of grid), "
              f"y[{min(wy):.2f},{max(wy):.2f}]"
              f"  ({dropped_vertical:,} vertical faces rejected)")
        # Printed for the same reason as the vertical-face count: no assertion on
        # the finished grid can tell a wrongly flooded cell from a deep one, so
        # the number of cell centres that fell inside a triangle's BOX and
        # outside the triangle is the visible measure of what the old rule got
        # wrong. It should be large -- the sea's triangles are tens of metres.
        print(f"  water {n_over:,} cell samples inside a face's box but outside "
              f"the face -- dry land the old bbox rule flooded")
        print(f"  water section {wcfg.name}: offset {w_offset:+.3f} m, magnet "
              f"{w_magoff:+.3f} m over {w_magrad:.2f} m of depth, applied to "
              f"{n_sea:,} `sea` cell samples")
    else:
        print(f"  water none ({len(wtris):,} upward faces, "
              f"{dropped_vertical:,} vertical rejected)")
    # ---- paint is not a floor, checked against the grid that was just built --
    #
    # Two ways this can go wrong and both are checked, because they fail in
    # opposite directions: a marking still IN the grid is the lip that was
    # reported, and a marking whose removal took the terrain with it is a hole in
    # the world, which is far worse.
    #
    # How independent each half is differs by family, and it is worth being exact
    # about it. For the ICONS the candidates come from the texture and the drape
    # shape while the drop comes from the ACP tag, so a marking --no-collide
    # fails to match is genuinely caught. For the STRIPS both use `^wt_`, so the
    # first counter can only confirm its own rule -- what still means something
    # there is the second, which asks whether the terrain the strip modulates
    # actually exists underneath, and that is the property removal could break.
    #
    # The first counter also cannot tell a surviving decal from TERRAIN authored
    # coplanar with one; beach_4 has one such face. Both drops are exhaustive
    # over their family by construction (a regex over every face's texture, a
    # regex over every mesh's name), so a coincident survivor is the terrain.
    lip = hole = 0
    for tri in paint_faces:
        px = sum(v[0] for v in tri) / 3.0
        pz = sum(v[2] for v in tri) / 3.0
        py = sum(v[1] for v in tri) / 3.0
        cx = int((px - min_x) / cell)
        cz = int((pz - min_z) / cell)
        if not (0 <= cx < nx and 0 <= cz < nz):
            continue
        c0 = starts[cz * nx + cx]
        c1 = starts[cz * nx + cx + 1]
        best = None
        for k in range(c0, c1):
            t = tris[flat[k]]
            # barycentric in XZ, the same test the water pass uses
            d = ((t[1][2] - t[2][2]) * (t[0][0] - t[2][0])
                 + (t[2][0] - t[1][0]) * (t[0][2] - t[2][2]))
            if abs(d) < 1e-12:
                continue
            wa = ((t[1][2] - t[2][2]) * (px - t[2][0])
                  + (t[2][0] - t[1][0]) * (pz - t[2][2])) / d
            wb = ((t[2][2] - t[0][2]) * (px - t[2][0])
                  + (t[0][0] - t[2][0]) * (pz - t[2][2])) / d
            wc = 1.0 - wa - wb
            if wa < 0.0 or wb < 0.0 or wc < 0.0:
                continue
            ty = wa * t[0][1] + wb * t[1][1] + wc * t[2][1]
            # The NEAREST kept surface in either direction, not the nearest
            # below. A strip is authored 5-21 mm above the terrain on average but
            # individual faces dip under it, and "nothing below me" then reads as
            # a hole when the ground is a few millimetres over the centroid --
            # which is how the first version of this check cried wolf 34 times on
            # country_4.
            if best is None or abs(ty - py) < abs(best - py):
                best = ty
        if best is None or abs(best - py) > 0.5:
            hole += 1
        elif abs(best - py) < 0.0005:
            lip += 1
    if paint_faces:
        print(f"  paint {len(paint_faces)} ground-marking faces: {lip} still "
              f"have a surface at their own height, {hole} have no terrain "
              f"within 0.5 m (a hole)")
    print(f"  file {os.path.getsize(a.out)/1048576:.1f} MB")


if __name__ == "__main__":
    main()
