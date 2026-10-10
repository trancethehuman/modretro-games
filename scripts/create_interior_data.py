"""Generate the engine tables for interiors, their people and the doors
that lead to them.

Reads content/interiors.json (create_interiors.py: scenes, points of
interest, people's spots), content/people.json (create_people.py: looks
and their roles), the city art (building footprints and landmarks) and the
neighbourhoods of content/places.json, and writes:

  engine/include/td_interior_data.h   interiors, points, people, gallery rooms
  engine/include/td_interior_scenes.h the interior scenes' far pointers
  engine/include/td_doors.h           doors per scene
  engine/src/td_interior_text.c       names, plaques, labels (banked)
  content/doors.json                  the doors, for review

Doors: every landmark with an interior gets one on the sidewalk in front of
its entrance (the door drawn on its south face, or the nearest sidewalk
beside the footprint). Then shops, cafes, diners, pubs, record and book
shops and office lobbies are spread through every scene (at most eight a
scene, well apart), each with a fictional name that suits its
neighbourhood; they share the generic interiors. `--check` verifies.
"""
from pathlib import Path
import json
import random
import sys
import world2x
from create_people_mix import AREA_TAG

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'project/plugins/toronto-driving/engine'
OUT_DATA = ENGINE / 'include/td_interior_data.h'
OUT_SCENES = ENGINE / 'include/td_interior_scenes.h'
OUT_DOORS = ENGINE / 'include/td_doors.h'
OUT_TEXT = ENGINE / 'src/td_interior_text.c'
OUT_JSON = ROOT / 'content/doors.json'
FONT = set(" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/.-+?<>$%#=,'!()&")
POINT_KINDS = ('elevator', 'desk', 'plaque', 'exhibit', 'window', 'arch_prev', 'arch_next', 'counter', 'painting', 'screen')
BEHAVES = ('wander', 'gaze', 'sit', 'counter', 'patrol', 'follow', 'window', 'exhibit')
KINDS = ('landmark', 'generic', 'gallery')
FACE = {'e': 0, 'w': 1, 's': 2, 'n': 3}
MAX_SPOTS = 8
# Landmarks with an interior: art name -> (interior key, door name).
LANDMARKS = {
    'CN Tower': ('cn_base', 'CN TOWER'),
    'Art Gallery of Ontario': ('ago', 'ART GALLERY (AGO)'),
    'Royal Ontario Museum': ('rom', 'ROYAL ONT. MUSEUM'),
    'Eaton Centre': ('eaton', 'EATON CENTRE'),
    'Union Station': ('union', 'UNION STATION'),
    'Dragon City': ('dragon_city', 'DRAGON CITY MALL'),
    'Dragon City Mall': ('dragon_city', 'DRAGON CITY MALL'),
    'Chinatown Centre': ('dragon_city', 'CHINATOWN CENTRE'),
    'BYTE BARN': ('electronics', 'BYTE BARN'),
    'Byte Barn': ('electronics', 'BYTE BARN'),
    'Dufferin Mall': ('eaton', 'DUFFERIN MALL'),
}
# Landmarks whose real main entrance is on the north face in this north-up
# map: the AGO on Dundas St W, the ROM on Bloor St W, Union Station on
# Front St W. Others use their south-face door.
NORTH_DOORS = ('Art Gallery of Ontario', 'Royal Ontario Museum', 'Union Station')
# Spot roles -> substrings of look keys that can fill them (first match wins).
ROLE_KEYS = {
    'viewer': ('artlover', 'visitor', 'viewer'), 'guard': ('security', 'guard'), 'docent': ('docent', 'guide'),
    'shopper': ('mallshopper', 'shopper'), 'teen': ('listener', 'student'),
    'clerk': ('clerk',), 'cashier': ('cashier',), 'barista': ('barista',), 'diner': ('diner',), 'tourist': ('tourist',),
    'commuter': ('traveller', 'coffee', 'office'), 'attendant': ('attendant', 'tower'), 'chef': ('chef',),
    'patron': ('regular', 'student', 'artist', 'digger', 'office'), 'office': ('office', 'receptionist'),
    'visitor': ('tourist', 'artlover', 'student', 'birder', 'traveller'),
    # No children in the game (user direction 2026-10-10): school-group
    # spots, if any, are filled by adult visitors.
    'kid': ('tourist', 'artlover', 'student'), 'teacher': ('guide', 'teacher'),
}
# Generic doors by neighbourhood kind (create_people_mix tags).
TEMPLATES = {
    'financial': ('office_lobby', 'cafe', 'office_lobby', 'diner'), 'downtown': ('cafe', 'office_lobby', 'corner_store', 'diner', 'pub'),
    'chinatown': ('chinatown_bakery', 'corner_store', 'chinatown_bakery', 'diner'), 'kensington': ('shop', 'cafe', 'corner_store', 'pub'),
    'campus': ('cafe', 'shop', 'diner'), 'waterfront': ('cafe', 'corner_store', 'diner'), 'entertainment': ('pub', 'diner', 'cafe'),
    'residential': ('corner_store', 'cafe', 'pub'), 'park': ('cafe', 'corner_store'), 'queen_west': ('shop', 'cafe', 'pub', 'diner'),
    'danforth': ('diner', 'cafe', 'pub', 'corner_store'), 'little_italy': ('cafe', 'pub', 'diner'), 'leslieville': ('cafe', 'diner', 'pub', 'shop'),
    'junction': ('pub', 'shop', 'cafe'), 'high_park': ('cafe', 'corner_store'), 'hospital': ('cafe', 'corner_store'),
    'transit': ('cafe', 'corner_store'), 'yonge_dundas': ('shop', 'diner', 'cafe'),
}
NAMES = {
    'cafe': ('BLUE DOOR CAFE', 'MAPLE BEAN', 'CORNER CUP', 'LAKESIDE ROAST', 'STREETCAR CAFE', 'THE DAILY GRIND', 'PORCH LIGHT CAFE',
             'TWO SPOONS', 'NORTHERN BREW', 'RED BRICK CAFE', 'GOOD MORNING CO', 'THE KETTLE ROOM'),
    'corner_store': ('NEIGHBOUR MART', 'DAY & NIGHT MART', 'CORNER VARIETY', 'ALL-DAY GROCERY', 'TINY MART', 'BLOCK VARIETY',
                     'SUNNY FOOD MART', 'OPEN LATE MART'),
    'diner': ('LAKESHORE DINER', 'EGGS & CO', 'THE GRIDDLE', 'SILVER SPOON', 'NIGHT OWL DINER', 'SUNNYSIDE GRILL', 'TRACK 9 DINER'),
    'shop': ('SPIN RECORDS', 'DOG-EAR BOOKS', 'CRATE DIGGERS', 'PAPER MOON BOOKS', 'NEEDLE DROP', 'SECOND CHAPTER', 'GROOVE LOT'),
    'pub': ('THE OLD KETTLE', 'MAPLE TAP', 'THE DUCK & DRUM', 'THE LANTERN', 'FOX & FERRY', 'THE BRASS BELL', 'NORTH STAR PUB'),
    'office_lobby': ('GRID TOWER LOBBY', 'BAYVIEW PLAZA', 'KING TOWER LOBBY', 'MAPLE PLACE LOBBY', 'HARBOUR TOWER', 'STONE TOWER'),
    'chinatown_bakery': ('GOLDEN BUN BAKERY', 'JADE MOON BAKERY', 'LUCKY TART BAKERY', 'PEARL BBQ & BAKERY'),
}
DOORS_PER_SCENE = 9
DOOR_GAP = 140


