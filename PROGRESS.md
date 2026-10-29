# Project Gotham Racing recomp - progress log

Newest first. Each entry: date, what changed, what was learned, what's next.

## Status
- Milestone: plays on Windows (Harry): races, sound, shadows, widescreen and ultrawide, skip intro, FOV, hide HUD
  checked; SMOKE OK headless (first boot, profile, arcade race)
- Achievements: 31 (ACHIEVEMENTS.md), conditions in src/game/achievement_set.cpp, tested headless with pokes
- Repository: GitHub hk21x/pgr1-recomp (made public by Harry for 1.0); MIT, copyright hk21x; SDK submodule on the fork's main (hk21x/XboxXBERecomp)
- XDK 3948: layout and signatures in sdk/xdk/3948
- Push buffers: USED (RunPushBuffer 0xEE0D0 from 0xA96BD/0xA989C, with fixups); the SDK draws them since 6bded69
- Open: the D3D12 render scale with MSAA fix to confirm on Windows; wheel untested (Harry has none to hand); draw
  distance; a PC Settings entry in the game's menus; system link and split screen untried
- Plan: .local/PGR1-plan.md (detailed, not committed) and .local/PGR1-summary.md

## Log

### 2026-10-29 - release 1.2 prepared: the launcher updates itself

Harry asked every game (through the EoN session) to take the shared launcher's update prompts and release. SDK at
0b5ff49 (fork main: launcher-update, NightFire's runtime fixes): smoke OK for PGR1 and PGR2, car select and race
frames live; launcher_cli latest against PGR1's repository answers v1.1 with its launcher and SHA-256. A release
launcher now offers newer releases at start and builds the game from its own tag. docs/releases/v1.2.md: the wheel fix,
the launcher updates (players download 1.2's launcher by hand once), the Discord server; README's update paragraph.
The tag v1.2 waits for Harry's go-ahead. Noted: the fixed-delay repro scripts (.local/mm) drift on this SDK (screens
come at slightly different times); the frames they caught were live, not stale.

### 2026-10-28 - steering wheels get the raw axis (PGR2's wheel fix, PGR1's half)

Harry asked (through the PGR2 session) for PGR2's wheel fix here. PGR1's controller poll 0xCFF10 tests the port's
XInputGetCapabilities subtype once (`CMP CL,0x10` at 0xD0113) and for anything but a wheel runs the four stick axes
through 0xCFE10, the gamepad dead zone and curve; the runtime never answered XInputGetCapabilities, so a wheel got the
pad's dead zone and bend. The SDK now answers it (a wheel for a port a bound steering device steers); PGR1's
XInputGetCapabilities is 0x13BFAD (XPP), named in game.toml and signed into sdk/xdk/3948 (the rescan reproduces
game.toml). Headless with PAD_WHEEL=7600: the game reopens the port mid-race and reads "steering wheel", the race
carries on. On the new SDK (physical memory to 128 MB for the host's buffers, NightFire's fixes): smoke OK for PGR1 and
PGR2, car select and race frames shown, the New York sequence loads. Next: Harry with a wheel.

### 2026-10-26 - release 1.1

Harry: consider the New York crash fixed; release. docs/releases/v1.1.md: the New York crash (best fit allocation),
the race HUD at 16:9 on wide screens, sharp text and HUD, XBE_MEM_LOG; tag v1.1.

### 2026-10-25 - crash loading a New York race: best fit allocation

Harry's sequence: a Quick Race in San Francisco, then Kudos Challenge > Introduction > Style Challenge (New York, Times
Square). Run headless with the memory log it loads every time: the failure depends on load timing (the radio and
loading threads allocate as a race loads), which Harry also saw (no crash on later tries). The contiguous pool put every
block at the highest free range that took it, so a small block arriving at the wrong moment split the big range a 22 MB
area needs. It now allocates best fit (the smallest range that takes a block, high first, ranges wholly in the first
16 MB last unless the request is limited to them). The sequence loads and races; smoke OK for PGR1 and PGR2. Harry's
latest log came from an older build (XBE_MEM_LOG printed nothing); the build should be at 083db8f / SDK 61b0059 or later.

