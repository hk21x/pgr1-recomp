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
- **[game]** Car select draws its car into a 1024x1024 target (the projection from 0xA592D, with that target bound) and
  shows it through a 2D quad, so that camera is not widened (0xA57F0's override tests the target's shape). The 256x256
  reflection targets still use a projection built while the 640x480 target was bound, so they stay narrowed.
- **[game]** The race intro's letterbox: GAMESTUFF_2's draw 0x74660 draws untextured quads above y 100 and below y 380, and
  a fade between, through 0xEDE60 (register, x, y, z, w). 640x280 is about 2.29:1, so the override sizes the bars to that
  shape on the frame (100 at 4:3, 53 at 16:9, none from 2.29:1).
- **[game]** The race's mix reaches full scale with the game's own volumes (over 13,000 clipped samples in 100 s of
  race, music at 0.17, effects at 0.25, the DSP effects not modelled). The radio is the WMA player's PCM streams;
  the DJ clips are ADPCM streams, with the ambience.
- **[game]** Loading screen: 0xC4A30 calls 0xA75E0 (loading.raw into the back buffer 0x2997D8, a plain copy into the front
  0x2997D4, Present), draws the text into each buffer (two presents), and the spinner 0xA7700 writes its K into both
  every 50 ms. **[console]** A CPU write without a lock is invisible to the GPU renderers' targets (headless draws in guest
  memory, so it cannot show there); the override marks both buffers after 0xA75E0.
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


## Releases

Each disc's default.xbe is another compile. Europe 1.0's code and data are Europe 1.1's moved by 0xE0 where hooks look.
USA 1.0 is a build with XDK 3925's D3D8 and DSOUND: data moves in steps (pgr1::Va in src/common/release.h), and
105 data files differ from Europe's (cars.ini only in DemoCar; the chapter files are the same). The Bink section moved whole.
Asia's xbe (c14b35767fddc5a7af4b3aa13b53e374be23fe57) has USA's sections with another version number (6), and every
data file the same as USA's: it plays as USA (GameInfo::same_code, the USA package); Japan's
(4c38cf630f7dc92a845d2141020157e78e682e3d) is another build. Neither is supported yet.

## Menus

- **[game]** Front end pages (Options 0x5CE80, Game Settings 0x5B320): a builder calls 0xC0760 (the byte remembering the
  focus), makes items, links them 0x64CC0 (items, count, 0), then per item 0xC0CC0 and 0xC0B90 (item, event, handler;
  5 A, 7 B), focuses one 0xC0E60 and titles the page 0x62A30. An item 0x62880: place in the list, label pointer, icon
  sprite (the int16s 0x2E8D80..., not a string id: a bad one crashes the draw), A's handler. A row 0x64C70 adds a table
  and its int: A opens a spinner (0x64810), each choice 0x18 bytes from +0x18 (text id, value, handler when chosen, the
  row's shower 0x5B720). 0x618F0 on A is for items that leave the page: on a row it freezes the spinner. Strings:
  a char pointer per StringList.ini id at 0x2E7400; ids 1361 to 1499 are unused.
- **[game]** Pause menu: a menu (0x2C bytes, root 0x159D3C) points to entries (0x34 bytes, id -1 ends them): text id,
  handler (menu, entry, action: 1 measure, 2 draw, 3 A, 4 left, 5 right), submenu, value data, +0x10 bit 0 hidden. 0x83BE0
  is the plain item; 0x192664 the menu open; a menu's +0x24 the item that opened it, whose +0x20 is its height. Text goes
  through a format ("%%"). USA draws it larger, and clips at about x 560. The root's table is static, so
  src/game/settings_entry.cpp copies it to insert RECOMP and copies its hidden flags each frame (0x85D20).
  The code that sets those flags was not found: nothing names the table's addresses directly.

## Frame rate

- **[game]** One fixed tick a frame: the main loop 0xC9710 calls 0xC5480 (every task's update once, adding to the counter
  0x299434), then draws (0xC4F20, which presents through 0xA5130), with no accumulator. The step is 0x15AA84 x 1/60
  (0x143784); 0xC9200 sets 0x15AA84 to 1.1 at 60 Hz, 1.32 at 50 Hz, so game time runs at 1.1x by design (read at 0x9420F
  by the world and cars update 0x941D0; 14 more places hardcode 1/60, and the rate 0x2BA9F0 has 79 readers). A late frame
  was slow motion. Replay fast forward (0x15A61C == 3) calls 0xC5480 four times (0xC9769-0xC9786). The FRAME_RATE task's
  init 0xC5400 sets interval 1 and the rate 60; with more than two players (0x2C695E) 0xC4650 sets 30 and interval 2.
  0xC8BD0 measures the frame time and lowers detail (0x15ABD0, 0x15ABD4, 0x159ADC) and an update budget (0x2B1DC0) past
  15 to 22 ms. Pads are polled once a tick (0xC64B0, XInputGetState at 0xD001B). src/game/frame_rate.cpp runs the
  main loop's tick as often as 1/60 s steps have passed (1 to 4).
- **[game]** Above 60: with the rate 0x2BA9F0 set to N and the step 0x15AA84 scaled by 60/N (USA keeps the 1.1 in its data
  at 0x158284 rather than setting it at boot), the game runs N ticks a second at the right speed: headless at 120 the car's
  acceleration and the race clock match 60. Blending in-between frames (the SDK's step_interpolation) did not work for PGR1:
  a step wants up to 30 MB of kept vertices, and each blended frame redraws its 3600 draws.
- **[game]** Saves carry the display's rate: 0xD0650 writes it (from the float 0x2BA9F4, 60 or 50) into the profile's
  byte 0x2C68E6, and a driver is only offered to load (0x5EDB0, 0x5EB6C: the list's byte 6 before each 0x40 byte entry
  from 0x190C10) and only loads (0xD0490, at 0xD0591) when it matches. So the display rate must stay 60; only the rate
  the game runs at (0x2BA9F0) is raised. The save file is HMAC-SHA1 (zero key) over the profile block, in its first 20 bytes.
