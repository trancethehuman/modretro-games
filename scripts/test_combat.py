"""Check the actual native weapon/recovery unit with bounded host adapters.

These tests establish combat state, ray ordering, solid-grid line of sight,
health/cargo separation and freeze/recovery rules. Native rendering, CPU, ABI,
physical persistence and player judgement remain separate checks.
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
with tempfile.TemporaryDirectory(prefix='toronto-combat-') as folder:
    tmp = Path(folder)
    shutil.copyfile(ROOT / 'tests/engine/gbvm_stubs.h', tmp / 'gbvm_stubs.h')
    for name in ('actor', 'bankdata', 'input'):
        (tmp / f'{name}.h').write_text('#include "gbvm_stubs.h"\n')
    (tmp / 'gbdk').mkdir()
    (tmp / 'gbdk/platform.h').write_text('#include "gbvm_stubs.h"\n')
    subprocess.run([compiler, '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unknown-pragmas', '-fsanitize=address,undefined', '-DACTOR_H',
                    '-I' + str(tmp), '-I' + str(ENGINE / 'include'),
                    str(ENGINE / 'src/td_combat.c'), str(ROOT / 'tests/engine/combat_harness.c'),
                    '-o', str(tmp / 'check')], check=True)
    subprocess.run([str(tmp / 'check')], check=True)
