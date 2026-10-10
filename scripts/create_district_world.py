"""Compile the sixteen-scene world into native data (td_district_world.h)
and content/districts/world.json.

Inputs: the 1x plan of district seams (content/districts/world_plan.json),
the double-scale art metadata and each scene's collision grid. Outputs:
- portals: the plan's seams between districts, mapped onto their scenes,
  plus the inner seams between a district's four scenes (one per road or
  path crossing, for route planning; the engine crosses an inner seam
  anywhere it is open);
- a next-hop table for route planning, on foot and by vehicle;
- six right-hand traffic loops per scene (one block each, in lane), with
  the traffic-signal junctions they stop at;
- the start position, the hospital and each scene's spray bay.
"""
import json
import re
import sys
from collections import deque
from pathlib import Path
import world2x as W

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / 'project/plugins/toronto-driving/engine/include/td_district_world.h'
COUNT = 16
LOOPS = 6
LANE = 10           # lane centre offset from the street centreline (px)
CAR = 7             # half the width of a car across its lane (px)
SIGNALS = 4         # signal junctions per scene
# New game: Union Station's forecourt in the core (plan position 576, 740).
START = W.map_point(0, 576, 740)


def plan_world():
    return json.loads((ROOT / 'content/districts/world_plan.json').read_text())


def metadata(old_id):
    if old_id == 0:
        return json.loads((ROOT / 'content/city_art.json').read_text())
    return json.loads((ROOT / f'content/districts/{W.OLD_NAMES[old_id]}_art.json').read_text())


def streets(old_id):
    """Street centrelines in district-world pixels: (axis, at, lo, hi, half)."""
    out = []
    if old_id == 0:
        import core2x
        for s in core2x.STREETS:
            if s['name'] != 'QUEENS PARK CRES':
                out.append((s['axis'], s['at'], min(s['a'], s['b']), max(s['a'], s['b']), s['half']))
        return out
    for road in metadata(old_id)['world']['roads']:
        for (x1, y1), (x2, y2) in zip(road['points'], road['points'][1:]):
            if y1 == y2:
                out.append(('h', y1, min(x1, x2), max(x1, x2), 24))
            else:
                out.append(('v', x1, min(y1, y2), max(y1, y2), 24))
    return out


