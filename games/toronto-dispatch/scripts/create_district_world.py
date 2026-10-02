"""Compile original seam definitions and western traffic loops into native data."""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / 'project/plugins/toronto-driving/engine/include/td_district_world.h'


def source():
    world = json.loads((ROOT / 'content/districts/world.json').read_text())
    assert [d['id'] for d in world['districts']] == [0, 1, 2]
    assert all(d['width_pixels'] == 1024 and d['height_pixels'] == 976 for d in world['districts'])
    portals = []
    for pair in world['portals']:
        a, b = pair['from'], pair['to']
        assert a['district'] != b['district']
        for origin, dest in ((a, b), (b, a)):
            assert origin['u'] in (24, 1000) and dest['u'] in (24, 1000)
            assert 32 <= origin['v'] < 944 and 32 <= dest['v'] < 944
            portals.append((origin['district'], dest['district'], origin['u'], origin['v'], dest['u'], dest['v'], int('vehicle' in pair['access'])))
    out = ['/* Generated original seam/traffic placement; create_district_world.py. */',
           '#ifndef TD_DISTRICT_WORLD_H', '#define TD_DISTRICT_WORLD_H',
           'typedef struct { UBYTE from,to; UWORD u,v,arrival_u,arrival_v; UBYTE vehicle; } td_portal_t;',
           f'#define TD_PORTALS {len(portals)}', 'static const td_portal_t td_portals[TD_PORTALS]={']
    out += ['    {' + ','.join(map(str, p)) + '},' for p in portals]
    out += ['};', 'static const char td_district_names[3][19]={']
    for district in world['districts']:
        name = district['name']
        assert len(name) <= 18 and '"' not in name
        out.append(f'    "{name}",')
    out += ['};', '#define TD_TRAFFIC_POINTS 16',
            'static const UBYTE td_west_traffic_counts[2][6]={']
    paths = []
    for slug in ('west', 'high_park'):
        data = json.loads((ROOT / f'content/districts/{slug}_art.json').read_text())
        loops = data['traffic_loops']
        assert len(loops) == 6
        for loop in loops:
            assert 4 <= len(loop) <= 16
            for a, b in zip(loop, loop[1:] + loop[:1]):
                assert (a[0] == b[0]) != (a[1] == b[1]), 'Traffic segments must be nonzero and cardinal'
        paths.append(loops)
        out.append('    {' + ','.join(str(len(p)) for p in loops) + '},')
    out += ['};', 'static const UWORD td_west_traffic[2][6][TD_TRAFFIC_POINTS][2]={']
    for loops in paths:
        out.append('  {')
        for loop in loops:
            out.append('    {' + ','.join('{' + ','.join(map(str, p)) + '}' for p in loop) + '},')
        out.append('  },')
    out += ['};', '#endif', '']
    return '\n'.join(out)


if __name__ == '__main__':
    generated = source()
    if '--check' in sys.argv:
        assert HEADER.read_text() == generated, 'District native data is stale; regenerate it'
        print('Native seam and traffic data matches original district metadata')
    else:
        HEADER.write_text(generated)
        print('Compiled reciprocal district seams and original western traffic loops')
