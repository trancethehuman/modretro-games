"""Exercise the unchanged native atlas API against independent source oracles.

This reads registered scene collisions and authored geographic water metadata,
then compiles the actual td_atlas.c with host integer/bank adapters only. Native
2bpp bytes are decoded independently; no generator rendering functions, plugin
calls, ROM builds, Game Boy timing or hardware claims are involved.
"""
from pathlib import Path
import json
import sys
import host_cflags
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT
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


def authored_water(old, point, metadata):
    """Water of a plan district at a district-world point: the core's Don,
    harbour and lake around its Islands; elsewhere the authored lake
    rectangles and Grenadier Pond (metadata in district-world pixels)."""
    x, y = point
    if old == 0:
        river, mainland = metadata["river"], metadata["mainland"]
        don = inside_box(point, (river[0], mainland[1], river[1], mainland[3]))
        lake = y >= mainland[3] and not any(inside_box(point, island) for island in metadata["islands"])
        return don or lake
    return (any(inside_box((x, y), box) for box in metadata["water"]) or
            bool(metadata.get("pond") and inside_polygon(point, metadata["pond"])))


def outer_water(old):
    """Plan water and pond of an outer district, in district-world pixels."""
    sys.path.insert(0, str(GAME / "scripts"))
    import outer_art
    plan = json.loads((GAME / f"content/districts/{('core', 'west', 'high_park', 'east')[old]}_art.json").read_text())["plan"]
    X, Y, rect = outer_art.mapper(old)
    return {"water": [(x, y, x + w, y + h) for x, y, w, h in (rect(r) for r in plan.get("water", []))],
            "pond": [[X(x), Y(y)] for x, y in plan.get("pond", [])]}


def fixture_header():
    sys.path.insert(0, str(GAME / "scripts"))
    import world2x
    world = json.loads((GAME / "content/districts/world.json").read_text())
    districts = world["districts"]
    count = re.findall(r"^#define TD_DISTRICT_COUNT (\d+)$",
                       (ENGINE / "include/td_district.h").read_text(), re.M)
    require(len(count) == 1 and len(districts) == int(count[0]) and
            [district["id"] for district in districts] == list(range(len(districts))),
            "Atlas oracle requires the actual registered district order/count.")
    require(len(districts) == 16, "Review the independent sixteen-scene water oracle before changing world coverage.")
    # Districts west to east: High Park, west, core, east, each 2000 x 1904.
    column = {2: 0, 1: 1, 0: 2, 3: 3}
    tw, th = world2x.WORLD_TW, world2x.WORLD_TH
    ground = [[0] * (4 * tw) for _ in range(th)]
    core = json.loads((GAME / "content/city_art.json").read_text())
    for old in range(4):
        metadata = core if old == 0 else outer_water(old)
        # The district's own scenes, stitched at their registered origins.
        for district in districts[old * 4:old * 4 + 4]:
            scene = json.loads((GAME / "project/project/scenes" / district["scene"] / "scene.gbsres").read_text())
            require((scene["width"], scene["height"]) == (128, 122) and scene["symbol"] == district["symbol"],
                    "Atlas oracle scene dimensions/identity disagree with registered native resources.")
            collisions = collision_bytes(scene["collisions"], 128 * 122)
            require(set(collisions) <= {0, 15, 16, 0x2F}, "Review newly introduced collision classes before assigning atlas colours.")
            ox, oy = district["origin"]
            require(district["atlas_x"] == column[old] * world2x.WORLD_W + ox and district["atlas_y"] == oy,
                    "The tested west-to-east district placement must match the registered world.")
            for position, collision in enumerate(collisions):
                x, y = ox // 8 + position % 128, oy // 8 + position // 128
                colour = 1 if collision == 0 else 2 if collision == 16 else (
                    3 if collision == 0x2F or authored_water(old, (x * 8 + 4, y * 8 + 4), metadata) else 0)
                ground[y][column[old] * tw + x] = colour
    # Independent downsampling: the commonest class of each 2 x 2 tiles,
    # road, walk, water, solid on a tie; then a lone non-road pixel takes
    # what three of its four neighbours share (twice).
    width, height = 4 * tw // 2, th // 2
    padded_width, padded_height = 512, (height + 7) // 8 * 8
    pixels = [[0] * padded_width for _ in range(padded_height)]
    for y in range(height):
        for x in range(width):
            votes = [ground[2 * y + dy][2 * x + dx] for dy in (0, 1) for dx in (0, 1)]
            pixels[y][x] = max((1, 2, 3, 0), key=lambda c: (votes.count(c), -(1, 2, 3, 0).index(c)))
    for _ in range(2):
        before = [row[:] for row in pixels]
        for y in range(1, height - 1):
            for x in range(1, width - 1):
                if before[y][x] == 1:
                    continue
                near = [before[y - 1][x], before[y + 1][x], before[y][x - 1], before[y][x + 1]]
                best = max(set(near), key=near.count)
                if near.count(best) >= 3 and best not in (before[y][x], 1):
                    pixels[y][x] = best
    classes = {v for row in pixels for v in row}
    require(classes == {0, 1, 2, 3}, "The oracle must exercise all four pixel classes.")
    # Check the independent water oracle at real geographic boundary examples.
    m = world2x.district_map(0)
    require(authored_water(0, m.point(868, 120), core) and
            not authored_water(0, m.point(400, 924), core) and
            authored_water(0, m.point(620, 924), core),
            "Core water oracle lost the Don River, Island land or intervening lake.")
    hp = outer_water(2)
    mh = world2x.district_map(2)
    require(inside_polygon(mh.point(650, 550), hp["pond"]) and not inside_polygon(mh.point(750, 550), hp["pond"]),
            "Independent pond oracle does not distinguish land and water.")
    metas = ["{%d,%d,%d,%d,%d,%s}" % (d["id"], d["atlas_x"] // 16, d["atlas_y"] // 16, 1024, 976, json.dumps(d["name"]))
             for d in districts]
    rows = ["typedef struct { UBYTE id; UWORD x,y,width,height; const char *name; } oracle_district_t;",
            f"#define ORACLE_DISTRICT_COUNT {len(districts)}",
            f"#define ORACLE_WIDTH {width}", f"#define ORACLE_HEIGHT {height}",
            f"#define ORACLE_ROW {padded_width}", f"#define ORACLE_PADDED_HEIGHT {padded_height}",
            "static const oracle_district_t oracle_districts[]={" + ",".join(metas) + "};",
            "static const UBYTE oracle_pixels[ORACLE_ROW*ORACLE_PADDED_HEIGHT]={"]
    rows += [",".join(map(str, row)) + "," for row in pixels]
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
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror", *host_cflags.extra_flags(compiler),
                        "-Wno-unknown-pragmas", "-fsanitize=address,undefined",
                        "-I", str(work), "-I", str(ENGINE / "include"),
                        str(HARNESS), "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == "__main__":
    main()
