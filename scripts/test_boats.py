"""Sanitize the actual controllable-boat C against PNG, water and OAM oracles.

Host fixtures use the committed collision grids and production module. They
establish logic, not native ABI/allocation/CPU timing or physical cartridge play.
"""
from pathlib import Path
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from collections import deque
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
GAME=ROOT/"games/toronto-dispatch"
ENGINE=GAME/"project/plugins/toronto-driving/engine"
sys.path.insert(0,str(GAME/"scripts"))
from create_atlas import decode_grid

def body(source,name):
    source=re.sub(r"/\*.*?\*/|//[^\n]*","",source,flags=re.S)
    match=re.search(r"\bvoid\s+"+re.escape(name)+r"\s*\([^)]*\)[^{]*\{",source)
    assert match,f"Missing native integration {name}"
    start,depth=match.end(),1
    for i in range(start,len(source)):
        depth+=(source[i]=="{")-(source[i]=="}")
        if not depth:return source[start:i]
    raise AssertionError("Unclosed production function")

def grids():
    result={}
    for district,name in ((0,"toronto_city"),(4,"toronto_port_lands")):
        scene=json.loads((GAME/f"project/project/scenes/{name}/scene.gbsres").read_text())
        result[district]=decode_grid(scene["collisions"],128*122)
    return result

def check_render_integration(core,helpers):
    render=body(core,"actors_render")
    before=body(helpers,"td_actor_render_prepare_local")
    ground_helper=body(helpers,"td_actor_render_ground")
    after=body(helpers,"td_actor_render_after")
    compact=lambda source:re.sub(r"\s+","",source)
    controlled="if(td_boats_controlled())td_boats_render();"
    ambient="if(!td_boats_controlled())td_boats_render();"
    combined=compact(render+before+ground_helper+after)
    assert combined.count("td_boats_render();")==2 and \
        combined.count(controlled)==combined.count(ambient)==1,"Keep one controlled and one ambient boat path"
    assert compact(body(helpers,"td_actor_render_before"))=="td_actor_render_prepare_local(1);"
    assert compact(body(helpers,"td_actor_render_prepare"))=="td_actor_render_prepare_local(0);"
    assert compact(before)=="if(td_actor_render_restored){if(force)td_guidance_road_restore();return;}td_actor_render_restored=1;"+ \
        "td_aircraft_render_restore();td_combat_render_restore();"+ \
        "if(force)td_guidance_road_restore();elsetd_guidance_road_prepare();td_scenery_restore();", \
        "Normal preparation retains an unchanged arrow; forced reuse releases it in reverse overlay ownership order"
    assert compact(after)=="td_traffic_lights_render();td_scenery_render();td_hospital_render();"+ambient+ \
        "td_sandbox_render();td_guidance_road_render();td_combat_render();"+ \
        "td_aircraft_render();td_actor_render_restored=0;","Banked post-ground overlay contents and order stay exact"
    assert render.count("td_actor_render_prepare();")==render.count("td_actor_render_after();")==1
    assert compact(render).count(controlled)==1 and not compact(render).count(ambient)
    assert render.count("SWITCH_ROM(_save);")==1
    assert render.count("td_actor_render_ground(window_hide_actors);")==1
    assert "SWITCH_ROM" not in ground_helper,"Banked ground code must retain its executing bank"
    assert "for (ground_index" in ground_helper and "actors_len<MAX_ACTORS?actors_len:MAX_ACTORS" in ground_helper
    assert ground_helper.count("td_actor_render_actor_local(actor);")==1 and \
        "td_actor_render_actor(actor);" not in ground_helper, "Same-bank ground dispatch uses its unchanged private near-call actor body"
    ground=render.index("td_actor_render_ground(window_hide_actors);")
    assert render.index("td_actor_render_prepare();")<render.index("td_actor_render_actor(&PLAYER);")
    # Use the uncompressed exact native conditional for actual function order;
    # its position must stay before the ground loop and both bank restorations.
    control=re.search(r"if\s*\(\s*td_boats_controlled\(\)\s*\)\s*td_boats_render\(\);",render)
    assert control and render.index("td_actor_render_actor(&PLAYER);")<control.start()<ground< \
        render.index("SWITCH_ROM(_save);")<render.index("td_actor_render_after();"), \
        "Controlled hull precedes NPCs; ambient helper follows ground and saved-bank restoration"

