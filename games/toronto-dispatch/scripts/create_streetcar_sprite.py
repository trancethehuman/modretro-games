#!/usr/bin/env python3
"""Author the original Queen tram PNG/native-metadata import pair.

This deliberately placed pixel art uses the existing project's four sprite
source colours and palette slot 0. It copies no TTC artwork or branding.
Only original-art sources are written; the public native_metadata importer
registers the finished pair. --check reads sources and optional imported output
without changing them. Decoded-pixel comparison keeps the check portable across
PNG encoder versions. This is authored-source evidence, not native gameplay.
"""
import argparse
import hashlib
import json
from pathlib import Path
import uuid

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "project"
ART = PROJECT / "original-art"
PNG = ART / "queen_streetcar.png"
METADATA = ART / "queen_streetcar.metadata.json"
MANIFEST = ART / "queen_streetcar_art.json"
NATIVE = PROJECT / "assets/sprites/queen_streetcar.png"
TRANSPARENT = (101, 255, 0)
DARK = (7, 24, 33)
MID = (134, 192, 108)
LIGHT = (224, 248, 207)
COLOURS = (TRANSPARENT, LIGHT, MID, DARK)
POSES = ("east", "south", "west", "north")


def ident(key):
    return str(uuid.uuid5(uuid.NAMESPACE_URL, "toronto-dispatch.queen-streetcar." + key))


NATIVE_ID = ident("native-sprite")


def digest(data):
    return hashlib.sha256(data).hexdigest()


def east_pose(doors):
    """A 28x12 body, three roof sections and an original right-side door."""
    image = Image.new("RGB", (32, 32), TRANSPARENT)
    d = ImageDraw.Draw(image)
    d.rectangle((2, 10, 29, 21), fill=DARK)
    d.rectangle((3, 11, 28, 20), fill=MID)
    # Cab glazing and cream roof distinguish this long vehicle from the cars.
    d.rectangle((5, 12, 25, 19), fill=LIGHT)
    d.rectangle((26, 12, 28, 18), fill=DARK)
    d.line((27, 12, 27, 18), fill=LIGHT)
    d.rectangle((3, 13, 4, 18), fill=DARK)
    d.point((3, 12), fill=LIGHT)
    d.point((3, 19), fill=LIGHT)
    # Articulated roof seam and two equipment housings; no letters or logos.
    d.line((13, 11, 13, 20), fill=DARK)
    d.line((14, 12, 14, 19), fill=MID)
    for x in (7, 18):
        d.rectangle((x, 14, x + 3, 17), fill=MID)
        d.line((x, 14, x + 3, 14), fill=DARK)
    for x in (6, 9, 16, 23):
        d.point((x, 11), fill=DARK)
        d.point((x, 20), fill=DARK)
    if doors:
        d.rectangle((18, 18, 21, 21), fill=DARK)
        d.line((18, 18, 18, 20), fill=LIGHT)
        d.line((19, 21, 21, 21), fill=LIGHT)
    else:
        d.line((20, 18, 20, 20), fill=MID)
        d.point((20, 20), fill=LIGHT)
    return image


def artwork():
    sheet = Image.new("RGB", (256, 32), TRANSPARENT)
    frames = []
    for doors in (False, True):
        east = east_pose(doors)
        images = (east, east.transpose(Image.Transpose.ROTATE_270),
                  east.transpose(Image.Transpose.ROTATE_180),
                  east.transpose(Image.Transpose.ROTATE_90))
        for image in images:
            frames.append(image)
            sheet.paste(image, ((len(frames) - 1) * 32, 0))
    return sheet, frames


def object_positions(frame):
    # Editor coordinates must also fit optimiseTiles' canvas: its origin is
    # (width/2-8,height-16), independently of the chosen actor anchor. Use
    # canvasOrigin(8,-8) below to place that visible canvas around actor.pos.
    # The compiler and hardware OAM offsets then preserve the source centre.
    if frame % 4 in (0, 2):
        return [(x - 8, 8, frame * 32 + x, 8) for x in (0, 8, 16, 24)]
    return [(x - 8, 16 - y, frame * 32 + x, y)
            for y in (0, 16) for x in (8, 16)]


