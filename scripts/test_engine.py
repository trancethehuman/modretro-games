"""Compile behavioral fixtures around actual native C; never builds or runs a ROM.

The production driver/state-update functions are included unchanged. Thin stubs
provide input, tiles, actors, UI, clock and SRAM. The sole source adaptation maps
literal SRAM pointers into a bounded host buffer and observes actual SRAM stores.
Fixtures include each transit service's two-second departure window, adjacent
closed edges, immediate fares/ride timing, pause/deadline handling and interrupted
boarding saves against the actual registered collision/content data. Fixture
counts must match the production headers; the original core content is pinned.
This establishes logic behavior,
not GBDK ABI, Game Boy CPU timing, rendering, cartridge persistence or hardware.
"""
from pathlib import Path
import importlib.util
import host_cflags
import json
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "project/plugins/toronto-driving/engine"
FIXTURES = ROOT / "tests/engine"


def load_module(path, name):
    import sys
    if str(path.parent) not in sys.path:
        sys.path.insert(0, str(path.parent))
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def integer_macro(path, name):
    values = re.findall(r"^\s*#\s*define\s+" + re.escape(name) +
                        r"\s+([0-9]+)\s*(?://[^\n]*|/\*[^\n]*\*/)?$",
                        path.read_text(), re.M)
    if len(values) != 1:
        raise ValueError(f"Fixture requires one literal integer definition of {name} in {path.name}.")
    return int(values[0])


def require(condition, message):
    if not condition:
        raise ValueError(message)


