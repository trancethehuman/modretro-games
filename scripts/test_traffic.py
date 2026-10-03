"""Host actual banked traffic C with independent geometry/phase fixtures.

This checks logic and actual registered junction data, not a ROM build, native
CPU/ABI, rendering, emergency art, save behavior or physical hardware.
"""
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
ENGINE = GAME / "project/plugins/toronto-driving/engine"


def main():
    spec = importlib.util.spec_from_file_location("td_signal_source", GAME / "scripts/create_traffic_signals.py")
    module = importlib.util.module_from_spec(spec)
    import sys
    sys.path.insert(0, str(GAME / "scripts"))
    spec.loader.exec_module(module)
    data = module.model()
    assert module.HEADER.read_text() == module.source(data), "Traffic signal source is stale"
    offsets = [0]
    for district in data:
        offsets.append(offsets[-1] + len(district))
    fixture = "static const unsigned oracle_offsets[]={" + ",".join(map(str, offsets)) + "};\n"
    fixture += "#ifndef TD_TRAFFIC_RENDER_FIXTURE\nstatic const unsigned oracle_signals[][3]={" + ",".join(
        "{%d,%d,%d}" % node[:3] for district in data for node in district) + "};\n"
    fixture += "#else\nstatic const unsigned oracle_lights[][5]={" + ",".join(
        "{%d,%d,%d,%d,%d}" % node for district in data for node in district) + "};\n#endif\n"
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; traffic checks did not run")
    with tempfile.TemporaryDirectory(prefix="toronto-traffic-tests-") as directory:
        work = Path(directory)
        shutil.copyfile(ROOT / "tests/engine/gbvm_stubs.h", work / "gbvm_stubs.h")
        for name in ("actor", "bankdata", "gbs_types"):
            (work / f"{name}.h").write_text('#include "gbvm_stubs.h"\n')
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text('#include "gbvm_stubs.h"\n')
        (work / "traffic_fixture.h").write_text(fixture)
        # Compile the real declaration under byte packing, matching GBVM's
        # native layout. The sanitizer executable keeps its normal host ABI.
        (work / "traffic_layout.c").write_text(
            '#pragma pack(push,1)\n#include "td_traffic.h"\n#pragma pack(pop)\n'
            '_Static_assert(sizeof(td_traffic_epoch_t)==87,"Native traffic epoch must be87 bytes");\n')
        subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-fsyntax-only",
                        "-I", str(work), "-I", str(ENGINE / "include"), str(work / "traffic_layout.c")], check=True)
        binary = work / "traffic-checks"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-fsanitize=address,undefined",
                        "-I", str(work), "-I", str(ENGINE / "include"),
                        str(ROOT / "tests/engine/traffic_harness.c"), str(ENGINE / "src/td_traffic.c"),
                        str(ENGINE / "src/td_traffic_signal_stop.c"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        (work / "scroll.h").write_text('#include "gbvm_stubs.h"\nextern WORD draw_scroll_x,draw_scroll_y;\n')
        (work / "compat.h").write_text('#include "gbvm_stubs.h"\nextern UBYTE VBK_REG;\n'
                                      'UBYTE *GetBkgAddr(void);\nUBYTE get_vram_byte(UBYTE *);\nvoid set_vram_byte(UBYTE *,UBYTE);\n'
                                      'void set_bkg_data(UBYTE,UBYTE,const UBYTE *);\n')
        binary = work / "traffic-lights-checks"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-fsanitize=address,undefined", "-DCGB",
                        "-I", str(work), "-I", str(ENGINE / "include"),
                        str(ROOT / "tests/engine/traffic_lights_harness.c"), str(ENGINE / "src/td_traffic_lights.c"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
