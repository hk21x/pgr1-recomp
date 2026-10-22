# Project Gotham Racing recomp - progress log

Newest first. Each entry: date, what changed, what was learned, what's next.

## Status
- Milestone: 1 - inventory done (Ghidra project `pgr1`, push buffers checked, GOTHPOK decoder)
- Repository: local git (main), SDK submodule on the fork's main; GitHub repo hk21x/pgr1-recomp (private) to make
- Push buffers: none (no D3DPushBuffer objects, no hand-written NV2A commands in game code)
- XDK: 3911-3950 (D3D8 3948); no signatures yet (`sdk/xdk/3948/` to make)
- Plan: .local/PGR1-plan.md (detailed, not committed) and .local/PGR1-summary.md

## Log

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
