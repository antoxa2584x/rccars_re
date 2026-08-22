# CREAT Studios "TIOX" pack format — RC Cars (engine SOURCE51)

**Status: SOLVED.** Container and payload cipher both fully recovered.
`unpack_tiox.py` extracts all 1,875 files from `RCCars.pack` cleanly.

Derived from the shipped reader/writer API in `CRSGame20.dll`:
`tioPackLoadRoot`, `tioPackFindFile`, `tioPackStoreFile`, `tiohbReadChunk`.

## Chunk header

    u16 id
    u32 end            absolute file offset, one past this chunk
    u32 count          ONLY if (id & 0x400) or (id & 0xF000) == 0x9000

The id's high nibble is a type tag:
`0x3xxx` = dword, `0x4xxx` = string, `0x0xxx`/`0x1xxx`/`0x5xxx` = container/blob.

## Layout

    0x5210  root                       end = end of directory (0x1F07A here)
      f32   version                    read directly, not a chunk (1.0)
      0x4011  string   comment         "CREAT Studio Pack 1.0"
      0x4014  string   basepath        "CREATSTUDIOGames"   <-- the cipher key
      0x3012  dword    string-pool size
      0x0613  file list (count-bearing) count = number of entries
        0x0220  entry (one per file)
          0x4021  string  name         backslash-delimited relative path
          0x3022  dword   v1 = payload size
          0x3023  dword   v2 = absolute offset of the blob's own chunk header
          0x3024  dword   v3 = flags; bit 0x20 = payload is enciphered
      0x0630  data region (count-bearing) directory end .. EOF

    Each stored file:
      0x1431  blob chunk (count-bearing, so 10-byte header)
              count = payload size, end = next blob's offset
              payload begins immediately after the 10-byte header

`tioPackFindFile` confirms this: it seeks to the entry offset, reads the chunk,
asserts `id == 0x1431` and `count == entry size`, then returns `tell()` as the
payload start.

## Payload cipher

`tioPackStoreFile` gates on `entry.flags & 0x20`. When set it calls the codec,
writes the buffer, then calls the codec *again* to restore the caller's memory —
so the transform is an involution. The codec is `FUN_10006BC0`:

    key  = root->basepath           ; "CREATSTUDIOGames", into a 32-byte NUL-padded buffer
    klen = strlen(key)              ; computed ONCE, up front (16)
    j    = 0
    for each byte:
        buf[i] ^= key[j]
        next = key[j+1]             ; lookahead BEFORE advancing
        j += 1
        if next == 0:               ; hit the terminator
            for t in 0..klen-1:
                key[t] = (key[t]*3 + 1) & 0xFF
            j = 0

The load-bearing subtlety: mutation can **introduce** NUL bytes mid-key.
`'U'` (0x55) maps to 0x00 on the first pass, so round 2 terminates after only 7
bytes instead of 16, and the effective round length keeps changing thereafter.
Miss that and you match the first 23 bytes and diverge forever after.

Round 1 is the literal string, so `keystream[0:16] == "CREATSTUDIOGames"`.

## Verification

Extracted 1,875/1,875 files, 493,061,058 bytes, 0 malformed, 0 encoded tails.
Structural check of every file with a known magic — **0 failures**:

| type  | count | check |
|-------|-------|-------|
| .csi  | 1316  | magic `MISC` (reversed `CSIM`) |
| .dat  |  163  | (proprietary, unchecked) |
| .ini  |  144  | fully printable text |
| .wav  |  118  | `RIFF`/`WAVE`; all 117 also identified by `file(1)` |
| .crs  |   64  | nested TIOX root chunk `0x5210` |
| .bspl |   30  | (proprietary, unchecked) |
| .spl  |   18  | fully printable text |
| .gpf  |   10  | (proprietary, unchecked) |
| .tga  |    9  | valid Targa; `file(1)` reports 128x256x32 RGBA |

## Notes for whoever picks this up