def text_ok(s, n=18):
    assert len(s) <= n and set(s) <= FONT, f'bad text {s!r}'
    return s


class Texts:
    """Text entries of up to four lines (title, then three)."""
    def __init__(self):
        self.entries, self.index = [], {}

    def add(self, lines):
        lines = [text_ok(str(l).upper()) for l in lines][:4]
        lines += [''] * (4 - len(lines))
        key = tuple(lines)
        if key not in self.index:
            self.index[key] = len(self.entries)
            self.entries.append(key)
        return self.index[key]


def looks():
    data = json.loads((ROOT / 'content/people.json').read_text())
    items = data['looks'] if isinstance(data, dict) else data
    return sorted(items, key=lambda l: l['index'])


def role_looks(items, role):
    keys = ROLE_KEYS.get(role, ())
    found = []
    for sub in keys:
        for l in items:
            if sub in l['key'] and l['index'] not in found and l['behave'] not in ('OFFICER', 'THIEF', 'TOUGH', 'RIVAL', 'RAGER',
                                                                                    'GOOSE', 'SCURRY', 'PIGEON', 'DOG', 'SIT', 'SLEEP', 'BEG'):
                found.append(l['index'])
    if not found:
        found = [l['index'] for l in items if l['behave'] in ('INDOOR', 'WALK')][:4]
    while len(found) < 4:
        found.append(found[len(found) % max(1, len(found))])
    return found[:4]


