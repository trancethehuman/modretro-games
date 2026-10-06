"""Generate curbside knock-over props and transit vehicle berths.

Reads the registered district collision grids, campaign stops and district
portals; writes the BANKED data header `td_street.h`. Props are decoration
the courier's vehicle can knock over: they sit in the gutter of straight road
segments, never change a collision value and keep clear of stops, parking
anchors and district seams. Berths are where the visible bus, streetcar or
ferry stops for each transit stop.

`--check` verifies the header is current without writing.
"""
import json
import re
import sys
from pathlib import Path
from check_campaign import ROOT, decode

ENGINE = ROOT / 'project/plugins/toronto-driving/engine'
OUTPUT = ENGINE / 'include/td_street.h'
PROPS_PER_DISTRICT = 96
KINDS = ('cone', 'bin', 'newsbox', 'barrel')
CLEARANCE = 32          # px from stops, anchors and seams
GUTTER = 5              # px from the curb line to the prop's position
LANE = 8                # px from road centre to a travel lane, as traffic uses
ROAD, WALK, SOLID = 0, 16, 15


def load_grid(scene):
    data = json.loads((ROOT / 'project/project/scenes' / scene / 'scene.gbsres').read_text())
    w, h = data['width'], data['height']
    cells = decode(data['collisions'])
    assert len(cells) == w * h
    return w, h, cells


def seed(*values):
    x = 2166136261
    for v in values:
        x = ((x ^ (v & 0xFFFFFFFF)) * 16777619) & 0xFFFFFFFF
    return x


def curb_runs(w, h, at):
    """Straight curb edges as (axis, line, start, end, road_side) in pixels.
    axis 'h': curb line y between a sidewalk row and a road row; 'v' likewise.
    road_side is +1 when the road lies below/right of the line."""
    runs = []
    for ty in range(1, h - 5):
        for side in (+1, -1):
            row = ty if side > 0 else ty - 1          # road tile row
            walk = ty - 1 if side > 0 else ty
            deep = [row + side * k for k in range(1, 5)]
            start = None
            for tx in range(w + 1):
                ok = (tx < w and at(tx, row) == ROAD and at(tx, walk) == WALK and
                      all(0 <= r < h and at(tx, r) == ROAD for r in deep))
                if ok and start is None:
                    start = tx
                if not ok and start is not None:
                    if tx - start >= 6:
                        runs.append(('h', ty * 8, start * 8 + 16, tx * 8 - 16, side))
                    start = None
    for tx in range(1, w - 5):
        for side in (+1, -1):
            col = tx if side > 0 else tx - 1
            walk = tx - 1 if side > 0 else tx
            deep = [col + side * k for k in range(1, 5)]
            start = None
            for ty in range(h + 1):
                ok = (ty < h and at(col, ty) == ROAD and at(walk, ty) == WALK and
                      all(0 <= c < w and at(c, ty) == ROAD for c in deep))
                if ok and start is None:
                    start = ty
                if not ok and start is not None:
                    if ty - start >= 6:
                        runs.append(('v', tx * 8, start * 8 + 16, ty * 8 - 16, side))
                    start = None
    return runs