def check_integration():
    init=body((ENGINE/"src/states/TORONTO.c").read_text(),"toronto_init")
    assert init.count("td_boats_bind();")==1
    clone=re.search(r"actors\s*\[\s*i\s*\]\s*=\s*PLAYER",init)
    assert clone and init.index("td_boats_bind();")<clone.start(),"Bind boat loader before clones"
    core=(ENGINE/"src/core/actor.c").read_text()
    helpers=(ENGINE/"src/td_actor_render.c").read_text()
    check_render_integration(core,helpers)
    # Distinct source mutations prevent a bank-compaction exemption from
    # accepting missing gates, duplicated hulls or changed layer/bank order.
    for changed_core,changed_helpers in (
        (core.replace("if(td_boats_controlled())td_boats_render();","td_boats_render();"),helpers),
        (core,helpers.replace("if(!td_boats_controlled())td_boats_render();","if(td_boats_controlled())td_boats_render();")),
        (core,helpers.replace("    td_sandbox_render();td_guidance_road_render();td_combat_render();\n    td_aircraft_render();","    td_aircraft_render();\n    td_sandbox_render();td_guidance_road_render();td_combat_render();")),
        (core.replace("    SWITCH_ROM(_save);\n    td_actor_render_after();","    td_actor_render_after();\n    SWITCH_ROM(_save);"),helpers),
        (core,helpers.replace("    td_scenery_restore();\n","")),
        (core,helpers.replace("td_aircraft_render_restore();td_combat_render_restore();","td_combat_render_restore();td_aircraft_render_restore();")),
        (core,helpers.replace("td_guidance_road_render();td_combat_render();","td_combat_render();td_guidance_road_render();")),
    ):
        try:check_render_integration(changed_core,changed_helpers)
        except AssertionError:pass
        else:raise AssertionError("Boat native renderer integration mutation accepted")
    names=("toronto_city","toronto_west","toronto_high_park","toronto_east","toronto_port_lands","toronto_islands","toronto_north")
    boat_id="22789632-122e-5a80-a195-5c8c6e7caeba"
    for district,name in enumerate(names):
        paths=(GAME/f"project/project/scenes/{name}/actors").glob("*.gbsres")
        loaders=[json.loads(p.read_text()) for p in paths]
        boats=[a for a in loaders if a.get("spriteSheetId")==boat_id]
        assert len(boats)==(0 if district==6 else 1)
        if not boats:continue
        a=boats[0];assert a["_index"]==(4 if district in (0,1,3) else 3)
        assert a["frame"]==3 and not a["animate"] and not a["persistent"]
        assert all(not a[k] for k in ("script","startScript","updateScript","hit1Script","hit2Script","hit3Script"))
    art=json.loads((GAME/"project/original-art/ambient_boat_art.json").read_text())
    assert art["player_control"] and not art["save_fields"] and art["hull_size_pixels"]==[16,24]
    assert art["source_unique_8x16_pairs_with_flips"]<=8 and art["scratch_tile_pairs"]==4
    collision=grids()
    # All authored landings belong to the reachable mainland pedestrian graph.
    for district,start in ((0,(560//8,720//8)),(4,(192//8,128//8))):
        grid=collision[district];visited={start};queue=deque([start])
        while queue:
            x,y=queue.popleft()
            for p in ((x-1,y),(x+1,y),(x,y-1),(x,y+1)):
                if 0<=p[0]<128 and 0<=p[1]<122 and p not in visited and not grid[p[1]*128+p[0]]&15:
                    visited.add(p);queue.append(p)
        for area,u,v in art["dock_foot_points"]:
            if area==district:assert (u//8,v//8) in visited,f"Boat landing{area,u,v} strands the courier"

def art_fixture():
    image=Image.open(GAME/"project/original-art/ambient_boat.png").convert("RGB")
    colours=((101,255,0),(224,248,207),(134,192,108),(7,24,33))
    out=[]
    for name,x,width,height in (("original_north",0,16,32),("original_east",16,24,16)):
        out.append(f"static const UBYTE {name}[{height}][{width}]={{")
        out.extend("{"+",".join(str(colours.index(image.getpixel((x+c,r)))) for c in range(width))+"}," for r in range(height))
        out.append("};")
    for district,grid in grids().items():
        out.append(f"static const UBYTE original_grid_{district}[15616]={{")
        out.extend(",".join(map(str,grid[i:i+128]))+"," for i in range(0,len(grid),128));out.append("};")
    return "\n".join(out)

def main():
    subprocess.run([sys.executable,str(GAME/"scripts/create_boat_sprite.py"),"--check"],check=True)
    check_integration()
    compiler=shutil.which(os.environ.get("CC","cc"))
    assert compiler,"Host C compiler unavailable"
    with tempfile.TemporaryDirectory(prefix="toronto-boats-") as directory:
        work=Path(directory);(work/"gbdk").mkdir()
        shutil.copyfile(ENGINE/"src/td_boats.c",work/"boats_under_test.c")
        (work/"gbdk/platform.h").write_text("""
#ifndef HOST_BOAT_PLATFORM_H
#define HOST_BOAT_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
typedef uint8_t UBYTE;typedef int8_t BYTE;typedef uint16_t UWORD;typedef int16_t WORD;
#define BANKED
#define TRUE 1
#define FALSE 0
#define CGB 1
#define J_RIGHT 1
#define J_LEFT 2
#define J_UP 4
#define J_DOWN 8
#define J_A 16
#define J_B 32
#define J_SELECT 64
#define J_START 128
#endif
""")
        for name in ("bankdata","actor","data_manager","gbs_types","scroll","shadow","ui","compat","collision"):
            (work/f"{name}.h").write_text('#include "host_boats.h"\n')
        (work/"td_district.h").write_text('#include "host_boats.h"\n#define TD_DISTRICT_COUNT 7\n#define TD_DISTRICT_NORTH 6\nUBYTE td_district_current(void);\n')
        (work/"host_boats.h").write_text("""
#ifndef HOST_BOATS_H
#define HOST_BOATS_H
#include <gbdk/platform.h>
typedef struct {UBYTE bank;const void *ptr;} far_ptr_t;
typedef struct {BYTE dy,dx;UBYTE dtile,props;} metasprite_t;
#define metasprite_end (-128)
typedef struct {UBYTE y,x,tile,prop;} OAM_item_t;
typedef struct {UWORD n_tiles;UBYTE tiles[2048];} tileset_t;
typedef struct {UBYTE n_metasprites;const metasprite_t *const *metasprites;far_ptr_t tileset,cgb_tileset;} spritesheet_t;
typedef struct actor actor_t;
struct actor {actor_t *prev,*next;UBYTE flags,base_tile;far_ptr_t sprite;};
#define ACTOR_FLAG_ACTIVE 32
#define ACTOR_FLAG_HIDDEN 2
extern actor_t actors[22],*actors_inactive_head;
extern UBYTE actors_len,allocated_hardware_sprites,VBK_REG,__render_shadow_OAM;
extern UBYTE win_pos_x,win_pos_y,win_dest_pos_x,win_dest_pos_y,WX_REG,WY_REG;
extern volatile OAM_item_t shadow_OAM[40],shadow_OAM2[40];
extern WORD draw_scroll_x,draw_scroll_y;extern far_ptr_t current_scene;
void deactivate_actor(actor_t *actor);
void MemcpyBanked(void *dest,const void *src,size_t length,UBYTE bank);
UBYTE ReadBankedUBYTE(const UBYTE *src,UBYTE bank);
void set_sprite_data(UBYTE first,UBYTE count,const UBYTE *pixels);
UBYTE tile_at(UBYTE x,UBYTE y);
#endif
""")
        (work/"boat_art_oracle.h").write_text(art_fixture())
        binary=work/"boat-regressions"
        subprocess.run([compiler,"-std=c11","-O1","-g","-Wall","-Wextra","-Werror",
                        "-Wno-unknown-pragmas", "-DACTOR_H","-Wno-pointer-to-int-cast","-fsanitize=address,undefined",
                        "-I",str(work),"-I",str(ENGINE/"include"),str(ROOT/"tests/engine/boats_harness.c"),
                        "-o",str(binary)],check=True)
        raise SystemExit(subprocess.run([str(binary)],check=False).returncode)
if __name__=="__main__":main()
