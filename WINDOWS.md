# Building and running PGR1 on Windows

Everything below runs on your PC; nothing here touches the Linux VM. FINDINGS.md explains the fixes referred to here.

## What you need

- Windows 10 or 11, 64 bit, a GPU with Direct3D 12 or 11.
- Visual Studio 2022 (or later) Community with **Desktop development with C++**. `build.bat` offers to add the two
  extra components it needs (**C++ Clang tools for Windows**, **C++ CMake tools for Windows**); say Y, or add them by
  hand under Modify, Individual components.
- Git for Windows: https://git-scm.com/download/win
- Your PGR1 disc's files: `default.xbe` with the folders beside it (Async, FMV, FrontEnd, Game, Images, media,
  Shaders, XDemos, ...), exactly as they are in the VM's `assets` folder. An .iso of the disc also works.
- About 3 GB free, plus room for your copy of the game.

## Steps

1. Clone with the SDK.
   ```
   git clone --recurse-submodules https://github.com/hk21x/pgr1-recomp.git
   cd pgr1-recomp
   ```
   Already cloned? `git pull`; `build.bat` brings the SDK submodule to the commit the checkout records. If the
   build ever says `xdk\3948\layout.toml does not exist`, the SDK is not the fork's: run
   `git submodule update --init sdk` and `git -C sdk log --oneline -1`.
2. Put the game in `assets\`: `assets\default.xbe` and its folders, or one `.iso` (then also copy `default.xbe`
   from the disc into `assets\`).
3. Run `build.bat`. It recompiles default.xbe (seconds) and compiles everything (a few minutes the first time on a
   fast PC; the game's code goes into `PGR1.dll`).
   Answer Y to "Play now?", or run `out\build\release\PGR1.exe`.
4. The first start formats the emulated cache partition and copies the game's files to it (about 90 seconds of
   black screen and a short movie), then the logos. Later starts reuse it. To repeat the first start, delete
   `out\build\release\hdd`.

Files the exe writes beside itself, in `out\build\release`:
- `logs\pgr1.log` the log of the last run (overwritten each start; send it with any report)
- `pgr1.toml` settings. Speed: the game's own frame rate is honoured (60 in menus and races, as on the console).
- `hdd\` the emulated hard disk (saves and caches)
- `CrashDumps\` a minidump when it crashes

**Settings window: F1**, or **Back + Start** on a controller. Tabs:
- Display: monitor, display mode, resolution, vsync, aspect ratio. PGR1 has no widescreen mode of its own: on a
  wide display the 3D widens to the window's shape, ultrawide included, the race HUD is laid out at 16:9 (its panels
  moved out, each at its own shape; one player) and the menus stay 4:3 in the middle.
- Graphics: render scale, anisotropic filtering, frame rate cap, antialiasing, frames-a-second counter, renderer.
  Sharp text and HUD (on by default: the menus', HUD's and text's textures are enlarged two or four times with sharp
  edges on a background thread, so lettering and gauges stay crisp at high resolutions; a texture shows at its
  original size for a moment the first time it appears).
- Game: skip intro (the movie, logos and warning; next start), hide the race HUD (at once), race camera field of
  view in % (50 to 200, at once).
  Radio music volume in % (0 to 400, at once): the music against engine and effects; 100 is the game's own balance.
- General: pause when the window is not in front.
- Wheel: steering wheels, pedals and shifters; force feedback follows the player's car in races.
- Controls: keys and controller remapping.

Keyboard (controller 1 while the game window has the focus; remap in F1 > Controls):
- Steer / menus: arrow keys or A / D; accelerate W (right trigger), brake S (left trigger)
- A: Enter or Space; B: Backspace; X: Q; Y: E; Start: Escape or P; Back: Tab

## Reporting a problem

Help and reports: the [Discord server](https://discord.gg/PQjkKsdtc). The files below help with any report.

- Send `out\build\release\logs\pgr1.log` and a short clip or screenshot.
- Wrong pictures: a frame capture. From a command prompt in the repository folder:
  ```
  set XBE_CAPTURE_DIR=C:\pgr1cap
  out\build\release\PGR1.exe
  ```
  Press **F3** when it looks wrong; the log says `d3d12: frame captured to C:\pgr1cap\<time>`. Copy that folder
  and the log into the VM's `.local/win/`.
- Sound: `set XBE_DSOUND_LOG=1` before starting lists every sound made and, every 5 seconds, the loudest playing.
  `XBE_DSOUND_LOG=2` also lists every stream playing (the radio's music and DJ among them) with its volume.
  `set PGR1_DEBUG_LOG=1` puts the game's own debug messages in the log (the files it loads, its sound set-up).
- Stutter: `set XBE_PERF=1` before starting adds timing lines to the log. `set PGR1_TICK_LOG=1` logs the game's steps
  and frames every 10 seconds (60 steps a second is real time); `set PGR1_FIXED_TIME=1` goes back to one step a frame, as
  on the console. `set PGR1_INTERP=1` with a frame rate cap above 60 draws blended frames between the game's steps in
  races (experimental: it flickers while the renderer is too slow for the extra frames).
- A crash with "contiguous allocation ... failed" in the log: `set XBE_MEM_LOG=1` before starting lists the game's large
  memory blocks as races load and end.

## The launcher and releases

`PGR1Launcher.exe` installs and plays the game for someone with no tools: choose the disc image or folder, press **Get
the game ready**, then Play. It downloads the release's package for that disc (`PGR1-E11-package.zip`,
`PGR1-E10-package.zip` or `PGR1-USA-package.zip`, by the disc's `default.xbe` SHA-1 in `launcher/game.json`: the exe, its
runtime DLLs and the compiler, no game code) into a `PGR1` folder beside itself and runs `PGR1.exe --prepare-code`, which
builds `PGR1.dll` from the disc. The new version is unpacked and built in `play\.next` and only then replaces the game.
An install from 1.3 or earlier (built from source) is updated in place, keeping `hdd` and the settings; **Manage > Remove
the old build tools** then deletes the old tools and source.

Make it: run `launcher.bat` (it needs what build.bat needs); it leaves `PGR1Launcher.exe` here. To try a package without
a release, run `build.bat`, then `release-package.bat`, and put the zip beside the launcher in an empty folder: a zip
beside the launcher is used before any download. Send `PGR1\launcher.log` with any report.

**Releasing:** a `v*` tag builds the launcher and makes a **draft** release with `docs/releases/<tag>.md` as its notes
(the build stops if that file is missing). Each disc is its own compile, so each needs its own package: for every disc
there is one of, in a clean checkout of the tag with that disc in `assets`, run `build.bat` then `release-package.bat`
(it names the zip from `default.xbe`'s SHA-1), and attach the zip and its `.json` (size and SHA-256, which the launcher
checks) to the draft. Then publish. A disc with no package cannot be installed with that release's launcher.