GALLERY_ROOMS = (
    ('AT WORK', lambda l: l['behave'] in ('INDOOR', 'OFFICER') or any(k in l['key'] for k in (
        'office', 'chef', 'construction', 'nurse', 'paramedic', 'firefighter', 'ttc', 'letter', 'carrier', 'vendor', 'clerk')),),
    ('ON THE STREET', lambda l: l['behave'] in ('BUSK', 'SIGN', 'SIT', 'SLEEP', 'BEG', 'JOG', 'SKATE', 'SLOW', 'PHOTO') or
     any(k in l['key'] for k in ('artist', 'preacher', 'protester', 'busker', 'skate', 'roller'))),
    ('OUT AND ABOUT', lambda l: l['behave'] == 'WALK'),
    ('TROUBLE', lambda l: l['behave'] in ('THIEF', 'TOUGH', 'RIVAL', 'RAGER')),
    ('CITY ANIMALS', lambda l: l['behave'] in ('GOOSE', 'SCURRY', 'PIGEON', 'DOG')),
)


def gallery(items, texts):
    left = list(items)
    rooms = []
    for title, test in GALLERY_ROOMS:
        group = [l for l in left if test(l)]
        left = [l for l in left if l not in group]
        chunks = [group[i:i + 8] for i in range(0, len(group), 8)] or []
        for n, chunk in enumerate(chunks):
            name = title if len(chunks) == 1 else f'{title} {"I" * (n + 1) if n < 3 else n + 1}'
            rooms.append((texts.add([name]), [l['index'] for l in chunk] + [255] * (8 - len(chunk))))
    for i in range(0, len(left), 8):
        chunk = left[i:i + 8]
        rooms.append((texts.add(['MORE FACES']), [l['index'] for l in chunk] + [255] * (8 - len(chunk))))
    return rooms


def interiors():
    data = json.loads((ROOT / 'content/interiors.json').read_text())
    return data['interiors']


def walkable(grid, x, y):
    tx, ty = x // 8, y // 8
    if not (0 <= tx < world2x.WORLD_TW and 0 <= ty < world2x.WORLD_TH):
        return False
    return grid[ty * world2x.WORLD_TW + tx] == 16


def door_point(grid, x, y, w, h, north=False):
    """Sidewalk point in front of a footprint's south (or north) door, else
    the nearest sidewalk just outside the footprint (world pixels)."""
    cx = x + w // 2
    for dy in (4, 8, 12):
        if north and walkable(grid, cx, y - dy):
            return cx, y - dy
        if not north and walkable(grid, cx, y + h + dy):
            return cx, y + h + dy
    best = None
    for px in range(x - 16, x + w + 17, 4):
        for py in range(y - 16, y + h + 17, 4):
            inside = x - 2 <= px < x + w + 2 and y - 2 <= py < y + h + 2
            if inside or not walkable(grid, px, py):
                continue
            d = abs(px - cx) + abs(py - (y + h))
            if best is None or d < best[0]:
                best = (d, px, py)
    return (best[1], best[2]) if best else None


def area_tag(new_id, u, v, places):
    slug = world2x.scene_slug(new_id).removeprefix('toronto_')
    for a in places[slug]['areas']:
        r = a['rect']
        if r[0] <= u < r[2] and r[1] <= v < r[3]:
            return AREA_TAG.get(a['name'], 'downtown' if new_id < 4 else 'residential'), a['name']
    return ('downtown' if new_id < 4 else 'residential'), ''


