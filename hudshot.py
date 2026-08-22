#!/usr/bin/env python3
"""
hudshot.py -- composite hudshot's geometry dump over the game's real textures.

ui_test says where every HUD quad went. This says what it LOOKS like, which is
the only way three of this project's worst bugs were ever found. It rasterises
the dumped triangles with per-vertex UV over a stand-in "world" so the alpha and
the blend are visible, and writes a PNG at the Vita's own 960x544.

  gcc -I<vita> -I<vita>/testgl -O2 hudshot.c <vita>/ui.c <vita>/race_ui.c \
      -lm -o hudshot
  ./hudshot <track> | python3 hudshot.py out.png --track <track>
"""
import argparse
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import csi2png

TEXROOT = "/mnt/c/tools/rccars_extracted/Textures.1"
LANG = "/mnt/c/Games/RC Cars/Language/English"


def load(name, track):
    """One texture as an HxWx4 float array, or None for the untextured pass."""
    if name in ("-", "?"):
        return None
    if name == "font.h":
        return None                     # the compiled-in fallback; not on disk
    if name == "map":
        # the ENGINE's level number, not the port's track index -- see
        # gen_hud_data.engine_levels
        import gen_hud_data
        order, _d = gen_hud_data.engine_levels(
            "/mnt/c/tools/rccars_extracted/Scripts/championship.ini")
        names10 = ["beach_1", "beach_2", "beach_3", "beach_4", "country_1",
                   "country_2", "country_3", "country_4", "urban_1", "urban_2"]
        name = "trackmap_%d" % (order.index(names10[track]) + 1)
    for root in (TEXROOT, LANG):
        p = os.path.join(root, name + ".csi")
        if os.path.exists(p):
            d = open(p, "rb").read()
            h = csi2png.parse_header(d)
            px = csi2png.decode_level(d, h.data_off, h.width, h.height, h)
            a = np.frombuffer(bytes(px[:h.width * h.height * 4]),
                              dtype=np.uint8).astype(np.float32)
            return a.reshape(h.height, h.width, 4) / 255.0
    print("  (no art for %s)" % name, file=sys.stderr)
    return None


def raster(dst, tri, tex):
    """One triangle, barycentric, with per-vertex UV and colour."""
    H, W = dst.shape[:2]
    xs = [v[0] for v in tri]
    ys = [v[1] for v in tri]
    x0, x1 = max(0, int(min(xs))), min(W, int(max(xs)) + 2)
    y0, y1 = max(0, int(min(ys))), min(H, int(max(ys)) + 2)
    if x1 <= x0 or y1 <= y0:
        return
    ax, ay = tri[0][0], tri[0][1]
    bx, by = tri[1][0], tri[1][1]
    cx, cy = tri[2][0], tri[2][1]
    det = (bx - ax) * (cy - ay) - (cx - ax) * (by - ay)
    if abs(det) < 1e-9:
        return
    px, py = np.meshgrid(np.arange(x0, x1) + 0.5, np.arange(y0, y1) + 0.5)
    w1 = ((px - ax) * (cy - ay) - (cx - ax) * (py - ay)) / det
    w2 = ((bx - ax) * (py - ay) - (px - ax) * (by - ay)) / det
    w0 = 1.0 - w1 - w2
    m = (w0 >= -1e-4) & (w1 >= -1e-4) & (w2 >= -1e-4)
    if not m.any():
        return
    def lerp(i):
        return w0 * tri[0][i] + w1 * tri[1][i] + w2 * tri[2][i]
    src = np.stack([lerp(4), lerp(5), lerp(6)], axis=-1)
    a = lerp(7)
    if tex is not None:
        th, tw = tex.shape[:2]
        u = np.clip((lerp(2) * tw).astype(int), 0, tw - 1)
        v = np.clip((lerp(3) * th).astype(int), 0, th - 1)
        t = tex[v, u]
        src = src * t[..., :3]
        a = a * t[..., 3]
    a = np.clip(a, 0, 1)[..., None]
    sub = dst[y0:y1, x0:x1, :3]
    mm = m[..., None]
    dst[y0:y1, x0:x1, :3] = np.where(mm, sub * (1 - a) + src * a, sub)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--track", type=int, default=0)
    a = ap.parse_args()

    lines = sys.stdin.read().split("\n")
    W = H = 0
    draws = []
    i = 0
    while i < len(lines):
        f = lines[i].split()
        i += 1
        if not f:
            continue
        if f[0] == "screen":
            W, H = int(f[1]), int(f[2])
        elif f[0] == "draw":
            n = int(f[2])
            vs = []
            for k in range(n):
                g = [float(x) for x in lines[i + k].split()]
                vs.append(g)
            i += n
            draws.append((f[1], vs))

    # A stand-in world: a sandy gradient, which is what most of these tracks
    # are and the background the shadow rule was written against.
    img = np.zeros((H, W, 4), np.float32)
    yy = np.linspace(0, 1, H)[:, None]
    img[..., 0] = 0.62 - 0.18 * yy
    img[..., 1] = 0.55 - 0.20 * yy
    img[..., 2] = 0.40 - 0.16 * yy
    img[..., 3] = 1.0

    cache = {}
    for name, vs in draws:
        if name not in cache:
            cache[name] = load(name, a.track)
        tex = cache[name]
        for k in range(0, len(vs) - 2, 3):
            raster(img, vs[k:k + 3], tex)

    Image.fromarray((np.clip(img[..., :3], 0, 1) * 255).astype(np.uint8)).save(a.out)
    print("wrote %s  (%d draws)" % (a.out, len(draws)))


if __name__ == "__main__":
    main()
