"""Check held navigation timing and action-button isolation in actual menu C."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'games/toronto-dispatch/project/plugins/toronto-driving/engine'
with tempfile.TemporaryDirectory(prefix='toronto-menu-repeat-') as folder:
    tmp = Path(folder)
    (tmp / 'gb').mkdir()
    (tmp / 'gbdk').mkdir()
    (tmp / 'gbdk/platform.h').write_text('#include <stdint.h>\ntypedef uint8_t UBYTE; typedef uint16_t UWORD;\n#define BANKED\n')
    (tmp / 'gb/gb.h').write_text('#define J_RIGHT 1\n#define J_LEFT 2\n#define J_UP 4\n#define J_DOWN 8\n#define J_A 16\n#define J_B 32\n#define J_SELECT 64\n#define J_START 128\n')
    (tmp / 'check.c').write_text(r'''
#include <assert.h>
#include <gb/gb.h>
#include "td_menu.h"
int main(void) {
    td_menu_reset();
    assert(td_menu_repeat(J_DOWN, 60)==0); /* Existing edge selects once. */
    assert(td_menu_repeat(J_DOWN, 10)==0);
    assert(td_menu_repeat(J_DOWN, 13)==0);
    assert(td_menu_repeat(J_DOWN, 1)==J_DOWN);
    assert(td_menu_repeat(J_DOWN, 7)==0);
    assert(td_menu_repeat(J_DOWN, 1)==J_DOWN);
    assert(td_menu_repeat(0, 1)==0);
    assert(td_menu_repeat(J_DOWN, 1)==0); /* Release restores initial delay. */
    assert(td_menu_repeat(J_RIGHT, 1)==0); /* Direction change is fresh. */
    assert(td_menu_repeat(J_RIGHT, 24)==J_RIGHT);
    assert(td_menu_repeat(J_RIGHT|J_A|J_B|J_START|J_SELECT, 8)==0);
    assert(td_menu_repeat(J_RIGHT, 8)==0); /* Chord release starts the delay again. */
    assert(td_menu_repeat(J_RIGHT, 24)==J_RIGHT);
    assert(td_menu_repeat(J_A|J_B|J_START|J_SELECT, 100)==0);
    assert(td_menu_repeat(J_UP|J_DOWN, 100)==0);
    assert(td_menu_repeat(J_LEFT|J_RIGHT, 100)==0);
    assert(td_menu_repeat(J_LEFT, 1)==0);
    assert(td_menu_repeat(J_LEFT, 65535)==J_LEFT); /* No catch-up cursor jump. */
    assert(td_menu_repeat(J_LEFT, 0)==0);
    td_menu_reset();
    assert(td_menu_repeat(J_LEFT, 8)==0); /* A different menu starts fresh. */
    return 0;
}
''')
    subprocess.run(['clang', '-std=c99', '-Wno-unknown-pragmas', '-I'+str(tmp), '-I'+str(ENGINE/'include'), str(ENGINE/'src/td_menu.c'), str(tmp/'check.c'), '-o', str(tmp/'check')], check=True)
    subprocess.run([str(tmp/'check')], check=True)
print('Menu repeat: timing, release/direction reset, opposing keys and action-button isolation pass.')