### 2026-10-25 - crash loading a New York race (Harry): first look

Harry's log: "contiguous allocation of 1529000 bytes failed ... 12920 KB in use" (hex: 22.2 MB, Times Square's day area
TimeSqua.DyV, New York's areas are the largest at up to 23 MB) on the second race of a session, then a write through the
null result. 13 MB in use of about 63 means the free space was split, not used up. Not reproduced headless: arcade San
Francisco then New York, and Quick Race San Francisco then New York (Wall Street, 23 MB), all load. The runtime's own
contiguous allocations are not mid-session (sound buffers point at the game's memory, stacks have their own region).
Added: the failure lists the largest free ranges, XBE_MEM_LOG=1 logs blocks of 1 MB or more. Next: Harry's exact
sequence of races (mode, city) and a log with XBE_MEM_LOG=1.

### 2026-10-25 - ultrawide: the race HUD laid out at 16:9

Harry: on ultrawide the HUD sat in the 4:3 centre (hud_aspect 4:3, since the first widescreen work) where PGR2's is at
16:9. Stretching it to 16:9 widens every letter and the speedometer by a third (rendered to compare), so the HUD's
panels are moved instead, each whole and at its true shape. The game's layout (0x7D270: anchors from the safe area,
FINDINGS.md) cannot move: letters are separate draws, and the game drops its medals and big digits left of x 0 and the
Kudos panel right of 640 (both seen). So the renderer moves each element as it squeezes it (SDK: gpu::State::
squeeze_beyond and squeeze_shift, off unless a hook sets them; no corner kept on an edge while set, since letters' edges
fall on it), and PGR1's hooks give every element the HUD task calls its side: text by its anchor slot (0x7D4E0), the
elements that read anchors themselves (0x7F040, 0x7F510, 0x7E8F0 left; 0x7EEF0, 0x813F0 right), the Kudos panel
(0x7FEC0, found by tracing which code drew it), else none; an element with a side keeps it for what it calls. One player
only; 4:3 screens unchanged. Checked headless at 21:9 and 16:9 (REF frames), car select and the race shown, smoke OK for
PGR1 and PGR2.

### 2026-10-25 - sharp text and HUD (Sharpen2D) on for PGR1

Harry asked (through the PGR2 session) for the SDK's 2D upscaler, as PGR2 has it: GameInfo::sharpen_2d = true
(F1 > Graphics, Sharp text and HUD, on by default). Headless with REF and XBE_SHARPEN_DUMP through the menus into a race:
42 textures enlarged, all 2D (PGR1's 2D is pretransformed, so the 3948 path needs nothing more): the fonts, the
speedometer, car maker logos, medals, number plates, the city maps and menu backdrops. Close-ups: the HUD font and the
speedometer's red band lose their pixel steps without blurring; soft glows stay soft. Whole frames: menus identical at
640x480 (the gain is at higher resolutions); race frames differ no more than two identical runs do. Noted: 82B64000 is
a menu picture (A8R8G8B8 256x256) and later race textures (DXT 64x512); a replacement is keyed by address only, but
its content check drops it once the memory is rewritten. Smoke OK.

### 2026-10-25 - release 1.0.1

Harry confirmed the car select fix on Windows. Release notes docs/releases/v1.0.1.md (the Music Manager crash, the
frozen car select screen, the vertex range safeguard, push buffer replay, the new sound and debug log switches); tag
v1.0.1 publishes it with PGR1Launcher.exe.

### 2026-10-25 - frozen screen at car select (two player reports): a regression from the SDK update

The Music Manager fix moved PGR1 to the fork's newest SDK, which reads frame buffers a title supplies in
D3DPRESENT_PARAMETERS (BufferSurfaces, DepthStencilSurface at +0x34..+0x40; added for Forza, XDK 5849). XDK 3948's
parameters end at +0x34, so the runtime read the caller's stack there: at CreateDevice it named game code (0xA56E0) the
back buffer, and from the race-loading PersistDisplay/Reset on, the screen kept showing an old frame (the city select
with the loading K) while the game drew and ran on: the players' freeze with sound, and flicker. Reproduced headless
(.local/mm/repro.sh: city select, A presses, START; the frame shown stayed old while draws climbed). Fixed in the SDK
with a layout key, PresentParams_BufferSurfaces (0x34 by default, unknown for 3948). After: car select and the race
draw. Smoke OK for PGR1 and PGR2; the smoke test only counts draws, so it cannot see a stale screen.
Lesson: an SDK update brings every game's work; for PGR1 check the shown frame at car select and in a race, not only
the smoke test.

### 2026-10-24 - first bug after release: the Music Manager crashed to desktop

Harry's log: options > music manager trapped on an indirect call to 0x5C410 (from 0xC2406). The screen's setup pushes
it as a callback (0x5D324), but a gap fill guess had decoded bytes of its body first, so the recompiler's immediate
scan refused it. Fixed by naming it in config/pgr1.toml [functions]. The recompiler now lists every refused immediate
(SDK); PGR1's other 12 are data (the XPP range, .rdata's edge, a constant) and the movie section's 0xE9400, which
works. Checked headless: the Music Manager opens, tracks add to the Gotham Virtual CD, and options, driver manager,
arcade, Kudos Challenge and Time Attack screens open without a trap. Smoke OK.

