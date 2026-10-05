"""Check authored/imported city sprites and sanitize actual native binding."""
from pathlib import Path
import json
import hashlib
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
ENGINE = GAME / "project/plugins/toronto-driving/engine"


def main():
    subprocess.run(["python3", str(GAME / "scripts/create_city_sprites.py"), "--check"], check=True)
    from PIL import Image
    # The taxi is appended. All previously approved service poses and exact
    # native IDs remain protected independently from the current generator.
    native=json.loads((GAME/'project/assets/sprites/city_fleet.png.gbsres').read_text())
    frames=native['states'][0]['animations'][0]['frames']
    assert len(frames)==21 and not frames[20]['tiles']
    assert hashlib.sha256(json.dumps(frames[:16],sort_keys=True,separators=(',',':')).encode()).hexdigest()=='ba0d3f4f4882686f86afc30d71009a2789f6b946b9e0720051f52e989032343d'
    with Image.open(GAME/'project/assets/sprites/city_fleet.png') as image:
        assert image.size==(160,16)
        assert hashlib.sha256(image.convert('RGB').crop((0,0,128,16)).tobytes()).hexdigest()=='1b7a7beadad9294ef1ff2013fa9512e4e71fba94e8bfbd3bc3bf2878a7d9ffdd'
    assert all(len(frame['tiles'])==2 and all(t['paletteIndex']==6 for t in frame['tiles']) for frame in frames[16:20])
    count = int(re.search(r"^#define TD_DISTRICT_COUNT (\d+)$",
                         (ENGINE / "include/td_district.h").read_text(), re.M)[1])
    districts = json.loads((GAME / "content/districts/world.json").read_text())["districts"]
    assert len(districts) == count and [d["id"] for d in districts] == list(range(count))
    loader_first = []
    for district in districts:
        actors = GAME / "project/project/scenes" / district["scene"] / "actors"
        fleet = json.loads((actors / "city_fleet_loader.gbsres").read_text())
        civilians = json.loads((actors / "city_civilians_loader.gbsres").read_text())
        assert civilians["_index"] == fleet["_index"] + 1
        # Native actor0 is the player; editor actor index0 becomes actor1.
        loader_first.append(fleet["_index"] + 1)
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
typedef uint16_t UWORD;
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
extern actor_t actors[22],*actors_inactive_head;
extern UBYTE actors_len;
void deactivate_actor(actor_t *actor);
#endif
""")
        (work / "data_manager.h").write_text('#include "actor.h"\n')
        (work / "bankdata.h").write_text('#include "actor.h"\n')
        (work / "city_loader_fixture.h").write_text(
            '#include "td_district.h"\nstatic const UBYTE host_loader_first[TD_DISTRICT_COUNT]={' +
            ','.join(map(str, loader_first)) + '};\n')
        binary = work / "city-sprites-regressions"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-DACTOR_H", "-fsanitize=address,undefined", "-I", str(work),
                        "-I", str(ENGINE / "include"), str(ROOT / "tests/engine/city_sprites_harness.c"),
                        "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == "__main__":
    main()
