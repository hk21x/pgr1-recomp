#!/usr/bin/env bash
# Build the native macOS port (Apple silicon): Metal renderer, SDL window, input and audio.
#   ./build-macos.sh            configure and build build/mac/PGR1
#   ./build-macos.sh run        build, then start the game
# Needs Xcode's command line tools and Homebrew's cmake, ninja, glslang and spirv-cross, and your disc in assets/:
# the .iso, and default.xbe copied out of it (scripts/xiso_get.py does that).
set -euo pipefail
cd "$(dirname "$0")"
for f in cmake ninja glslang spirv-cross; do
  command -v "$f" >/dev/null || { echo "missing $f: brew install cmake ninja glslang spirv-cross" >&2; exit 1; }
done
if [ ! -f assets/default.xbe ]; then
  iso=$(ls assets/*.iso 2>/dev/null | head -1 || true)
  [ -n "$iso" ] || { echo "put your disc image (.iso) in assets/" >&2; exit 1; }
  python3 -I scripts/xiso_get.py "$iso" default.xbe assets/default.xbe
fi
[ -f sdk/cmake/XbeRecomp.cmake ] || git submodule update --init
cmake -S . -B build/mac -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build/mac --target PGR1
echo "built build/mac/PGR1"
if [ "${1:-}" = run ]; then exec build/mac/PGR1; fi