### 2026-10-23 - release 1.0

Harry tested the launcher end to end (build and play from his disc). MIT license (copyright hk21x), release notes
(docs/releases/v1.0.md), tag v1.0 on 11daa6c: the release workflow builds PGR1Launcher.exe and publishes the
release with those notes.

### 2026-10-23 - the radio reviewed (Harry: it may be broken, as PGR2's was)

Headless, each part of the radio recorded alone (new XBE_DSOUND_SOLO / XBE_DSOUND_SOLO_TAG) and compared with the
files decoded by ffmpeg:
- The music: the WMA player (0xBF070, two PCM streams from 0xBF2E6) plays each track whole, at the right speed and
  without gaps (its envelope follows the track at a constant offset; the waveform matches at 0.99).
- The DJ: L_*.wav (Xbox ADPCM, 44.1 kHz stereo) streamed through the ADPCM stream set (0xAB650); the runtime's decode
  matches ffmpeg's to rounding; it drops the 65th sample of each block, which is padding (64 samples a block, as the
  file's header says).
- PGR2's fault (a voice routed only to effect sends) does not apply: the radio's voices use the default speaker bins.
- Levels as the game sets them: music at volume 0.689, the DJ at 0.294; in a race the music sits about 9 dB under the
  engine and effects.
- PGR1_DEBUG_LOG=1 (src/game/hooks.cpp) puts the game's own debug messages in the log (0x37490); with the radio's
  debug level 0x2E94AC at 5 it narrates WMAPlayer and the radio's choices.
Harry, on Windows: the radio's audio works fine. Closed.

### 2026-10-23 - SDK review fixes, 31 achievements, the README

- SDK review (everything since the shared scripts): fixed an unplugged force feedback wheel never being removed, any
  rumble pad being taken for a wheel (half throttle and brake), wheel settings unclamped, XBE_DSOUND_LOG read on every
  call, duplicate placeholder addresses, 3948's second CreateDevice leaking 5 MB, an uncapped constant count, the
  RunPushBuffer log reading unchecked memory; build.bat and launcher.bat no longer move sdk/ back past newer or
  uncommitted SDK work. The PGR2 session fixed what the review found in its work (the depth-as-colour lookup never
  ran on the GPU renderers). Smoke OK for both games.
- Achievements: three agents mapped the profile (0x2C68E0, 0x7540 bytes), the Kudos Challenge levels, cars, Kudos and
  the race state (FINDINGS.md). The 13 levels are the seasons, cities and tours; World Champion does not exist (its
  files are never loaded), so Quick Race and Arcade Race completion replace it. The saved Kudos total 0x2C6900 changes
  only on a load or leaving results, so "unchanged this frame" keeps loads from awarding. Tested headless by poking
  the values as the game writes them: each fires at once, nothing on a load-like change or a normal race.
- README in PGR2's layout with Harry's screenshot (docs/images/pgr1-pc-port.webp): launcher, building, controls,
  features, status, troubleshooting.
- Next: Harry tries the achievements on Windows; the repo going public and a release are his call.

