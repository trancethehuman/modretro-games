"""Gate ambient aircraft in a matching native ROM/NOI pair.

Reads the pinned GBVM packed scene/sprite/tileset descriptors, not source tile
estimates. Checks actual frame cells, selected OBJ tile pairs, per-scene OBJ
allocation and bank-1 gameplay BKG separation from scratch IDs32–46. It does not
execute the renderer or prove OAM scanline limits, frame pacing or hardware.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

from check_rom_memory import read_symbols

AIRCRAFT = "_sprite_ambient_aircraft"
COURIER = "_sprite_top_down_vehicles_and_courier"
QUEEN = "_sprite_queen_streetcar"
STREETLIFE = ("_sprite_city_fleet", "_sprite_city_civilians", "_sprite_ambient_boat")
GAME = Path(__file__).resolve().parents[1] / "games/toronto-dispatch"
WORLD = GAME / "content/districts/world.json"
SCENES = tuple("_" + district["symbol"] for district in json.loads(WORLD.read_text())["districts"])
QUEEN_SCENES = {"_scene_toronto_city", "_scene_toronto_west", "_scene_toronto_east"}
NORTH_SCENE = "_scene_toronto_north"
SOURCE_COLOURS = ((101, 255, 0), (224, 248, 207), (134, 192, 108), (7, 24, 33))
CITY_LAYOUTS = (("city_fleet", 17, 2, 16), ("city_civilians", 25, 1, 24),
                ("ambient_boat", 3, 1, 6))


def city_scene_assets(name: str) -> tuple[str, ...]:
    """North is inland: it has fleet/civilians, without a boat loader."""
    return STREETLIFE[:2] if name == NORTH_SCENE else STREETLIFE


def city_loader_assets(name: str) -> list[str]:
    return ([QUEEN] if name in QUEEN_SCENES else []) + [AIRCRAFT] + list(city_scene_assets(name))


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


def compiled_cells(rom: bytes, table: int, frame: int, maximum: int) -> list[tuple[int, int, int, int]]:
    """Read a bounded native near-pointer frame, retaining tile/props ownership."""
    address = int.from_bytes(read(rom, table + frame * 2, 2), "little")
    pointer = (table & 0xFF0000) | address
    cells, x, y = [], 0, 0
    for index in range(maximum + 1):
        dy, dx, tile, props = read(rom, pointer + index * 4, 4)
        if dy == 0x80:
            return cells
        x += dx if dx < 128 else dx - 256
        y += dy if dy < 128 else dy - 256
        cells.append((x, y, tile, props))
    raise ValueError("Compiled frame has no bounded end marker")


def obj_pixels(rom: bytes, tileset: int, tile: int, props: int) -> tuple[int, ...]:
    """Decode the selected 8x16 OBJ pair including compiler X/Y dedup flips."""
    data = read(rom, tileset + 2 + tile * 16, 32)
    rows = [[((data[y * 2] >> (7 - x)) & 1) |
             (((data[y * 2 + 1] >> (7 - x)) & 1) << 1) for x in range(8)] for y in range(16)]
    if props & 0x40:
        rows.reverse()
    if props & 0x20:
        rows = [list(reversed(row)) for row in rows]
    return tuple(pixel for row in rows for pixel in row)


def source_city_poses() -> dict[str, tuple[list[list[tuple]], int]]:
    """Independent original PNG/metadata oracle, not compiled tile estimates."""
    from PIL import Image
    result = {}
    for name, count, objects, maximum in CITY_LAYOUTS:
        art = GAME / "project/original-art"
        meta = json.loads((art / (name + ".metadata.json")).read_text())
        frames = meta["states"][0]["animations"][0]["frames"]
        if meta["symbol"] != "sprite_" + name or len(frames) != count:
            raise ValueError(f"{name}: original native frame identity changed")
        with Image.open(art / (name + ".png")) as source:
            image = source.convert("RGB")
            poses = []
            for index, frame in enumerate(frames):
                if len(frame["tiles"]) != (0 if index == count - 1 else objects):
                    raise ValueError(f"{name}: original visible/empty frame footprint changed")
                cells = []
                for part, tile in enumerate(frame["tiles"]):
                    authored_x = 4 if name == "city_civilians" else part * 8
                    if (tile["x"], tile["y"]) != (authored_x, 0) or tile["priority"]:
                        raise ValueError(f"{name}: original fixed OBJ footprint changed")
                    pixels = [image.getpixel((tile["sliceX"] + x, tile["sliceY"] + y))
                              for y in range(16) for x in range(8)]
                    try:
                        rows = [[SOURCE_COLOURS.index(pixel) for pixel in pixels[y * 8:(y + 1) * 8]]
                                for y in range(16)]
                    except ValueError as error:
                        raise ValueError(f"{name}: unexpected original source colour") from error
                    if tile["flipY"]:
                        rows.reverse()
                    if tile["flipX"]:
                        rows = [list(reversed(row)) for row in rows]
                    cells.append((tile["x"], 0, tile["paletteIndex"],
                                  tuple(pixel for row in rows for pixel in row)))
                # Pinned GB Studio emits cells from right to left. The native
                # cell coordinate identifies that SAME source object.
                cells.sort(key=lambda cell: cell[0], reverse=True)
                poses.append(cells)
        result["_sprite_" + name] = poses, maximum
    return result


def inspect_city_poses(rom: bytes, symbols: dict[str, int], expected: dict,
                       errors: list[str], details: list[str]) -> None:
    for name in STREETLIFE:
        if name not in symbols or name + "_metasprites" not in symbols:
            errors.append(f"{name}: required living-city sprite/frame table is missing")
            continue
        poses, maximum = expected[name]
        descriptor = read(rom, symbols[name], 23)
        table = symbols[name + "_metasprites"]
        if descriptor[0] != len(poses):
            errors.append(f"{name}: {descriptor[0]} compiled frames, expected {len(poses)}")
            continue
        if symbols[name] >> 16 != table >> 16 or int.from_bytes(descriptor[3:5], "little") != table & 0xFFFF:
            errors.append(f"{name}: descriptor does not reference its named native frame table")
            continue
        pointers = far(descriptor[17:20]), far(descriptor[20:23])
        counts = tuple(tiles(rom, pointer) for pointer in pointers)
        if any(not count or count & 1 or count > maximum for count in counts):
            errors.append(f"{name}: OBJ allocations {counts} must be nonzero even counts <= {maximum}")
        boat_pairs = []
        for frame, wanted in enumerate(poses):
            try:
                cells = compiled_cells(rom, table, frame, len(wanted))
            except ValueError as error:
                errors.append(f"{name} frame {frame}: {error}")
                continue
            if [(x, y) for x, y, _, _ in cells] != [(x, y) for x, y, _, _ in wanted]:
                errors.append(f"{name} frame {frame}: clipped/shifted OBJ footprint or nonempty loader")
                continue
            for (x, y, tile, props), (_, _, palette, pixels) in zip(cells, wanted):
                bank = bool(props & 8)
                if tile & 1 or tile + 1 >= counts[bank]:
                    errors.append(f"{name} frame {frame}: invalid selected OBJ bank{int(bank)} pair {tile}/{tile + 1}")
                    continue
                if props & 0x97 != palette:
                    errors.append(f"{name} frame {frame}: native palette/priority/DMG properties differ from source")
                if obj_pixels(rom, pointers[bank], tile, props) != pixels:
                    errors.append(f"{name} frame {frame} at {x},{y}: selected/flipped compiled pixels differ from original pose")
                if name == "_sprite_ambient_boat":
                    boat_pairs.append((int(bank), tile))
        if name == "_sprite_ambient_boat" and (len(boat_pairs) != 2 or len(set(boat_pairs)) != 2):
            errors.append("boat hull and writable reserve must use distinct compiled OBJ pairs")
        details.append(f"{name.removeprefix('_sprite_')}: {len(poses)-1} exact original poses + empty; OBJ banks={counts}")


def inspect_city_loaders(rom: bytes, symbols: dict[str, int], name: str,
                         scene: bytes, errors: list[str]) -> None:
    wanted = city_loader_assets(name)
    if scene[3] != len(wanted):
        errors.append(f"{name}: native actor count does not match captured loader order")
        return
    actors = far(scene[32:35])
    if symbols.get(name + "_actors") != actors:
        errors.append(f"{name}: scene does not reference its named native actor table")
        return
    # Pinned GBVM gbs_types.h actor_t is packed56: frame15, tick18,
    # reserve21, sprite far38, scripts41/44, collision51. The compiler does
    # not initialize actor.frame from the source resource's declared frame;
    # runtime capture/unlink hides these loaders before normal presentation.
    for index, asset in enumerate(wanted):
        actor = read(rom, actors + index * 56, 56)
        if far(actor[38:41]) != symbols.get(asset):
            errors.append(f"{name}: loader {index} owns the wrong native sprite")
        if actor[14:18] != bytes(4) or actor[18] != 255 or actor[21] or any(actor[41:47]) or actor[51]:
            errors.append(f"{name}: loader {index} changed compiler defaults, scripts, reserve or collision group")
    scene_directory = GAME / "project/project/scenes" / name.removeprefix("_scene_")
    source_layouts = CITY_LAYOUTS
    if name == NORTH_SCENE:
        source_layouts = (("ambient_aircraft", 14, 4, 26),) + CITY_LAYOUTS[:2]
        if {path.name for path in (scene_directory / "actors").glob("*.gbsres")} != {
                asset + "_loader.gbsres" for asset, _, _, _ in source_layouts}:
            errors.append(f"{name}: declared North actors must be exactly aircraft/fleet/civilians, without boat or Queen")
    for asset, count, _, _ in source_layouts:
        resource = json.loads((scene_directory / "actors" / (asset + "_loader.gbsres")).read_text())
        native = json.loads((GAME / "project/assets/sprites" / (asset + ".png.gbsres")).read_text())
        index = wanted.index("_sprite_" + asset)
        if resource["_index"] != index or resource["spriteSheetId"] != native["id"] or \
                resource["frame"] != count - 1 or resource["animate"]:
            errors.append(f"{name}: declared {asset} loader no longer selects the verified empty source frame")


def inspect_details(rom: bytes, symbols: dict[str, int], require_streetlife: bool = False) -> tuple[list[str], list[str]]:
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

    city_names = tuple(name for name in STREETLIFE if name in symbols)
    if city_names and len(city_names)!=len(STREETLIFE):
        errors.append("Incomplete compiled living-city sprite set")
    if require_streetlife:
        inspect_city_poses(rom, symbols, source_city_poses(), errors, details)
    named_pointers = {symbols[name]: name for name in (AIRCRAFT, COURIER, QUEEN)+city_names}
    for name in SCENES:
        # Packed scene_t fields: dimensions/type/counts/reserve8, player far8,
        # background far11, sprite list far29. Verified against pinned GBVM.
        scene = read(rom, symbols[name], 35)
        if require_streetlife:
            inspect_city_loaders(rom, symbols, name, scene, errors)
        player, extra = far(scene[8:11]), []
        if player != symbols[COURIER]:
            errors.append(f"{name}: compiled player sprite changed")
        if scene[6]:
            pointer = far(scene[29:32])
            rows = read(rom, pointer, scene[6] * 3)
            extra = [far(rows[index:index + 3]) for index in range(0, len(rows), 3)]
        scene_assets = [player] + extra
        wanted = {symbols[COURIER], symbols[AIRCRAFT]} | {
            symbols[asset] for asset in city_names if asset in city_scene_assets(name)}
        if name in QUEEN_SCENES:
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
        sprites = [symbols[QUEEN], symbols[AIRCRAFT]] if name in QUEEN_SCENES else [symbols[AIRCRAFT]]
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
    if NORTH_SCENE in symbols:
        # Supply the complete global city-art set while omitting the inland
        # boat from North's compiled scene list. Other scenes retain all art.
        for asset in STREETLIFE:
            pair = tile_set(2), tile_set(2)
            descriptor = bytearray(23)
            descriptor[17:20], descriptor[20:23] = fp(pair[0]), fp(pair[1])
            symbols[asset] = put(descriptor)
        for name in SCENES:
            extras = ([QUEEN] if name in QUEEN_SCENES else []) + [AIRCRAFT] + list(
                STREETLIFE[:2] if name == NORTH_SCENE else STREETLIFE)
            pointer = put(b"".join(fp(symbols[asset]) for asset in extras))
            offset = rom_offset(symbols[name], len(rom))
            rom[offset + 6] = len(extras)
            rom[offset + 29:offset + 32] = fp(pointer)
        assert not inspect(bytes(rom), symbols), "valid inland compiled sprite allocation rejected"
        checks += 1
        for extras in ([AIRCRAFT, STREETLIFE[0], STREETLIFE[1], STREETLIFE[2]],
                       [AIRCRAFT, STREETLIFE[0], STREETLIFE[1], QUEEN],
                       [AIRCRAFT, STREETLIFE[1]]):
            pointer = put(b"".join(fp(symbols[asset]) for asset in extras))
            broken = rom.copy();offset = rom_offset(symbols[NORTH_SCENE], len(rom))
            broken[offset + 6] = len(extras)
            broken[offset + 29:offset + 32] = fp(pointer)
            assert inspect(bytes(broken), symbols), "North scene accepted boat/Queen allocation or missing fleet"
            checks += 1
    print(f"PASS: {checks} synthetic compiled-aircraft gate cases")
    city_self_test()
    return 0


def city_self_test() -> None:
    """Asymmetric synthetic pixels detect wrong pose/bank/flip, not just size."""
    rom = bytearray(4 * 0x4000)
    symbols, expected, frame_pointers, allocations = {}, {}, {}, {}
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

    def encode(pattern: tuple[int, ...]) -> bytes:
        return bytes(sum(((pattern[y * 8 + x] >> plane) & 1) << (7 - x) for x in range(8))
                     for y in range(16) for plane in range(2))

    patterns = [tuple((x + y * 2) % 4 for y in range(16) for x in range(8)),
                tuple((x * 3 + y + 1) % 4 for y in range(16) for x in range(8))]
    symbols[AIRCRAFT], symbols[QUEEN] = put(bytes(1)), put(bytes(1))
    for asset, count, objects, maximum in CITY_LAYOUTS:
        name = "_sprite_" + asset
        pair = tuple(put((objects * 2).to_bytes(2, "little") +
                         b"".join(encode(pattern) for pattern in patterns[:objects])) for _ in range(2))
        allocations[name] = pair
        poses, pointers = [], []
        for frame in range(count):
            records, cells = bytearray(), []
            bank = (frame // 2) & 1 if objects == 2 else frame & 1
            flip_x, flip_y = bool(frame & 2), bool(frame & 4)
            palette = (3 + frame // 4 if asset == "city_fleet" else
                       1 + (frame // 6) % 2 if asset == "city_civilians" else 0)
            x = 0
            if frame != count - 1:
                for part in range(objects):
                    xx = (objects - 1 - part) * 8
                    props = bank * 8 + flip_x * 32 + flip_y * 64 + palette
                    records += bytes((0, (xx - x) & 255, part * 2, props))
                    pixels = tuple(patterns[part][(15 - y if flip_y else y) * 8 +
                                                  (7 - px if flip_x else px)]
                                   for y in range(16) for px in range(8))
                    cells.append((xx, 0, palette, pixels))
                    x = xx
            records += b"\x80\0\0\0"
            pointers.append(put(records));poses.append(cells)
        frame_pointers[name] = pointers
        table = put(b"".join((pointer & 0xFFFF).to_bytes(2, "little") for pointer in pointers))
        descriptor = bytearray(23);descriptor[0] = count
        descriptor[3:5] = (table & 0xFFFF).to_bytes(2, "little")
        descriptor[17:20], descriptor[20:23] = fp(pair[0]), fp(pair[1])
        symbols[name], symbols[name + "_metasprites"] = put(descriptor), table
        expected[name] = poses, maximum
    scenes, actor_pointers = {}, {}
    for name in SCENES:
        wanted = city_loader_assets(name)
        actors = bytearray()
        for asset in wanted:
            actor = bytearray(56);actor[18] = 255;actor[38:41] = fp(symbols[asset])
            actors += actor
        pointer = put(actors);actor_pointers[name] = pointer
        scene = bytearray(35);scene[3] = len(wanted);scene[32:35] = fp(pointer)
        scenes[name] = bytes(scene);symbols[name + "_actors"] = pointer

    def check(data: bytes, names: dict, scene_data: dict | None = None) -> list[str]:
        errors, details = [], []
        inspect_city_poses(data, names, expected, errors, details)
        for name, scene in (scenes if scene_data is None else scene_data).items():
            inspect_city_loaders(data, names, name, scene, errors)
        return errors

    assert not check(bytes(rom), symbols), "valid synthetic living-city bytes rejected"
    checks = 1
    corruptions = []
    for name in STREETLIFE:
        for frame in (0, len(expected[name][0]) - 2):
            corruptions.extend(((frame_pointers[name][frame], 1, 7),  # footprint
                                (frame_pointers[name][frame], 2, 1),  # odd pair
                                (frame_pointers[name][frame], 3, 0x80)))  # roof priority
        corruptions.append((frame_pointers[name][-1], 0, 0))  # nonempty loader pose
        original = rom[rom_offset(allocations[name][0], len(rom)) + 2]
        corruptions.append((allocations[name][0], 2, original ^ 1))  # exact source pixels
    corruptions.extend(((frame_pointers[STREETLIFE[0]][3], 3, 3),  # missing flip/bank
                        (frame_pointers[STREETLIFE[1]][6], 3, 1),  # wrong variant palette
                        (frame_pointers[STREETLIFE[2]][1], 3, 0),  # writable hull alias
                        (allocations[STREETLIFE[0]][0], 0, 17),   # allocation limit/evenness
                        (allocations[STREETLIFE[1]][1], 0, 0),    # absent selected bank
                        (actor_pointers[SCENES[0]] + 2 * 56, 38, 0),  # wrong loader owner
                        (actor_pointers[SCENES[0]] + 3 * 56, 41, 1)))  # scripted loader
    for pointer, offset, value in corruptions:
        broken = rom.copy();broken[rom_offset(pointer, len(rom)) + offset] = value
        try:
            errors = check(bytes(broken), symbols)
        except ValueError:
            errors = ["Malformed compiled pointer/resource"]
        assert errors, f"corrupt synthetic living-city data accepted at {pointer:X}+{offset}={value}"
        checks += 1
    broken_symbols = dict(symbols);del broken_symbols[STREETLIFE[0] + "_metasprites"]
    assert check(bytes(rom), broken_symbols), "missing native living-city frame table accepted"
    checks += 1
    if NORTH_SCENE in scenes:
        # Three packed resource actors are distinct from the courier/player.
        # An inland boat must fail both excess-count and substituted-owner cases.
        assert city_loader_assets(NORTH_SCENE) == [AIRCRAFT, STREETLIFE[0], STREETLIFE[1]]
        checks += 1
        for asset in (STREETLIFE[2], QUEEN):
            broken = rom.copy()
            pointer = actor_pointers[NORTH_SCENE] + 2 * 56 + 38
            start = rom_offset(pointer, len(broken))
            broken[start:start + 3] = fp(symbols[asset])
            assert check(bytes(broken), symbols), "North loader accepted a boat or Queen owner"
            checks += 1
        for count in (2, 4):
            changed = dict(scenes)
            scene = bytearray(scenes[NORTH_SCENE]);scene[3] = count
            changed[NORTH_SCENE] = bytes(scene)
            assert check(bytes(rom), symbols, changed), "North loader accepted missing/excess actor count"
            checks += 1
    print(f"PASS: {checks} synthetic compiled living-city pose/loader gate cases")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path, nargs="?")
    parser.add_argument("symbols", type=Path, nargs="?")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--require-streetlife", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        if args.rom or args.symbols:
            parser.error("--self-test takes no ROM/NOI paths")
        return self_test()
    if not args.rom or not args.symbols:
        parser.error("Supply the matching ROM and symbols.noi paths")
    try:
        rom = args.rom.read_bytes()
        symbols = read_symbols(args.symbols.read_text())
        errors, details = inspect_details(rom, symbols, args.require_streetlife)
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
        if args.require_streetlife:
            print("PASS: exact original fleet/civilian/boat OBJ poses, palettes, empty frames and independent boat reserve")
    return bool(errors)


if __name__ == "__main__":
    raise SystemExit(main())
