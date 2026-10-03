"""Exercise the unchanged native atlas API against independent source oracles.

This reads registered scene collisions and authored geographic water metadata,
then compiles the actual td_atlas.c and available data units with host adapters. Native
2bpp bytes are decoded independently; no generator rendering functions, plugin
calls, ROM builds, Game Boy timing or hardware claims are involved.
"""
from pathlib import Path
import json
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
ENGINE = GAME / "project/plugins/toronto-driving/engine"
HARNESS = ROOT / "tests/engine/atlas_harness.c"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def collision_bytes(encoded, length):
    """Parse the documented native byte-run format without generator imports."""
    tokens = list(re.finditer(r"([0-9a-fA-F]{2})(!|[0-9a-fA-F]+\+)", encoded))
    result, end = [], 0
    for token in tokens:
        require(token.start() == end, "Collision byte runs contain an undecodable gap.")
        value, run = int(token[1], 16), token[2]
        count = 1 if run == "!" else int(run[:-1], 16)
        require(0 < count <= length - len(result), "Collision run is empty or exceeds the registered grid.")
        result.extend([value] * count)
        end = token.end()
    require(end == len(encoded) and len(result) == length,
            "Collision byte runs do not exactly fill the registered scene.")
    return result


def inside_box(point, box):
    x, y = point
    left, top, right, bottom = box
    return left <= x < right and top <= y < bottom


def inside_polygon(point, vertices):
    """Integer winding number, independent of the generator's ray casting."""
    x, y = point
    winding = 0
    for index, (ax, ay) in enumerate(vertices):
        bx, by = vertices[(index + 1) % len(vertices)]
        side = (bx - ax) * (y - ay) - (x - ax) * (by - ay)
        if side == 0 and min(ax, bx) <= x <= max(ax, bx) and min(ay, by) <= y <= max(ay, by):
            return True
        if ay <= y < by and side > 0:
            winding += 1
        elif by <= y < ay and side < 0:
            winding -= 1
    return winding != 0


def authored_water(scene, point, metadata):
    x, y = point
    if scene == "toronto_city":
        river, mainland = metadata["river"], metadata["mainland"]
        don = inside_box(point, (river[0], mainland[1], river[1], mainland[3]))
        lake = y >= mainland[3] and not any(inside_box(point, island) for island in metadata["islands"])
        return don or lake
    rectangles = [(left, top, left + width, top + height)
                  for left, top, width, height in metadata["water"]]
    return (any(inside_box((x, y), box) for box in rectangles) or
            bool(metadata.get("pond") and inside_polygon(point, metadata["pond"])))


