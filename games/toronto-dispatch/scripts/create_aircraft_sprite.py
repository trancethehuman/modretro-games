#!/usr/bin/env python3
"""Author original ambient plane/helicopter pixels and native import metadata.

Only original-art sources are written unless --sync-native is selected.
Each cardinal aircraft or larger ground jet shadow occupies four
8x16 OAM objects, the stippled shadow two, and startup is an empty metasprite.
--check verifies decoded pixels, palette, object coverage and compiler-safe
canvas coordinates, plus the imported asset and registered native loader slots,
without importing, building or claiming native acceptance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import uuid

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "project/original-art"
PNG = ART / "ambient_aircraft.png"
METADATA = ART / "ambient_aircraft.metadata.json"
MANIFEST = ART / "ambient_aircraft_art.json"
PREVIEW = ART / "ambient_aircraft_preview.png"
TRANSPARENT = (101, 255, 0)
DARK = (7, 24, 33)
MID = (134, 192, 108)
LIGHT = (224, 248, 207)
COLOURS = (TRANSPARENT, LIGHT, MID, DARK)
DIRECTIONS = ("east", "south", "west", "north")
SYMBOL = "sprite_ambient_aircraft"
QUEEN_ID = "028d86c5-aef3-5752-87c9-817b5eb2a2ad"
GAMEPLAY_SCENES = tuple(d["scene"] for d in json.loads((ROOT / "content/districts/world.json").read_text())["districts"])
QUEEN_SCENES = ("toronto_city", "toronto_west", "toronto_east")


def ident(key):
    return str(uuid.uuid5(uuid.NAMESPACE_URL, "toronto-dispatch.ambient-aircraft." + key))


NATIVE_ID = ident("native-sprite")


def digest(data):
    return hashlib.sha256(data).hexdigest()


def plane():
    """East-facing commuter plane: swept wings, tailplane and dark cockpit."""
    image = Image.new("RGB", (32, 32), TRANSPARENT)
    d = ImageDraw.Draw(image)
    # Tail first, then broad wings; a bright fuselage stays readable above them.
    d.polygon([(2, 10), (5, 10), (9, 14), (9, 17), (5, 21), (2, 21),
               (4, 16), (4, 15)], fill=DARK)
    d.polygon([(3, 11), (5, 11), (8, 15), (8, 16), (5, 20), (3, 20),
               (5, 16), (5, 15)], fill=MID)
    d.polygon([(11, 8), (15, 8), (22, 14), (22, 17),
               (15, 23), (11, 23), (14, 17), (14, 14)], fill=DARK)
    d.polygon([(12, 9), (15, 9), (20, 14), (20, 17),
               (15, 22), (12, 22), (15, 17), (15, 14)], fill=LIGHT)
    d.line((13, 10, 17, 13), fill=MID)
    d.line((13, 21, 17, 18), fill=MID)
    for y in (10, 19):
        d.rectangle((17, y, 20, y + 2), fill=DARK)
        d.line((18, y, 19, y), fill=LIGHT)
    d.polygon([(4, 14), (23, 12), (27, 13), (29, 15), (29, 16),
               (27, 18), (23, 19), (4, 17)], fill=DARK)
    d.polygon([(5, 15), (23, 13), (26, 14), (28, 15), (28, 16),
               (26, 17), (23, 18), (5, 16)], fill=LIGHT)
    d.line((9, 17, 22, 17), fill=MID)
    d.rectangle((23, 14, 25, 17), fill=DARK)
    d.line((24, 14, 25, 14), fill=MID)
    d.point((26, 15), fill=LIGHT)
    d.line((5, 15, 9, 15), fill=MID)
    d.point((11, 8), fill=LIGHT)
    d.point((11, 23), fill=MID)
    return image


def helicopter(phase):
    """East-facing cabin and tail, with two deliberately placed rotor poses."""
    image = Image.new("RGB", (32, 32), TRANSPARENT)
    d = ImageDraw.Draw(image)
    d.line((3, 15, 19, 15), fill=DARK, width=4)
    d.line((4, 15, 17, 15), fill=MID, width=2)
    d.line((4, 11, 4, 20), fill=DARK)
    d.point((4, 11), fill=LIGHT)
    d.point((4, 20), fill=LIGHT)
    d.rectangle((17, 11, 25, 20), fill=DARK)
    d.rectangle((18, 12, 25, 19), fill=MID)
    d.polygon([(25, 11), (28, 13), (29, 15), (29, 16),
               (28, 18), (25, 20)], fill=DARK)
    d.polygon([(24, 12), (27, 13), (28, 15), (28, 16),
               (27, 18), (24, 19)], fill=LIGHT)
    d.rectangle((25, 13, 27, 18), fill=DARK)
    d.line((26, 13, 27, 14), fill=LIGHT)
    # Skids use the dark outline; the exposed light rotor contrasts the cabin.
    d.line((17, 10, 27, 10), fill=DARK)
    d.line((17, 21, 27, 21), fill=DARK)
    if phase == 0:
        d.rectangle((18, 8, 19, 23), fill=DARK)
        d.rectangle((7, 14, 30, 15), fill=DARK)
        d.line((18, 9, 18, 22), fill=LIGHT)
        d.line((8, 14, 29, 14), fill=LIGHT)
    else:
        d.polygon([(10, 8), (12, 8), (27, 22), (27, 23),
                   (25, 23), (10, 10)], fill=DARK)
        d.polygon([(25, 8), (27, 8), (27, 10), (12, 23),
                   (10, 23), (10, 22)], fill=DARK)
        d.line((11, 9, 26, 22), fill=LIGHT)
        d.line((11, 22, 26, 9), fill=LIGHT)
    d.rectangle((17, 14, 20, 17), fill=DARK)
    d.rectangle((18, 15, 19, 16), fill=MID)
    d.point((18, 15), fill=LIGHT)
    return image


def shadow():
    image = Image.new("RGB", (32, 32), TRANSPARENT)
    for y in range(12, 20):
        for x in range(8, 24):
            # Pixel-centred 16x8 ellipse; deliberate gaps leave the road visible.
            if (2 * x - 31) ** 2 + 4 * (2 * y - 31) ** 2 <= 256 and (x + y) % 2 == 0:
                image.putpixel((x, y), DARK)
    return image


def jet_shadow():
    """A banked jet shadow: long nose, swept wings and a small tailplane.

    The diagonal wing halves occupy opposite16x16 quadrants. Every exact
    cardinal rotation therefore fits four8x16 objects across a32x32 canvas.
    Stippling lets the road remain visible without altering its background.
    """
    mask = Image.new("1", (32, 32))
    d = ImageDraw.Draw(mask)
    wing = [(8, 0), (11, 0), (15, 11), (15, 15), (8, 14), (9, 10)]
    d.polygon(wing, fill=1)
    d.polygon([(31 - x, 31 - y) for x, y in wing], fill=1)
    d.polygon([(0, 8), (3, 8), (7, 14), (7, 15), (0, 14)], fill=1)
    d.rectangle((0, 14, 15, 15), fill=1)
    d.rectangle((16, 16, 28, 17), fill=1)
    d.polygon([(24, 16), (28, 16), (31, 17), (28, 19), (24, 18)], fill=1)
    image = Image.new("RGB", (32, 32), TRANSPARENT)
    for y in range(32):
        for x in range(32):
            if mask.getpixel((x, y)) and (x + y) % 2 == 0:
                image.putpixel((x, y), DARK)
    return image


def artwork():
    frames = []
    for east in (plane(), helicopter(0), helicopter(1)):
        frames.extend((east, east.transpose(Image.Transpose.ROTATE_270),
                       east.transpose(Image.Transpose.ROTATE_180),
                       east.transpose(Image.Transpose.ROTATE_90)))
    frames.append(shadow())
    frames.append(Image.new("RGB", (32, 32), TRANSPARENT))
    east = jet_shadow()
    frames.extend((east, east.transpose(Image.Transpose.ROTATE_270),
                   east.transpose(Image.Transpose.ROTATE_180),
                   east.transpose(Image.Transpose.ROTATE_90)))
    sheet = Image.new("RGB", (len(frames) * 32, 32), TRANSPARENT)
    for index, image in enumerate(frames):
        sheet.paste(image, (index * 32, 0))
    return sheet, frames


def object_positions(frame):
    # These are the Queen sprite's verified nonnegative optimiser-mask positions.
    if frame == 12:
        return [(x - 8, 8, frame * 32 + x, 8) for x in (8, 16)]
    if frame == 13:
        return []
    if frame >= 14:
        return [(x - 8, 16 - y, frame * 32 + x, y)
                for x, y in zip((0, 8, 16, 24),
                                (0, 0, 16, 16) if (frame - 14) % 2 == 0 else (16, 16, 0, 0))]
    if frame % 4 in (0, 2):
        return [(x - 8, 8, frame * 32 + x, 8) for x in (0, 8, 16, 24)]
    return [(x - 8, 16 - y, frame * 32 + x, y)
            for y in (0, 16) for x in (8, 16)]


def patterns(sheet, height):
    result = set()
    for frame in range(18):
        for _, _, sx, sy in object_positions(frame):
            for offset in range(0, 16, height):
                crop = sheet.crop((sx, sy + offset, sx + 8, sy + offset + height))
                pixels = crop.load()
                result.add(bytes(COLOURS.index(pixels[x, y])
                                 for y in range(crop.height) for x in range(crop.width)))
    return result


def flipped_patterns(sheet, frames):
    """Source cost model after legal8x16 X/Y dedup; compiled gate is final."""
    result = set()
    for frame in frames:
        for _, _, sx, sy in object_positions(frame):
            pixels = tuple(COLOURS.index(sheet.getpixel((sx + x, sy + y)))
                           for y in range(16) for x in range(8))
            result.add(min(bytes(pixels[(15 - y if fy else y) * 8 + (7 - x if fx else x)]
                                 for y in range(16) for x in range(8))
                           for fx in (0, 1) for fy in (0, 1)))
    return result


def metadata(png_bytes, sheet):
    frames = []
    for frame in range(18):
        objects = [{"id": ident(f"frame-{frame}-object-{number}"),
                    "x": x, "y": y, "sliceX": sx, "sliceY": sy,
                    "flipX": False, "flipY": False, "palette": 0,
                    "paletteIndex": 0, "objPalette": "OBP0", "priority": False}
                   for number, (x, y, sx, sy) in enumerate(object_positions(frame))]
        frames.append({"id": ident("hidden-frame") if frame == 13 else ident(f"frame-{frame}"), "tiles": objects})
    animations = [{"id": ident("poses"), "frames": frames}]
    animations += [{"id": ident(f"empty-animation-{n}"),
                    "frames": [{"id": ident(f"empty-frame-{n}"), "tiles": []}]}
                   for n in range(1, 8)]
    return {"_resourceType": "sprite", "id": ident("source-sprite"),
            "name": "Ambient aircraft", "symbol": SYMBOL, "filename": PNG.name,
            "width": sheet.width, "height": sheet.height,
            "checksum": hashlib.sha1(png_bytes).hexdigest(),
            "numTiles": len(patterns(sheet, 16)),
            "canvasOriginX": 8, "canvasOriginY": -8,
            "canvasWidth": 32, "canvasHeight": 32,
            "boundsX": -16, "boundsY": -8, "boundsWidth": 32, "boundsHeight": 16,
            "animSpeed": 255,
            "states": [{"id": ident("state"), "name": "", "animationType": "fixed",
                        "flipLeft": False, "animations": animations}]}


def check_frames(sheet, frames, meta):
    pixels = sheet.load()
    assert {pixels[x, y] for y in range(sheet.height) for x in range(sheet.width)} <= set(COLOURS)
    fixed = meta["states"][0]["animations"][0]["frames"]
    assert len(fixed) == 18 and fixed[13]["tiles"] == []
    assert digest(sheet.crop((0, 0, 13 * 32, 32)).tobytes()) == \
        "bc20efa4a2602f32d958b2b2294f12924fee27fee31aab1a6221da23ace3c589", \
        "Approved original plane/helicopter/shadow pixels changed"
    for index, source in enumerate(frames):
        reconstructed = Image.new("RGB", (32, 32), TRANSPARENT)
        cells = fixed[index]["tiles"]
        assert len(cells) == (0 if index == 13 else 2 if index == 12 else 4)
        for tile in cells:
            assert 0 <= 8 + tile["x"] <= 24 and 0 <= 16 - tile["y"] <= 16
            crop = sheet.crop((tile["sliceX"], tile["sliceY"],
                               tile["sliceX"] + 8, tile["sliceY"] + 16))
            # Actual compiler/OAM offsets with canvasOrigin(8,-8), pivot(16,16).
            reconstructed.paste(crop, (8 + tile["x"], 16 - tile["y"]))
        assert reconstructed.tobytes() == source.tobytes(), f"Pose {index} clips or shifts"
        if index == 12 or index >= 14:
            assert {source.getpixel((x, y)) for y in range(32) for x in range(32)} == {TRANSPARENT, DARK}
        elif 8 <= index < 12:
            assert source.tobytes() != frames[index - 4].tobytes(), "Rotor phases duplicate"
    old_area = sum(frames[12].getpixel((x, y)) != TRANSPARENT for y in range(32) for x in range(32))
    for source in frames[14:]:
        assert sum(source.getpixel((x, y)) != TRANSPARENT for y in range(32) for x in range(32)) > old_area * 2, "Jet must read larger than the old ellipse"
    assert len(flipped_patterns(sheet, range(14, 18)) - flipped_patterns(sheet, range(13))) <= 8, \
        "Jet source cost exceeds eight additional pairs across both OBJ banks"


def encoded(value):
    return (json.dumps(value, indent=2) + "\n").encode()


def manifest(sheet, frames, png_bytes, meta_bytes):
    poses = []
    for index, image in enumerate(frames):
        opaque = [(x, y) for y in range(32) for x in range(32)
                  if image.getpixel((x, y)) != TRANSPARENT]
        bounds = [min(x for x, y in opaque), min(y for x, y in opaque),
                  max(x for x, y in opaque) + 1, max(y for x, y in opaque) + 1] if opaque else []
        poses.append({"frame": index,
                      "kind": "plane" if index < 4 else "helicopter" if index < 12 else "shadow" if index == 12 else "hidden" if index == 13 else "jet_shadow",
                      "direction": DIRECTIONS[(index - 14) % 4] if index >= 14 else DIRECTIONS[index % 4] if index < 12 else None,
                      "rotor_phase": (index - 4) // 4 if 4 <= index < 12 else None,
                      "source_rect": [index * 32, 0, 32, 32], "opaque_bounds": bounds,
                      "oam_objects": 0 if index == 13 else 2 if index == 12 else 4,
                      "maximum_objects_per_scanline": 0 if index == 13 else 2 if index >= 12 or index % 4 in (1, 3) else 4})
    return {"schema_version": 1, "status": "Original source art; native acceptance separate",
            "licence": "MIT", "notice": "Original hand-placed pixels; no photo, logo or sprite copied",
            "generator": "scripts/create_aircraft_sprite.py",
            "native_asset_id": NATIVE_ID, "native_symbol": SYMBOL,
            "source_png": "original-art/ambient_aircraft.png",
            "source_metadata": "original-art/ambient_aircraft.metadata.json",
            "destination": "assets/sprites/ambient_aircraft.png",
            "png_sha256": digest(png_bytes), "metadata_sha256": digest(meta_bytes),
            "decoded_rgb_sha256": digest(sheet.tobytes()), "palette_slot": 0,
            "source_colours": ["65ff00", "e0f8cf", "86c06c", "071821"],
            "unique_patterns_8x8": len(patterns(sheet, 8)),
            "unique_patterns_8x16": len(patterns(sheet, 16)), "poses": poses,
            "unique_flipped_patterns_8x16": len(flipped_patterns(sheet, range(18))),
            "jet_additional_flipped_patterns_8x16": len(flipped_patterns(sheet, range(14, 18)) - flipped_patterns(sheet, range(13))),
            "shadow_frame": 12, "hidden_frame": 13,
            "jet_shadow_frames": [14, 15, 16, 17],
            "original_decoded_rgb_sha256": "bc20efa4a2602f32d958b2b2294f12924fee27fee31aab1a6221da23ace3c589",
            "pivot": "Aircraft share source centre (16,16), mapped to actor world position",
            "runtime": "Original plane/heli overlay and optional ellipse remain exact; larger jet shadows append four ground objects without roof patches; empty startup 13 remains invisible",
            "verification": "Decoded pixels, palette, OAM coverage and optimiser-safe metadata only; no import, build, runtime or hardware claim"}


def preview(frames):
    grid = Image.new("RGB", (4 * 40, 5 * 40), TRANSPARENT)
    for index, frame in enumerate(frames):
        grid.paste(frame, ((index % 4) * 40 + 4, (index // 4) * 40 + 4))
    return grid.resize((640, 800), Image.Resampling.NEAREST)


def normalized_resource(value):
    """Ignore importer-owned identifiers without rewriting registered IDs."""
    if isinstance(value, dict):
        return {key: normalized_resource(item) for key, item in value.items()
                if key not in ("id", "symbol", "name")}
    if isinstance(value, list):
        return [normalized_resource(item) for item in value]
    return value


def check_registered(png_bytes, meta, project=None):
    """Read imported resources and loader ownership; never mutate the project.

    An injectable root permits rejection fixtures in a temporary directory.
    Source/native child IDs intentionally remain different after import.
    """
    project = ROOT / "project" if project is None else Path(project)
    native_path = project / "assets/sprites/ambient_aircraft.png"
    assert native_path.read_bytes() == png_bytes, "Registered aircraft PNG differs from import source"
    native = json.loads(native_path.with_suffix(".png.gbsres").read_text())
    assert native["id"] == NATIVE_ID, "Registered aircraft asset ID changed"
    assert native["symbol"] == SYMBOL, "Registered aircraft native symbol changed"
    assert normalized_resource(native) == normalized_resource(meta), "Registered aircraft frame/object layout differs"
    poses = native["states"][0]["animations"][0]["frames"]
    assert len(poses) == 18 and [len(frame["tiles"]) for frame in poses] == [4] * 12 + [2, 0] + [4] * 4, \
        "Registered aircraft requires original poses/shadow/empty plus four four-object jet shadows"
    loader_ids = set()
    for name in GAMEPLAY_SCENES:
        directory = project / "project/scenes" / name / "actors"
        actor_path = directory / "ambient_aircraft_loader.gbsres"
        loader = json.loads(actor_path.read_text())
        assert loader["_resourceType"] == "actor" and loader["spriteSheetId"] == NATIVE_ID, \
            f"{name}: aircraft loader references the wrong native asset"
        assert loader["frame"] == 13 and not loader["animate"], f"{name}: aircraft loader must start empty at frame13"
        assert loader["_index"] == (1 if name in QUEEN_SCENES else 0), \
            f"{name}: aircraft loader no longer occupies its reserved native slot"
        loader_ids.add(loader["id"])
        actors = [json.loads(path.read_text()) for path in directory.glob("*.gbsres")]
        assert len([actor for actor in actors if actor["spriteSheetId"] == NATIVE_ID]) == 1, \
            f"{name}: aircraft needs exactly one native loader"
        indices = [actor["_index"] for actor in actors]
        assert len(indices) == len(set(indices)), f"{name}: native actor order has duplicate indices"
        queen = [actor for actor in actors if actor["spriteSheetId"] == QUEEN_ID]
        if name not in QUEEN_SCENES:
            assert not queen, f"{name}: no-Queen loader order must be preserved"
        else:
            assert len(queen) == 1 and queen[0]["_index"] == 0, f"{name}: Queen loader must retain native index0"
    assert len(loader_ids) == len(GAMEPLAY_SCENES), "Aircraft loaders must retain unique native actor IDs"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--sync-native", action="store_true", help="Sync approved source pixels/layout while preserving every existing imported frame/object ID")
    args = parser.parse_args()
    sheet, frames = artwork()
    if args.check:
        with Image.open(PNG) as image:
            assert image.size == sheet.size and image.convert("RGB").tobytes() == sheet.tobytes(), "Source pixels differ"
    else:
        ART.mkdir(parents=True, exist_ok=True)
        sheet.save(PNG, compress_level=9)
    png_bytes = PNG.read_bytes()
    meta = metadata(png_bytes, sheet)
    check_frames(sheet, frames, meta)
    meta_bytes = encoded(meta)
    info = manifest(sheet, frames, png_bytes, meta_bytes)
    info_bytes = encoded(info)
    enlarged = preview(frames)
    if args.check:
        assert METADATA.read_bytes() == meta_bytes, "Metadata differs"
        assert MANIFEST.read_bytes() == info_bytes, "Manifest differs"
        with Image.open(PREVIEW) as image:
            assert image.convert("RGB").tobytes() == enlarged.tobytes(), "Source preview differs"
        check_registered(png_bytes, meta)
    else:
        METADATA.write_bytes(meta_bytes)
        MANIFEST.write_bytes(info_bytes)
        enlarged.save(PREVIEW, compress_level=9)
        if args.sync_native:
            native_path = ROOT / "project/assets/sprites/ambient_aircraft.png"
            native = json.loads(native_path.with_suffix(".png.gbsres").read_text())
            fixed = native["states"][0]["animations"][0]["frames"]
            assert len(fixed) in (14, 18), "Sync requires the retained original imported resource"
            native_path.write_bytes(png_bytes)
            for key in ("width", "height", "checksum", "numTiles"):
                native[key] = meta[key]
            fixed[14:] = meta["states"][0]["animations"][0]["frames"][14:]
            native_path.with_suffix(".png.gbsres").write_bytes(encoded(native))
            check_registered(png_bytes, meta)
    print(f"Aircraft: 12 original poses + shadow + empty + 4 jet shadows; 4/2/0 OAM objects; {info['unique_patterns_8x8']} unique 8x8 / {info['unique_patterns_8x16']} unique 8x16 patterns")
    print(f"Native ID: {NATIVE_ID}; PNG SHA-256: {info['png_sha256']}; metadata SHA-256: {info['metadata_sha256']}")


if __name__ == "__main__":
    main()
