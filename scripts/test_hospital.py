"""Check original hospital badge, real collision footprints and native renderer.

The sanitizer exercises actual C with independent VRAM/window adapters. Source
location and full-foot checks do not establish native timing or hardware visuals.
"""
from pathlib import Path
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / 'games/toronto-dispatch'
ENGINE = GAME / 'project/plugins/toronto-driving/engine'
sys.path.insert(0, str(GAME / 'scripts'))
from create_navigation import inputs
from PIL import Image

hospital = json.loads((GAME / 'content/hospital.json').read_text())
header = (ENGINE / 'include/td_hospital.h').read_text()
code = (ENGINE / 'src/td_hospital.c').read_text()
source = next(s for s in json.loads((GAME / 'content/sources.json').read_text())
              if s['id'] == hospital['source'])
assert source['url'] == 'https://www.uhn.ca/corporate/Directions/Pages/directions_TGH.aspx'
assert source['reviewed'] == hospital['reviewed'] == '2026-10-05'
assert hospital['district'] == 0 and hospital['scene'] == 'toronto_city'
constants = {name: int(value) for name, value in re.findall(r'#define (TD_HOSPITAL_\w+) (\d+)', header)}
assert constants == {'TD_HOSPITAL_DISTRICT': 0, 'TD_HOSPITAL_U': 504,
                     'TD_HOSPITAL_V': 344, 'TD_HOSPITAL_MARKER_U': 512,
                     'TD_HOSPITAL_MARKER_V': 344, 'TD_HOSPITAL_TILE': 254}
assert hospital['exit'] == {'u': constants['TD_HOSPITAL_U'], 'v': constants['TD_HOSPITAL_V']}
marker = hospital['marker']
assert (marker['u'], marker['v'], marker['width'], marker['height'], marker['tile_bank'],
        marker['tile'], marker['palette']) == (512, 344, 8, 8, 1, 254, 7)
assert marker['original_art'] and not marker['uses_official_logo']
assert marker['new_oam_objects'] == marker['new_mutable_state_bytes'] == 0
assert len(marker['original_cells']) == 8 and all(len(row) == 8 for row in marker['original_cells'])
assert all(c in '0123' for row in marker['original_cells'] for c in row)
pixels = []
for row in marker['original_cells']:
    pixels.extend(sum((int(c) & (1 << plane) != 0) << (7 - x) for x, c in enumerate(row))
                  for plane in range(2))
body = re.search(r'td_hospital_badge\[\]\s*=\s*\{([^}]+)\}', code).group(1)
assert [int(v, 16) for v in re.findall(r'0x[0-9a-fA-F]+', body)] == pixels
palette = json.loads((GAME / 'project/project/palettes/default_ui.gbsres').read_text())['colors']
expected = [tuple(int(palette[int(c)][i:i+2], 16) for i in (0, 2, 4))
            for row in marker['original_cells'] for c in row]
with Image.open(GAME / marker['source_png']) as image:
    image = image.convert('RGB')
    assert image.size == (8, 8) and [image.getpixel((x, y)) for y in range(8) for x in range(8)] == expected
goals, grids = inputs()
assert (0, 504, 344) not in goals, 'The hospital must not renumber or add quest/navigation goals'
grid = grids[0]
half = hospital['recovery']['foot_half_width']
assert half == 3 and hospital['recovery']['runtime_requires_full_body_and_live_vehicle_clearance']
expected_offsets = [(0, 0), (16, 0), (-16, 0), (0, 16), (0, -16),
                    (16, 16), (-16, 16), (16, -16), (-16, -16)]
candidates = hospital['recovery']['candidates']
assert len(candidates) == len(expected_offsets)
for point, (dx, dy) in zip(candidates, expected_offsets):
    u, v = point['u'], point['v']
    assert (u, v) == (504 + dx, 344 + dy)
    clear = all(not grid[y * 128 + x] & 15 for y in range((v - half) // 8, (v + half) // 8 + 1)
                for x in range((u - half) // 8, (u + half) // 8 + 1))
    assert clear == point['raw_body_clear'], f'Hospital full-foot geometry changed at {u}/{v}'
assert sum(point['raw_body_clear'] for point in candidates) == 6
assert hospital['campaign_changes'] == {'new_quest_stops': 0, 'renumbered_quest_stops': 0, 'new_navigation_goals': 0}
compiler = shutil.which(os.environ.get('CC', 'cc'))
assert compiler, 'Host C compiler unavailable'
with tempfile.TemporaryDirectory(prefix='toronto-hospital-') as folder:
    tmp = Path(folder)
    shutil.copyfile(ROOT / 'tests/engine/gbvm_stubs.h', tmp / 'gbvm_stubs.h')
    shutil.copyfile(ENGINE / 'src/td_hospital.c', tmp / 'hospital_under_test.c')
    (tmp / 'gbdk').mkdir()
    (tmp / 'gbdk/platform.h').write_text('#include "gbvm_stubs.h"\n#define CGB 1\n')
    (tmp / 'bankdata.h').write_text('#include "gbvm_stubs.h"\n')
    for name in ('scroll', 'ui', 'compat'):
        (tmp / f'{name}.h').write_text('#include "host_hospital.h"\n')
    (tmp / 'host_hospital.h').write_text('''#ifndef HOST_HOSPITAL_H
#define HOST_HOSPITAL_H
#include "gbvm_stubs.h"
extern UBYTE VBK_REG,WX_REG,WY_REG,win_pos_x,win_pos_y,win_dest_pos_x,win_dest_pos_y;
UBYTE *GetBkgAddr(void);
UBYTE get_vram_byte(UBYTE *address);
void set_vram_byte(UBYTE *address,UBYTE value);
#endif
''')
    subprocess.run([compiler, '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unknown-pragmas', '-fsanitize=address,undefined', '-DACTOR_H',
                    '-I' + str(tmp), '-I' + str(ENGINE / 'include'),
                    str(ROOT / 'tests/engine/hospital_render_harness.c'), '-o', str(tmp / 'check')], check=True)
    subprocess.run([str(tmp / 'check')], check=True)
print('Hospital geography: original source/pixels and all nine complete-foot collision candidates verified')
