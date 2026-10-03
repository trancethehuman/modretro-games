"""Host actual police/road/traffic C against registered collision fixtures.

Checks continuous bounded navigation and caller admission, not native timing,
ABI, wanted/save rules, sprite presentation or physical hardware. The fused
traffic mover's future-tram link adapter fails closed if unexpectedly called;
its behavior is exercised by the separate traffic/runtime harnesses.
"""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
ENGINE = GAME / "project/plugins/toronto-driving/engine"


def main():
    sys.path.insert(0, str(GAME / "scripts"))
    from check_campaign import decode
    spec = importlib.util.spec_from_file_location("police_signals", GAME / "scripts/create_traffic_signals.py")
    source = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(source)
    nodes = source.model()
    world = json.loads((GAME / "content/districts/world.json").read_text())
    fixture = "static const UBYTE oracle_grids[][128*122]={\n"
    for district in world["districts"]:
        scene = json.loads((GAME / "project/project/scenes" / district["scene"] / "scene.gbsres").read_text())
        grid = decode(scene["collisions"])
        assert len(grid) == 128 * 122
        fixture += "{" + ",".join(map(str, grid)) + "},\n"
    fixture += "};\nstatic const UWORD oracle_nodes[][3]={\n"
    fixture += "".join("{%d,%d,%d},\n" % (district, *node[:2])
                       for district, values in enumerate(nodes) for node in values)
    fixture += "};\n"
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; police checks did not run")
    with tempfile.TemporaryDirectory(prefix="toronto-police-tests-") as directory:
        work = Path(directory)
        shutil.copyfile(ROOT / "tests/engine/gbvm_stubs.h", work / "gbvm_stubs.h")
        for name in ("actor", "bankdata", "input"):
            (work / f"{name}.h").write_text('#include "gbvm_stubs.h"\n')
        (work / "collision.h").write_text('#include "gbvm_stubs.h"\n'
            'extern UBYTE tile_hit_x,tile_hit_y;\n'
            'UBYTE tile_col_test_range_x(UBYTE,UBYTE,UBYTE,UBYTE);\n'
            'UBYTE tile_col_test_range_y(UBYTE,UBYTE,UBYTE,UBYTE);\n')
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text('#include "gbvm_stubs.h"\n')
        (work / "police_fixture.h").write_text(fixture)
        binary = work / "police-checks"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-fsanitize=address,undefined",
                        "-I", str(work), "-I", str(ENGINE / "include"),
                        str(ROOT / "tests/engine/police_harness.c"),
                        str(ENGINE / "src/td_police.c"), str(ENGINE / "src/td_roads.c"),
                        str(ENGINE / "src/td_traffic.c"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
