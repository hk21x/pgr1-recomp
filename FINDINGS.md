# Project Gotham Racing recomp - findings

What the work has established, by topic. PROGRESS.md has the dated log with the evidence for each item. Each item
is marked **[console]** (how the Xbox, its XDK or the runtime behaves: the fix is in the SDK) or **[game]** (this
game's own code and data, with its addresses).

## The image
- **[game]** `default.xbe` sections: .text 0x11000-0xE93F0 (game, XAPI, libc), D3D 0xE9940, XGRPH 0xFC620,
  DSOUND 0xFD340, WMADEC, BINK (several), XPP 0x13B080, .rdata 0x142820, .data 0x1563A0, DOLBY. The D3D section
  holds its own data too (device pointer at 0xFC5F0).
- **[game]** Recorded push buffers ARE used (corrected 2026-10-22; phase 1 said none): the game calls
  D3DDevice_RunPushBuffer 0xEE0D0 from 0xA96BD and 0xA989C with a model object's push buffer (+0x34) and builds fixups
  for it in 0xAB230 (D3DPushBuffer_BeginFixup 0xEE060, D3DPushBuffer_SetVertexShaderConstant 0xEDFA0 three times with
  the world matrix from GetTransform(6), EndFixup 0xEE090). The game's own code writes no NV2A commands, and nothing
  stores an immediate 0x20001 Common, which is why the phase 1 scan missed it. Where the push buffers' contents come
  from (data files or recorded at run time) is not traced yet.

## Data files
- **[game]** GOTHPOK container (INIs, areas, cars, shaders, sounds): "GOTHPOK\x1a", u32 unpacked size at +8, u16 at +0xC
  (0 = stored, data copied), 0xCCCC, u32 at +0x10 kept by the loader but not used to unpack, data at +0x14. Packed data:
  a u32 flag word, then 30 items from its top bit down: 0 = a literal byte, 1 = a u16 match; the word's low 2 bits b
  give length = (w >> (14 - b)) + 3 and distance = (w & (0x3FFF >> b)) + 1. Unpacker FUN_000C99C0, loader
  FUN_000CA2C0 (FileFastOpen, via the z: cache). `scripts/gothpok.py` (one file, or `--tree dir out`).
- **[game]** FUN_00046E00 is the fatal error screen (draws a 1-bit message bitmap, then loops forever); the loader
  calls it when a file can't be opened.

## XDK 3948 (D3D8)
- **[console]** Render state numbering differs from 5659: 82 simple (0-81), 34 deferred (82-115), 28 complex
  (116-143); 5659 has 92, 44 and 30. SetRenderStateNotInline 0xEA5E0: `< 0x52` simple, `< 0x74` deferred, else the
  complex setter table 0xF8258 indexed by state - 116 (0xEA5E5, 0xEA609, 0xEA62F; simple ones go through the
  encode table 0x142A08 to the simple setter 0xEA590 at 0xEA5F8). Deferred 82-115 are 5659's FogEnable to
  PatchSegments (each read where 5659's is: fog 0xF1340, wraps 0xF1140, lighting 0xF1CB0/0xF0A60, point sprites
  0xF0C50); no SwapFilter, PresentationInterval or unused slots. Complex: 5659's order without MultiSampleMode and
  MultiSampleRenderTargetMode (one MultiSampleType at 134, setter 0xEA530) and without SampleAlpha; each setter
  checked by its method (FrontFace 0x3A0, LogicOp 0x17BC, MultiSampleMask 0x1D7C, ShadowFunc 0x1E6C, cull 0x1D84).
- **[console]** Texture stage state numbering: ColorOp, ColorArg0-2, AlphaOp, AlphaArg0-2, ResultArg,
  TextureTransformFlags (0-9), then AddressU to AlphaKill (10-21), BumpEnvMat00 on as in 5659. The texture setup
  0xF1140 packs slots 10-12 into method 0x1B04 (address) and 13-21 into the filter and control words; slot 0 is
  compared with D3DTOP_BUMPENVMAP. 32 per stage at D3D_DeferredTextureState 0xF9400.
- **[console]** The runtime keeps 5659's numbers and translates at the title's tables (sdk runtime/xdk/state_numbering.h,
  layout.toml `StateNumbering = 3948`). Simple states need no table: they are keyed by the encode table's method.

## PC features
- **[game]** The intro movie: startup 0xC9200 plays `FMV\Intro.bik` (IntroJ/G/F/S/I by language, 0xC9383) through the
  movie player 0xE9780 (cdecl: the path, result unused; 0xDD920 loads the Bink sections around it with 1, then 0).
  skip_intro returns from it for those paths.
- **[game]** No widescreen mode: the game never calls XGetVideoFlags (0xD4321). Every 3D projection is built by
  0xA57F0 (cdecl: out, height/width (0x2BBB64, 0.75 from 0xC8158), near, far, horizontal and vertical fov; the only
  fptan in the game), x scale cot(hfov / aspect / 2) at out[0]. Widescreen is Hor+: out[0] times 4:3 over the frame's
  shape. No culling gaps seen at 2.39:1.
- **[game]** The HUD and menus are pretransformed (FVF 0x144) with depth on and the comparison ALWAYS (0x207); the SDK's
  2D squeeze now counts only real depth tests, and squeezes PGR1's 2D into the centred 4:3 (GameInfo::hud_aspect).