### 2026-10-22 - render scale with MSAA froze D3D12 (device removed)

Harry: D3D12 only, MSAA on, any render scale change: the game stalls (about 10 fps, nothing new drawn) and never
recovers. The new diagnostics showed pipelines failing with 887A0005 (DXGI_ERROR_DEVICE_REMOVED) from the moment of
the change; video memory was fine (2 of 19.6 GB). Cause (SDK): RescaleTargets dropped every MSAA twin, which records
the depth twin's resolve into its depth target, then erased the depth targets, releasing them under that unexecuted
command; the GPU wrote freed memory. Every removal now goes through ReleaseTarget (resources kept until the frame's
fence), also the failed colour remake and the extension reset. D3D11 holds its resources itself. Merged with the
fork's main (PGR2's squeeze logging and depth-as-colour); smoke OK for PGR1 and PGR2. PGR2 gets the fix with its
next SDK update. Next: Harry retests D3D12, MSAA 8x, render scale changes.

### 2026-10-22 - the icon

Harry's drawing (.local/win/pgr1-icon.png, 1254 px, opaque) cut to its circle (the corners transparent) as
launcher/icon.png (512 px) and launcher/icon.ico (256 as PNG, 128, 64, 48, 32, 24, 16), as PGR2's. CMakeLists
embeds launcher/icon.ico into PGR1.exe (resource 1) and the launcher's build uses it.

### 2026-10-22 - PC features: the whole intro skipped, field of view, hide HUD

- skip_intro now also skips the logos and warning: Frontend.ini's own SkipIntroScreens (0x2E92A8) is set when the
  movie is skipped. Headless: the title in under a second; a race reached from it.
- fov_percent (50 to 200, PGR1_FOV): the race camera's zoom (its call to 0xA58E0 at 0x7863E) scaled; checked at 150%.
- hide_hud (PGR1_HIDE_HUD): the ARCADE_RACER task's drawing (0x820F0) skipped; checked in a race.
- Wheel force feedback: GameInfo::wheel from the player's speed (0x2EC9D8, m/s; checked against the speedometer,
  26.8 for 57 mph and 32.3 for 71) while the race HUD is drawn; the gear (H-pattern shifters) not yet.
- 60 fps races: not needed. PGR1 races at 60 already (frames match vertical blanks in races; the clock keeps time).
- Not done: draw distance (location.ini's IngameDrawDistance is read into the location records, 0x2D160A + city *
  0x26E +0x1C, but nothing read Financial District's during a race: a watchpoint saw no read), the PC Settings entry.
Next (Harry, Windows): skip intro, field of view and hide HUD in the settings window (F1); a wheel if you have one.

### 2026-10-22 - Windows round 2: widescreen good; skip intro skips only the movie; car shadow an outline on D3D12

Harry: 16:9 and 21:9 work. "Skip intro doesn't work": his log shows `pgr1: intro movie d:\fmv\intro.bik skipped`, so
what remains is the Microsoft and Bizarre logos and the warning (about 15 s of timed screens, START skips each);
not found yet (string ids 21/22, the mode runner 0xC4D40 with modes 0x15B9E0 and 0x15BE78).
Car shadow on D3D12: only an outline. Headless (XBE_CLEAR_LOG=1, REF_DRAWLOG): the car is drawn from above into a
256x256 R5G6B5 target (03A10000, cleared white each frame with flags F0, no depth, 21 draws, FVF 0x112) and one quad
(FVF 0x142, blend ZERO/SRCCOLOR) multiplies it onto the road. The CPU reference renderer gets a solid shadow.
Harry: the same on D3D11. His D3D12 capture: the shadow target holds the car's dark silhouette (76,76,88) with
alpha 0 (cleared white, alpha 255); the shadow quad's shader discards texels with alpha 0 (AlphaKill on stage 0), so
only the filtered edges drew: the outline. R5G6B5 has no alpha: the console reads it as 1. SDK fix: kTexOpaque, both
renderers force alpha 1 when a target is read as R5G6B5, X1R5G5B5 or X8R8G8B8 (swizzled or linear). wincheck and
DXC pass, both smoke tests pass. Harry: shadows solid on Windows (2026-10-22).

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
