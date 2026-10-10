"""Generate the navigation names the HUD announces: neighbourhoods, landmarks
and junctions in every scene.

Driving or walking into a new neighbourhood shows its name; passing a
landmark shows what it is; crossing a junction shows its two streets
("SPADINA & DUNDAS"), the way Torontonians give directions. Neighbourhood
names and extents follow the City of Toronto neighbourhood and BIA names
recorded in content/districts/core-research.json ("navigation"); where a
neighbourhood is wider than this compressed map, its rectangle is design.
Landmarks come from the generated art (content/city_art.json and the
district art files), junctions from the street layouts.

Places are worked out per district in district-world pixels (the double-
scale drawing, world2x.py) and cut into its four scenes.

Writes content/places.json and engine/src/td_places.c. `--check` verifies
both without writing.
"""
from pathlib import Path
import json, sys
import city_layout as L
import core2x
import west_layout as W
import east_layout as E
import world2x
from create_city_art import REGIONS

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'project/plugins/toronto-driving/engine'
OUT_C = ENGINE / 'src/td_places.c'
OUT_JSON = ROOT / 'content/places.json'
FONT = set(" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/.-+?<>$%#=,'!()&")
WIDTH = 20                 # one HUD row
REGION = 128               # junction and landmark lookup buckets (pixels)
MARK_REACH = 24            # a landmark is announced this close to its footprint
JUNCTION_REACH = 8         # beyond the asphalt square of a junction
AREA_HOLD = 16             # engine TD_AREA_HOLD: a neighbourhood holds this far outside it

# Core neighbourhoods: art regions (create_city_art.REGIONS, real metres on
# the downtown grid) renamed where the art name is a landmark rather than a
# neighbourhood. Landmarks keep their own announcements.
CORE_AREA_NAMES = {
    'RIVERDALE PARK': 'RIVERDALE',
    'RIVERSIDE / PORT LANDS': 'SOUTH RIVERDALE',
    'ALLAN GARDENS': 'GARDEN DISTRICT',
    'CITY HALL': 'BAY ST CORRIDOR',
    'UNION STATION': 'FINANCIAL DISTRICT',
    'CN TOWER / ROGERS CENTRE': 'ENTERTAINMENT DIST',
    'ENTERTAINMENT DISTRICT': 'ENTERTAINMENT DIST',
    'LIBERTY VILLAGE / FORT YORK': 'LIBERTY VILLAGE',
    'UNIVERSITY OF TORONTO': 'UNIV OF TORONTO',
    'QUEENS PARK': 'BAY ST CORRIDOR',
}
# Checked before the regions (first match wins): the waterfront below the
# rail corridor, the Islands and Fort York's side of Liberty Village.
CORE_AREAS_FIRST = [
    ('HANLANS POINT', (336, 880, 600, 976)),
    ('CENTRE ISLAND', (600, 880, 792, 976)),
    ('WARDS ISLAND', (792, 860, 1024, 976)),
    ('INNER HARBOUR', (0, 848, 1024, 976)),
    ('HARBOURFRONT', (256, 800, 640, 848)),
    ('EAST BAYFRONT', (640, 800, 848, 848)),
    ('WATERFRONT', (0, 800, 1024, 848)),
    ('FORT YORK', (176, 640, 256, 800)),
    # Queen's Park itself (the art region of that name runs east to Yonge),
    # and Victoria University across Queen's Park from the museum.
    ('QUEENS PARK', (440, 128, 584, 288)),
    ('UNIV OF TORONTO', (544, 88, 640, 176)),
]
# Other scenes: pixel rectangles over their compressed road graphs, in the
# layouts (first match wins; the last covers the rest of the scene).
SCENE_AREAS = {1: W.WEST_AREAS, 2: W.HIGH_PARK_AREAS, 3: E.EAST_AREAS}
# HUD names for landmarks (art names on the left). Names not listed here are
# upper-cased as they are; a None skips the entry.
MARK_NAMES = {
    'Ontario Legislative Building': 'ONTARIO LEGISLATURE',
    'Royal Ontario Museum': 'ROYAL ONTARIO MUSEUM',
    'Art Gallery of Ontario': 'ART GALLERY (AGO)',
    'Allan Gardens Palm House': 'PALM HOUSE',
    'QUEENS PARK': 'QUEENS PARK',
    'King Edward VII statue': None,
    'Hanlans service pavilion': 'HANLANS POINT',
    'Centre Island pavilion': 'CENTRE ISLAND',
    'Wards Island cottages': 'WARDS ISLAND',
    'Distillery District': 'DISTILLERY',
    'Roncesvalles Carhouse': 'STREETCAR CARHOUSE',
    'Sorauren Park fieldhouse': None,
    'Junction heritage brick block': None,
    'Sunnyside Bathing Pavilion': 'SUNNYSIDE PAVILION',
    'Ashbridge Estate house': 'ASHBRIDGE ESTATE',
    'Carlaw Works': None,        # fictional
    'SPRAY BAY': None,
    'YONGE-DUNDAS SCREEN': 'SANKOFA SQUARE',   # Yonge-Dundas Square, renamed by City Council (2023; unveiled 2025)
    'UNIVERSITY OF TORONTO': None,              # the campus lawn fill; the area carries the name
    'Toronto General Hospital': 'HOSPITAL ROW',  # University Ave's hospitals, as locals call them
    'Mount Sinai Hospital': 'HOSPITAL ROW',
    'Princess of Wales Theatre': 'PRINCESS OF WALES',
    'Royal Alexandra Theatre': 'ROYAL ALEX THEATRE',
}
# Short street names for junctions (suffixes dropped as people say them).
SUFFIXES = (' ST W', ' ST E', ' ST', ' AVE', ' RD', ' BLVD W', ' BLVD', ' DR', ' CRES', ' TRL')
SHORT = {'THE QUEENSWAY': 'QUEENSWAY', 'LAKE SHORE BLVD W': 'LAKE SHORE', 'QUEENS PARK CRES': 'QUEENS PARK',
         'PAPE AVE': 'PAPE'}


