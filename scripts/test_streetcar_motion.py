"""Exercise unchanged Queen pose/sweep C against independent native-grid oracles.

Only GBDK integer types and banking annotations are adapted. Registered scene
collision bytes are decoded without importing art/path generators. These tests
do not establish ROM banking, rendered motion, OAM limits or hardware behavior.
"""
from pathlib import Path
import json
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
ENGINE = GAME / "project/plugins/toronto-driving/engine"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def collision_bytes(encoded, length):
    result, end = [], 0
    for token in re.finditer(r"([0-9a-fA-F]{2})(!|[0-9a-fA-F]+\+)", encoded):
        require(token.start() == end, "Registered collision byte runs contain a gap.")
        count = 1 if token[2] == "!" else int(token[2][:-1], 16)
        require(0 < count <= length - len(result), "Registered collision run exceeds its grid.")
        result.extend([int(token[1], 16)] * count)
        end = token.end()
    require(end == len(encoded) and len(result) == length,
            "Registered collision runs must exactly fill the native scene.")
    return result


def native_fixture():
    world = json.loads((GAME / "content/districts/world.json").read_text())
    districts = world["districts"]
    header = (ENGINE / "include/td_district.h").read_text()
    count = int(re.search(r"#define TD_DISTRICT_COUNT (\d+)", header)[1])
    require(len(districts) == count and [d["id"] for d in districts] == list(range(count)),
            "Registered world IDs and native district count disagree.")
    require([d["scene"] for d in districts[:4]] ==
            ["toronto_city", "toronto_west", "toronto_high_park", "toronto_east"],
            "Review changes to the Queen corridor's native district identities.")
    rows = []
    for district in districts:
        resource = GAME / "project/project/scenes" / district["scene"] / "scene.gbsres"
        scene = json.loads(resource.read_text())
        require(scene["symbol"] == district["symbol"] and scene["type"] == "TORONTO" and
                (scene["width"], scene["height"]) == (128, 122),
                "Registered native collision resource identity/dimensions differ.")
        grid = collision_bytes(scene["collisions"], 128 * 122)
        rows.append("{" + ",".join(map(str, grid)) + "}")
    campaign = json.loads((GAME / "content/campaign.json").read_text())
    platforms = campaign["stops"][43:51]
    require(len(platforms) == 8 and [s["id"] for s in platforms] == list(range(43, 51)) and
            all(s["transit"] == 4 for s in platforms), "Queen platforms must retain IDs43..50.")
    platform_rows = [f"{{{s['u']},{s['v']},{s['district']}}}" for s in platforms]
    return (f"#define HOST_DISTRICTS {count}\n"
            "static const unsigned char host_collision[HOST_DISTRICTS][128*122]={\n" +
            ",\n".join(rows) + "\n};\n"
            "static const struct {unsigned u,v,district;} host_platforms[8]={" +
            ",".join(platform_rows) + "};\n")


def main():
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; streetcar regressions did not run.")
    fixture = native_fixture()
    with tempfile.TemporaryDirectory(prefix="toronto-streetcar-tests-") as directory:
        work = Path(directory)
        for name in ("td_streetcar", "td_transit"):
            shutil.copyfile(ENGINE / f"src/{name}.c", work / f"{name}.c")
            shutil.copyfile(ENGINE / f"include/{name}.h", work / f"{name}.h")
        shutil.copyfile(ENGINE / "include/td_district.h", work / "td_district.h")
        (work / "native_streetcar_fixture.h").write_text(fixture)
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text("""#ifndef HOST_PLATFORM_H
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
""")
        (work / "bankdata.h").write_text("""#ifndef HOST_BANKDATA_H
#define HOST_BANKDATA_H
typedef struct { UBYTE bank; const void *ptr; } far_ptr_t;
#endif
""")
        binary = work / "streetcar-regressions"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-fsanitize=address,undefined", "-I", str(work),
                        str(work / "td_streetcar.c"), str(work / "td_transit.c"),
                        str(ROOT / "tests/engine/streetcar_harness.c"), "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == "__main__":
    main()
