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
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "games/toronto-dispatch/project/plugins/toronto-driving/engine"
FIXTURES = ROOT / "tests/engine"


def load_module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    saved_path=list(sys.path)
    try:
        sys.path.insert(0,str(path.parent))
        spec.loader.exec_module(module)
    finally:
        sys.path[:]=saved_path
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


def historical_stop(stop, districts):
    """Narrow independent geometry exception; retain the historical hashes."""
    old = {20:(444,928,0),21:(720,920,0),22:(848,896,0),
           24:(560,928,0),25:(760,944,0),26:(912,912,0)}
    new = {20:(320,280,5),21:(512,448,5),22:(920,280,5),
           24:(160,600,5),25:(512,744,5),26:(904,440,5)}
    copy = dict(stop)
    if stop['id'] in old:
        expected = new[stop['id']] if districts > 5 else old[stop['id']]
        require((stop['u'],stop['v'],stop.get('district',0)) == expected,
                f"Island stop{stop['id']} differs from its exact staged geometry.")
        copy['u'],copy['v'],copy['district'] = old[stop['id']]
    return copy


def legacy_island_fixture():
    """Expected historic points come from raw retained tiles, never ROM masks."""
    data=json.loads((FIXTURES/'legacy_island_geometry.json').read_text())
    require(data['encodedCollisionSha256']=='4de9abe15b7fe9fb42e31d345f0e0d2a726dbd94826c928a5adb562132836c92' and
            data['decodedCollisionSha256']=='081c14a28c866c8d7285f1697ac3ecff3d5429f18a2c6e321e351be0ffaec5d8',
            'Historical Island collision provenance changed.')
    import hashlib
    require(hashlib.sha256(b''.join(bytes.fromhex(region['tilesHex']) for region in data['regions'])).hexdigest()==
            '6485098fdb85691c019069d6123b029c1f9ab7d62887ae2a6f9bca881b78beff',
            'Frozen historical Island raw tile bytes changed.')
    rows=['static const UBYTE td_fixture_legacy_tiles[3][204]={']
    for region in data['regions']:
        tiles=bytes.fromhex(region['tilesHex'])
        require(len(tiles)==region['width']*region['height'] and all(tile in (15,16) for tile in tiles),
                'Historical Island raw tile subset is inconsistent.')
        rows.append('{'+','.join(map(str,tiles))+'},')
    rows.extend(['};',''])
    return '\n'.join(rows)


