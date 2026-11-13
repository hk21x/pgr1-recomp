#!/usr/bin/env python3
"""Write the game partition of an Xbox disc image alone, up to its last used sector: a plain XISO, a fraction of a full
redump's size (for a phone). The game opens either.
usage: xiso_trim.py <image.iso> <out.iso>"""
import struct, sys

SECTOR = 2048
MAGIC = b"MICROSOFT*XBOX*MEDIA"


def find_partition(f):
    for off in (0, 0x18300000):  # plain, redump (the offsets the runtime's disc reader takes)
        f.seek(off + 32 * SECTOR)
        if f.read(20) == MAGIC:
            return off
    sys.exit("no XISO header found")


def used_end(f, part, sector, size):
    """the end, in bytes from the partition, of a directory table and everything under it"""
    end = sector * SECTOR + size
    f.seek(part + sector * SECTOR)
    table = f.read(size)
    stack = [0]
    seen = set()
    while stack:
        pos = stack.pop() * 4
        if pos in seen or pos + 14 > len(table):
            continue
        seen.add(pos)
        left, right, start, length, attr, nlen = struct.unpack_from("<HHIIBB", table, pos)
        if left == 0xFFFF:
            continue
        if attr & 0x10:
            if length:
                end = max(end, used_end(f, part, start, length))
        else:
            end = max(end, start * SECTOR + length)
        if left:
            stack.append(left)
        if right:
            stack.append(right)
    return end


def main():
    iso, out = sys.argv[1:3]
    with open(iso, "rb") as f:
        part = find_partition(f)
        f.seek(part + 32 * SECTOR + 20)
        root_sector, root_size = struct.unpack("<II", f.read(8))
        end = used_end(f, part, root_sector, root_size)
        end = max(end, 33 * SECTOR)
        end = (end + SECTOR - 1) // SECTOR * SECTOR
        f.seek(part)
        left = end
        with open(out, "wb") as o:
            while left:
                chunk = f.read(min(left, 64 << 20))
                if not chunk:
                    sys.exit("image ends early")
                o.write(chunk)
                left -= len(chunk)
    print(f"partition 0x{part:X}: {end >> 20} MB -> {out}")


main()
