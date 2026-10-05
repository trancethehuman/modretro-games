"""Original St. George campus details on three existing Core footprints.

No map imagery, logos or imported architecture pixels. Every stamp is an
8-pixel native cell. Existing terrain, priority, palette bytes, roads and ROM
footprint remain authoritative; named coordinates are compressed game design.
"""
from PIL import Image, ImageDraw

COLORS = ("#071821", "#306850", "#86c06c", "#e0f8cf")
RGB = tuple(tuple(bytes.fromhex(c[1:])) for c in COLORS)
LANDMARKS = (
    {"id": "robarts", "name": "Robarts Library", "footprint": [368, 96, 32, 40],
     "character": "Angular concrete roof and repeated vertical bays", "real_address": "130 St. George Street"},
    {"id": "university-college", "name": "University College", "footprint": [408, 208, 32, 40],
     "character": "Romanesque stone tower, clock, arched arcade and original UT letter sign", "real_address": "15 King's College Circle"},
    {"id": "convocation", "name": "Convocation Hall", "footprint": [368, 208, 32, 40],
     "character": "Symmetric domed rotunda, central oculus and columned frontage", "real_address": "31 King's College Circle"},
)


def patterns(image):
    result = set()
    for y in range(0, image.height, 8):
        for x in range(0, image.width, 8):
            tile = image.crop((x, y, x + 8, y + 8))
            result.add(min(tile.tobytes(), tile.transpose(Image.Transpose.FLIP_LEFT_RIGHT).tobytes(),
                           tile.transpose(Image.Transpose.FLIP_TOP_BOTTOM).tobytes(),
                           tile.transpose(Image.Transpose.ROTATE_180).tobytes()))
    return result


def tile(rows):
    assert len(rows) == 8 and all(len(row) == 8 for row in rows)
    result = Image.new("RGB", (8, 8))
    result.putdata([RGB[int(c)] for row in rows for c in row])
    return result


def _symmetric_quarters(quarter):
    """A 16x16 quadrant supplies at most four new flip-canonical cells."""
    result = Image.new("RGB", (32, 32))
    result.paste(quarter, (0, 0))
    result.paste(quarter.transpose(Image.Transpose.FLIP_LEFT_RIGHT), (16, 0))
    result.paste(quarter.transpose(Image.Transpose.FLIP_TOP_BOTTOM), (0, 16))
    result.paste(quarter.transpose(Image.Transpose.ROTATE_180), (16, 16))
    return result


def paint(image, blocks):
    assert image.mode == "RGB" and image.size == (1024, 976)
    before = image.copy()
    for item in LANDMARKS:
        assert any([b[k] for k in ("x", "y", "width", "depth")] == item["footprint"]
                   and b.get("landmark") is None for b in blocks), "Campus needs its existing generic footprint"
    stamped = []

    def stamp(x, y, artwork):
        assert x % 8 == y % 8 == 0 and artwork.width % 8 == artwork.height % 8 == 0
        image.paste(artwork, (x, y))
        stamped.extend((xx, yy) for yy in range(y, y + artwork.height, 8)
                       for xx in range(x, x + artwork.width, 8))

    # Robarts' angular roof uses just six symmetric native motifs. Concrete
    # plinths fill its retained rectangular collision body, avoiding a false
    # visually empty corner. No claim to a surveyed building outline is made.
    half = Image.new("RGB", (16, 24), RGB[1])
    pen = ImageDraw.Draw(half)
    pen.polygon([(15, 0), (15, 23), (0, 23)], fill=RGB[3])
    pen.line((15, 0, 0, 23), fill=RGB[0])
    pen.line((15, 5, 4, 23), fill=RGB[2])
    pen.line((15, 12, 9, 23), fill=RGB[0])
    roof = Image.new("RGB", (32, 24))
    roof.paste(half, (0, 0)); roof.paste(half.transpose(Image.Transpose.FLIP_LEFT_RIGHT), (16, 0))
    stamp(368, 96, roof)

    # UC's clock/tower is horizontally mirrored: two new canonical clock
    # cells, an arcade cell and one original text monogram, four in total.
    clock_left = Image.new("RGB", (8, 16), RGB[1])
    for y in range(16):
        for x in range(8):
            rr = (15 - 2*x)**2 + (15 - 2*y)**2
            if rr <= 13**2: clock_left.putpixel((x, y), RGB[0])
            if rr <= 11**2: clock_left.putpixel((x, y), RGB[3])
            if (x == 7 and 4 <= y <= 8) or (y == 8 and 4 <= x <= 7):
                clock_left.putpixel((x, y), RGB[0])
    clock_half = clock_left.crop((0,0,8,8)); lower_half = clock_left.crop((0,8,8,16))
    stamp(416, 200, clock_half); stamp(424, 200, clock_half.transpose(Image.Transpose.FLIP_LEFT_RIGHT))
    stamp(416, 208, lower_half); stamp(424, 208, lower_half.transpose(Image.Transpose.FLIP_LEFT_RIGHT))
    arcade = tile(("33333333", "33111133", "31000013", "31033013",
                   "31033013", "31033013", "31033013", "00000000"))
    for x in (408, 416, 424, 432): stamp(x, 240, arcade)
    monogram = tile(("11111111", "30303330", "30300300", "30300300",
                     "30300300", "33300300", "11111111", "00000000"))
    stamp(416, 232, monogram)

    # The symmetric dome consists of four canonical quadrant cells and one
    # repeated column cell. Robarts instead reuses an existing straight glass
    # bay motif, keeping its angular concrete mass distinct from this rotunda.
    quadrant = Image.new("RGB", (16, 16), RGB[1]); pen = ImageDraw.Draw(quadrant)
    for y in range(16):
        for x in range(16):
            rr = (31 - 2*x)**2 + (31 - 2*y)**2
            if rr <= 31**2: quadrant.putpixel((x, y), RGB[2 if rr < 27**2 else 0])
            if rr < 23**2 and (x == y or x == 15 or y == 15): quadrant.putpixel((x, y), RGB[3])
            if rr < 7**2: quadrant.putpixel((x, y), RGB[0])
    stamp(368, 208, _symmetric_quarters(quadrant))
    columns = tile(("33333333", "31133113", "31033013", "31033013",
                    "31033013", "31033013", "31133113", "00000000"))
    library_bay = before.crop((416,104,424,112))
    for x in (368, 376, 384, 392):
        stamp(x, 240, columns); stamp(x, 120, library_bay); stamp(x, 128, library_bay)

    # The existing pale path/forecourt strips remain accessible. One repeated
    # paving motif makes their campus stonework readable. No stamp covers road markings, mission
    # points, destructible props or another campus/ROM collision footprint.
    path = tile(("33313333", "33313333", "33313333", "11111111",
                 "33333331", "33333331", "33333331", "11111111"))
    for x in range(368, 440, 8): stamp(x, 144, path)
    for x in range(368, 440, 8): stamp(x, 256, path)

    introduced = patterns(image) - patterns(before)
    assert len(introduced) <= 15, "Campus exceeds its bounded additive tile plan"
    return {"stamp_cells": [list(c) for c in sorted(set(stamped), key=lambda c: (c[1], c[0]))],
            "introduced_canonical_patterns": len(introduced),
            "before_canonical_patterns": len(patterns(before)),
            "after_canonical_patterns": len(patterns(image))}
