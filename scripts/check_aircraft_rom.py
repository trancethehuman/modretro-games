"""Gate ambient aircraft in a matching native ROM/NOI pair.

Reads the pinned GBVM packed scene/sprite/tileset descriptors, not source tile
estimates. Checks actual frame cells, selected OBJ tile pairs, per-scene OBJ
allocation and bank-1 gameplay BKG separation from scratch IDs32–46. It does not
execute the renderer or prove OAM scanline limits, frame pacing or hardware.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import sys

from check_rom_memory import read_symbols

AIRCRAFT = "_sprite_ambient_aircraft"
COURIER = "_sprite_top_down_vehicles_and_courier"
QUEEN = "_sprite_queen_streetcar"
SCENES = ("_scene_toronto_city", "_scene_toronto_west",
          "_scene_toronto_high_park", "_scene_toronto_east")


def rom_offset(symbol: int, size: int) -> int:
    bank, address = symbol >> 16, symbol & 0xFFFF
    if not (bank > 0 and 0x4000 <= address < 0x8000):
        raise ValueError(f"Expected a banked ROM symbol, got {symbol:X}")
    offset = bank * 0x4000 + address - 0x4000
    if offset >= size:
        raise ValueError("Symbol lies outside supplied ROM")
    return offset


def read(rom: bytes, symbol: int, length: int) -> bytes:
    if length < 0 or (symbol & 0xFFFF) + length > 0x8000:
        raise ValueError("Resource crosses its native ROM bank boundary")
    start = rom_offset(symbol, len(rom))
    result = rom[start:start + length]
    if len(result) != length:
        raise ValueError("Truncated compiled resource")
    return result


def far(data: bytes) -> int:
    if len(data) != 3:
        raise ValueError("Truncated native far pointer")
    bank, address = data[0], int.from_bytes(data[1:], "little")
    if bank == address == 0:
        return 0
    if not bank or not 0x4000 <= address < 0x8000:
        raise ValueError("Invalid compiled native far pointer")
    return bank << 16 | address


def tiles(rom: bytes, pointer: int) -> int:
    if not pointer:
        return 0
    count = int.from_bytes(read(rom, pointer, 2), "little")
    if count > 255:
        raise ValueError("Compiled tileset exceeds pinned GBVM 8-bit load count")
    read(rom, pointer + 2, count * 16)
    return count


def sprite_tiles(rom: bytes, pointer: int) -> tuple[int, int]:
    # Packed spritesheet_t: n_frames1/emote2/near tables6/bounds8/far tiles3+3.
    descriptor = read(rom, pointer, 23)
    return tiles(rom, far(descriptor[17:20])), tiles(rom, far(descriptor[20:23]))


def inspect_details(rom: bytes, symbols: dict[str, int]) -> tuple[list[str], list[str]]:
    required = (AIRCRAFT, AIRCRAFT + "_metasprites", COURIER, QUEEN) + SCENES
    missing = [name for name in required if name not in symbols]
    if missing:
        raise ValueError("Required compiled native symbols missing: " + ", ".join(missing))
    errors, details = [], []
    asset = read(rom, symbols[AIRCRAFT], 23)
    table = symbols[AIRCRAFT + "_metasprites"]
    bank = table >> 16
    if asset[0] != 14:
        errors.append(f"aircraft descriptor has {asset[0]} frames, expected 14")
    if (symbols[AIRCRAFT] >> 16) != bank or int.from_bytes(asset[3:5], "little") != (table & 0xFFFF):
        errors.append("aircraft descriptor does not reference its named compiled frame table")
    counts = sprite_tiles(rom, symbols[AIRCRAFT])
    for index, count in enumerate(counts):
        if not count or count & 1 or count > 128:
            errors.append(f"aircraft OBJ bank{index} allocation {count} is not a nonzero even count <=128")
    details.append(f"aircraft tiles: OBJ bank0={counts[0]}, bank1={counts[1]}")
    pointers = read(rom, table, 28)
    for frame in range(14):
        address = int.from_bytes(pointers[frame * 2:frame * 2 + 2], "little")
        pointer = bank << 16 | address
        x = y = 0
        cells, ended = [], False
        # Six records is a bounded ceiling, not a scan through unrelated data.
        for index in range(6):
            dy, dx, tile, props = read(rom, pointer + index * 4, 4)
            if dy == 0x80:
                ended = True
                break
            y += dy if dy < 128 else dy - 256
            x += dx if dx < 128 else dx - 256
            cells.append((x, y))
            if tile & 1:
                errors.append(f"frame {frame}: odd 8x16 OBJ tile index {tile}")
            tile_bank = bool(props & 8)
            if tile + 1 >= counts[tile_bank]:
                errors.append(f"frame {frame}: tile pair {tile}/{tile + 1} exceeds OBJ bank{int(tile_bank)} count {counts[tile_bank]}")
            if props & ~0x68:
                errors.append(f"frame {frame}: unexpected OBJ properties {props:02X}")
        expected = [] if frame == 13 else ([(8, 8), (0, 8)] if frame == 12 else (
            [(16, 8), (8, 8), (0, 8), (-8, 8)] if frame % 4 in (0, 2)
            else [(8, 16), (0, 16), (8, 0), (0, 0)]))
        if not ended:
            errors.append(f"frame {frame}: no bounded end marker")
        if sorted(cells) != sorted(expected):
            errors.append(f"frame {frame}: compiled cells {cells}; expected {expected}")

    named_pointers = {symbols[name]: name for name in (AIRCRAFT, COURIER, QUEEN)}
    for name in SCENES:
        # Packed scene_t fields: dimensions/type/counts/reserve8, player far8,
        # background far11, sprite list far29. Verified against pinned GBVM.
        scene = read(rom, symbols[name], 35)
        player, extra = far(scene[8:11]), []
        if player != symbols[COURIER]:
            errors.append(f"{name}: compiled player sprite changed")
        if scene[6]:
            pointer = far(scene[29:32])
            rows = read(rom, pointer, scene[6] * 3)
            extra = [far(rows[index:index + 3]) for index in range(0, len(rows), 3)]
        scene_assets = [player] + extra
        wanted = {symbols[COURIER], symbols[AIRCRAFT]}
        if name != "_scene_toronto_high_park":
            wanted.add(symbols[QUEEN])
        if set(scene_assets) != wanted or len(scene_assets) != len(wanted):
            errors.append(f"{name}: expected one compiled instance of each city sprite asset")
        total = 0
        for index, pointer in enumerate(scene_assets):
            allocations = sprite_tiles(rom, pointer)
            if any(value & 1 for value in allocations):
                errors.append(f"{name}: odd 8x16 asset allocation at {pointer:X}")
            allocation = max(allocations)
            if index == 0:
                allocation = max(allocation, scene[7])
            total += allocation
        if total > 128:
            errors.append(f"{name}: combined OBJ allocation {total} exceeds 128 tiles per bank")
        background = read(rom, far(scene[11:14]), 14)
        bkg1 = tiles(rom, far(background[5:8]))
        if bkg1 > 32:
            errors.append(f"{name}: bank-1 gameplay BKG count {bkg1} overlaps aircraft scratch IDs32–46")
        labels = ", ".join(named_pointers.get(pointer, f"{pointer:X}").removeprefix("_sprite_")
                           for pointer in scene_assets)
        details.append(f"{name.removeprefix('_scene_')}: OBJ allocation={total}/128; bank-1 BKG={bkg1}/32; {labels}")
    return errors, details


def inspect(rom: bytes, symbols: dict[str, int]) -> list[str]:
    return inspect_details(rom, symbols)[0]


def self_test() -> int:
    """Synthetic compiled bytes test the gate's rejection paths, not game art."""
    rom = bytearray(8 * 0x4000)
    symbols = {}
    cursor = 0x4000

    def put(data: bytes) -> int:
        nonlocal cursor
        pointer = 0x10000 | cursor
        offset = rom_offset(pointer, len(rom))
        rom[offset:offset + len(data)] = data
        cursor += len(data)
        return pointer

    def fp(pointer: int) -> bytes:
        return bytes((pointer >> 16,)) + (pointer & 0xFFFF).to_bytes(2, "little")

    def tile_set(count: int) -> int:
        return put(count.to_bytes(2, "little") + bytes(count * 16))

    frame_pointers = []
    for frame in range(14):
        cells = [] if frame == 13 else ([(8, 8), (0, 8)] if frame == 12 else (
            [(16, 8), (8, 8), (0, 8), (-8, 8)] if frame % 4 in (0, 2)
            else [(8, 16), (0, 16), (8, 0), (0, 0)]))
        records = bytearray();x = y = 0
        for xx, yy in cells:
            records += bytes(((yy - y) & 255, (xx - x) & 255, 0, 0));x, y = xx, yy
        records += b"\x80\0\0\0"
        frame_pointers.append(put(records))
    symbols[AIRCRAFT + "_metasprites"] = put(b"".join((p & 0xFFFF).to_bytes(2, "little") for p in frame_pointers))
    allocations = {}
    for name, count in ((AIRCRAFT, 26), (COURIER, 56), (QUEEN, 10)):
        pair = tile_set(count), tile_set(count)
        allocations[name] = pair
        descriptor = bytearray(23)
        descriptor[0] = 14
        descriptor[3:5] = (symbols[AIRCRAFT + "_metasprites"] & 0xFFFF).to_bytes(2, "little")
        descriptor[17:20], descriptor[20:23] = fp(pair[0]), fp(pair[1])
        symbols[name] = put(descriptor)
    bkg1 = tile_set(16)
    background = bytearray(14);background[5:8] = fp(bkg1)
    background_pointer = put(background)
    for name in SCENES:
        sprites = [symbols[AIRCRAFT]] if "high_park" in name else [symbols[QUEEN], symbols[AIRCRAFT]]
        list_pointer = put(b"".join(fp(p) for p in sprites))
        scene = bytearray(35);scene[6] = len(sprites)
        scene[8:11], scene[11:14], scene[29:32] = fp(symbols[COURIER]), fp(background_pointer), fp(list_pointer)
        symbols[name] = put(scene)
    assert not inspect(bytes(rom), symbols), "valid synthetic compiled resources were rejected"
    checks = 1
    for pointer, relative, value in (
        (frame_pointers[0], 1, 15),             # clipped/shifted horizontal cell
        (frame_pointers[7], 2, 1),             # odd 8x16 tile index
        (frame_pointers[12], 8, 0),            # extra shadow object
        (frame_pointers[13], 0, 0),            # nonempty startup
        (frame_pointers[4], 2, 26),            # bank0 tile pair beyond allocation
        (frame_pointers[4], 3, 0x80),          # aircraft hidden beneath roof
        (allocations[AIRCRAFT][0], 0, 128),    # combined OBJ overflow
        (bkg1, 0, 34),                        # scratch collision
    ):
        broken = rom.copy();broken[rom_offset(pointer, len(rom)) + relative] = value
        assert inspect(bytes(broken), symbols), "malformed synthetic compiled data was accepted"
        checks += 1
    print(f"PASS: {checks} synthetic compiled-aircraft gate cases")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path, nargs="?")
    parser.add_argument("symbols", type=Path, nargs="?")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        if args.rom or args.symbols:
            parser.error("--self-test takes no ROM/NOI paths")
        return self_test()
    if not args.rom or not args.symbols:
        parser.error("Supply the matching ROM and symbols.noi paths")
    try:
        rom = args.rom.read_bytes()
        errors, details = inspect_details(rom, read_symbols(args.symbols.read_text()))
    except (OSError, ValueError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    print(f"ROM SHA-256 {hashlib.sha256(rom).hexdigest()}")
    print(f"NOI SHA-256 {hashlib.sha256(args.symbols.read_bytes()).hexdigest()}")
    for detail in details:
        print(detail)
    for error in errors:
        print(f"FAIL: {error}", file=sys.stderr)
    if not errors:
        print("PASS: twelve four-object aircraft poses, two-object shadow, empty startup, compiled tile pairs and scene allocations")
    return bool(errors)


if __name__ == "__main__":
    raise SystemExit(main())