def blocks_of(old_id):
    if old_id == 0:
        data = json.loads((ROOT / 'content/city_art.json').read_text())
        blocks = list(data['blocks'])
        for d in data['districts']:
            if d.get('kind') == 'landmark' and d['name'] == 'CN Tower':
                x, y, w, h = d['rect']
                blocks.append({'x': x, 'y': y + 32, 'width': w, 'depth': 16, 'landmark': 'CN Tower'})
        return blocks
    slug = {1: 'west', 2: 'high_park', 3: 'east'}[old_id]
    return json.loads((ROOT / f'content/districts/{slug}_art.json').read_text())['blocks']


def doors(keys, texts):
    places = json.loads((ROOT / 'content/places.json').read_text())['scenes']
    out = {d: [] for d in range(16)}
    named = {}
    for old_id in range(4):
        grid = world2x.world_grid(old_id)
        blocks = blocks_of(old_id)
        # Landmarks first.
        for b in blocks:
            lm = b.get('landmark')
            if lm not in LANDMARKS or LANDMARKS[lm][0] not in keys:
                continue
            p = door_point(grid, b['x'], b['y'], b['width'], b['depth'], lm in NORTH_DOORS)
            assert p, f'no door for {lm}'
            new_id, u, v = world2x.scene_of(old_id, *p)
            key, name = LANDMARKS[lm]
            if any(d['interior'] == key and d['name'] == name for s in out.values() for d in s):
                continue
            out[new_id].append({'u': u, 'v': v, 'interior': key, 'name': name, 'landmark': lm})
        # Then shops, spread out, in commercial blocks first.
        rng = random.Random(f'toronto-dispatch doors {old_id}')
        cands = []
        for b in blocks:
            if b.get('landmark') or b['width'] < 24 or b['depth'] < 24:
                continue
            p = door_point(grid, b['x'], b['y'], b['width'], b['depth'])
            if not p or p[1] < b['y'] + b['depth']:
                continue
            new_id, u, v = world2x.scene_of(old_id, *p)
            if not (24 <= u < world2x.SCENE_W - 24 and 24 <= v < world2x.SCENE_H - 24):
                continue
            tag, area = area_tag(new_id, u, v, places)
            # Chinatown's shopfronts first (closer together), then the
            # commercial streets, then the quiet ones.
            rank = 0 if tag == 'chinatown' else 1 if tag not in ('residential', 'park', 'high_park') else 2
            cands.append((rank, rng.random(), new_id, u, v, tag, area))
        cands.sort()
        for rank, _, new_id, u, v, tag, area in cands:
            scene = out[new_id]
            if len(scene) >= DOORS_PER_SCENE:
                continue
            if rank == 0 and sum(1 for d in scene if d.get('tag') == 'chinatown') >= 3:
                continue
            gap = 64 if rank == 0 else DOOR_GAP
            if any(abs(d['u'] - u) + abs(d['v'] - v) < gap for d in scene):
                continue
            options = [t for t in TEMPLATES.get(tag, TEMPLATES['downtown']) if t in keys]
            if not options:
                continue
            kind = options[len(scene) % len(options)]
            # The next name of that kind not already used in this scene.
            used = {d['name'] for d in scene}
            n = named.get(kind, 0)
            for step in range(len(NAMES[kind])):
                name = NAMES[kind][(n + step) % len(NAMES[kind])]
                if name not in used:
                    break
            named[kind] = n + step + 1
            scene.append({'u': u, 'v': v, 'interior': kind, 'name': name, 'area': area, 'tag': tag})
    return out


