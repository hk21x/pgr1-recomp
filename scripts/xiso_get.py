#!/usr/bin/env python3
"""Extract one file from the root directory of an Xbox XISO (plain or full redump image).
usage: xiso_get.py <image.iso> <name> <out>   (default.xbe for the build: scripts/xiso_get.py assets/game.iso default.xbe assets/default.xbe)"""
import struct, sys

SECTOR = 2048
MAGIC = b"MICROSOFT*XBOX*MEDIA"


def find_partition(f):
    for off in (0, 0x18300000, 0x2080000, 0xFD90000):  # plain, redump, XGD3, XGD2 variants
        f.seek(off + 32 * SECTOR)
        if f.read(20) == MAGIC:
            return off
    sys.exit("no XISO header found")


def lookup(f, part, sector, size, name):
    f.seek(part + sector * SECTOR)
    table = f.read(size)
    stack = [0]
    while stack:
        pos = stack.pop() * 4
        if pos + 14 > len(table):
            continue
        left, right, start, length, attr, nlen = struct.unpack_from("<HHIIBB", table, pos)
        if left == 0xFFFF:
            continue
        n = table[pos + 14:pos + 14 + nlen].decode("latin-1")
        if n.lower() == name.lower():
            return start, length
        if left:
            stack.append(left)
        if right:
            stack.append(right)
    sys.exit(f"{name} not in root")


def main():
    iso, name, out = sys.argv[1:4]
    with open(iso, "rb") as f:
        part = find_partition(f)
        f.seek(part + 32 * SECTOR + 20)
        root_sector, root_size = struct.unpack("<II", f.read(8))
        start, length = lookup(f, part, root_sector, root_size, name)
        f.seek(part + start * SECTOR)
        with open(out, "wb") as o:
            o.write(f.read(length))
    print(f"partition 0x{part:X}: {name} {length} bytes -> {out}")


main()
