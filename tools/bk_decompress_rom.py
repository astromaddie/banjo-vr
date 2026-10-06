#!/usr/bin/env python3
"""Decompress the Banjo-Kazooie US 1.0 ROM into the layout BanjoRecomp's N64Recomp config expects.

A Python port of the decomp's own tool (lib/bk-decomp/tools/bk_rom_compressor/src/decomp/main.rs),
so the build needs no Rust toolchain: every overlay (code and data, each a 0x11 0x72 header, a
big-endian size and a raw deflate stream) is inflated and the pieces are written back after the
ROM's uncompressed head in the same order (haunted and desert swap places, as in the original).

    bk_decompress_rom.py <Banjo-Kazooie (USA).z64> banjo.us.v10.decompressed.z64
"""
import hashlib
import sys
import zlib

US_MD5 = "b29599651a13f681c9923d69354bf4a3"

# Start and end of each overlay's compressed code and data, then the end of the last one.
US_OFFSETS = [
    0xF19250, 0xF19250 + 0x1D09B,  # core1
    0xF37F90, 0xF37F90 + 0x64B50,  # core2
    0xFA3FD0, 0xFA3FD0 + 0x1DC6,   # whale
    0xFA5F50, 0xFA5F50 + 0x2D96,   # haunted
    0xFA9150, 0xFA9150 + 0x512E,   # desert
    0xFAE860, 0xFAE860 + 0x328B,   # beach
    0xFB24A0, 0xFB24A0 + 0x1E39,   # jungle
    0xFB44E0, 0xFB44E0 + 0x5130,   # swamp
    0xFB9A30, 0xFB9A30 + 0x4BB2,   # ship
    0xFBEBE0, 0xFBEBE0 + 0x540F,   # snow
    0xFC4810, 0xFC4810 + 0x23FF,   # training
    0xFC6F20, 0xFC6F20 + 0x1BDC,   # intro
    0xFC9150, 0xFC9150 + 0x6548,   # witch
    0xFD0420, 0xFD0420 + 0x5640,   # battle
    0xFD6190, 0xFD6190 + 0x416F,   # tree
    0xFDAA10, 0xFDAA10 + 0xE,      # coshow
    0xFDAA30,
]


def to_big_endian(rom: bytes) -> bytes:
    sig = rom[:4]
    if sig == b"\x80\x37\x12\x40":
        return rom
    if sig == b"\x40\x12\x37\x80":  # little endian (.n64)
        return b"".join(rom[i:i + 4][::-1] for i in range(0, len(rom), 4))
    if sig == b"\x37\x80\x40\x12":  # byte swapped (.v64)
        return b"".join(rom[i:i + 2][::-1] for i in range(0, len(rom), 2))
    raise SystemExit("not an N64 ROM")


def bk_unzip(data: bytes) -> bytes:
    if data[:2] != b"\x11\x72":
        raise SystemExit("overlay without the BK header")
    expected = int.from_bytes(data[2:6], "big")
    out = zlib.decompressobj(wbits=-15).decompress(data[6:])
    if len(out) != expected:
        raise SystemExit(f"overlay inflated to {len(out)} bytes, header says {expected}")
    return out


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    rom = to_big_endian(open(sys.argv[1], "rb").read())
    digest = hashlib.md5(rom).hexdigest()
    if digest != US_MD5:
        raise SystemExit(f"unsupported ROM (md5 {digest}); BanjoRecomp needs the US 1.0 ROM")
    pieces = [bk_unzip(rom[a:b]) for a, b in zip(US_OFFSETS, US_OFFSETS[1:])]
    pieces[6], pieces[8] = pieces[8], pieces[6]
    pieces[7], pieces[9] = pieces[9], pieces[7]
    with open(sys.argv[2], "wb") as out:
        out.write(rom[:US_OFFSETS[0]])
        for piece in pieces:
            out.write(piece)


if __name__ == "__main__":
    main()
