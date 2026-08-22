#!/usr/bin/env python3
"""
Inspect a packed .vsc and check the invariants the Vita app's visual subsystems
depend on. The app itself cannot be run here, so this is where "the data is
right" gets established: the surface flags, the markers, the textures drawn from
code, and the baked shadow silhouette.

It also reproduces, in Python, the two pieces of runtime geometry that are easy
to get wrong and impossible to eyeball from source: the checkpoint centroids
(FUN_0052abc0 averages the polyline) and the shadow's UV mapping (which has to
land the car's own footprint inside the texture).

Usage: python3 vsc_check.py <file.vsc> [more.vsc ...]
"""

import math
import os
import re
import struct
import sys

BATCH_SKY, BATCH_WATER, BATCH_COAST, BATCH_STREAM, BATCH_FALL = 1, 2, 4, 8, 16
BATCH_MODULATE = 32
BATCH_TRANSP = 64
BATCH_POOL = 128
BATCH_BLEND = 256
KINDS = [("sky", BATCH_SKY), ("water", BATCH_WATER), ("coast", BATCH_COAST),
         ("stream", BATCH_STREAM), ("fall", BATCH_FALL),
         ("pool", BATCH_POOL),
         ("modulate", BATCH_MODULATE), ("transp", BATCH_TRANSP),
         ("blend", BATCH_BLEND)]
ANY_WATER = BATCH_WATER | BATCH_COAST | BATCH_STREAM | BATCH_FALL | BATCH_POOL

# The textures the engine's WaterLOD module owns. NOTHING may carry one of these
# and not be animated -- that is the invariant below, and it is a property of the
# shipped art rather than a restatement of pack_vsc.py's name rule, which is the
# whole point (the `pCylinder` lesson: a flag check that asks whether the flag
# matches the rule that set it passes everything).
#
# Over the ten tracks 361 meshes carry one of the three and every single one is a
# water surface; nothing else references them. Shown to fail: with pack_vsc.py's
# old `^`-anchored needles it fires on exactly 12 meshes -- country_1's seven
# `pool<n>` ponds, beach_4's two, country_3's two `NOSHDW_LM0_wtr_fall<n>` and
# beach_2's `LM0_NOSHDW_coast1` -- which is the full set of what the anchors
# missed, and every one of them shipped as opaque ground.
WATER_TEX_RE = re.compile(r"^(sea|coast_base|stream)$", re.I)

# Where the SOURCE scenes are, for the one check that needs them (see
# sb_blend_faces). Overridden with --sb; unset, that check prints a note and the
# rest of the tool is unaffected.
SB_DIR = "/mnt/c/Games/RC Cars/RCCarsDB"

# what the app looks up by name and silently does without if missing
WANT_TRACK_TEX = ["water_wave", "cp_ar_2_f1", "cp_ar_2_f2", "cp_ar_2_f3",
                  "cp_ar_3_f1", "cp_ar_3_f2", "cp_ar_3_f3"]


class R:
    def __init__(self, b):
        self.b = b
        self.p = 0

    def u(self, n):
        v = struct.unpack_from("<%dI" % n, self.b, self.p)
        self.p += 4 * n
        return v[0] if n == 1 else v

    def f(self, n):
        v = struct.unpack_from("<%df" % n, self.b, self.p)
        self.p += 4 * n
        return v[0] if n == 1 else v

    def name(self):
        (n,) = struct.unpack_from("<H", self.b, self.p)
        self.p += 2
        s = self.b[self.p:self.p + n].decode("latin-1")
        self.p += n
        return s


def load(path):
    r = R(open(path, "rb").read())
    magic = r.b[:4].decode("latin-1")
    r.p = 4
    ver = int(magic[3])
    n_tex = r.u(1)
    n_bat = r.u(1)
    n_part = r.u(1) if ver >= 4 else 0
    n_mark = r.u(1) if ver >= 5 else 0
    shadow_radius = r.f(1) if ver >= 5 else 0.0
    # VSC8: the model name table. A track writes 0 here and no names; props.vsc
    # names its 13. The FIELD is unconditional from 8 on -- pack_vsc.py once
    # omitted it and the loader read the first texture's name length as the count.
    n_models = r.u(1) if ver >= 8 else 0
    models = [r.name() for _ in range(n_models)]

    tex = []
    for _ in range(n_tex):
        nm = r.name()
        w, h, fmt, mips = struct.unpack_from("<HHBB", r.b, r.p)
        r.p += 6
        bpp = 4 if fmt == 1 else 2
        off = r.p
        for lvl in range(mips):
            r.p += max(1, w >> lvl) * max(1, h >> lvl) * bpp
        tex.append({"name": nm, "w": w, "h": h, "fmt": fmt, "mips": mips,
                    "off": off})

    parts = []
    for _ in range(n_part):
        parts.append(r.name())
        r.p += 4 + 16 * 4 * 3

    markers = []
    for _ in range(n_mark):
        nm = r.name()
        x, y, z, yaw = r.f(4)
        markers.append((nm, x, y, z, yaw))

    vsz = 28 if ver >= 6 else 20
    batches = []
    for _ in range(n_bat):
        ti = r.u(1)
        fl = r.u(1)
        part = r.u(1) if ver >= 4 else 0
        lm = r.u(1) if ver >= 6 else 0xFFFFFFFF
        env = r.u(1) if ver >= 7 else 0
        model = r.u(1) if ver >= 8 else 0
        tex2 = mask = 0xFFFFFFFF
        if ver >= 9:
            tex2 = r.u(1)
            mask = r.u(1)
        nv = r.u(1)
        ni = r.u(1)
        vo = r.p
        r.p += nv * vsz
        # VSC7: normals follow the vertices, and ONLY on an env-mapped batch.
        # Reading them unconditionally puts the whole stream out of step, which
        # is what this tool exists to notice.
        no = r.p if env else None
        if env:
            r.p += nv * 12
        # VSC9: the blend's two extra UV sets, on the same rule -- after the
        # normals, before the indices, and only on a blend batch.
        bo = r.p if (fl & BATCH_BLEND) else None
        if fl & BATCH_BLEND:
            r.p += nv * 16
        r.p += ni * 2
        batches.append({"tex": ti, "flags": fl, "part": part, "lm": lm,
                        "env": env, "model": model, "tex2": tex2,
                        "mask": mask, "nv": nv, "ni": ni, "off": vo,
                        "nrm": no, "buv": bo})
    return dict(path=path, ver=ver, raw=r.b, tex=tex, markers=markers,
                models=models,
                batches=batches, shadow_radius=shadow_radius, parts=parts)


