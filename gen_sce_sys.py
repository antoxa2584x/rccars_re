#!/usr/bin/env python3
"""Build the Vita VPK's icon and LiveArea art from the game's own assets.

    GameIcon.ico  -> sce_sys/icon0.png                      128x128
    header.jpg    -> sce_sys/livearea/contents/bg.png       840x500
                  -> sce_sys/livearea/contents/startup.png  280x158

The sizes are the Vita's, not ours: icon0 is fixed at 128x128 and the LiveArea
background at 840x500, and a mis-sized image is not scaled by the shell, it is
rejected.  Everything is flattened onto black -- neither slot has an alpha
channel.

All three are written as **8-bit INDEXED** PNGs (colour type 3), not 24-bit
truecolour.  A truecolour LiveArea asset makes VitaShell's package promotion
fail with 0x8010113d, which is what the icon addition first shipped as.  256
colours is not much for a photographic background, so the palette is adaptive
per image and Floyd-Steinberg dithered; `verify()` re-reads the IHDR of each
file so a Pillow change that quietly writes something else cannot slip past.

Run from anywhere; paths are absolute.
"""

import struct

from PIL import Image, ImageFilter

SRC = "/mnt/c/Games/RC Cars"
DST = "/mnt/c/tools/rccars_vita/sce_sys"

ICON = 128
BG_W, BG_H = 840, 500
GATE_W, GATE_H = 280, 158

# How far the blurred filler behind the header is darkened, so the sharp copy
# in front of it reads as the subject rather than competing with its own blur.
FILL_DIM = 0.55
FILL_BLUR = 18


def cover(im, w, h):
    """Scale to fill w x h, cropping the overhang off both sides."""
    s = max(w / im.width, h / im.height)
    r = im.resize((round(im.width * s), round(im.height * s)), Image.LANCZOS)
    x, y = (r.width - w) // 2, (r.height - h) // 2
    return r.crop((x, y, x + w, y + h))


def contain(im, w, h):
    """Scale to fit inside w x h, keeping all of it."""
    s = min(w / im.width, h / im.height)
    return im.resize((round(im.width * s), round(im.height * s)), Image.LANCZOS)


def save8(im, path):
    """Write an RGB image as an 8-bit indexed PNG.

    `optimize` is deliberately NOT passed: with it Pillow is free to shrink the
    palette and drop the bit depth to 4 or 2, and the shell wants 8.  Padding
    the palette to a full 256 entries pins that even for a flat image like the
    icon, whose own colour count could otherwise fall under 16.
    """
    p = im.convert("RGB").convert(
        "P", palette=Image.ADAPTIVE, colors=256, dither=Image.FLOYDSTEINBERG
    )
    pal = p.getpalette() or []
    p.putpalette(pal + [0] * (768 - len(pal)))
    p.save(path, format="PNG", bits=8)


def verify(path, w, h):
    """Re-read the IHDR and assert size, bit depth 8 and colour type 3."""
    d = open(path, "rb").read()
    assert d[:8] == b"\x89PNG\r\n\x1a\n", path + ": not a PNG"
    assert d[12:16] == b"IHDR", path + ": IHDR is not the first chunk"
    iw, ih, depth, ctype = struct.unpack(">IIBB", d[16:26])
    assert (iw, ih) == (w, h), f"{path}: {iw}x{ih}, want {w}x{h}"
    assert depth == 8, f"{path}: bit depth {depth}, want 8"
    assert ctype == 3, f"{path}: colour type {ctype}, want 3 (indexed)"
    print(f"  {path.split('/')[-1]:14} {iw}x{ih} 8-bit indexed  {len(d):>7} bytes")


def check_vpk(path):
    """Assert the sce_sys art INSIDE a built .vpk is 8-bit indexed.

    The generator's own `verify()` only covers the files on disk, and the vpk
    is what VitaShell promotes -- a stale build directory would ship the old
    truecolour art with a freshly fixed source tree and look fixed.
    """
    import zipfile

    want = {
        "sce_sys/icon0.png": (ICON, ICON),
        "sce_sys/livearea/contents/bg.png": (BG_W, BG_H),
        "sce_sys/livearea/contents/startup.png": (GATE_W, GATE_H),
    }
    z = zipfile.ZipFile(path)
    have = set(z.namelist())
    for name, (w, h) in want.items():
        assert name in have, f"{path}: {name} is missing"
        d = z.read(name)
        assert d[:8] == b"\x89PNG\r\n\x1a\n", f"{name}: not a PNG"
        assert d[12:16] == b"IHDR", f"{name}: IHDR is not the first chunk"
        iw, ih, depth, ctype = struct.unpack(">IIBB", d[16:26])
        assert (iw, ih) == (w, h), f"{name}: {iw}x{ih}, want {w}x{h}"
        assert depth == 8, f"{name}: bit depth {depth}, want 8"
        assert ctype == 3, f"{name}: colour type {ctype}, want 3 (indexed)"
        print(f"  {name:42} {iw}x{ih} 8-bit indexed  {len(d):>7} bytes")
    assert "sce_sys/livearea/contents/template.xml" in have, "template.xml missing"
    assert "sce_sys/param.sfo" in have, "param.sfo missing"
    assert "eboot.bin" in have, "eboot.bin missing"
    print(path, "OK")


def main():
    # icon0 -- the .ico is a multi-frame file; ask for the largest frame and
    # downsample, rather than letting PIL hand back the 16x16 one.
    ico = Image.open(SRC + "/GameIcon.ico")
    ico.size = (256, 256)
    ico = ico.convert("RGBA")
    flat = Image.new("RGB", ico.size, (0, 0, 0))
    flat.paste(ico, (0, 0), ico)
    save8(flat.resize((ICON, ICON), Image.LANCZOS), DST + "/icon0.png")

    hdr = Image.open(SRC + "/header.jpg").convert("RGB")

    # bg -- header.jpg is 460x215, i.e. 2.14:1 against the LiveArea's 1.68:1.
    # A straight cover-crop would throw away a third of the width and upscale
    # 2.3x; a blurred cover behind a contained copy keeps the whole image.
    bg = cover(hdr, BG_W, BG_H).filter(ImageFilter.GaussianBlur(FILL_BLUR))
    bg = Image.eval(bg, lambda v: int(v * FILL_DIM))
    fg = contain(hdr, BG_W, BG_H)
    bg.paste(fg, ((BG_W - fg.width) // 2, (BG_H - fg.height) // 2))
    save8(bg, DST + "/livearea/contents/bg.png")

    # startup (the gate) -- 1.77:1, close enough to the header that a crop
    # costs only the far edges and keeps the logo.
    save8(cover(hdr, GATE_W, GATE_H), DST + "/livearea/contents/startup.png")

    print("wrote to", DST)
    verify(DST + "/icon0.png", ICON, ICON)
    verify(DST + "/livearea/contents/bg.png", BG_W, BG_H)
    verify(DST + "/livearea/contents/startup.png", GATE_W, GATE_H)


if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1 and sys.argv[1] == "--check-vpk":
        check_vpk(sys.argv[2])
    else:
        main()
