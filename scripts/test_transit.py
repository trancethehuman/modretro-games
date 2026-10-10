"""Exercise the unchanged banked transit module with an independent route oracle.

Only GBDK types and bank annotations are adapted. This checks native C logic,
not ROM bank placement, scene transitions, physical boarding or cartridge play.
"""
from pathlib import Path
import os
import host_cflags
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'project/plugins/toronto-driving/engine'


def main():
    compiler = shutil.which(os.environ.get('CC', 'cc'))
    if not compiler:
        raise SystemExit('Host C compiler unavailable; transit regressions did not run.')
    with tempfile.TemporaryDirectory(prefix='toronto-transit-tests-') as directory:
        work = Path(directory)
        shutil.copyfile(ENGINE / 'src/td_transit.c', work / 'transit_under_test.c')
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
        binary = work / 'transit-regressions'
        subprocess.run([compiler, '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', *host_cflags.extra_flags(compiler),
                        '-Wno-unknown-pragmas', '-fsanitize=address,undefined',
                        '-I', str(work), '-I', str(ENGINE / 'include'),
                        str(ROOT / 'tests/engine/transit_harness.c'), '-o', str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == '__main__':
    main()
