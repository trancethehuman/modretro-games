"""Sanitize exact core scene traversal with actual ground OAM admission.

The core function is extracted byte-for-byte; host bank/register adapters do
not prove cartridge timing. Stale tail actors deliberately remain ACTIVE.
"""
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "games/toronto-dispatch/project/plugins/toronto-driving/engine"


def core_render():
    source = (ENGINE / "src/core/actor.c").read_text()
    match = re.search(r"void actors_render\(void\) NONBANKED \{", source)
    assert match, "Missing actual actors_render"
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


def main():
    core = (ENGINE / "src/core/actor.c").read_text()
    assert re.search(r"\bUBYTE\s+screen_x\s*,\s*screen_y\s*;", core), \
        "Native stock screen coordinates must retain their byte width"
    helper = (ENGINE / "src/td_actor_render.c").read_text()
    declaration = (ENGINE / "include/td_actor_render.h").read_text()
    assert helper.startswith("#pragma bank 255\n"), "Actor helper lost native autobanking"
    assert re.search(r"\bextern\s+UBYTE\s+screen_x\s*,\s*screen_y\s*;", helper), \
        "Banked traversal must use the stock byte-width screen coordinate ABI"
    signature = "const metasprite_t *td_actor_render_pose(const void *descriptor,UBYTE bank,UBYTE frame) NONBANKED"
    assert helper.count(signature + " {") == 1 and declaration.count(signature + ";") == 1, \
        "The compact metadata selector must remain in fixed HOME with its matching ABI"
    for name,args in (("before","void"), ("prepare","void"), ("ground","UBYTE window_hide_actors"), ("after","void")):
        assert helper.count(f"void td_actor_render_{name}({args}) BANKED {{") == 1, \
            "Ground dispatch must retain its banked native ABI"
        assert declaration.count(f"void td_actor_render_{name}({args}) BANKED;") == 1, \
            "Fixed-bank callers need the matching banked declaration"
    compiler = shutil.which(os.environ.get("CC", "cc"))
    assert compiler, "Host C compiler unavailable"
    with tempfile.TemporaryDirectory(prefix="toronto-actor-scene-") as directory:
        work = Path(directory)
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text("""#ifndef HOST_SCENE_PLATFORM_H
#define HOST_SCENE_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
typedef uint8_t UBYTE;typedef int8_t BYTE;typedef uint16_t UWORD;typedef int16_t WORD;
#define BANKED
#define NONBANKED
#define LCDCF_OBJ16 4
#define CGB 1
#define S_PRIORITY 128
#endif
""")
        (work / "gbdk/metasprites.h").write_text("""#ifndef HOST_SCENE_META_H
#define HOST_SCENE_META_H
#include <gbdk/platform.h>
typedef struct {BYTE dy,dx;UBYTE dtile,props;} metasprite_t;
#define metasprite_end (-128)
#endif
""")
        (work / "host_actor_scene.h").write_text("""#ifndef HOST_ACTOR_SCENE_H
#define HOST_ACTOR_SCENE_H
#include <gbdk/platform.h>
#include <gbdk/metasprites.h>
typedef struct {UBYTE y,x,tile,prop;} OAM_item_t;
typedef struct {UBYTE bank;void *ptr;} far_ptr_t;
typedef struct {UBYTE n_metasprites;const metasprite_t *const *metasprites;struct{BYTE x,y;} emote_origin;} spritesheet_t;
typedef struct {struct {UWORD x,y;} pos;UBYTE flags,frame,base_tile;far_ptr_t sprite;} actor_t;
#define MAX_ACTORS 22
#define PLAYER actors[0]
#define ACTOR_FLAG_HIDDEN 2
#define ACTOR_FLAG_ACTIVE 32
#define ACTOR_FLAG_DISABLED 64
#define ACTOR_FLAG_PINNED 128
#define CHK_FLAG(value,mask) ((value)&(mask))
#define SUBPX_TO_PX(value) ((value)>>5)
#define DEVICE_WINDOW_PX_OFFSET_X 7
#define EMOTE_BOUNCE_FRAMES 1
#define BANK(value) 7
#define SWITCH_ROM(value) (CURRENT_BANK=(value))
extern actor_t actors[22],*emote_actor;
extern UBYTE CURRENT_BANK,actors_len,emote_timer,LCDC_REG,WX_REG,WY_REG,show_actors_on_overlay;
extern UBYTE _is_CGB,overlay_priority;
extern UBYTE allocated_sprite_tiles,allocated_hardware_sprites,__render_shadow_OAM;
extern UBYTE screen_x,screen_y;
extern WORD draw_scroll_x,draw_scroll_y,scroll_x,scroll_y;
extern const BYTE emote_offsets[1];
extern const metasprite_t emote_metasprite_8_16[],emote_metasprite_8_8[];
extern volatile OAM_item_t shadow_OAM[40],shadow_OAM2[40];
void MemcpyBanked(void *,const void *,size_t,UBYTE);
UBYTE ReadBankedUBYTE(const UBYTE *,UBYTE);
UBYTE move_metasprite(const metasprite_t *,UBYTE,UBYTE,WORD,WORD);
void td_scenery_restore(void);void td_scenery_render(void);
void td_aircraft_render_restore(void);void td_aircraft_render(void);
void td_traffic_lights_render(void);void td_sandbox_render(void);void td_boats_render(void);
UBYTE td_boats_controlled(void);
void host_actor_pose_probe(const spritesheet_t *);
#endif
""")
        for name in ("actor", "data_manager", "shadow", "scroll", "math", "macro", "bankdata"):
            (work / (name + ".h")).write_text('#include "host_actor_scene.h"\n')
        (work / "core_render_under_test.c").write_text(
            '#include "host_actor_scene.h"\n#include "td_actor_render.h"\n' + core_render() + "\n")
        # Observe the exact selected resource after its real bank selection.
        # This one host-only probe models mapped memory; production has no
        # counter, state, altered pointer lookup, traversal or OAM admission.
        selection = "    SWITCH_ROM(bank);\n"
        assert helper.count(selection) == 1, "Unexpected metadata selector shape"
        (work / "actor_render_under_test.c").write_text(
            helper.replace(selection, selection + "    host_actor_pose_probe(sprite);\n"))
        binary = work / "actor-scene-regressions"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-Wno-pointer-to-int-cast", "-DACTOR_H",
                        "-include", str(work / "host_actor_scene.h"), "-fsanitize=address,undefined",
                        "-I", str(work), "-I", str(ENGINE / "include"),
                        str(ROOT / "tests/engine/actor_scene_render_harness.c"),
                        str(work / "core_render_under_test.c"), str(work / "actor_render_under_test.c"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
