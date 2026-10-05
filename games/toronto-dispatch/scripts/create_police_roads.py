"""ROM-only right-hand police lanes on registered, compressed named roads.

The source graph admits only original road intervals and explicitly bounded
junction/endcap connectors. Every edge is swept at the native half5 body.
Two-bit return choices are shortest-path policies to the actual role2 patrol
vertices, not a runtime search or position correction. No signal is invented.
"""
import heapq
import json
import sys
from pathlib import Path

from check_campaign import decode
from city_layout import COLS, ROWS
from create_traffic_signals import intervals
from district_sources import read_district_art

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / 'project/plugins/toronto-driving/engine/include/td_police_road_data.h'


def road_source(district):
    if district['id'] == 0:
        names_h = ('BLOOR STREET', 'WELLESLEY / HARBORD', 'COLLEGE / CARLTON',
                   'DUNDAS STREET', 'QUEEN STREET', 'KING STREET', 'FRONT STREET', 'QUEENS QUAY')
        names_v = ('DUFFERIN STREET', 'BATHURST STREET', 'SPADINA AVENUE', 'UNIVERSITY AVENUE',
                   'BAY STREET', 'YONGE STREET', 'JARVIS STREET', 'PARLIAMENT STREET', 'BROADVIEW AVENUE')
        roads = [{'name': name, 'points': [[0 if y in (64, 288, 400, 528, 640) else 24, y],
                     [848 if y in (640, 720) else 1024 if y in (64, 400, 528) else 992, y]]}
                 for name, y in zip(names_h, ROWS)]
        # Match the current Core street-name adapter's east-bank labels.
        roads[0]['points'][1][0] = 912
        roads[2]['points'][1][0] = 816
        roads += [{'name': 'DANFORTH AVENUE', 'points': [[912, 64], [1024, 64]]},
                  {'name': 'GERRARD ST EAST', 'points': [[816, 288], [992, 288]]}]
        roads += [{'name': name, 'points': [[x, 24], [x, 816]]} for name, x in zip(names_v, COLS)]
        patrol = json.loads((ROOT / 'content/core_traffic.json').read_text())['traffic_loops'][2]
    else:
        art = read_district_art(district)
        roads = art['roads']
        patrol = art['traffic_loops'][2] if district.get('traffic_enabled', True) else []
    return roads, [tuple(point) for point in patrol]


