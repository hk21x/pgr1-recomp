#!/usr/bin/env bash
# Build the iOS / iPadOS port (an app for your own device, signed with your Apple developer team).
#   ./build-ios.sh                  configure and build build/ios (Release, any iOS device)
#   ./build-ios.sh install DEVICE   build, then install on DEVICE (a name or UDID from `xcrun devicectl list devices`)
#   ./build-ios.sh disc DEVICE      copy the game to the app on DEVICE: the disc image trimmed to its game data
# Needs XBE_IOS_TEAM (your team ID) and XBE_IOS_BUNDLE_ID (any bundle ID your team can register), in the environment
# or in .local/ios.env. The recompiled code comes from the macOS build: run ./build-macos.sh first.
set -euo pipefail
cd "$(dirname "$0")"
[ -f .local/ios.env ] && . .local/ios.env
: "${XBE_IOS_TEAM:?set XBE_IOS_TEAM, your Apple developer team ID (in the environment or .local/ios.env)}"
: "${XBE_IOS_BUNDLE_ID:?set XBE_IOS_BUNDLE_ID, e.g. com.yourname.pgr1 (in the environment or .local/ios.env)}"
[ -f generated/game/codegen.stamp ] || { echo "no recompiled code yet: run ./build-macos.sh first" >&2; exit 1; }

device_id() {
  xcrun devicectl list devices 2>/dev/null | awk -v d="$1" 'index($0, d) { for (i = 1; i <= NF; i++) if ($i ~ /^[0-9A-F-]{25,}$/) { print $i; exit } }'
}

if [ "${1:-}" = disc ]; then
  dev=$(device_id "${2:?device name or UDID}")
  [ -n "$dev" ] || { echo "no device ${2}" >&2; exit 1; }
  iso=$(ls assets/*.iso 2>/dev/null | head -1 || true)
  [ -n "$iso" ] || { echo "put your disc image (.iso) in assets/" >&2; exit 1; }
  name=$(basename "$iso")
  mkdir -p .local/ios
  [ -f ".local/ios/$name" ] || python3 -I scripts/xiso_trim.py "$iso" ".local/ios/$name"
  xcrun devicectl device copy to --device "$dev" --domain-type appDataContainer --domain-identifier "$XBE_IOS_BUNDLE_ID" \
    --source ".local/ios/$name" --destination "Documents/$name"
  echo "copied $name to the app on ${2}"
  exit 0
fi

cmake -S . -B build/ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 \
  -DXBE_IOS_TEAM="$XBE_IOS_TEAM" -DXBE_IOS_BUNDLE_ID="$XBE_IOS_BUNDLE_ID"
dest="generic/platform=iOS"
if [ "${1:-}" = install ]; then
  dev=$(device_id "${2:?device name or UDID}")
  [ -n "$dev" ] || { echo "no device ${2}" >&2; exit 1; }
  dest="id=$dev"
fi
xcodebuild -project build/ios/game.xcodeproj -scheme PGR1 -configuration Release -destination "$dest" \
  -allowProvisioningUpdates build -quiet
app=build/ios/Release-iphoneos/PGR1.app
echo "built $app"
if [ "${1:-}" = install ]; then
  xcrun devicectl device install app --device "$dev" "$app"
  echo "installed on ${2}"
fi
