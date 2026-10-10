"""Generate sidewalk pickups and transit vehicle berths.

Reads the registered district collision grids, campaign stops and district
portals; writes the BANKED data header `td_street.h`. Pickups are cash, first
aid and ammunition the courier collects on foot or by driving over them: they
lie on the pavement beside straight curbs, well apart from each other, never
change a collision value and keep clear of stops, parking anchors and
district seams. Lost parcels (twenty in all) are the hidden collectibles:
deep in parks, plazas and squares, away from any road, each found once. Berths are where the visible bus, streetcar or ferry stops for
each transit stop.

`--check` verifies the header is current without writing.
"""
import json
import re
import sys
from pathlib import Path
from check_campaign import ROOT, decode

ENGINE = ROOT / 'project/plugins/toronto-driving/engine'
OUTPUT = ENGINE / 'include/td_street.h'
PICKUPS_PER_DISTRICT = 14  # per scene (sixteen scenes; indexes fit a byte)
KINDS = ('cash', 'first_aid', 'ammo', 'parcel')
PARCELS = 20            # lost parcels in all: two in each core scene, one elsewhere
PARCEL_SPACING = 128    # px between lost parcels (Chebyshev)
CLEARANCE = 32          # px from stops, anchors and seams
SPACING = 144           # px between pickups (Chebyshev)
KERB = 4                # px from the curb line into the pavement
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


def load_priority(scene):
    """CGB background-priority flags (attribute bit 7) per tile: roof lips,
    canopies and overhanging upper floors that hide sprites."""
    attrs = json.loads((ROOT / 'project/original-art' / (scene + '_attributes.json')).read_text())
    return [bool(a & 0x80) for a in attrs]