def district_model(district):
    if not district.get('traffic_enabled', True):
        return {'vertices': [], 'third': [], 'goals': [], 'edges': [], 'nodes': [], 'lanes': []}
    roads, patrol = road_source(district)
    assert 1 <= len(patrol) <= 8, 'Return policies have two bits per authored patrol target'
    scene = json.loads((ROOT / 'project/project/scenes' / district['scene'] / 'scene.gbsres').read_text())
    assert (scene['width'], scene['height']) == (128, 122)
    grid = decode(scene['collisions'])

    def body(point):
        u, v = point
        return 8 <= u <= 1016 and 8 <= v <= 968 and all(
            grid[y * 128 + x] == 0 for y in range((v - 5) // 8, (v + 5) // 8 + 1)
            for x in range((u - 5) // 8, (u + 5) // 8 + 1))

    def sweep(a, b):
        assert (a[0] == b[0]) != (a[1] == b[1]), (a, b)
        direction = ((b[0] > a[0]) - (b[0] < a[0]), (b[1] > a[1]) - (b[1] < a[1]))
        return all(body((a[0] + direction[0] * step, a[1] + direction[1] * step))
                   for step in range(abs(a[0] - b[0]) + abs(a[1] - b[1]) + 1))

    named_segments = [(road['name'], number, a, b) for road in roads
                      for number, (a, b) in enumerate(zip(road['points'], road['points'][1:]))]
    segments = [(a, b) for _, _, a, b in named_segments]
    assert all((a[0] == b[0]) != (a[1] == b[1]) for a, b in segments)
    hs, vs = intervals(segments, True), intervals(segments, False)
    nodes = {(x, y) for y, left, right in hs for x, top, bottom in vs
             if left <= x <= right and top <= y <= bottom}
    lanes = []
    for horizontal, merged in ((True, hs), (False, vs)):
        for lateral, low, high in merged:
            crossings = [p for p in nodes if (p[1] if horizontal else p[0]) == lateral
                         and low <= (p[0] if horizontal else p[1]) <= high]
            # A bend's square connector extends only 8px past its named centre,
            # within the existing intersection artwork and verified road body.
            start = low - 8 if any((p[0] if horizontal else p[1]) == low for p in crossings) else low
            end = high + 8 if any((p[0] if horizontal else p[1]) == high for p in crossings) else high
            start = max(8, ((start + 7) // 8) * 8)
            end = min(1016 if horizontal else 968, (end // 8) * 8)

            def clear(along):
                return all(body((along, lateral + offset) if horizontal else (lateral + offset, along))
                           for offset in (-8, 8))

            parts, run = [], None
            for along in range(start, end + 1, 8):
                if clear(along) and (run is None or all(clear(p) for p in range(along - 7, along))):
                    if run is None:
                        run = along
                else:
                    if run is not None and along - 8 > run:
                        parts.append((run, along - 8))
                    run = along if clear(along) else None
            if run is not None and end > run:
                parts.append((run, end))
            for lo, hi in parts:
                for offset in (-8, 8):
                    direction = (0 if offset == 8 else 2) if horizontal else (3 if offset == 8 else 1)
                    lanes.append((horizontal, lateral + offset, lo, hi, direction))
    graph, provenance = {}, {}

    def add(a, b, owner):
        if a == b:
            return
        assert sweep(a, b), (district['scene'], a, b)
        graph.setdefault(a, set()).add(b)
        graph.setdefault(b, set())
        provenance.setdefault((a, b), set()).add(owner)

    def point(horizontal, lateral, along):
        return (along, lateral) if horizontal else (lateral, along)

    for horizontal, lateral, lo, hi, direction in lanes:
        stops = {lo, hi}
        for x, y in nodes:
            centre = y if horizontal else x
            if abs(lateral - centre) != 8:
                continue
            along = x if horizontal else y
            stops.update(p for p in (along - 8, along + 8) if lo <= p <= hi)
        for x, y in patrol:
            if (y if horizontal else x) == lateral and lo <= (x if horizontal else y) <= hi:
                stops.add(x if horizontal else y)
        order = sorted(stops, reverse=direction >= 2)
        for a, b in zip(order, order[1:]):
            owners = [(name, number) for name, number, p, q in named_segments
                      if (p[1] == q[1]) == horizontal
                      and abs(lateral - (p[1] if horizontal else p[0])) == 8
                      and max(min(p[0], q[0]) if horizontal else min(p[1], q[1]), min(a, b))
                          <= min(max(p[0], q[0]) if horizontal else max(p[1], q[1]), max(a, b))]
            assert owners, (district['scene'], horizontal, lateral, a, b)
            for name, number in owners:
                add(point(horizontal, lateral, a), point(horizontal, lateral, b), ('lane', direction, name, number))
    for x, y in nodes:
        corners = [(x - 8, y + 8), (x + 8, y + 8), (x + 8, y - 8), (x - 8, y - 8)]
        for a, b in zip(corners, corners[1:] + corners[:1]):
            if a in graph and b in graph and sweep(a, b):
                add(a, b, ('junction', x, y))
    for horizontal, lateral, lo, hi, direction in lanes:
        along = hi if direction < 2 else lo
        a = point(horizontal, lateral, along)
        other = lateral + (16 if direction in (1, 2) else -16)
        b = point(horizontal, other, along)
        if a in graph and b in graph and sweep(a, b):
            add(a, b, ('endcap', direction))
    for a, b in zip(patrol, patrol[1:] + patrol[:1]):
        if abs(a[0] - b[0]) + abs(a[1] - b[1]) == 16 and a in graph and b in graph and sweep(a, b):
            add(a, b, ('patrol_endcap',))
    assert all(point in graph for point in patrol), (district['scene'], patrol)
    # Split overlapping compressed junction edges at every actual vertex. This
    # makes an interior Q4 position's directed continuation unambiguous.
    normalized, owners = {p: set() for p in graph}, {}
    for (a, b), owner in provenance.items():
        candidates = [p for p in graph if (a[0] == b[0] == p[0] and min(a[1], b[1]) <= p[1] <= max(a[1], b[1]))
                      or (a[1] == b[1] == p[1] and min(a[0], b[0]) <= p[0] <= max(a[0], b[0]))]
        candidates.sort(key=lambda p: abs(p[0] - a[0]) + abs(p[1] - a[1]))
        for p, q in zip(candidates, candidates[1:]):
            normalized[p].add(q)
            owners.setdefault((p, q), set()).update(owner)
    graph = normalized
    reverse = {p: [] for p in graph}
    for p, exits in graph.items():
        for q in exits:
            reverse[q].append(p)
    distances = []
    for goal in patrol:
        costs, queue = {goal: 0}, [(0, goal)]
        while queue:
            cost, q = heapq.heappop(queue)
            if cost != costs[q]:
                continue
            for p in reverse[q]:
                candidate = cost + abs(p[0] - q[0]) + abs(p[1] - q[1])
                if candidate < costs.get(p, 65535):
                    costs[p] = candidate
                    heapq.heappush(queue, (candidate, p))
        distances.append(costs)
    supported = set.intersection(*(set(costs) for costs in distances))
    assert set(patrol) <= supported
    # A pursuit must not enter an isolated road fragment with no legal return.
    graph = {p: sorted(q for q in graph[p] if q in supported) for p in sorted(supported, key=lambda p: (p[1], p[0]))}
    assert max(map(len, graph.values())) <= 3
    indices = {p: index for index, p in enumerate(graph)}
    vertices, third, edges = [], [], []
    for p, exits in graph.items():
        assert p[0] % 8 == p[1] % 8 == 0
        choices = 0
        for goal_index, costs in enumerate(distances):
            if p == patrol[goal_index]:
                continue
            best = min(range(len(exits)), key=lambda n: abs(p[0] - exits[n][0]) + abs(p[1] - exits[n][1]) + costs[exits[n]])
            assert costs[exits[best]] < costs[p], 'Every return step strictly decreases the ROM potential'
            choices |= (best + 1) << (goal_index * 2)
        dest = [indices[q] for q in exits]
        if len(dest) == 3:
            third.append((indices[p], dest[2]))
        vertices.append((p[0] // 8, p[1] // 8, dest[0] if dest else 65535,
                         (dest[1] | (32768 if len(dest) == 3 else 0)) if len(dest) > 1 else 65535, choices))
        edges.extend((p, q, sorted(owners[(p, q)])) for q in exits)
    assert len(vertices) < 32768
    return {'vertices': vertices, 'third': third, 'goals': patrol, 'edges': edges,
            'nodes': sorted(nodes), 'lanes': lanes}


def model():
    rows = json.loads((ROOT / 'content/districts/world.json').read_text())['districts']
    assert [row['id'] for row in rows] == list(range(len(rows)))
    return [district_model(row) for row in rows]


def source(data):
    offsets, thirds, goals = [0], [0], [0]
    for district in data:
        offsets.append(offsets[-1] + len(district['vertices']))
        thirds.append(thirds[-1] + len(district['third']))
        goals.append(goals[-1] + len(district['goals']))
    lines = ['/* Generated by create_police_roads.py; native half5 swept right-hand lanes. */',
             '#ifndef TD_POLICE_ROAD_DATA_H', '#define TD_POLICE_ROAD_DATA_H',
             f'#define TD_POLICE_ROAD_DISTRICTS {len(data)}',
             f'#define TD_POLICE_ROAD_VERTICES {offsets[-1]}',
             f'#define TD_POLICE_ROAD_DATA_BYTES {offsets[-1]*8+thirds[-1]*4+goals[-1]*4+(len(data)+1)*6}',
             'static const UWORD td_police_offsets[]={' + ','.join(map(str, offsets)) + '};',
             'static const UWORD td_police_third_offsets[]={' + ','.join(map(str, thirds)) + '};',
             'static const UWORD td_police_goal_offsets[]={' + ','.join(map(str, goals)) + '};',
             'static const td_police_vertex_t td_police_vertices[]={']
    for district in data:
        lines += ['  {' + ','.join(map(str, row)) + '},' for row in district['vertices']]
    lines += ['};', 'static const UWORD td_police_thirds[][2]={']
    for district in data:
        lines += ['  {' + ','.join(map(str, row)) + '},' for row in district['third']]
    if not thirds[-1]:
        lines.append('  {65535,65535},')
    lines += ['};', 'static const UWORD td_police_goals[][2]={']
    for district in data:
        lines += ['  {' + ','.join(map(str, row)) + '},' for row in district['goals']]
    return '\n'.join(lines + ['};', '#endif', ''])


if __name__ == '__main__':
    data = model()
    generated = source(data)
    if '--check' in sys.argv:
        assert HEADER.read_text() == generated, 'Police road graph is stale'
    else:
        HEADER.write_text(generated)
    print('Police ROM lanes: ' + ', '.join(str(len(row['vertices'])) for row in data) +
          ' vertices; all return policies strictly decrease')
