# Project Gotham Racing recomp - progress log

Newest first. Each entry: date, what changed, what was learned, what's next.

## Status
- Milestone: 1 - inventory done (Ghidra project `pgr1`, push buffers checked, GOTHPOK decoder)
- Repository: local git (main), SDK submodule on the fork's main; GitHub hk21x/pgr1-recomp (private)
- Push buffers: USED (RunPushBuffer 0xEE0D0 from 0xA96BD/0xA989C, with fixups); the runtime does not replay them yet
- XDK: 3911-3950 (D3D8 3948); no signatures yet (`sdk/xdk/3948/` to make)
- Plan: .local/PGR1-plan.md (detailed, not committed) and .local/PGR1-summary.md

## Log

### 2026-10-22 - phase 2: XDK 3948 signatures, state numbering, layout; push buffers are used after all

- Signatures: sdk/xdk/3948 seeded from 5659; the first scan found 20 of 250. Hand work (Ghidra dumps of both games'
  libraries, feature matching against PGR2's named 5659 functions, every match checked on PGR1's code) took it to
  137 functions and 11 of 13 data; the rescan reproduces game.toml. Not linked (left 0): XNet, sockets, XOnline, the
  CRT math entry points (PGR1 inlines fsin/fcos/fpatan), BinkGetSummary and about 60 D3D/DSOUND functions the game
  never calls. Not signable: D3D_PresentInterval (a device field in 3948, +0x2B64) and XPP_MemoryUnitType (MU_Init
  0x13B24C is not on the linear sweep).
- Render and texture stage state numbering differ from 5659 (FINDINGS.md); the SDK runtime now translates
  (state_numbering.h, layout StateNumbering).
- sdk/xdk/3948/layout.toml from PGR1's code, every value cited. New runtime cases, each a layout key: frame buffer
  surfaces inside the device (CDevice_FrameSurfaces 0x2150, CDevice_AutoDepthSurface 0x219C), a vertical blank
  callback that takes the count (VBlankCallback_Count), DSOUND mix bins as a mask (DSound_MixBinMask) and Lock's
  older flag bits (DSound_LockFlagsSwapped).
- Correction to phase 1: PGR1 runs recorded push buffers (D3DDevice_RunPushBuffer 0xEE0D0, two call sites, with
  vertex shader constant fixups). Reported to Harry; the runtime does not replay them yet.
- 3948 differences still to handle at boot: Present (stdcall 4) instead of Swap; GetBackBuffer / GetDepthStencilSurface
  (out-pointer forms) instead of the 2 forms; SetVertexShaderConstant (register -96..95) instead of the 1/4/Fast forms;
  the inline-state helpers 0xEA5C0 / 0xEA640 and immediate-mode SetVertexData*; the present interval (device +0x2B64,
  set from the present parameters by the library's own CreateDevice); Bink's frame hold (PGR1's movie loop paces
  itself on BinkWait); DSOUND calls the runtime does not replace (SetConeOrientation, SetMode).
Next: Harry's call on push buffers; then the runtime's 3948 entry points above, recompile, first headless boot.

### 2026-10-22 - phase 1: Ghidra project, push buffer check, GOTHPOK decoder

- Ghidra: `.local/game.exe` from `xbe2pe.py config/game.toml` (no names yet: game.toml is the template's placeholder
  until phase 2's scan), imported and analysed into `~/ghidra-projects/pgr1`; `scripts/ghidra.sh`
  works with GHIDRA_PROJECT=pgr1.
- Push buffers: **none**. PushBufferScan: the game's .text (0x11000-0xE93F0) stores no NV2A method headers (the only
  3 hits are the string "d:\" at 0xC9126, 0xCA2CE, 0xCAC2D; the rest are pointers into .rdata and code). Nothing
  writes a push buffer object's Common (0x00020001): the one such immediate, 0xF1FA9 in the D3D section, is method
  data inside a command (after method 0x40294). The resource release paths (0xED090, 0xED160) test type 2 as every
  XDK's do. The only XPR (`media\Font.xpr`) holds a texture. Recorded push buffers are not a risk for this game.
- GOTHPOK: the game's unpacker is FUN_000C99C0, called from the file loader FUN_000CA2C0 (`FileFastOpen`, reads
  through the z: cache) and the fatal error screen FUN_00046E00. Format in FINDINGS.md; `scripts/gothpok.py` decodes
  it. All 1,691 GOTHPOK files on the disc (INIs, areas, cars, shaders, wavs) unpack to their stated sizes
  (`.local/unpacked`, 1.3 GB, analysis only).
- Shared with PGR2: the SDK and the methods; not the data formats. PGR1's car INIs (`[CAR DYNAMICS]`) and its
  GOTHPOK container differ from PGR2's (`[DYNAMICS]`, INDX paks with deflate), so PGR2's ConvertPak and INI notes do
  not apply. Like PGR2 it caches files on z: ("Reading cached file").
- NTSC / Platinum comparison: no other release in the VM yet.
Next: phase 2, seed `sdk/xdk/3948/` from 5659, scan, count misses; compare render state numbering with 5659.

### 2026-10-22 - the disc reviewed; phase 0 (shared ground)

Harry's PAL disc, extracted in assets/ (2.3 GB, 4,743 files):
- "Project Gotham Racing", title id 4D530003, region PAL, version 5, built 2002-02-07; default.xbe SHA-1
  4878c3bdbd44663b64ce64986b313e8c6b7f8917.
- Libraries: XAPILIB 1.0.3950, D3D8 3948, XGRAPHC 3911, DSOUND 3936, XBOXKRNL 3911, LIBC 3911, LIBCPMT 3911. No XNET,
  XONLINE, XMV, D3DX8 or XVOICE: no system link or Live; xbe_symbols stops with "no signatures" (it wants
  sdk/xdk/3948/, the D3D8 build).
- Movies are Bink (FMV/*.bik, RAD 1998-1999 linked in); 11 vertex programs in Shaders/*.xvu; every INI and area file
  wrapped in GOTHPOK (magic "GOTHPOK\x1a", the uncompressed size, 01 00 CC CC, compressed data); 12 districts with
  day and night sets; split screen; custom soundtracks (TDATA\FFFE0000\MUSIC\ST.DB); XDemos/XDemos.xbe launched from
  the menu.
Phase 0 (in PGR2's repository and the SDK): the fork's main is the one SDK line for both games; the shared scripts
live in sdk/scripts with this game's values in scripts/game.env; the template carries the launcher, the release
workflow and these files. This skeleton: the template with PGR1's names, the SDK cloned from the fork's main (made a
submodule when the repository is created), launcher/game.json with the PAL release.
Next: phase 1, a Ghidra project for PGR1, the push buffer check, the GOTHPOK decoder.