def patterns(sheet, height):
    result = set()
    for frame in range(8):
        for _, _, sx, sy in object_positions(frame):
            for offset in range(0, 16, height):
                crop = sheet.crop((sx, sy + offset, sx + 8, sy + offset + height))
                pixels = crop.load()
                result.add(bytes(COLOURS.index(pixels[x, y])
                                 for y in range(crop.height) for x in range(crop.width)))
    return result


def metadata(png_bytes, sheet):
    frames = []
    for frame in range(8):
        objects = [{"id": ident(f"frame-{frame}-object-{number}"),
                    "x": x, "y": y, "sliceX": sx, "sliceY": sy,
                    "flipX": False, "flipY": False, "palette": 0,
                    "paletteIndex": 0, "objPalette": "OBP0", "priority": False}
                   for number, (x, y, sx, sy) in enumerate(object_positions(frame))]
        frames.append({"id": ident(f"frame-{frame}"), "tiles": objects})
    # Fixed sprites compile animation 0 only. Keep a genuine empty metasprite
    # after the eight moving poses so runtime startup can select frame 8 safely.
    frames.append({"id": ident("hidden-frame"), "tiles": []})
    animations = [{"id": ident("poses"), "frames": frames}]
    animations += [{"id": ident(f"empty-animation-{n}"),
                    "frames": [{"id": ident(f"empty-frame-{n}"), "tiles": []}]}
                   for n in range(1, 8)]
    return {"_resourceType": "sprite", "id": ident("source-sprite"),
            "name": "Queen streetcar", "symbol": "sprite_queen_streetcar",
            "filename": PNG.name, "width": sheet.width, "height": sheet.height,
            "checksum": hashlib.sha1(png_bytes).hexdigest(),
            "numTiles": len(patterns(sheet, 16)),
            "canvasOriginX": 8, "canvasOriginY": -8,
            "canvasWidth": 32, "canvasHeight": 32,
            "boundsX": -14, "boundsY": -6, "boundsWidth": 28, "boundsHeight": 12,
            "animSpeed": 255,
            "states": [{"id": ident("state"), "name": "", "animationType": "fixed",
                        "flipLeft": False, "animations": animations}]}


def manifest(sheet, frames, png_bytes, meta_bytes):
    rows = []
    for index, image in enumerate(frames):
        pixels = image.load()
        opaque = [(x, y) for y in range(32) for x in range(32)
                  if pixels[x, y] != TRANSPARENT]
        bounds = [min(x for x, y in opaque), min(y for x, y in opaque),
                  max(x for x, y in opaque) + 1, max(y for x, y in opaque) + 1]
        expected = [2, 10, 30, 22] if index % 4 in (0, 2) else [10, 2, 22, 30]
        assert bounds == expected, (index, bounds)
        rows.append({"frame": index, "direction": POSES[index % 4],
                     "doors_open": index >= 4, "source_rect": [index * 32, 0, 32, 32],
                     "opaque_bounds": bounds, "oam_objects": 4,
                     "maximum_objects_per_scanline": 4 if index % 4 in (0, 2) else 2})
    count8, count16 = len(patterns(sheet, 8)), len(patterns(sheet, 16))
    assert count8 <= 64 and count16 <= 32
    pixels = sheet.load()
    assert {pixels[x, y] for y in range(sheet.height)
            for x in range(sheet.width)} <= set(COLOURS)
    return {"schema_version": 1, "status": "Original authored sprite; native acceptance separate",
            "licence": "MIT", "notice": "Original game artwork; no TTC logo, photo or sprite copied",
            "generator": "scripts/create_streetcar_sprite.py",
            "native_asset_id": NATIVE_ID, "native_symbol": "sprite_queen_streetcar",
            "source_png": "original-art/queen_streetcar.png",
            "source_metadata": "original-art/queen_streetcar.metadata.json",
            "destination": "assets/sprites/queen_streetcar.png",
            "png_sha256": digest(png_bytes), "metadata_sha256": digest(meta_bytes),
            "decoded_rgb_sha256": digest(sheet.tobytes()),
            "palette_slot": 0, "source_colours": ["65ff00", "e0f8cf", "86c06c", "071821"],
            "reference_cells_8x8": 64, "unique_patterns_8x8": count8,
            "unique_patterns_8x16": count16, "poses": rows,
            "hidden_frame": 8,
            "loader_initial_visibility": "Compiler loads authored actor inactive; runtime must set hidden/frame 8 explicitly before activation",
            "pivot": "Actor world position is body centre; four poses share that centre",
            "collision": "Default horizontal 28x12; runtime must select vertical 12x28 for south/north",
            "verification": "Source pixels, metadata and OAM object counts only; no build/runtime/hardware claim"}


