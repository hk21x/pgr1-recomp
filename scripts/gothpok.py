#!/usr/bin/env python3
# Unpacks the game's GOTHPOK files (INIs, areas, cars) for analysis; the game's own unpacker is FUN_000c99c0.
#   python3 scripts/gothpok.py <in> [out]       one file (out defaults to stdout)
#   python3 scripts/gothpok.py --tree <dir> <outdir>   every GOTHPOK file under dir, same relative paths
# Header: "GOTHPOK\x1a", u32 unpacked size, u16 packed (0 = stored), u16 0xCCCC, u32 (unused here), data at 0x14.
# Data: a u32 flag word, then 30 items read from its top bit down (1 = match, 0 = literal byte); the word's low
# 2 bits b split each u16 match into length (top 2+b bits, +3) and distance (low 14-b bits, +1).
import struct
import sys
from pathlib import Path

MAGIC = b'GOTHPOK\x1a'


def unpack(d):
    if d[:8] != MAGIC:
        raise ValueError('not a GOTHPOK file')
    size, packed = struct.unpack_from('<IH', d, 8)
    if not packed:
        return d[0x14:0x14 + size]
    out, i = bytearray(), 0x14
    while True:
        flags = struct.unpack_from('<I', d, i)[0]
        b = flags & 3
        i += 4
        for _ in range(30):
            if flags & 0x80000000:
                w = struct.unpack_from('<H', d, i)[0]
                i += 2
                n, dist = (w >> (14 - b)) + 3, (w & (0x3FFF >> b)) + 1
                if dist > len(out) or len(out) + n > size:
                    raise ValueError('corrupt input at %#x' % i)
                for _ in range(n):
                    out.append(out[-dist])
            else:
                out.append(d[i])
                i += 1
            if len(out) >= size:
                return bytes(out)
            flags = (flags << 1) & 0xFFFFFFFF


if __name__ == '__main__':
    if sys.argv[1] == '--tree':
        src, dst = Path(sys.argv[2]), Path(sys.argv[3])
        n = bad = 0
        for f in sorted(p for p in src.rglob('*') if p.is_file()):
            d = f.read_bytes()
            if d[:8] != MAGIC:
                continue
            try:
                o = dst / f.relative_to(src)
                o.parent.mkdir(parents=True, exist_ok=True)
                o.write_bytes(unpack(d))
                n += 1
            except Exception as e:
                print(f, e)
                bad += 1
        print('%d unpacked, %d failed' % (n, bad))
    else:
        data = unpack(Path(sys.argv[1]).read_bytes())
        if len(sys.argv) > 2:
            Path(sys.argv[2]).write_bytes(data)
        else:
            sys.stdout.buffer.write(data)
