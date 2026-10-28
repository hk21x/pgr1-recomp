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
- **[game]** Race HUD layout: 0x7D270 (called first by the HUD task 0x820F0) sets the anchors from the safe area
  percentage 0x159B40: x,y pairs from 0x192484, 8 bytes apart (top left, bottom left, top right, bottom right, top and
  bottom centre, centre). Text takes its place from an anchor slot through 0x7D4E0 (fastcall, the slot in ecx); some
  elements read anchors themselves (0x7F040, 0x7F510 top left; 0x7E8F0 bottom left: Lap; 0x7EEF0, 0x813F0 bottom right:
  the speedometer); the Kudos panel and its pop-ups are 0x7FEC0 (through 0x7F8E0), the targets 0x81170. The game does not
  draw its medals and big digits left of x 0 nor the Kudos panel right of 640, so a wider layout cannot move the game's
  coordinates: the renderer moves each element (gpu::State::squeeze_shift, src/game/hooks.cpp).
- **[game]** The HUD and menus are pretransformed (FVF 0x144) with depth on and the comparison ALWAYS (0x207); the SDK's
  2D squeeze now counts only real depth tests, and squeezes PGR1's 2D into the centred 4:3 (GameInfo::hud_aspect).
- **[game]** The car shadow: the car drawn from above into a 256x256 R5G6B5 target (cleared white, no depth, FVF
  0x112, colour TFACTOR), then a quad (FVF 0x142) multiplies it onto the road with alpha kill on. The draws leave
  alpha 0 in the silhouette; the console reads an R5G6B5 target's alpha as 1. **[console]** The SDK now does too.
- **[game]** Start-up screens: after the movie the front end shows the Kudos splash, "Presented By Microsoft", the
  Bizarre Creations logo and the warning (about 21 s). Frontend.ini's own SkipIntroScreens (0x2E92A8; read into it
  by the INI loader 0x4DAB4, tested by the movie start 0xC9379 and the intro handler 0x566B1) skips them all.
  SkipPresentation (0x2E92A0, copied to 0x2C85C4 at 0x53C2C) is something else; left alone.
- **[game]** Race camera: every camera's projection goes through 0xA58E0 (cdecl: zoom, ...; fov 1.14 rad over the
  zoom, horizontal negated); the race camera calls it at 0x7863E (zoom 1; 2 in the fly-by), the car shadow camera at
  0x284BB (12), menus at 0xA6F39 and 0xAA65C (1.8); the rear mirror builds its own at 0xA59CA.
- **[game]** The game's task table (0x15B9F0 to 0x15BEB8, 24 bytes each: name, flags, four handlers; slot 2 draws,
  0x119E0 is an empty function): ARCADE_RACER (0x15BDB0) draws the race HUD at 0x820F0. OVERLAYS (0xC4CA0) and
  COMMON_LOAD (0x94280) are not the HUD.
- **[game]** Wheels: the controller init 0xCFD00 and poll 0xCFF10 read each port's XInputGetCapabilities (0x13BFAD, XPP;
  the record at 0x29ACFC + port x 0x8C, its SubType first) when they open it. The poll's only subtype test,
  `CMP CL,0x10` at 0xD0113 (XINPUT_DEVSUBTYPE_GC_WHEEL), skips 0xCFE10 for the four stick axes: the gamepad dead zone
  and response curve. **[console]** The runtime now answers XInputGetCapabilities (wheel for a port a bound steering
  device steers), so a wheel gets its raw axis as on the console.
- **[game]** Draw distance: location.ini's IngameDrawDistance / PresentationDrawDistance per city, read into the
  location records (0x2D160A + city * 0x26E, +0x1C and +0x20); their reader not found yet.

## Achievements (memory map)
- **[game]** The driver profile is one block, 0x2C68E0 to 0x2CDE20 (0x7540 bytes), the "driver" save: read whole by
  0xD0210 (from 0xD0530), written by 0xD0350 (from 0xD0745), reset by 0x53A70. A load rewrites it in one frame.
- **[game]** Saved Kudos total: int 0x2C6900, stored by 0x57B00 (the sum 0x57420 of the race records, arcade arrays and
  the bonus int 0x2CDD4D) only on leaving a results screen (0x6B837, autosave 0xD1180) and on a load (0x8C420). The
  live total is the float 0x2C8FFF (0x573A0).
- **[game]** Kudos Challenge: 13 levels from frontend\chap01-13.ini (loader 0x53F00; table 0x2E4D20 + level x 0x18:
  name, first race +0xC, race count +0x10; count 0x2E5AC4): introduction, amateur season, san francisco, london, tokyo,
  new york, professional season, world tour, challenge tour, endurance season, world class season, invitational
  season, bonus level. Race records 0x2C90E1 + race x 0x19; medals are computed from Kudos against the race's targets
  (0x57300, race definitions 0x2DA460 + race x 0x48). At a level race's end (mode 1) 0x6A1A7 stores 0x46110's summary
  into 0x2C6C2C + level: bit 0 every race a medal (complete; 0x6A06B opens the next), bit 1 all silver, bit 2 all gold.
  Bonus level opens when 1-12 are all silver (0x2C90D1 bit 0); 0x2CA053 all gold.
- **[game]** Quick Race (mode 2) and Arcade Race (mode 3): 4 levels each, bit 0 of 0x2C6C39 + 4 (mode 3) + level
  (0x425A0).
- **[game]** Cars: 26 groups (29 cars, frontend cars.ini; table 0x2E4E58), unlocked when bit 0 of 0x2CD765 + 4 x group is
  set. MINI, MR2 and SLK from the start; One on One wins (0x6AAF0), Quick/Arcade levels (0x425A0) and the Kudos totals
  of extras.ini, 1,000 to 200,000 (0x6BB10), unlock the rest.
- **[game]** Race state: 0x2E71C8 = 1 while a recording plays (attract demo, replays; 0x69160). Mode 0x2C6F80 (1 Kudos
  Challenge, 2 Quick Race, 3 Kudos Race, 4 Time Attack, 6 multiplayer, 7 Medal Pursuit); race type 0x2C6982 (0 hot lap,
  1 timed run, 2 time attack, 3 one on one, 5 street race, 6 challenge, 8 car challenge, 9 multiplayer, 10 quick race,
  11 kudos race; names 0x2E593C); human players 0x2C695E. At the last lap 0x887A0 sets, in one frame, player 1's race
  over 0x2E5D49, position 0x2E5D51 (1 first) and finished properly 0x2E5D4D (a timed-out challenge only the first).
- **[game]** Player 1's race Kudos 0x2C6CD9 (int16s, reset by 0x69060, inside the profile): contacts +6 (0x45760), each
  Kudos type's count from +8 (two wheels 0x2C6CE9, cone gates 0x2C6CF5), combo +0x2E. Gearbox 0x2C694E (0 manual;
  player 1's race copy 0x2C6E52), night 0x2E71BC (0x68190 from the hour 0x2E7208 and the city's daylight). Medal
  Pursuit counts new medals in 0x2C6CC1 (0x696A0).

