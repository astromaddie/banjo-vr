#!/usr/bin/env python3
"""Validate the user's US 1.0 ROM, measure Banjo, optionally decompress overlays.

No assets are extracted to disk. Overlay placement follows bk_rom_compressor's
src/decomp/main.rs (USA offsets and the beach/desert pair swap).
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

US_SHA1 = '1fe1632098865f639e22c11b9a81ee8f29c75d7a'
OVERLAYS = [
    (0xF19250, 0x1D09B), (0xF37F90, 0x64B50), (0xFA3FD0, 0x1DC6),
    (0xFA5F50, 0x2D96), (0xFA9150, 0x512E), (0xFAE860, 0x328B),
    (0xFB24A0, 0x1E39), (0xFB44E0, 0x5130), (0xFB9A30, 0x4BB2),
    (0xFBEBE0, 0x540F), (0xFC4810, 0x23FF), (0xFC6F20, 0x1BDC),
    (0xFC9150, 0x6548), (0xFD0420, 0x5640), (0xFD6190, 0x416F),
    (0xFDAA10, 0xE),
]


def unzip(data):
    assert data[:2] == b'\x11\x72', 'invalid RareZip header'
    out = zlib.decompress(data[6:], -15)
    assert len(out) == struct.unpack_from('>I', data, 2)[0], 'RareZip size mismatch'
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    parser.add_argument('--decompress', type=Path)
    args = parser.parse_args()
    rom = args.rom.read_bytes()
    assert hashlib.sha1(rom).hexdigest() == US_SHA1, 'requires big-endian US 1.0 ROM'
    table = 0x5E90
    count = struct.unpack_from('>I', rom, table)[0]
    base = table + 8 + count * 8
    models = []
    for asset in (0x34D, 0x34E):
        off, compressed, _ = struct.unpack_from('>IHH', rom, table + 8 + asset * 8)
        end = struct.unpack_from('>I', rom, table + 8 + (asset + 1) * 8)[0]
        data = rom[base + off:base + end]
        if compressed:
            data = unzip(data)
        vertex_offset = struct.unpack_from('>I', data, 0x10)[0]
        header = struct.unpack_from('>12h', data, vertex_offset)
        vertices = [struct.unpack_from('>3h', data, vertex_offset + 24 + i * 16)
                    for i in range(header[10])]
        low = [min(v[j] for v in vertices) for j in range(3)]
        high = [max(v[j] for v in vertices) for j in range(3)]
        assert low == list(header[:3]) and high == list(header[3:6])
        models.append(dict(asset=hex(asset), vertices=len(vertices), minimum=low,
                           maximum=high, height_units=high[1] - low[1]))
    print(json.dumps(dict(rom_sha1=US_SHA1, models=models), indent=2))
    if args.decompress:
        assert not args.decompress.exists(), 'refusing to overwrite an existing ROM'
        offsets = [v for off, size in OVERLAYS for v in (off, off + size)] + [0xFDAA30]
        blocks = [unzip(rom[a:b]) for a, b in zip(offsets, offsets[1:])]
        blocks[6], blocks[8] = blocks[8], blocks[6]
        blocks[7], blocks[9] = blocks[9], blocks[7]
        with args.decompress.open('xb') as out:
            out.write(rom[:offsets[0]])
            for block in blocks:
                out.write(block)
        print('Decompressed ROM:', args.decompress.resolve())


if __name__ == '__main__':
    main()
