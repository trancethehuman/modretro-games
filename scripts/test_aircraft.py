"""Run sanitizer fixtures against unchanged production ambient-flight C.

Only platform storage types and the BANKED qualifier are adapted. This does not
build a ROM, verify native sprite rendering, or establish physical performance.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "games/toronto-dispatch/project/plugins/toronto-driving/engine"


def main():
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; aircraft regressions did not run.")
    with tempfile.TemporaryDirectory(prefix="toronto-aircraft-tests-") as directory:
        work = Path(directory)
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text(
            "#include <stdint.h>\n"
            "typedef uint8_t UBYTE;\n"
            "typedef uint16_t UWORD;\n"
            "typedef int16_t WORD;\n"
            "#define BANKED\n"
        )
        binary = work / "aircraft-regressions"
        command = [compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
                   "-Wno-unknown-pragmas", "-fsanitize=address,undefined",
                   "-I", str(work), "-I", str(ENGINE / "include"),
                   str(ROOT / "tests/engine/aircraft_harness.c"),
                   str(ENGINE / "src/td_aircraft.c"), "-o", str(binary)]
        subprocess.run(command, check=True)
        result = subprocess.run([str(binary)], check=False)
        raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