def native_fixture(game, include):
    """Read every registered scene; never synthesize an unavailable district."""
    validator = load_module(game / "scripts/check_campaign.py", "td_collision_validator")
    core = load_module(game / "scripts/create_district_jobs.py", "td_core_content_pin")
    district_header, game_header = include / "td_district.h", include / "td_game.h"
    districts = integer_macro(district_header, "TD_DISTRICT_COUNT")
    max_districts = integer_macro(include / "td_world.h", "TD_WORLD_MAX_DISTRICTS")
    stops = integer_macro(game_header, "TD_STOPS")
    quests = integer_macro(game_header, "TD_QUESTS")
    complete_bytes = integer_macro(game_header, "TD_COMPLETE_BYTES")
    width = integer_macro(district_header, "TD_DISTRICT_TILE_WIDTH")
    height = integer_macro(district_header, "TD_DISTRICT_TILE_HEIGHT")
    require(3 <= districts <= max_districts, "Registered world must retain the three tested districts and fit the native world limit.")
    require(35 <= stops <= 64 and 80 <= quests <= min(255, complete_bytes * 8),
            "Campaign must retain existing tested content and fit native stop/bitmap limits.")
    require((width, height) == (128, 122), "Fixture requires the current native scene dimensions; review runtime bounds before changing them.")

    world = json.loads((game / "content/districts/world.json").read_text())
    entries = world["districts"]
    require(isinstance(entries, list) and len(entries) == districts and
            all(type(entry["id"]) is int for entry in entries) and
            [entry["id"] for entry in entries] == list(range(districts)),
            "Registered world IDs must be contiguous, ordered and match TD_DISTRICT_COUNT.")
    require([entry["scene"] for entry in entries] == [f"toronto_{d}_{q}" for d in ("core", "west", "high_park", "east")
                                                        for q in ("nw", "ne", "sw", "se")],
            "The tested sixteen-scene district identities must remain unchanged.")
    require(len({entry["scene"] for entry in entries}) == districts and
            len({entry["symbol"] for entry in entries}) == districts,
            "Registered scene names and symbols must be unique.")
    grids, widths, heights = [], [], []
    for entry in entries:
        require(isinstance(entry["scene"], str) and re.fullmatch(r"[a-z][a-z0-9_]*", entry["scene"]),
                "Registered scene name must identify one repository scene directory.")
        resource = game / "project/project/scenes" / entry["scene"] / "scene.gbsres"
        require(resource.is_file(), f"Registered native scene is unavailable: {entry['scene']}.")
        scene = json.loads(resource.read_text())
        require(scene["_resourceType"] == "scene" and scene["type"] == "TORONTO" and
                scene["symbol"] == entry["symbol"], f"Registered native scene identity differs: {entry['scene']}.")
        require(type(scene["width"]) is int and type(scene["height"]) is int and
                (scene["width"], scene["height"]) == (width, height) and
                (entry["width_pixels"], entry["height_pixels"]) == (width * 8, height * 8),
                f"Registered native scene dimensions differ: {entry['scene']}.")
        require(isinstance(scene["collisions"], str), f"Native collision runs must be encoded text: {entry['scene']}.")
        collision = validator.decode(scene["collisions"])
        require(len(collision) == width * height and all(type(value) is int and 0 <= value <= 255 for value in collision),
                f"Native collision fixture has inconsistent bytes or dimensions: {entry['scene']}.")
        widths.append(width); heights.append(height)
        grids.append("{" + ",".join(map(str, collision)) + "}")

    campaign = json.loads((game / "content/campaign.json").read_text())
    stop_rows, job_rows = campaign["stops"], campaign["quests"]
    require(len(stop_rows) == stops and len(job_rows) == quests,
            "Campaign fixture counts must match production TD_STOPS and TD_QUESTS.")
    require(all(type(stop["id"]) is int for stop in stop_rows) and
            [stop["id"] for stop in stop_rows] == list(range(stops)), "Campaign stop IDs must remain contiguous and ordered.")
    require([job["id"] for job in job_rows] == [f"contract-{index:02d}" for index in range(1, quests + 1)],
            "Campaign contract IDs must remain contiguous and ordered.")
    original_stops = [{field: stop[field] for field in ("id", "district", "u", "v", "name", "transit")} for stop in stop_rows[:27]]
    original_jobs = [{field: job[field] for field in core.BASE_QUEST_FIELDS} for job in job_rows[:72]]
    require(validator.canonical_sha(original_stops) == core.BASE_STOPS_SHA256 and
            all(stop["district"] >> 2 == 0 and stop.get("reserved", 0) == 0 for stop in stop_rows[:27]),
            "Original 27 core stops changed.")
    require(validator.canonical_sha(original_jobs) == core.BASE_QUESTS_SHA256,
            "Original 72 contract fields changed.")
    content = ["static const td_stop_t td_fixture_stops[TD_STOPS]={"]
    for stop in stop_rows:
        district, flags = stop.get("district", 0), stop.get("reserved", 0)
        require(type(district) is int and 0 <= district < districts and type(flags) is int and 0 <= flags <= 255 and
                type(stop["u"]) is int and 0 <= stop["u"] < width * 8 and
                type(stop["v"]) is int and 0 <= stop["v"] < height * 8 and
                type(stop["transit"]) is int and 0 <= stop["transit"] <= 4 and
                isinstance(stop["name"], str) and stop["name"].isascii() and len(stop["name"]) <= 18,
                f"Stop cannot be represented by the native fixture: {stop['id']}.")
        content.append("{%d,%d,%s,%d,%d,%d}," % (
            stop["u"], stop["v"], json.dumps(stop["name"]), stop["transit"], district, flags))
    content += ["};", "static const td_job_t td_fixture_jobs[TD_QUESTS]={"]
    for job in job_rows:
        route = job["route"]
        require(isinstance(route, list) and 2 <= len(route) <= 12 and
                all(type(index) is int and 0 <= index < stops for index in route) and
                type(job["required_vehicle"]) is int and job["required_vehicle"] in (0, 1, 2, 3, 255) and
                type(job["kind_id"]) is int and 0 <= job["kind_id"] <= 7 and
                type(job["min_completed"]) is int and 0 <= job["min_completed"] < quests and
                type(job["time_limit_seconds"]) is int and 0 < job["time_limit_seconds"] <= 65535 and
                type(job["reward"]) is int and 0 < job["reward"] <= 65535 and
                isinstance(job["title"], str) and job["title"].isascii() and len(job["title"]) <= 18,
                f"Contract cannot be represented by the native fixture: {job['id']}.")
        after = 255 if job.get("after") is None else [row["id"] for row in job_rows].index(job["after"])
        content.append("{%s,%d,%d,%d,%d,%d,%d,%d,{%s}}," % (
            json.dumps(job["title"]), job["kind_id"], len(route), job["required_vehicle"],
            job["min_completed"], after, job["time_limit_seconds"], job["reward"],
            ",".join(map(str, route + [255] * (12 - len(route))))))
    content += ["};", ""]
    return ("static const unsigned native_widths[TD_DISTRICT_COUNT]={" + ",".join(map(str, widths)) + "};\n"
            "static const unsigned native_heights[TD_DISTRICT_COUNT]={" + ",".join(map(str, heights)) + "};\n"
            "static const UBYTE native_collision[TD_DISTRICT_COUNT][TD_DISTRICT_TILE_WIDTH*TD_DISTRICT_TILE_HEIGHT]={" +
            ",".join(grids) + "};\n" + "\n".join(content))


