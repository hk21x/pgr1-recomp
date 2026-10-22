# Project Gotham Racing recomp - findings

What the work has established, by topic. PROGRESS.md has the dated log with the evidence for each item. Each item
is marked **[console]** (how the Xbox, its XDK or the runtime behaves: the fix is in the SDK) or **[game]** (this
game's own code and data, with its addresses).

## The image
- **[game]** `default.xbe` sections: .text 0x11000-0xE93F0 (game, XAPI, libc), D3D 0xE9940, XGRPH 0xFC620,
  DSOUND 0xFD340, WMADEC, BINK (several), XPP 0x13B080, .rdata 0x142820, .data 0x1563A0, DOLBY. The D3D section
  holds its own data too (device pointer at 0xFC5F0).
- **[game]** No push buffers: no D3DPushBuffer objects are made and the game's code writes no NV2A commands
  (PROGRESS.md, 2026-10-22).

## Data files
- **[game]** GOTHPOK container (INIs, areas, cars, shaders, sounds): "GOTHPOK\x1a", u32 unpacked size at +8, u16 at +0xC
  (0 = stored, data copied), 0xCCCC, u32 at +0x10 kept by the loader but not used to unpack, data at +0x14. Packed data:
  a u32 flag word, then 30 items from its top bit down: 0 = a literal byte, 1 = a u16 match; the word's low 2 bits b
  give length = (w >> (14 - b)) + 3 and distance = (w & (0x3FFF >> b)) + 1. Unpacker FUN_000C99C0, loader
  FUN_000CA2C0 (FileFastOpen, via the z: cache). `scripts/gothpok.py` (one file, or `--tree dir out`).
- **[game]** FUN_00046E00 is the fatal error screen (draws a 1-bit message bitmap, then loops forever); the loader
  calls it when a file can't be opened.
