#!/usr/bin/env python3
"""Author original compact fleet and civilian PNG/native-metadata pairs.

The public native_metadata importer registers these sources separately. Each
fleet pose uses two 8x16 objects; each compact human uses one; palette variants and opposite headings reuse
the same source patterns. --check reads generated/imported resources only.
Source checks are not compiled allocation, native rendering or hardware proof.
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
TRANSPARENT = (101, 255, 0)
LIGHT = (224, 248, 207)
MID = (134, 192, 108)
DARK = (7, 24, 33)
COLOURS = (TRANSPARENT, LIGHT, MID, DARK)
SCENES = tuple(d["scene"] for d in json.loads((ROOT / "content/districts/world.json").read_text())["districts"])
PALETTES = (
    ("Civilian teal", ["F8F0E0", "F1D4AB", "329DA1", "182536"], 1),
    ("Civilian ochre", ["F8F0E0", "DCAE80", "E1AA36", "342839"], 2),
    ("Police fleet", ["F8F0E0", "F8F4DD", "4083D2", "182536"], 3),
    ("Fire fleet", ["F8F0E0", "F8E7B7", "DE4A48", "251E2D"], 4),
    ("Ambulance fleet", ["F8F0E0", "FFF5E4", "EA6253", "263C53"], 5),
    ("City bus fleet", ["F8F0E0", "F8EBC1", "E3AD35", "203545"], 6),
)
FLEET_PALETTE_SLOTS = (3, 4, 5, 6, 6)  # Taxi reuses the original gold bus palette.


def ident(key):
    return str(uuid.uuid5(uuid.NAMESPACE_URL, "toronto-dispatch.city-sprites." + key))


def encoded(value):
    return (json.dumps(value, indent=2) + "\n").encode()


def fleet_east(kind):
    image = Image.new("RGB", (16, 16), TRANSPARENT)
    d = ImageDraw.Draw(image)
    if kind == 4:  # Original gold taxi sedan: short roof light and checker trim.
        d.rectangle((2,3,4,4),fill=DARK);d.rectangle((10,3,12,4),fill=DARK)
        d.rectangle((2,11,4,12),fill=DARK);d.rectangle((10,11,12,12),fill=DARK)
        d.rectangle((1,5,14,10),fill=DARK)
        d.rectangle((2,4,12,11),fill=DARK)
        d.rectangle((2,5,13,10),fill=MID)
        d.rectangle((4,4,9,11),fill=MID)
        d.line((4,5,4,10),fill=DARK)  # Separate rear and front windows.
        d.line((10,5,10,10),fill=DARK)
        d.line((11,6,11,9),fill=LIGHT)
        d.rectangle((6,7,8,8),fill=LIGHT)
        d.point((7,7),fill=DARK)  # Readable roof sign, independent of siren art.
        for x in (5,7,9):
            d.point((x,4),fill=DARK);d.point((x,11),fill=LIGHT)
        d.point((13,5),fill=LIGHT);d.point((13,10),fill=LIGHT)
        d.line((1,6,1,9),fill=DARK)
    elif kind == 0:  # Recognisable patrol sedan: wheels, hood, glass and lightbar.
        d.rectangle((2,3,4,4),fill=DARK);d.rectangle((10,3,12,4),fill=DARK)
        d.rectangle((2,11,4,12),fill=DARK);d.rectangle((10,11,12,12),fill=DARK)
        d.rectangle((1,5,14,10),fill=DARK)
        d.rectangle((2,4,12,11),fill=DARK)
        d.rectangle((2,5,13,10),fill=LIGHT)
        d.rectangle((4,4,9,11),fill=MID)
        d.line((4,5,4,10),fill=DARK)  # Rear glass.
        d.line((10,5,10,10),fill=DARK)  # Front windshield.
        d.line((11,6,11,9),fill=MID)  # Hood visibly ahead of cabin.
        d.line((7,5,7,10),fill=LIGHT)
        d.point((7,5),fill=DARK);d.point((7,10),fill=MID)
        d.point((13,5),fill=LIGHT);d.point((13,10),fill=LIGHT)
        d.line((1,6,1,9),fill=MID)
    else:
        # All compact service vehicles have a separate front cab and box body.
        d.rectangle((1, 3, 14, 12), fill=DARK)
        d.rectangle((2, 4, 13, 11), fill=MID)
        d.rectangle((10, 4, 13, 11), fill=LIGHT)
        d.line((12, 5, 12, 10), fill=DARK)
        d.line((9, 4, 9, 11), fill=DARK)
        # Exposed tire pairs and front lights anchor the vehicle to the road.
        d.rectangle((2,2,4,3),fill=DARK);d.rectangle((10,2,12,3),fill=DARK)
        d.rectangle((2,12,4,13),fill=DARK);d.rectangle((10,12,12,13),fill=DARK)
        d.point((14,4),fill=LIGHT);d.point((14,11),fill=LIGHT)
        if kind == 1:  # Fire apparatus: longitudinal ladder with cross rungs.
            d.rectangle((3, 5, 8, 10), fill=DARK)
            d.line((3, 5, 8, 5), fill=LIGHT)
            d.line((3, 10, 8, 10), fill=LIGHT)
            for x in (4, 6, 8):
                d.line((x, 6, x, 9), fill=LIGHT)
            d.point((11, 4), fill=MID)
        elif kind == 2:  # Ambulance: bright box and original two-band roof sign.
            d.rectangle((2, 4, 8, 11), fill=LIGHT)
            d.rectangle((4, 6, 6, 9), fill=MID)
            d.line((3, 7, 7, 7), fill=MID)
            d.line((2, 10, 8, 10), fill=MID)
            d.line((10, 4, 11, 4), fill=MID)
        else:  # Bus: repeated roof/window divisions and a broad front screen.
            d.rectangle((2, 5, 8, 10), fill=LIGHT)
            for x in (3, 5, 7):
                d.line((x, 4, x, 5), fill=DARK)
                d.line((x, 10, x, 11), fill=DARK)
            d.line((4, 7, 7, 7), fill=MID)
            d.line((12, 4, 12, 11), fill=DARK)
    return image


def civilian_east(step,kind=0):
    image = Image.new("RGB", (16, 16), TRANSPARENT)
    d = ImageDraw.Draw(image)
    # Six by ten readable pixels, centred inside a single 8x16 hardware OBJ.
    d.rectangle((6, 3, 9, 5), fill=DARK)
    d.rectangle((7, 4, 9, 5), fill=LIGHT)
    d.point((9, 4), fill=DARK)
    d.rectangle((6, 6, 9, 9), fill=MID)
    d.point((5, 7 if step else 8), fill=LIGHT)
    d.point((10, 8 if step else 7), fill=LIGHT)
    d.line((6, 10, 6+step, 12), fill=DARK)
    d.line((9, 10, 9-step, 12), fill=DARK)
    if kind==1:  # Worker: broad hardhat and dark work-overall straps.
        d.line((5,3,10,3),fill=MID)
        d.point((6,7),fill=DARK);d.point((9,7),fill=DARK)
    elif kind==2:  # Backpacker: visible bag, strap and a darker long jacket.
        d.rectangle((5,6,6,8),fill=DARK)
        d.point((6,7),fill=LIGHT);d.line((9,6,9,9),fill=DARK)
    elif kind==3:  # Senior: pale hair, cardigan front and a long cane.
        d.line((6,3,9,3),fill=LIGHT)
        d.line((8,6,8,9),fill=LIGHT)
        d.line((10,8,10,12),fill=DARK)
    return image


def airborne():
    image = Image.new("RGB", (16, 16), TRANSPARENT)
    d = ImageDraw.Draw(image)
    d.rectangle((8, 3, 10, 5), fill=DARK)
    d.rectangle((9, 4, 10, 5), fill=LIGHT)
    d.polygon(((6, 6), (9, 6), (9, 9), (6, 10)), fill=MID)
    d.line((5, 6, 4, 4), fill=LIGHT)
    d.line((10, 7, 11, 6), fill=LIGHT)
    d.line((6, 10, 4, 12), fill=DARK)
    d.line((8, 10, 10, 11), fill=DARK)
    return image


def prone():
    image = Image.new("RGB", (16, 16), TRANSPARENT)
    d = ImageDraw.Draw(image)
    # Still, non-graphic ground pose: face, jacket and separated shoes.
    d.rectangle((4, 9, 5, 11), fill=DARK)
    d.rectangle((6, 8, 8, 11), fill=MID)
    d.rectangle((9, 8, 11, 10), fill=DARK)
    d.rectangle((10, 9, 11, 10), fill=LIGHT)
    d.point((7, 12), fill=LIGHT)
    return image


def resource(name, models, poses):
    sheet = Image.new("RGB", (len(models) * 16, 16), TRANSPARENT)
    for i, image in enumerate(models):
        sheet.paste(image, (i * 16, 0))
    frames = []
    for frame, (model, flip_x, flip_y, palette) in enumerate(poses):
        objects = []
        for part in range(1 if name == "city_civilians" else 2):
            objects.append({"id": ident(f"{name}.frame-{frame}.object-{part}"),
                "x": 4 if name == "city_civilians" else part * 8, "y": 0,
                "sliceX": model * 16 + (4 if name == "city_civilians" else (1 - part if flip_x else part) * 8),
                "sliceY": 0, "flipX": flip_x, "flipY": flip_y,
                "palette": 0, "paletteIndex": palette,
                "objPalette": "OBP0", "priority": False})
        frames.append({"id": ident(f"{name}.frame-{frame}"), "tiles": objects})
    frames.append({"id": ident(f"{name}.hidden"), "tiles": []})
    animations = [{"id": ident(f"{name}.poses"), "frames": frames}]
    animations += [{"id": ident(f"{name}.animation-{i}"), "frames": [
        {"id": ident(f"{name}.empty-{i}"), "tiles": []}]} for i in range(1, 8)]
    patterns = {sheet.crop((tile["sliceX"],tile["sliceY"],tile["sliceX"]+8,tile["sliceY"]+16)).tobytes()
                for frame in frames for tile in frame["tiles"]}
    meta = {"_resourceType": "sprite", "id": ident(name + ".source"),
        "name": name.replace("_", " ").title(), "symbol": "sprite_" + name,
        "filename": name + ".png", "width": sheet.width, "height": 16,
        "numTiles": len(patterns), "canvasOriginX": 8, "canvasOriginY": 8,
        "canvasWidth": 16, "canvasHeight": 16,
        "boundsX": 2, "boundsY": 2, "boundsWidth": 12, "boundsHeight": 12,
        "animSpeed": 255, "states": [{"id": ident(name + ".state"),
            "name": "", "animationType": "fixed", "flipLeft": False,
            "animations": animations}]}
    return sheet, meta


def artwork():
    fleet_models = []
    fleet_poses = []
    for kind,palette in enumerate(FLEET_PALETTE_SLOTS):
        east = fleet_east(kind)
        fleet_models.extend((east, east.transpose(Image.Transpose.ROTATE_270)))
        fleet_poses.extend(((kind * 2, False, False, palette),
                            (kind * 2 + 1, False, False, palette),
                            (kind * 2, True, False, palette),
                            (kind * 2 + 1, False, True, palette)))
    civilian_models = [civilian_east(step,kind) for kind in range(4) for step in range(2)] + [airborne(),prone()]
    civilian_poses = []
    for variant in range(4):
        palette=1+(variant&1)
        civilian_poses.extend(((variant*2, False, False, palette),
            (variant*2+1, False, False, palette), (variant*2, True, False, palette),
            (variant*2+1, True, False, palette), (8, False, False, palette),
            (9, False, False, palette)))
    return {"city_fleet": resource("city_fleet", fleet_models, fleet_poses),
            "city_civilians": resource("city_civilians", civilian_models, civilian_poses)}


def normalized(value):
    if isinstance(value, dict):
        return {k: normalized(v) for k, v in value.items() if k not in ("id", "name", "symbol")}
    if isinstance(value, list):
        return [normalized(v) for v in value]
    return value


def check_registered(name, meta, png):
    native_path = PROJECT / "assets/sprites" / (name + ".png")
    assert native_path.read_bytes() == png, f"{name}: imported pixels differ"
    native = json.loads(native_path.with_suffix(".png.gbsres").read_text())
    assert native["id"] == ident(name + ".native")
    assert normalized(native) == normalized(meta), f"{name}: imported poses differ"
    def native_ids(value):
        if isinstance(value,dict):
            if "id" in value:yield value["id"]
            for child in value.values():yield from native_ids(child)
        elif isinstance(value,list):
            for child in value:yield from native_ids(child)
    ids=list(native_ids(native));assert len(ids)==len(set(ids)),f"{name}: duplicate native resource IDs"
    loader_name = name + "_loader.gbsres"
    for scene in SCENES:
        actors = PROJECT / "project/scenes" / scene / "actors"
        loader = json.loads((actors / loader_name).read_text())
        queen = scene in ("toronto_city", "toronto_west", "toronto_east")
        assert loader["_index"] == (2 if queen else 1) + (name == "city_civilians")
        assert loader["spriteSheetId"] == native["id"]
        assert loader["frame"] == len(meta["states"][0]["animations"][0]["frames"]) - 1
        assert not loader["animate"]
        resources = [json.loads(path.read_text()) for path in actors.glob("*.gbsres")]
        assert len({actor["_index"] for actor in resources}) == len(resources)
        assert sum(actor["spriteSheetId"] == native["id"] for actor in resources) == 1


def check_palettes():
    for name, colours, slot in PALETTES:
        path = PROJECT / "project/palettes" / (name.lower().replace(" ", "_") + ".gbsres")
        palette = json.loads(path.read_text())
        assert palette["colors"] == colours
        # Hardware precision must retain the three opaque value families.
        rgb555 = [sum((int(colour[start:start + 2], 16) >> 3) << shift
                      for start, shift in ((0, 0), (2, 5), (4, 10))) for colour in colours]
        assert len(set(rgb555[1:])) == 3
        for scene in SCENES:
            resource = json.loads((PROJECT / "project/scenes" / scene / "scene.gbsres").read_text())
            assert resource["spritePaletteIds"][slot] == palette["id"]


def check_poses(name, sheet, meta):
    """Reconstruct editor objects independently and compare intended flips."""
    frames = meta["states"][0]["animations"][0]["frames"]
    rendered = []
    for frame in frames[:-1]:
        image = Image.new("RGB", (16, 16), TRANSPARENT)
        for tile in frame["tiles"]:
            assert tile["x"] in ((4,) if name == "city_civilians" else (0, 8)) and tile["y"] == 0
            crop = sheet.crop((tile["sliceX"], tile["sliceY"],
                               tile["sliceX"] + 8, tile["sliceY"] + 16))
            if tile["flipX"]:
                crop = crop.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
            if tile["flipY"]:
                crop = crop.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
            image.paste(crop, (tile["x"], 0))
        rendered.append(image)
    if name == "city_fleet":
        for kind,palette in enumerate(FLEET_PALETTE_SLOTS):
            east, south, west, north = rendered[kind * 4:kind * 4 + 4]
            assert south.tobytes() == east.transpose(Image.Transpose.ROTATE_270).tobytes()
            assert west.tobytes() == east.transpose(Image.Transpose.FLIP_LEFT_RIGHT).tobytes()
            assert north.tobytes() == south.transpose(Image.Transpose.FLIP_TOP_BOTTOM).tobytes()
            assert {t["paletteIndex"] for f in frames[kind * 4:kind * 4 + 4]
                    for t in f["tiles"]} == {palette}
        assert len({rendered[kind * 4].tobytes() for kind in range(5)}) == 5
    else:
        for variant in range(4):
            east0, east1, west0, west1, hit, fallen = rendered[variant * 6:variant * 6 + 6]
            assert west0.tobytes() == east0.transpose(Image.Transpose.FLIP_LEFT_RIGHT).tobytes()
            assert west1.tobytes() == east1.transpose(Image.Transpose.FLIP_LEFT_RIGHT).tobytes()
            assert len({east0.tobytes(), east1.tobytes(), hit.tobytes(), fallen.tobytes()}) == 4
            assert {t["paletteIndex"] for f in frames[variant * 6:variant * 6 + 6]
                    for t in f["tiles"]} == {1+(variant&1)}
        assert len({rendered[variant*6].tobytes() for variant in range(4)})==4
        for variant in range(4):
            assert rendered[variant*6+4].tobytes()==rendered[4].tobytes()
            assert rendered[variant*6+5].tobytes()==rendered[5].tobytes()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--sources-only", action="store_true")
    args = parser.parse_args()
    ART.mkdir(parents=True, exist_ok=True)
    for name, (sheet, meta) in artwork().items():
        png_path = ART / (name + ".png")
        if args.check:
            with Image.open(png_path) as existing:
                assert existing.convert("RGB").tobytes() == sheet.tobytes()
        else:
            sheet.save(png_path, compress_level=9)
        png = png_path.read_bytes()
        meta["checksum"] = hashlib.sha1(png).hexdigest()
        meta_bytes = encoded(meta)
        frames = meta["states"][0]["animations"][0]["frames"]
        objects = 1 if name == "city_civilians" else 2
        assert [len(f["tiles"]) for f in frames] == [objects] * (len(frames) - 1) + [0]
        assert {sheet.getpixel((x, y)) for y in range(sheet.height)
                for x in range(sheet.width)} <= set(COLOURS)
        check_poses(name, sheet, meta)
        info = {"schema_version": 1, "licence": "MIT",
            "notice": "Original hand-placed pixels; no logo, photograph or copied game sprite",
            "generator": "scripts/create_city_sprites.py", "native_asset_id": ident(name + ".native"),
            "native_symbol": "sprite_" + name, "source_png": "original-art/" + name + ".png",
            "source_metadata": "original-art/" + name + ".metadata.json",
            "destination": "assets/sprites/" + name + ".png",
            "png_sha256": hashlib.sha256(png).hexdigest(),
            "metadata_sha256": hashlib.sha256(meta_bytes).hexdigest(),
            "decoded_rgb_sha256": hashlib.sha256(sheet.tobytes()).hexdigest(),
            "visible_frames": len(frames) - 1, "hidden_frame": len(frames) - 1,
            "objects_per_visible_pose": objects, "palette_slots": sorted({
                t["paletteIndex"] for f in frames for t in f["tiles"]}),
            "source_unique_8x16_patterns": meta["numTiles"],
            "allocation_target_max_8x8_per_obj_bank": 24,
            "frame_layout": "police/fire/ambulance/bus/taxi each E,S,W,N; taxi shares gold bus palette6" if name == "city_fleet" else
                "commuter/worker/backpacker/senior each E0,E1,W0,W1,airborne,prone; palettes1/2 reused",
            "verification": "Source and import checks only; compiler allocation/native rendering/hardware separate"}
        for path, content in ((ART / (name + ".metadata.json"), meta_bytes),
                              (ART / (name + "_art.json"), encoded(info))):
            if args.check:
                assert path.read_bytes() == content, f"{path.name} differs"
            else:
                path.write_bytes(content)
        preview = sheet.resize((sheet.width * 4, 64), Image.Resampling.NEAREST)
        preview_path = ART / (name + "_preview.png")
        if args.check:
            with Image.open(preview_path) as existing:
                assert existing.convert("RGB").tobytes() == preview.tobytes()
            if not args.sources_only:
                check_registered(name, meta, png)
        else:
            preview.save(preview_path, compress_level=9)
        print(f"{name}: {len(frames)-1} visible + empty, {objects} OBJ, {meta['numTiles']} source 8x16 patterns; ID {info['native_asset_id']}")
    if args.check and not args.sources_only:
        check_palettes()


if __name__ == "__main__":
    main()