def encoded(value):
    return (json.dumps(value, indent=2) + "\n").encode()


def check_native_pivots(sheet, frames, meta):
    """Check optimiser canvas clipping, compiler offsets and OAM origins.

    This is source verification of centred body pixels, not a ROM execution.
    Actor position is translated to (16,16) for comparison with each source.
    """
    fixed_x, fixed_y = meta["canvasOriginX"] - 8, meta["canvasOriginY"] - 8
    mask_x, mask_y = meta["canvasWidth"] // 2 - 8, meta["canvasHeight"] - 16
    for index, frame in enumerate(meta["states"][0]["animations"][0]["frames"][:8]):
        rendered = Image.new("RGB", (32, 32), TRANSPARENT)
        for tile in frame["tiles"]:
            # A tile outside this mask can be discarded even if the eventual
            # compiled OAM coordinate would have been correct. That caused
            # the first native east/west frames to compile as empty sprites.
            assert 0 <= mask_x + tile["x"] <= meta["canvasWidth"] - 8
            assert 0 <= mask_y - tile["y"] <= meta["canvasHeight"] - 16
            crop = sheet.crop((tile["sliceX"], tile["sliceY"],
                               tile["sliceX"] + 8, tile["sliceY"] + 16))
            x = 16 + tile["x"] - fixed_x - 8
            y = 16 - tile["y"] - fixed_y - 16
            rendered.paste(crop, (x, y))
        assert rendered.tobytes() == frames[index].tobytes(), f"Native pivot differs in pose {index}"
    assert meta["states"][0]["animations"][0]["frames"][8]["tiles"] == []


def normalized_resource(value):
    if isinstance(value, dict):
        return {k: normalized_resource(v) for k, v in value.items()
                if k not in ("id", "symbol", "name")}
    if isinstance(value, list):
        return [normalized_resource(v) for v in value]
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    sheet, frames = artwork()
    if args.check:
        with Image.open(PNG) as existing:
            assert existing.size == sheet.size and existing.convert("RGB").tobytes() == sheet.tobytes(), "Source pixels differ"
    else:
        ART.mkdir(parents=True, exist_ok=True)
        sheet.save(PNG, compress_level=9)
    png_bytes = PNG.read_bytes()
    meta = metadata(png_bytes, sheet)
    check_native_pivots(sheet, frames, meta)
    meta_bytes = encoded(meta)
    info_bytes = encoded(manifest(sheet, frames, png_bytes, meta_bytes))
    if args.check:
        assert METADATA.read_bytes() == meta_bytes, "Source metadata differs"
        assert MANIFEST.read_bytes() == info_bytes, "Source manifest differs"
        if NATIVE.exists():
            assert NATIVE.read_bytes() == png_bytes, "Registered PNG differs from import source"
            native = json.loads(NATIVE.with_suffix(".png.gbsres").read_text())
            assert native["id"] == NATIVE_ID and native["symbol"] == "sprite_queen_streetcar"
            assert normalized_resource(native) == normalized_resource(json.loads(meta_bytes)), "Registered frame/object layout differs"
    else:
        METADATA.write_bytes(meta_bytes)
        MANIFEST.write_bytes(info_bytes)
    info = json.loads(info_bytes)
    print(f"Queen tram: 8 poses, 4 OAM objects each, {info['unique_patterns_8x8']} unique 8x8 / {info['unique_patterns_8x16']} unique 8x16 patterns")
    print(f"Native ID: {NATIVE_ID}; source SHA-256: {info['png_sha256']}")


if __name__ == "__main__":
    main()
