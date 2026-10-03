"""Sanitize actual people clock/contact optimization against independent oracles."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "games/toronto-dispatch/project/plugins/toronto-driving/engine"


def rail_queries():
    """Extract unchanged query/helper bodies without the unrelated actor/scene ABI.

    The complete runtime is exercised by the repository's full engine harness.
    This focused unit links these exact bodies to actual timetable/sweep C.
    """
    source = (ENGINE / "src/td_streetcar_runtime.c").read_text()
    functions = []
    for name in ("mode", "box", "overlap", "near", "clear", "held", "held_body",
                 "foot_clear", "pedestrian_clear"):
        name = "td_streetcar_runtime_" + name
        match = re.search(r"(?m)^(?:static )?UBYTE " + name + r"\([^;]*?\)\s*(?:BANKED\s*)?\{", source)
        if not match:
            raise ValueError(f"Missing actual production query {name}")
        end, depth = match.end(), 1
        while depth:
            if source[end] == "{":
                depth += 1
            elif source[end] == "}":
                depth -= 1
            end += 1
        functions.append(source[match.start():end])
    return "\n\n".join(functions)


def main():
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; people hotspot checks did not run")
    with tempfile.TemporaryDirectory(prefix="toronto-people-hotspots-") as directory:
        work = Path(directory)
        shutil.copyfile(ENGINE / "src/td_people.c", work / "people_under_test.c")
        (work / "rail_queries_under_test.c").write_text(rail_queries())
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text("""#ifndef HOST_PEOPLE_PLATFORM_H
#define HOST_PEOPLE_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
typedef uint8_t UBYTE;
typedef uint16_t UWORD;
typedef int16_t WORD;
#define BANKED
#define TRUE 1
#define FALSE 0
#endif
""")
        (work / "actor.h").write_text("""#ifndef HOST_PEOPLE_ACTOR_H
#define HOST_PEOPLE_ACTOR_H
#include <gbdk/platform.h>
typedef struct { struct { UWORD x,y; } pos; UBYTE flags; } actor_t;
#define ACTOR_FLAG_HIDDEN 2
extern actor_t actors[21];
#endif
""")
        (work / "bankdata.h").write_text("""#ifndef HOST_PEOPLE_BANKDATA_H
#define HOST_PEOPLE_BANKDATA_H
#include <gbdk/platform.h>
typedef struct { UBYTE bank; const void *ptr; } far_ptr_t;
#endif
""")
        binary = work / "people-hotspot-regressions"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-fsanitize=address,undefined", "-I", str(work),
                        "-I", str(ENGINE / "include"), str(ROOT / "tests/engine/people_hotspot_harness.c"),
                        str(ENGINE / "src/td_streetcar.c"), str(ENGINE / "src/td_transit.c"),
                        "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == "__main__":
    main()
