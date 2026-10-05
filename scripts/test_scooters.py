"""Actual bounded scooter/sandbox/traffic C with raw registered city grids.

Host behavior is separate from native allocation, rendering, frame timing and
cartridge evidence. Adapters model hardware ranges and external scene guards.
"""
from pathlib import Path
import importlib.util
import hashlib
import base64
import gzip
import json
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
    # Compile an immutable original R8 implementation separately. Replaying
    # whole updates protects ordering, residual clocks, wrong scenes, wrecks,
    # vacant shoves, ram/owner behavior and render poses beyond pure queries.
    history=json.loads((ROOT/'tests/fixtures/scooter_performance_before.json').read_text())
    assert history['schema']==1 and history['baselineRomSha256']=='bdcebd6ff4463cb745f2fe47255119b381d5a098e57a8ba27d5652bfc2a97b0d'
    original={}
    pins={'td_scooter.c':'00a0347974d2101afbe97aa03afcaba2853162d9bce2f2cd7b9c08eeea1c5285',
          'td_rider_traffic.c':'bc6ffbc8228a8cbc5c55b969b6e73a8cb5dc9c0d808b1b2adc57d0f8ebf342ef'}
    for name,pin in pins.items():
        row=history['sources'][name];raw=gzip.decompress(base64.b64decode(row['gzipBase64']))
        assert row['sha256']==pin and hashlib.sha256(raw).hexdigest()==pin, 'Original R8 replay source changed'
        original[name]=raw.decode()
    shared=(ROOT/'tests/engine/scooter_harness.c').read_text().split('static int raw_body(',1)[0]
    (tmp/'scooter_replay.c').write_text(shared+(ROOT/'tests/engine/scooter_performance_harness.c').read_text())
    traces=[]
    for label in ('original-r8','candidate'):
        (tmp/'scooter_actual.inc').write_text('\n'.join(original[name] if label=='original-r8' and name in original
            else (ENGINE/'src'/name).read_text() for name in ('td_scooter.c','td_sandbox.c','td_traffic.c','td_rider_traffic.c')))
        binary=tmp/label
        subprocess.run([compiler,'-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
            '-Wno-unknown-pragmas','-Wno-unused-const-variable','-Wno-unused-function','-Wno-misleading-indentation',
            '-fsanitize=address,undefined','-DACTOR_H','-I'+str(tmp),'-I'+str(ENGINE/'include'),
            str(tmp/'scooter_replay.c'),'-o',str(binary)],check=True)
        traces.append(subprocess.run([str(binary)],stdout=subprocess.PIPE,check=True).stdout)
    assert traces[0]==traces[1], 'Whole actual scooter update replay differs from original R8'
    print(json.dumps({'wholeOriginalR8Replay':{'snapshots':7*80*80,'exactStateBytesCompared':len(traces[0]),
        'traceSha256':hashlib.sha256(traces[0]).hexdigest(),'sourceBodiesPinned':pins,
        'scope':'Host actual-C behavior only; no native timing, renderer allocation or hardware claim'}},indent=2))