def place_props(district, w, h, at, avoid):
    def clear(u, v):
        return all(max(abs(u - a), abs(v - b)) >= CLEARANCE for a, b in avoid)

    props = []
    for axis, line, start, end, side in curb_runs(w, h, at):
        pos = start + seed(district, line, start, side) % 48
        while pos + 24 <= end:
            s = seed(district, axis == 'h', line, pos, side)
            kind = (0, 0, 1, 2, 3, 1)[s % 6]
            count = (3, 2, 1)[(s >> 4) % 3] if kind in (0, 3) else 1 + (s >> 4) % 2
            gutter = line + side * GUTTER - (1 if side < 0 else 0)
            group = []
            for k in range(count):
                along = pos + k * 10
                u, v = (along, gutter) if axis == 'h' else (gutter, along)
                if along > end or not clear(u, v):
                    group = []
                    break
                tile = at(u // 8, v // 8)
                assert tile == ROAD, (district, u, v, tile)
                group.append((u, v, kind))
            props += group
            pos += 56 + (s >> 8) % 48
    # Spread the cap evenly over the district rather than truncating its north.
    if len(props) > PROPS_PER_DISTRICT:
        step = len(props) / PROPS_PER_DISTRICT
        props = [props[int(i * step)] for i in range(PROPS_PER_DISTRICT)]
    return props


def road_centre(at, w, h, u, v):
    """Centre (px) of the east-west road nearest the stop, measured where the
    road is narrowest so a crossing street does not widen it."""
    best = None
    for du in range(-48, 49, 8):
        tx = (u + du) // 8
        if not 0 <= tx < w:
            continue
        for dv in range(0, 48, 8):
            for sign in (1, -1):
                ty = (v + sign * dv) // 8
                if 0 <= ty < h and at(tx, ty) == ROAD:
                    top = bottom = ty
                    while top > 0 and at(tx, top - 1) == ROAD:
                        top -= 1
                    while bottom < h - 1 and at(tx, bottom + 1) == ROAD:
                        bottom += 1
                    span = (bottom - top + 1, abs(dv), (top * 8 + (bottom + 1) * 8) // 2)
                    if best is None or span < best:
                        best = span
                    break
            else:
                continue
            break
    assert best and best[0] <= 8, ('no east-west road near stop', u, v, best)
    return best[2]


def ferry_berth(at, w, h, u, v):
    """Ferry centre beside the dock: 22 px into the nearest open water to the
    north or south, and which side the water is on (+1 south, -1 north)."""
    tx = u // 8
    for dist in range(1, 8):
        for sign in (1, -1):
            ty = v // 8 + sign * dist
            run = [v // 8 + sign * (dist + k) for k in range(6)]
            if all(0 <= r < h and at(tx, r) == SOLID for r in run):
                edge = ty * 8 if sign > 0 else (ty + 1) * 8
                return edge + sign * 22, sign
    raise AssertionError(('no water beside ferry dock', u, v))


def build():
    world = json.loads((ROOT / 'content/districts/world.json').read_text())
    campaign = json.loads((ROOT / 'content/campaign.json').read_text())
    stops = campaign['stops']
    grids = {d['id']: load_grid(d['scene']) for d in world['districts']}
    lines = ['/* Generated by scripts/create_street_life.py from registered collisions. */',
             '#ifndef TD_STREET_H', '#define TD_STREET_H',
             f'#define TD_PROP_KINDS {len(KINDS)}', f'#define TD_PROPS_PER_DISTRICT {PROPS_PER_DISTRICT}',
             f'#define TD_STREET_STOPS {len(stops)}',
             '#ifdef TD_STREET_DATA']
    starts, counts, flat = [], [], []
    summary = []
    for d in world['districts']:
        did = d['id']
        w, h, cells = grids[did]
        at = lambda x, y, c=cells, w=w: c[y * w + x]
        avoid = [(s['u'], s['v']) for s in stops if s['district'] == did]
        avoid += [(s['parking_anchor']['u'], s['parking_anchor']['v']) for s in stops
                  if s['district'] == did and 'parking_anchor' in s]
        for p in world['portals']:
            for end in (p['from'], p['to']):
                if end['district'] == did:
                    avoid.append((end['u'], end['v']))
        props = place_props(did, w, h, at, avoid)
        starts.append(len(flat)); counts.append(len(props)); flat += props
        summary.append(f"{d['scene'].removeprefix('toronto_')}:{len(props)}")
    lines.append('static const UBYTE td_prop_start[4]={' + ','.join(map(str, starts)) + '};')
    lines.append('static const UBYTE td_prop_count[4]={' + ','.join(map(str, counts)) + '};')
    lines.append('static const UWORD td_prop_u[%d]={%s};' % (len(flat), ','.join(str(p[0]) for p in flat)))
    lines.append('static const UWORD td_prop_v[%d]={%s};' % (len(flat), ','.join(str(p[1]) for p in flat)))
    lines.append('static const UBYTE td_prop_kind[%d]={%s};' % (len(flat), ','.join(str(p[2]) for p in flat)))
    lines.append('/* Interleaved coarse 8-pixel (u,v) pairs for the per-frame range scan. */')
    lines.append('static const UBYTE td_prop_uv8[%d]={%s};' % (2 * len(flat), ','.join(f'{p[0] >> 3},{p[1] >> 3}' for p in flat)))
    # Berths: bus and streetcar stop in the lane of their travel direction on
    # the east-west road beside the stop; the ferry lies off its dock.
    source = (ENGINE / 'src/td_transit.c').read_text()
    route = lambda name: {int(x) for x in re.search(name + r'\[\] = \{([^}]*)\}', source).group(1).split(',')}
    street = route('td_transit_bus_stops') | {i for i, s in enumerate(stops) if s['transit'] == 4}
    ferry = route('td_transit_ferry_stops')
    bu, bv, bside = [], [], []
    berths = 0
    for i, s in enumerate(stops):
        w, h, cells = grids[s['district']]
        at = lambda x, y, c=cells, w=w: c[y * w + x]
        if i in street:
            centre = road_centre(at, w, h, s['u'], s['v'])
            # Stop short of a junction: the nearest point whose cross-section
            # is an ordinary road, preferring the stop's own position.
            def junction(u):
                ty, tx = centre // 8, u // 8
                return sum(1 for k in range(-6, 7) if 0 <= ty + k < h and at(tx, ty + k) == ROAD) > 8
            u = min((s['u'] + 8 * k for k in range(-8, 9)), key=lambda u: (junction(u), abs(u - s['u']), -u))
            bu.append(u); bv.append(centre); bside.append(0); berths += 1
        elif i in ferry:
            v, side = ferry_berth(at, w, h, s['u'], s['v'])
            bu.append(s['u']); bv.append(v); bside.append(1 if side > 0 else 2); berths += 1
        else:
            bu.append(0); bv.append(0); bside.append(0)
    lines.append('static const UWORD td_berth_u[%d]={%s};' % (len(stops), ','.join(map(str, bu))))
    lines.append('static const UWORD td_berth_v[%d]={%s};' % (len(stops), ','.join(map(str, bv))))
    lines.append('static const UBYTE td_berth_district[%d]={%s};' % (len(stops), ','.join(str(s['district']) for s in stops)))
    lines.append('/* Ferry water side: 1 south, 2 north; 0 for street berths. */')
    lines.append('static const UBYTE td_berth_side[%d]={%s};' % (len(stops), ','.join(map(str, bside))))
    lines += ['#endif', '#endif', '']
    return '\n'.join(lines), summary, berths, len(stops)


def main():
    text, summary, berths, stops = build()
    if '--check' in sys.argv:
        assert OUTPUT.read_text() == text, 'Stale street data: run scripts/create_street_life.py'
        print(f"Street props and berths match collisions: props {' '.join(summary)}; {berths} transit berths.")
        return
    OUTPUT.write_text(text)
    print(f"Wrote street props {' '.join(summary)} and {berths} transit berths for {stops} stops.")


if __name__ == '__main__':
    main()
