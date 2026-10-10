"""Exercise actual td_district.c with bounded resources and VM allocation stubs.

No source rewriting, plugin calls or ROM builds. Wider host pointer types and
fake banked reads intentionally cannot establish GBDK ABI or native GBVM timing.
"""
from pathlib import Path
import json
import host_cflags
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "project/plugins/toronto-driving/engine"
HARNESS = ROOT / "tests/engine/district_bridge_harness.c"


def validate_far_initializer(source, symbols):
    initializer = re.search(r"td_district_scenes\s*\[TD_DISTRICT_COUNT\]\s*=\s*\{(.*?)\};", source, re.S)
    if not initializer:
        raise SystemExit("Production far-scene initializer was not found; district tests did not run.")
    body = re.sub(r"/\*.*?\*/|//[^\n]*", "", initializer.group(1), flags=re.S)
    bindings = re.findall(r"TO_FAR_PTR_T\(\s*(\w+)\s*\)", body)
    remainder = re.sub(r"TO_FAR_PTR_T\(\s*\w+\s*\)|[\s,]", "", body)
    if remainder or bindings != symbols:
        raise SystemExit("Production far-scene bindings must exactly match registered world order/count; omitted refs must not zero-fill.")


def registered_districts(source):
    game = ROOT
    world = json.loads((game / "content/districts/world.json").read_text())
    districts = world["districts"]
    count = int(re.search(r"#define TD_DISTRICT_COUNT (\d+)", (ENGINE / "include/td_district.h").read_text()).group(1))
    if len(districts) != count or not 3 <= count <= 32 or [d["id"] for d in districts] != list(range(count)):
        raise SystemExit("Registered world and production district count/order disagree.")
    symbols = [d["symbol"] for d in districts]
    if len(set(symbols)) != count or any(not re.fullmatch(r"[A-Za-z_]\w*", symbol) for symbol in symbols):
        raise SystemExit("Registered district symbols must be unique native C identifiers.")
    validate_far_initializer(source, symbols)
    for district in districts:
        scene = json.loads((game / "project/project/scenes" / district["scene"] / "scene.gbsres").read_text())
        if scene["symbol"] != district["symbol"] or scene["type"] != "TORONTO" or (scene["width"], scene["height"]) != (128, 122):
            raise SystemExit(f"Registered native scene metadata disagrees for {district['symbol']}.")
    return districts


def test_binding_guard(source, districts):
    symbols = [district["symbol"] for district in districts]
    first, second = symbols[:2]
    omitted = re.sub(r"TO_FAR_PTR_T\(\s*" + first + r"\s*\)\s*,?", "", source, count=1)
    duplicate = re.sub(r"TO_FAR_PTR_T\(\s*" + first + r"\s*\)", "TO_FAR_PTR_T(" + second + ")", source, count=1)
    zero_fill = re.sub(r"TO_FAR_PTR_T\(\s*" + first + r"\s*\)", "{0,0}", source, count=1)
    for name, altered in (("omitted binding", omitted), ("duplicate binding", duplicate), ("explicit null binding", zero_fill)):
        try:
            validate_far_initializer(altered, symbols)
        except SystemExit:
            continue
        raise SystemExit(f"Far-scene binding guard failed to reject {name}.")
    print("District native binding guard: 3 malformed initializers rejected", flush=True)


def fixture_header(districts):
    count = len(districts)
    lines = [f"#define TD_HOST_DISTRICT_COUNT {count}",
             "static UBYTE collision_data[TD_HOST_DISTRICT_COUNT][128*122];"]
    # Distinct scene and collision banks, always different from the queued
    # WRAM script's bank1. All32 supported fixture banks fit one native byte.
    for i, district in enumerate(districts):
        symbol = district["symbol"]
        lines += [f"#define TD_HOST_BANK_{symbol} {3 + i * 7}",
                  f"const scene_t {symbol}={{128,122,{{{5 + i * 7},collision_data[{i}]}}}};"]
    lines += ["static const scene_t *const resources[]={" + ",".join("&" + d["symbol"] for d in districts) + "};",
              "static const UBYTE scene_banks[]={" + ",".join(str(3 + i * 7) for i in range(count)) + "};",
              "static const UBYTE collision_banks[]={" + ",".join(str(5 + i * 7) for i in range(count)) + "};", ""]
    return "\n".join(lines)


def main():
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; district bridge regressions did not run.")
    production = (ENGINE / "src/td_district.c").read_text()
    districts = registered_districts(production)
    test_binding_guard(production, districts)
    with tempfile.TemporaryDirectory(prefix="toronto-district-tests-") as directory:
        work = Path(directory)
        # Compile precisely the unchanged snapshot whose native bindings were
        # validated, even if another agent subsequently edits the source file.
        (work / "district_under_test.c").write_text(production)
        (work / "native_district_fixture.h").write_text(fixture_header(districts))
        # The single fixture owns thin hardware/ABI adapters. Production API
        # declarations still come from the unmodified td_district.h.
        for name in ("bankdata", "gbs_types", "collision", "data_manager", "vm", "vm_exceptions"):
            (work / f"{name}.h").write_text("/* Host declarations are supplied by district_bridge_harness.c. */\n")
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text("/* Host integer types and bank annotations come from the fixture. */\n")
        (work / "data").mkdir()
        for symbol in (district["symbol"] for district in districts):
            (work / "data" / f"{symbol}.h").write_text(f"extern const scene_t {symbol};\n")
        binary = work / "district-bridge-regressions"
        subprocess.run(
            [compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror", *host_cflags.extra_flags(compiler),
             "-Wno-unknown-pragmas", "-fsanitize=address,undefined",
             "-I", str(work), "-I", str(ENGINE / "include"),
             str(HARNESS), "-o", str(binary)], check=True,
        )
        result = subprocess.run([str(binary)], check=False)
        raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
