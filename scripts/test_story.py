"""Exercise the actual original story API and reconstruct its native portrait pixels.

Expected dialogue/trigger data comes from editable JSON. Pixel expectations come
from original PNGs rather than generated C arrays. Sanitized host adapters bound
VRAM writes and text rows; a native ROM must separately prove banking/readability.
"""
import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
ENGINE = GAME / "project/plugins/toronto-driving/engine"
FIXTURES = ROOT / "tests/engine"


def oracle():
    story = json.loads((GAME / "content/story.json").read_text())
    art = json.loads((GAME / "content/story_art.json").read_text())
    portraits = [entry["name"] for entry in art["portraits"]]
    assert len(story["scenes"]) == 8 and portraits == ["june", "moss", "vale"]
    rows = [f"#define ORACLE_TILE_COUNT {art['tile_count']}",
            f"#define ORACLE_PAGE_COUNT {art['story_pages']}",
            "typedef struct { UBYTE scene,portrait; const char *speaker,*lines[4]; } oracle_page_t;",
            "static const UBYTE oracle_thresholds[]={" +
            ",".join(str(scene["minimum_completed"]) for scene in story["scenes"]) + "};"]
    offset, firsts = 0, []
    for scene in story["scenes"]:
        firsts.append(offset)
        offset += len(scene["pages"])
    assert offset == art["story_pages"]
    rows.append("static const UBYTE oracle_firsts[]={" + ",".join(map(str, firsts + [offset])) + "};")
    rows.append("static const char * const oracle_titles[]={" + ",".join(json.dumps(scene["title"]) for scene in story["scenes"]) + "};")
    rows.append("static const oracle_page_t oracle_pages[]={")
    for scene in story["scenes"]:
        for page in scene["pages"]:
            rows.append("{%u,%u,%s,{%s}}," % (scene["id"], portraits.index(page["portrait"]),
                        json.dumps(page["speaker"]), ",".join(map(json.dumps, page["lines"]))))
    rows.append("};\nstatic const UBYTE oracle_pixels[3][12800]={")
    for entry in art["portraits"]:
        with Image.open(GAME / "project/original-art/story" / entry["filename"]) as image:
            assert image.mode == "P" and image.size == (160, 80)
            pixels = list(image.get_flattened_data())
            assert set(pixels) <= {0, 1, 2, 3}
            rows.append("{" + ",".join(map(str, pixels)) + "},")
    rows.append("};\n")
    return "\n".join(rows)


def main():
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; story regressions did not run.")
    subprocess.run(["python3", str(GAME / "scripts/create_story.py"), "--check"], check=True)
    with tempfile.TemporaryDirectory(prefix="toronto-story-tests-") as directory:
        work = Path(directory)
        shutil.copyfile(FIXTURES / "gbvm_stubs.h", work / "gbvm_stubs.h")
        shutil.copyfile(ENGINE / "src/td_story.c", work / "story_under_test.c")
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text("""#include "gbvm_stubs.h"
extern UBYTE VBK_REG;
void set_bkg_data(UBYTE first,UBYTE count,const UBYTE *tiles);
void set_win_tiles(UBYTE x,UBYTE y,UBYTE width,UBYTE height,const UBYTE *tiles);
""")
        (work / "data_manager.h").write_text('#include "gbvm_stubs.h"\n')
        (work / "compat.h").write_text('#include "gbvm_stubs.h"\n')
        (work / "gbs_types.h").write_text("""#ifndef STORY_HOST_TYPES_H
#define STORY_HOST_TYPES_H
#include "gbvm_stubs.h"
/* Host field-bearing adapters, not a claim about packed native ABI offsets. */
typedef struct { far_ptr_t background; } story_scene_t;
#define scene_t story_scene_t
typedef struct { far_ptr_t cgb_tileset; } background_t;
typedef struct { UWORD n_tiles; UBYTE tiles[3072]; } tileset_t;
#endif
""")
        (work / "story_oracle.h").write_text(oracle())
        binary = work / "story-regressions"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-Wno-deprecated-declarations", "-fsanitize=address,undefined", "-DCGB",
                        "-I", str(work), "-I", str(ENGINE / "include"),
                        str(FIXTURES / "story_harness.c"), "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == "__main__":
    main()