def build():
    items = looks()
    ins = interiors()
    keys = [i['key'] for i in ins]
    for k in ('cn_base', 'cn_lookout', 'gallery_hall', 'gallery_art'):
        assert k in keys, f'interior {k} is missing'
    texts = Texts()
    names, kinds, entry, exit_ = [], [], [], []
    pt_first, pts, np_first, nps = [], [], [], []
    lifts = {}
    for k, i in enumerate(ins):
        names.append(texts.add([i['name']]))
        kinds.append(KINDS.index(i['kind']))
        entry.append(i['entrance'])
        # Exits are inclusive rectangles in the art data; the engine's are half open.
        x0, y0, x1, y1 = i['exit']
        exit_.append([x0, y0, x1 + 1, y1 + 1])
        pt_first.append(len(pts))
        for n, p in enumerate(i['points']):
            kind = POINT_KINDS.index(p['kind'])
            if p['kind'] == 'elevator' and i['key'] not in lifts:
                lifts[i['key']] = n
            text = texts.add(p['text']) if p.get('text') else 255
            plinth = p.get('plinth', [p['x'], p['y']])
            pts.append((kind, p['x'], p['y'], text, plinth[0], plinth[1]))
        np_first.append(len(nps))
        spots = i.get('npc_spots', [])[:MAX_SPOTS]
        for s in spots:
            behave = BEHAVES.index(s['behave'])
            area = s.get('area') or [s['x'] - 16, s['y'] - 8, s['x'] + 16, s['y'] + 8]
            leader = s.get('leader', 255)
            nps.append((role_looks(items, s['role']), behave, s['x'], s['y'], FACE.get(s.get('face', 's'), 2),
                        area, 255 if leader is None else leader))
    pt_first.append(len(pts))
    np_first.append(len(nps))
    rooms = gallery(items, texts)
    door_table = doors(set(keys), texts)
    flat = []
    first = [0]
    for d in range(16):
        for door in door_table[d]:
            flat.append((door['u'], door['v'], keys.index(door['interior']), texts.add([door['name']])))
        first.append(len(flat))

    h = ['/* Generated by scripts/create_interior_data.py from content/interiors.json and',
         ' * content/people.json: interiors, their points of interest and people. */',
         '#ifndef TD_INTERIOR_DATA_H', '#define TD_INTERIOR_DATA_H', f'#define TD_INTERIORS {len(ins)}']
    h += [f'#define TD_IN_{i["key"].upper()} {k}' for k, i in enumerate(ins)]
    h += [f'#define TD_IK_{n.upper()} {k}' for k, n in enumerate(KINDS)]
    h += [f'#define TD_IP_{n.upper()} {k}' for k, n in enumerate(POINT_KINDS)]
    h += [f'#define TD_NB_{n.upper()} {k}' for k, n in enumerate(BEHAVES)]
    h += [f'#define TD_IN_BASE_LIFT {lifts.get("cn_base", 0)}', f'#define TD_IN_LOOKOUT_LIFT {lifts.get("cn_lookout", 0)}',
          f'#define TD_GALLERY_ROOMS {len(rooms)}', f'#define TD_IN_POINTS {len(pts)}', f'#define TD_IN_SPOTS {len(nps)}',
          '/* Text entry id, line 0 (title) to 3, into dest (19 bytes). */',
          'void td_interior_text(UBYTE id,UBYTE line,char *dest) BANKED;',
          '#ifdef TD_INTERIOR_DATA']
    c = lambda name, ctype, vals: f'static const {ctype} {name}[{len(vals)}]={{' + ','.join(str(v) for v in vals) + '};'
    h.append(c('td_in_kind', 'UBYTE', kinds))
    h.append(c('td_in_name', 'UBYTE', names))
    h.append('static const UWORD td_in_entry[TD_INTERIORS][2]={' + ','.join('{%d,%d}' % tuple(e) for e in entry) + '};')
    h.append('static const UWORD td_in_exit[TD_INTERIORS][4]={' + ','.join('{%d,%d,%d,%d}' % tuple(e) for e in exit_) + '};')
    h.append(c('td_in_pt_first', 'UBYTE', pt_first))
    h.append(c('td_in_pt_kind', 'UBYTE', [p[0] for p in pts]))
    h.append(c('td_in_pt_x', 'UWORD', [p[1] for p in pts]))
    h.append(c('td_in_pt_y', 'UWORD', [p[2] for p in pts]))
    h.append(c('td_in_pt_text', 'UBYTE', [p[3] for p in pts]))
    h.append(c('td_in_pt_px', 'UWORD', [p[4] for p in pts]))
    h.append(c('td_in_pt_py', 'UWORD', [p[5] for p in pts]))
    h.append(c('td_in_np_first', 'UBYTE', np_first))
    h.append('static const UBYTE td_in_np_look[%d][4]={' % max(1, len(nps)) + ','.join('{%s}' % ','.join(map(str, s[0])) for s in nps) + '};')
    h.append(c('td_in_np_behave', 'UBYTE', [s[1] for s in nps]))
    h.append(c('td_in_np_x', 'UWORD', [s[2] for s in nps]))
    h.append(c('td_in_np_y', 'UWORD', [s[3] for s in nps]))
    h.append(c('td_in_np_face', 'UBYTE', [s[4] for s in nps]))
    h.append('static const UWORD td_in_np_area[%d][4]={' % max(1, len(nps)) + ','.join('{%d,%d,%d,%d}' % tuple(s[5]) for s in nps) + '};')
    h.append(c('td_in_np_leader', 'UBYTE', [s[6] for s in nps]))
    h.append(c('td_gallery_title', 'UBYTE', [r[0] for r in rooms]))
    h.append('static const UBYTE td_gallery_looks[TD_GALLERY_ROOMS][8]={' + ','.join('{%s}' % ','.join(map(str, r[1])) for r in rooms) + '};')
    h += ['#endif', '#endif', '']

    sc = ['/* Generated by scripts/create_interior_data.py: interior scene pointers. */',
          '#ifndef TD_INTERIOR_SCENES_H', '#define TD_INTERIOR_SCENES_H']
    sc += [f'#include "data/scene_{i["slug"]}.h"' for i in ins]
    sc.append('static const far_ptr_t td_in_scene[TD_INTERIORS]={' + ','.join(f'TO_FAR_PTR_T(scene_{i["slug"]})' for i in ins) + '};')
    sc += ['#endif', '']

    dh = ['/* Generated by scripts/create_interior_data.py: doors into interiors, per scene',
          ' * (whole pixels of the sidewalk in front of each door). */',
          '#ifndef TD_DOORS_H', '#define TD_DOORS_H', f'#define TD_DOORS {len(flat)}', '#ifdef TD_DOORS_DATA']
    dh.append(c('td_door_first', 'UBYTE', first))
    dh.append(c('td_door_u', 'UWORD', [d[0] for d in flat]))
    dh.append(c('td_door_v', 'UWORD', [d[1] for d in flat]))
    dh.append(c('td_door_in', 'UBYTE', [d[2] for d in flat]))
    dh.append(c('td_door_text', 'UBYTE', [d[3] for d in flat]))
    dh += ['#endif', '#endif', '']
    assert len(flat) < 255 and len(texts.entries) < 255 and len(pts) < 255 and len(nps) < 255

    t = ['#pragma bank 255', '/* Generated by scripts/create_interior_data.py: interior and door text. */',
         '#include <gbdk/platform.h>', '#include <string.h>', '#include "td_interior_data.h"',
         f'static const char td_in_text[{len(texts.entries)}][4][19]={{']
    for e in texts.entries:
        t.append('    {' + ','.join(json.dumps(l) for l in e) + '},')
    t += ['};', 'void td_interior_text(UBYTE id,UBYTE line,char *dest) BANKED {',
          f'    if(id>={len(texts.entries)}||line>3){{*dest=0;return;}}',
          '    strcpy(dest,td_in_text[id][line]);', '}', '']

    review = {'about': 'Doors into interiors per scene (generated by scripts/create_interior_data.py).',
              'scenes': {world2x.scene_slug(d): door_table[d] for d in range(16)}}
    return {OUT_DATA: '\n'.join(h), OUT_SCENES: '\n'.join(sc), OUT_DOORS: '\n'.join(dh), OUT_TEXT: '\n'.join(t),
            OUT_JSON: json.dumps(review, indent=1) + '\n'}, len(flat), len(ins), len(rooms)


def main():
    files, ndoors, nins, nrooms = build()
    if '--check' in sys.argv:
        for path, text in files.items():
            assert path.exists() and path.read_text() == text, f'stale: {path.relative_to(ROOT)}; run scripts/create_interior_data.py'
        print(f'Interior tables match: {nins} interiors, {ndoors} doors, {nrooms} gallery rooms.')
        return
    for path, text in files.items():
        path.write_text(text)
    print(f'Wrote interior tables: {nins} interiors, {ndoors} doors, {nrooms} gallery rooms.')


if __name__ == '__main__':
    main()