def place_pickups(district, w, h, at, avoid, priority):
    """Pickups on the pavement side of straight curbs, at most one per run
    stretch and at least SPACING apart, spread evenly over the district. The
    icon is drawn above its position (x -4..+3, y -13..-4); it must not touch
    a tile whose background priority would hide it."""
    def clear(u, v):
        return all(max(abs(u - a), abs(v - b)) >= CLEARANCE for a, b in avoid)

    def visible(u, v):
        return not any(priority[(y // 8) * w + x // 8]
                       for x in (u - 4, u + 3) for y in (v - 13, v - 8, v - 4) if 0 <= y < h * 8)

    candidates = []
    for axis, line, start, end, side in curb_runs(w, h, at):
        pos = start + seed(district, line, start, side) % 64
        while pos <= end:
            # The pavement lies on the other side of the curb from the road.
            kerb = line - side * KERB - (1 if side > 0 else 0)
            u, v = (pos, kerb) if axis == 'h' else (kerb, pos)
            if clear(u, v) and at(u // 8, v // 8) == WALK and visible(u, v):
                candidates.append((u, v, seed(district, axis == 'h', line, pos, side)))
            pos += 96
    # Deterministic shuffle, then greedy spacing.
    candidates.sort(key=lambda c: c[2])
    chosen = []
    for u, v, s in candidates:
        if all(max(abs(u - a), abs(v - b)) >= SPACING for a, b, _ in chosen):
            chosen.append((u, v, s))
        if len(chosen) == PICKUPS_PER_DISTRICT:
            break
    # North-to-south order; kinds rotate cash, first aid, cash, ammunition.
    chosen.sort(key=lambda c: (c[1], c[0]))
    return [(u, v, (0, 1, 0, 2)[(k + district) % 4]) for k, (u, v, _) in enumerate(chosen)]


def place_parcels(district, w, h, at, avoid, priority, pickups):
    """Lost parcels on open ground at least two tiles from any road, clear of
    stops, seams and the other pickups, well apart from each other."""
    def visible(u, v):
        return not any(priority[(y // 8) * w + x // 8]
                       for x in (u - 4, u + 3) for y in (v - 13, v - 8, v - 4) if 0 <= y < h * 8)
    taken = list(avoid) + [(u, v) for u, v, _ in pickups]
    candidates = []
    for ty in range(4, h - 4):
        for tx in range(4, w - 4):
            if at(tx, ty) != WALK or any(at(tx + dx, ty + dy) == ROAD for dx in range(-2, 3) for dy in range(-2, 3)) or \
                    any(at(tx + dx, ty + dy) != WALK for dx in (-1, 0, 1) for dy in (-1, 0, 1)):
                continue
            u, v = tx * 8 + 4, ty * 8 + 6
            if visible(u, v) and all(max(abs(u - a), abs(v - b)) >= 40 for a, b in taken):
                candidates.append((seed(district, 7, u, v), u, v))
    candidates.sort()
    chosen = []
    for _, u, v in candidates:
        if all(max(abs(u - a), abs(v - b)) >= PARCEL_SPACING for a, b in chosen):
            chosen.append((u, v))
        if len(chosen) == parcels_in(district):
            break
    assert len(chosen) == parcels_in(district), (district, len(chosen))
    return [(u, v, KINDS.index('parcel')) for u, v in sorted(chosen, key=lambda c: (c[1], c[0]))]


def parcels_in(district):
    return 2 if district < 4 else 1


def road_centre(at, w, h, u, v):
    """Centre (px) of the east-west road nearest the stop, measured where the
    road is narrowest so a crossing street does not widen it."""
    best = None
    for du in range(-96, 97, 8):
        tx = (u + du) // 8
        if not 0 <= tx < w:
            continue
        for dv in range(0, 96, 8):
            for sign in (1, -1):
                ty = (v + sign * dv) // 8
                if 0 <= ty < h and at(tx, ty) == ROAD:
                    top = bottom = ty
                    while top > 0 and at(tx, top - 1) == ROAD:
                        top -= 1
                    while bottom < h - 1 and at(tx, bottom + 1) == ROAD:
                        bottom += 1
                    span = (bottom - top + 1, abs(dv), abs(du), (top * 8 + (bottom + 1) * 8) // 2)
                    # A north-south road runs on: only a crossing counts.
                    if span[0] <= 8 and (best is None or span < best):
                        best = span
    assert best, ('no east-west road near stop', u, v)
    return best[3]


def ferry_berth(at, w, h, u, v):
    """Ferry centre beside the dock: 22 px into the nearest open water to the
    north or south, and which side the water is on (+1 south, -1 north)."""
    tx = u // 8
    for dist in range(1, 8):
        for sign in (1, -1):
            ty = v // 8 + sign * dist
            run = [v // 8 + sign * (dist + k) for k in range(6)]
            if all(0 <= r < h and at(tx, r) & SOLID == SOLID for r in run):
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
             f'#define TD_PICKUP_KINDS {len(KINDS)}', f'#define TD_PICKUPS_PER_DISTRICT {PICKUPS_PER_DISTRICT}',
             f'#define TD_PICKUP_PARCEL {KINDS.index("parcel")}',
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
        priority = load_priority(d['scene'])
        pickups = place_pickups(did, w, h, at, avoid, priority)
        assert len(pickups) >= 6, (d['scene'], len(pickups))
        pickups += place_parcels(did, w, h, at, avoid, priority, pickups)
        starts.append(len(flat)); counts.append(len(pickups)); flat += pickups
        summary.append(f"{d['scene'].removeprefix('toronto_')}:{len(pickups)}")
    assert len(flat) < 255
    lines.append(f'static const UBYTE td_pickup_start[{len(starts)}]={{' + ','.join(map(str, starts)) + '};')
    lines.append(f'static const UBYTE td_pickup_count[{len(counts)}]={{' + ','.join(map(str, counts)) + '};')
    lines.append('static const UWORD td_pickup_u[%d]={%s};' % (len(flat), ','.join(str(p[0]) for p in flat)))
    lines.append('static const UWORD td_pickup_v[%d]={%s};' % (len(flat), ','.join(str(p[1]) for p in flat)))
    lines.append('/* 0 cash, 1 first aid, 2 ammunition, 3 lost parcel. */')
    lines.append('static const UBYTE td_pickup_kind[%d]={%s};' % (len(flat), ','.join(str(p[2]) for p in flat)))
    parcel_of, parcels = [], 0
    for p in flat:
        if p[2] == KINDS.index('parcel'):
            parcel_of.append(parcels); parcels += 1
        else:
            parcel_of.append(255)
    lines.insert(lines.index('#ifdef TD_STREET_DATA'), f'#define TD_PICKUP_PARCELS {parcels}')
    lines.append('/* Lost parcel number of each pickup (255: not a parcel). */')
    lines.append('static const UBYTE td_parcel_of[%d]={%s};' % (len(flat), ','.join(map(str, parcel_of))))
    lines.append('/* Interleaved coarse 8-pixel (u,v) pairs for the per-frame range scan. */')
    lines.append('static const UBYTE td_pickup_uv8[%d]={%s};' % (2 * len(flat), ','.join(f'{p[0] >> 3},{p[1] >> 3}' for p in flat)))
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
        print(f"Street pickups and berths match collisions: pickups {' '.join(summary)}; {berths} transit berths.")
        return
    OUTPUT.write_text(text)
    print(f"Wrote street pickups {' '.join(summary)} and {berths} transit berths for {stops} stops.")


if __name__ == '__main__':
    main()