def main():
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; engine regressions did not run.")
    # First include the real world module's private generated arrays in this
    # shared translation unit so direct portal fixtures inspect production data.
    # One translation unit: the generated people tables are emitted once,
    # at their first inclusion, for every module that reads them.
    original = ("#define TD_PEOPLE_DATA\n#define TD_PEOPLE_MIX_DATA\n#define TD_INTERIOR_DATA\n#define TD_DOORS_DATA\n" +
                (ENGINE / "src/td_street.c").read_text() + '\n' +
                (ENGINE / "src/td_transit.c").read_text() + '\n' +
                (ENGINE / "src/td_world.c").read_text() + '\n' +
                (ENGINE / "src/states/TORONTO.c").read_text() + '\n' +
                (ENGINE / "src/td_life.c").read_text() + '\n' +
                (ENGINE / "src/td_drive.c").read_text() + '\n' +
                (ENGINE / "src/td_life_draw.c").read_text() + '\n' +
                (ENGINE / "src/td_shots.c").read_text() + '\n' +
                (ENGINE / "src/td_save.c").read_text() + '\n' +
                (ENGINE / "src/td_routes.c").read_text() + '\n' +
                (ENGINE / "src/td_daynight.c").read_text() + '\n' +
                (ENGINE / "src/td_anim.c").read_text() + '\n' +
                (ENGINE / "src/td_scenery.c").read_text() + '\n' +
                (ENGINE / "src/td_overlay.c").read_text() + '\n' +
                (ENGINE / "src/td_special.c").read_text() + '\n' +
                (ENGINE / "src/td_vehicles.c").read_text() + '\n' +
                (ENGINE / "src/td_people.c").read_text() + '\n' +
                (ENGINE / "src/td_npc.c").read_text() + '\n' +
                (ENGINE / "src/td_people_text.c").read_text() + '\n' +
                (ENGINE / "src/td_menu.c").read_text() + '\n' +
                (ENGINE / "src/td_interior.c").read_text() + '\n' +
                (ENGINE / "src/td_interior_text.c").read_text())

    # Adapt the hardware address only; leave every gameplay routine unmodified.
    def host_sram(match):
        address = int(match.group("address"), 16)
        return f"({match.group('cast')})(td_test_sram + {address - 0xA000})"

    source, mapped = re.subn(
        r"\((?P<cast>\s*volatile\s+UBYTE\s*\*\s*)\)\s*(?P<address>0x[AaBb][0-9A-Fa-f]{3})\b",
        host_sram,
        original,
    )
    if not mapped:
        raise SystemExit("SRAM adapter no longer matches production source; review host fixture before running.")
    # Observe the real write order and simulate power failure after any byte store.
    # The expression and loop ordering are retained; only memory-mapped I/O is adapted.
    source, stores = re.subn(r"\bram\[([^\]]+)\]\s*=(?!=)\s*([^;]+);",
                            r"td_host_sram_store(&ram[\1],(UBYTE)(\2));", source)
    if not stores:
        raise SystemExit("SRAM store adapter no longer matches; interrupted-save tests did not run.")

    with tempfile.TemporaryDirectory(prefix="toronto-engine-tests-") as directory:
        work = Path(directory)
        (work / "engine_under_test.c").write_text(source)
        shutil.copyfile(ENGINE / "src/td_content.c", work / "content_under_test.c")
        shutil.copyfile(ENGINE / "src/td_street_names.c", work / "street_names_under_test.c")
        shutil.copyfile(ENGINE / "src/td_places.c", work / "places_under_test.c")
        game = ROOT
        (work / "native_collision_fixture.h").write_text(native_fixture(game, ENGINE / "include"))
        shutil.copyfile(FIXTURES / "gbvm_stubs.h", work / "gbvm_stubs.h")
        for name in ("actor", "camera", "scroll", "collision", "input", "data_manager", "ui", "compat", "system", "bankdata"):
            (work / f"{name}.h").write_text('#include "gbvm_stubs.h"\n')
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text('#include "gbvm_stubs.h"\n')
        binary = work / "engine-regressions"
        command = [compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", *host_cflags.extra_flags(compiler),
                   "-Wno-unknown-pragmas", "-Wno-parentheses", "-fsanitize=address,undefined",
                   "-I", str(work), "-I", str(ENGINE / "include"),
                   str(FIXTURES / "runtime_harness.c"), "-o", str(binary)]
        subprocess.run(command, check=True)
        result = subprocess.run([str(binary)], check=False)
        raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
