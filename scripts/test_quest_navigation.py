"""Validate actual ROM navigation decode and ground overlay under sanitizers.

An independent C full-grid Dijkstra checks every returned route step and its
intervening full-body sweep. Host adapters do not prove ROM banking, frame
timing, visual readability or cartridge execution.
"""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
ENGINE = GAME / "project/plugins/toronto-driving/engine"


def main():
    sys.path.insert(0, str(GAME / "scripts"))
    import create_navigation as navigation
    data = navigation.model()
    generated = navigation.files(data)
    for relative, text in generated.items():
        if (ENGINE / relative).read_text() != text:
            raise ValueError(f"Registered navigation source is stale: {relative}")
    _, grids = navigation.inputs()
    bounds = "static void verify_private_decoder_bounds(void){\n"
    for unit, patterns in enumerate(data[1]):
        name = f"td_navigation_pattern_{unit}"
        bounds += f'    expect(!{name}(65535,0)&&!{name}(0,255),"Pattern-unit invalid indices are rejected before ROM reads");\n'
        ends = []
        for pattern in patterns:
            pairs = int.from_bytes(pattern[1:], "little")
            ends.append(((pairs >> ((pattern[0] - 1) * 10)) & 1023) >> 3)
        bounds += '    {static const UBYTE ends[]={' + ','.join(map(str, ends)) + '};\n'
        bounds += (f'        for(unsigned pattern=0;pattern<{len(patterns)};pattern++)\n'
                   '            for(unsigned column=ends[pattern]+1;column<128;column++)\n'
                   f'                expect(!{name}(pattern,column),"Every unused legal-width column is rejected before row-run overflow");}}\n')
    for path in sorted(ENGINE.glob("src/td_navigation_rows_*.c")):
        bounds += f'    expect({path.stem}(255,0)==65535&&{path.stem}(0,255)==65535,"Row-unit invalid fields/rows are rejected before ROM reads");\n'
    bounds += '    expect(td_navigation_ordinal(255,0,0,0)==255&&td_navigation_ordinal(0,255,0,0)==255&&td_navigation_ordinal(0,0,255,0)==255,"Shared full-body mask bounds reject invalid inputs");\n}\n'
    fixture = "static const UBYTE oracle_grids[][128*122]={\n" + "".join(
        "{" + ",".join(map(str, grid)) + "},\n" for grid in grids) + "};\n"
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if not compiler:
        raise SystemExit("Host C compiler unavailable; quest navigation checks did not run")
    with tempfile.TemporaryDirectory(prefix="toronto-quest-navigation-") as directory:
        work = Path(directory)
        shutil.copyfile(ROOT / "tests/engine/gbvm_stubs.h", work / "gbvm_stubs.h")
        shutil.copyfile(ENGINE / "src/td_navigation.c", work / "navigation_under_test.c")
        shutil.copyfile(ENGINE / "src/td_guidance.c", work / "guidance_under_test.c")
        (work / "navigation_fixture.h").write_text(fixture + 'static void expect(int,const char *);\n' + bounds)
        (work / "navigation_host.h").write_text('''#ifndef NAVIGATION_HOST_H
#define NAVIGATION_HOST_H
#include "gbvm_stubs.h"
extern UBYTE VBK_REG;
UBYTE *GetBkgAddr(void);
UBYTE get_vram_byte(UBYTE *);
void set_vram_byte(UBYTE *,UBYTE);
void set_bkg_data(UBYTE,UBYTE,const UBYTE *);
#endif
''')
        for name in ("actor", "ui", "bankdata", "scroll", "gbs_types", "compat"):
            (work / f"{name}.h").write_text('#include "navigation_host.h"\n')
        (work / "gbdk").mkdir()
        (work / "gbdk/platform.h").write_text('#include "navigation_host.h"\n')
        sources = sorted(ENGINE.glob("src/td_navigation_*.c"))
        binary = work / "quest-navigation-checks"
        subprocess.run([compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unknown-pragmas", "-fsanitize=address,undefined", "-DCGB",
                        "-I", str(work), "-I", str(ENGINE / "include"),
                        str(ROOT / "tests/engine/quest_navigation_harness.c"),
                        *map(str, sources), "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], check=False).returncode)


if __name__ == "__main__":
    main()
