"""Host OAM/scanline oracles for actual signed ground rendering.

These checks establish bounds and stable admission logic, not cartridge timing
or the user's intermittent physical flicker diagnosis.
"""
from pathlib import Path
import hashlib
import os
import re
import shutil
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
ENGINE=ROOT/'games/toronto-dispatch/project/plugins/toronto-driving/engine'

def function(source, signature):
    match=re.search(re.escape(signature)+r'\s*\{',source)
    assert match, 'Missing exact renderer function ABI'
    end,depth=match.end(),1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[match.start():end]

def check_near_bodies():
    source=(ENGINE/'src/td_actor_render.c').read_text()
    for local,original,changes,pin in (
        ('static void td_actor_render_local(const metasprite_t *pose,UBYTE bank,UBYTE base,WORD x,WORD y)',
         'void td_actor_render(const metasprite_t *pose,UBYTE bank,UBYTE base,WORD x,WORD y) BANKED',(),
         '41202fdea740bdc490b4774df6d85dce73090af35c49f750b1c707cd2c98a78e'),
        ('static void td_actor_render_actor_local(actor_t *actor)',
         'void td_actor_render_actor(actor_t *actor) BANKED',
         (('td_actor_render_local(', 'td_actor_render('),),
         'cb09109efbcfe29980969b7e28447e2617c4ee52c2b64e318c598e8c1dd81cb9'),
    ):
        body=function(source,local).replace(local,original,1)
        for before,after in changes:
            assert body.count(before)==1,'Unexpected internal renderer call count'
            body=body.replace(before,after)
        assert hashlib.sha256(body.encode()).hexdigest()==pin, \
            'Private near-call body changed the frozen cull/metadata/clip/scanline/OAM logic'
    assert function(source,'void td_actor_render(const metasprite_t *pose,UBYTE bank,UBYTE base,WORD x,WORD y) BANKED')== \
        'void td_actor_render(const metasprite_t *pose,UBYTE bank,UBYTE base,WORD x,WORD y) BANKED {\n    td_actor_render_local(pose,bank,base,x,y);\n}', \
        'Public BANKED admission entry must be only a direct private call'
    assert function(source,'void td_actor_render_actor(actor_t *actor) BANKED')== \
        'void td_actor_render_actor(actor_t *actor) BANKED {\n    td_actor_render_actor_local(actor);\n}', \
        'Public BANKED actor entry must be only a direct private call'

def main():
    check_near_bodies()
    compiler=shutil.which(os.environ.get('CC','cc'))
    if not compiler:raise SystemExit('Host C compiler unavailable')
    with tempfile.TemporaryDirectory(prefix='toronto-actor-render-') as directory:
        work=Path(directory);(work/'gbdk').mkdir()
        (work/'gbdk/platform.h').write_text('''#ifndef HOST_PLATFORM_H
#define HOST_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
typedef uint8_t UBYTE; typedef int8_t BYTE;
typedef uint16_t UWORD; typedef int16_t WORD;
#define BANKED
#define NONBANKED
#define LCDCF_OBJ16 4
extern UBYTE LCDC_REG;
#endif
''')
        (work/'gbdk/metasprites.h').write_text('''#ifndef HOST_META_H
#define HOST_META_H
#include <gbdk/platform.h>
typedef struct { BYTE dy,dx; UBYTE dtile,props; } metasprite_t;
#define metasprite_end (-128)
#endif
''')
        (work/'host_actor_render.h').write_text('''#ifndef HOST_RENDER_H
#define HOST_RENDER_H
#include <gbdk/platform.h>
#include <gbdk/metasprites.h>
typedef struct { UBYTE y,x,tile,prop; } OAM_item_t;
typedef struct { UBYTE bank; const void *ptr; } far_ptr_t;
typedef struct { UBYTE n_metasprites; const metasprite_t *const *metasprites; } spritesheet_t;
typedef struct { struct { UWORD x,y; } pos; UBYTE flags,frame,base_tile; far_ptr_t sprite; } actor_t;
#define ACTOR_FLAG_PINNED 128
#define ACTOR_FLAG_ACTIVE 32
#define ACTOR_FLAG_HIDDEN 2
#define ACTOR_FLAG_DISABLED 64
#define CHK_FLAG(value,mask) ((value)&(mask))
#define MAX_ACTORS 22
#define SWITCH_ROM(value) host_actor_switch(value)
#define SUBPX_TO_PX(v) ((v)>>5)
extern actor_t actors[22];
extern UBYTE actors_len,screen_x,screen_y,WX_REG,WY_REG;
extern WORD draw_scroll_x,draw_scroll_y;
extern volatile OAM_item_t shadow_OAM[40],shadow_OAM2[40];
extern UBYTE allocated_hardware_sprites,__render_shadow_OAM;
extern UBYTE CURRENT_BANK;
void host_actor_switch(UBYTE bank);
void MemcpyBanked(void *dest,const void *source,size_t size,UBYTE bank);
UBYTE ReadBankedUBYTE(const UBYTE *source,UBYTE bank);
#endif
''')
        for name in ('actor','shadow','data_manager','scroll','math','macro','bankdata'):(work/(name+'.h')).write_text('#include "host_actor_render.h"\n')
        binary=work/'actor-render-regressions'
        subprocess.run([compiler,'-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unknown-pragmas','-DACTOR_H',
            '-Wno-pointer-to-int-cast','-include',str(work/'host_actor_render.h'),'-fsanitize=address,undefined','-I',str(work),'-I',str(ENGINE/'include'),
            str(ROOT/'tests/engine/actor_render_harness.c'),str(ENGINE/'src/td_actor_render.c'),'-o',str(binary)],check=True)
        raise SystemExit(subprocess.run([str(binary)],check=False).returncode)

if __name__=='__main__':main()
