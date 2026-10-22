# Project Gotham Racing recomp - progress log

Newest first. Each entry: date, what changed, what was learned, what's next.

## Status
- Milestone: 3 - SMOKE OK headless: first boot, profile, arcade race (San Francisco, Mini) driven to the end of the run
- Repository: local git (main), SDK submodule on the fork's main; GitHub hk21x/pgr1-recomp (private)
- Push buffers: USED (RunPushBuffer 0xEE0D0 from 0xA96BD/0xA989C, with fixups); the runtime does not replay them yet
- XDK: 3911-3950 (D3D8 3948); no signatures yet (`sdk/xdk/3948/` to make)
- Plan: .local/PGR1-plan.md (detailed, not committed) and .local/PGR1-summary.md

## Log

### 2026-10-22 - Windows round 2: widescreen good; skip intro skips only the movie; car shadow an outline on D3D12

Harry: 16:9 and 21:9 work. "Skip intro doesn't work": his log shows `pgr1: intro movie d:\fmv\intro.bik skipped`, so
what remains is the Microsoft and Bizarre logos and the warning (about 15 s of timed screens, START skips each);
not found yet (string ids 21/22, the mode runner 0xC4D40 with modes 0x15B9E0 and 0x15BE78).
Car shadow on D3D12: only an outline. Headless (XBE_CLEAR_LOG=1, REF_DRAWLOG): the car is drawn from above into a
256x256 R5G6B5 target (03A10000, cleared white each frame with flags F0, no depth, 21 draws, FVF 0x112) and one quad
(FVF 0x142, blend ZERO/SRCCOLOR) multiplies it onto the road. The CPU reference renderer gets a solid shadow.
Next: a D3D12 capture of a race frame (XBE_CAPTURE_DIR + F3) and a D3D11 comparison from Harry.

### 2026-10-22 - PC features: skip intro, widescreen and ultrawide

- skip_intro (settings option, PGR1_SKIP_INTRO=1 headless): the movie player returns at once for FMV\Intro*.bik.
- Widescreen: PGR1 is 4:3 only and was stretched on wide displays. The projection builder 0xA57F0 is hooked so 3D
  widens (Hor+: same height, wider view) to the frame's shape (16:9, or the display's with aspect = 'screen').
  SDK: GameInfo::hud_aspect (PGR1 4:3) for the 2D squeeze, and the squeeze skips only draws whose depth comparison
  can fail (PGR1 draws its 2D with depth on and ALWAYS). Checked at 2.39:1 headless (FRAME_ASPECT_X100=239): the
  car keeps its shape, scenery reaches the edges, HUD and menu panels 4:3 in the middle. PGR2 checked: none of its
  pretransformed draws have depth on with ALWAYS, so its squeeze is unchanged.
- Both smoke tests pass on the SDK branch (fork pc-features; its first commit is titled "wip": these changes).
Next (Harry, Windows): skip intro, 16:9 and the 3440x1440 'screen' aspect in races and menus. Then FOV, hide HUD.

### 2026-10-22 - Windows: plays; sound overloaded, fixed by the library's mix bin headroom

Harry's first Windows build: plays, sound "completely broken". His capture: 27% of samples at full scale (RMS -2 dB)
in a race. Headless (WAV=, XBE_DSOUND_LOG=1): about 35 voices play at once in a race, the game's samples are mastered
hot (engine loops RMS -2 to -8 dBFS) and it sets voice headroom 0 on many (0xB00E8). The library's DirectSoundCreate
(0x1008A7) sets mix bin headroom 1 (6 dB) on bins 0-30, which the runtime's host DirectSoundCreate never applied.
SDK: layout key DSound_MixBinHeadroom (3936: 1; 5659 sets none). Race mix now RMS -8 to -10 dB; clipped samples a
second from about 21,900 to 400-700 in the busiest moments. Both smoke tests pass.
Also: build.bat now updates the sdk submodule (Harry's checkout had an empty sdk/ and CMake fetched upstream, which
has no xdk/3948), and the CMake fallback fetches the fork.
Harry: clean on Windows (2026-10-22).
Next: Windows parity, then PC features.

### 2026-10-22 - a race headless; smoke test

- Input works (the earlier missed press came before the title accepted it). First boot: cache copy to vblank 2940,
  title at 4200, then load or create new, setup driver, save to, main menu, arcade race, Kudos note, skill, city,
  car, transmission; GO near 7500. scripts/pad/race.txt presses START every 300 vblanks to 6900, then holds RT;
  scripts/pad/boot.txt stops at the title.
- `scripts/smoke.sh`: SMOKE OK, race 2651 draws a frame over the last 300 frames, no traps. SMOKE_MENU is empty:
  PGR1's menus show 3D cars (600 to 900 draws a frame), so the 2D-menu check does not apply.
- PGR2's smoke test passes on the same SDK commit (menu 11, race 2007 draws a frame).
- RunPushBuffer is still not called in the race or menus seen so far (no log line); the next place to look is
  other modes and cities.
Next: Windows (Harry's build), push buffer replay once the game calls it, the remaining 3948 behaviours as they show.

### 2026-10-22 - first headless boot: intro, logos, attract demo race

`scripts/harness.sh 240 scripts/pad/none.txt REF=1800`: no trap in 240 s, 11,400 frames, about 2,200 draws a frame
in the demo race (captures: Bink intro, Bizarre Creations logo, F50 at dusk, a city course with the mirror).
Fixed on the way (SDK unless noted):
- XAPI's DbgPrint is `int 2Dh; int3` (0xD55D4): the kernel steps over the int3; the recompiler's analysis now does.
- Bink's MMX check (0x131360) pushes ds and es: the recompiler pushed 2 bytes, x86 pushes 4 (operand size 32);
  0x126E10 returned with esi 0 and BinkCopyToBuffer read address 0x21C.
- The runtime's Bink frame hold skipped the first decode; PGR1's movie loop waits on BinkWait itself
  (game_info.cpp: `bink_frame_hold = false`, a new GameInfo field).
- The loading screen reads loading.raw (0x140000 bytes) into a 0x12C000 buffer (0xA75E0 -> 0xCAF80), 80 KB past it;
  the buffer sat at the very top of RAM. The runtime now keeps the top 128 KB as the console does.
- IDirectSoundBuffer_SetMode / SetConeOrientation ran library code on host sound objects: now host functions.
- 3948 entry points: Present, GetBackBuffer, GetDepthStencilSurface, SetVertexShaderConstant (register + 96),
  RunPushBuffer (logged, not replayed; not called in the first 4 minutes), the present interval (device +0x2B64).
Next: PGR2's smoke test on this SDK branch; a pad script to the title menu and a race; push buffer replay when the
game calls RunPushBuffer.

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
