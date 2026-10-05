"""Check registered shop resources and sanitize the actual native room module.

Host execution verifies logic against authored collision data. ROM linkage,
native rendering, allocation and cartridge play remain separate checks.
"""
from pathlib import Path
import json
import os
import shutil
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
GAME=ROOT/'games/toronto-dispatch'
ENGINE=GAME/'project/plugins/toronto-driving/engine'
sys.path.insert(0,str(GAME/'scripts'))
from create_atlas import decode_grid
from check_district_world import decode


def resources():
    manifest=json.loads((GAME/'content/shop_interiors.json').read_text())
    engine=json.loads((ENGINE/'engine.json').read_text())
    types=[x for x in engine['sceneTypes'] if x['key']=='TORONTO_SHOP']
    assert len(types)==1 and 'src/td_shops.c' in types[0]['files']
    grids=[]
    for room in manifest['rooms']:
        key=room['key'];name='shop_'+key
        scene=json.loads((GAME/f'project/project/scenes/toronto_{name}/scene.gbsres').read_text())
        asset=GAME/f'project/assets/backgrounds/{name}.png'
        background=json.loads(asset.with_suffix('.png.gbsres').read_text())
        assert scene['type']=='TORONTO_SHOP' and (scene['width'],scene['height'])==(20,18)
        assert scene['symbol']=='scene_toronto_'+name and scene['backgroundId']==background['id']
        assert asset.read_bytes()==(GAME/f'project/original-art/{name}.png').read_bytes()
        assert decode(background['tileColors'])==room['attributes']
        assert decode_grid(scene['collisions'],360)==room['collisions']
        assert scene['playerSpriteSheetId']=='95080aab-0201-545e-be5a-f5e79b9a693e'
        keepers=list((GAME/f'project/project/scenes/toronto_{name}/actors').glob('*.gbsres'))
        assert len(keepers)==1
        keeper=json.loads(keepers[0].read_text())
        assert keeper['_index']==0 and keeper['spriteSheetId']=='e89b6d16-588b-5bfb-ae62-3fe34d537c86'
        assert not keeper['animate'] and not keeper['persistent'] and keeper['moveSpeed']==0
        assert all(not keeper[s] for s in ('script','startScript','updateScript','hit1Script','hit2Script','hit3Script'))
        grids.append('static const UBYTE room_grid_'+key+'[360]={'+','.join(map(str,room['collisions']))+'};')
    return '\n'.join(grids)


def main():
    subprocess.run([sys.executable,str(GAME/'scripts/create_shop_art.py'),'--check'],check=True)
    fixture=resources()
    compiler=shutil.which(os.environ.get('CC','cc'));assert compiler,'Host C compiler unavailable'
    with tempfile.TemporaryDirectory(prefix='toronto-shops-') as directory:
        work=Path(directory);(work/'gbdk').mkdir();(work/'data').mkdir()
        shutil.copyfile(ENGINE/'src/td_shops.c',work/'shop_under_test.c')
        (work/'gbdk/platform.h').write_text('''
#ifndef HOST_SHOP_PLATFORM_H
#define HOST_SHOP_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
typedef uint8_t UBYTE;typedef int8_t BYTE;typedef uint16_t UWORD;typedef int16_t WORD;
#define BANKED
#define TRUE 1
#define FALSE 0
#define J_RIGHT 1
#define J_LEFT 2
#define J_UP 4
#define J_DOWN 8
#define J_A 16
#define J_B 32
#define J_SELECT 64
#define J_START 128
#endif
''')
        (work/'host_shops.h').write_text('''
#ifndef HOST_SHOPS_H
#define HOST_SHOPS_H
#include <gbdk/platform.h>
typedef struct {UBYTE bank;const void *ptr;} far_ptr_t;
#define TO_FAR_PTR_T(name) {42,&name}
typedef struct {struct {UWORD x,y;} pos;UBYTE anim_tick,frame_start,frame_end;} actor_t;
extern actor_t actors[22];
#define PLAYER actors[0]
extern UBYTE actors_len,VBK_REG,joy,joy_pressed,camera_settings;
extern WORD camera_x,camera_y,camera_offset_x,camera_offset_y,camera_deadzone_x,camera_deadzone_y;
extern UWORD sys_time;
extern far_ptr_t current_scene;
#define COLLISION_ALL 15
#define EXCEPTION_CHANGE_SCENE 1
UBYTE tile_at(UBYTE x,UBYTE y);
void actor_set_frames(actor_t *actor,UBYTE start,UBYTE end);
void set_win_tiles(UBYTE x,UBYTE y,UBYTE width,UBYTE height,const UBYTE *tiles);
void ui_set_pos(UBYTE x,UBYTE y);
void *script_execute(UBYTE bank,const UBYTE *script,void *context,UBYTE count);
#endif
''')
        for name in ('bankdata','actor','camera','collision','data_manager','input','scroll','system','ui','vm','vm_exceptions'):
            (work/f'{name}.h').write_text('#include "host_shops.h"\n')
        for key in ('grocery','corner_store','repair_shop'):
            (work/f'data/scene_toronto_shop_{key}.h').write_text(f'#include "host_shops.h"\nextern const UBYTE scene_toronto_shop_{key};\n')
        (work/'shop_grid_oracle.h').write_text(fixture)
        binary=work/'shop-regressions'
        subprocess.run([compiler,'-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
                        '-Wno-unknown-pragmas','-DACTOR_H','-fsanitize=address,undefined',
                        '-I',str(work),'-I',str(ENGINE/'include'),str(ROOT/'tests/engine/shops_harness.c'),
                        '-o',str(binary)],check=True)
        raise SystemExit(subprocess.run([str(binary)],check=False).returncode)


if __name__=='__main__':main()
