#!/usr/bin/env python3
"""Compile an original ground schematic of the registered native districts.

Standard library only. No scene, asset, contract or collision resource is changed.
--check validates sources, every visible tile budget, and generated files without
writing. This is a source/data check, not a ROM, performance or hardware test.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "project/plugins/toronto-driving/engine"
OUTPUT = ROOT / "content/atlas.json"
HEADER = ENGINE / "include/td_atlas.h"
SOURCE = ENGINE / "src/td_atlas.c"
SCALE = 8
VIEW_WIDTH, VIEW_HEIGHT = 20, 12
VISIBLE_LIMIT = 172  # CGB bank1 tiles16..187; markers8..14 and font192..240.
DATA_LIMIT = 12288  # Leave at least4KiB for banked code in the same16KiB unit.
SOLID, ROAD, WALK, WATER = range(4)
EXPECTED_SCENES = ("toronto_city", "toronto_west", "toronto_high_park", "toronto_east")
EXPECTED_OFFSETS = ((2048, 0), (1024, 0), (0, 0), (3072, 0))


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(path.read_text())


def integer(value, minimum, maximum, label):
    require(type(value) is int and minimum <= value <= maximum, f"Invalid {label}")
    return value


def decode_grid(text, size):
    require(isinstance(text, str) and text, "Missing native collision encoding")
    result, cursor = [], 0
    while cursor < len(text):
        match = re.match(r"([0-9a-fA-F]{2})(!|[0-9a-fA-F]+\+)", text[cursor:])
        require(match is not None, "Malformed native collision RLE")
        value = int(match[1], 16)
        count = 1 if match[2] == "!" else int(match[2][:-1], 16)
        require(value in (0, 16, 15) and 0 < count <= size - len(result),
                "Invalid native collision flag or run length")
        result.extend([value] * count)
        cursor += match.end()
    require(len(result) == size, "Incomplete native collision grid")
    return result


def rectangle(values, width, height, endpoints=False):
    require(isinstance(values, list) and len(values) == 4, "Malformed water/land rectangle")
    x, y, a, b = [integer(v, 0, max(width, height), "rectangle coordinate") for v in values]
    right, bottom = (a, b) if endpoints else (x + a, y + b)
    require(0 <= x < right <= width and 0 <= y < bottom <= height,
            "Water/land rectangle exceeds its district")
    return x, y, right, bottom


def contains(rect, x, y):
    left, top, right, bottom = rect
    return left <= x < right and top <= y < bottom


def polygon_contains(points, x, y):
    """Exact integer ray casting, including a point on an authored boundary."""
    inside = False
    for (ax, ay), (bx, by) in zip(points, points[1:] + points[:1]):
        cross = (bx - ax) * (y - ay) - (x - ax) * (by - ay)
        if cross == 0 and min(ax, bx) <= x <= max(ax, bx) and min(ay, by) <= y <= max(ay, by):
            return True
        if (ay > y) != (by > y) and ((cross > 0) == (by > ay)):
            inside = not inside
    return inside


def water_model(district, metadata):
    """Use authored water only; native road/walk permission overrides this mask."""
    width, height = district["width_pixels"], district["height_pixels"]
    if district["id"] == 0:
        river = metadata.get("river")
        require(isinstance(river, list) and len(river) == 2, "Core river bounds are missing")
        left, right = [integer(v, 0, width, "river coordinate") for v in river]
        require(left < right, "Invalid core river width")
        mainland = rectangle(metadata.get("mainland"), width, height, endpoints=True)
        require(isinstance(metadata.get("islands"), list) and metadata["islands"], "Missing core Island ground")
        islands = [rectangle(r, width, height, endpoints=True) for r in metadata["islands"]]
        shapes = {"river": [left, mainland[1], right, mainland[3]],
                  "harbour_from_y": mainland[3], "island_land_exclusions": [list(r) for r in islands]}

        def wet(x, y):
            return contains(shapes["river"], x, y) or (y >= mainland[3] and not any(contains(r, x, y) for r in islands))
        return shapes, wet

    require(isinstance(metadata.get("water"), list), "Missing authored water rectangles")
    rectangles = [rectangle(r, width, height) for r in metadata["water"]]
    pond = metadata.get("pond", [])
    require(isinstance(pond, list) and (not pond or len(pond) >= 3), "Malformed pond polygon")
    for p in pond:
        require(isinstance(p, list) and len(p) == 2, "Malformed pond vertex")
        integer(p[0], 0, width - 1, "pond x")
        integer(p[1], 0, height - 1, "pond y")
    require(not pond or len(set(map(tuple, pond))) >= 3, "Degenerate pond polygon")
    require(not pond or sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(pond, pond[1:] + pond[:1])) != 0,
            "Pond polygon has no area")
    if district["id"] == 3:
        require(not rectangles and not pond, "East atlas must not invent Don/Port Lands water")
    shapes = {"rectangles": [list(r) for r in rectangles], "pond_polygon": pond}

    def wet(x, y):
        return any(contains(r, x, y) for r in rectangles) or bool(pond and polygon_contains(pond, x, y))
    return shapes, wet


def pack_tile(pixels):
    require(len(pixels) == 64 and all(v in range(4) for v in pixels), "Invalid schematic tile")
    result = []
    for row in range(8):
        low = high = 0
        for value in pixels[row * 8:row * 8 + 8]:
            low = (low << 1) | (value & 1)
            high = (high << 1) | (value >> 1)
        result.extend((low, high))
    return bytes(result)


def unpack_tile(data):
    require(len(data) == 16, "Invalid native2bpp tile length")
    return [((data[y * 2] >> (7 - x)) & 1) | (((data[y * 2 + 1] >> (7 - x)) & 1) << 1)
            for y in range(8) for x in range(8)]


def model():
    world_path = ROOT / "content/districts/world.json"
    world = read(world_path)
    districts = world.get("districts")
    canonical_text = (ENGINE / "include/td_district.h").read_text()
    counts = re.findall(r"^#define\s+TD_DISTRICT_COUNT\s+(\d+)\s*$", canonical_text, re.M)
    require(len(counts) == 1, "Canonical native district count is unavailable")
    for key, expected in (("TD_DISTRICT_PIXEL_WIDTH", 1024), ("TD_DISTRICT_PIXEL_HEIGHT", 976),
                          ("TD_DISTRICT_TILE_WIDTH", 128), ("TD_DISTRICT_TILE_HEIGHT", 122)):
        matches = re.findall(r"^#define\s+" + key + r"\s+(\d+)\s*$", canonical_text, re.M)
        require(len(matches) == 1 and int(matches[0]) == expected, "Atlas/canonical native dimensions disagree")
    require(isinstance(districts, list) and len(districts) == int(counts[0]) == 4,
            "Atlas requires the four actually registered native districts")
    require([d.get("id") for d in districts] == list(range(4)) and all(type(d.get("id")) is int for d in districts),
            "Native district IDs must be unique, contiguous and ordered")
    bounds = []
    for district in districts:
        i = district["id"]
        require(district.get("scene") == EXPECTED_SCENES[i], "Unregistered or reassigned atlas district")
        require(district.get("symbol") == "scene_" + EXPECTED_SCENES[i], "Invalid native scene symbol")
        require((district.get("width_pixels"), district.get("height_pixels")) == (1024, 976),
                "Atlas/native district dimensions disagree")
        x = integer(district.get("atlas_x"), 0, 65535 - 1024, "atlas x")
        y = integer(district.get("atlas_y"), 0, 65535 - 976, "atlas y")
        require(x % SCALE == y % SCALE == 0, "Atlas offsets must align to native tiles")
        bounds.append((x, y, x + 1024, y + 976))
        name = district.get("name")
        require(isinstance(name, str) and 1 <= len(name) <= 18 and all(32 <= ord(c) <= 126 for c in name)
                and '"' not in name and "\\" not in name, "District title exceeds native font bounds")
    for i, a in enumerate(bounds):
        for b in bounds[i + 1:]:
            require(not (a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3]),
                    "Registered atlas districts overlap")
    require(tuple((d["atlas_x"], d["atlas_y"]) for d in districts) == EXPECTED_OFFSETS,
            "Current atlas layout changed; review geography and generated budgets")
    width = max(r[2] for r in bounds) // SCALE
    height = max(r[3] for r in bounds) // SCALE
    tile_width, tile_height = (width + 7) // 8, (height + 7) // 8
    require((width, height, tile_width, tile_height) == (512, 122, 64, 16), "Unexpected native atlas dimensions")
    raster = [[SOLID] * (tile_width * 8) for _ in range(tile_height * 8)]
    resources, compiled_districts = [], []
    for district in districts:
        i = district["id"]
        scene_path = ROOT / "project/project/scenes" / district["scene"] / "scene.gbsres"
        require(scene_path.is_file(), "Atlas district has no registered scene resource")
        scene = read(scene_path)
        require(scene.get("type") == "TORONTO" and scene.get("symbol") == district["symbol"]
                and (scene.get("width"), scene.get("height")) == (128, 122),
                "Unavailable or malformed registered scene")
        grid = decode_grid(scene.get("collisions"), 128 * 122)
        metadata_path = ROOT / ("content/city_art.json" if i == 0 else
                                "content/districts/" + district["scene"].removeprefix("toronto_") + "_art.json")
        metadata = read(metadata_path)
        require(metadata.get("dimensions") == [1024, 976], "Art/water metadata dimensions disagree")
        if i:
            require(metadata.get("collisions") == grid, "Authored and registered district collisions disagree")
        shapes, wet = water_model(district, metadata)
        origin_x, origin_y = district["atlas_x"] // SCALE, district["atlas_y"] // SCALE
        counts = Counter()
        for y in range(122):
            for x in range(128):
                native = grid[y * 128 + x]
                value = ROAD if native == 0 else WALK if native == 16 else WATER if wet(x * 8 + 4, y * 8 + 4) else SOLID
                raster[origin_y + y][origin_x + x] = value
                counts[value] += 1
                require((value == ROAD) == (native == 0) and (value == WALK) == (native == 16),
                        "Atlas changed native ground permissions")
        compiled_districts.append({"id": i, "name": district["name"], "x": origin_x, "y": origin_y,
                                   "width": 128, "height": 122, "water_shapes": shapes,
                                   "pixel_counts": {str(v): counts[v] for v in range(4)}})
        resources.append({"district": i, "scene": str(scene_path.relative_to(ROOT)),
                          "scene_sha256": sha(scene_path.read_bytes()),
                          "collision_encoding_sha256": sha(scene["collisions"].encode()),
                          "collision_bytes_sha256": sha(bytes(grid)),
                          "art_metadata": str(metadata_path.relative_to(ROOT)),
                          "art_metadata_sha256": sha(metadata_path.read_bytes())})

    patterns, indices, lookup = [], [], {}
    for ty in range(tile_height):
        for tx in range(tile_width):
            pixels = [value for row in raster[ty * 8:ty * 8 + 8] for value in row[tx * 8:tx * 8 + 8]]
            packed = pack_tile(pixels)
            require(unpack_tile(packed) == pixels, "Native2bpp tile round trip failed")
            if packed not in lookup:
                lookup[packed] = len(patterns)
                patterns.append(packed)
            indices.append(lookup[packed])
    require(0 < len(patterns) <= 65535 and all(0 <= i < len(patterns) for i in indices),
            "Atlas dictionary indices cannot be represented natively")
    # Prove all225 allowed20x12 viewports, rather than checking district centres.
    visible = []
    for y in range(tile_height - VIEW_HEIGHT + 1):
        visible.append([len({indices[yy * tile_width + xx]
                             for yy in range(y, y + VIEW_HEIGHT) for xx in range(x, x + VIEW_WIDTH)})
                        for x in range(tile_width - VIEW_WIDTH + 1)])
    worst = max(v for row in visible for v in row)
    require(worst <= VISIBLE_LIMIT, "Atlas exceeds the reserved visible ground tile cache")
    pattern_bytes, index_bytes, metadata_bytes = len(patterns) * 16, len(indices) * 2, len(districts) * (4 + 19)
    require(all(n < 16384 for n in (pattern_bytes, index_bytes, metadata_bytes))
            and pattern_bytes + index_bytes + metadata_bytes <= DATA_LIMIT,
            "Atlas data needs another ROM bank; keep4KiB for BANKED code")
    # Reconstruct the complete raster independently from the dictionary layout.
    reconstructed = [[SOLID] * (tile_width * 8) for _ in range(tile_height * 8)]
    for i, pattern in enumerate(indices):
        pixels = unpack_tile(patterns[pattern])
        x, y = (i % tile_width) * 8, (i // tile_width) * 8
        for dy in range(8):
            reconstructed[y + dy][x:x + 8] = pixels[dy * 8:dy * 8 + 8]
    require(reconstructed == raster, "Dictionary schematic differs from source raster")
    require(all(v == SOLID for row in raster[height:] for v in row), "Atlas padding must be solid")
    flat = bytes(v for row in raster[:height] for v in row[:width])
    return {"format": "toronto-native-atlas-1", "status": "Original generated schematic; ROM/UI not yet verified",
            "projection": "North-up ground permissions at one map pixel per native8x8 tile; original compression",
            "scope": "Only four registered scenes, not complete former Toronto coverage or new transit service",
            "scale": SCALE, "width_pixels": width, "height_pixels": height,
            "padded_height_pixels": tile_height * 8, "tile_width": tile_width, "tile_height": tile_height,
            "ground_values": {"solid": SOLID, "road": ROAD, "walk": WALK, "water": WATER},
            "water_rule": "Road0/walk16 override water. Only solid15 uses authored wet tile-centre masks. Rectangles are half-open; pond boundaries inclusive. Core harbour excludes Island land. East has no water.",
            "world_sha256": sha(world_path.read_bytes()), "canonical_district_header_sha256": sha(canonical_text.encode()),
            "sources": resources, "districts": compiled_districts,
            "visual_west_to_east": [d["id"] for d in sorted(compiled_districts, key=lambda d: (d["y"], d["x"]))],
            "raster_values_sha256": sha(flat), "patterns_2bpp_hex": [p.hex() for p in patterns],
            "tile_pattern_indices": indices,
            "budgets": {"dictionary_patterns": len(patterns), "dictionary_bytes": pattern_bytes,
                        "indices_bytes": index_bytes, "metadata_bytes": metadata_bytes,
                        "native_data_bytes": pattern_bytes + index_bytes + metadata_bytes,
                        "data_limit_bytes": DATA_LIMIT, "code_reserve_bytes": 4096,
                        "visible_width_tiles": VIEW_WIDTH, "visible_height_tiles": VIEW_HEIGHT,
                        "visible_pattern_limit": VISIBLE_LIMIT, "worst_visible_patterns": worst,
                        "viewport_pattern_counts": visible, "persistent_wram_bytes": 0,
                        "native_build_verified": False, "ui_render_verified": False}}


def header(data):
    return "\n".join([
        "/* Generated by scripts/create_atlas.py; original native ground schematic. */",
        "#ifndef TD_ATLAS_H", "#define TD_ATLAS_H", "#include <gbdk/platform.h>",
        f"#define TD_ATLAS_SCALE {SCALE}", f"#define TD_ATLAS_WIDTH_PIXELS {data['width_pixels']}",
        f"#define TD_ATLAS_HEIGHT_PIXELS {data['height_pixels']}",
        f"#define TD_ATLAS_TILE_WIDTH {data['tile_width']}", f"#define TD_ATLAS_TILE_HEIGHT {data['tile_height']}",
        f"#define TD_ATLAS_PATTERNS {data['budgets']['dictionary_patterns']}",
        f"#define TD_ATLAS_VIEW_WIDTH {VIEW_WIDTH}", f"#define TD_ATLAS_VIEW_HEIGHT {VIEW_HEIGHT}",
        f"#define TD_ATLAS_VISIBLE_LIMIT {VISIBLE_LIMIT}",
        f"#define TD_ATLAS_WORST_VISIBLE {data['budgets']['worst_visible_patterns']}",
        f"#define TD_ATLAS_SOLID {SOLID}", f"#define TD_ATLAS_ROAD {ROAD}",
        f"#define TD_ATLAS_WALK {WALK}", f"#define TD_ATLAS_WATER {WATER}", "",
        "/* All output buffers must reside in WRAM. FALSE leaves every output unchanged.",
        " * Atlas positions are whole MAP pixels, never GBVM Q5 camera coordinates. */",
        "UBYTE td_atlas_bounds(UWORD *width_pixels,UWORD *height_pixels) BANKED;",
        "/* Local input coordinates are whole WORLD pixels within the registered district. */",
        "UBYTE td_atlas_position(UBYTE district,UWORD local_u,UWORD local_v,UWORD *x,UWORD *y) BANKED;",
        "/* Tile coordinates; count1..20, no row wrapping. IDs index the pattern dictionary. */",
        "UBYTE td_atlas_row(UBYTE tile_x,UBYTE tile_y,UBYTE count,UWORD *patterns) BANKED;",
        "/* One16-byte Game Boy2bpp tile, low/high bitplanes interleaved per row. */",
        "UBYTE td_atlas_pattern(UWORD id,UBYTE *tile16) BANKED;",
        "/* A nineteen-byte terminated name. Padding and unregistered areas return FALSE. */",
        "UBYTE td_atlas_district(UWORD x,UWORD y,char *name19) BANKED;",
        "#endif", ""])


def source(data):
    districts = data["districts"]
    lines = ["/* Generated by scripts/create_atlas.py; source hashes and budgets in content/atlas.json. */",
             "#pragma bank 255", "#include <string.h>", '#include "td_atlas.h"', '#include "td_district.h"',
             f"typedef char td_atlas_registered_count_matches[(TD_DISTRICT_COUNT=={len(districts)})?1:-1];",
             "typedef char td_atlas_registered_dimensions_match[(TD_DISTRICT_PIXEL_WIDTH==1024&&TD_DISTRICT_PIXEL_HEIGHT==976)?1:-1];",
             f"static const UWORD td_atlas_origins[{len(districts)}][2]={{"]
    lines += ["    {" + f"{d['x']},{d['y']}" + "}," for d in districts]
    lines += ["};", f"static const char td_atlas_names[{len(districts)}][19]={{"]
    lines += [f'    "{d["name"]}",' for d in districts]
    lines += ["};", "static const UBYTE td_atlas_tiles[TD_ATLAS_PATTERNS][16]={"]
    lines += ["    {" + ",".join(str(v) for v in bytes.fromhex(p)) + "}," for p in data["patterns_2bpp_hex"]]
    lines += ["};", "static const UWORD td_atlas_map[TD_ATLAS_TILE_WIDTH*TD_ATLAS_TILE_HEIGHT]={"]
    indices = data["tile_pattern_indices"]
    lines += ["    " + ",".join(map(str, indices[i:i + 16])) + "," for i in range(0, len(indices), 16)]
    lines += ["};", "",
              "UBYTE td_atlas_bounds(UWORD *width_pixels,UWORD *height_pixels) BANKED {",
              "    if(!width_pixels||!height_pixels)return FALSE;",
              "    *width_pixels=TD_ATLAS_WIDTH_PIXELS;*height_pixels=TD_ATLAS_HEIGHT_PIXELS;return TRUE;", "}",
              "UBYTE td_atlas_position(UBYTE district,UWORD local_u,UWORD local_v,UWORD *x,UWORD *y) BANKED {",
              "    if(!x||!y||district>=TD_DISTRICT_COUNT||local_u>=TD_DISTRICT_PIXEL_WIDTH||local_v>=TD_DISTRICT_PIXEL_HEIGHT)return FALSE;",
              "    *x=td_atlas_origins[district][0]+(local_u>>3);",
              "    *y=td_atlas_origins[district][1]+(local_v>>3);return TRUE;", "}",
              "UBYTE td_atlas_row(UBYTE tile_x,UBYTE tile_y,UBYTE count,UWORD *patterns) BANKED {",
              "    UBYTE i;UWORD offset;",
              "    if(!patterns||!count||count>TD_ATLAS_VIEW_WIDTH||tile_x>=TD_ATLAS_TILE_WIDTH||tile_y>=TD_ATLAS_TILE_HEIGHT||count>TD_ATLAS_TILE_WIDTH-tile_x)return FALSE;",
              "    offset=((UWORD)tile_y<<6)+tile_x;",
              "    for(i=0;i<count;i++)patterns[i]=td_atlas_map[offset+i];return TRUE;", "}",
              "UBYTE td_atlas_pattern(UWORD id,UBYTE *tile16) BANKED {",
              "    if(!tile16||id>=TD_ATLAS_PATTERNS)return FALSE;",
              "    memcpy(tile16,td_atlas_tiles[id],16);return TRUE;", "}",
              "UBYTE td_atlas_district(UWORD x,UWORD y,char *name19) BANKED {",
              "    UBYTE district;UWORD left,top;",
              "    if(!name19||x>=TD_ATLAS_WIDTH_PIXELS||y>=TD_ATLAS_HEIGHT_PIXELS)return FALSE;",
              "    for(district=0;district<TD_DISTRICT_COUNT;district++){",
              "        left=td_atlas_origins[district][0];top=td_atlas_origins[district][1];",
              "        if(x>=left&&x-left<TD_DISTRICT_PIXEL_WIDTH/TD_ATLAS_SCALE&&y>=top&&y-top<TD_DISTRICT_PIXEL_HEIGHT/TD_ATLAS_SCALE){",
              "            memcpy(name19,td_atlas_names[district],19);return TRUE;", "        }", "    }",
              "    return FALSE;", "}", ""]
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Validate all sources/budgets and deterministic generated output without writes")
    args = parser.parse_args()
    data = model()
    generated = [(OUTPUT, json.dumps(data, indent=2) + "\n"), (HEADER, header(data)), (SOURCE, source(data))]
    for path, expected in generated:
        if args.check:
            require(path.is_file() and path.read_text() == expected, f"Stale generated atlas: {path.relative_to(ROOT)}")
        else:
            path.write_text(expected)
    budget = data["budgets"]
    print(f"Native atlas {'matches' if args.check else 'generated'}:512x122,64x16 tiles,{budget['dictionary_patterns']} patterns; "
          f"every20x12 viewport<={budget['worst_visible_patterns']}/{VISIBLE_LIMIT}; "
          f"{budget['native_data_bytes']}/{DATA_LIMIT} ROM data bytes,no persistent WRAM. ROM/UI proof pending.")


if __name__ == "__main__":
    main()