def fixture_header(registered_islands=True):
    world = json.loads((GAME / "content/districts/world.json").read_text())
    districts = world["districts"]
    count = re.findall(r"^#define TD_DISTRICT_COUNT (\d+)$",
                       (ENGINE / "include/td_district.h").read_text(), re.M)
    require(len(count) == 1 and len(districts) == int(count[0]) and
            [district["id"] for district in districts] == list(range(len(districts))),
            "Atlas oracle requires the actual registered district order/count.")
    require(4 <= len(districts) <= 32 and
            [district['scene'] for district in districts[:4]] ==
            ['toronto_city', 'toronto_west', 'toronto_high_park', 'toronto_east'] and
            [(district['atlas_x'], district['atlas_y']) for district in districts[:4]] ==
            [(2048, 0), (1024, 0), (0, 0), (3072, 0)],
            "Existing atlas district identities and placement must remain append-only.")
    actual_width = max(district["atlas_x"] + district["width_pixels"] for district in districts) // 8
    actual_height = max(district["atlas_y"] + district["height_pixels"] for district in districts) // 8
    require(20 <= (actual_width + 7) // 8 <= 255 and 12 <= (actual_height + 7) // 8 <= 255,
            "Oracle dimensions exceed the public native UBYTE atlas interface.")
    padded_width = (actual_width + 7) // 8 * 8
    padded_height = (actual_height + 7) // 8 * 8
    pixels = [0] * (padded_width * padded_height)
    occupied = set()
    metas = []
    classes = [0] * 4
    for district in districts:
        scene_name = district["scene"]
        require(re.fullmatch(r'toronto_[a-z0-9_]+', scene_name), "Invalid registered scene identity.")
        scene = json.loads((GAME / "project/project/scenes" / scene_name / "scene.gbsres").read_text())
        require((scene["width"], scene["height"]) == (128, 122) and scene["symbol"] == district["symbol"],
                "Atlas oracle scene dimensions/identity disagree with registered native resources.")
        require((district["width_pixels"], district["height_pixels"]) == (1024, 976) and
                all(type(district[field]) is int and district[field] >= 0 and district[field] % 8 == 0
                    for field in ("atlas_x", "atlas_y")), "Atlas placement must be nonnegative and tile aligned.")
        collisions = collision_bytes(scene["collisions"], 128 * 122)
        require(set(collisions) <= {0, 15, 16}, "Review newly introduced collision classes before assigning atlas colours.")
        # Independently read the declared source path rather than importing
        # the production generator's metadata resolver.
        metadata_path = district.get('art_source', 'content/city_art.json' if district['id'] == 0 else (
            'content/districts/' + scene_name.removeprefix('toronto_') + '_art.json'))
        require(isinstance(metadata_path, str) and metadata_path.startswith('content/') and
                not Path(metadata_path).is_absolute() and '..' not in Path(metadata_path).parts and
                (GAME / metadata_path).resolve().is_relative_to((GAME / 'content').resolve()),
                'Atlas water source must be a confined public content path.')
        metadata = json.loads((GAME / metadata_path).read_text())
        require(metadata['dimensions'] == [1024, 976], "Atlas oracle water metadata dimensions disagree.")
        offset_x, offset_y = district["atlas_x"] // 8, district["atlas_y"] // 8
        name = district["name"]
        require(name.isascii() and len(name) <= 18, "District name must fit the native 19-byte output.")
        metas.append("{%d,%d,%d,%d,%d,%s}" %
                     (district["id"], offset_x, offset_y, 1024, 976, json.dumps(name)))
        for position, collision in enumerate(collisions):
            tile_x, tile_y = position % 128, position // 128
            atlas_x, atlas_y = offset_x + tile_x, offset_y + tile_y
            require((atlas_x, atlas_y) not in occupied, "Registered atlas districts overlap.")
            occupied.add((atlas_x, atlas_y))
            # Road and walking permissions take precedence over rivers/ponds:
            # a bridge remains visible land even when its centre is wet.
            colour = 1 if collision == 0 else 2 if collision == 16 else (
                3 if authored_water(scene_name, (tile_x * 8 + 4, tile_y * 8 + 4), metadata) else 0)
            pixels[atlas_y * padded_width + atlas_x] = colour
            classes[colour] += 1
    require(len(occupied) == len(districts) * 128 * 122 and all(classes),
            "The oracle must cover each registered district and all four pixel classes.")
    # Check the independent water oracle at real geographic boundary examples.
    core = json.loads((GAME / "content/city_art.json").read_text())
    require(authored_water("toronto_city", (892, 100), core) and
            authored_water("toronto_city", (400, 924), core) and
            authored_water("toronto_city", (620, 924), core),
            "Core water oracle lost the Don River or kept removed duplicate Island ground.")
    if registered_islands:
        island = next((d for d in districts if d['id'] == 5), None)
        require(island is not None and island['scene'] == 'toronto_islands' and
                (island['atlas_x'], island['atlas_y']) == (2048, 976),
                'Fuller Islands must occupy its registered north-up cell without a road seam.')
        island_art = json.loads((GAME / island['art_source']).read_text())
        require(authored_water('toronto_islands', (20, 20), island_art) and
                not authored_water('toronto_islands', (512, 448), island_art),
                'Independent Island water oracle must distinguish lake from the Centre dock.')
    hp = json.loads((GAME / "content/districts/high_park_art.json").read_text())
    require(inside_polygon((650, 550), hp["pond"]) and not inside_polygon((750, 550), hp["pond"]),
            "Independent pond oracle does not distinguish land and water.")
    rows = ["typedef struct { UBYTE id; UWORD x,y,width,height; const char *name; } oracle_district_t;",
            f"#define ORACLE_DISTRICT_COUNT {len(districts)}",
            f"#define ORACLE_WIDTH {actual_width}", f"#define ORACLE_HEIGHT {actual_height}",
            f"#define ORACLE_PADDED_WIDTH {padded_width}",
            f"#define ORACLE_PADDED_HEIGHT {padded_height}",
            "static const oracle_district_t oracle_districts[]={" + ",".join(metas) + "};",
            "static const UBYTE oracle_pixels[ORACLE_PADDED_WIDTH*ORACLE_PADDED_HEIGHT]={"]
    rows += [",".join(map(str, pixels[offset:offset + padded_width])) + ","
             for offset in range(0, len(pixels), padded_width)]
    rows += ["};", ""]
    return "\n".join(rows)


def main():
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; atlas regressions did not run.")
    source = ENGINE / "src/td_atlas.c"
    header = ENGINE / "include/td_atlas.h"
    require(source.is_file() and header.is_file(), "Actual atlas source/API is unavailable; regressions did not run.")
    oracle = fixture_header()
    with tempfile.TemporaryDirectory(prefix="toronto-atlas-tests-") as directory:
        work = Path(directory)
        shutil.copyfile(source, work / "atlas_under_test.c")
        (work / "atlas_oracle.h").write_text(oracle)
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text("""#ifndef HOST_ATLAS_PLATFORM_H
#define HOST_ATLAS_PLATFORM_H
#include <stdint.h>
typedef uint8_t UBYTE;
typedef int8_t BYTE;
typedef uint16_t UWORD;
typedef int16_t WORD;
#define BANKED
#define TRUE 1
#define FALSE 0
#endif
""")
        (work / "bankdata.h").write_text("""#ifndef HOST_ATLAS_BANKDATA_H
#define HOST_ATLAS_BANKDATA_H
#include <gbdk/platform.h>
typedef struct { UBYTE bank; const void *ptr; } far_ptr_t;
#endif
""")
        binary = work / "atlas-regressions"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-fsanitize=address,undefined",
                        "-I", str(work), "-I", str(ENGINE / "include"),
                        str(HARNESS),
                        *map(str, sorted(ENGINE.glob('src/td_atlas_patterns_*.c'))),
                        *map(str, sorted(ENGINE.glob('src/td_atlas_rows_*.c'))),
                        "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == "__main__":
    main()
