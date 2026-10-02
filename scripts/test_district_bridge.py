"""Exercise actual td_district.c with bounded resources and VM allocation stubs.

No source rewriting, plugin calls or ROM builds. Wider host pointer types and
fake banked reads intentionally cannot establish GBDK ABI or native GBVM timing.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "games/toronto-dispatch/project/plugins/toronto-driving/engine"
HARNESS = ROOT / "tests/engine/district_bridge_harness.c"


def main():
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; district bridge regressions did not run.")
    with tempfile.TemporaryDirectory(prefix="toronto-district-tests-") as directory:
        work = Path(directory)
        shutil.copyfile(ENGINE / "src/td_district.c", work / "district_under_test.c")
        # The single fixture owns thin hardware/ABI adapters. Production API
        # declarations still come from the unmodified td_district.h.
        for name in ("bankdata", "gbs_types", "collision", "data_manager", "vm", "vm_exceptions"):
            (work / f"{name}.h").write_text("/* Host declarations are supplied by district_bridge_harness.c. */\n")
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text("/* Host integer types and bank annotations come from the fixture. */\n")
        (work / "data").mkdir()
        for symbol in ("scene_toronto_city", "scene_toronto_west", "scene_toronto_high_park"):
            (work / "data" / f"{symbol}.h").write_text(f"extern const scene_t {symbol};\n")
        binary = work / "district-bridge-regressions"
        subprocess.run(
            [compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
             "-Wno-unknown-pragmas", "-fsanitize=address,undefined",
             "-I", str(work), "-I", str(ENGINE / "include"),
             str(HARNESS), "-o", str(binary)], check=True,
        )
        result = subprocess.run([str(binary)], check=False)
        raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