def short(name):
    name = name.upper()
    if name in SHORT:
        return SHORT[name]
    for s in SUFFIXES:
        if name.endswith(s):
            return name[:-len(s)]
    return name


def hud(name):
    assert len(name) <= WIDTH and set(name) <= FONT, ('HUD place name', name)
    return name


def junction_name(ns, ew):
    a, b = short(ns), short(ew)
    a2 = a.replace(' PARK', ' PK')
    for text in (f'{a} & {b}', f'{a2} & {b}', f'{a2}/{b}'):
        if len(text) <= WIDTH:
            return hud(text)
    return None            # too long for one row: the street name still shows


def inverse(anchors, px):
    """Real metres for pixel px (inverse of city_layout's mapping)."""
    pts = sorted((x, m) for m, x in anchors)
    if px <= pts[0][0]:
        return pts[0][1] - (pts[0][0] - px) * 100
    for (x0, m0), (x1, m1) in zip(pts, pts[1:]):
        if px <= x1:
            return m0 + (px - x0) * (m1 - m0) / (x1 - x0)
    return pts[-1][1] + (px - pts[-1][0]) * 100


def plan_rect(old, rect):
    """A plan rectangle (x0, y0, x1, y1) in district-world pixels; the plan's
    scene edges stay the district's edges."""
    m = world2x.district_map(old)
    x0, y0, x1, y1 = rect
    X0 = 0 if x0 <= 0 else m.fx.map_int(x0)
    Y0 = 0 if y0 <= 0 else m.fy.map_int(y0)
    X1 = world2x.WORLD_W if x1 >= world2x.OLD_W else m.fx.map_int(x1)
    Y1 = world2x.WORLD_H if y1 >= world2x.OLD_H else m.fy.map_int(y1)
    return (X0, Y0, X1, Y1)


def core_areas():
    return [(n, plan_rect(0, r)) for n, r in core_areas_plan()]


