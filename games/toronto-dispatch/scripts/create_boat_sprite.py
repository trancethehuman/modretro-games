#!/usr/bin/env python3
"""Author an original 16x24 launch, captain/passenger and isolated OBJ reserves.

North/east source poses share mirrored cells; the native renderer supplies the
other directions and a moving wake, clips bridge pixels, and drives the boat.
--check is read-only. Source limits are separate from compiled/native limits.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import uuid
from PIL import Image, ImageDraw
from create_atlas import decode_grid

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "project"
ART = PROJECT / "original-art"
TRANSPARENT = (101, 255, 0)
COLOURS = (TRANSPARENT, (224, 248, 207), (134, 192, 108), (7, 24, 33))
DOCKS = ((0, 560, 800), (0, 400, 800), (0, 736, 800),
         (4, 424, 72), (4, 424, 352), (4, 504, 384))

def ident(key):
    return str(uuid.uuid5(uuid.NAMESPACE_URL, "toronto-dispatch.boats." + key))

def same_png(left, right):
    with Image.open(io.BytesIO(left)) as a, Image.open(io.BytesIO(right)) as b:
        return a.format == b.format == "PNG" and a.mode == b.mode and a.size == b.size and \
            a.tobytes() == b.tobytes() and a.getpalette() == b.getpalette() and \
            a.info.get("transparency") == b.info.get("transparency")

def north_art():
    image = Image.new("RGB", (16, 32), TRANSPARENT)
    d = ImageDraw.Draw(image)
    d.polygon(((7, 0), (8, 0), (13, 5), (15, 12), (14, 21), (12, 23),
               (3, 23), (1, 21), (0, 12), (2, 5)), fill=COLOURS[3])
    d.polygon(((7, 2), (8, 2), (11, 6), (13, 12), (12, 20), (3, 20),
               (2, 12), (4, 6)), fill=COLOURS[1])
    d.rectangle((4, 9, 11, 17), fill=COLOURS[2])
    d.line((3, 8, 12, 8), fill=COLOURS[3])
    # Two original small people seated inside the launch.
    d.rectangle((4, 10, 6, 12), fill=COLOURS[3])
    d.rectangle((4, 11, 6, 12), fill=COLOURS[1])
    d.rectangle((4, 13, 6, 15), fill=COLOURS[3])
    d.rectangle((4, 14, 6, 15), fill=COLOURS[1])
    for y in range(32):
        for x in range(8):
            image.putpixel((15 - x, y), image.getpixel((x, y)))
    return image

def east_art():
    image = Image.new("RGB", (24, 16), TRANSPARENT)
    d = ImageDraw.Draw(image)
    d.polygon(((0, 7), (4, 2), (19, 2), (23, 7), (23, 8),
               (19, 13), (4, 13), (0, 8)), fill=COLOURS[3])
    d.polygon(((2, 7), (5, 4), (18, 4), (21, 7), (21, 8),
               (18, 11), (5, 11), (2, 8)), fill=COLOURS[1])
    d.rectangle((8, 4, 15, 11), fill=COLOURS[2])
    d.rectangle((10, 4, 13, 6), fill=COLOURS[3])
    d.rectangle((10, 5, 13, 6), fill=COLOURS[1])
    d.rectangle((10, 9, 13, 11), fill=COLOURS[3])
    d.rectangle((10, 10, 13, 11), fill=COLOURS[1])
    for y in range(16):
        for x in range(8):
            image.putpixel((23 - x, y), image.getpixel((x, y)))
    return image

WATER = ((0, 168, 96, 760), (96, 168, 392, 48), (432, 0, 64, 528),
         (96, 464, 400, 72), (96, 640, 704, 64), (752, 608, 112, 120),
         (792, 704, 32, 224), (0, 936, 1024, 40))
DECKS = ((416, 96, 96, 64), (416, 256, 96, 64),
         (160, 144, 64, 96), (160, 448, 64, 104),
         (160, 624, 64, 96), (776, 792, 64, 64))

def body_water(district, x, y, grid):
    if not (0 <= x < 1024 and 0 <= y < 976):
        return False
    if district == 0:
        water = y >= 816
        deck = False
    else:
        water = any(a <= x < a + w and b <= y < b + h for a, b, w, h in WATER)
        deck = any(a <= x < a + w and b <= y < b + h for a, b, w, h in DECKS)
    return water and (grid[(y // 8) * 128 + x // 8] == 15 or deck)

def validate_routes():
    count = 0
    grids = {}
    for district, u, top, bottom, name in ((0, 560, 840, 896, "toronto_city"),
                                          (4, 464, 64, 432, "toronto_port_lands")):
        scene = json.loads((PROJECT / f"project/scenes/{name}/scene.gbsres").read_text())
        grid = grids[district] = decode_grid(scene["collisions"], 128 * 122)
        for v in range(top, bottom + 1):
            for y in range(v - 12, v + 12):
                for x in range(u - 8, u + 8):
                    assert body_water(district, x, y, grid), "Launch hull leaves authored water/deck"
                    count += 1
    for district, u, v in DOCKS:
        for x in (u - 3, u, u + 3):
            for y in (v - 3, v, v + 3):
                assert not grids[district][(y // 8) * 128 + x // 8] & 15, "Dock foot landing is blocked"
    port = json.loads((ROOT / "content/districts/port_lands_art.json").read_text())
    assert tuple(map(tuple, port["water"])) == WATER, "Review boat water when authored geography changes"
    return count

def model(png_checksum=None):
    image = Image.new("RGB", (72, 32), TRANSPARENT)
    image.paste(north_art(), (0, 0)); image.paste(east_art(), (16, 0))
    for part in range(4):
        for y in range(16):
            for x in range(8):
                colour = 1 + ((x * (part + 2) + y + (y // (part + 2)) + part) % 3)
                image.putpixel((40 + part * 8 + x, y), COLOURS[colour])
    # GB Studio cell Y increases upward. The compiler masks each cell within
    # the32x32 canvas at(8+x,16-y), then emits(x-8,-8-y).
    # Keep every source rectangle fully inside that mask: a fourth column at
    # x24 would be silently discarded. North and scratch therefore use2x2.
    cells = (((0, 16, 0, 0), (8, 16, 8, 0), (0, 0, 0, 16), (8, 0, 8, 16)),
             ((0, 0, 16, 0), (8, 0, 24, 0), (16, 0, 32, 0)),
             tuple(((part % 2) * 8, 16 - (part // 2) * 16, 40 + part * 8, 0) for part in range(4)))
    frames = []
    for frame, parts in enumerate(cells):
        frames.append({"id": ident(f"frame-{frame}"), "tiles": [
            {"id": ident(f"tile-{frame}-{part}"), "x": x, "y": y, "sliceX": sx, "sliceY": sy,
             "flipX": False, "flipY": False, "palette": 0, "paletteIndex": 0,
             "objPalette": "OBP0", "priority": False}
            for part, (x, y, sx, sy) in enumerate(parts)]})
    frames.append({"id": ident("empty-frame"), "tiles": []})
    animations = [{"id": ident("poses"), "frames": frames}] + [
        {"id": ident(f"empty-animation-{i}"), "frames": [{"id": ident(f"empty-{i}"), "tiles": []}]}
        for i in range(1, 8)]
    png = io.BytesIO(); image.save(png, format="PNG"); blob = png.getvalue()
    meta = {"_resourceType": "sprite", "id": ident("editable-source-sprite"), "name": "Harbour launch",
            "symbol": "sprite_ambient_boat", "filename": "ambient_boat.png", "width": 72, "height": 32,
            "checksum": hashlib.sha1(blob).hexdigest() if png_checksum is None else png_checksum,
            "numTiles": 0, "canvasOriginX": 16, "canvasOriginY": 16, "canvasWidth": 32, "canvasHeight": 32,
            "boundsX": 8, "boundsY": 4, "boundsWidth": 16, "boundsHeight": 24, "animSpeed": 255,
            "states": [{"id": ident("state"), "name": "", "animationType": "fixed",
                        "flipLeft": False, "animations": animations}]}
    pairs = []
    for parts in cells:
        for _, _, sx, sy in parts:
            pixels = image.crop((sx, sy, sx + 8, sy + 16))
            variants = [pixels, pixels.transpose(Image.Transpose.FLIP_LEFT_RIGHT),
                        pixels.transpose(Image.Transpose.FLIP_TOP_BOTTOM),
                        pixels.transpose(Image.Transpose.FLIP_LEFT_RIGHT).transpose(Image.Transpose.FLIP_TOP_BOTTOM)]
            pairs.append(min(p.tobytes() for p in variants))
    unique = len(set(pairs))
    assert unique <= 8, f"Boat source/reserve exceeds sixteen selected raw tiles: {unique} pairs"
    manifest = {"status": "Original source; compiled allocation/native driving and hardware remain separate gates",
                "native_frame_order": ["north_hull", "east_hull", "four_independent_scratch_pairs", "empty_startup"],
                "source_colours": [list(c) for c in COLOURS], "hull_objects": 4, "scratch_tile_pairs": 4,
                "compiled_reserved_tiles_per_bank_limit": 10, "sprite_palette": 7, "loader_frame": 3,
                "persistent_native_state_target_bytes": 38, "maximum_persistent_state_bytes": 40,
                "fictional_routes": [{"district": 0, "u": 560, "top": 840, "bottom": 896},
                                     {"district": 4, "u": 464, "top": 64, "bottom": 432}],
                "port_deck_clip_rectangles": [list(r) for r in DECKS],
                "water_rectangles": [list(r) for r in WATER], "dock_foot_points": [list(p) for p in DOCKS],
                "player_control": True, "passenger_service": False, "save_fields": False,
                "hull_size_pixels": [16, 24], "visible_seated_people": 2,
                "wake": "Animated original ripple pixels reuse hull scratch objects without additional OBJ/BKG tiles",
                "source_unique_8x16_pairs_with_flips": unique, "source_raw_8x8_patterns_upper_bound": unique * 2,
                "water_full_hull_checks": validate_routes()}
    return {"ambient_boat.png": blob, "ambient_boat.metadata.json": (json.dumps(meta, indent=2) + "\n").encode(),
            "ambient_boat_art.json": (json.dumps(manifest, indent=2) + "\n").encode()}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true"); parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    checksum = hashlib.sha1((ART / "ambient_boat.png").read_bytes()).hexdigest() if args.check else None
    files = model(checksum)
    if args.check:
        for name, value in files.items():
            existing = (ART / name).read_bytes()
            assert same_png(existing, value) if name.endswith(".png") else existing == value, f"Stale boat: {name}"
        native = json.loads((PROJECT / "assets/sprites/ambient_boat.png.gbsres").read_text())
        source = json.loads(files["ambient_boat.metadata.json"])
        for key in ("id", "name", "symbol"): source[key] = native[key]
        assert source == native, "Boat native metadata differs from authored poses"
        assert (PROJECT / "assets/sprites/ambient_boat.png").read_bytes() == (ART / "ambient_boat.png").read_bytes()
    elif not args.dry_run:
        for name, value in files.items(): (ART / name).write_bytes(value)
        path = PROJECT / "assets/sprites/ambient_boat.png.gbsres"
        native = json.loads(path.read_text()); source = json.loads(files["ambient_boat.metadata.json"])
        for key in ("id", "name", "symbol"): source[key] = native[key]
        path.write_text(json.dumps(source, indent=2) + "\n")
        (PROJECT / "assets/sprites/ambient_boat.png").write_bytes(files["ambient_boat.png"])
        for path in (PROJECT / "project/scenes").glob("*/actors/*.gbsres"):
            actor = json.loads(path.read_text())
            if actor.get("spriteSheetId") == native["id"]:
                actor["frame"] = 3; path.write_text(json.dumps(actor, indent=2) + "\n")
    print("Original 16x24 launch: two seated people, at most4OBJ, four scratch pairs; native/hardware unverified.")

if __name__ == "__main__": main()
