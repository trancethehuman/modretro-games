"""Compile the original newcomer drama and deliberately authored native portraits.

Story art is an engine-owned window panel rather than a new exploration scene.
It uses the unchanged navy/mint/gold UI palette, no OAM objects, bank1 tiles
16..187 and existing font tiles192..252. Editable narrative lives in story.json;
whole-pixel artwork recipes stay here. No downloaded or Nintendo artwork is used.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "project/plugins/toronto-driving/engine"
ART = ROOT / "project/original-art/story"
PORTRAITS = ("june", "moss", "vale")
WIDTH, HEIGHT = 160, 80
TILE_FIRST, TILE_LIMIT = 16, 172
THRESHOLDS = (0, 1, 8, 16, 32, 56, 80, 104)
GLYPHS = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/.-+?<>$%#="


def render(portrait):
    """Original over-shoulder courier and smaller conversation partner."""
    image = Image.new("P", (WIDTH, HEIGHT), 0)
    palette = json.loads((ROOT / "project/project/palettes/default_ui.gbsres").read_text())["colors"]
    rgb = [tuple(bytes.fromhex(color)) for color in palette]
    image.putpalette([channel for color in rgb for channel in color] + [0] * (768 - 12))
    draw = ImageDraw.Draw(image)
    # The distant city has a simple bright sky, patterned facades and a tower.
    draw.rectangle((0, 0, 159, 41), fill=1)
    draw.rectangle((0, 42, 159, 79), fill=0)
    draw.rectangle((0, 14, 15, 41), fill=0)
    draw.rectangle((8, 8, 23, 41), fill=0)
    draw.rectangle((32, 24, 55, 41), fill=0)
    draw.rectangle((56, 16, 71, 41), fill=0)
    draw.rectangle((144, 16, 159, 41), fill=0)
    for x, height in ((16, 12), (40, 28), (64, 20), (152, 20)):
        for y in range(height, 39, 8):
            draw.rectangle((x, y, x + 3, y + 3), fill=2)
    draw.line((82, 3, 82, 39), fill=0, width=2)
    draw.rectangle((78, 14, 86, 17), fill=0)
    draw.rectangle((80, 12, 84, 13), fill=2)
    draw.line((0, 44, 159, 44), fill=2)
    draw.rectangle((96, 72, 151, 74), fill=1)
    # Farther-away partner: a human head, face, coat, hands and feet.
    draw.rectangle((115, 8, 134, 14), fill=0)
    draw.rectangle((111, 15, 138, 35), fill=0)
    draw.rectangle((114, 16, 135, 32), fill=3)
    draw.rectangle((117, 29, 133, 36), fill=2)
    draw.rectangle((108, 35, 141, 60), fill=0)
    draw.rectangle((111, 38, 138, 59), fill=1 if portrait != "vale" else 2)
    draw.rectangle((106, 40, 110, 57), fill=3)
    draw.rectangle((139, 40, 143, 57), fill=3)
    draw.rectangle((113, 60, 122, 69), fill=0)
    draw.rectangle((128, 60, 137, 69), fill=0)
    draw.rectangle((111, 68, 123, 71), fill=2)
    draw.rectangle((127, 68, 139, 71), fill=2)
    draw.point((120, 23), fill=0)
    draw.point((130, 23), fill=0)
    draw.line((122, 28, 128, 28), fill=0)
    if portrait == "june":
        draw.rectangle((114, 11, 134, 17), fill=2)
        draw.rectangle((112, 14, 116, 28), fill=2)
        draw.rectangle((133, 14, 137, 28), fill=2)
        draw.rectangle((123, 37, 126, 52), fill=2)
        draw.rectangle((130, 42, 135, 46), fill=3)
    elif portrait == "moss":
        draw.rectangle((112, 9, 135, 17), fill=2)
        draw.rectangle((109, 16, 139, 19), fill=0)
        draw.rectangle((116, 27, 133, 32), fill=0)
        draw.rectangle((117, 38, 132, 59), fill=2)
        draw.line((117, 38, 117, 50), fill=0)
        draw.line((132, 38, 132, 50), fill=0)
        draw.rectangle((121, 45, 128, 49), fill=0)
    else:
        draw.rectangle((115, 11, 134, 17), fill=0)
        draw.rectangle((117, 21, 131, 23), fill=0)
        draw.rectangle((123, 37, 128, 59), fill=0)
        draw.polygon(((118, 37), (123, 45), (122, 37)), fill=3)
        draw.polygon(((133, 37), (128, 45), (129, 37)), fill=3)
    # Near courier, seen from behind: the oversized shoulder/bag establishes
    # composition without altering the accepted small overworld person art.
    draw.polygon(((0, 79), (0, 69), (14, 53), (25, 48), (56, 48), (70, 57), (81, 79)), fill=0)
    draw.polygon(((2, 79), (3, 70), (18, 56), (27, 52), (53, 52), (65, 59), (75, 79)), fill=1)
    draw.rectangle((26, 25, 55, 48), fill=0)
    draw.rectangle((30, 39, 51, 51), fill=2)
    draw.rectangle((24, 22, 55, 36), fill=0)
    draw.rectangle((29, 20, 51, 23), fill=0)
    draw.rectangle((27, 31, 54, 34), fill=1)
    draw.line((24, 50, 54, 76), fill=2, width=5)
    draw.rectangle((39, 61, 69, 79), fill=0)
    draw.rectangle((43, 65, 66, 79), fill=2)
    draw.rectangle((48, 69, 60, 72), fill=0)
    draw.line((5, 78, 34, 78), fill=3)
    return image


def tile_bytes(image, x, y):
    return bytes(value for row in range(y, y + 8)
                 for value in (sum((image.getpixel((x + column, row)) & 1) << (7 - column) for column in range(8)),
                               sum(((image.getpixel((x + column, row)) >> 1) & 1) << (7 - column) for column in range(8))))


def validate_story(story):
    assert story["schema"] == 1 and story["original"] is True
    assert len(story["scenes"]) == 8
    assert "ethnicity" in story["protagonist"]["background"].lower()
    pages = []
    for identity, (scene, threshold) in enumerate(zip(story["scenes"], THRESHOLDS)):
        assert scene["id"] == identity
        assert scene["minimum_completed"] == threshold
        assert 4 <= len(scene["pages"]) <= 12
        for text in (scene["title"],):
            assert 0 < len(text) <= 20 and set(text) <= set(GLYPHS), (scene["id"], text)
        for page in scene["pages"]:
            assert page["portrait"] in PORTRAITS
            assert page["speaker"] in ("DRIVER", "JUNE", "MOSS", "VALE")
            assert len(page["lines"]) == 4
            for text in page["lines"]:
                assert 0 < len(text) <= 20 and set(text) <= set(GLYPHS), (scene["id"], text, len(text))
            pages.append(page)
    assert len(pages) < 255
    return pages


def build():
    story = json.loads((ROOT / "content/story.json").read_text())
    pages = validate_story(story)
    images = [render(portrait) for portrait in PORTRAITS]
    patterns, identities, maps = [], {}, []
    for image in images:
        tilemap = []
        for y in range(0, HEIGHT, 8):
            for x in range(0, WIDTH, 8):
                pattern = tile_bytes(image, x, y)
                if pattern not in identities:
                    identities[pattern] = len(patterns)
                    patterns.append(pattern)
                tilemap.append(TILE_FIRST + identities[pattern])
        maps.append(tilemap)
    assert len(patterns) <= TILE_LIMIT, (len(patterns), TILE_LIMIT)
    source = ["/* Generated original story/portrait data. Edit story.json/create_story.py. */",
              f"#define TD_STORY_TILE_COUNT {len(patterns)}",
              f"#define TD_STORY_PAGE_COUNT {len(pages)}",
              "typedef struct { UBYTE scene,portrait; char speaker[7],lines[4][21]; } td_story_page_t;",
              "static const UBYTE td_story_thresholds[TD_STORY_SCENES]={" + ",".join(map(str, THRESHOLDS)) + "};"]
    firsts = []
    offset = 0
    for scene in story["scenes"]:
        firsts.append(offset)
        offset += len(scene["pages"])
    source.append("static const UBYTE td_story_firsts[TD_STORY_SCENES+1]={" + ",".join(map(str, firsts + [offset])) + "};")
    source.append("static const char td_story_titles[TD_STORY_SCENES][21]={")
    source.extend("    " + json.dumps(scene["title"]) + "," for scene in story["scenes"])
    source.append("};\nstatic const td_story_page_t td_story_pages[TD_STORY_PAGE_COUNT]={")
    for scene in story["scenes"]:
        for page in scene["pages"]:
            source.append("    {%u,%u,%s,{%s}}," %
                          (scene["id"], PORTRAITS.index(page["portrait"]), json.dumps(page["speaker"]),
                           ",".join(json.dumps(line) for line in page["lines"])))
    source.append("};\nstatic const UBYTE td_story_tiles[TD_STORY_TILE_COUNT*16]={")
    source.extend("    " + ",".join(map(str, pattern)) + "," for pattern in patterns)
    source.append("};\nstatic const UBYTE td_story_maps[3][200]={")
    source.extend("    {" + ",".join(map(str, tilemap)) + "}," for tilemap in maps)
    source.append("};\n")
    manifest = {"schema": 1, "source": "create_story.py", "original": True,
                "composition": "Original near courier back/shoulder, distant human speaker; dialogue only.",
                "dimensions": [WIDTH, HEIGHT], "palette": "default-ui", "palette_slot": 7,
                "tile_bank": 1, "first_tile": TILE_FIRST, "tile_count": len(patterns),
                "last_tile": TILE_FIRST + len(patterns) - 1, "font_first": 192,
                "new_oam_objects": 0, "story_scenes": 8, "story_pages": len(pages),
                "persistent_state_bytes": 0, "save_bits": list(range(104, 112)),
                "portraits": [{"name": portrait, "filename": f"{portrait}.png",
                              "decoded_pixels_sha256": hashlib.sha256(image.tobytes()).hexdigest()}
                             for portrait, image in zip(PORTRAITS, images)]}
    return images, ("\n".join(source)).encode(), (json.dumps(manifest, indent=2) + "\n").encode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    images, data, manifest = build()
    outputs = {ENGINE / "include/td_story_data.h": data, ROOT / "content/story_art.json": manifest}
    if args.check:
        for path, expected in outputs.items():
            assert path.read_bytes() == expected, ("Stale original story data", path)
        for portrait, expected in zip(PORTRAITS, images):
            with Image.open(ART / f"{portrait}.png") as actual:
                assert actual.mode == expected.mode and actual.size == expected.size
                assert actual.getpalette() == expected.getpalette() and actual.tobytes() == expected.tobytes()
    else:
        ART.mkdir(parents=True, exist_ok=True)
        for path, content in outputs.items():
            path.write_bytes(content)
        for portrait, image in zip(PORTRAITS, images):
            image.save(ART / f"{portrait}.png", compress_level=9)
    metadata = json.loads(manifest)
    print(f"Original newcomer story:8 scenes/{metadata['story_pages']} pages; "
          f"{metadata['tile_count']} native tiles16..{metadata['last_tile']}, palette7, zero new OAM/persistent bytes")


if __name__ == "__main__":
    main()
