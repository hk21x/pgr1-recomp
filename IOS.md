# iPhone and iPad

The iOS port uses the macOS port's code (Metal renderer, SDL window, input and audio) and adds on-screen controls.
It installs on your own device, signed with your own Apple developer team. You cannot distribute it.

## Build and install

1. Build the macOS port first: `./build-macos.sh`. That build runs the recompiler, and the iOS build uses its output.
2. Sign in to your Apple ID in Xcode → Settings → Accounts.
3. Put your team ID and a bundle ID in `.local/ios.env`:
   ```
   XBE_IOS_TEAM=ABCDE12345
   XBE_IOS_BUNDLE_ID=com.yourname.pgr1
   ```
4. Connect the device and turn on Developer Mode. Then run:
   ```
   ./build-ios.sh install "My iPhone"
   ./build-ios.sh disc "My iPhone"
   ```
   `disc` trims the disc image in `assets/` to its game data (about 3.3 GB) and copies it into the app's folder.
   You can also copy a `.iso` into the app's folder with the Files app (On My iPhone → PGR1).

The app needs a device with an A-series or M-series chip that supports BC texture formats. The iPhone 16 Pro Max has
been tested.

## Controls

- Left side: the stick appears where your thumb touches. The d-pad is at the top left.
- Right side: GAS and BRAKE at the bottom; A, B, X and Y above them; W (white) and BK (black) at the top.
- Top: BACK and START. MENU opens the settings window, which you use by tapping.
- A Bluetooth controller also works. The on-screen controls hide while it is connected, and come back when you touch
  the screen.

## Files

The app's folder in the Files app holds the disc image, `settings.toml`, `logs/`, `hdd/` (saves) and
`shadercache-metal.bin`. The first time a scene shows, its shaders compile and the game can pause briefly. Later runs
use the cache.

## What is different on iOS

- The guest's 4 GB space can be at any address, and the second physical memory window is a `vm_remap` of the first,
  because iOS does not let apps use `shm_open`.
- The app requests the extended virtual addressing and increased memory limit entitlements.
- The game stops when the app goes to the background.
