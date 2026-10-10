"""Exercise unchanged banked world source with real data and synthetic graphs.

Only GBDK types/bank annotations are stubbed. This does not build a ROM, test
bank-switch timing or establish expanded geography/performance/hardware.
"""
from pathlib import Path
import os
import host_cflags
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'games/toronto-dispatch/project/plugins/toronto-driving/engine'


def main():
    compiler = shutil.which(os.environ.get('CC', 'cc'))
    if not compiler:
        raise SystemExit('Host C compiler unavailable; world regressions did not run.')
    with tempfile.TemporaryDirectory(prefix='toronto-world-tests-') as directory:
        work = Path(directory)
        shutil.copyfile(ENGINE / 'src/td_world.c', work / 'world_under_test.c')
        (work / 'gbdk').mkdir()
        (work / 'gbdk/platform.h').write_text('''#ifndef HOST_PLATFORM_H
#define HOST_PLATFORM_H
#include <stdint.h>
typedef uint8_t UBYTE;
typedef int8_t BYTE;
typedef uint16_t UWORD;
typedef int16_t WORD;
#define BANKED
#define TRUE 1
#define FALSE 0
#endif
''')
        (work / 'bankdata.h').write_text('''#ifndef HOST_BANKDATA_H
#define HOST_BANKDATA_H
#include <gbdk/platform.h>
typedef struct { UBYTE bank; const void *ptr; } far_ptr_t;
#endif
''')
        binary = work / 'world-navigation-regressions'
        subprocess.run([compiler, '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', *host_cflags.extra_flags(compiler),
                        '-Wno-unknown-pragmas', '-fsanitize=address,undefined',
                        '-I', str(work), '-I', str(ENGINE / 'include'),
                        str(ROOT / 'tests/engine/world_navigation_harness.c'), '-o', str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == '__main__':
    main()
