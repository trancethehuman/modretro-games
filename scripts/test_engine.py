"""Compile behavioral fixtures around actual native C; never builds or runs a ROM.

The production driver/state-update functions are included unchanged. Thin stubs
provide input, tiles, actors, UI, clock and SRAM. The sole source adaptation maps
literal SRAM pointers into a bounded host buffer and observes actual SRAM stores.
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
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "games/toronto-dispatch/project/plugins/toronto-driving/engine"
FIXTURES = ROOT / "tests/engine"


def main():
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; engine regressions did not run.")
    original = ((ENGINE / "src/states/TORONTO.c").read_text() + '\n' +
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
        validator_path = ROOT / "games/toronto-dispatch/scripts/check_campaign.py"
        spec = importlib.util.spec_from_file_location("td_collision_validator", validator_path)
        validator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(validator)
        game = ROOT / "games/toronto-dispatch"
        world = json.loads((game / "content/districts/world.json").read_text())
        if [district["id"] for district in world["districts"]] != [0, 1, 2]:
            raise SystemExit("Native fixture requires the actual three registered districts.")
        grids, widths, heights = [], [], []
        for district in world["districts"]:
            scene = json.loads((game / "project/project/scenes" / district["scene"] / "scene.gbsres").read_text())
            collision = validator.decode(scene["collisions"])
            if len(collision) != scene["width"] * scene["height"] or (scene["width"], scene["height"]) != (128, 122):
                raise SystemExit("Native collision fixture has inconsistent dimensions.")
            widths.append(scene["width"])
            heights.append(scene["height"])
            grids.append("{" + ",".join(map(str, collision)) + "}")
        campaign = json.loads((game / "content/campaign.json").read_text())
        if len(campaign["stops"]) != 35 or len(campaign["quests"]) != 80:
            raise SystemExit("Host district fixtures require the integrated 35-stop/80-contract content.")
        content = ["static const td_stop_t td_fixture_stops[TD_STOPS]={"]
        for stop in campaign["stops"]:
            content.append("{%d,%d,%s,%d,%d,%d}," % (
                stop["u"], stop["v"], json.dumps(stop["name"]), stop["transit"],
                stop.get("district", 0), stop.get("reserved", 0)))
        content += ["};", "static const td_job_t td_fixture_jobs[TD_QUESTS]={"]
        for job in campaign["quests"]:
            route = job["route"] + [255] * (12 - len(job["route"]))
            content.append("{%s,%d,%d,%d,%d,%d,%d,{%s}}," % (
                json.dumps(job["title"]), job["kind_id"], len(job["route"]),
                job["required_vehicle"], job["min_completed"], job["time_limit_seconds"],
                job["reward"], ",".join(map(str, route))))
        content += ["};", ""]
        (work / "native_collision_fixture.h").write_text(
            "static const unsigned native_widths[3]={" + ",".join(map(str, widths)) + "};\n"
            "static const unsigned native_heights[3]={" + ",".join(map(str, heights)) + "};\n"
            "static const UBYTE native_collision[3][128*122]={" + ",".join(grids) + "};\n" +
            "\n".join(content))
        shutil.copyfile(FIXTURES / "gbvm_stubs.h", work / "gbvm_stubs.h")
        for name in ("actor", "camera", "scroll", "collision", "input", "data_manager", "ui", "compat", "system", "bankdata"):
            (work / f"{name}.h").write_text('#include "gbvm_stubs.h"\n')
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text('#include "gbvm_stubs.h"\n')
        binary = work / "engine-regressions"
        command = [compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
                   "-Wno-unknown-pragmas", "-Wno-parentheses", "-fsanitize=address,undefined",
                   "-I", str(work), "-I", str(ENGINE / "include"),
                   str(FIXTURES / "runtime_harness.c"), "-o", str(binary)]
        subprocess.run(command, check=True)
        result = subprocess.run([str(binary)], check=False)
        raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
