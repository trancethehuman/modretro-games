"""Sanitize the actual native flyby renderer against independent pixel/OAM oracles.

Host VRAM adapters do not establish Game Boy timing or hardware performance.
Original PNG pixels supply masks independently of the renderer's row operations.
"""
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import tempfile

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
ENGINE = GAME / "project/plugins/toronto-driving/engine"


def fixture():
    image = Image.open(GAME / "project/original-art/ambient_aircraft.png").convert("RGB")
    colours = [(0x65, 0xff, 0), (0xe0, 0xf8, 0xcf), (0x86, 0xc0, 0x6c), (7, 0x18, 0x21)]
    pixels = [colours.index(image.getpixel((x, y))) for y in range(32)
              for frame in range(18) for x in range(frame * 32, frame * 32 + 32)]
    # A conventional frame-major image, not the generator's reconstruction.
    rows = ["static const UBYTE original_pixels[18][32][32]={"]
    for frame in range(18):
        rows += ["{"] + ["{" + ",".join(str(colours.index(image.getpixel((frame * 32 + x, y))))
                                               for x in range(32)) + "}," for y in range(32)] + ["},"]
    rows += ["};"]
    assert len(pixels) == 18 * 32 * 32
    return "\n".join(rows)


def check_actor_override():
    source = (ENGINE / "src/core/actor.c").read_text()
    tail = source[source.index("#pragma bank 255"):]
    player_before="            td_actor_render_actor(&PLAYER);"
    player_after="""            SWITCH_ROM(PLAYER.sprite.bank);
            spritesheet_t *sprite = PLAYER.sprite.ptr;

            allocated_hardware_sprites += move_metasprite(
                *(sprite->metasprites + PLAYER.frame),
                PLAYER.base_tile,
                allocated_hardware_sprites,
                screen_x,
                screen_y
            );"""
    actor_before="        td_actor_render_actor(actor);"
    actor_after="""        SWITCH_ROM(actor->sprite.bank);
        spritesheet_t *sprite = actor->sprite.ptr;

        allocated_hardware_sprites += move_metasprite(
            *(sprite->metasprites + actor->frame),
            actor->base_tile,
            allocated_hardware_sprites,
            screen_x,
            screen_y
        );"""
    ground_before='    // Stable runtime slot order: fleet/parked car, people, tram, then marker.\n    // The stock activation-list order can rotate when actors enter/leave view.\n    ground_count=actors_len<MAX_ACTORS?actors_len:MAX_ACTORS;\n    for (ground_index = 2; ground_index <= ground_count; ground_index++) {\n        actor = &actors[ground_index == ground_count ? 1 : ground_index];\n        if (!CHK_FLAG(actor->flags, ACTOR_FLAG_ACTIVE) ||\n            CHK_FLAG(actor->flags, ACTOR_FLAG_HIDDEN | ACTOR_FLAG_DISABLED)) {\n           continue;\n        }\n\n        if (CHK_FLAG(actor->flags, ACTOR_FLAG_PINNED)) {\n            screen_x = SUBPX_TO_PX(actor->pos.x);\n            screen_y = SUBPX_TO_PX(actor->pos.y);\n        } else {\n            screen_x = SUBPX_TO_PX(actor->pos.x) - draw_scroll_x;\n            screen_y = SUBPX_TO_PX(actor->pos.y) - draw_scroll_y;\n        }\n\n        if (((window_hide_actors) && (((screen_x + 8) > WX_REG) && ((screen_y - 8) > WY_REG)))) {\n            continue;\n        }\n        td_actor_render_actor(actor);\n    }\n'
    order_before="""    // Stable runtime slot order: fleet/parked car, people, tram, then marker.
    // The stock activation-list order can rotate when actors enter/leave view.
    ground_count=actors_len<MAX_ACTORS?actors_len:MAX_ACTORS;
    for (ground_index = 2; ground_index <= ground_count; ground_index++) {
        actor = &actors[ground_index == ground_count ? 1 : ground_index];
        if (!CHK_FLAG(actor->flags, ACTOR_FLAG_ACTIVE) ||
            CHK_FLAG(actor->flags, ACTOR_FLAG_HIDDEN | ACTOR_FLAG_DISABLED)) {"""
    order_after="""    // Render all actors
    for (actor = PLAYER.prev; (actor); actor = actor->prev){
        if (CHK_FLAG(actor->flags, ACTOR_FLAG_HIDDEN | ACTOR_FLAG_DISABLED)) {"""
    assert tail.count("    td_actor_render_ground(window_hide_actors);\n") == 1
    helper = (ENGINE / "src/td_actor_render.c").read_text()
    expected_ground = "void td_actor_render_ground(UBYTE window_hide_actors) BANKED {\n    actor_t *actor;\n    UBYTE ground_index,ground_count;\n" + ground_before.replace("td_actor_render_actor(actor);", "td_actor_render_actor_local(actor);") + "\n}\n"
    assert helper.count(expected_ground) == 1, "Banked ground helper must retain the exact approved traversal and clipping logic"
    assert "SWITCH_ROM" not in expected_ground, "Banked ground traversal must never switch its executing code bank"
    tail = tail.replace("    td_actor_render_ground(window_hide_actors);\n", ground_before)
    assert tail.count("    UBYTE _save = CURRENT_BANK;\n") == 1
    tail = tail.replace("    UBYTE _save = CURRENT_BANK;\n", "    UBYTE _save = CURRENT_BANK;\n    static actor_t *actor;\n    UBYTE ground_index,ground_count;\n")
    edits = [('#include "td_scenery.h"\n', ""),
 ("#include \"td_aircraft_render.h\"\n", ""),
             ('#include "data_manager.h"\n', ""),
             ("    td_actor_render_before();\n\n", ""),

             ('#include "td_boats.h"\n', ""),
             ('#include "td_sandbox.h"\n', ""),
             ('#include "td_traffic_lights.h"\n', ""),

             ("    if(td_boats_controlled())td_boats_render();\n\n", ""),


             ('#include "td_actor_render.h"\n', ""),
             ("    td_actor_render_after();\n", ""),
             ("    UBYTE ground_index,ground_count;\n", ""),
             (player_before, player_after),(actor_before,actor_after),(order_before,order_after)]
    for before, after in edits:
        assert tail.count(before) == 1, "Actor override must contain each scoped living-city addition exactly once"
        tail = tail.replace(before, after)
    # The pinned stock override omitted its terminal newline. Normalize that
    # formatting byte only; every remaining audited stock byte stays exact.
    assert hashlib.sha256(tail.removesuffix("\n").encode()).hexdigest() == "b7360f4e84c720090e127aa047daa21f7d70517a4a2d9512d2a96f568e9ee5f5", \
        "Actor override changed beyond the audited living-city and signed-admission hooks"
    assert "Copyright (c) 2020 Toxa" in source and "THE SOFTWARE IS PROVIDED" in source


