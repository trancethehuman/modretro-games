"""Gate ambient aircraft in a matching native ROM/NOI pair.

Reads the pinned GBVM packed scene/sprite/tileset descriptors, not source tile
estimates. Checks actual frame cells, selected OBJ tile pairs, per-scene OBJ
allocation and bank-1 gameplay BKG separation from scratch IDs64–78. It does not
execute the renderer or prove OAM scanline limits, frame pacing or hardware.
The current living-city gate also requires its restore/post-ground dispatches
to reside in a switchable bank while actors_render stays in fixed bank0.
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
GROUND_SPRITES = (COURIER, QUEEN, STREETLIFE[0], STREETLIFE[1], "_sprite_workshop_courier")
GROUND_BOUNDS = (-8, 16, 0, 16)
GAME = Path(__file__).resolve().parents[1] / "games/toronto-dispatch"
WORLD = GAME / "content/districts/world.json"
SCENES = tuple("_" + district["symbol"] for district in json.loads(WORLD.read_text())["districts"])
QUEEN_SCENES = {"_scene_toronto_city", "_scene_toronto_west", "_scene_toronto_east"}
NORTH_SCENE = "_scene_toronto_north"
# Authored signal offsets contain junctions in all six mainland districts;
# Islands has no traffic lights. IDs47/48 are reserved in their BKG bank1.
SIGNAL_SCENES = set(SCENES) - {"_scene_toronto_islands"}
SOURCE_COLOURS = ((101, 255, 0), (224, 248, 207), (134, 192, 108), (7, 24, 33))
CITY_LAYOUTS = (("city_fleet", 21, 2, 24), ("city_civilians", 25, 1, 24),
                ("ambient_boat", 4, 4, 10))


def city_scene_assets(name: str) -> tuple[str, ...]:
    """North is inland: it has fleet/civilians, without a boat loader."""
    return STREETLIFE[:2] if name == NORTH_SCENE else STREETLIFE


def city_loader_assets(name: str) -> list[str]:
    return ([QUEEN] if name in QUEEN_SCENES else []) + [AIRCRAFT] + list(city_scene_assets(name))


def inspect_actor_banks(rom: bytes, symbols: dict[str, int]) -> tuple[list[str], list[str]]:
    """Check linked locations and GBDK call-bank tag, not source pragmas alone."""
    helpers = ("td_actor_render_before", "td_actor_render_prepare", "td_actor_render_ground", "td_actor_render_after")
    entries = helpers + ("td_actor_render", "td_actor_render_actor")
    required = ("_actors_render", "_td_actor_render_pose", "___sdcc_bcall_ehl", "s__HOME", "l__HOME") + \
        tuple(symbol for name in entries for symbol in ("_" + name, "b_" + name))
    missing = [name for name in required if name not in symbols]
    if missing:
        return ["Required compiled actor dispatch symbols missing: " + ", ".join(missing)], []
    errors, details = [], []
    core = symbols["_actors_render"]
    home_start,home_size = symbols["s__HOME"],symbols["l__HOME"]
    home_end = home_start + home_size
    if not (0 < home_start < 0x4000 and home_size > 0 and home_end <= 0x4000):
        errors.append("Native fixed-bank HOME allocation lies outside bank0")
    elif 0x4000 - home_end < 128:
        errors.append(f"Native fixed-bank HOME headroom {0x4000 - home_end} is below the required128 bytes")
    if not 0 < core < 0x4000:
        errors.append("actors_render must remain in native fixed bank0")
    elif not home_start <= core < home_end:
        errors.append("actors_render lies outside its native fixed-bank HOME allocation")
    pose = symbols["_td_actor_render_pose"]
    if not 0 < pose < 0x4000:
        errors.append("td_actor_render_pose must remain in native fixed bank0")
    elif not home_start <= pose < home_end:
        errors.append("td_actor_render_pose lies outside its native fixed-bank HOME allocation")
    if "b_td_actor_render_pose" in symbols:
        errors.append("NONBANKED td_actor_render_pose must not carry a GBDK call-bank tag")
    locations = []
    for name in entries:
        helper = symbols["_" + name]
        bank, address = helper >> 16, helper & 0xFFFF
        if not (0 < bank <= 255 and 0x4000 <= address < 0x8000):
            errors.append(f"{name} must reside in a native switchable ROM bank")
        elif bank * 0x4000 + address - 0x4000 >= len(rom):
            errors.append(f"{name} lies outside the supplied native ROM")
        if symbols["b_" + name] != bank:
            errors.append(f"{name} GBDK call-bank tag differs from its linked code bank")
        locations.append(f"{name.removeprefix('td_actor_render_')} bank{bank}:{address:04X}")
    bank = symbols["_td_actor_render_ground"] >> 16
    near = ("td_actor_render_local", "td_actor_render_actor_local", "td_actor_render_prepare_local")
    for name in near:
        if "b_" + name in symbols:
            errors.append(f"Private near-call {name} must not carry a GBDK call-bank tag")
    for name in ("td_actor_render", "td_actor_render_actor", "td_actor_render_before", "td_actor_render_prepare"):
        if symbols["_" + name] >> 16 != bank:
            errors.append(f"{name} must share the ground renderer's switchable bank")
    if not errors:
        # Stock NOI excludes static function labels. Derive each private
        # address from its one thin public wrapper, decoding only through the
        # wrapper's unconditional RET/tail JP. No private exports are added.
        near_addresses = {}
        wrappers = (("td_actor_render", near[0]), ("td_actor_render_actor", near[1]),
                    ("td_actor_render_before", near[2]), ("td_actor_render_prepare", near[2]))
        for caller,callee in wrappers:
            pointer = symbols["_" + caller]
            end = min([value for value in symbols.values() if value >> 16 == bank and
                       (pointer & 0xFFFF) < (value & 0xFFFF) < 0x8000] or [(bank << 16) | 0x8000])
            code = read(rom, pointer, (end & 0xFFFF) - (pointer & 0xFFFF))
            transfers = sm83_direct_transfers(code, stop_at_return=True)
            if symbols["___sdcc_bcall_ehl"] in transfers:
                errors.append(f"{caller} still uses a compiled bank-switching trampoline inside the private renderer path")
            if len(transfers) != 1 or not 0x4000 <= transfers[0] < (pointer & 0xFFFF):
                errors.append(f"{caller} lacks its compiled direct near call/jump to a single preceding private body")
                continue
            target = (bank << 16) | transfers[0]
            if callee in near_addresses and near_addresses[callee] != target:
                errors.append("Overlay prepare wrappers differ in their actual private target")
            near_addresses[callee] = target
            if "_" + callee in symbols and symbols["_" + callee] != near_addresses[callee]:
                errors.append(f"Optional private near-call {callee} symbol differs from its actual wrapper target")
        if not errors:
            for caller,callee in (("td_actor_render_actor_local", "td_actor_render_local"),
                                 ("td_actor_render_ground", "td_actor_render_actor_local")):
                pointer = near_addresses.get(caller, symbols.get("_" + caller))
                if caller == "td_actor_render_actor_local":
                    end = symbols["_td_actor_render_actor"]
                else:
                    # Static prepare code follows ground but has no stock NOI
                    # label. Its independently decoded wrappers give the exact
                    # ground boundary; its overlay bank calls are legitimate.
                    end = near_addresses["td_actor_render_prepare_local"]
                    if not pointer < end < symbols["_td_actor_render_before"]:
                        errors.append("Private overlay prepare target does not bound the ground renderer")
                        continue
                code = read(rom, pointer, (end & 0xFFFF) - (pointer & 0xFFFF))
                transfers = sm83_direct_transfers(code)
                if (near_addresses[callee] & 0xFFFF) not in transfers:
                    errors.append(f"{caller} lacks its compiled direct near call/jump to {callee}")
                if symbols["___sdcc_bcall_ehl"] in transfers:
                    errors.append(f"{caller} still uses a compiled bank-switching trampoline inside the private renderer path")
    if not errors:
        details.append(f"actor dispatch: fixed entry bank0:{core:04X}, fixed pose bank0:{pose:04X}, HOME headroom={0x4000-home_end} bytes; " + "; ".join(locations))
    return errors, details


def sm83_direct_transfers(code: bytes, stop_at_return: bool = False) -> list[int]:
    """Decode instruction boundaries; an immediate containing CD is no call.

    Pinned SM83 code may use a tail JP instead of CALL for a thin wrapper.
    This examines each whole linked function, not an unaligned byte search.
    """
    three = {0x01, 0x08, 0x11, 0x21, 0x31, 0xC2, 0xC3, 0xC4, 0xCA, 0xCC,
             0xCD, 0xD2, 0xD4, 0xDA, 0xDC, 0xEA, 0xFA}
    two = {0x06, 0x0E, 0x10, 0x16, 0x18, 0x1E, 0x20, 0x26, 0x28, 0x2E,
           0x30, 0x36, 0x38, 0x3E, 0xCB, 0xC6, 0xCE, 0xD6, 0xDE, 0xE0,
           0xE6, 0xE8, 0xEE, 0xF0, 0xF6, 0xF8, 0xFE}
    offset, targets = 0, []
    while offset < len(code):
        op = code[offset];length = 3 if op in three else 2 if op in two else 1
        if offset + length > len(code):
            raise ValueError("Truncated native renderer instruction at function boundary")
        if op in (0xCD, 0xC3):
            targets.append(int.from_bytes(code[offset + 1:offset + 3], "little"))
        if stop_at_return and op in (0xC9, 0xC3):
            return targets
        offset += length
    if stop_at_return:
        raise ValueError("Native thin renderer wrapper has no unconditional RET/tail JP")
    return targets


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


def inspect_ground_bounds(rom: bytes, symbols: dict[str, int]) -> tuple[list[str], list[str]]:
    """Independently prove every ground frame fits the early OAM-origin cull."""
    errors, count = [], 0
    for name in GROUND_SPRITES:
        if name not in symbols or name + "_metasprites" not in symbols:
            errors.append(f"{name}: missing compiled ground sprite/frame table")
            continue
        descriptor = read(rom, symbols[name], 23)
        table = symbols[name + "_metasprites"]
        if not descriptor[0]:
            errors.append(f"{name}: ground sheet has no compiled frames")
        if symbols[name] >> 16 != table >> 16 or int.from_bytes(descriptor[3:5], "little") != table & 0xFFFF:
            errors.append(f"{name}: ground descriptor does not reference its named compiled frame table")
            continue
        for frame in range(descriptor[0]):
            count += 1
            cells = compiled_cells(rom, table, frame, 8)
            if any(not (GROUND_BOUNDS[0] <= x <= GROUND_BOUNDS[1] and
                        GROUND_BOUNDS[2] <= y <= GROUND_BOUNDS[3]) for x, y, _, _ in cells):
                errors.append(f"{name} frame{frame}: cumulative ground OAM origin exceeds verified visibility bounds{GROUND_BOUNDS}")
    return errors, [f"ground visibility: {count} compiled frames within OAM origins x[-8,16], y[0,16]"] if not errors else []


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
                expected_objects = (4, 3, 4, 0)[index] if name == "ambient_boat" else \
                    (0 if index == count - 1 else objects)
                if len(frame["tiles"]) != expected_objects:
                    raise ValueError(f"{name}: original visible/empty frame footprint changed")
                cells = []
                for part, tile in enumerate(frame["tiles"]):
                    authored_x = 4 if name == "city_civilians" else part * 8
                    authored_y = 0
                    if name == "ambient_boat" and index != 1:
                        authored_x, authored_y = (part % 2) * 8, 16 - (part // 2) * 16
                    if (tile["x"], tile["y"]) != (authored_x, authored_y) or tile["priority"]:
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
                    cells.append((tile["x"] - (meta["canvasOriginX"] - 8),
                                  -tile["y"] - (meta["canvasOriginY"] - 8), tile["paletteIndex"],
                                  tuple(pixel for row in rows for pixel in row)))
                # Pinned GB Studio emits cells from right to left. The native
                # cell coordinate identifies that SAME source object.
                cells.sort(key=lambda cell: cell[0], reverse=True)
                poses.append(cells)
        result["_sprite_" + name] = poses, maximum
    return result


def source_courier_poses() -> dict[str, tuple[list[list[tuple]], int]]:
    """All45 retained native poses plus four one-OBJ original sidearm poses."""
    from PIL import Image
    meta = json.loads((GAME / "project/assets/sprites/dispatch_topdown.png.gbsres").read_text())
    frames = [frame for state in meta["states"] for frame in state["animations"][0]["frames"]]
    if len(frames) != 49 or hashlib.sha256(json.dumps(frames[:45], sort_keys=True,
            separators=(",", ":")).encode()).hexdigest() != \
            "447809e9dae652940d4d86c8feaa421127487e55af8b17431d34663ad2621125":
        raise ValueError("Retained courier frame identities/meanings or appended armed pose count changed")
    poses = []
    with Image.open(GAME / "project/original-art/dispatch_topdown.png") as source:
        image = source.convert("RGB")
        if image.size != (256, 64):
            raise ValueError("Appended courier canvas must be256x64")
        old = b"".join(image.crop(((n % 16)*16, (n // 16)*16, (n % 16+1)*16,
                                  (n // 16+1)*16)).tobytes() for n in range(45))
        if hashlib.sha256(old).hexdigest() != "2d21d95ded6713698785b0e0ba8f7ece1268dda32e05f3983b2e4031c4b1b1aa":
            raise ValueError("Approved courier/vehicle/beacon pixels0..44 changed")
        for index, frame in enumerate(frames):
            cells = []
            if index >= 45 and (len(frame["tiles"]) != 1 or frame["tiles"][0]["x"] != 4):
                raise ValueError("Armed courier requires the unchanged one-OBJ footprint")
            for part in frame["tiles"]:
                pixels = tuple(SOURCE_COLOURS.index(image.getpixel((part["sliceX"]+x, part["sliceY"]+y)))
                               for y in range(16) for x in range(8))
                cells.append((part["x"], -part["y"], part["paletteIndex"], pixels))
            poses.append(cells)
    return {COURIER: (poses, 48)}


def aircraft_cells(frame: int) -> list[tuple[int, int]]:
    """Independent approved native OBJ origins, including staggered jets."""
    if frame == 13:
        return []
    if frame == 12:
        return [(8, 8), (0, 8)]
    if frame >= 14:
        return [(x - 8, y) for x, y in zip((0, 8, 16, 24),
                (0, 0, 16, 16) if (frame - 14) % 2 == 0 else (16, 16, 0, 0))]
    return ([(16, 8), (8, 8), (0, 8), (-8, 8)] if frame % 4 in (0, 2)
            else [(8, 16), (0, 16), (8, 0), (0, 0)])


def source_aircraft_poses() -> dict[str, tuple[list[list[tuple]], int]]:
    """Read every original PNG pixel independently of native frame metadata."""
    from PIL import Image
    with Image.open(GAME / "project/original-art/ambient_aircraft.png") as source:
        image = source.convert("RGB")
        if image.size != (18 * 32, 32) or hashlib.sha256(image.crop((0, 0, 13 * 32, 32)).tobytes()).hexdigest() != \
                "bc20efa4a2602f32d958b2b2294f12924fee27fee31aab1a6221da23ace3c589":
            raise ValueError("Retained original plane/helicopter/ellipse pixels or new jet canvas changed")
        poses = []
        for frame in range(18):
            cells = []
            covered = set()
            for x, y in aircraft_cells(frame):
                pixels = tuple(SOURCE_COLOURS.index(image.getpixel((frame * 32 + x + 8 + dx, y + dy)))
                               for dy in range(16) for dx in range(8))
                cells.append((x, y, 0, pixels))
                covered.update((x + 8 + dx, y + dy) for dy in range(16) for dx in range(8))
            for y in range(32):
                for x in range(32):
                    pixel = image.getpixel((frame * 32 + x, y))
                    if pixel not in SOURCE_COLOURS or (pixel != SOURCE_COLOURS[0] and (x, y) not in covered):
                        raise ValueError(f"Aircraft frame{frame}: source colour/coverage escapes the independent four-object geometry")
                    if frame >= 14 and pixel not in (SOURCE_COLOURS[0], SOURCE_COLOURS[3]):
                        raise ValueError("A jet ground shadow must use only transparent/dark original pixels")
            poses.append(cells)
    #Retained26 raw tiles/bank plus no more than8 new jet patterns per bank.
    return {AIRCRAFT: (poses, 34)}


def inspect_city_poses(rom: bytes, symbols: dict[str, int], expected: dict,
                       errors: list[str], details: list[str]) -> None:
    for name in expected:
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
        boat_hull_pairs, boat_scratch_pairs = set(), []
        for frame, wanted in enumerate(poses):
            try:
                cells = compiled_cells(rom, table, frame, len(wanted))
            except ValueError as error:
                errors.append(f"{name} frame {frame}: {error}")
                continue
            expected_cells = {(x, y): (palette, pixels) for x, y, palette, pixels in wanted}
            if len(expected_cells) != len(wanted) or \
                    sorted((x, y) for x, y, _, _ in cells) != sorted(expected_cells):
                errors.append(f"{name} frame {frame}: clipped/shifted OBJ footprint or nonempty loader")
                continue
            for x, y, tile, props in cells:
                palette, pixels = expected_cells[(x, y)]
                bank = bool(props & 8)
                if tile & 1 or tile + 1 >= counts[bank]:
                    errors.append(f"{name} frame {frame}: invalid selected OBJ bank{int(bank)} pair {tile}/{tile + 1}")
                    continue
                if props & 0x97 != palette:
                    errors.append(f"{name} frame {frame}: native palette/priority/DMG properties differ from source")
                if obj_pixels(rom, pointers[bank], tile, props) != pixels:
                    errors.append(f"{name} frame {frame} at {x},{y}: selected/flipped compiled pixels differ from original pose")
                if name == "_sprite_ambient_boat":
                    pair = (int(bank), tile)
                    if frame == 2:
                        boat_scratch_pairs.append(pair)
                    elif frame < 2:
                        boat_hull_pairs.add(pair)
        if name == "_sprite_ambient_boat" and \
                (len(boat_scratch_pairs) != 4 or len(set(boat_scratch_pairs)) != 4 or
                 boat_hull_pairs.intersection(boat_scratch_pairs)):
            errors.append("boat must own four unique writable OBJ pairs disjoint from every hull pair")
        details.append(f"{name.removeprefix('_sprite_')}: {sum(bool(pose) for pose in poses)} exact original poses + {sum(not pose for pose in poses)} empty; OBJ banks={counts}")


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
        source_layouts = (("ambient_aircraft", 18, 4, 34),) + CITY_LAYOUTS[:2]
        if {path.name for path in (scene_directory / "actors").glob("*.gbsres")} != {
                asset + "_loader.gbsres" for asset, _, _, _ in source_layouts}:
            errors.append(f"{name}: declared North actors must be exactly aircraft/fleet/civilians, without boat or Queen")
    for asset, count, _, _ in source_layouts:
        resource = json.loads((scene_directory / "actors" / (asset + "_loader.gbsres")).read_text())
        native = json.loads((GAME / "project/assets/sprites" / (asset + ".png.gbsres")).read_text())
        index = wanted.index("_sprite_" + asset)
        if resource["_index"] != index or resource["spriteSheetId"] != native["id"] or \
                resource["frame"] != (13 if asset == "ambient_aircraft" else count - 1) or resource["animate"]:
            errors.append(f"{name}: declared {asset} loader no longer selects the verified empty source frame")


def inspect_details(rom: bytes, symbols: dict[str, int], require_streetlife: bool = False, require_combat: bool = False) -> tuple[list[str], list[str]]:
    required = (AIRCRAFT, AIRCRAFT + "_metasprites", COURIER, QUEEN) + SCENES
    missing = [name for name in required if name not in symbols]
    if missing:
        raise ValueError("Required compiled native symbols missing: " + ", ".join(missing))
    errors, details = [], []
    if require_streetlife:
        bank_errors, bank_details = inspect_actor_banks(rom, symbols)
        errors.extend(bank_errors)
        details.extend(bank_details)
        bounds_errors, bounds_details = inspect_ground_bounds(rom, symbols)
        errors.extend(bounds_errors)
        details.extend(bounds_details)
    asset = read(rom, symbols[AIRCRAFT], 23)
    table = symbols[AIRCRAFT + "_metasprites"]
    bank = table >> 16
    if asset[0] != 18:
        errors.append(f"aircraft descriptor has {asset[0]} frames, expected 18")
    if (symbols[AIRCRAFT] >> 16) != bank or int.from_bytes(asset[3:5], "little") != (table & 0xFFFF):
        errors.append("aircraft descriptor does not reference its named compiled frame table")
    counts = sprite_tiles(rom, symbols[AIRCRAFT])
    for index, count in enumerate(counts):
        if not count or count & 1 or count > 128:
            errors.append(f"aircraft OBJ bank{index} allocation {count} is not a nonzero even count <=128")
    details.append(f"aircraft tiles: OBJ bank0={counts[0]}, bank1={counts[1]}")
    if any(count > 34 for count in counts):
        errors.append(f"aircraft OBJ allocations {counts} exceed retained26 + jet8 raw tiles/bank")
    pointers = read(rom, table, 36)
    for frame in range(18):
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
        expected = aircraft_cells(frame)
        if not ended:
            errors.append(f"frame {frame}: no bounded end marker")
        if sorted(cells) != sorted(expected):
            errors.append(f"frame {frame}: compiled cells {cells}; expected {expected}")

    city_names = tuple(name for name in STREETLIFE if name in symbols)
    if city_names and len(city_names)!=len(STREETLIFE):
        errors.append("Incomplete compiled living-city sprite set")
    if require_streetlife:
        inspect_city_poses(rom, symbols, source_city_poses(), errors, details)
        inspect_city_poses(rom, symbols, source_aircraft_poses(), errors, details)
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
        if name in SIGNAL_SCENES and bkg1 > 47:
            errors.append(f"{name}: bank-1 gameplay BKG count {bkg1} overlaps traffic-light scratch IDs47–48")
        if bkg1 > 64:
            errors.append(f"{name}: bank-1 gameplay BKG count {bkg1} overlaps aircraft scratch IDs64–78")
        if bkg1 > 80:
            errors.append(f"{name}: bank-1 gameplay BKG count {bkg1} overlaps scenery scratch IDs80–97")
        # UI font192..240 and guidance241..252 remain outside all gameplay
        # scratch owners. Paused atlas16..187 intentionally reuses gameplay
        # tiles; suppression/restoration has separate actual-renderer tests.
        if bkg1 > 192:
            errors.append(f"{name}: bank-1 gameplay BKG count {bkg1} overlaps UI font/guidance IDs192–252")
        labels = ", ".join(named_pointers.get(pointer, f"{pointer:X}").removeprefix("_sprite_")
                           for pointer in scene_assets)
        details.append(f"{name.removeprefix('_scene_')}: OBJ allocation={total}/128; bank-1 BKG={bkg1}/{47 if name in SIGNAL_SCENES else 64}; {labels}")
    if require_combat:
        inspect_city_poses(rom, symbols, source_courier_poses(), errors, details)
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
    for frame in range(18):
        cells = aircraft_cells(frame)
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
        descriptor[0] = 18
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
        (frame_pointers[14], 1, 15),           # shifted ground jet
        (frame_pointers[15], 3, 0x80),         # wrong jet road priority
        (allocations[AIRCRAFT][0], 0, 128),    # combined OBJ overflow
        (bkg1, 0, 66),                        # scratch collision
    ):
        broken = rom.copy();broken[rom_offset(pointer, len(rom)) + relative] = value
        assert inspect(bytes(broken), symbols), "malformed synthetic compiled data was accepted"
        checks += 1
    #47 allocated background tiles end at46;48 collides with the first
    # actual light pattern while remaining below BOTH air64 and scenery80.
    bkg_offset = rom_offset(bkg1, len(rom))
    safe = rom.copy();safe[bkg_offset:bkg_offset + 2] = (47).to_bytes(2, "little")
    assert not inspect(bytes(safe), symbols), "exact last safe signal-background tile rejected"
    checks += 1
    broken = rom.copy();broken[bkg_offset:bkg_offset + 2] = (48).to_bytes(2, "little")
    errors = inspect(bytes(broken), symbols)
    assert errors and all("traffic-light scratch" in error for error in errors), \
        "first light alias must fail independently of aircraft/scenery/UI limits"
    assert len(errors) == len(SIGNAL_SCENES), "light alias must cover every signal-bearing district"
    checks += 1
    island = "_scene_toronto_islands"
    if island in symbols:
        #Isolate the no-signal island background so stricter mainland lights
        # cannot accidentally mask the independent air/props/UI thresholds.
        island_bkg = tile_set(48)
        island_background = bytearray(14);island_background[5:8] = fp(island_bkg)
        island_pointer = put(island_background)
        scene_offset = rom_offset(symbols[island], len(rom))
        isolated = rom.copy();isolated[scene_offset + 11:scene_offset + 14] = fp(island_pointer)
        assert not inspect(bytes(isolated), symbols), "no-signal island incorrectly inherits mainland light slots"
        checks += 1
        for count, wanted in ((81, "scenery scratch"), (193, "UI font/guidance")):
            aliased = isolated.copy();offset = rom_offset(island_bkg, len(rom))
            aliased[offset:offset + 2] = count.to_bytes(2, "little")
            errors = inspect(bytes(aliased), symbols)
            assert any(wanted in error for error in errors) and \
                not any("traffic-light scratch" in error for error in errors), \
                "distinct scenery/UI ownership alias was not identified"
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
    actor_bank_self_test()
    ground_bounds_self_test()
    city_self_test()
    aircraft_pixel_self_test()
    return 0


def aircraft_pixel_self_test() -> None:
    """Original pixel negatives prove jet/source/bank/flip allocation checks."""
    expected = source_aircraft_poses()
    wanted, _ = expected[AIRCRAFT]
    rom = bytearray(4 * 0x4000)
    cursor, patterns, selected = 0x4000, {}, []

    def put(data: bytes) -> int:
        nonlocal cursor
        pointer = 0x10000 | cursor
        offset = rom_offset(pointer, len(rom))
        rom[offset:offset + len(data)] = data
        cursor += len(data)
        return pointer

    def fp(pointer: int) -> bytes:
        return bytes((pointer >> 16,)) + (pointer & 0xFFFF).to_bytes(2, "little")

    def encode(pixels: tuple[int, ...]) -> bytes:
        return bytes(sum(((pixels[y * 8 + x] >> plane) & 1) << (7 - x) for x in range(8))
                     for y in range(16) for plane in range(2))

    for cells in wanted:
        objects = []
        for x, y, palette, pixels in cells:
            key, fx, fy = min((tuple(pixels[(15 - row if fy else row) * 8 +
                                                   (7 - column if fx else column)]
                                    for row in range(16) for column in range(8)), fx, fy)
                              for fx in (0, 1) for fy in (0, 1))
            if key not in patterns:
                patterns[key] = len(patterns)
            index = patterns[key]
            objects.append((x, y, (index // 2) * 2, palette | (index & 1) * 8 | fx * 32 | fy * 64))
        selected.append(objects)
    groups = [b"".join(encode(pixels) for pixels, index in patterns.items() if index % 2 == bank)
              for bank in (0, 1)]
    allocations = [put((len(group) // 16).to_bytes(2, "little") + group) for group in groups]
    frame_pointers = []
    for objects in selected:
        records = bytearray();x = y = 0
        for xx, yy, tile, props in objects:
            records += bytes(((yy - y) & 255, (xx - x) & 255, tile, props));x, y = xx, yy
        frame_pointers.append(put(records + b"\x80\0\0\0"))
    table = put(b"".join((pointer & 0xFFFF).to_bytes(2, "little") for pointer in frame_pointers))
    descriptor = bytearray(23);descriptor[0] = 18
    descriptor[3:5] = (table & 0xFFFF).to_bytes(2, "little")
    descriptor[17:20], descriptor[20:23] = fp(allocations[0]), fp(allocations[1])
    symbol = put(descriptor)
    symbols = {AIRCRAFT: symbol, AIRCRAFT + "_metasprites": table}

    def check(data: bytes) -> list[str]:
        errors = []
        inspect_city_poses(data, symbols, expected, errors, [])
        return errors

    assert not check(bytes(rom)), "valid exact original/new jet pixels rejected"
    checks = 1
    jet = rom_offset(frame_pointers[14], len(rom))
    mutations = [(jet + 1, rom[jet + 1] ^ 1),
                 (jet + 3, rom[jet + 3] ^ 0x20),
                 (jet + 3, rom[jet + 3] ^ 8),
                 (jet + 3, rom[jet + 3] | 0x80),
                 (jet + 3, rom[jet + 3] | 1),
                 (rom_offset(frame_pointers[13], len(rom)), 0),
                 (rom_offset(symbol, len(rom)), 17),
                 (rom_offset(allocations[0], len(rom)), 36)]
    for offset, value in mutations:
        broken = rom.copy();broken[offset] = value
        assert check(bytes(broken)), "malformed compiled jet/source allocation accepted"
        checks += 1
    bank = bool(rom[jet + 3] & 8)
    offset = rom_offset(allocations[bank], len(rom)) + 2 + rom[jet + 2] * 16
    broken = rom.copy();broken[offset] ^= 0x80
    assert any("compiled pixels differ" in error for error in check(bytes(broken))), \
        "jet tile-byte mutation escaped the exact decoded pixel oracle"
    checks += 1
    print(f"PASS: {checks} synthetic exact aircraft/jet pixel allocation cases")


def actor_bank_self_test() -> None:
    """Independent negatives catch a missing helper, fixed-bank drift and tag drift."""
    rom = bytearray(8 * 0x4000)
    symbols = {"_actors_render": 0x1B20, "_td_actor_render_pose": 0x2200, "_td_actor_render_after": 0x34800,
               "b_td_actor_render_after": 3, "_td_actor_render_before": 0x34700,
               "b_td_actor_render_before": 3, "_td_actor_render_prepare": 0x34880,
               "b_td_actor_render_prepare": 3, "_td_actor_render_ground": 0x34600,
               "b_td_actor_render_ground": 3, "s__HOME": 0x1599, "l__HOME": 0x2900,
               "_td_actor_render_local": 0x34200, "_td_actor_render_actor_local": 0x34400,
               "_td_actor_render": 0x34300, "b_td_actor_render": 3,
               "_td_actor_render_actor": 0x34500, "b_td_actor_render_actor": 3,
               "_td_actor_render_prepare_local": 0x34680,
               "_synthetic_actor_end": 0x34900, "___sdcc_bcall_ehl": 0x3D23}
    calls = (("td_actor_render", "td_actor_render_local"),
             ("td_actor_render_actor", "td_actor_render_actor_local"),
             ("td_actor_render_actor_local", "td_actor_render_local"),
             ("td_actor_render_ground", "td_actor_render_actor_local"),
             ("td_actor_render_before", "td_actor_render_prepare_local"),
             ("td_actor_render_prepare", "td_actor_render_prepare_local"))
    for caller,callee in calls:
        offset = rom_offset(symbols["_" + caller], len(rom))
        rom[offset:offset+4] = bytes([0xCD]) + (symbols["_" + callee] & 0xFFFF).to_bytes(2, "little") + bytes([0xC9])
    assert not inspect_actor_banks(rom, symbols)[0], "valid compiled actor dispatch rejected"
    checks = 1
    stripped = {key:value for key,value in symbols.items() if key not in ("_td_actor_render_local", "_td_actor_render_actor_local", "_td_actor_render_prepare_local")}
    assert not inspect_actor_banks(rom, stripped)[0], "stock NOI without static private labels rejected"
    checks += 1
    boundary = dict(symbols);boundary["l__HOME"] = 0x3F80 - boundary["s__HOME"]
    assert not inspect_actor_banks(rom, boundary)[0], "exact128-byte fixed-bank headroom rejected"
    checks += 1
    for address in (symbols["s__HOME"], symbols["s__HOME"] + symbols["l__HOME"] - 1):
        boundary = dict(symbols);boundary["_td_actor_render_pose"] = address
        assert not inspect_actor_banks(rom, boundary)[0], "inclusive first/last fixed-HOME pose address rejected"
        checks += 1
    negatives = [("_actors_render", 0x14000, "fixed bank0"),
                 ("_actors_render", 0x1598, "outside its native fixed-bank HOME"),
                 ("l__HOME", 0x3F81 - symbols["s__HOME"], "below the required128"),
                 ("l__HOME", 0x4001 - symbols["s__HOME"], "outside bank0"),
                 ("s__HOME", None, "symbols missing"),
                 ("_td_actor_render_pose", None, "symbols missing"),
                 ("_td_actor_render_pose", 0x14000, "fixed bank0"),
                 ("_td_actor_render_pose", 0x1598, "outside its native fixed-bank HOME"),
                 ("_td_actor_render_pose", symbols["s__HOME"] + symbols["l__HOME"], "outside its native fixed-bank HOME"),
                 ("b_td_actor_render_pose", 0, "must not carry a GBDK call-bank tag"),
                 ("b_td_actor_render_pose", 3, "must not carry a GBDK call-bank tag")]
    for helper in ("td_actor_render_before", "td_actor_render_prepare", "td_actor_render_ground", "td_actor_render_after", "td_actor_render", "td_actor_render_actor"):
        negatives.extend((("_" + helper, None, "symbols missing"),
                          ("b_" + helper, None, "symbols missing"),
                          ("_" + helper, 0x1B20, "switchable ROM bank"),
                          ("b_" + helper, 4, "call-bank tag"),
                          ("_" + helper, 0x84000, "outside the supplied native ROM")))
    for helper in ("td_actor_render_local", "td_actor_render_actor_local", "td_actor_render_prepare_local"):
        negatives.extend((("_" + helper, 0x44000, "differs from its actual wrapper target"),
                          ("b_" + helper, 0, "must not carry a GBDK call-bank tag"),
                          ("b_" + helper, 3, "must not carry a GBDK call-bank tag")))
    for name, value, expected in negatives:
        broken = dict(symbols)
        if value is None:
            del broken[name]
        else:
            broken[name] = value
        assert any(expected in error for error in inspect_actor_banks(rom, broken)[0]), \
            "compiled actor dispatch bank drift accepted"
        checks += 1
    for caller,callee in calls:
        offset = rom_offset(symbols["_" + caller], len(rom))
        address = (symbols["_" + callee] & 0xFFFF).to_bytes(2, "little")
        changed = rom.copy();changed[offset] = 0xC3
        assert not inspect_actor_banks(changed, symbols)[0], "native direct tail jump rejected"
        checks += 1
        for replacement,expected in (
            (bytes([0x00,0x00,0x00,0xC9]), "lacks its compiled direct near"),
            (bytes([0xCD,0x23,0x3D,0xC9]), "bank-switching trampoline"),
            (bytes([0x01,0xCD,address[0],address[1],0xC9]), "lacks its compiled direct near"),
            (bytes([0xCD])+address+bytes([0xCD,0x23,0x3D,0xC9]), "bank-switching trampoline")):
            changed = rom.copy();changed[offset:offset+len(replacement)] = replacement
            assert any(expected in error for error in inspect_actor_banks(changed, symbols)[0]), \
                "missing/banked/unaligned/additional renderer transfer accepted"
            checks += 1
    changed = rom.copy(); offset = rom_offset(symbols["_td_actor_render_prepare_local"], len(rom))
    changed[offset:offset+4] = bytes([0xCD,0x23,0x3D,0xC9])
    assert not inspect_actor_banks(changed, stripped)[0], "adjacent private overlay bank call included in ground"
    checks += 1
    # An actual ground transfer before the verified boundary still fails.
    offset = rom_offset(symbols["_td_actor_render_ground"], len(rom)) + 8
    changed[offset:offset+4] = bytes([0xCD,0x23,0x3D,0xC9])
    assert any("bank-switching trampoline" in error for error in inspect_actor_banks(changed, stripped)[0]), \
        "actual ground bank call escaped the exact boundary"
    checks += 1
    print(f"PASS: {checks} synthetic compiled actor-dispatch bank cases")


def ground_bounds_self_test() -> None:
    """Boundary cells and distinct malformed frames test the compiled proof."""
    rom = bytearray(2 * 0x4000)
    pointer = 0x14000
    symbols, poses = {}, []
    for name in GROUND_SPRITES:
        offset = rom_offset(pointer, len(rom))
        pose = pointer
        rom[offset:offset + 12] = bytes((0, 248, 0, 0, 16, 24, 2, 0, 128, 0, 0, 0))
        table = pointer + 12
        rom[offset + 12:offset + 14] = (pose & 0xFFFF).to_bytes(2, "little")
        descriptor = bytearray(23);descriptor[0] = 1;descriptor[3:5] = (table & 0xFFFF).to_bytes(2, "little")
        rom[offset + 14:offset + 37] = descriptor
        symbols[name], symbols[name + "_metasprites"] = pointer + 14, table
        poses.append(pose);pointer += 40
    assert not inspect_ground_bounds(bytes(rom), symbols)[0], "exact compiled ground bounds rejected"
    checks = 1
    for byte, value in ((1, 247), (5, 25), (0, 255), (4, 17)):
        broken = rom.copy();broken[rom_offset(poses[0], len(rom)) + byte] = value
        assert any("visibility bounds" in error for error in inspect_ground_bounds(bytes(broken), symbols)[0]), \
            "out-of-bounds cumulative ground frame accepted"
        checks += 1
    missing = dict(symbols);del missing[GROUND_SPRITES[-1] + "_metasprites"]
    assert inspect_ground_bounds(bytes(rom), missing)[0], "indoor keeper omitted from visibility proof"
    checks += 1
    broken = rom.copy();offset = rom_offset(symbols[GROUND_SPRITES[0]], len(rom));broken[offset] = 0
    assert inspect_ground_bounds(bytes(broken), symbols)[0], "zero-frame ground sheet accepted"
    checks += 1
    broken = rom.copy();broken[offset + 3] ^= 1
    assert inspect_ground_bounds(bytes(broken), symbols)[0], "aliased ground frame table accepted"
    checks += 1
    print(f"PASS: {checks} synthetic compiled ground-visibility cases")


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
                tuple((x * 3 + y + 1) % 4 for y in range(16) for x in range(8)),
                tuple((x * 2 + y * 3 + y // 3 + 1) % 4 for y in range(16) for x in range(8)),
                tuple((x * 3 + y * 2 + x // 3 + y // 5 + 2) % 4 for y in range(16) for x in range(8))]
    symbols[AIRCRAFT], symbols[QUEEN] = put(bytes(1)), put(bytes(1))
    for asset, count, objects, maximum in CITY_LAYOUTS:
        name = "_sprite_" + asset
        pair = tuple(put((objects * 2).to_bytes(2, "little") +
                         b"".join(encode(pattern) for pattern in patterns[:objects])) for _ in range(2))
        allocations[name] = pair
        poses, pointers = [], []
        for frame in range(count):
            records, cells = bytearray(), []
            # Four north hull cells share three pairs with the east hull;
            # four writable scratch pairs occupy the other bank. Identical
            # source patterns in both banks make alias negatives prove pair
            # ownership even when the decoded pixels still match exactly.
            bank = (1 if frame == 2 else 0) if asset == "ambient_boat" else \
                ((frame // 2) & 1 if objects == 2 else frame & 1)
            flip_x, flip_y = bool(frame & 2), bool(frame & 4)
            palette = (3 + min(frame // 4, 3) if asset == "city_fleet" else
                       1 + (frame // 6) % 2 if asset == "city_civilians" else 0)
            x = y = 0
            if frame != count - 1:
                positions = ([(-8, -24), (0, -24), (-8, -8), (0, -8)] if frame != 1 else
                             [(-8 + part * 8, -8) for part in range(3)]) \
                    if asset == "ambient_boat" else [(4 if asset == "city_civilians" else part * 8, 0)
                                                    for part in range(objects)]
                for part in reversed(range(len(positions))):
                    xx, yy = positions[part]
                    props = bank * 8 + flip_x * 32 + flip_y * 64 + palette
                    records += bytes(((yy - y) & 255, (xx - x) & 255, part * 2, props))
                    pixels = tuple(patterns[part][(15 - y if flip_y else y) * 8 +
                                                  (7 - px if flip_x else px)]
                                   for y in range(16) for px in range(8))
                    cells.append((xx, yy, palette, pixels))
                    x, y = xx, yy
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
                        (frame_pointers[STREETLIFE[0]][16], 3, 5),  # taxi must retain gold palette6
                        (frame_pointers[STREETLIFE[1]][6], 3, 1),  # wrong variant palette
                        (allocations[STREETLIFE[0]][0], 0, 26),   # fleet exceeds24 per bank
                        (allocations[STREETLIFE[1]][1], 0, 0),    # absent selected bank
                        (actor_pointers[SCENES[0]] + 2 * 56, 38, 0),  # wrong loader owner
                        (actor_pointers[SCENES[0]] + 3 * 56, 41, 1)))  # scripted loader
    boat_scratch = frame_pointers[STREETLIFE[2]][2]
    corruptions.extend((boat_scratch, part * 4 + 3, 32) for part in range(4))  # clear bank bit only: all four hull aliases
    corruptions.extend(((frame_pointers[STREETLIFE[2]][0], 0, 0),  # lose north second row
                        (frame_pointers[STREETLIFE[2]][1], 8, 0x80),  # east pose loses third object
                        (boat_scratch, 12, 0x80),  # scratch reserve loses fourth object
                        (allocations[STREETLIFE[2]][1], 0, 12)))  # boat bank exceeds10 tiles
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
    parser.add_argument("--require-combat", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        if args.rom or args.symbols:
            parser.error("--self-test takes no ROM/NOI paths")
        return self_test()
    if not args.rom or not args.symbols:
        parser.error("Supply the matching ROM and symbols.noi paths")
    try:
        if args.require_streetlife:
            header = (GAME / "project/plugins/toronto-driving/engine/include/td_actor_render.h").read_text()
            for name, value in zip(("MIN_X", "MAX_X", "MIN_Y", "MAX_Y"), GROUND_BOUNDS):
                if f"#define TD_GROUND_{name} {f'({value})' if value < 0 else value}" not in header:
                    raise ValueError("Native ground visibility constants differ from the independent compiled bound proof")
        rom = args.rom.read_bytes()
        symbols = read_symbols(args.symbols.read_text())
        errors, details = inspect_details(rom, symbols, args.require_streetlife or args.require_combat, args.require_combat)
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
        print("PASS: original twelve aircraft poses, ellipse/empty, four large ground jet shadows, compiled tile pairs and scene allocations")
        if args.require_streetlife:
            print("PASS: exact original fleet/civilian/boat OBJ poses, palettes, empty frames and independent boat reserve")
    if not errors and args.require_combat:
        print("PASS: all45 preserved courier poses and four exact one-OBJ armed poses within scene allocations<=128")
    return bool(errors)


if __name__ == "__main__":
    raise SystemExit(main())
