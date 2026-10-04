"""Sanitize unchanged native boat C against original pixels and OAM oracles.

The integration checks catch lost loader bindings before actor cloning. Host
banked-ROM/VRAM adapters establish logic, not GBDK ABI, CPU timing or hardware.
Actual authored collision grids are checked by the sprite generator separately.
"""
from pathlib import Path
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
ENGINE = GAME / "project/plugins/toronto-driving/engine"


def function_body(source, name):
    source = re.sub(r"/\*.*?\*/|//[^\n]*", "", source, flags=re.S)
    match = re.search(r"\bvoid\s+" + re.escape(name) + r"\s*\([^)]*\)[^{]*\{", source)
    assert match, f"Missing native function: {name}"
    start, depth = match.end(), 1
    for index in range(start, len(source)):
        depth += (source[index] == "{") - (source[index] == "}")
        if not depth:
            return source[start:index]
    raise AssertionError(f"Unclosed native function: {name}")


def check_integration():
    init = function_body((ENGINE / "src/states/TORONTO.c").read_text(), "toronto_init")
    assert init.count("td_boats_bind();") == 1, "Capture exactly one authored boat loader"
    binding = init.index("td_boats_bind();")
    clone = re.search(r"actors\s*\[\s*i\s*\]\s*=\s*PLAYER", init)
    assert clone and binding < clone.start(), "Boat binding must precede actor cloning"
    assert not re.search(r"\btd_boats_reset\s*\(", init[binding:]), \
        "Reset after bind erases the captured native sprite and scene"
    bind = function_body((ENGINE / "src/td_boats.c").read_text(), "td_boats_bind")
    assert bind.count("td_boats_reset();") == 1 and bind.index("td_boats_reset();") < bind.index("loader=&actors[index]"), \
        "Boat bind must reset before capturing the new scene loader"
    render = function_body((ENGINE / "src/core/actor.c").read_text(), "actors_render")
    assert render.count("td_boats_render();") == render.count("td_aircraft_render();") == 1
    assert render.index("td_boats_render();") < render.index("td_aircraft_render();"), \
        "Aircraft OAM admission must include the previously appended boat"

    # The appended pedestrian scene still captures the empty resource loader,
    # but must not invent an Island boat route or displace mainland loaders.
    world = json.loads((GAME / "content/districts/world.json").read_text())
    expected_scenes = ("toronto_city", "toronto_west", "toronto_high_park",
                       "toronto_east", "toronto_port_lands", "toronto_islands")
    assert [(row["id"], row["scene"]) for row in world["districts"]] == \
        list(enumerate(expected_scenes)), "Review boat integration when registered district identities change"
    sprite_id = "22789632-122e-5a80-a195-5c8c6e7caeba"
    loader_ids = set()
    for district, name in enumerate(expected_scenes):
        directory = GAME / "project/project/scenes" / name
        scene = json.loads((directory / "scene.gbsres").read_text())
        actors = [json.loads(path.read_text()) for path in (directory / "actors").glob("*.gbsres")]
        loaders = [actor for actor in actors if actor["spriteSheetId"] == sprite_id]
        assert len(loaders) == 1, f"{name}: capture exactly one authored boat loader"
        loader = loaders[0]
        expected_index = 4 if district in (0, 1, 3) else 3
        assert loader["_index"] == expected_index, f"{name}: native boat loader slot changed"
        assert loader["frame"] == 2 and not loader["animate"] and not loader["persistent"], \
            f"{name}: boat loader must remain empty and stationary"
        assert all(not loader[field] for field in ("script", "startScript", "updateScript",
                                                   "hit1Script", "hit2Script", "hit3Script")), \
            f"{name}: resource loader gained gameplay scripts"
        assert scene["spritePaletteIds"][7] == "4f09c0e2-01e7-57a7-9868-ffdc3d84cc79", \
            f"{name}: boat palette ownership changed"
        assert loader["id"] not in loader_ids, "Boat loader resource identities must be unique"
        loader_ids.add(loader["id"])
    art = json.loads((GAME / "project/original-art/ambient_boat_art.json").read_text())
    assert art["fictional_routes"] == [{"district": 0, "u": 560, "top": 840, "bottom": 896},
                                      {"district": 4, "u": 464, "top": 64, "bottom": 432}], \
        "Core harbour and Port water lanes must stay unchanged; Islands have no boat route"
    assert art["port_deck_clip_rectangles"] == [[416, 96, 96, 64], [416, 256, 96, 64]]
    assert art["water_full_hull_checks"] == 54528


