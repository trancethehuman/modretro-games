"""Exercise actual native map/dispatch/result UI with bounded VRAM adapters.

No production source is rewritten. Host tiles model the public GBDK tile-index
interface, not LCDC signed addressing, actual bank switching, raster timing,
GBVM actor rendering or hardware. Separate native playtests remain necessary.
"""
from pathlib import Path
import json
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "games/toronto-dispatch/project/plugins/toronto-driving/engine"
FIXTURES = ROOT / "tests/engine"


def content_oracle():
    """Expected authored text comes from editable JSON, never parsed C rows."""
    game = ROOT / "games/toronto-dispatch"
    campaign = json.loads((game / "content/campaign.json").read_text())
    districts = json.loads((game / "content/districts/world.json").read_text())["districts"]
    stops, jobs = campaign["stops"], campaign["quests"]
    quote = json.dumps
    if [stop["id"] for stop in stops] != list(range(len(stops))):
        raise ValueError("UI stop oracle needs contiguous authored stop IDs")
    if any(len(job["route"]) > 12 or not 2 <= len(job["route"]) for job in jobs):
        raise ValueError("UI oracle requires meaningful routes within the actual native twelve-stop buffer")
    for text in [stop["name"] for stop in stops] + [job["title"] for job in jobs] + [entry["name"] for entry in districts]:
        if not text.isascii() or len(text) > 18:
            raise ValueError("Authored UI text exceeds its native eighteen-character field")
    rows = ["/* Independent expected content from editable campaign/world JSON. */",
            "static const td_stop_t host_ui_stops[]={"]
    rows.extend("{%u,%u,%s,%u,%u,%u}," %
                (stop["u"], stop["v"], quote(stop["name"]), stop["transit"], stop["district"], stop.get("reserved", 0))
                for stop in stops)
    rows.append("};\nstatic const td_job_t host_ui_jobs[]={")
    for job in jobs:
        route = job["route"] + [255] * (12 - len(job["route"]))
        rows.append("{%s,%u,%u,%u,%u,%u,%u,{%s}}," %
                    (quote(job["title"]), job["kind_id"], len(job["route"]), job["required_vehicle"],
                     job["min_completed"], job["time_limit_seconds"], job["reward"], ",".join(map(str, route))))
    rows.append("};\nstatic const char host_ui_briefs[][37]={")
    for job in jobs:
        if len(job["brief"]) != 2 or any(len(line) > 18 or not line.isascii() for line in job["brief"]):
            raise ValueError("UI brief oracle requires two bounded authored lines")
        rows.append(quote("".join(line.ljust(18) for line in job["brief"])) + ",")
    rows.append("};\nstatic const char host_ui_districts[][19]={")
    rows.extend(quote(entry["name"]) + "," for entry in districts)
    rows.append("};\nstatic const UWORD host_ui_atlas_origins[][2]={")
    for entry in districts:
        if any(type(entry[axis]) is not int or entry[axis] < 0 or entry[axis] % 8
               for axis in ("atlas_x", "atlas_y")):
            raise ValueError("UI atlas oracle requires nonnegative authored tile-aligned origins")
        rows.append("{%u,%u}," % (entry["atlas_x"], entry["atlas_y"]))
    width = max(entry["atlas_x"] + entry["width_pixels"] for entry in districts)
    height = max(entry["atlas_y"] + entry["height_pixels"] for entry in districts)
    rows.append("};\n#define HOST_UI_ATLAS_TILE_WIDTH %u\n#define HOST_UI_ATLAS_TILE_HEIGHT %u" %
                ((width + 63) // 64, (height + 63) // 64))
    rows.append("static const char host_ui_chapters[][21]={")
    # Compact authored captions are UI requirements. Validate their groups
    # against editable quest metadata instead of reading the renderer table.
    captions = {
        "First shift": "FIRST SHIFT",
        "Neighbourhood connections": "NEIGHBOURHOODS",
        "City events": "CITY EVENTS",
        "Crossing the city": "CROSS THE CITY",
        "Waterfront work": "WATERFRONT",
        "Arts and audiences": "ARTS/AUDIENCES",
        "Evening dispatch": "EVENING",
        "Across the network": "NETWORK",
        "Master courier": "MASTER COURIER",
        "Western package routes": "WEST ROUTES",
        "Eastern package connections": "EAST ROUTES",
        "Port Lands loading and park rounds": "PORT LANDS",
        "Northern hills and station rounds": "UPTOWN HILLS",
    }
    if len(jobs) != 104:
        raise ValueError("Review the thirteen authored dispatch groups after campaign changes")
    for start in range(0, len(jobs), 8):
        chapter = jobs[start]["chapter"]
        if chapter not in captions or any(job["chapter"] != chapter for job in jobs[start:start + 8]):
            raise ValueError("Dispatch group differs from authored eight-offer chapter metadata")
        caption = f"{start // 8 + 1:02}/13 {captions[chapter]}"
        if len(caption) > 20:
            raise ValueError("Chapter caption exceeds the native twenty-column row")
        rows.append(quote(caption) + ",")
    rows.append("};\n")
    return "\n".join(rows)


def district_name_adapter():
    """Use the unchanged real engine adapter; no scene/driver is linked here."""
    source = (ENGINE / "src/states/TORONTO.c").read_text()
    match = re.search(r"^void td_get_district_name\([^\n]+\) BANKED \{\n[^{}]+\n\}", source, re.M)
    if not match:
        raise ValueError("Actual district-name adapter changed; review its renderer-test dependencies")
    return '#include <string.h>\n#include "td_game.h"\n#include "td_world.h"\n' + match.group() + "\n"


def main():
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; atlas UI regressions did not run.")
    with tempfile.TemporaryDirectory(prefix="toronto-atlas-ui-tests-") as directory:
        work = Path(directory)
        shutil.copyfile(ENGINE / "src/td_ui.c", work / "ui_under_test.c")
        shutil.copyfile(ENGINE / "src/td_atlas.c", work / "atlas_under_test.c")
        shutil.copyfile(ENGINE / "src/td_transit.c", work / "transit_under_test.c")
        shutil.copyfile(ENGINE / "src/td_content.c", work / "content_under_test.c")
        shutil.copyfile(ENGINE / "src/td_world.c", work / "world_under_test.c")
        (work / "district_name_adapter.c").write_text(district_name_adapter())
        (work / "ui_content_oracle.h").write_text(content_oracle())
        shutil.copyfile(FIXTURES / "gbvm_stubs.h", work / "gbvm_stubs.h")
        (work / "ui_host.h").write_text("""#ifndef TD_ATLAS_UI_HOST_H
#define TD_ATLAS_UI_HOST_H
#include "gbvm_stubs.h"
extern UBYTE VBK_REG,text_drawn;
void set_win_tiles(UBYTE x,UBYTE y,UBYTE width,UBYTE height,const UBYTE *tiles);
void set_bkg_data(UBYTE first,UBYTE count,const UBYTE *tiles);
void ui_set_pos(UBYTE x,UBYTE y);
#endif
""")
        for name in ("actor", "ui", "camera", "scroll", "system", "bankdata", "data_manager"):
            (work / f"{name}.h").write_text('#include "ui_host.h"\n')
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text('#include "ui_host.h"\n')
        binary = work / "atlas-ui-regressions"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-Wno-deprecated-declarations", "-fsanitize=address,undefined",
                        "-I", str(work), "-I", str(ENGINE / "include"),
                        str(FIXTURES / "atlas_ui_harness.c"), str(work / "transit_under_test.c"),
                        str(work / "content_under_test.c"), str(work / "world_under_test.c"),
                        str(work / "district_name_adapter.c"),
                        *map(str, sorted(ENGINE.glob('src/td_atlas_patterns_*.c'))),
                        *map(str, sorted(ENGINE.glob('src/td_atlas_rows_*.c'))),
                        "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == "__main__":
    main()