def verts(s, b):
    vsz = 28 if s["ver"] >= 6 else 20
    fmt = "<7f" if s["ver"] >= 6 else "<5f"
    return [struct.unpack_from(fmt, s["raw"], b["off"] + i * vsz)
            for i in range(b["nv"])]


# ---------------------------------------------------------------------------
# The independent count: how many faces the SOURCE SCENE asks to be blended.
#
# Everything else here is a property of the packed file, which cannot notice the
# packer declining to pack something -- and that is the one failure this feature
# has already had for the whole life of the port: sb2obj.py read the BAS1 and
# LM_1 layers and stopped, so 3,608 faces of authored transition arrived as hard
# edges and nothing was missing from the .vsc to say so.
#
# So this walks the .sb itself, with its own 20-line reader rather than sb2obj's,
# and counts the faces carrying BOTH a second base layer and an alpha layer. It
# must equal the packed blend triangle count exactly: one face is one triangle
# and none is dropped. `--no-blend` reports 3,608 against 0.
def sb_blend_faces(path):
    b = open(path, "rb").read()

    def chunk(p):
        cid, end = struct.unpack_from("<HI", b, p)
        n = p + 6
        if (cid & 0x400) or (cid & 0xF000) == 0x9000:
            n += 4
        return cid, end, n

    def kids(p, end):
        while p + 6 <= end:
            cid, cend, body = chunk(p)
            if cend <= p or cend > end:
                return
            yield cid, body, cend
            p = cend

    n = 0

    def walk(p, end, depth):
        nonlocal n
        for cid, body, cend in kids(p, end):
            if cid == 0x0617:                       # the face array
                for _fid, fb, fe in kids(body, cend):
                    tags = set()
                    for c3, b3, e3 in kids(fb + 4, fe):   # skip the u32 lead
                        if c3 != 0x063A:
                            continue
                        for _c4, b4, e4 in kids(b3, e3):  # 0x023B layers
                            for c5, b5, e5 in kids(b4, e4):
                                if c5 == 0x303D and e5 - b5 >= 4:
                                    tags.add(struct.unpack_from("<I", b, b5)[0])
                    # '2SAB' -> BAS2 and '1PLA' -> ALP1, byte-reversed as
                    # everywhere in .sb
                    if 0x42415332 in tags and 0x414C5031 in tags:
                        n += 1
            # Descend into CONTAINERS only. The type nibble says which: 0 and 9
            # nest, and 7/8 are float and blob payloads -- an embedded 512x512
            # lightmap image parsed as chunks walks off the end of the file,
            # which is exactly what it did the first time. sb2obj.walk_all draws
            # the same line.
            if depth < 24 and body < cend and (cid & 0xF000) in (0x0000, 0x9000):
                walk(body, cend, depth + 1)

    # skip the type registry: the first scene node starts the graph
    p = 0
    while p < len(b):
        cid, end, body = chunk(p)
        if end <= p or end > len(b):
            break
        if cid == 0x9200:
            break
        p = end
    walk(p, len(b), 0)
    return n


def blend_uvs(s, b):
    """(u2, v2, mu, mv) per vertex for a blend batch."""
    return [struct.unpack_from("<4f", s["raw"], b["buv"] + i * 16)
            for i in range(b["nv"])]


