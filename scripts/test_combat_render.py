"""Sanitize the actual one-tile muzzle compositor and conditional restoration.

Independent pixel/VRAM adapters check every CGB attribute, palette and flip,
map-page changes, roof/HUD admission and stale-owner protection. This is host
logic evidence, not native VRAM timing or physical visual acceptance.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'games/toronto-dispatch/project/plugins/toronto-driving/engine'
compiler = shutil.which(os.environ.get('CC', 'cc'))
assert compiler, 'Host C compiler unavailable'
with tempfile.TemporaryDirectory(prefix='toronto-combat-render-') as folder:
    tmp = Path(folder)
    shutil.copyfile(ROOT / 'tests/engine/gbvm_stubs.h', tmp / 'gbvm_stubs.h')
    shutil.copyfile(ENGINE / 'src/td_combat_render.c', tmp / 'renderer_under_test.c')
    (tmp / 'gbdk').mkdir()
    (tmp / 'gbdk/platform.h').write_text('#include "gbvm_stubs.h"\n#define CGB 1\n')
    (tmp / 'bankdata.h').write_text('#include "gbvm_stubs.h"\n')
    for name in ('scroll', 'ui', 'compat'):
        (tmp / f'{name}.h').write_text('#include "host_combat_render.h"\n')
    (tmp / 'host_combat_render.h').write_text('''#ifndef HOST_COMBAT_RENDER_H
#define HOST_COMBAT_RENDER_H
#include "gbvm_stubs.h"
extern UBYTE VBK_REG,WX_REG,WY_REG,win_pos_x,win_pos_y,win_dest_pos_x,win_dest_pos_y;
UBYTE *GetBkgAddr(void);
UBYTE get_vram_byte(UBYTE *address);
void set_vram_byte(UBYTE *address,UBYTE value);
void get_bkg_data(UBYTE first,UBYTE count,UBYTE *data);
void set_bkg_data(UBYTE first,UBYTE count,const UBYTE *data);
#endif
''')
    subprocess.run([compiler, '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unknown-pragmas', '-fsanitize=address,undefined', '-DACTOR_H',
                    '-I' + str(tmp), '-I' + str(ENGINE / 'include'),
                    str(ROOT / 'tests/engine/combat_render_harness.c'), '-o', str(tmp / 'check')], check=True)
    subprocess.run([str(tmp / 'check')], check=True)