def legacy_north_fixture():
    """Immutable raw old Core oracle, never derived from production overlay."""
    import hashlib
    data=json.loads((FIXTURES/'legacy_north_geometry.json').read_text())
    raw=bytes.fromhex(data['collisionHex'])
    require(data['sourceSceneSha256']=='c59b93b6c66bf1b8f9b63289f20f2d248de84cbb09d29ab25f6b9ca51a40291c' and
            (data['width'],data['height'])==(128,122) and len(raw)==128*122 and
            hashlib.sha256(raw).hexdigest()=='e50f0de9eb501c110162c707836594371b289e3a58ac9ebb6b64e2bc9f24f14e',
            'Frozen pre-North Core geometry changed.')
    return 'static const UBYTE td_fixture_north_core_old[15616]={'+','.join(map(str,raw))+'};\n'


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
    require([entry["scene"] for entry in entries[:3]] == ["toronto_city", "toronto_west", "toronto_high_park"],
            "The tested core/west/High Park district identities must remain unchanged.")
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
    historical_stops = [historical_stop(stop,districts) for stop in stop_rows]
    original_stops = [{field: stop[field] for field in ("id", "u", "v", "name", "transit")} for stop in historical_stops[:27]]
    original_jobs = [{field: job[field] for field in core.BASE_QUEST_FIELDS} for job in job_rows[:72]]
    require(validator.canonical_sha(original_stops) == core.BASE_STOPS_SHA256 and
            all(stop.get("district", 0) == 0 and stop.get("reserved", 0) == 0 for stop in historical_stops[:27]),
            "Original 27 core stops changed.")
    require(validator.canonical_sha(original_jobs) == core.BASE_QUESTS_SHA256,
            "Original 72 contract fields changed.")
    # Snapshot the complete pre-Port campaign, independently of the content
    # generator. Old saves identify these records by ordinal, not by name.
    # Planning/source metadata can evolve without changing native semantics.
    prefix_stops = [{field: stop.get(field, 0) for field in
                     ("id", "u", "v", "name", "transit", "district", "reserved")}
                    for stop in historical_stops[:51]]
    prefix_jobs = [{field: job[field] for field in
                    ("id", "title", "brief", "kind_id", "required_vehicle",
                     "min_completed", "route", "time_limit_seconds", "reward")}
                   for job in job_rows[:88]]
    require(validator.canonical_sha(prefix_stops) ==
            "68f6104038ab9724ca197de13ecef7528ab5e8a4713ee8d05cff8b8004a73325",
            "The existing 51 native stop identities/fields changed.")
    require(validator.canonical_sha(prefix_jobs) ==
            "ae5d1177fc297337b7efe449144d6ffbe188c333d3607d6710080c9d065e130a",
            "The existing 88 native contracts or briefings changed.")
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
        content.append("{%s,%d,%d,%d,%d,%d,%d,{%s}}," % (
            json.dumps(job["title"]), job["kind_id"], len(route), job["required_vehicle"],
            job["min_completed"], job["time_limit_seconds"], job["reward"],
            ",".join(map(str, route + [255] * (12 - len(route))))))
    content += ["};", ""]
    return ("static const unsigned native_widths[TD_DISTRICT_COUNT]={" + ",".join(map(str, widths)) + "};\n"
            "static const unsigned native_heights[TD_DISTRICT_COUNT]={" + ",".join(map(str, heights)) + "};\n"
            "static const UBYTE native_collision[TD_DISTRICT_COUNT][TD_DISTRICT_TILE_WIDTH*TD_DISTRICT_TILE_HEIGHT]={" +
            ",".join(grids) + "};\n" + "\n".join(content)+legacy_island_fixture()+legacy_north_fixture())


def main():
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; engine regressions did not run.")
    # First include the real world module's private generated arrays in this
    # shared translation unit so direct portal fixtures inspect production data.
    original = ((ENGINE / "src/td_transit.c").read_text() + '\n' +
                (ENGINE / "src/td_world.c").read_text() + '\n' +
                (ENGINE / "src/td_streetcar.c").read_text() + '\n' +
                (ENGINE / "src/td_streetcar_runtime.c").read_text() + '\n' +
                (ENGINE / "src/td_aircraft.c").read_text() + '\n' +
                (ENGINE / "src/td_people.c").read_text() + '\n' +
                (ENGINE / "src/td_traffic.c").read_text() + '\n' +
                (ENGINE / "src/td_roads.c").read_text() + '\n' +
                (ENGINE / "src/td_terrain.c").read_text() + '\n' +
                (ENGINE / "src/states/TORONTO.c").read_text() + '\n' +
                (ENGINE / "src/td_save.c").read_text() + '\n' +
                (ENGINE / "src/td_routes.c").read_text())

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
        game = ROOT / "games/toronto-dispatch"
        (work / "native_collision_fixture.h").write_text(native_fixture(game, ENGINE / "include"))
        shutil.copyfile(FIXTURES / "gbvm_stubs.h", work / "gbvm_stubs.h")
        for name in ("actor", "camera", "scroll", "collision", "input", "data_manager", "ui", "compat", "system", "bankdata", "gbs_types"):
            (work / f"{name}.h").write_text('#include "gbvm_stubs.h"\n')
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text('#include "gbvm_stubs.h"\n')
        binary = work / "engine-regressions"
        command = [compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
                   "-Wno-unknown-pragmas", "-Wno-parentheses", "-fsanitize=address,undefined",
                   "-I", str(work), "-I", str(ENGINE / "include"),
                   str(FIXTURES / "runtime_harness.c"), str(ENGINE / "src/td_police.c"),
                   str(ENGINE / "src/td_police_lanes.c"),
                   str(ENGINE / "src/td_traffic_signal_stop.c"), "-o", str(binary)]
        subprocess.run(command, check=True)
        result = subprocess.run([str(binary)], check=False)
        raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