def check(path):
    s = load(path)
    base = os.path.basename(path)
    names = [t["name"] for t in s["tex"]]
    ok = True

    def bad(msg):
        nonlocal ok
        ok = False
        print("  FAIL %s" % msg)

    print("%s  VSC%d  %d tex  %d batches  %d markers"
          % (base, s["ver"], len(s["tex"]), len(s["batches"]), len(s["markers"])))

    is_car = "__shadow" in names

    # ---- surface flags --------------------------------------------------
    if not is_car:
        tally = {n: sum(1 for b in s["batches"] if b["flags"] & bit)
                 for n, bit in KINDS}
        tris = {n: sum(b["ni"] // 3 for b in s["batches"] if b["flags"] & bit)
                for n, bit in KINDS}
        print("  surfaces  " + "  ".join("%s %d (%d tris)" % (n, tally[n], tris[n])
                                         for n, _ in KINDS if tally[n]))
        if s["ver"] >= 5 and not tally["water"]:
            print("  note: no water surface tagged -- this level may have none")
        # ---- a water TEXTURE must be on a water surface -------------------
        for b in s["batches"]:
            if b["tex"] >= len(names):
                continue
            if WATER_TEX_RE.match(names[b["tex"]]) and not (b["flags"] & ANY_WATER):
                bad("batch textured `%s` carries no water flag -- it will draw "
                    "as opaque ground (%d tris)" % (names[b["tex"]], b["ni"] // 3))

        # Only the ANIMATOR kinds can conflict. `modulate` and `transp` are
        # render modes, not surfaces, and a coast band could legitimately be
        # tagged transparent by the artists.
        anim = [(n, bit) for n, bit in KINDS
                if bit not in (BATCH_MODULATE, BATCH_TRANSP)]
        for b in s["batches"]:
            kinds = [n for n, bit in anim if b["flags"] & bit]
            if len(kinds) > 1:
                bad("batch is both %s -- the animators would fight over it"
                    % " and ".join(kinds))

        # ---- the sky is ONE batch, and it is the size of the whole level --
        #
        # A BATCH_SKY batch is drawn camera-locked with depth writes off, so
        # anything wrongly flagged does not merely look odd -- it leaves the
        # place it belongs and follows the camera as a ghost. That is exactly
        # what shipped: pack_vsc.py used to pick the dome by the mesh name
        # `^pCylinder`, which is what Maya calls every cylinder primitive, so
        # beach_1's column shafts and beach_3's round walls near the start were
        # drawn as sky while four tracks got no sky batch at all.
        #
        # Neither half of that can be caught by asking whether the flag matches
        # the rule that set it. What separates a dome from a wall is SIZE: the
        # ten domes span 94.6%-100.0% of their own track's XZ diagonal, and the
        # widest thing the old rule mistook for one spanned 6.1% (beach_3's
        # 17.1 m round wall in a 292.8 m level). The bound below sits an order
        # of magnitude clear of both, and is measured off the packed geometry
        # rather than off any name.
        def xz_diag(vlist):
            if not vlist:
                return 0.0
            return math.hypot(max(v[0] for v in vlist) - min(v[0] for v in vlist),
                              max(v[2] for v in vlist) - min(v[2] for v in vlist))

        # ---- the transition decals ---------------------------------------
        #
        # pack_vsc.py decides BATCH_MODULATE from the PIXELS -- near-greyscale
        # with a neutral-128 border -- and never looks at the name. So the name
        # is an independent witness: every one of these ten tracks lays a
        # `wt_wetsand` strip along its sand/wet-sand boundary, and if that batch
        # is not flagged then the pixel rule has stopped recognising the one
        # thing it exists for. Asserting the pixel rule against itself would
        # pass on any change at all.
        for b in s["batches"]:
            nm = s["tex"][b["tex"]]["name"] if b["tex"] < len(s["tex"]) else ""
            if nm.lower().startswith("wt_") and not (b["flags"] & BATCH_MODULATE):
                bad("the '%s' batch is not flagged modulate -- it will draw as "
                    "an opaque grey band with two hard edges, which is the "
                    "sharp sand transition as reported" % nm)
            if (b["flags"] & BATCH_MODULATE) and b["lm"] != 0xFFFFFFFF:
                # it multiplies ground that is already lit; multiplying by the
                # atlas again darkens the strip twice
                bad("a modulate batch ('%s') carries a lightmap" % nm)

        # ---- the transition BANDS ----------------------------------------
        #
        # A band is two ground textures lerped by a mask, all three named by the
        # face's own UV layers (see THE SURFACE TRANSITION in pack_vsc.py). Three
        # things are checked, and none of them is the packer's rule restated.
        #
        # 1. THE PIXELS. A mask has to be a full ramp or there is nothing to
        #    fade: `alpha` runs 0 to 255 with 51.6% of its texels strictly
        #    between, against 0% for any ground texture in the game. The packer
        #    picks the mask by the LAYER TAG and never looks at a pixel, so this
        #    is an independent witness -- and it is the check that would catch the
        #    luminance never reaching the alpha channel, which is the one
        #    conversion between the .csi and the GPU.
        # 2. THE SHAPE. These bands are one quad wide with the mask's v running
        #    0 to 1 across them, so 98% of a band's vertices sit at one end of
        #    the ramp or the other. A batch whose mask v is all mid-range is a
        #    band that will read as a uniform half-mix -- the failure mode of
        #    mixing up which UV layer is which, which is exactly what the layers
        #    are easy to get wrong about.
        # 3. THE LIGHT. Every transition face in the ten tracks carries an LM_1
        #    layer, and the third pass needs it: an unlit band next to lit ground
        #    reads as a bright stripe.
        for b in s["batches"]:
            if not (b["flags"] & BATCH_BLEND):
                continue
            nm = names[b["tex"]] if b["tex"] < len(names) else "?"
            if b["tex2"] >= len(names) or b["mask"] >= len(names):
                bad("a blend batch ('%s') names no second texture or no mask "
                    "-- tex2 %#x mask %#x" % (nm, b["tex2"], b["mask"]))
                continue
            if b["tex2"] == b["tex"]:
                bad("a blend batch blends '%s' with itself" % nm)
            if b["lm"] == 0xFFFFFFFF:
                bad("the blend batch '%s'/'%s' carries no lightmap -- it will "
                    "read as a bright stripe across lit ground"
                    % (nm, names[b["tex2"]]))
            mt = s["tex"][b["mask"]]
            if mt["fmt"] != 1:
                bad("the mask '%s' is packed without an alpha channel (fmt %d) "
                    "-- the combiner reads a mask's ALPHA and cannot read its "
                    "RGB, so every band would draw fully opaque"
                    % (mt["name"], mt["fmt"]))
            else:
                n = mt["w"] * mt["h"]
                a = s["raw"][mt["off"] + 3:mt["off"] + n * 4:4]
                part = sum(1 for x in a if 8 < x < 248)
                if min(a) > 8 or max(a) < 247 or part * 100 < 20 * n:
                    bad("the mask '%s' is not a ramp: alpha %d..%d, %.1f%% "
                        "part-way (want 0..255 and >=20%%)"
                        % (mt["name"], min(a), max(a), 100.0 * part / n))
            # NEITHER base texture may be a cut-out. The blend pass replaces the
            # fragment's alpha with the mask and draws with the alpha test OFF,
            # so a base texture with rejectable texels would lose them. Measured
            # over the nine tracks that have bands: not one of their 20-odd base
            # textures has a single texel at or below the 0.5 threshold --
            # `fence` on urban_2 reads like a cut-out and is a solid wall map.
            for ti in (b["tex"], b["tex2"]):
                bt = s["tex"][ti]
                if bt["fmt"] != 1:
                    continue
                n = bt["w"] * bt["h"]
                al = s["raw"][bt["off"] + 3:bt["off"] + n * 4:4]
                rej = sum(1 for x in al if x <= 127)
                if rej:
                    bad("the blend base texture '%s' has %d cut-out texels "
                        "(%.1f%%) -- the blend pass replaces alpha with the mask "
                        "and runs with the alpha test off, so they would fill in"
                        % (bt["name"], rej, 100.0 * rej / n))
            if b["buv"] is None:
                bad("a blend batch carries no blend UVs")
                continue
            uv = blend_uvs(s, b)
            ends = sum(1 for (_u2, _v2, _mu, mv) in uv
                       if abs(mv) < 0.02 or abs(mv - 1.0) < 0.02)
            # HALF, not most. The typical band is one quad wide and lands 98% of
            # its vertices on one end of the ramp or the other, but beach_4 and
            # country_3 author some two quads deep and the weakest batch in the
            # ten tracks sits at 64.6%. What this has to separate is a band from
            # the layers being mixed up: the base UVs run to v = 17 and the
            # lightmap's are interior atlas coordinates, so either of those in
            # this slot puts almost nothing at an end.
            if ends * 100 < 50 * len(uv):
                bad("the '%s'/'%s' band's mask v is not a 0-to-1 ramp across it "
                    "-- only %d of %d vertices at an end of it"
                    % (nm, names[b["tex2"]], ends, len(uv)))
            # A mask is not a surface. Nothing may draw one as its base texture:
            # its RGB is packed white, so a batch that did would be a white hole.
            for o in s["batches"]:
                if o["tex"] == b["mask"]:
                    bad("'%s' is used as a base texture as well as a mask"
                        % mt["name"])
                    break
        # ---- and the count the SOURCE asks for ---------------------------
        if SB_DIR:
            sb = os.path.join(SB_DIR, os.path.splitext(base)[0] + ".sb")
            if os.path.exists(sb):
                want = sb_blend_faces(sb)
                got = sum(b["ni"] // 3 for b in s["batches"]
                          if b["flags"] & BATCH_BLEND)
                if want != got:
                    bad("the scene authors %d transition faces and the file "
                        "carries %d blend triangles -- every authored face is "
                        "one triangle and none is dropped" % (want, got))
                else:
                    print("  blend     %d authored transition faces, all packed"
                          % want)
            else:
                print("  note: %s not found -- the authored transition count is "
                      "unchecked" % os.path.basename(sb))

        # Every band is exclusively a band: the other bits are other draw paths
        # (water animates its vertices, modulate multiplies, transp blends by its
        # own alpha) and a batch in two of them is drawn twice.
        for b in s["batches"]:
            if (b["flags"] & BATCH_BLEND) and (b["flags"] & (ANY_WATER
                                                             | BATCH_MODULATE
                                                             | BATCH_TRANSP
                                                             | BATCH_SKY)):
                bad("a blend batch also carries flags %#x -- two draw paths "
                    "would each draw it" % (b["flags"] & ~BATCH_BLEND))

        # ---- the painted track markings ---------------------------------
        #
        # pack_vsc.py flags these off two ARTIST NAMES -- the mesh tag TRANSP<n>
        # and an `icon_*` texture -- so neither name can be the assertion. The
        # PIXELS can: a thing worth blending has a broad alpha ramp, and a
        # cut-out has a one-pixel antialiased border and nothing else. Measured
        # over these tracks, everything the artists tagged runs 17.2% to 52.5% of
        # texels strictly between transparent and opaque, and the true cut-outs
        # sit at 0.1% (trellis) to 1.3% (trees_2). The bound below is an order of
        # magnitude clear of both.
        #
        # The dangerous direction is a CUT-OUT being blended: its border stops
        # being rejected and the leaves grow a halo. So this only fires on a
        # flagged batch, and only when the texture has an alpha channel at all --
        # SAIL_G and SAW are tagged and carry none, which is harmless because
        # blending at a uniform alpha of 1 is what opaque already does.
        for b in s["batches"]:
            if not (b["flags"] & BATCH_TRANSP) or b["tex"] >= len(s["tex"]):
                continue
            t = s["tex"][b["tex"]]
            if t["fmt"] != 1:
                continue                      # no alpha channel packed
            px = s["raw"][t["off"]:t["off"] + t["w"] * t["h"] * 4]
            al = px[3::4]
            if not al:
                continue
            part = sum(1 for v in al if 8 < v < 248) * 100.0 / len(al)
            if part < 5.0:
                bad("'%s' is flagged transp but only %.1f%% of its alpha is "
                    "partial -- that is a cut-out, and blending it will halo "
                    "the edges instead of rejecting them" % (t["name"], part))

        sky = [b for b in s["batches"] if b["flags"] & BATCH_SKY]
        if len(sky) != 1:
            bad("%d batches flagged sky, expected exactly 1 -- 0 leaves the "
                "dome writing depth in the world and envmap_init with no "
                "texture for the car's glance, and >1 means ordinary geometry "
                "is being drawn camera-locked" % len(sky))
        track_diag = xz_diag([v for b in s["batches"] for v in verts(s, b)])
        for b in sky:
            frac = xz_diag(verts(s, b)) / track_diag if track_diag else 0.0
            print("  sky       tex '%s'  %d tris  spans %.1f%% of the track"
                  % (s["tex"][b["tex"]]["name"] if b["tex"] < len(s["tex"])
                     else "NONE", b["ni"] // 3, frac * 100))
            if frac < 0.5:
                bad("the sky batch spans only %.1f%% of the level -- a dome "
                    "spans ~95%%, so this is ordinary geometry about to be "
                    "drawn camera-locked and vanish from where it belongs"
                    % (frac * 100))

    # ---- markers --------------------------------------------------------
    if s["markers"]:
        cps = {}
        waves = []
        suns = []
        for nm, x, y, z, yaw in s["markers"]:
            if nm.startswith("water_wave_"):
                waves.append((nm, yaw))
            elif nm.upper() == "SUN_AF":
                suns.append((x, y, z))
            else:
                head = nm.split("_")
                k = int(head[1])
                cps.setdefault(k, []).append((nm, x, y, z))
        # cp_init walks 1, 2, 3, ... and stops at the first gap
        seq = sorted(cps)
        if seq != list(range(1, len(seq) + 1)):
            bad("checkpoint numbering is not contiguous: %s" % seq)
        for k in seq:
            pts = cps[k]
            names_k = sorted(p[0] for p in pts)
            # the base point must exist or cp_init drops the whole checkpoint
            if "cp_%d" % k not in names_k:
                bad("cp_%d has refining points but no base marker" % k)
            n = len(pts)
            if n > 33:
                bad("cp_%d has %d points, the loader's cap is 33" % (k, n))
            # the arrow is anchored on cp_N itself; the cp_N_M points only bend
            # the spine on the way to the next one
            base = next(p for p in pts if p[0] == "cp_%d" % k)
            span = max(math.dist(base[1:], p[1:]) for p in pts) if n > 1 else 0.0
            print("  cp_%-2d %2d pts  arrow at (%8.2f, %6.2f, %8.2f)  "
                  "spine reaches %.1f m"
                  % (k, n, base[1], base[2], base[3], span))
        for nm, yaw in waves:
            print("  %-16s yaw %7.2f deg -> heads (%.2f, %.2f)"
                  % (nm, yaw, math.sin(math.radians(yaw)),
                     math.cos(math.radians(yaw))))

        # ---- the sun ----
        #
        # AT MOST ONE, because FUN_00478fd0 takes the FIRST node modFindName
        # returns and a second would be silently ignored -- so two in a file is
        # an authoring or packing mistake, not a choice.
        #
        # ZERO is legal and is the answer for urban_1 and urban_2. What is NOT
        # legal is a marker with no textures behind it: sun.c disables itself if
        # `sun_disc` is missing, so the sun would go quietly absent exactly the
        # way it did before it was implemented. Bound to the FIVE the engine's own
        # FUN_00478f60 loads.
        if len(suns) > 1:
            bad("%d SUN_AF markers; FUN_00478fd0 takes the first and ignores "
                "the rest" % len(suns))
        if suns:
            x, y, z = suns[0]
            need = ["sun_disc", "lens_flare_1", "lens_flare_2", "lens_flare_3",
                    "shine"]
            have = {t["name"].lower() for t in s["tex"]}
            miss = [t for t in need if t not in have]
            if miss:
                bad("SUN_AF is packed but %s missing -- sun.c disables itself "
                    "without sun_disc, so the sun goes quietly absent"
                    % ", ".join(miss))
            # It has to be ABOVE the world and out of it: a sun at ground level
            # would put the disc inside the terrain and the flare permanently
            # blocked. The ten shipped markers run 8.0 to 84.2 m up.
            if y <= 1.0:
                bad("SUN_AF is at y %.2f -- at or below the ground, so the disc "
                    "is inside the terrain and the flare never clears" % y)
            print("  SUN_AF           at (%8.2f, %6.2f, %8.2f)  "
                  "elevation %.1f deg, %.1f m out"
                  % (x, y, z, math.degrees(math.atan2(y, math.hypot(x, z))),
                     math.hypot(x, z)))
        else:
            print("  SUN_AF           none -- urban_1 and urban_2, and that is "
                  "the original's own answer for them")

    # ---- textures the app looks up by name ------------------------------
    if is_car:
        t = next(t for t in s["tex"] if t["name"] == "__shadow")
        if t["fmt"] != 1:
            bad("__shadow is not RGBA; its alpha is the silhouette")
        px = s["raw"][t["off"]:t["off"] + t["w"] * t["h"] * 4]
        alpha = px[3::4]
        cover = sum(1 for a in alpha if a > 8) / float(len(alpha))
        # the car must be inside the texture, and must not fill it: a
        # silhouette touching the border means shadow-radius is too small and
        # CLAMP_TO_EDGE would smear it outward forever
        w, h = t["w"], t["h"]
        border = (list(alpha[:w]) + list(alpha[-w:])
                  + [alpha[y * w] for y in range(h)]
                  + [alpha[y * w + w - 1] for y in range(h)])
        print("  __shadow  %dx%d  radius %.3f m  %.1f%% covered  "
              "peak border alpha %d"
              % (w, h, s["shadow_radius"], cover * 100, max(border)))
        if s["shadow_radius"] <= 0.0:
            bad("__shadow is packed but the header radius is 0 -- shadow.c "
                "would fall back to ShadowSize and mis-scale the decal")
        if cover < 0.05:
            bad("__shadow is nearly empty -- the bake found no geometry")
        if cover > 0.75:
            bad("__shadow fills the texture -- shadow-radius is too small")
        if max(border) > 8:
            bad("__shadow touches its border -- raise shadow-radius")

        # ---- every exhaust names its own LEVEL ---------------------------
        #
        # carparts.c reads the booster level off the PART name, because the
        # texture suffix is a different numbering on two cars out of three --
        # the table is in pack_vsc.py's CAR_PARTS_EXTRA comment. That only
        # works if pack_vsc kept booster_<n> AND compressor_<n> addressable:
        # those two are SIBLINGS, not parent and child, so naming one and not
        # the other leaves a quarter of each exhaust on __root__, belonging to
        # no level and therefore drawn at all four.
        #
        # Asserted here rather than left to carparts_test because it is a
        # property of the FILE. Drop the names from the packer and every car
        # silently reverts to the texture rule that was wrong.
        lvl_of = {}
        for i, nm in enumerate(s["parts"]):
            for pre in ("booster_", "compressor_"):
                if nm.startswith(pre) and len(nm) == len(pre) + 1 \
                        and nm[-1].isdigit():
                    lvl_of[i] = int(nm[-1])
        per_level = {}
        for b in s["batches"]:
            if b["part"] in lvl_of:
                per_level.setdefault(lvl_of[b["part"]], []).append(b)
        orphan = [b for b in s["batches"]
                  if "turbo" in names[b["tex"]].lower()
                  and b["part"] not in lvl_of]
        print("  exhausts  %s"
              % ("  ".join("L%d:%dx/%dtri"
                           % (l, len(v), sum(x["ni"] // 3 for x in v))
                           for l, v in sorted(per_level.items()))
                 or "NONE NAMED"))
        if sorted(per_level) != [1, 2, 3, 4]:
            bad("the four exhaust levels are not all named by a part -- repack "
                "with a pack_vsc.py that lists booster_<n> in CAR_PARTS_EXTRA, "
                "or carparts.c falls back to the texture suffix, which is the "
                "wrong number on two cars out of three")
        for l, v in sorted(per_level.items()):
            if len(v) < 2:
                bad("exhaust level %d is a single batch -- compressor_%d is "
                    "missing from CAR_PARTS_EXTRA, so that half of the pipe "
                    "belongs to no level and draws at every one" % (l, l))
            if len({names[x["tex"]] for x in v}) != 1:
                bad("exhaust level %d spans more than one texture" % l)
        if orphan:
            bad("%d exhaust batches belong to no level" % len(orphan))

        # ---- all four SKINS are packed, on every page the car uses ---------
        #
        # The engine builds these names rather than tabling them: FUN_0049fc80
        # sprintf's "car_askin%i%i" / "car_bskin%i%i" from (car+1, skin+1) and
        # FUN_0050bf90 hands each to FUN_005352a0, which re-points every texture
        # whose name starts with the prefix. So the file has to CARRY the other
        # three of each -- the mesh only ever references skin 1 -- and this is
        # the assertion that catches a repack dropping them from --extra-tex.
        # carparts_test would catch it too, but only on a car that loads; this is
        # a property of the file and belongs where the file is read.
        #
        # HOW MANY PAGES IS NOT THE SAME ANSWER FOR ALL THREE CARS, and the digit
        # in the name says which car this is, so the rule is checkable here with
        # nothing passed in: car 1 has one page (FUN_0049fc80 writes NULL for the
        # second, and no car_bskin1<n> exists in any texture set), cars 2 and 3
        # have two. That also catches the other direction -- a car packed with
        # ANOTHER car's skins in its --extra-tex list, which would load, draw the
        # right paint at skin 1, and repaint into a different car at skin 2.
        skin_re = re.compile(r"^car_([ab])skin(\d)(\d)$")
        # Keyed by (page letter, CAR digit), not by the letter alone: a car packed
        # with another car's list would otherwise print its two cars' skins merged
        # into one page and read like a car that simply has gaps.
        pages = {}
        for t in s["tex"]:
            m = skin_re.match(t["name"])
            if m:
                pages.setdefault((m.group(1), int(m.group(2))),
                                 {})[int(m.group(3))] = t
        cars = {car for (_pg, car) in pages}
        worn = {names[b["tex"]] for b in s["batches"]
                if skin_re.match(names[b["tex"]])}
        print("  skins     %s   worn %s"
              % ("  ".join("car_%sskin%d:%s"
                           % (pg, car, ",".join(str(k) for k in sorted(v)))
                           for (pg, car), v in sorted(pages.items())) or "NONE",
                 ",".join(sorted(worn)) or "-"))
        if not pages:
            bad("no car_?skin texture at all -- the body has no paint")
        elif len(cars) != 1:
            bad("skins for more than one car in one file: %s -- check this "
                "car's --extra-tex list names its OWN skins" % sorted(cars))
        else:
            car_n = sorted(cars)[0]
            want = 1 if car_n == 1 else 2
            if len(pages) != want:
                bad("car %d has %d atlas page(s), the engine loads %d for it "
                    "(FUN_0049fc80)" % (car_n, len(pages), want))
            for (pg, _car), got in sorted(pages.items()):
                missing = [k for k in range(1, 5) if k not in got]
                if missing:
                    bad("car_%sskin%d is missing skin(s) %s -- add them to this "
                        "car's --extra-tex list, or the menu's Skin row pins "
                        "itself at 1/1"
                        % (pg, car_n, ",".join(str(k) for k in missing)))
                # carparts.c swaps these blind, so a page whose skins are not all
                # the same size would change the paint's resolution with the
                # selection. Nothing in the packer can currently do that -- one
                # --texset covers the file -- which is exactly why it is worth a
                # line here rather than a comment.
                sizes = {(t["w"], t["h"], t["fmt"]) for t in got.values()}
                if len(sizes) != 1:
                    bad("car_%sskin%d's skins are not all the same format/size: "
                        "%s" % (pg, car_n, sorted(sizes)))
            # The mesh must wear one skin per page, or scene_load leaves that
            # batch on a texture carparts.c never resolves and the paint the car
            # was authored with is unreachable.
            if len(worn) != len(pages):
                bad("the meshes reference %d skin texture(s) for %d page(s)"
                    % (len(worn), len(pages)))

        # ---- the exhaust puts SOLID AREA under the cut-out threshold ------
        #
        # The premise behind carparts.c marking these batches BATCH_ALPHA_LOWREF
        # and main.c drawing them in a second pass with the alpha-test reference
        # dropped to 0. Every <prefix>turbo_<n> is ARGB4444 whose alpha sits on a
        # plateau at nibble 7 -- 119 as a byte, 0.467 -- and the world's cut-out
        # test is glAlphaFunc(GL_GREATER, 0.5), which lands just above it. Drawn
        # with the rest of the body, every one of those texels is discarded and
        # the pipes and boosters have holes punched through them.
        #
        # NOTE WHAT THIS DOES AND DOES NOT SAY. It says those texels are being
        # discarded, which is a fact about the threshold. It does NOT say they
        # are translucent, and that reading is what made main.c blend the pass
        # and turn solid brass velocity stacks 47% see-through. The plateau
        # covers opaque metal -- see BATCH_ALPHA_LOWREF in scene.h for the
        # artwork and the upgr_boost* shop icons that settle it. The measurement
        # here is unchanged and still the right one; only what may be concluded
        # from it is narrower than it looked.
        #
        # A cut-out texture has partial alpha too, but only as an antialiased
        # BORDER, spread thinly over many values. What separates the exhaust is
        # that one intermediate value covers a solid area, so that is what this
        # measures -- not "has any partial alpha", which every foliage texture
        # in the game would also pass.
        # Per CAR, not per texture: the Buggy's turbo_2 really is close to a
        # plain cut-out (0.3% plateau), and the lower reference costs a cut-out
        # nothing -- alpha 255 stays opaque and alpha 0 is still discarded by
        # GREATER 0. What would make the whole second pass pointless is a car
        # where NONE of the four has area under the threshold.
        plateaus = []
        for t in s["tex"]:
            if "turbo" not in t["name"] or t["fmt"] != 1:
                continue
            n = t["w"] * t["h"]
            alpha = s["raw"][t["off"] + 3:t["off"] + 4 * n:4]
            hist = [0] * 256
            for a in alpha:
                hist[a] += 1
            mid = max(range(1, 255), key=lambda a: hist[a])
            frac = hist[mid] / float(n)
            plateaus.append((t["name"], mid, frac))
            print("  %-18s alpha plateau at %d (%.2f), %.1f%% of the image"
                  % (t["name"], mid, mid / 255.0, frac * 100))
        real = [p for p in plateaus if p[2] >= 0.01]
        if plateaus and not real:
            bad("no exhaust on this car has solid area under the 0.5 cut-out "
                "threshold -- if they are all plain cut-outs, BATCH_ALPHA_LOWREF "
                "and main.c's second car pass are buying nothing")
        for nm, mid, _ in real:
            if mid / 255.0 > 0.5:
                bad("%s's plateau is ABOVE the 0.5 alpha test, so it was never "
                    "being discarded and the holes in the exhaust are not the "
                    "bug they were thought to be" % nm)
    elif s["ver"] >= 5:
        missing = [n for n in WANT_TRACK_TEX if n not in names]
        if missing:
            bad("textures the app draws from code are absent: %s"
                % ", ".join(missing))
        else:
            print("  sprite textures  all %d present" % len(WANT_TRACK_TEX))

    # ---- water tiles: one subdivision per patch --------------------------
    wat = [b for b in s["batches"] if b["flags"] & BATCH_WATER]
    if wat:
        ys = [v[1] for b in wat for v in verts(s, b)]
        print("  water     %d batches, %d verts, y in [%.2f, %.2f]"
              % (len(wat), sum(b["nv"] for b in wat), min(ys), max(ys)))

    # ---- lightmaps, and the resolution gap the 128 px bug went through ----
    if not is_car and s["ver"] >= 6:
        lit = [b for b in s["batches"] if b["lm"] != 0xFFFFFFFF]
        atlases = {b["lm"] for b in lit}
        print("  lightmap  %d of %d batches lit, %d atlas(es): %s"
              % (len(lit), len(s["batches"]), len(atlases),
                 ", ".join(sorted(names[i] for i in atlases)) or "-"))
        if not lit:
            bad("no batch carries a lightmap -- the LM_1 UV layer was dropped")
        for b in lit:
            vs = verts(s, b)
            if all(v[5] == 0.0 and v[6] == 0.0 for v in vs):
                bad("a lit batch has all-zero lightmap UVs")
                break

    # ---- the env-map glance, and the normals it cannot work without -------
    #
    # Same shape of assertion as the lightmaps above, for the same reason: the
    # class and the normals are both packed by NAME MATCHING, so a rename in
    # Car.sb or a slip in ENVIR_RE/GRE1_RE leaves a car that loads and draws
    # perfectly and simply has no shine on it. The glance is also the one place a
    # zero normal is fatal rather than ugly -- it collapses the sphere-map UV to
    # the middle of the texture, which is a flat wash of one colour.
    if is_car and s["ver"] >= 7:
        env = [b for b in s["batches"] if b["env"]]
        by_class = {}
        for b in env:
            by_class[b["env"]] = by_class.get(b["env"], 0) + 1
        names_by_class = {1: "body", 2: "GRE1", 3: "GRE2", 4: "upgrades"}
        print("  envmap    %d of %d batches glance (%s)"
              % (len(env), len(s["batches"]),
                 ", ".join("%s %d" % (names_by_class.get(k, k), v)
                           for k, v in sorted(by_class.items())) or "-"))
        if not env:
            bad("no batch is env-mapped -- the ENVIR_CAR_BODY / _GRE<n> names "
                "did not match anything")
        if 1 not in by_class:
            bad("no ENVIR_CAR_BODY batch: the painted shell has no glance")
        for b in env:
            if b["nrm"] is None:
                bad("an env batch carries no normals")
                break
            ns = [struct.unpack_from("<3f", s["raw"], b["nrm"] + i * 12)
                  for i in range(b["nv"])]
            worst = min(math.sqrt(n[0] ** 2 + n[1] ** 2 + n[2] ** 2) for n in ns)
            if worst < 0.9:
                bad("an env batch has a normal of length %.3f -- not unit" % worst)
                break

    # The pack ships the same texture at 512, 256 and 128 px, and picking the
    # wrong set is invisible to every structural check -- it is what shipped for
    # weeks. Assert the big world textures are not the low-detail copies.
    big = [t for t in s["tex"] if not t["name"].startswith("__")]
    if big:
        worst = min(big, key=lambda t: t["w"] * t["h"])
        best = max(big, key=lambda t: t["w"] * t["h"])
        print("  textures  largest %dx%d (%s), smallest %dx%d (%s)"
              % (best["w"], best["h"], best["name"],
                 worst["w"], worst["h"], worst["name"]))
        if best["w"] * best["h"] < 512 * 512:
            bad("no texture reaches 512x512 -- this looks like Textures.2 or .3, "
                "not the high-detail set")

    # ---- RGB565 channel order -------------------------------------------
    #
    # The port shipped for weeks with blue in the high bits, on a misreading of
    # vitaGL: `glTexImage2D(GL_RGB, GL_RGB, GL_UNSIGNED_SHORT_5_6_5)` memcpy's
    # the pixels into GXM's U5U6U5_RGB, which the SDK also spells R5G6B5 -- red
    # high. On hardware that showed as blue sand and gold water. Vita3K reads it
    # the other way and so looked right, which is how it survived.
    #
    # Checked against the SOURCE .csi, not against a statistic over the pixels.
    # fix565.py first tried the latter -- mean(high field) > mean(low field), on
    # the theory that a texture set is warmer than it is cool -- and car1.vsc
    # defeated it, because the Overkill's paint is genuinely blue. A test whose
    # signal is the content it is controlling for cannot fail usefully.
    matched = order_from_source(s)
    if matched is None:
        print("  565 order  no texture matched a .csi source -- NOT CHECKED")
    else:
        which, name = matched
        print("  565 order  %s (matched '%s' against its .csi)" % (which, name))
        if which != "red-high":
            bad("RGB565 is packed %s. Red must be in the high bits -- see "
                "pack_vsc.csi_texture. Run fix565.py on this file." % which)

    print("  %s" % ("OK" if ok else "PROBLEMS ABOVE"))
    return ok


_CSI_INDEX = None


def order_from_source(s, csidir="/mnt/c/tools/rccars_extracted"):
    """('red-high'|'blue-high', texture name), or None if nothing could be
    matched. Compares a packed 565 level 0 with both packings of the decoded
    source, so it is exact and independent of what the image happens to look
    like. The generated textures ('__shadow') and the embedded lightmap atlases
    have no source, hence the loop."""
    global _CSI_INDEX
    try:
        import csi2png
        import pack_vsc
    except ImportError:
        return None
    if _CSI_INDEX is None:
        _CSI_INDEX = pack_vsc.index_csi(csidir)

    for t in s["tex"]:
        if t["fmt"] != 0:
            continue
        src = _CSI_INDEX.get(t["name"].lower())
        if not src:
            continue
        try:
            d = open(src, "rb").read()
            h = csi2png.parse_header(d)
            if h.width != t["w"] or h.height != t["h"]:
                continue
            off = h.data_off
            if off < csi2png.HDR_SIZE or off + t["w"] * t["h"] * h.bpp > len(d):
                off = csi2png.HDR_SIZE
            rgba = csi2png.decode_level(d, off, t["w"], t["h"], h)
        except Exception:
            continue

        n = t["w"] * t["h"]
        got = struct.unpack_from("<%dH" % n, s["raw"], t["off"])
        as_red = tuple(((rgba[i * 4] >> 3) << 11) | ((rgba[i * 4 + 1] >> 2) << 5)
                       | (rgba[i * 4 + 2] >> 3) for i in range(n))
        if got == as_red:
            return "red-high", t["name"]
        as_blue = tuple(((rgba[i * 4 + 2] >> 3) << 11)
                        | ((rgba[i * 4 + 1] >> 2) << 5) | (rgba[i * 4] >> 3)
                        for i in range(n))
        if got == as_blue:
            return "blue-high", t["name"]
    return None


if __name__ == "__main__":
    args = sys.argv[1:]
    if "--sb" in args:
        i = args.index("--sb")
        SB_DIR = args[i + 1]
        del args[i:i + 2]
    good = all(check(p) for p in args)
    sys.exit(0 if good else 1)