def main():
    check_actor_override()
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; flyby renderer checks did not run")
    with tempfile.TemporaryDirectory(prefix="toronto-aircraft-render-") as directory:
        work = Path(directory)
        shutil.copyfile(ENGINE / "src/td_aircraft_render.c", work / "renderer_under_test.c")
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text("""#ifndef HOST_AIRCRAFT_PLATFORM_H
#define HOST_AIRCRAFT_PLATFORM_H
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
        (work / "bankdata.h").write_text('#include "host_aircraft.h"\n')
        for name in ["actor", "data_manager", "gbs_types", "scroll", "shadow", "compat", "ui"]:
            (work / f"{name}.h").write_text('#include "host_aircraft.h"\n')
        (work / "td_district.h").write_text('#include "host_aircraft.h"\n#define TD_DISTRICT_COUNT 4\nUBYTE td_district_current(void);\n')
        (work / "host_aircraft.h").write_text("""#ifndef HOST_AIRCRAFT_H
#define HOST_AIRCRAFT_H
#include <gbdk/platform.h>
typedef struct { UBYTE bank; const void *ptr; } far_ptr_t;
typedef struct { BYTE dy,dx; UBYTE dtile,props; } metasprite_t;
#define metasprite_end (-128)
typedef struct { UBYTE y,x,tile,prop; } OAM_item_t;
typedef struct { UWORD n_tiles; UBYTE tiles[4096]; } tileset_t;
typedef struct { UBYTE n_metasprites; const metasprite_t *const *metasprites; far_ptr_t tileset,cgb_tileset; } spritesheet_t;
typedef struct actor actor_t;
struct actor { actor_t *prev,*next; UBYTE flags,base_tile; far_ptr_t sprite; };
#define ACTOR_FLAG_ACTIVE 32
#define ACTOR_FLAG_HIDDEN 2
extern actor_t actors[22],*actors_inactive_head;
extern UBYTE actors_len,allocated_hardware_sprites,VBK_REG,__render_shadow_OAM;
extern UBYTE win_pos_x,win_pos_y,win_dest_pos_x,win_dest_pos_y,WX_REG,WY_REG;
extern volatile OAM_item_t shadow_OAM[40],shadow_OAM2[40];
extern WORD draw_scroll_x,draw_scroll_y;
extern far_ptr_t current_scene;
extern UBYTE image_attr_bank,image_tile_width,image_tile_height;
extern UBYTE *image_attr_ptr;
void deactivate_actor(actor_t *actor);
void MemcpyBanked(void *dest,const void *src,size_t length,UBYTE bank);
UBYTE ReadBankedUBYTE(const UBYTE *src,UBYTE bank);
UBYTE *GetBkgAddr(void);
UBYTE get_vram_byte(UBYTE *addr);
void set_vram_byte(UBYTE *addr,UBYTE value);
void get_bkg_data(UBYTE first,UBYTE count,UBYTE *data);
void set_bkg_data(UBYTE first,UBYTE count,const UBYTE *data);
void get_sprite_data(UBYTE first,UBYTE count,UBYTE *data);
#endif
""")
        (work / "art_oracle.h").write_text(fixture())
        binary = work / "aircraft-render-regressions"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-DACTOR_H", "-Wno-pointer-to-int-cast",
                        "-fsanitize=address,undefined", "-I", str(work), "-I", str(ENGINE / "include"),
                        str(ROOT / "tests/engine/aircraft_render_harness.c"), "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == "__main__":
    main()
