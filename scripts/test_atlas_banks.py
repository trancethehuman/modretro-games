"""Compile generated atlas banking against independent synthetic pixel oracles.

All generated files live in temporary directories. Host BANKED annotations
cannot prove Game Boy bank switching, native timing or cartridge execution.
"""
import copy
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / 'games/toronto-dispatch'
sys.path.insert(0, str(GAME / 'scripts'))
import atlas_banks
import create_atlas
import test_atlas


def synthetic_dictionary():
    # Keep the four existing positions, then append a disconnected synthetic
    # scene with fractional atlas-tile alignment to exercise both padding axes.
    districts = [
        {'id': i, 'name': f'FIXTURE {i}', 'x': x, 'y': y, 'width': 128, 'height': 122}
        for i, (x, y) in enumerate(((256, 0), (128, 0), (0, 0), (384, 0), (507, 395)))
    ]
    width, height, tile_width, tile_height = 635, 517, 80, 65
    padded_width, padded_height = tile_width * 8, tile_height * 8
    pixels = [0] * (padded_width * padded_height)
    for y in range(height):
        for x in range(width):
            if not any(d['x'] <= x < d['x'] + 128 and d['y'] <= y < d['y'] + 122 for d in districts):
                continue
            # Neighbouring tile pairs share a key, so each20x12 viewport stays
            # below172 while the whole dictionary exceeds512 distinct tiles.
            key = (y // 8) * 40 + (x // 8) // 2 + 1
            pixels[y * padded_width + x] = (key >> (2 * (((y % 8) * 8 + x % 8) % 6))) & 3
    patterns, indices, lookup = [], [], {}
    for ty in range(tile_height):
        for tx in range(tile_width):
            tile = [pixels[(ty * 8 + y) * padded_width + tx * 8 + x] for y in range(8) for x in range(8)]
            packed = create_atlas.pack_tile(tile).hex()
            if packed not in lookup:
                lookup[packed] = len(patterns)
                patterns.append(packed)
            indices.append(lookup[packed])
    worst = max(len({indices[yy * tile_width + xx]
                     for yy in range(y, y + 12) for xx in range(x, x + 20)})
                for y in range(tile_height - 11) for x in range(tile_width - 19))
    assert len(patterns) > 512 and len(indices) > 4096 and worst <= 172
    data = {'districts': districts, 'width_pixels': width, 'height_pixels': height,
            'tile_width': tile_width, 'tile_height': tile_height,
            'patterns_2bpp_hex': patterns, 'tile_pattern_indices': indices,
            'budgets': {'dictionary_patterns': len(patterns), 'worst_visible_patterns': worst}}
    metas = ['{%d,%d,%d,1024,976,%s}' % (d['id'], d['x'], d['y'], json.dumps(d['name'])) for d in districts]
    oracle = ['typedef struct { UBYTE id; UWORD x,y,width,height; const char *name; } oracle_district_t;',
              '#define ORACLE_DISTRICT_COUNT 5', f'#define ORACLE_WIDTH {width}', f'#define ORACLE_HEIGHT {height}',
              f'#define ORACLE_PADDED_WIDTH {padded_width}', f'#define ORACLE_PADDED_HEIGHT {padded_height}',
              'static const oracle_district_t oracle_districts[]={' + ','.join(metas) + '};',
              'static const UBYTE oracle_pixels[ORACLE_PADDED_WIDTH*ORACLE_PADDED_HEIGHT]={']
    oracle += [','.join(map(str, pixels[start:start + padded_width])) + ','
               for start in range(0, len(pixels), padded_width)]
    oracle += ['};', '']
    return data, '\n'.join(oracle)


def compile_fixture(compiler, work, data, oracle):
    work.mkdir()
    chunks = create_atlas.files(data)
    assert chunks == atlas_banks.files(data) and create_atlas.source(data) == atlas_banks.source(data)
    expected_patterns, expected_rows = atlas_banks.units(data)
    assert sum(name.startswith('src/td_atlas_patterns_') for name in chunks) == expected_patterns
    assert sum(name.startswith('src/td_atlas_rows_') for name in chunks) == expected_rows
    sources = []
    for name, text in chunks.items():
        path = work / Path(name).name
        path.write_text(text)
        if name.endswith('.c'):
            sources.append(path)
            match = re.search(r'static const UBYTE patterns\[(\d+)\]\[16\]', text)
            if match:
                assert int(match[1]) * 16 <= atlas_banks.UNIT_DATA_LIMIT
            else:
                match = re.search(r'static const UWORD indices\[(\d+)\]', text)
                assert match and int(match[1]) * 2 <= atlas_banks.UNIT_DATA_LIMIT
    (work / 'atlas_under_test.c').write_text(create_atlas.source(data))
    (work / 'td_atlas.h').write_text(create_atlas.header(data))
    (work / 'td_district.h').write_text('#include <gbdk/platform.h>\n' +
        f'#define TD_DISTRICT_COUNT {len(data["districts"])}\n' +
        '#define TD_DISTRICT_PIXEL_WIDTH 1024\n#define TD_DISTRICT_PIXEL_HEIGHT 976\n')
    (work / 'atlas_oracle.h').write_text(oracle)
    (work / 'gbdk').mkdir()
    (work / 'gbdk/platform.h').write_text('''#ifndef HOST_ATLAS_PLATFORM_H
#define HOST_ATLAS_PLATFORM_H
#include <stdint.h>
typedef uint8_t UBYTE;
typedef int8_t BYTE;
typedef uint16_t UWORD;
typedef int16_t WORD;
#define BANKED
#define TRUE 1
#define FALSE 0
#endif
''')
    binary = work / 'banked-atlas-regressions'
    subprocess.run([compiler, '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unknown-pragmas', '-fsanitize=address,undefined', '-I', str(work),
                    str(ROOT / 'tests/engine/atlas_harness.c'), *map(str, sources), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    return expected_patterns, expected_rows


def encoded_grid(grid):
    out, start = [], 0
    while start < len(grid):
        end = start + 1
        while end < len(grid) and grid[end] == grid[start]:
            end += 1
        out.append(f'{grid[start]:02x}' + ('!' if end - start == 1 else f'{end-start:x}+'))
        start = end
    return ''.join(out)


def appended_source_fixture(base):
    """Model a fifth registered resource in temp files, including new water."""
    game = base / 'synthetic-game'
    engine = game / 'project/plugins/toronto-driving/engine'
    (engine / 'include').mkdir(parents=True)
    original_header = (create_atlas.ENGINE / 'include/td_district.h').read_text()
    (engine / 'include/td_district.h').write_text(re.sub(r'#define TD_DISTRICT_COUNT \d+', '#define TD_DISTRICT_COUNT 5', original_header))
    world = json.loads((GAME / 'content/districts/world.json').read_text())
    world['districts'] = world['districts'][:4]
    world['districts'].append({'id': 4, 'name': 'SYNTHETIC WATER', 'scene': 'toronto_fixture',
                              'symbol': 'scene_toronto_fixture', 'width_pixels': 1024, 'height_pixels': 976,
                              'atlas_x': 4056, 'atlas_y': 3160})
    (game / 'content/districts').mkdir(parents=True)
    world_path = game / 'content/districts/world.json'
    world_path.write_text(json.dumps(world))
    for district in world['districts'][:4]:
        scene = Path('project/project/scenes') / district['scene'] / 'scene.gbsres'
        (game / scene).parent.mkdir(parents=True)
        shutil.copyfile(GAME / scene, game / scene)
        meta = Path('content/city_art.json') if district['id'] == 0 else (
            Path('content/districts') / (district['scene'].removeprefix('toronto_') + '_art.json'))
        shutil.copyfile(GAME / meta, game / meta)
    grid = []
    for y in range(122):
        for x in range(128):
            grid.append(0 if 480 <= x * 8 + 4 < 544 else 16 if 688 <= y * 8 + 4 < 720 else 15)
    scene_path = game / 'project/project/scenes/toronto_fixture/scene.gbsres'
    scene_path.parent.mkdir(parents=True)
    scene_path.write_text(json.dumps({'type': 'TORONTO', 'symbol': 'scene_toronto_fixture',
                                     'width': 128, 'height': 122, 'collisions': encoded_grid(grid)}))
    (game / 'content/districts/fixture_art.json').write_text(json.dumps({
        'dimensions': [1024, 976], 'collisions': grid, 'water': [[0, 600, 1024, 376]],
        'pond': [[100, 100], [300, 100], [300, 300], [100, 300]],
    }))
    old_create_root, old_create_engine = create_atlas.ROOT, create_atlas.ENGINE
    old_test_game, old_test_engine = test_atlas.GAME, test_atlas.ENGINE
    try:
        create_atlas.ROOT, create_atlas.ENGINE = game, engine
        test_atlas.GAME, test_atlas.ENGINE = game, engine
        data = create_atlas.model()
        # This is explicitly a temporary synthetic fifth scene, not the
        # production six-district registration guarded by the default oracle.
        oracle = test_atlas.fixture_header(registered_islands=False)
        assert (data['tile_width'], data['tile_height']) == (80, 65)
        assert data['budgets']['index_units'] == 2 and data['budgets']['max_unit_data_bytes'] <= 8192
        assert data['districts'][4]['water_shapes']['rectangles'] == [[0, 600, 1024, 976]]
        # Read packed source pixels at independently selected native tile centres.
        def pixel(u, v):
            x, y = 507 + u // 8, 395 + v // 8
            pattern = data['tile_pattern_indices'][(y // 8) * 80 + x // 8]
            tile = bytes.fromhex(data['patterns_2bpp_hex'][pattern])
            bit = 7 - x % 8
            return ((tile[(y % 8) * 2] >> bit) & 1) | (((tile[(y % 8) * 2 + 1] >> bit) & 1) << 1)
        assert pixel(804, 804) == 3 and pixel(516, 804) == 1 and pixel(804, 700) == 2
        assert pixel(204, 204) == 3 and pixel(404, 204) == 0
        cases = []
        changed = copy.deepcopy(world);changed['districts'][4]['atlas_y'] = 0;changed['districts'][4]['atlas_x'] = 3072
        cases.append((changed, 'overlap'))
        changed = copy.deepcopy(world);changed['districts'][0]['atlas_y'] = 8
        cases.append((changed, 'Existing atlas layout'))
        changed = copy.deepcopy(world);changed['districts'][4]['id'] = 3
        cases.append((changed, 'contiguous'))
        changed = copy.deepcopy(world);changed['districts'][4]['atlas_x'] = 15360
        cases.append((changed, 'UBYTE'))
        changed = copy.deepcopy(world);changed['districts'][0]['scene'] = 'toronto_fixture'
        cases.append((changed, 'Reassigned'))
        for changed, reason in cases:
            world_path.write_text(json.dumps(changed))
            try:
                create_atlas.model()
            except ValueError as error:
                assert reason in str(error), f'Expected {reason!r}, got {str(error)!r}'
            else:
                raise AssertionError(f'Invalid appended atlas model was accepted: {reason}')
        world_path.write_text(json.dumps(world))
        return data, oracle, len(cases)
    finally:
        create_atlas.ROOT, create_atlas.ENGINE = old_create_root, old_create_engine
        test_atlas.GAME, test_atlas.ENGINE = old_test_game, old_test_engine


def main():
    compiler = shutil.which(os.environ.get('CC', 'cc'))
    if not compiler:
        raise SystemExit('Host C compiler unavailable; atlas banking regressions did not run.')
    data, oracle = synthetic_dictionary()
    invalid = []
    changed = copy.deepcopy(data);changed['tile_width'] = 256;invalid.append(changed)
    changed = copy.deepcopy(data);changed['tile_pattern_indices'].pop();invalid.append(changed)
    changed = copy.deepcopy(data);changed['tile_pattern_indices'][0] = len(data['patterns_2bpp_hex']);invalid.append(changed)
    changed = copy.deepcopy(data);changed['tile_pattern_indices'][0] = True;invalid.append(changed)
    changed = copy.deepcopy(data);changed['patterns_2bpp_hex'][0] = '00';invalid.append(changed)
    changed = copy.deepcopy(data);changed['districts'][4]['x'] += 1;invalid.append(changed)
    changed = copy.deepcopy(data);changed['districts'][4]['name'] = 'INVALID " NAME';invalid.append(changed)
    for changed in invalid:
        try:
            atlas_banks.files(changed)
        except ValueError:
            pass
        else:
            raise AssertionError('Malformed banked data was accepted')
    with tempfile.TemporaryDirectory(prefix='toronto-atlas-banks-') as directory:
        base = Path(directory)
        units = compile_fixture(compiler, base / 'dictionary', data, oracle)
        source_data, source_oracle, model_rejections = appended_source_fixture(base)
        compile_fixture(compiler, base / 'registered', source_data, source_oracle)
    assert units == (math.ceil(len(data['patterns_2bpp_hex']) / 512), 2)
    print(f'Banked atlas host fixtures:80x65 tiles,{len(data["patterns_2bpp_hex"])} patterns,5200 indices, '
          f'{units[0]} pattern/{units[1]} index units; {len(invalid)} malformed data and '
          f'{model_rejections} append-policy cases rejected. Current project files were not generated.')


if __name__ == '__main__':
    main()
