"""Check authored/imported city sprites and sanitize actual native binding."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
ENGINE = GAME / "project/plugins/toronto-driving/engine"


def main():
    subprocess.run(["python3", str(GAME / "scripts/create_city_sprites.py"), "--check"], check=True)
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; city sprite bindings did not run")
    with tempfile.TemporaryDirectory(prefix="toronto-city-sprites-") as directory:
        work = Path(directory)
        shutil.copyfile(ENGINE / "src/td_city_sprites.c", work / "sprites_under_test.c")
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text("""#ifndef HOST_CITY_PLATFORM_H
#define HOST_CITY_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
typedef uint8_t UBYTE;
#define BANKED
#endif
""")
        (work / "actor.h").write_text("""#ifndef HOST_CITY_ACTOR_H
#define HOST_CITY_ACTOR_H
#include <gbdk/platform.h>
typedef struct { UBYTE bank; const void *ptr; } far_ptr_t;
typedef struct actor actor_t;
struct actor { actor_t *prev,*next; far_ptr_t sprite; unsigned pos_x,pos_y;
    UBYTE flags,base_tile,frame,frame_start,frame_end,anim_tick; };
#define PLAYER actors[0]
#define ACTOR_FLAG_ACTIVE 32
#define ACTOR_FLAG_HIDDEN 2
extern actor_t actors[21],*actors_inactive_head;
extern UBYTE actors_len;
void deactivate_actor(actor_t *actor);
#endif
""")
        (work / "data_manager.h").write_text('#include "actor.h"\n')
        (work / "data_manager.h").write_text('#include "actor.h"\n')
        (work / "td_district.h").write_text('#include <gbdk/platform.h>\n#define TD_DISTRICT_COUNT 5\nUBYTE td_district_current(void);\n')
        binary = work / "city-sprites-regressions"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-fsanitize=address,undefined", "-I", str(work),
                        "-I", str(ENGINE / "include"), str(ROOT / "tests/engine/city_sprites_harness.c"),
                        "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == "__main__":
    main()