def art_fixture():
    image = Image.open(GAME / "project/original-art/ambient_boat.png").convert("RGB")
    colours = [(101, 255, 0), (224, 248, 207), (134, 192, 108), (7, 24, 33)]
    rows = ["static const UBYTE original_pixels[16][8]={"]
    rows.extend("{" + ",".join(str(colours.index(image.getpixel((x, y)))) for x in range(8)) + "},"
                for y in range(16))
    return "\n".join(rows + ["};"])


def main():
    check_integration()
    subprocess.run([sys.executable, str(GAME / "scripts/create_boat_sprite.py"), "--check"], check=True)
    imported = json.loads((GAME / "project/assets/sprites/ambient_boat.png.gbsres").read_text())
    assert imported["id"] == "22789632-122e-5a80-a195-5c8c6e7caeba"
    assert (GAME / "project/assets/sprites/ambient_boat.png").read_bytes() == \
        (GAME / "project/original-art/ambient_boat.png").read_bytes(), "Imported boat pixels differ"
    district_count = int(re.search(r"#define TD_DISTRICT_COUNT (\d+)",
                                  (ENGINE / "include/td_district.h").read_text()).group(1))
    assert district_count == 6, "Review loader slots and water routes when districts change"
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; boat checks did not run")
    with tempfile.TemporaryDirectory(prefix="toronto-boats-") as directory:
        work = Path(directory)
        shutil.copyfile(ENGINE / "src/td_boats.c", work / "boats_under_test.c")
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text("""#ifndef HOST_BOAT_PLATFORM_H
#define HOST_BOAT_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
typedef uint8_t UBYTE;
typedef int8_t BYTE;
typedef uint16_t UWORD;
typedef int16_t WORD;
#define BANKED
#define TRUE 1
#define FALSE 0
#define CGB 1
#endif
""")
        for name in ["bankdata", "actor", "data_manager", "gbs_types", "scroll", "shadow", "ui", "compat"]:
            (work / f"{name}.h").write_text('#include "host_boats.h"\n')
        (work / "td_district.h").write_text('#include "host_boats.h"\n' +
                                          f'#define TD_DISTRICT_COUNT {district_count}\n' +
                                          'UBYTE td_district_current(void);\n')
        (work / "host_boats.h").write_text("""#ifndef HOST_BOATS_H
#define HOST_BOATS_H
#include <gbdk/platform.h>
typedef struct { UBYTE bank; const void *ptr; } far_ptr_t;
typedef struct { BYTE dy,dx; UBYTE dtile,props; } metasprite_t;
#define metasprite_end (-128)
typedef struct { UBYTE y,x,tile,prop; } OAM_item_t;
typedef struct { UWORD n_tiles; UBYTE tiles[2048]; } tileset_t;
typedef struct { UBYTE n_metasprites; const metasprite_t *const *metasprites; far_ptr_t tileset,cgb_tileset; } spritesheet_t;
typedef struct actor actor_t;
struct actor { actor_t *prev,*next; UBYTE flags,base_tile; far_ptr_t sprite; };
#define ACTOR_FLAG_ACTIVE 32
#define ACTOR_FLAG_HIDDEN 2
extern actor_t actors[21],*actors_inactive_head;
extern UBYTE actors_len,allocated_hardware_sprites,VBK_REG,__render_shadow_OAM;
extern UBYTE win_pos_x,win_pos_y,win_dest_pos_x,win_dest_pos_y,WX_REG,WY_REG;
extern volatile OAM_item_t shadow_OAM[40],shadow_OAM2[40];
extern WORD draw_scroll_x,draw_scroll_y;
extern far_ptr_t current_scene;
void deactivate_actor(actor_t *actor);
void MemcpyBanked(void *dest,const void *src,size_t length,UBYTE bank);
UBYTE ReadBankedUBYTE(const UBYTE *src,UBYTE bank);
void set_sprite_data(UBYTE first,UBYTE count,const UBYTE *pixels);
#endif
""")
        (work / "boat_art_oracle.h").write_text(art_fixture())
        binary = work / "boat-regressions"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-Wno-pointer-to-int-cast", "-fsanitize=address,undefined",
                        "-I", str(work), "-I", str(ENGINE / "include"),
                        str(ROOT / "tests/engine/boats_harness.c"), "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == "__main__":
    main()
