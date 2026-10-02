"""Exercise unchanged native map UI and atlas source with bounded VRAM adapters.

No production source is rewritten. Host tiles model the public GBDK tile-index
interface, not LCDC signed addressing, actual bank switching, raster timing,
GBVM actor rendering or hardware. Separate native playtests remain necessary.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "games/toronto-dispatch/project/plugins/toronto-driving/engine"
FIXTURES = ROOT / "tests/engine"


def main():
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; atlas UI regressions did not run.")
    with tempfile.TemporaryDirectory(prefix="toronto-atlas-ui-tests-") as directory:
        work = Path(directory)
        shutil.copyfile(ENGINE / "src/td_ui.c", work / "ui_under_test.c")
        shutil.copyfile(ENGINE / "src/td_atlas.c", work / "atlas_under_test.c")
        shutil.copyfile(ENGINE / "src/td_transit.c", work / "transit_under_test.c")
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
                        *map(str, sorted(ENGINE.glob('src/td_atlas_patterns_*.c'))),
                        *map(str, sorted(ENGINE.glob('src/td_atlas_rows_*.c'))),
                        "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == "__main__":
    main()
