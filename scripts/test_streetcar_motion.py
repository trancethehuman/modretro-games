"""Exercise actual Queen pose/sweep C against independent native-grid oracles.

Only GBDK integer types and banking annotations are adapted. Registered scene
collision bytes are decoded without importing art/path generators. These tests
also expose the unchanged private interpolation helper in a host-only wrapper;
the value oracle uses wide arithmetic and independently authored distances.
They do not establish ROM banking, CPU savings, rendered motion, OAM limits or
hardware behavior.
"""
from pathlib import Path
import json
import os
import re
import shutil
import subprocess
import sys
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


def progress_probe(source):
    """Expose actual private code/data; never use generated values as an oracle.

    Host pointers are wider than the native two-byte same-bank pointers. The
    production SDCC pointer guard establishes that ABI at ROM build time; here
    count actual declarations/elements and explicitly model native data bytes.
    """
    rows = re.findall(r"static\s+const\s+UWORD\s+(td_streetcar_progress_[0-9]+)\[([0-9]+)\]", source)
    require(len(rows) == 6 and len({name for name, _ in rows}) == 6 and
            all(int(count) == 121 for _, count in rows),
            "Review the six constant121-tick interpolation resources before changing their native budget.")
    require(re.search(r"static\s+const\s+UWORD\s*\*\s*const\s+td_streetcar_progress_routes\[16\]", source),
            "Interpolation route pointers must remain ROM constants in the same compilation unit.")
    require("td_streetcar_progress_near_pointer_fits" in source and
            re.search(r"sizeof\(td_streetcar_progress_routes\[0\]\)\s*==\s*2", source),
            "Native two-byte near-pointer ABI must retain its compile-time guard.")
    stripped = re.sub(r"/\*.*?\*/|//[^\n]*", "", source, flags=re.S)
    helper = re.search(r"static\s+UWORD\s+td_streetcar_progress\(UBYTE\s+route,UBYTE\s+tick\)\s*\{([^}]+)\}", stripped)
    require(helper and "/" not in helper[1] and "%" not in helper[1] and
            not re.search(r"\btd_streetcar_length\s*\(", stripped),
            "Private interpolation must not retain runtime division or route-length folding.")
    sizes = "+".join(f"sizeof({name})" for name, _ in rows)
    print("Streetcar source resource gate: six const121-tick tables,16 same-bank pointers; "
          "1484 modeled native ROM bytes, no interpolation division or runtime length fold.", flush=True)
    return f"""
/* Host-only private probes. Production source above is unchanged. */
UWORD host_streetcar_progress(UBYTE route,UBYTE tick){{return td_streetcar_progress(route,tick);}}
unsigned host_streetcar_progress_value_bytes(void){{return {sizes};}}
unsigned host_streetcar_progress_route_count(void){{
    return sizeof(td_streetcar_progress_routes)/sizeof(td_streetcar_progress_routes[0]);
}}
"""


def main():
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; streetcar regressions did not run.")
    subprocess.run([sys.executable, "-B", str(GAME / "scripts/create_streetcar_progress.py"), "--check"], check=True)
    production = (ENGINE / "src/td_streetcar.c").read_text()
    probe = progress_probe(production)
    fixture = native_fixture()
    with tempfile.TemporaryDirectory(prefix="toronto-streetcar-tests-") as directory:
        work = Path(directory)
        for name in ("td_streetcar", "td_transit"):
            shutil.copyfile(ENGINE / f"src/{name}.c", work / f"{name}.c")
            shutil.copyfile(ENGINE / f"include/{name}.h", work / f"{name}.h")
        (work / "td_streetcar_under_test.c").write_text(production + probe)
        shutil.copyfile(ENGINE / "include/td_district.h", work / "td_district.h")
        shutil.copyfile(ENGINE / "include/td_game.h", work / "td_game.h")
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
                        str(work / "td_streetcar_under_test.c"), str(work / "td_transit.c"),
                        str(ROOT / "tests/engine/streetcar_harness.c"), "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == "__main__":
    main()
