"""Host actual police/road/traffic C against registered collision fixtures.

Checks continuous bounded navigation and caller admission, not native timing,
ABI, wanted/save rules, sprite presentation or physical hardware. The fused
traffic mover's future-tram link adapter fails closed if unexpectedly called;
its behavior is exercised by the separate traffic/runtime harnesses.
"""
import importlib.util
import json
import os
import re
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
    road_spec = importlib.util.spec_from_file_location("police_roads", GAME / "scripts/create_police_roads.py")
    road_source = importlib.util.module_from_spec(road_spec)
    road_spec.loader.exec_module(road_source)
    road_data = road_source.model()
    assert (ENGINE / "include/td_police_road_data.h").read_text() == road_source.source(road_data)
    assert sum(len(d["vertices"]) for d in road_data) < 32768
    assert road_data[5]["vertices"] == [] and road_data[5]["goals"] == []
    world = json.loads((GAME / "content/districts/world.json").read_text())
    count = int(re.search(r"^#define TD_DISTRICT_COUNT (\d+)$",
                         (ENGINE / "include/td_district.h").read_text(), re.M)[1])
    assert len(world["districts"]) == count
    assert [d["id"] for d in world["districts"]] == list(range(count))
    assert all(d.get("traffic_enabled", True) for d in world["districts"][:5])
    assert world["districts"][5]["scene"] == "toronto_islands" and not world["districts"][5]["traffic_enabled"]
    fixture = "static const UBYTE oracle_grids[][128*122]={\n"
    for district in world["districts"]:
        scene = json.loads((GAME / "project/project/scenes" / district["scene"] / "scene.gbsres").read_text())
        grid = decode(scene["collisions"])
        assert len(grid) == 128 * 122
        fixture += "{" + ",".join(map(str, grid)) + "},\n"
    fixture += "};\nstatic const UBYTE oracle_traffic_enabled[TD_DISTRICT_COUNT]={" + ",".join(
        "1" if d.get("traffic_enabled", True) else "0" for d in world["districts"]) + "};\n"
    fixture += "static const UWORD oracle_nodes[][3]={\n"
    fixture += "".join("{%d,%d,%d},\n" % (district, *node[:2])
                       for district, values in enumerate(nodes) for node in values)
    fixture += "};\nstatic const UWORD oracle_vertices[][3]={\n"
    fixture += "".join("{%d,%d,%d},\n" % (district, row[0]*8, row[1]*8)
                       for district, data in enumerate(road_data) for row in data["vertices"])
    fixture += "};\nstatic const UWORD oracle_patrol[][TD_POLICE_MAX_GOALS][2]={\n"
    fixture = "#define TD_POLICE_MAX_GOALS 8\n" + fixture
    for data in road_data:
        fixture += "{" + ",".join("{%d,%d}" % point for point in data["goals"]) + "},\n"
    fixture += "};\nstatic const UBYTE oracle_patrol_counts[]={" + ",".join(str(len(d["goals"])) for d in road_data) + "};\n"
    fixture += "static const UWORD oracle_edges[][5]={\n"
    fixture += "".join("{%d,%d,%d,%d,%d},\n" % (district, *a, *b)
                       for district, data in enumerate(road_data) for a,b,_ in data["edges"])
    fixture += "};\nstatic const UWORD oracle_roads[][5]={\n"
    for district in world["districts"]:
        roads, _ = road_source.road_source(district)
        for road in roads:
            for a,b in zip(road["points"],road["points"][1:]):
                fixture += "{%d,%d,%d,%d,%d},\n" % (district["id"],*a,*b)
    fixture += "};\nstatic const UWORD oracle_junctions[][3]={\n"
    for district in world["districts"]:
        roads, _ = road_source.road_source(district)
        segments = [(a,b) for road in roads for a,b in zip(road["points"],road["points"][1:])]
        crossings = {(c[0],a[1]) for a,b in segments for c,d in segments if a[1]==b[1] and c[0]==d[0]
                     and min(a[0],b[0])<=c[0]<=max(a[0],b[0]) and min(c[1],d[1])<=a[1]<=max(c[1],d[1])}
        fixture += "".join("{%d,%d,%d},\n" % (district["id"],*p) for p in sorted(crossings))
    fixture += "};\nstatic const UWORD oracle_endcaps[][5]={\n"
    fixture += "".join("{%d,%d,%d,%d,%d},\n" % (district,*a,*b)
                       for district,data in enumerate(road_data) for a,b,owners in data["edges"]
                       if any(owner[0] in ("endcap","patrol_endcap") for owner in owners))
    fixture += "};\n"
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; police checks did not run")
    with tempfile.TemporaryDirectory(prefix="toronto-police-tests-") as directory:
        work = Path(directory)
        shutil.copyfile(ROOT / "tests/engine/gbvm_stubs.h", work / "gbvm_stubs.h")
        for name in ("actor", "bankdata", "input", "data_manager"):
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
                    "-Wno-unknown-pragmas", "-fsanitize=address,undefined", "-DACTOR_H",
                        "-I", str(work), "-I", str(ENGINE / "include"),
                        str(ROOT / "tests/engine/police_harness.c"),
                        str(Path(sys.argv[sys.argv.index("--source")+1]) if "--source" in sys.argv else ENGINE / "src/td_police.c"),
                        str(ENGINE / "src/td_police_lanes.c"), str(ENGINE / "src/td_roads.c"),
                        str(ENGINE / "src/td_traffic.c"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
