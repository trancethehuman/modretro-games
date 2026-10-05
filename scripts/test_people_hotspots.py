"""Sanitize actual people clock/contact optimization against independent oracles."""
from pathlib import Path
import importlib.util
import json
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "games/toronto-dispatch/project/plugins/toronto-driving/engine"


def terrain_fixture():
    game=ROOT/"games/toronto-dispatch"
    spec=importlib.util.spec_from_file_location("people_raw_collision_decoder",game/"scripts/check_campaign.py")
    validator=importlib.util.module_from_spec(spec);spec.loader.exec_module(validator)
    world=json.loads((game/"content/districts/world.json").read_text())
    rows=[]
    for district,entry in enumerate(world["districts"]):
        assert entry["id"]==district
        scene=json.loads((game/"project/project/scenes"/entry["scene"]/"scene.gbsres").read_text())
        assert (scene["width"],scene["height"])==(128,122)
        tiles=validator.decode(scene["collisions"])
        assert len(tiles)==15616 and all(0<=value<=255 for value in tiles)
        rows.append("{"+",".join(map(str,tiles))+"}")
    return "static const UBYTE people_raw_terrain[TD_DISTRICT_COUNT][15616]={"+",".join(rows)+"};\n"


def walkable_query():
    source=(ENGINE/"src/td_roads.c").read_text()
    match=re.search(r"(?m)^UBYTE td_road_walkable\([^;]*?\)\s*BANKED\s*\{",source)
    assert match
    end,depth=match.end(),1
    while depth:
        if source[end]=="{":depth+=1
        elif source[end]=="}":depth-=1
        end+=1
    return source[match.start():end]


def rail_queries():
    """Extract unchanged query/helper bodies without the unrelated actor/scene ABI.

    The complete runtime is exercised by the repository's full engine harness.
    This focused unit links these exact bodies to actual timetable/sweep C.
    """
    source = (ENGINE / "src/td_streetcar_runtime.c").read_text()
    functions = []
    for name in ("mode", "box", "overlap", "near", "clear", "held", "held_body",
                 "held_clear", "foot_clear", "pedestrian_clear"):
        name = "td_streetcar_runtime_" + name
        match = re.search(r"(?m)^(?:static )?UBYTE " + name + r"\([^;]*?\)\s*(?:BANKED\s*)?\{", source)
        if not match:
            raise ValueError(f"Missing actual production query {name}")
        end, depth = match.end(), 1
        while depth:
            if source[end] == "{":
                depth += 1
            elif source[end] == "}":
                depth -= 1
            end += 1
        functions.append(source[match.start():end])
    return "\n\n".join(functions)


def route_queries():
    """Link exact production route selection alongside the fixture selector.

    Rename only its exported symbol to avoid the fixture's controlled routes;
    bodies, constants, visible-route query and authored ROM arrays stay actual.
    """
    source = (ENGINE / "src/td_routes.c").read_text()
    functions = []
    for name in ("td_route_distance", "td_route_position", "td_route_first", "td_refresh_routes"):
        match = re.search(r"(?m)^(?:static )?(?:UBYTE|UWORD|void) " + name +
                          r"\([^;]*?\)\s*(?:BANKED\s*)?\{", source)
        if not match:
            raise ValueError(f"Missing actual production route query {name}")
        end, depth = match.end(), 1
        while depth:
            if source[end] == "{":
                depth += 1
            elif source[end] == "}":
                depth -= 1
            end += 1
        functions.append(source[match.start():end])
    return "\n\n".join(functions).replace("void td_refresh_routes(", "void td_test_refresh_routes(")


def main():
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; people hotspot checks did not run")
    with tempfile.TemporaryDirectory(prefix="toronto-people-hotspots-") as directory:
        work = Path(directory)
        people=(ENGINE / "src/td_people.c").read_text()
        read='fu=car->pos.x>>1;fv=car->pos.y>>1;'
        assert people.count(read)==1
        (work/"people_under_test.c").write_text(people.replace(read,'host_people_word_pairs++;'+read))
        (work/"people_raw_terrain.h").write_text(terrain_fixture())
        (work/"walkable_query_under_test.c").write_text(walkable_query())
        (work / "rail_queries_under_test.c").write_text(rail_queries())
        routes=route_queries()
        loop='for(j=first;j<td_route_counts[district];j++){'
        assert routes.count(loop)==1
        (work/"route_queries_under_test.c").write_text(routes.replace(loop,loop+'host_route_candidates++;'))
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text("""#ifndef HOST_PEOPLE_PLATFORM_H
#define HOST_PEOPLE_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
typedef uint8_t UBYTE;
typedef int8_t BYTE;
typedef uint16_t UWORD;
typedef int16_t WORD;
#define BANKED
#define TRUE 1
#define FALSE 0
#endif
""")
        (work / "actor.h").write_text("""#ifndef HOST_PEOPLE_ACTOR_H
#define HOST_PEOPLE_ACTOR_H
#include <gbdk/platform.h>
typedef struct { struct { UWORD x,y; } pos; UBYTE flags; } actor_t;
#define ACTOR_FLAG_HIDDEN 2
extern actor_t actors[22];
#endif
""")
        (work / "bankdata.h").write_text("""#ifndef HOST_PEOPLE_BANKDATA_H
#define HOST_PEOPLE_BANKDATA_H
#include <gbdk/platform.h>
typedef struct { UBYTE bank; const void *ptr; } far_ptr_t;
#endif
""")
        (work/"collision.h").write_text("""#include <gbdk/platform.h>
extern UBYTE tile_hit_x,tile_hit_y;
UBYTE tile_at(UBYTE x,UBYTE y);
UBYTE tile_col_test_range_x(UBYTE mask,UBYTE row,UBYTE first,UBYTE last);
UBYTE tile_col_test_range_y(UBYTE mask,UBYTE column,UBYTE first,UBYTE last);
""")
        binary = work / "people-hotspot-regressions"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-DACTOR_H", "-fsanitize=address,undefined", "-I", str(work),
                        "-I", str(ENGINE / "include"), str(ROOT / "tests/engine/people_hotspot_harness.c"),
                        str(ENGINE / "src/td_streetcar.c"), str(ENGINE / "src/td_transit.c"),
                        "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == "__main__":
    main()
