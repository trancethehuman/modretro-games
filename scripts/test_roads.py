"""Host actual bulk road queries against every registered collision map.

The range adapters model the pinned NONBANKED GBVM contract; independent
pixel-footprint/continuous-segment oracles check geometry and hit-global
preservation. Host results do not establish native ABI or CPU performance.
"""
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
    world = json.loads((GAME / "content/districts/world.json").read_text())
    assert [entry["id"] for entry in world["districts"]] == list(range(len(world["districts"])))
    fixture = "static const UBYTE oracle_grids[][128*122]={\n"
    for district in world["districts"]:
        scene = json.loads((GAME / "project/project/scenes" / district["scene"] / "scene.gbsres").read_text())
        assert (scene["width"], scene["height"]) == (128, 122)
        grid = decode(scene["collisions"])
        assert len(grid) == 128 * 122 and all(0 <= value <= 255 for value in grid)
        fixture += "{" + ",".join(map(str, grid)) + "},\n"
    fixture += "};\n"
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; road checks did not run")
    with tempfile.TemporaryDirectory(prefix="toronto-road-tests-") as directory:
        work = Path(directory)
        shutil.copyfile(ROOT / "tests/engine/gbvm_stubs.h", work / "gbvm_stubs.h")
        (work / "input.h").write_text('#include "gbvm_stubs.h"\n')
        (work / "collision.h").write_text('#include "gbvm_stubs.h"\n'
            'extern UBYTE tile_hit_x,tile_hit_y;\n'
            'UBYTE tile_col_test_range_x(UBYTE,UBYTE,UBYTE,UBYTE);\n'
            'UBYTE tile_col_test_range_y(UBYTE,UBYTE,UBYTE,UBYTE);\n')
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text('#include "gbvm_stubs.h"\n')
        (work / "roads_fixture.h").write_text(fixture)
        binary = work / "roads-checks"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-fsanitize=address,undefined",
                        "-I", str(work), "-I", str(ENGINE / "include"),
                        str(ROOT / "tests/engine/roads_harness.c"), str(ENGINE / "src/td_roads.c"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
