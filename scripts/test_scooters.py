"""Actual bounded scooter/sandbox/traffic C with raw registered city grids.

Host behavior is separate from native allocation, rendering, frame timing and
cartridge evidence. Adapters model hardware ranges and external scene guards.
"""
from pathlib import Path
import importlib.util
import hashlib
import os
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
ENGINE=ROOT/'games/toronto-dispatch/project/plugins/toronto-driving/engine'
spec=importlib.util.spec_from_file_location('scooter_native_fixture',ROOT/'scripts/test_engine.py')
fixture=importlib.util.module_from_spec(spec);spec.loader.exec_module(fixture)
compiler=shutil.which(os.environ.get('CC','cc'))
assert compiler, 'Host C compiler unavailable'
# The bank split removes only the appended rider routine. Preserve the entire
# accepted R6 old traffic implementation, not just representative outputs.
assert hashlib.sha256((ENGINE/'src/td_traffic.c').read_bytes()).hexdigest()=='08caa5b91742aa5ed6c7cdf8a34656a489c25386d255db8966d17c857ca5036e', 'Bank split changed original fleet traffic source'
with tempfile.TemporaryDirectory(prefix='toronto-scooters-') as folder:
    tmp=Path(folder);shutil.copyfile(ROOT/'tests/engine/gbvm_stubs.h',tmp/'gbvm_stubs.h')
    for name in ('actor','collision','scroll','input','bankdata'):
        (tmp/f'{name}.h').write_text('#include "gbvm_stubs.h"\n')
    (tmp/'td_actor_render.h').write_text('#include "gbvm_stubs.h"\nvoid td_actor_render_actor(actor_t *actor);\n')
    (tmp/'gbdk').mkdir();(tmp/'gbdk/platform.h').write_text('#include "gbvm_stubs.h"\n')
    (tmp/'native_fixture.h').write_text(fixture.native_fixture(ROOT/'games/toronto-dispatch',ENGINE/'include'))
    (tmp/'scooter_actual.inc').write_text('\n'.join((ENGINE/'src'/name).read_text() for name in
        ('td_scooter.c','td_sandbox.c','td_traffic.c','td_rider_traffic.c')))
    subprocess.run([compiler,'-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
        '-Wno-unknown-pragmas','-Wno-unused-const-variable','-Wno-misleading-indentation',
        '-fsanitize=address,undefined','-DACTOR_H','-I'+str(tmp),'-I'+str(ENGINE/'include'),
        str(ROOT/'tests/engine/scooter_harness.c'),'-o',str(tmp/'check')],check=True)
    subprocess.run([str(tmp/'check')],check=True)
