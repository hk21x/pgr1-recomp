# macOS

A native build for Apple silicon Macs. The game's code is recompiled to arm64. Metal draws the frames, and SDL3
handles the window, keyboard, controllers and sound (CoreAudio). It does not use Wine or Rosetta.

## Build

You need:
- macOS on Apple silicon, with Xcode or its command line tools (`xcode-select --install`).
- Homebrew's `cmake`, `ninja`, `glslang` and `spirv-cross`: `brew install cmake ninja glslang spirv-cross`.
- Your disc image (`.iso`) of a [supported release](README.md#supported-version), in `assets/`.

```
git clone --recurse-submodules <this repository>
cd pgr1-recomp
cp ~/path/to/your/game.iso assets/
./build-macos.sh run
```

`build-macos.sh` copies `default.xbe` out of the image into `assets/` (`scripts/xiso_get.py`), because the build
recompiles from that file. It then builds `build/mac/PGR1` and, with `run`, starts it. The game finds the image in
`assets/` beside or above the executable. The first build takes a few minutes, and later builds are quicker.

The game writes the same files as on Windows beside the executable: `pgr1.toml`, `hdd/`, `logs/pgr1.log`, and
`screenshots/` (F12, as `.tga`).

## Controls

The same as on Windows (README): the keyboard plays as controller 1, and SDL reads Xbox, PlayStation and Switch
controllers. F1 opens the settings window. Cmd+Enter or Alt+Enter switches between a window and full screen.

## Differences from the Windows build

- The renderer is Metal. The game's HLSL goes through glslang (to SPIR-V) and SPIRV-Cross (to Metal's shading
  language) when a draw first needs it.
- MSAA and SMAA are not in the Metal renderer yet. Render scale works. FXAA, sharpening and the upscale filter use
  the Windows build's shader. It compiles on Metal, but nobody has compared its look with Windows yet.
- A NOOVERWRITE lock of a render target waits for the GPU, where Windows reads back a frame-old copy.
- There is no DirectInput wheel support. SDL reads a wheel only if SDL lists it as a controller.
- There is no launcher, splash or game-code DLL: the game's code links into the executable.

## Diagnostics

- `XBE_PERF=1`: every 10 seconds the log gets a line with the frame rate, draws, command buffers, read-backs and GPU
  waits per frame.
- `XBE_SHOT_EVERY=N`: a screenshot of every Nth frame into `screenshots/`.
- The headless harness runs on macOS too: `HOST_FLAGS=-DPGR1_RELEASE=2 GAME=assets/<image>.iso scripts/smoke.sh`.
  It needs Homebrew's `bash` first in `PATH`. `PGR1_RELEASE` 2 is the USA and Asia discs, 1 Europe 1.0, 3 Japan,
  and 0 Europe 1.1.