def core_areas_plan():
    out = [(n, r) for n, r in CORE_AREAS_FIRST]
    for name, u0, u1, w0, w1, _ in REGIONS:
        x0, x1 = L.map_u(u0), L.map_u(u1)
        y0, y1 = sorted((L.map_w(w0), L.map_w(w1)))
        # Regions at the edge of the anchors run on to the scene edge.
        if u0 <= min(m for m, _ in L.U_ANCHORS):
            x0 = 0
        if u1 >= max(m for m, _ in L.U_ANCHORS):
            x1 = 1024
        if w1 >= max(m for m, _ in L.W_ANCHORS):
            y0 = 0
        out.append((CORE_AREA_NAMES.get(name, name), (x0, y0, x1, y1)))
    out.append(('DOWNTOWN', (0, 0, 1024, 976)))
    return out


def core_junctions():
    hs = [s for s in core2x.STREETS if s['axis'] == 'h']; vs = [s for s in core2x.STREETS if s['axis'] == 'v']
    out = []

    def label(s, p):
        return next((n for f, t, n in s['names'] if f < p <= t), s['name'])
    for v in vs:
        for h in hs:
            if h['a'] <= v['at'] <= h['b'] and v['a'] <= h['at'] <= v['b']:
                rx, ry = v['half'] + JUNCTION_REACH, h['half'] + JUNCTION_REACH
                name = junction_name(label(v, h['at']), label(h, v['at']))
                if name:
                    out.append((name, (v['at'] - rx, h['at'] - ry, v['at'] + rx, h['at'] + ry)))
    return out


def graph_junctions(spec, half):
    """Junctions of different named roads in a polyline road graph: crossings,
    and a road ending on (or turning at) another."""
    def road_name(r):
        n = r['name'].upper().replace(' STREET WEST', ' ST W').replace(' STREET EAST', ' ST E').replace(' STREET', ' ST')
        n = n.replace(' AVENUE', ' AVE').replace(' BOULEVARD WEST', ' BLVD W').replace(' BOULEVARD', ' BLVD').replace(' ROAD', ' RD')
        n = n.replace(' DRIVE', ' DR').replace(' NORTH FRAGMENT', '').replace(' SOUTH FRAGMENT', '')
        if n.endswith(' SOUTH APPROACH'):
            n = n[:-len(' SOUTH APPROACH')]
        return n

    def segs(r):
        return [(tuple(a), tuple(b)) for a, b in zip(r['points'], r['points'][1:])]

    def meet(s, t):
        (ax, ay), (bx, by) = s; (cx, cy), (dx, dy) = t
        if ax == bx and cy == dy:          # s vertical, t horizontal
            x, y = ax, cy
        elif ay == by and cx == dx:
            x, y = cx, ay
        else:                              # parallel: a shared end point
            common = {s[0], s[1]} & {t[0], t[1]}
            return next(iter(common), None)
        if min(ax, bx) <= x <= max(ax, bx) and min(ay, by) <= y <= max(ay, by) and \
           min(cx, dx) <= x <= max(cx, dx) and min(cy, dy) <= y <= max(cy, dy):
            return (x, y)
        return None

    def axis_at(r, p):
        """'v' when the road runs north-south through p (else 'h')."""
        for (ax, ay), (bx, by) in segs(r):
            if ax == bx == p[0] and min(ay, by) <= p[1] <= max(ay, by):
                return 'v'
        return 'h'
    roads = spec['roads']
    found, pair_name = {}, {}
    for i, r in enumerate(roads):
        for j in range(i + 1, len(roads)):
            q = roads[j]
            if road_name(r) == road_name(q):
                continue
            for s in segs(r):
                for t in segs(q):
                    p = meet(s, t)
                    if p is None or p in found:
                        continue
                    a, b = (r, q) if axis_at(r, p) == 'v' else (q, r)
                    if axis_at(a, p) == axis_at(b, p):
                        a, b = sorted((r, q), key=lambda k: road_name(k))
                    pair = frozenset((road_name(a), road_name(b)))
                    if pair not in pair_name:
                        pair_name[pair] = junction_name(road_name(a), road_name(b))
                    found[p] = pair_name[pair]
    reach = half + JUNCTION_REACH
    # One box per junction; where two roads share a jog, their two corners
    # make one box.
    boxes = []
    for (x, y), n in sorted(found.items(), key=lambda k: (k[0][1], k[0][0])):
        if not n:
            continue
        box = [x - reach, y - reach, x + reach, y + reach]
        for other in boxes:
            o = other[1]
            if other[0] == n and o[0] <= box[2] and box[0] <= o[2] and o[1] <= box[3] and box[1] <= o[3]:
                other[1] = [min(o[0], box[0]), min(o[1], box[1]), max(o[2], box[2]), max(o[3], box[3])]
                break
        else:
            boxes.append([n, box])
    return [(n, tuple(b)) for n, b in boxes]


