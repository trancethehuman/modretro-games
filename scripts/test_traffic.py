"""Host actual banked traffic C with independent geometry/phase fixtures.

This checks logic and actual registered junction data, not a ROM build, native
CPU/ABI, rendering, emergency art, save behavior or physical hardware.
"""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
ENGINE = GAME / "project/plugins/toronto-driving/engine"


def main():
    spec = importlib.util.spec_from_file_location("td_signal_source", GAME / "scripts/create_traffic_signals.py")
    module = importlib.util.module_from_spec(spec)
    import sys
    sys.path.insert(0, str(GAME / "scripts"))
    spec.loader.exec_module(module)
    data = module.model()
    assert module.HEADER.read_text() == module.source(data), "Traffic signal source is stale"
    offsets = [0]
    for district in data:
        offsets.append(offsets[-1] + len(district))
    fixture = "static const unsigned oracle_offsets[]={" + ",".join(map(str, offsets)) + "};\n"
    fixture += "#ifndef TD_TRAFFIC_RENDER_FIXTURE\nstatic const unsigned oracle_signals[][3]={" + ",".join(
        "{%d,%d,%d}" % node[:3] for district in data for node in district) + "};\n"
    fixture += "#else\nstatic const unsigned oracle_lights[][5]={" + ",".join(
        "{%d,%d,%d,%d,%d}" % node for district in data for node in district) + "};\n#endif\n"
    visual=json.loads((GAME/'content/traffic_signal_art.json').read_text())['heads']
    flat=[(d,*node) for d,nodes in enumerate(data) for node in nodes]
    assert len(visual)==len(flat)
    for p,(d,u,v,arms,x,y) in zip(visual,flat):
        assert (p['district'],*p['junction'],p['arms'],*p['ew_cell'])==(d,u,v,arms,x,y)
        xx,yy=p['ns_cell'];slug=['city','west','high_park','east','port_lands','islands','north'][d]
        # Whole-junction coarse culls depend on these exact authored offsets.
        # Check both poles independently of the renderer's binary search.
        for hx,hy in (p['ew_cell'],p['ns_cell']):
            assert -32<=hx*8-u<=32 and -32<=hy*8-v<=24
        scene=json.loads((GAME/f'project/project/scenes/toronto_{slug}/scene.gbsres').read_text())
        bg=json.loads((GAME/f'project/assets/backgrounds/toronto_{slug}.png.gbsres').read_text())
        assert abs(xx-x)+abs(yy-y)==1 and 0<=xx<128 and 0<=yy<122
        assert not module.decode(scene['collisions'])[yy*128+xx]&15 and not module.decode(bg['tileColors'])[yy*128+xx]&128
    fixture+='#ifdef TD_TRAFFIC_RENDER_FIXTURE\nstatic const unsigned oracle_extra[][2]={'+','.join('{%d,%d}'%tuple(p['ns_cell']) for p in visual)+'};\n#endif\n'
    # Terrain reuse is checked against the editable scenes themselves. Keep
    # this fixture out of the renderer TU, which has a separate VRAM oracle.
    world=json.loads((GAME/'content/districts/world.json').read_text())
    assert [d['id'] for d in world['districts']]==list(range(len(world['districts'])))
    fixture+='#ifndef TD_TRAFFIC_RENDER_FIXTURE\nstatic const UBYTE oracle_road_grids[][128*122]={\n'
    for district in world['districts']:
        scene=json.loads((GAME/'project/project/scenes'/district['scene']/'scene.gbsres').read_text())
        assert (scene['width'],scene['height'])==(128,122)
        grid=module.decode(scene['collisions'])
        assert len(grid)==128*122 and all(0<=tile<=255 for tile in grid)
        fixture+='{'+','.join(map(str,grid))+'},\n'
    fixture+='};\n#endif\n'
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; traffic checks did not run")
    with tempfile.TemporaryDirectory(prefix="toronto-traffic-tests-") as directory:
        work = Path(directory)
        shutil.copyfile(ROOT / "tests/engine/gbvm_stubs.h", work / "gbvm_stubs.h")
        for name in ("actor", "bankdata", "gbs_types", "collision"):
            (work / f"{name}.h").write_text('#include "gbvm_stubs.h"\n')
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text('#include "gbvm_stubs.h"\n')
        (work / "traffic_fixture.h").write_text(fixture)
        # Compile the real declaration under byte packing, matching GBVM's
        # native layout. The sanitizer executable keeps its normal host ABI.
        (work / "traffic_layout.c").write_text(
            '#pragma pack(push,1)\n#include "td_traffic.h"\n#pragma pack(pop)\n'
            '_Static_assert(sizeof(td_traffic_epoch_t)==111,"Native traffic epoch must be111 bytes");\n')
        subprocess.run([compiler, "-std=c11", "-DACTOR_H", "-Wall", "-Wextra", "-Werror", "-fsyntax-only",
                        "-I", str(work), "-I", str(ENGINE / "include"), str(work / "traffic_layout.c")], check=True)
        binary = work / "traffic-checks"
        subprocess.run([compiler, "-std=c11", "-DACTOR_H", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-fsanitize=address,undefined",
                        "-I", str(work), "-I", str(ENGINE / "include"),
                        str(ROOT / "tests/engine/traffic_harness.c"), str(ENGINE / "src/td_traffic.c"),
                        str(ENGINE / "src/td_traffic_signal_stop.c"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        (work / "scroll.h").write_text('#include "gbvm_stubs.h"\nextern WORD draw_scroll_x,draw_scroll_y;\n')
        (work / "compat.h").write_text('#include "gbvm_stubs.h"\nextern UBYTE VBK_REG;\n'
                                      'UBYTE *GetBkgAddr(void);\nUBYTE get_vram_byte(UBYTE *);\nvoid set_vram_byte(UBYTE *,UBYTE);\n'
                                      'void set_bkg_data(UBYTE,UBYTE,const UBYTE *);\n')
        binary = work / "traffic-lights-checks"
        light_source=(ENGINE/'src/td_traffic_lights.c').read_text()
        loop='for(head=0;head<2;head++){'
        assert light_source.count(loop)==1
        (work/'traffic_lights_under_test.c').write_text('extern unsigned host_signal_head_steps;\n'+
            light_source.replace(loop,loop+'host_signal_head_steps++;'))
        subprocess.run([compiler, "-std=c11", "-DACTOR_H", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-fsanitize=address,undefined", "-DCGB",
                        "-I", str(work), "-I", str(ENGINE / "include"),
                        str(ROOT / "tests/engine/traffic_lights_harness.c"), str(work / "traffic_lights_under_test.c"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
