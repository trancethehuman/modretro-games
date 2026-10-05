"""Run the actual driving unit against bounded geometry/contact adapters.

Checks genuine stationary steering, forward/reverse yaw, cardinal traction,
slower cruise caps, held-throttle impact recovery and fresh-foot interaction.
Vehicle rebound is checked in all16 headings, in reverse, and against rear
terrain/vehicle/tram obstructions with bounded impulse and held input.
This is host logic evidence; native CPU/rendering/hardware remain separate.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
ENGINE=ROOT/'games/toronto-dispatch/project/plugins/toronto-driving/engine'
compiler=shutil.which(os.environ.get('CC','cc'))
assert compiler, 'Host C compiler unavailable'
with tempfile.TemporaryDirectory(prefix='toronto-motion-feedback-') as folder:
    tmp=Path(folder)
    shutil.copyfile(ROOT/'tests/engine/gbvm_stubs.h',tmp/'gbvm_stubs.h')
    for name in ('actor','input','compat','collision','scroll','camera','bankdata'):
        (tmp/f'{name}.h').write_text('#include "gbvm_stubs.h"\n')
    (tmp/'gbdk').mkdir()
    (tmp/'gbdk/platform.h').write_text('#include "gbvm_stubs.h"\n')
    subprocess.run([compiler,'-std=c11','-O1','-g','-Wall','-Wextra','-Wno-unknown-pragmas',
                    '-fsanitize=address,undefined','-DACTOR_H','-I'+str(tmp),'-I'+str(ENGINE/'include'),
                    str(ENGINE/'src/td_motion.c'),str(ROOT/'tests/engine/motion_feedback_harness.c'),
                    '-o',str(tmp/'check')],check=True)
    subprocess.run([str(tmp/'check')],check=True)