def marks_from(entries):
    out, seen = [], set()
    for name, rect in entries:
        label = MARK_NAMES.get(name, name.upper())
        if label is None or label in seen:
            continue
        seen.add(label)
        x, y, w, h = rect
        out.append((hud(label), (x - MARK_REACH, y - MARK_REACH, x + w + MARK_REACH, y + h + MARK_REACH)))
    return out


def core_marks():
    art = json.loads((ROOT / 'content/city_art.json').read_text())
    entries = [(d['name'], d['rect']) for d in art['districts'] if d['kind'] in ('landmark', 'scenery')]
    entries += [(d['name'], d['rect']) for d in art['districts'] if d['kind'] == 'park']
    return marks_from(entries)


def scene_marks(slug, old):
    art = json.loads((ROOT / f'content/districts/{slug}_art.json').read_text())['plan']
    entries = [(p['name'], p['rect']) for p in art.get('parks', [])]
    for p in art.get('parks', []):
        for f in p.get('features', []):
            if f['kind'] == 'paddock':
                entries.append(('HIGH PARK ZOO', f['rect']))
        if p.get('fieldhouse'):
            pass
    if art.get('pond'):
        xs = [p[0] for p in art['pond']]; ys = [p[1] for p in art['pond']]
        entries.append(('GRENADIER POND', [min(xs), min(ys), max(xs) - min(xs), max(ys) - min(ys)]))
    entries += [(m['name'], [m['x'], m['y'], m['width'], m['depth']]) for m in art.get('landmarks', [])]
    # Plan rectangles (x, y, w, h) in district-world pixels.
    mapped = []
    for name, (x, y, w, h) in entries:
        X0, Y0, X1, Y1 = plan_rect(old, (x, y, x + w, y + h))
        mapped.append((name, [X0, Y0, X1 - X0, Y1 - Y0]))
    return marks_from(mapped)


def world_spec(slug):
    return json.loads((ROOT / f'content/districts/{slug}_art.json').read_text())['world']


def split(district_places):
    """Each scene's share of its district's places, in scene pixels; areas
    keep their order (first match wins)."""
    scenes = []
    for new_id in range(16):
        places = district_places[new_id >> 2]
        ox, oy = world2x.scene_origin(new_id)
        scene = {}
        for kind in ('areas', 'marks', 'junctions'):
            out = []
            for name, (x0, y0, x1, y1) in places[kind]:
                if x1 <= ox or x0 >= ox + world2x.SCENE_W or y1 <= oy or y0 >= oy + world2x.SCENE_H:
                    continue
                out.append((name, (max(x0 - ox, 0), max(y0 - oy, 0), min(x1 - ox, world2x.SCENE_W), min(y1 - oy, world2x.SCENE_H))))
            scene[kind] = out
        scenes.append(scene)
    return scenes


