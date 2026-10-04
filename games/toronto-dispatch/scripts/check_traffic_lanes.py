"""Independent right-hand-lane gate for all registered fictional fleet patrols.

The hand-reviewed fixture owns named road sections and exact 16px connectors;
this does not import the authoring generator's lane-validation implementation.
Every closed-loop edge is checked against original registered collision bytes,
including each role's square body and a conservative 8px body. Native traffic,
pursuit, signals, retreat, speed, timing and hardware still need their own tests.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]
FIXTURE = REPO / 'tests/fixtures/traffic_lanes.json'
HEADER = ROOT / 'project/plugins/toronto-driving/engine/include/td_district_world.h'
COUNTS = 7
checks = 0
body_samples = 0


def require(value, message):
    global checks
    checks += 1
    if not value:
        raise AssertionError(message)


def canonical_sha(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def read(path):
    return json.loads(path.read_text())


def decode_runs(text):
    """Decode actual GB Studio byte runs, independently of production generators."""
    out, at = [], 0
    while at < len(text):
        require(at + 2 < len(text), 'Truncated collision byte run')
        value = int(text[at:at + 2], 16)
        at += 2
        if text[at] == '!':
            count, at = 1, at + 1
        else:
            end = text.index('+', at)
            count, at = int(text[at:end], 16), end + 1
        require(count > 0, 'Empty collision byte run')
        out.extend([value] * count)
    return out


def direction(a, b):
    require(len(a) == len(b) == 2 and all(type(v) is int for v in a + b), 'Noninteger pixel coordinate')
    require((a[0] == b[0]) != (a[1] == b[1]), 'Noncardinal/zero traffic edge')
    return 'E' if b[0] > a[0] else 'W' if b[0] < a[0] else 'S' if b[1] > a[1] else 'N'


def axis(d):
    return 0 if d in ('E', 'W') else 1


def section(roads, reference):
    require(set(reference) == {'road', 'segment'}, 'Unexpected named-road reference')
    require(reference['road'] in roads and type(reference['segment']) is int, 'Missing named road')
    points = roads[reference['road']]
    n = reference['segment']
    require(0 <= n < len(points) - 1, 'Invalid named-road section')
    a, b = points[n:n + 2]
    d = direction(a, b)
    return axis(d), a[1 - axis(d)], sorted((a[axis(d)], b[axis(d)]))


def validate_edge(a, b, expected, roads, label):
    d = direction(a, b)
    require(d == expected['direction'], label + ': wrong travel direction')
    coordinate = axis(d)
    if expected['kind'] == 'turnaround':
        require([a, b] == expected['endpoints'], label + ': connector moved/expanded')
        require(abs(b[coordinate] - a[coordinate]) == 16, label + ': connector is not16px')
        require(bool(expected['reason']), label + ': undeclared connector')
        return
    require(expected['kind'] == 'lane' and bool(expected['road_sections']), label + ': missing principal road')
    refs = [section(roads, r) for r in expected['road_sections']]
    require(all(r[0] == coordinate for r in refs), label + ': wrong road axis')
    require(len({r[1] for r in refs}) == 1, label + ': noncollinear road ownership')
    centre = refs[0][1]
    # Right-hand in screen north-up coordinates: E south, W north, S west, N east.
    offset = 8 if d in ('E', 'N') else -8
    require(a[1 - coordinate] == b[1 - coordinate] == centre + offset,
            label + ': centreline/wrong-side lane')
    ranges = sorted(r[2] for r in refs)
    reach = ranges[0][1]
    for interval in ranges[1:]:
        require(reach >= interval[0], label + ': disconnected named-road ownership')
        reach = max(reach, interval[1])
    lo, hi = sorted((a[coordinate], b[coordinate]))
    require(ranges[0][0] - 8 <= lo <= hi <= reach + 8,
            label + ': beyond bounded8px junction join')


def body_clear(grid, u, v, half):
    # Exact whole-pixel terrain query used by native road helpers: includes
    # both boundary tiles, not just the centre or a line of samples.
    if not (8 <= u <= 1016 and 8 <= v <= 968):
        return False
    return all(grid[y * 128 + x] == 0
               for y in range((v - half) // 8, (v + half) // 8 + 1)
               for x in range((u - half) // 8, (u + half) // 8 + 1))


def on_segment(point, first, last):
    return ((first[0] == last[0] == point[0] and min(first[1], last[1]) <= point[1] <= max(first[1], last[1])) or
            (first[1] == last[1] == point[1] and min(first[0], last[0]) <= point[0] <= max(first[0], last[0])))


def loops_check(district, loops, metadata, oracle, grid, initial=None, test_metadata=True):
    global body_samples
    if district == 5:
        require(loops == [] and metadata == [], 'Islands acquired road traffic')
        return []
    require(len(loops) == len(oracle['roles']) == 6, 'Each road district must retain all six fleet roles')
    if test_metadata:
        require(len(metadata) == 6, 'Missing six-role semantic authoring metadata')
    roads = {r['name']: r['points'] for r in oracle['roads']}
    edges = []
    for role, (loop, owned) in enumerate(zip(loops, oracle['roles'])):
        require(4 <= len(loop) <= 16 and len(loop) == len(owned), f'{district}/{role}: route/ownership count')
        if test_metadata:
            require(len(metadata[role]) == len(loop), f'{district}/{role}: metadata count')
        for index, (a, b, expected) in enumerate(zip(loop, loop[1:] + loop[:1], owned)):
            label = f'district{district}/role{role}/edge{index}'
            validate_edge(a, b, expected, roads, label)
            if test_metadata:
                actual = metadata[role][index]
                require(actual.get('kind') == expected['kind'] and actual.get('direction') == expected['direction'], label + ': misleading lane metadata')
                if expected['kind'] == 'lane':
                    declared = actual.get('road_sections', [])
                    required = {(r['road'], r['segment']) for r in expected['road_sections']}
                    require(required <= {(r['road'], r['segment']) for r in declared}, label + ': missing principal road ownership metadata')
                    validate_edge(a, b, actual, roads, label + '/authored metadata')
                else:
                    require(type(actual.get('reason')) is str and bool(actual['reason'].strip()), label + ': missing connector reason')
                    # The transverse connector has to join two legitimate
                    # adjacent principal lanes, not excuse a long reversal.
                    for neighbor in ((index - 1) % len(loop), (index + 1) % len(loop)):
                        require(owned[neighbor]['kind'] == 'lane', label + ': connector adjoins another exception')
                    refs = actual.get('road_sections')
                    require(type(refs) is list and bool(refs), label + ': connector road ownership absent')
                    cap_axis = axis(expected['direction'])
                    midpoint = (a[cap_axis] + b[cap_axis]) // 2
                    spans = []
                    for reference in refs:
                        road_axis, centre, span = section(roads, reference)
                        require(road_axis != cap_axis and centre == midpoint,
                                label + ': connector named road axis/centre mismatch')
                        spans.append(span)
                    spans.sort()
                    reach = spans[0][1]
                    for span in spans[1:]:
                        require(span[0] <= reach, label + ': disconnected connector owners')
                        reach = max(reach, span[1])
                    require(spans[0][0] - 8 <= a[1 - cap_axis] <= reach + 8,
                            label + ': connector does not join its named road')

            d = direction(a, b)
            coord = axis(d)
            step = 1 if b[coord] > a[coord] else -1
            half = (5, 6, 5, 7, 6, 7)[role]
            for along in range(a[coord], b[coord] + step, step):
                u, v = (along, a[1]) if coord == 0 else (a[0], along)
                body_samples += 2
                require(body_clear(grid, u, v, half), label + ': actual role body touches nonroad tile')
                require(body_clear(grid, u, v, 8), label + ': conservative 8px body touches nonroad tile')
            edges.append((role, index, coord, a[1 - coord], min(a[coord], b[coord]), max(a[coord], b[coord]), d, expected['kind']))
        start = initial[role] if initial is not None else {'u': loop[0][0], 'v': loop[0][1], 'leg': 1}
        require(set(start) == {'u', 'v', 'leg'} and all(type(v) is int for v in start.values()), f'{district}/{role}: invalid spawn')
        leg = start['leg']
        require(0 <= leg < len(loop), f'{district}/{role}: invalid target leg')
        prev = (leg - 1) % len(loop)
        require(on_segment([start['u'], start['v']], loop[prev], loop[leg]), f'{district}/{role}: spawn not on current authored segment')
        require([start['u'], start['v']] != loop[leg], f'{district}/{role}: spawn begins with ambiguous completed leg')
        require(body_clear(grid, start['u'], start['v'], 8), f'{district}/{role}: blocked spawn')
        # Native target-leg headings must use the closing predecessor for leg0.
        heading = {'E': 0, 'S': 1, 'W': 2, 'N': 3}[direction(loop[prev], loop[leg])]
        require(0 <= role * 8 + heading * 2 < 48, f'{district}/{role}: native fleet frame range')
        for target in range(len(loop)):
            first, last = loop[(target - 1) % len(loop)], loop[target]
            d = direction(first, last)
            require(d == owned[(target - 1) % len(loop)]['direction'], f'{district}/{role}: closing/predecessor ownership')
            # Q4 fractional positions and one-step separating retreats remain
            # on this exact segment; no rounded-pixel or reversed-leg alias.
            ca = axis(d)
            low, high = sorted((first[ca] * 16, last[ca] * 16))
            for q in (low, low + 1, low + 8, high - 8, high - 1, high):
                p = [first[0] * 16, first[1] * 16]
                p[ca] = q
                require(on_segment(p, [x * 16 for x in first], [x * 16 for x in last]), f'{district}/{role}: Q4 retreat interval')
                p[1 - ca] += 1
                require(not on_segment(p, [x * 16 for x in first], [x * 16 for x in last]), f'{district}/{role}: off-segment Q4 accepted')
    opposing_check(district, edges)
    for a, first in enumerate(initial or [{'u': p[0][0], 'v': p[0][1], 'leg': 1} for p in loops]):
        for b, last in enumerate((initial or [{'u': p[0][0], 'v': p[0][1], 'leg': 1} for p in loops])[a + 1:], a + 1):
            require(abs(first['u'] - last['u']) >= 16 or abs(first['v'] - last['v']) >= 16, f'{district}: overlapping initial fleet{a}/{b}')
    return edges


def opposing_check(district, edges):
    # Opposite through-travel cannot share a line or overlap parallel bodies.
    # Short transverse turns are excluded; terrain/endpoint tests still apply.
    # Junction crossing traffic is queued by the separately tested runtime.
    opposite = {'E': 'W', 'W': 'E', 'N': 'S', 'S': 'N'}
    for i, first in enumerate(edges):
        for last in edges[i + 1:]:
            if first[7] != 'lane' or last[7] != 'lane' or first[2] != last[2] or opposite[first[6]] != last[6]:
                continue
            if max(first[4], last[4]) >= min(first[5], last[5]):
                continue
            gap = abs(first[3] - last[3])
            require(gap >= 16, f'{district}: shared/insufficient opposing lane gap roles{first[0]}/{last[0]}')


def c_array(text, name):
    match = re.search(r'\b' + re.escape(name) + r'\s*(?:\[[^]]*\]\s*)+=\s*\{', text)
    require(bool(match), 'Missing native array ' + name)
    at, depth, end = match.end() - 1, 0, None
    for index in range(at, len(text)):
        depth += (text[index] == '{') - (text[index] == '}')
        if depth == 0:
            end = index + 1
            break
    require(end is not None, 'Unclosed native array ' + name)
    payload = re.sub(r'/\*.*?\*/|//[^\n]*', '', text[at:end], flags=re.S)
    require(bool(re.fullmatch(r'[\d\s{},]+', payload)), 'Unexpected native expression ' + name)
    payload = re.sub(r',\s*}', '}', payload).replace('{', '[').replace('}', ']')
    return json.loads(payload)


def verify_native_arrays(text, models):
    for name, dimensions in (('td_traffic_counts', '[7][6]'),
                             ('td_traffic_paths', '[7][6][TD_TRAFFIC_POINTS][2]'),
                             ('td_traffic_enabled', '[7]'),
                             ('td_core_traffic_spawn_u', '[6]'),
                             ('td_core_traffic_spawn_v', '[6]'),
                             ('td_core_traffic_spawn_leg', '[6]')):
        matches = re.findall(r'\b' + name + r'\s*((?:\[[^]]*\]\s*)+)=', text)
        require(len(matches) == 1 and re.sub(r'\s+', '', matches[0]) == dimensions,
                'Native table declaration/uniqueness mismatch ' + name)
    counts = c_array(text, 'td_traffic_counts')
    paths = c_array(text, 'td_traffic_paths')
    enabled = c_array(text, 'td_traffic_enabled')
    require(len(counts) == len(paths) == len(enabled) == 7, 'Native traffic is not district-indexed0..6')
    require(enabled == [1, 1, 1, 1, 1, 0, 1], 'Native foot-only district capability changed')
    for district in range(7):
        loops = models[district]['loops'] or [[] for _ in range(6)]
        require(counts[district] == [len(p) for p in loops], f'{district}: native counts differ from source')
        require(len(paths[district]) == 6, f'{district}: native role dimension')
        for slot, loop in enumerate(loops):
            require(paths[district][slot] == (loop or [[0, 0]]), f'{district}/{slot}: native pixel path/closing edge mismatch')
    for name, key in (('td_core_traffic_spawn_u', 'u'), ('td_core_traffic_spawn_v', 'v'), ('td_core_traffic_spawn_leg', 'leg')):
        require(c_array(text, name) == [s[key] for s in models[0]['initial']], 'Native Core spawn mismatch ' + name)
    require('td_west_traffic' not in text, 'Obsolete district-minus-one table remains')


def protected_json_check(relative, value, pin, verify_fresh=True):
    value = copy.deepcopy(value)
    for key in pin.get('omit_top_keys', []):
        value.pop(key, None)
    for key in pin.get('omit_geography_keys', []):
        value['geography_metadata'].pop(key)
    sources = pin.get('geography_sha256_sources')
    if sources is not None:
        rows = value['geography_metadata']
        require(type(rows) is list and len(rows) == len(sources),
                'Job provenance row count changed: ' + relative)
        for row, source in zip(rows, sources):
            require(row['path'] == source, 'Job provenance source changed: ' + relative)
            if verify_fresh:
                # These generators hash canonical metadata JSON, not file bytes.
                # Omit only this hash leaf; keep paths, notices and every other
                # job/stop/source field pinned to the authenticated predecessor.
                require(row['sha256'] == canonical_sha(read(ROOT / source)),
                        'Job geography provenance stale: ' + relative)
            row.pop('sha256')
    require(canonical_sha(value) == pin['sha256'],
            'Nontraffic district/client content changed: ' + relative)


def protected_check(fixture):
    for relative, expected in fixture['protected_files'].items():
        require(hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() == expected, 'Protected asset/campaign/world changed: ' + relative)
    for relative, pin in fixture['protected_json'].items():
        protected_json_check(relative, read(ROOT / relative), pin)
    north = read(ROOT / 'content/districts/north_art.json')
    jobs = read(ROOT / 'content/districts/north_jobs.json')
    geo = jobs['geography_metadata']
    require(geo['source_layout_sha256'] == north['source_layout_sha256'] == hashlib.sha256((ROOT / 'content/districts/north_layout.json').read_bytes()).hexdigest(), 'North layout provenance not refreshed exactly')
    fields = ('dimensions', 'roads', 'footpaths', 'rails', 'rail_crossings',
              'landmarks', 'parks', 'water', 'blocked_ravines', 'private_ground',
              'forced_foot_masks', 'closed_frontiers', 'ports', 'core_throat_proposal',
              'stop_candidates', 'traffic_loops', 'road_half_width', 'walk_half_width',
              'footpath_half_width', 'rail_half_width')
    require(geo['geometry_sha256'] == canonical_sha({k: north[k] for k in fields}),
            'North geometry provenance not refreshed exactly')


def load_models(fixture):
    world = read(ROOT / 'content/districts/world.json')
    require([d['id'] for d in world['districts']] == list(range(7)), 'Registered district IDs changed')
    models = []
    for oracle, district in zip(fixture['districts'], world['districts']):
        require(district['id'] == oracle['id'] and district['scene'] == oracle['scene'], 'Original scene identity changed')
        art = read(ROOT / oracle['art_source'])
        traffic = read(ROOT / 'content/core_traffic.json') if not district['id'] else art
        require(traffic.get('roads') == oracle['roads'], f'{district["id"]}: named-road geometry changed')
        scene = read(ROOT / 'project/project/scenes' / district['scene'] / 'scene.gbsres')
        require((scene['width'], scene['height']) == (128, 122), 'Unexpected native collision dimensions')
        grid = decode_runs(scene['collisions'])
        require(len(grid) == 128 * 122 and set(grid) <= {0, 15, 16}, 'Malformed registered collision grid')
        initial = traffic.get('traffic_initial') if district['id'] == 0 else None
        if district['id'] == 0:
            require(initial == [{'u': 80, 'v': 280, 'leg': 1}, {'u': 200, 'v': 392, 'leg': 1}, {'u': 320, 'v': 168, 'leg': 1}, {'u': 440, 'v': 632, 'leg': 1}, {'u': 824, 'v': 240, 'leg': 1}, {'u': 216, 'v': 72, 'leg': 0}], 'Core reviewed spawn changed')
        models.append({'loops': traffic['traffic_loops'], 'metadata': traffic.get('traffic_lane_segments', []), 'grid': grid, 'initial': initial})
    return models


def check_models(fixture, models):
    for oracle, model in zip(fixture['districts'], models):
        loops_check(oracle['id'], model['loops'], model['metadata'], oracle, model['grid'], model['initial'])


def must_reject(action, label):
    global checks
    try:
        action()
    except (AssertionError, ValueError, KeyError, IndexError, TypeError):
        checks += 1
        return
    raise AssertionError('Negative fixture unexpectedly admitted: ' + label)


def self_test(fixture, models, native):
    for relative, pin in fixture['protected_json'].items():
        if 'geography_sha256_sources' not in pin:
            continue
        original = read(ROOT / relative)
        def bad_content(change):
            value = copy.deepcopy(original)
            change(value)
            protected_json_check(relative, value, pin)
        for index in range(len(pin['geography_sha256_sources'])):
            must_reject(lambda index=index: bad_content(lambda value: value['geography_metadata'][index].update(sha256='0' * 64)),
                        'Stale job geography hash ' + relative + '/' + str(index))
        must_reject(lambda: bad_content(lambda value: value['geography_metadata'][0].update(path='content/city_art.json')),
                    'Job geography path substituted ' + relative)
        must_reject(lambda: bad_content(lambda value: value['geography_metadata'][0].update(notice='')),
                    'Job geography attribution removed ' + relative)
        must_reject(lambda: bad_content(lambda value: value['geography_metadata'].pop()),
                    'Job geography row removed ' + relative)
        must_reject(lambda: bad_content(lambda value: value.update(world_sha256='0' * 64)),
                    'Job source field altered ' + relative)
        for field, key in (('stops', 'u'), ('quests', 'reward'), ('quests', 'time_limit_seconds')):
            require(bool(original[field]), 'Missing job content mutation target ' + relative)
            must_reject(lambda field=field, key=key: bad_content(lambda value: value[field][0].update({key: value[field][0][key] + 1})),
                        'Job semantic content altered ' + relative + '/' + field + '/' + key)
    # Direct independent cardinal oracle: corrupt a principal coordinate in
    # every direction, without relying on a generator's expected route array.
    seen = set()
    for oracle, model in zip(fixture['districts'], models):
        for slot, owned in enumerate(oracle['roles']):
            loop = model['loops'][slot]
            roads = {r['name']: r['points'] for r in oracle['roads']}
            for index, expected in enumerate(owned):
                a, b = loop[index], loop[(index + 1) % len(loop)]
                if expected['kind'] != 'lane':
                    far = list(b); far[axis(expected['direction'])] += 1
                    must_reject(lambda: validate_edge(a, far, expected, roads, 'expanded endcap'), '17px connector')
                    continue
                d = expected['direction'];seen.add(d);coordinate = axis(d);offset = 8 if d in ('E', 'N') else -8
                for delta in (-offset, -2 * offset):
                    aa, bb = list(a), list(b);aa[1 - coordinate] += delta;bb[1 - coordinate] += delta
                    must_reject(lambda: validate_edge(aa, bb, expected, roads, 'bad lane'), 'centreline/wrong-side ' + d)
                must_reject(lambda: validate_edge(b, a, expected, roads, 'reversed lane'), 'principal reversal ' + d)
    require(seen == {'E', 'S', 'W', 'N'}, 'Cardinal mutation coverage missing')
    def bad_model(district, update):
        corrupt = copy.deepcopy(models);update(corrupt[district]);check_models(fixture, corrupt)
    must_reject(lambda: bad_model(2, lambda m: m['loops'][0].extend([[952, 184], [952, 168]])), 'HighPark naive centreline jog/retrace')
    must_reject(lambda: bad_model(2, lambda m: m['loops'][1].insert(3, [936, 184])), 'HighPark opposing collinear collapse')
    must_reject(lambda: opposing_check(2, [(0, 1, 1, 952, 104, 184, 'S', 'lane'),
                                          (0, 2, 1, 952, 168, 184, 'N', 'lane')]),
                'HighPark Keele/Parkside same-line reversal')
    must_reject(lambda: opposing_check(2, [(1, 2, 0, 184, 840, 936, 'E', 'lane'),
                                          (1, 3, 0, 184, 440, 936, 'W', 'lane')]),
                'HighPark Annette same-line collapse')

    must_reject(lambda: bad_model(0, lambda m: m['initial'][0].update(leg=0)), 'Core wrong predecessor/target spawn')
    must_reject(lambda: bad_model(0, lambda m: m['initial'][0].update(u=80, v=288)), 'Core centreline spawn')
    must_reject(lambda: bad_model(5, lambda m: m.update(loops=[[[100, 100], [200, 100], [200, 200], [100, 200]]])), 'Islands road traffic')
    must_reject(lambda: bad_model(3, lambda m: m['metadata'][0][0]['road_sections'][0].update(road='Queen Street East')), 'Forged lane ownership')
    require(c_array(native, 'td_traffic_counts')[5] == [0] * 6, 'Zero native Island row')
    must_reject(lambda: verify_native_arrays(native.replace('td_traffic_counts', 'td_west_traffic_counts'), models), 'Old district-minus-one native layout')
    must_reject(lambda: verify_native_arrays(native.replace('td_core_traffic_spawn_leg', 'td_core_wrong_spawn_leg'), models), 'Missing native custom Core spawn')
    must_reject(lambda: verify_native_arrays(native.replace('td_traffic_counts[7][6]', 'td_traffic_counts[8][6]'), models), 'Oversized implicit zero district row')
    bad_count = re.sub(r'(td_traffic_counts\[7\]\[6\]=\{\s*\{)4', r'\g<1>0', native, count=1)
    require(bad_count != native, 'Native count mutation did not apply')
    must_reject(lambda: verify_native_arrays(bad_count, models), 'Compiled Core target-count corruption')
    bad_path = re.sub(r'(td_traffic_paths\[7\]\[6\]\[TD_TRAFFIC_POINTS\]\[2\]=\{\s*\{\s*\{\{)840', r'\g<1>841', native, count=1)
    require(bad_path != native, 'Native point mutation did not apply')
    must_reject(lambda: verify_native_arrays(bad_path, models), 'Compiled Core target-coordinate corruption')
    must_reject(lambda: bad_model(0, lambda m: m['metadata'][0][1].update(road_sections=[])),
                'Unowned transverse connector')

    # Independent opposing-body threshold checks: inclusive terrain extents
    # and strict runtime overlap allow16px separation of two half8 bodies.
    first = (0, 0, 0, 100, 20, 120, 'E', 'lane')
    for gap in (0, 15):
        second = (1, 0, 0, 100 + gap, 40, 80, 'W', 'lane')
        must_reject(lambda: opposing_check(0, [first, second]), 'Opposing hull gap ' + str(gap))
    opposing_check(0, [first, (1, 0, 0, 116, 40, 80, 'W', 'lane')])
    for malformed in ('00', '000+', '00!0'):
        must_reject(lambda: decode_runs(malformed), 'Malformed native collision run')
    # A solid tile only in a long leg's middle must fail the continuous union,
    # even though both ends/spawn remain safe and named-road ownership agrees.
    def solid_middle(model):
        model['grid'][35 * 128 + 50] = 15  # Core College lane at400/280.
    must_reject(lambda: bad_model(0, solid_middle), 'Solid obstacle in swept middle')



def baseline(fixture, commit=None):
    """Deliberately failing predecessor; immutable actual Git source optional."""
    if commit is not None:
        require(commit == fixture['baseline_commit'], 'Unexpected historical traffic baseline')
        for relative, sha in fixture['baseline']['source_files'].items():
            payload = subprocess.check_output(['git', 'show', commit + ':games/toronto-dispatch/' + relative], cwd=REPO)
            require(hashlib.sha256(payload).hexdigest() == sha, 'Historical native source pin mismatch')
        for relative, pin in fixture['protected_json'].items():
            if 'original_source_sha256' not in pin:
                continue
            payload = subprocess.check_output(['git', 'show', commit + ':games/toronto-dispatch/' + relative], cwd=REPO)
            require(hashlib.sha256(payload).hexdigest() == pin['original_source_sha256'],
                    'Historical job source pin mismatch: ' + relative)
            protected_json_check(relative, json.loads(payload), pin, verify_fresh=False)
        for district in fixture['baseline']['loops'][1:]:
            oracle = fixture['districts'][district['district']]
            payload = subprocess.check_output(['git', 'show', commit + ':games/toronto-dispatch/' + oracle['art_source']], cwd=REPO)
            require(json.loads(payload)['traffic_loops'] == district['loops'], 'Historical source loop snapshot mismatch')
    # Transcribe the exact pinned b766 Core formulas by leg index, rather than
    # mistakenly treating coldspawn as point0 or reversing the vertical shuttle.
    expected_core = []
    for row in (288, 400, 176, 640):
        expected_core.append([[840 if leg < 2 else 48,
                               row + (-8 if leg in (0, 3) else 8)] for leg in range(4)])
    expected_core.append([[824 if leg in (0, 3) else 808, 792 if leg < 2 else 48] for leg in range(4)])
    expected_core.append([[640, 72], [216, 72], [216, 168], [640, 168], [808, 168], [808, 72]])
    require(fixture['baseline']['loops'][0]['loops'] == expected_core, 'Historical Core target formula transcription changed')
    bad, good = [], []
    # Generic historical diagnosis uses all immutable named-road sections. A
    #16px transverse edge is a candidate connector; each longer through edge
    # has to match an actual right-hand lane of the corresponding road axis.
    for district in fixture['baseline']['loops']:
        i = district['district']; oracle = fixture['districts'][i]
        roads = {r['name']: r['points'] for r in oracle['roads']}
        for slot, loop in enumerate(district['loops']):
            errors = []
            for a, b in zip(loop, loop[1:] + loop[:1]):
                d = direction(a, b);coordinate = axis(d)
                if abs(b[coordinate] - a[coordinate]) <= 16:
                    continue
                admitted = False
                for name, pts in roads.items():
                    for n in range(len(pts) - 1):
                        try:
                            validate_edge(a, b, {'kind': 'lane', 'direction': d, 'road_sections': [{'road': name, 'segment': n}]}, roads, 'baseline')
                            admitted = True;break
                        except AssertionError:
                            pass
                    if admitted:break
                if not admitted:errors.append((a, b, d))
            (bad if errors else good).append([i, slot])
    require(len(bad) >= 30, 'Historical malformed-lane baseline was not rejected')
    print(f'Historical baseline rejected {len(bad)}/36 road patrols; {len(good)} already legal: {good}')
    return bad


def validate_lanes(data):
    """Fail-closed authoring API retained for create_district_world.py.

    This checks its passed source data against the separate semantic fixture,
    not merely the source's own lane declarations. Native collision resources
    stay authoritative even while generating updated traffic metadata.
    """
    district = data.get('district', data.get('id'))
    require(type(district) is int and 0 <= district < 7, 'Unregistered traffic metadata district')
    oracle = read(FIXTURE)['districts'][district]
    require(data.get('roads') == oracle['roads'], 'Authored named-road geometry differs from independent fixture')
    scene = read(ROOT / 'project/project/scenes' / oracle['scene'] / 'scene.gbsres')
    require((scene['width'], scene['height']) == (128, 122), 'Authored traffic native grid dimensions')
    grid = decode_runs(scene['collisions'])
    require(len(grid) == 128 * 122, 'Authored traffic native grid size')
    before = body_samples
    edges = loops_check(district, data.get('traffic_loops'), data.get('traffic_lane_segments', []),
                        oracle, grid, data.get('traffic_initial') if district == 0 else None)
    return {'edges': len(edges), 'body_samples': body_samples - before}


def check(run_mutations=True):
    """Public source gate also used by check_district_world.py; never writes."""
    fixture = read(FIXTURE)
    require(fixture['schema_version'] == 1 and len(fixture['districts']) == 7, 'Invalid independent fixture')
    require(fixture['actual_square_half_pixels'] == [5, 6, 5, 7, 6, 7] and
            fixture['conservative_half_pixels'] == fixture['right_hand_offset_pixels'] ==
            fixture['junction_extension_pixels'] == 8, 'Independent body/lane bounds changed')
    protected_check(fixture)
    models = load_models(fixture)
    initial_bodies = body_samples
    check_models(fixture, models)
    source_body_samples = body_samples - initial_bodies
    native = HEADER.read_text()
    verify_native_arrays(native, models)
    if run_mutations:
        self_test(fixture, models, native)
    return {'checks': checks, 'patrols': 36, 'edges': sum(len(p) for m in models for p in m['loops']),
            'protected_files': len(fixture['protected_files']), 'body_samples': source_body_samples,
            'source_scope_only': True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--self-test', action='store_true', help='explicitly request negative fixtures (also run by default)')
    parser.add_argument('--baseline', action='store_true', help='prove immutable predecessor is rejected; successful negative test exits0')
    parser.add_argument('--baseline-commit', help='also authenticate exact predecessor Git source before rejection')
    args = parser.parse_args()
    if args.baseline or args.baseline_commit:
        baseline(read(FIXTURE), args.baseline_commit)
    else:
        result = check()
        print(f"All {result['patrols']} patrol roles/{result['edges']} closed edges/{result['body_samples']} body samples: named right-hand lanes, exact connectors, whole-body terrain, spawn/headings, native data and original-content pins pass")
    print(f'Traffic lane gate: {checks:,} checks, 0 failures. Source/geometry evidence; no native performance or hardware claim.')


if __name__ == '__main__':
    try:
        main()
    except (AssertionError, ValueError, KeyError, IndexError, TypeError) as exc:
        print('Traffic lane gate FAILED: ' + str(exc), file=sys.stderr)
        sys.exit(1)