def grid_fn(old_id):
    grid = W.world_grid(old_id)

    def at(x, y):
        if not (0 <= x < W.WORLD_W and 0 <= y < W.WORLD_H):
            return 15
        return grid[(y // 8) * W.WORLD_TW + x // 8]

    def clear(x, y, half, car):
        for ty in range((y - half) // 8, (y + half) // 8 + 1):
            for tx in range((x - half) // 8, (x + half) // 8 + 1):
                value = at(tx * 8, ty * 8)
                if (value != 0) if car else (value & 15):
                    return False
        return True
    return at, clear


# ---------------------------------------------------------------- portals
def outer_portals():
    """The plan's district seams on their double-scale scenes."""
    out = []
    for pair in plan_world()['portals']:
        ends = []
        for end in (pair['from'], pair['to']):
            m = W.district_map(end['district'])
            X = 24 if end['u'] == 24 else W.WORLD_W - 24
            assert end['u'] in (24, 1000), end
            Y = m.fy.map_int(end['v'])
            ends.append(W.scene_of(end['district'], X, Y))
        out.append({'name': pair['name'], 'from': ends[0], 'to': ends[1], 'access': pair['access'],
                    'modeled_crossing_pixels': 48})
    return out


def inner_portals():
    """One portal per open run across each inner seam of each district."""
    out = []
    for old_id in range(4):
        at, clear = grid_fn(old_id)
        # Vertical seam X = SEAM_X (scenes q and q+1), scanned down Y; the
        # horizontal seam Y = SEAM_Y (q and q+2), scanned along X.
        for vertical in (True, False):
            length = W.WORLD_H if vertical else W.WORLD_W
            run = []
            for p in range(0, length + 8, 8):
                x, y = (W.SEAM_X, p) if vertical else (p, W.SEAM_Y)
                ok = p < length and not at(x, y) & 15
                split = p == (W.SEAM_Y if vertical else W.SEAM_X)
                if ok and not split:
                    run.append(p)
                    continue
                if run:
                    out += _seam_runs(old_id, vertical, run, clear)
                run = [p] if ok else []
    return out


def _seam_runs(old_id, vertical, run, clear):
    """Portals for one open run along a seam: one at the middle of each road
    in it (cars and walkers); a run without a road (a park, a path) gets
    walkers' portals at its middle, or every 96 px along a wide one."""
    at, _ = grid_fn(old_id)
    roads, current = [], []
    for p in run + [None]:
        x, y = ((W.SEAM_X, p) if vertical else (p, W.SEAM_Y)) if p is not None else (0, 0)
        if p is not None and at(x, y) == 0:
            current.append(p)
            continue
        if len(current) >= 3:
            roads.append((current[0] + current[-1] + 8) // 2)
        current = []
    lo, hi = run[0], run[-1] + 8
    if roads:
        centres = [(c, True) for c in roads]
    else:
        centres = [((lo + hi) // 2, False)] if hi - lo <= 96 else [(c, False) for c in range(lo + 48, hi - 40, 96)]
    out = []
    for c, road in centres:
        x, y = (W.SEAM_X, c) if vertical else (c, W.SEAM_Y)
        a = W.scene_of(old_id, x - 1 if vertical else x, y - (0 if vertical else 1))
        b = W.scene_of(old_id, x, y)
        a = (a[0], 1000, a[2]) if vertical else (a[0], a[1], 952)
        b = (b[0], 24, b[2]) if vertical else (b[0], b[1], 24)
        lateral = a[2] if vertical else a[1]
        if not 32 <= lateral < (944 if vertical else 992):
            continue
        car = road and all(at(x + dx, y + dy) == 0 for dx in (-8, 0, 8) for dy in (-8, 0, 8))
        # Both ends are the same district-world point (the scenes overlap).
        out.append({'name': f'{W.OLD_NAMES[old_id]} inner seam', 'from': a, 'to': b,
                    'access': ['foot', 'vehicle'] if car else ['foot'], 'modeled_crossing_pixels': 8})
    return out


def next_hops(portals):
    """next[onfoot][from][to]: the neighbour to head for (255: unreachable)."""
    table = [[[255] * COUNT for _ in range(COUNT)] for _ in range(2)]
    for onfoot in (0, 1):
        edges = {(p[0], p[1]) for p in portals if onfoot or p[6]}
        for to in range(COUNT):
            dist = {to: 0}
            queue = deque([to])
            while queue:
                node = queue.popleft()
                for a, b in edges:
                    if b == node and a not in dist:
                        dist[a] = dist[node] + 1
                        queue.append(a)
            for frm in range(COUNT):
                if frm == to or frm not in dist:
                    continue
                table[onfoot][frm][to] = min(b for a, b in edges if a == frm and dist.get(b, 99) == dist[frm] - 1)
    return table


# ---------------------------------------------------------------- traffic
def traffic(old_id):
    """Per scene of the district: six loops round blocks (in lane, right-hand
    traffic) and its signal junctions, all in scene pixels."""
    at, clear = grid_fn(old_id)
    lines = streets(old_id)
    out = {}
    for q in range(4):
        new_id = old_id * 4 + q
        ox, oy = W.scene_origin(new_id)
        # Loops stay inside the scene's own side of each seam.
        box = (ox + 40, oy + 40, ox + 984, oy + 936)
        faces = [f for f in _faces(lines, box, clear) if len(f) <= TD_POINTS]
        # Where blocks are few (the outer districts' long arterials), cars
        # also run out and back along a street, turning at its ends.
        if len(faces) < LOOPS:
            faces += _out_and_back(lines, box, clear)[:LOOPS - len(faces)]
        faces = [list(f) for f in dict.fromkeys(tuple(f) for f in faces)]
        assert faces, (W.scene_slug(new_id), 'no street for traffic')
        # Small blocks first, then spread out: farthest-point picks.
        def size(f):
            xs, ys = [p[0] for p in f], [p[1] for p in f]
            return max(xs) - min(xs) + max(ys) - min(ys)

        def centre(f):
            return sum(p[0] for p in f) / len(f), sum(p[1] for p in f) / len(f)
        faces.sort(key=size)
        chosen = [faces[0]]
        while len(chosen) < min(LOOPS, len(faces)):
            def spread(f):
                cx, cy = centre(f)
                return min(abs(cx - centre(c)[0]) + abs(cy - centre(c)[1]) for c in chosen)
            chosen.append(max((f for f in faces if f not in chosen), key=spread))
        loops = [[(x - ox, y - oy) for x, y in chosen[k % len(chosen)]] for k in range(LOOPS)]
        # Signal junctions: loop corners where two streets cross on all arms.
        corners = []
        for f in chosen:
            for x, y in _centrelines(f):
                arms = sum(clear(x + dx, y + dy, 4, True) for dx, dy in ((0, -56), (0, 56), (-56, 0), (56, 0)))
                if arms == 4 and (x, y) not in corners:
                    corners.append((x, y))
        signals = [(x - ox, y - oy) for x, y in corners[:SIGNALS]]
        out[new_id] = (loops, signals)
    return out


TD_POINTS = 8


def _faces(lines, box, clear):
    """Blocks of the street graph inside box, as in-lane loops: the graph's
    nodes are street crossings and corners, its faces are found by always
    turning right, and each face is inset by LANE (traffic keeps right, so
    the lane is on the block's side when it is driven clockwise)."""
    x0, y0, x1, y1 = box
    nodes = set()
    for a in lines:
        for b in lines:
            if a[0] == 'h' and b[0] == 'v' and a[2] <= b[1] <= a[3] and b[2] <= a[1] <= b[3]:
                x, y = b[1], a[1]
                if x0 <= x <= x1 and y0 <= y <= y1:
                    nodes.add((x, y))
    adj = {n: set() for n in nodes}
    lane = {}
    for axis, at_, lo, hi, half in lines:
        on = sorted(n for n in nodes if (n[1] if axis == 'h' else n[0]) == at_ and lo <= (n[0] if axis == 'h' else n[1]) <= hi)
        for a, b in zip(on, on[1:]):
            off = min(LANE, half - 9)
            if _clear_run(a, b, clear, CAR, off):
                adj[a].add(b)
                adj[b].add(a)
                lane[a, b] = lane[b, a] = max(off, lane.get((a, b), 0))
    # Dead-end spurs are not part of any block's boundary.
    spurs = [n for n in adj if len(adj[n]) < 2]
    while spurs:
        n = spurs.pop()
        for m in adj.pop(n, ()):
            adj[m].discard(n)
            if len(adj[m]) < 2:
                spurs.append(m)
    faces, used = [], set()
    for a in adj:
        for b in adj[a]:
            if (a, b) in used:
                continue
            face, edge = [], (a, b)
            ok = True
            while edge not in used:
                used.add(edge)
                face.append(edge[0])
                u, v = edge
                d = (_sign(v[0] - u[0]), _sign(v[1] - u[1]))
                # Right, straight, left (screen coordinates, y down).
                turns = [(-d[1], d[0]), d, (d[1], -d[0])]
                nxt = None
                for t in turns:
                    for w in adj[v]:
                        if (_sign(w[0] - v[0]), _sign(w[1] - v[1])) == t:
                            nxt = w
                            break
                    if nxt:
                        break
                if nxt is None:
                    ok = False          # a dead end: not a block
                    break
                edge = (v, nxt)
                if len(face) > 64:
                    ok = False
                    break
            if not ok or edge != (a, b):
                continue
            corners = _corners(face)
            # Interior on the right: clockwise on screen, positive area.
            area = sum(p[0] * q[1] - q[0] * p[1] for p, q in zip(corners, corners[1:] + corners[:1]))
            if area <= 0 or len(corners) < 4:
                continue
            loop = _inset(corners, lambda p, q: _lane_of(p, q, face, lane))
            if all(_clear_run(p, q, clear, CAR, 0) for p, q in zip(loop, loop[1:] + loop[:1])):
                faces.append(loop)
                JUNCTIONS[tuple(loop)] = corners
    return faces


def _out_and_back(lines, box, clear):
    """Out-and-back loops on the longest clear street runs in the box, one
    per street segment, longest first."""
    x0, y0, x1, y1 = box
    runs = []
    for axis, at_, lo, hi, half in lines:
        off = min(LANE, half - 9)
        if axis == 'h' and y0 <= at_ <= y1:
            a, b = max(lo, x0), min(hi, x1)
            lane = [(a, at_ + off), (b, at_ + off), (b, at_ - off), (a, at_ - off)]
        elif axis == 'v' and x0 <= at_ <= x1:
            a, b = max(lo, y0), min(hi, y1)
            lane = [(at_ - off, a), (at_ - off, b), (at_ + off, b), (at_ + off, a)]
        else:
            continue
        if b - a >= 160 and all(_clear_run(p, q, clear, CAR, 0) for p, q in zip(lane, lane[1:] + lane[:1])):
            runs.append((b - a, lane))
    return [lane for _, lane in sorted(runs, key=lambda r: -r[0])]


def _sign(v):
    return (v > 0) - (v < 0)


def _corners(face):
    out = []
    n = len(face)
    for i in range(n):
        p, c, q = face[i - 1], face[i], face[(i + 1) % n]
        if (_sign(c[0] - p[0]), _sign(c[1] - p[1])) != (_sign(q[0] - c[0]), _sign(q[1] - c[1])):
            out.append(c)
    return out


def _lane_of(p, q, face, lane):
    """Lane offset of the street run from corner p to corner q of a face."""
    i = face.index(p)
    return min(lane[face[i], face[(i + 1) % len(face)]], lane[face[face.index(q) - 1], q])


def _inset(corners, offset):
    """Move each corner to the right of both of its edges by the edges'
    lane offsets (offset(p, q) for the run p -> q)."""
    out = []
    n = len(corners)
    for i in range(n):
        p, c, q = corners[i - 1], corners[i], corners[(i + 1) % n]
        d1 = (_sign(c[0] - p[0]), _sign(c[1] - p[1]))
        d2 = (_sign(q[0] - c[0]), _sign(q[1] - c[1]))
        l1, l2 = offset(p, c), offset(c, q)
        out.append((c[0] - d1[1] * l1 - d2[1] * l2, c[1] + d1[0] * l1 + d2[0] * l2))
    return out


JUNCTIONS = {}


def _centrelines(loop):
    """The street crossings a block loop turns at (none for out-and-back)."""
    return JUNCTIONS.get(tuple(loop), [])


def _clear_run(a, b, clear, half=8, lane=LANE):
    """A street run is drivable in both lanes (lane=LANE) or on one line."""
    n = max(abs(b[0] - a[0]), abs(b[1] - a[1]))
    if n == 0:
        return True
    dx, dy = _sign(b[0] - a[0]), _sign(b[1] - a[1])
    for off in ((-lane, lane) if lane else (0,)):
        for step in range(0, n + 1, 4):
            x = a[0] + dx * step + (-dy) * off
            y = a[1] + dy * step + dx * off
            if not clear(x, y, half, True):
                return False
    return True


# ---------------------------------------------------------- fixed places
def spray_bays():
    """Each district's body shop bay centre, in its scene."""
    out = {}
    for old_id in range(4):
        meta = metadata(old_id)
        if old_id == 0:
            X, Y = meta['spray_bay']
        else:
            bay = meta['world']['spray_bay']
            X, Y = bay['x'] + 16, bay['y'] + 8
        d, u, v = W.scene_of(old_id, X, Y)
        out[d] = (u, v)
    return out


def hospital():
    """On the sidewalk at the door of Toronto General Hospital."""
    blocks = [b for b in metadata(0)['blocks'] if b.get('landmark') == 'Toronto General Hospital']
    assert len(blocks) == 1
    b = blocks[0]
    at, clear = grid_fn(0)
    x = (b['x'] + b['width'] // 2) // 8 * 8 + 4
    for y in range(b['y'] + b['depth'], b['y'] + b['depth'] + 64, 4):
        if clear(x, y, 4, False):
            return W.scene_of(0, x, y + 4)
    raise AssertionError('no sidewalk at the hospital door')


def build():
    include = HEADER.parent
    canonical = int(re.search(r'#define TD_DISTRICT_COUNT (\d+)', (include / 'td_district.h').read_text()).group(1))
    assert canonical == COUNT
    pairs = outer_portals() + inner_portals()
    portals = []
    for pair in pairs:
        a, b = pair['from'], pair['to']
        assert a[0] != b[0], pair
        horizontal = a[1] in (24, 1000)
        if horizontal:
            assert b[1] == 1024 - a[1] and 32 <= a[2] < 944 and 32 <= b[2] < 944, pair
        else:
            assert a[2] in (24, 952) and b[2] == 976 - a[2] and 32 <= a[1] < 992 and 32 <= b[1] < 992, pair
        for origin, dest in ((a, b), (b, a)):
            portals.append((origin[0], dest[0], origin[1], origin[2], dest[1], dest[2], int('vehicle' in pair['access'])))
    assert len(portals) <= 512 and len(set(portals)) == len(portals)
    hops = next_hops(portals)
    for onfoot in (0, 1):
        for frm in range(COUNT):
            for to in range(COUNT):
                assert frm == to or hops[onfoot][frm][to] != 255, ('unreachable', onfoot, frm, to)
    flows = {}
    for old_id in range(4):
        flows.update(traffic(old_id))
    bays = spray_bays()
    sd, su, sv = START
    hd, hu, hv = hospital()
    out = ['/* Generated by scripts/create_district_world.py: scene seams, routing,',
           '   traffic loops and fixed places of the sixteen-scene world. */',
           '#ifndef TD_DISTRICT_WORLD_H', '#define TD_DISTRICT_WORLD_H',
           '#include "td_world.h"', f'#define TD_WORLD_GENERATED_DISTRICTS {canonical}',
           f'#define TD_PORTALS {len(portals)}',
           f'#define TD_START_DISTRICT {sd}', f'#define TD_START_U {su}', f'#define TD_START_V {sv}',
           f'#define TD_HOSPITAL_DISTRICT {hd}', f'#define TD_HOSPITAL_U {hu}', f'#define TD_HOSPITAL_V {hv}',
           f'typedef char td_world_signal_slots[(TD_SIGNALS=={SIGNALS})?1:-1];',
           f'typedef char td_world_traffic_points[(TD_TRAFFIC_POINTS=={TD_POINTS})?1:-1];',
           '#endif /* TD_DISTRICT_WORLD_H */',
           '/* Data blocks: each once, in the file that defines its switch (they',
           '   stay outside the include guard, so an earlier include of the macros',
           '   does not hide them). */',
           '#if defined(TD_WORLD_DATA) && !defined(TD_WORLD_DATA_DONE)',
           '#define TD_WORLD_DATA_DONE',
           'static const td_portal_t td_portals[TD_PORTALS]={']
    out += ['    {' + ','.join(map(str, p)) + '},' for p in portals]
    out += ['};', f'static const UBYTE td_world_next[2][{COUNT}][{COUNT}]={{']
    for onfoot in (0, 1):
        out.append('  {' + ','.join('{' + ','.join(map(str, row)) + '}' for row in hops[onfoot]) + '},')
    out += ['};', f'static const char td_district_names[{COUNT}][19]={{']
    for name in W.SCENE_NAMES:
        assert len(name) <= 18 and '"' not in name
        out.append(f'    "{name}",')
    out += ['};', f'static const UBYTE td_world_traffic_counts[{COUNT}][{LOOPS}]={{']
    out += ['    {' + ','.join(str(len(loop)) for loop in flows[d][0]) + '},' for d in range(COUNT)]
    out += ['};', f'static const UWORD td_world_traffic[{COUNT}][{LOOPS}][TD_TRAFFIC_POINTS][2]={{']
    for d in range(COUNT):
        loops, _ = flows[d]
        out.append('  {' + ','.join('{' + ','.join('{%d,%d}' % p for p in loop) + '}' for loop in loops) + '},')
    out += ['};', f'static const UWORD td_world_signals_at[{COUNT}][TD_SIGNALS][2]={{']
    for d in range(COUNT):
        _, signals = flows[d]
        signals = signals + [(0xFFFF, 0xFFFF)] * (SIGNALS - len(signals))
        out.append('  {' + ','.join('{%d,%d}' % s for s in signals) + '},')
    out += ['};', '#endif /* TD_WORLD_DATA */',
            f'/* Spray bay centre per scene; 0xFFFF where the scene has none. */',
            '#if defined(TD_SPRAY_DATA) && !defined(TD_SPRAY_DATA_DONE)',
            '#define TD_SPRAY_DATA_DONE',
            f'static const UWORD td_spray_at[{COUNT}][2]={{' +
            ','.join('{%d,%d}' % bays.get(d, (0xFFFF, 0xFFFF)) for d in range(COUNT)) + '};',
            '#endif', '']
    metadata_bytes = COUNT * 19 + len(portals) * 11 + COUNT * LOOPS * 16 + COUNT * SIGNALS * 4 + 2 * COUNT * COUNT
    assert metadata_bytes <= 12288, 'World metadata needs another ROM bank'
    world = {
        'status': 'Sixteen native scenes: four compressed districts drawn at double scale (world2x.py).',
        'projection': 'North-up local pixels; each scene is 1024 by 976 and overlaps its neighbours by 48 px. '
                      'Positions in the 1x plan map through world2x.map_point.',
        'districts': [{'id': d, 'name': W.SCENE_NAMES[d], 'scene': W.scene_slug(d), 'symbol': 'scene_' + W.scene_slug(d),
                       'plan_district': d >> 2, 'origin': list(W.scene_origin(d)),
                       'width_pixels': W.SCENE_W, 'height_pixels': W.SCENE_H,
                       # The city map lays the districts out west to east.
                       'atlas_x': [2, 1, 0, 3][d >> 2] * W.WORLD_W + W.scene_origin(d)[0],
                       'atlas_y': W.scene_origin(d)[1]} for d in range(COUNT)],
        'portals': [{'name': p['name'], 'from': dict(zip(('district', 'u', 'v'), p['from'])),
                     'to': dict(zip(('district', 'u', 'v'), p['to'])), 'access': p['access'],
                     'modeled_crossing_pixels': p['modeled_crossing_pixels']} for p in pairs],
        'start': dict(zip(('district', 'u', 'v'), START)),
        'hospital': dict(zip(('district', 'u', 'v'), (hd, hu, hv))),
        'spray_bays': [{'district': d, 'u': u, 'v': v} for d, (u, v) in sorted(bays.items())],
        'traffic': [{'district': d, 'loops': [[list(p) for p in loop] for loop in flows[d][0]],
                     'signals': [list(s) for s in flows[d][1]]} for d in range(COUNT)],
        'notice': 'Road shapes, mission entrances, timetables and art are original game design based on '
                  'separately attributed geographic facts; the districts do not cover all of the former City of Toronto.',
    }
    return '\n'.join(out), json.dumps(world, indent=1) + '\n'


if __name__ == '__main__':
    header, world = build()
    path = ROOT / 'content/districts/world.json'
    if '--check' in sys.argv:
        assert HEADER.read_text() == header, 'District native data is stale; regenerate it'
        assert path.read_text() == world, 'content/districts/world.json is stale'
        print('Native seam, routing and traffic data matches the sixteen scenes')
    else:
        HEADER.write_text(header)
        path.write_text(world)
        print('Compiled sixteen-scene seams, routing, traffic loops and fixed places')
