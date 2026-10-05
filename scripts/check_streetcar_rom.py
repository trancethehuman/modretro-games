"""Read the compiled Queen streetcar metasprites from a matching ROM/NOI pair.

This gate catches the real compiler failure where canvas clipping discarded
horizontal objects. It checks compiled data, not playback or physical hardware.
"""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import sys

from check_rom_memory import read_symbols


def rom_offset(symbol: int, size: int) -> int:
    bank, address = symbol >> 16, symbol & 0xFFFF
    if not (bank > 0 and 0x4000 <= address < 0x8000):
        raise ValueError(f"Expected a banked ROM symbol, got {symbol:X}")
    offset = bank * 0x4000 + address - 0x4000
    if offset >= size:
        raise ValueError("Symbol lies outside the supplied ROM")
    return offset


def inspect(rom: bytes, symbols: dict[str, int]) -> list[str]:
    name = "_sprite_queen_streetcar_metasprites"
    if name not in symbols:
        raise ValueError("Queen streetcar table is missing from these symbols")
    table = symbols[name]
    bank = table >> 16
    start = rom_offset(table, len(rom))
    if start + 18 > len(rom):
        raise ValueError("Truncated nine-frame pointer table")
    errors = []
    for frame in range(9):
        address = int.from_bytes(rom[start + frame * 2:start + frame * 2 + 2], "little")
        offset = rom_offset((bank << 16) | address, len(rom))
        x = y = 0
        cells = []
        ended = False
        # Eight objects is a bounded ceiling, not a scan into unrelated data.
        for index in range(9):
            chunk = rom[offset + index * 4:offset + index * 4 + 4]
            if len(chunk) != 4:
                break
            dy, dx, tile, props = chunk
            if dy == 0x80:
                ended = True
                break
            if index == 8:
                break
            y += dy if dy < 128 else dy - 256
            x += dx if dx < 128 else dx - 256
            cells.append((x, y))
            if tile & 1:
                errors.append(f"frame {frame}: odd tile index {tile} in an 8x16 object")
            if props & 7:
                errors.append(f"frame {frame}: unexpected OBJ palette {props & 7}")
        expected = [] if frame == 8 else (
            [(16, 8), (8, 8), (0, 8), (-8, 8)] if frame % 4 in (0, 2)
            else [(8, 16), (0, 16), (8, 0), (0, 0)]
        )
        if not ended:
            errors.append(f"frame {frame}: no bounded end marker")
        if sorted(cells) != sorted(expected):
            errors.append(f"frame {frame}: compiled cells {cells}; expected {expected}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path)
    parser.add_argument("symbols", type=Path)
    args = parser.parse_args()
    try:
        rom = args.rom.read_bytes()
        errors = inspect(rom, read_symbols(args.symbols.read_text()))
    except (OSError, ValueError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    print(f"ROM SHA-256 {hashlib.sha256(rom).hexdigest()}")
    print(f"NOI SHA-256 {hashlib.sha256(args.symbols.read_bytes()).hexdigest()}")
    for error in errors:
        print(f"FAIL: {error}", file=sys.stderr)
    if not errors:
        print("PASS: eight four-object cardinal/door poses and one empty startup frame")
    return bool(errors)


if __name__ == "__main__":
    raise SystemExit(main())