- Extraction output lives in `C:\tools\rccars_extracted\`.
- The engine's *other* cipher (`FUN_10001590`, CRS string tables, chunk cases
  `CSRC`/`CSRD`) is a different scheme — running key with an additive increment
  from `handle+0x48`. Don't confuse the two; it decodes `.tbl` localization data,
  not pack payloads.
- Dead ends worth not repeating: there is no bytewise-XOR decode loop on the
  *read* path at all. The tio handle block-readers (`+0x4C8`) are plain `fread`
  wrappers with optional byte-swapping for the `HF` big-endian primitives. The
  cipher is only visible in the **writer**, because the reader reuses the same
  involution from a different call site.

# .sb scene database — SOLVED

`.sb` reuses the **same TIOX chunk framing** as the pack. Chunk `end` fields are
ABSOLUTE file offsets at every nesting depth, including inside face records.
Tools: `sbdump.py` (structure), `sb2obj.py` (geometry export).

## File layout

    0x3801                  file marker
    0x4802  string          "CREAT Studio Scene Project 6.0"
    0x9A00 * 26             type registry. The chunk's COUNT field is the type
                            4CC, stored byte-reversed (HSEM->MESH, CSED->DESC,
                            RTAM->MATR, LLOC->COLL, NLPS->SPLN). Child 0x4803
                            string gives the name: MOD_MESH, MOD_LIGHT, ...
    0x9200 [4CC]            scene nodes, nested into a scene graph

26 node types: MESH, LIGHT, CAMERA, TEXTURE, MATERIAL, GLBTEXTURE, HIDING,
COLLISION, EMPTY, INSTANCE, MARKER, SEQUENCE, ANIMATION, SPLINE, DYNAMIC,
CONSTRAINT, CHARVOL, VOLUME, SCRIPT, MODEL, MAT_SHADER, FOLDER, SOUND,
SNDCHANNEL, DESCR, ALL.

`beach_1.sb` contains 627 MOD_MESH nodes (487 leaves), 78 GLBTEXTURE,
37 MARKER, 22 INSTANCE, 14 FOLDER, 8 VOLUME.

## MOD_MESH node

    0x4003  string          node name (e.g. "polySurface3693")
    0x7411 [n]  n * vec3    vertex positions
    0x7413 [n]  n * vec2    UVs -- per face-vertex, NOT per vertex
    0x7414 [2]  2 * vec3    bounding box (min, max)
    0x540B [9] / 0x5438 [9] 3x3 matrices
    0x0617 [n]              n face records

**Verified:** the bbox computed from the `0x7411` array reproduces the stored
`0x7414` bbox exactly. Type nibble 7 = float array, 5 = float matrix,
8 = raw blob, 3 = dword, 4 = string.

## Face record (0x8218)

A `u32` lead value, then chunks (this offset threw me at first -- parsing from
byte 0 yields garbage):

    0x3419 [3]  3 * u32     TRIANGLE VERTEX INDICES
    0x3020                  packed ARGB colour
    0x7027      vec3        face normal
    0x063A [n] -> 0x023B    n UV-set layers; each layer holds:
        0x303C              per-layer index/base value
        0x303D              layer 4CC (byte-reversed, as everywhere in .sb):
                              '1SAB' -> "BAS1"  base texture UV set
                              '1_ML' -> "LM_1"  lightmap UV set
        0x343F [3] 3 * u32  UV indices into 0x7413 for THAT layer

Quads arrive pre-split: `(3,0,1)` + `(3,1,2)` for a 0-1-2-3 quad.

### The trap that cost the most time

**`0x343F` is never a vertex index.** Every `0x343F` -- in every layer -- indexes
the UV array. Vertex indices live in `0x3419`, which looks like an ordinary
dword chunk but is count-bearing with three values.

This is easy to get wrong because on meshes where UV count happens to be close
to vertex count, `0x343F` produces perfectly plausible triangles: the first
sample mesh inspected had 48 verts and yielded `(3,0,1)`, `(3,1,2)` -- a textbook
quad split. It only fails on meshes where UVs outnumber vertices.

Decisive test across all 487 meshes in `beach_1.sb`:

| candidate      | meshes fully in range |
|----------------|-----------------------|
| `0x3419` triple| **487 / 487**         |
| first `0x343F` | 310 / 487             |

Two wrong theories, both disproved by measurement rather than argument:
"take only the first 0x343F" (34% -> 21% overrun, but never zero), and
"meshes carry multiple 0x7411 vertex chunks" (accumulating them changed the
output by exactly zero triangles).

## Materials and textures

Textures are declared by `MOD_GLBTEXTURE` nodes:

    0x4003  string   texture name -- matches the .csi basename in the pack
    0x3408  u32      texture index
    0x4034  string   original source path on the dev machine
                     ("//Venus/E/project/SmashCars/Beach/...")
    0x8232  blob     EMBEDDED CSI image; present instead of 0x4034 for
                     lightmaps, which have no source file. 786,592 bytes
                     = 512*512*3 + 160, i.e. a plain RGB888 CSI.

A face's UV layer (`0x023B`) binds to a texture through `0x303C`, which is the
texture index; `0x303D` says which set it is (`BAS1` base, `LM_1` lightmap).
So the chain is:

    face -> 0x023B layer -> 0x303C index -> MOD_GLBTEXTURE -> name -> .csi

### Texture indices are SCOPED, not global

Indices are scoped to the declaring subtree. Resolve through the enclosing
scope chain, innermost first. A flat global table looks correct on tracks and
silently collapses everywhere else:

| file        | GLBTEXTURE nodes | distinct indices |
|-------------|------------------|------------------|
| beach_1     | 78               | 78               |
| country_1   | 64               | 64               |
| **Car**     | **101**          | **9**            |
| **Interface**| **131**         | **1**            |
| **stone**   | **16**           | **2**            |

All 12 track files use globally unique indices, so a flat table passes every
test you would naturally run on a track -- and quietly maps 101 car textures
onto 9 materials.

### Coverage

777 / 782 texture references resolve to an extracted `.csi`. All 12 tracks are
100%. The 5 unresolved are `S_SKY` (Car), `axe` and `beton_stoyka` (Roadcar),
`cockpit` and `lights` (cockpit) -- these have no matching file in the pack and
are likely generated at runtime or supplied from elsewhere.

`sb2obj.py` writes a companion `.mtl`, groups faces by `usemtl` so a viewer gets
one draw call per texture, and converts embedded lightmaps out to
`embedded/*.png`. Lightmaps never enter the material table via faces (OBJ has no
second UV set) but are extracted anyway, since the image data is real.

## Verification

All 18 databases export with **0 out-of-range triangles**:

| file        | meshes | verts  | uvs     | tris   |
|-------------|--------|--------|---------|--------|
| beach_1     | 487    | 35,815 | 143,750 | 52,358 |
| beach_2..4  | 466-628| 21-29k | 122-175k| 32-44k |
| country_1..4| 412-1068| 26-30k| 156-180k| 40-45k |
| urban_1..2  | 886-890| 23-27k | 142-171k| 34-40k |
| Car         | 405    | 18,631 | 37,274  | 29,084 |
| people      | 11     | 3,512  | 14,144  | 6,708  |

`Interface.sb` and `RaceTextures.sb` contain no meshes, as expected -- they are
UI and texture registries.

A top-down rasterisation of `beach_1` renders as a recognisable beach level:
shoreline, terrain relief, ramps and platforms. Two meshes blanket the whole map
(`pCylinder6`, a skydome, and a `NOSHDW_` plane) and should be filtered when
previewing.

## MOD_COLLISION — the index layer is solved, the blob is not

Every track carries exactly one `MOD_COLLISION` node (4.5 MB on `beach_1`). It is
a **uniform 3D voxel grid over the render meshes**, not a separate collision
hull:

    0x3498 [3]   nx, ny, nz          156, 12, 320 on beach_1
    0x7099       vec3 bbox min       (-63.0, -3.1074, -100.0)
    0x709A       vec3 bbox max       ( 15.0,  7.9600,   60.0)
    0x309E       u32 total entries   128,289
    0x809D       u16[nx*ny*nz]       per-voxel ENTRY COUNT (sums to 0x309E)
    0x809C       u16[309E]           mesh index, in MOD_MESH traversal order
    0x349F       u32[309E]           that mesh's triangle count
    0x1500       byte[2,607,647]     not decoded

Cells are 0.5 m in X and Z (78/156, 160/320) and 0.9223 m in Y. `0x809D` is a
plain count array: walk the voxels and the running sum indexes `0x809C` and
`0x349F` together.

**`0x349F` is a checksum, and it is what proves the mapping.** Enumerate the
scene's `MOD_MESH` nodes in traversal order, then for each distinct `0x809C`
value check `meshes[value].n_triangles == 0x349F[value]`: **483 of 483 match** at
offset 0, versus 46 and 45 at ±1.

The useful conclusion for a port is a negative one: **the game collides against
its render geometry.** 483 of `beach_1`'s 627 mesh nodes appear, and the 144
absent ones are empty group nodes plus four meshes totalling 152 triangles — so
52,206 of 52,358 triangles are collidable. There is no separate coarser collision
mesh to recover, and no set of drive-through props to exclude. `pack_col.py`'s
derive-from-visible-geometry approach was the right call; it currently ships
37,085 triangles, the 15,051-triangle difference being the `sea` faces it drops
on purpose.

`0x1500` is still open. The `1<<14` / `1<<21` patterns in it look like per-voxel
per-mesh triangle bitmasks — which is what the grid needs to avoid testing a
2,012-triangle mesh whole in every voxel it touches. The readers are in the
engine's collision module: `0x00454970` (sphere) and `0x004557e0` (segment).

# CSI texture format — SOLVED

Recovered from the engine's own header validator (`FUN_004452C0`, called by
`csiLoadHeader`) and channel decoder (`csiGetARGBFormat`), both in `RCCars.exe`.
Converter: `csi2png.py`. **All 1,316 textures convert, 0 failures.**

## Header — fixed 160 bytes

    0x00  u32   magic 'MISC'        reversed 'CSIM' (engine 4CC convention)
    0x04  f32   version
    0x08  u32   header size = 160   (validator requires exactly this)
    0x0C  u32   data offset         (validator requires >= 160)
    0x10  u32   width               (nonzero)
    0x14  u32   height              (nonzero)
    0x18  u32   bytes per pixel     (nonzero)
    0x1C  u32   mipmap count        (validator requires 1..10)
    0x20  u32   channel count
    0x24  u8[]  channel ids, 1 ASCII char each: 'A' 'R' 'G' 'B' 'I'
    0x2C  u16[] per-channel bit range, stride 2, little-endian
              high byte = end bit, low byte = start bit
              bits = hi - lo + 1;  0xFFFF = channel absent

Pixel data starts at the data offset: mip 0 first, then each half-size level,
each `width*height*bpp` bytes (dimensions floored at 1).

**Verified:** predicting file size as `160 + sum over levels of (w>>i)*(h>>i)*bpp`
matches **1316/1316** files exactly. PNG output dimensions match all 1316 headers.

## Pixel formats present in RC Cars

Channels are packed into a `bpp`-byte little-endian integer and extracted by bit
range, so the format is fully data-driven rather than an enum. Observed:

| bpp | channels | ranges                   | format    |
|-----|----------|--------------------------|-----------|
| 2   | RGB      | 11-15, 5-10, 0-4         | RGB565    |
| 2   | ARGB     | 12-15, 8-11, 4-7, 0-3    | ARGB4444  |
| 3   | RGB      | 16-23, 8-15, 0-7         | RGB888    |
| 4   | ARGB     | 24-31, 16-23, 8-15, 0-7  | ARGB8888  |

Most common shapes: `(2bpp, RGB, 9 mips)` 284 files, `(2bpp, RGB, 8 mips)` 248,
`(4bpp, ARGB, 1 mip)` 240 — the last group being UI art, which has no mips.

An `'I'` channel id means intensity/greyscale (replicated across RGB). None of
RC Cars' textures use it, but the engine supports it.

## Relevance to a Vita port

Everything here is plain uncompressed packed-integer pixel data — no DXT in the
`.csi` files themselves, despite the D3D8 renderer supporting DXT1-5. RGB565 and
ARGB4444 are natively supported by GXM, so those (that is, most of the texture
set) can be uploaded to the Vita with no conversion at all. Mip chains are
already present and correctly ordered, which is exactly what you want.

## Previous next-step note: the .csi texture format

1,316 textures now decode to a container starting with magic `MISC`, followed by
a float and 32-bit width/height. The engine side is already named and
decompiled: `csiCSI2Image`, `csiGetMipmap`, `csiGetARGBFormat`,
`csiHeaderApplyFormat`, plus `imIMAGE`/`imFORMAT`/`imPIXEL` structures — see
`RCCars.exe.named.c`. That is the next format to crack, and it is far better
documented than the pack was.
