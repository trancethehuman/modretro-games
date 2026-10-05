#!/usr/bin/env python3
"""Run actual composed native C theft/ownership/road-body fixtures with sanitizers.

The production sandbox and TORONTO callbacks are included unchanged by the
shared compiler fixture. Registered raw scene collision bytes independently
check whole parked bodies; exhaustive intervals check physical cardinal
sweeps. Host adapters do not establish GBDK timing, OAM or cartridge behavior.
"""
from pathlib import Path
import json
import os
import subprocess
import sys
root=Path(__file__).resolve().parents[1]
# The cheap full-pose cull relies on these exact original source envelopes;
# compiled metasprite/OAM behavior is independently checked by native guards.
for name,frames in (("dispatch_topdown",32),("city_fleet",20)):
    asset=root/"games/toronto-dispatch/project/assets/sprites"/(name+".png.gbsres")
    data=json.loads(asset.read_text())
    assert (data["canvasOriginX"],data["canvasOriginY"],data["canvasWidth"],data["canvasHeight"])==(8,8,16,16)
    poses=[frame for state in data["states"] for animation in state["animations"] for frame in animation["frames"]]
    assert len(poses)>=frames
    assert all(sorted((tile["x"],tile["y"]) for tile in pose["tiles"])==[(0,0),(8,0)] for pose in poses[:frames]), "Vehicle envelope changed; review parked full-pose culling."
env=dict(os.environ,TD_SANDBOX_ONLY='1')
raise SystemExit(subprocess.run([sys.executable,str(root/'scripts/test_engine.py')],cwd=root,env=env).returncode)
