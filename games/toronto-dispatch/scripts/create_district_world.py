"""Compile original seams and district-indexed right-hand patrols into native data."""
import json
import re
import sys
from pathlib import Path
from district_sources import read_district_art
from check_traffic_lanes import validate_lanes

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / 'project/plugins/toronto-driving/engine/include/td_district_world.h'


def source():
    world = json.loads((ROOT / 'content/districts/world.json').read_text())
    include = HEADER.parent
    canonical = int(re.search(r'#define TD_DISTRICT_COUNT (\d+)', (include / 'td_district.h').read_text()).group(1))
    assert len(world['districts']) == canonical and 2 <= canonical <= 32
    assert [d['id'] for d in world['districts']] == list(range(canonical))
    assert all(d['width_pixels'] == 1024 and d['height_pixels'] == 976 for d in world['districts'])
    enabled = []
    for district in world['districts']:
        flag = district.get('traffic_enabled', True)
        assert type(flag) is bool, 'Traffic capability must be explicit boolean'
        enabled.append(flag)
    assert enabled[0], 'Core authored traffic must remain enabled'

    def endpoint(point):
        assert all(type(point[key]) is int for key in ('district', 'u', 'v'))
        assert point['district'] in range(canonical)
        u, v = point['u'], point['v']
        if u in (24, 1000):
            assert 32 <= v < 944
            return 'horizontal'
        assert v in (24, 952) and 32 <= u < 992
        return 'vertical'

    portals = []
    for pair in world['portals']:
        a, b = pair['from'], pair['to']
        assert a['district'] != b['district']
        axis = endpoint(a)
        assert endpoint(b) == axis
        assert enabled[a['district']] and enabled[b['district']], 'Foot-only ferry districts cannot acquire ordinary road seams'
        assert (b['u'] == 1024 - a['u']) if axis == 'horizontal' else (b['v'] == 976 - a['v'])
        assert 'foot' in pair['access'] and set(pair['access']) <= {'foot', 'vehicle'}
        for origin, dest in ((a, b), (b, a)):
            portals.append((origin['district'], dest['district'], origin['u'], origin['v'], dest['u'], dest['v'], int('vehicle' in pair['access'])))
    assert len(portals) <= 512 and len(set(portals)) == len(portals)
    out = ['/* Generated original seam/traffic placement; create_district_world.py. */',
           '#ifndef TD_DISTRICT_WORLD_H', '#define TD_DISTRICT_WORLD_H',
           '#include "td_world.h"', f'#define TD_WORLD_GENERATED_DISTRICTS {canonical}',
           f'#define TD_PORTALS {len(portals)}', '#ifdef TD_WORLD_DATA',
           'static const td_portal_t td_portals[TD_PORTALS]={']
    out += ['    {' + ','.join(map(str, p)) + '},' for p in portals]
    out += ['};', f'static const char td_district_names[{canonical}][19]={{']
    for district in world['districts']:
        name = district['name']
        assert len(name) <= 18 and all(32 <= ord(c) <= 126 for c in name) and '"' not in name and '\\' not in name
        out.append(f'    "{name}",')
    out += ['};', f'static const UBYTE td_traffic_counts[{canonical}][6]={{']
    paths = []
    core = json.loads((ROOT / 'content/core_traffic.json').read_text())
    assert core['schema_version'] == 1 and core['district'] == 0
    for district in world['districts']:
        assert district['scene'].startswith('toronto_')
        slug = district['scene'].removeprefix('toronto_')
        data = core if district['id'] == 0 else read_district_art(district)
        loops = data['traffic_loops']
        if not enabled[district['id']]:
            assert loops == [] and data['roads'] == [] and data['ports'] == [], 'Foot-only district cannot contain ordinary road traffic or seams'
            paths.append([[] for _ in range(6)])
            out.append('    {0,0,0,0,0,0},')
            continue
        assert len(loops) == 6
        validate_lanes(data)
        for loop in loops:
            assert 4 <= len(loop) <= 16
            for a, b in zip(loop, loop[1:] + loop[:1]):
                assert (a[0] == b[0]) != (a[1] == b[1]), 'Traffic segments must be nonzero and cardinal'
                assert all(type(value) is int for value in a + b)
                assert 0 <= a[0] < 1024 and 0 <= a[1] < 976
        paths.append(loops)
        out.append('    {' + ','.join(str(len(p)) for p in loops) + '},')
    spawn = core['traffic_initial']
    assert len(spawn) == 6
    for role, pose in enumerate(spawn):
        assert set(pose) == {'u', 'v', 'leg'} and all(type(value) is int for value in pose.values())
        loop = paths[0][role]
        assert 0 <= pose['leg'] < len(loop)
        a, b = loop[pose['leg'] - 1], loop[pose['leg']]
        u, v = pose['u'], pose['v']
        assert ((a[0] == u == b[0] and min(a[1], b[1]) <= v <= max(a[1], b[1])) or
                (a[1] == v == b[1] and min(a[0], b[0]) <= u <= max(a[0], b[0]))), 'Core spawn must lie on its actual target segment'
    # Eleven bytes/portal,65 bytes/slot and30 Core spawn bytes; leave4KiB for code.
    metadata_bytes = canonical * 20 + len(portals) * 11 + canonical * 6 * 65 + 30
    assert metadata_bytes <= 12288, 'World metadata needs another ROM bank; do not exhaust banked code space'
    out += ['};', f'static const UBYTE td_traffic_enabled[{canonical}]={{' + ','.join(str(int(flag)) for flag in enabled) + '};',
            f'static const UWORD td_traffic_paths[{canonical}][6][TD_TRAFFIC_POINTS][2]={{']
    for loops in paths:
        out.append('  {')
        for loop in loops:
            out.append('    {' + (','.join('{' + ','.join(map(str, p)) + '}' for p in loop) if loop else '{0,0}') + '},')
        out.append('  },')
    out.append('};')
    for name, ctype, key in (('u', 'UWORD', 'u'), ('v', 'UWORD', 'v'), ('leg', 'UBYTE', 'leg')):
        out.append(f'static const {ctype} td_core_traffic_spawn_{name}[6]={{' + ','.join(str(p[key]) for p in spawn) + '};')
    out += ['#endif /* TD_WORLD_DATA */', '#endif', '']
    return '\n'.join(out)


if __name__ == '__main__':
    generated = source()
    if '--check' in sys.argv:
        assert HEADER.read_text() == generated, 'District native data is stale; regenerate it'
        print('Native seam and traffic data matches original district metadata')
    else:
        HEADER.write_text(generated)
        print('Compiled reciprocal seams and all district-indexed right-hand patrols')