def build():
    districts = [
        {'areas': core_areas(), 'marks': core_marks(), 'junctions': core_junctions()},
        {'areas': [(n, plan_rect(1, r)) for n, r in SCENE_AREAS[1]], 'marks': scene_marks('west', 1),
         'junctions': graph_junctions(world_spec('west'), 24)},
        {'areas': [(n, plan_rect(2, r)) for n, r in SCENE_AREAS[2]], 'marks': scene_marks('high_park', 2),
         'junctions': graph_junctions(world_spec('high_park'), 24)},
        {'areas': [(n, plan_rect(3, r)) for n, r in SCENE_AREAS[3]], 'marks': scene_marks('east', 3),
         'junctions': graph_junctions(world_spec('east'), 24)},
    ]
    scenes = split(districts)
    for s in scenes:
        for kind in ('areas', 'marks', 'junctions'):
            for name, _ in s[kind]:
                hud(name)
    return scenes


def c_source(scenes):
    kinds = ('areas', 'marks', 'junctions')
    names = {k: [] for k in kinds}
    for s in scenes:
        for k in kinds:
            for name, _ in s[k]:
                if name not in names[k]:
                    names[k].append(name)
    for k in kinds:
        assert len(names[k]) < 255, (k, len(names[k]))
    # One spot table for all three kinds, bucketed by 128-pixel region so a
    # lookup reads only the spots near the courier. Areas keep their order
    # (first match wins) and are bucketed with their hold margin.
    spots, spot_start, spot_list = [], [], []
    firsts = []
    for s in scenes:
        first = len(spots)
        firsts.append(first)
        for kind, k in enumerate(kinds):
            for name, (x0, y0, x1, y1) in s[k]:
                # Bytes in 4-pixel units (inclusive), so the lookup compares
                # bytes; a neighbourhood's rectangle includes its hold margin.
                m = AREA_HOLD if kind == 0 else 0
                spots.append((max(0, x0 - m) // 4 * 4, max(0, y0 - m) // 4 * 4,
                              min(1020, (x1 + m + 3) // 4 * 4 - 4), min(972, (y1 + m + 3) // 4 * 4 - 4),
                              kind, names[k].index(name)))
        for ry in range(8):
            for rx in range(8):
                X0, Y0, X1, Y1 = rx * REGION, ry * REGION, rx * REGION + REGION - 1, ry * REGION + REGION - 1
                spot_start.append(len(spot_list))
                for i in range(first, len(spots)):
                    x0, y0, x1, y1, kind, _ = spots[i]
                    if x0 <= X1 and X0 <= x1 and y0 <= Y1 and Y0 <= y1:
                        spot_list.append(i - first)
    spot_start.append(len(spot_list))
    assert len(spots) < 65536 and len(spot_list) < 65536
    assert all(f2 - f1 <= 256 for f1, f2 in zip(firsts, firsts[1:] + [len(spots)]))
    all_names = names['areas'] + names['marks'] + names['junctions']
    first = [0, len(names['areas']), len(names['areas']) + len(names['marks'])]
    lines = ['/* Generated by scripts/create_places.py: neighbourhood, landmark and',
             ' * junction names for the HUD (content/places.json lists them). */',
             '#pragma bank 255', '#include <string.h>', '#include "td_game.h"', '#include "td_district.h"', '#include "td_places.h"',
             '/* Rectangles in 4-pixel units, inclusive; eight bytes so indexing shifts. */',
             'typedef struct { UBYTE x0,y0,x1,y1,kind,name,pad0,pad1; } td_spot_t;',
             f'typedef char td_places_scenes_match[(TD_DISTRICT_COUNT=={len(scenes)}&&TD_AREA_HOLD=={AREA_HOLD})?1:-1];',
             f'static const char td_place_names[{len(all_names)}][{WIDTH + 1}]={{']
    lines += [f'  "{n}",' for n in all_names]
    lines += ['};', f'static const UBYTE td_place_first[3]={{{",".join(map(str, first))}}};',
              f'static const td_spot_t td_spots[{len(spots)}]={{']
    lines += ['  {%d,%d,%d,%d,%d,%d,0,0},' % (x0 // 4, y0 // 4, x1 // 4, y1 // 4, kind, name) for x0, y0, x1, y1, kind, name in spots]
    lines += ['};', f'static const UWORD td_spot_start[{len(spot_start)}]={{{",".join(map(str, spot_start))}}};',
              f'static const UBYTE td_spot_list[{len(spot_list)}]={{{",".join(map(str, spot_list))}}};',
              '/* Each scene\'s spots start here; td_spot_list counts from it. */',
              f'static const UWORD td_spot_first[{len(firsts)}]={{{",".join(map(str, firsts))}}};',
              '#define TD_HOLD4 (TD_AREA_HOLD/4)',
              '/* Name ids (TD_PLACE_NONE for none) of the places at (u,v): ids[0] the',
              '   neighbourhood, kept while (u,v) stays within TD_AREA_HOLD pixels of the',
              '   current one (ids[0] on entry) so a boundary street does not flicker;',
              '   ids[1] a landmark within reach; ids[2] the junction. A neighbourhood\'s',
              '   stored rectangle includes the hold margin; it contains (u,v) when',
              '   (u,v) is TD_HOLD4 units inside that. */',
              'void td_get_places(UWORD u,UWORD v,UBYTE *ids) BANKED {',
              '    UBYTE area=TD_PLACE_NONE,hold=0,x,y,n;UWORD k;const UBYTE *list;const td_spot_t *s,*base;',
              '    ids[1]=ids[2]=TD_PLACE_NONE;',
              '    if(td.district>=TD_DISTRICT_COUNT||u>=1024||v>=976){ids[0]=TD_PLACE_NONE;return;}',
              '    k=(UWORD)td.district*64+((v>>7)<<3)+(u>>7);list=td_spot_list+td_spot_start[k];n=(UBYTE)(td_spot_start[k+1]-td_spot_start[k]);',
              '    x=(UBYTE)(u>>2);y=(UBYTE)(v>>2);base=td_spots+td_spot_first[td.district];',
              '    for(;n;n--){',
              '        s=base+*list++;',
              '        if(x<s->x0||x>s->x1||y<s->y0||y>s->y1)continue;',
              '        if(!s->kind){',
              '            if(s->name==ids[0])hold=1;',
              '            if(area==TD_PLACE_NONE&&x>=s->x0+TD_HOLD4&&x<=s->x1-TD_HOLD4&&y>=s->y0+TD_HOLD4&&y<=s->y1-TD_HOLD4)area=s->name;',
              '        }else if(ids[s->kind]==TD_PLACE_NONE)ids[s->kind]=s->name;',
              '    }',
              '    if(!hold)ids[0]=area;',
              '}',
              '/* Name `id` of a place kind (TD_PLACE_AREA, _MARK, _JUNCTION) into d. */',
              'void td_get_place_name(UBYTE kind,UBYTE id,char *d) BANKED {',
              '    memcpy(d,td_place_names[td_place_first[kind]+id],TD_PLACE_NAME);',
              '}']
    return '\n'.join(lines) + '\n'


def main(check=False):
    scenes = build()
    labels = [world2x.scene_slug(i).removeprefix('toronto_') for i in range(16)]
    content = {'about': 'Names the HUD announces: neighbourhoods on entry, landmarks within reach, junctions crossed. '
                        'Rectangles are native pixels of each scene; sources and the compression notes are in '
                        'content/districts/core-research.json ("navigation").',
               'scenes': {labels[d]: {k: [{'name': n, 'rect': list(r)} for n, r in s[k]] for k in ('areas', 'marks', 'junctions')}
                          for d, s in enumerate(scenes)}}
    texts = {OUT_JSON: json.dumps(content, indent=1) + '\n', OUT_C: c_source(scenes)}
    if check:
        for path, text in texts.items():
            assert path.read_text() == text, f'Stale places output: {path.relative_to(ROOT)}'
        print('Places match their generator.')
        return
    for path, text in texts.items():
        path.write_text(text)
    counts = [(len(s['areas']), len(s['marks']), len(s['junctions'])) for s in scenes]
    print(f'Wrote places (areas, landmarks, junctions per scene): {counts}.')


if __name__ == '__main__':
    main(check='--check' in sys.argv)
