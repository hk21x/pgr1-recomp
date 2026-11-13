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
`screenshots/` (F12: the frame as shown, as `.tga`).

## Controls

The same as on Windows (README): the keyboard plays as controller 1, and SDL reads Xbox, PlayStation and Switch
controllers. F1 opens the settings window. Cmd+Enter or Alt+Enter switches between a window and full screen.

## The best look

These hold 60 fps in races on an M-series Mac at 3440x1440. Set them in F1, or in `pgr1.toml`:

```
msaa = "msaa4"          # 4x MSAA on the 3D scene
alpha_coverage = true   # with MSAA: smooth edges on alpha-tested fences, trees and wires
world_textures = true   # the world's textures enlarged twice with sharp edges
render_percent = 150    # 150% of the screen's resolution, downsampled
sharpen = 30
aniso = 16
```

Render scale 200%, SMAA, and FXAA with sharpening also held 60 fps in testing. SMAA and FXAA cost less than MSAA,
but they smooth less.

## Differences from the Windows build

- The renderer is Metal. The game's HLSL goes through glslang (to SPIR-V) and SPIRV-Cross (to Metal's shading
  language) when a draw first needs it. `shadercache-metal.bin` beside the game keeps the translations for later
  runs. A new scene can still hitch briefly the first time, while Metal compiles its shaders.
- MSAA resolves through Metal's render pass store action: colour, and depth and stencil from sample 0.
- Metal's samplers on macOS have no mip LOD bias. The pixel shaders add each stage's bias instead.
- Windows draws a line one console pixel wide with a geometry shader. Metal has no geometry shaders, so the line is
  drawn again, moved by whole host pixels. A blended line is drawn once, so it is thinner.
- Metal has three texture border colours. Any other colour shows as the nearest one, and the log says so.
- A NOOVERWRITE lock of a render target waits for the GPU, where Windows reads back a frame-old copy.
- There is no DirectInput wheel support. SDL reads a wheel only if SDL lists it as a controller.
- There is no launcher, splash or game-code DLL: the game's code links into the executable.

## Diagnostics

- `XBE_PERF=1`: every 10 seconds the log gets a line with the frame rate, draws, command buffers, read-backs and GPU
  waits per frame.
- `XBE_SHOT_EVERY=N`: a screenshot of every Nth frame as shown (after anti-aliasing and filters, before the
  settings window) into `screenshots/`.
- The headless harness runs on macOS too: `HOST_FLAGS=-DPGR1_RELEASE=2 GAME=assets/<image>.iso scripts/smoke.sh`.
  It needs Homebrew's `bash` first in `PATH`. `PGR1_RELEASE` 2 is the USA and Asia discs, 1 Europe 1.0, 3 Japan,
  and 0 Europe 1.1.
