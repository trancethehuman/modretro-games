"""Host OAM/scanline oracles for actual signed ground rendering.

These checks establish bounds and stable admission logic, not cartridge timing
or the user's intermittent physical flicker diagnosis.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
ENGINE=ROOT/'games/toronto-dispatch/project/plugins/toronto-driving/engine'

def main():
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
#define SUBPX_TO_PX(v) ((v)>>5)
extern WORD draw_scroll_x,draw_scroll_y;
extern volatile OAM_item_t shadow_OAM[40],shadow_OAM2[40];
extern UBYTE allocated_hardware_sprites,__render_shadow_OAM;
void MemcpyBanked(void *dest,const void *source,size_t size,UBYTE bank);
UBYTE ReadBankedUBYTE(const UBYTE *source,UBYTE bank);
#endif
''')
        for name in ('actor','shadow','data_manager','scroll','math'):(work/(name+'.h')).write_text('#include "host_actor_render.h"\n')
        binary=work/'actor-render-regressions'
        subprocess.run([compiler,'-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unknown-pragmas',
            '-Wno-pointer-to-int-cast','-fsanitize=address,undefined','-I',str(work),'-I',str(ENGINE/'include'),
            str(ROOT/'tests/engine/actor_render_harness.c'),str(ENGINE/'src/td_actor_render.c'),'-o',str(binary)],check=True)
        raise SystemExit(subprocess.run([str(binary)],check=False).returncode)

if __name__=='__main__':main()
