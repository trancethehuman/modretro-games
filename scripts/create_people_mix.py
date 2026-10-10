"""Generate which people walk where (engine/include/td_people_mix.h).

Every scene is cut into 128-pixel cells; each cell takes the crowd of the
neighbourhood it lies in (the HUD neighbourhoods of content/places.json,
named after the City of Toronto neighbourhoods and BIAs). A crowd is a
32-entry table of looks (content/people.json), filled in proportion to each
look's weight for that kind of place: suits and couriers in the Financial
District, students at the university, grocers and elders in Chinatown,
joggers and geese by the lake. After dark one walker in four comes from the
night crowd instead. Officers (one route in eight) are chosen by the engine.

`--check` verifies the header without writing.
"""
from pathlib import Path
import json
import random
import sys
import world2x

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'project/plugins/toronto-driving/engine/include/td_people_mix.h'
TAGS = ('downtown', 'financial', 'chinatown', 'kensington', 'campus', 'waterfront', 'entertainment', 'residential',
        'park', 'queen_west', 'danforth', 'little_italy', 'leslieville', 'junction', 'high_park', 'hospital',
        'transit', 'yonge_dundas')
# HUD neighbourhood -> kind of crowd.
AREA_TAG = {
    'UNIV OF TORONTO': 'campus', 'HARBORD VILLAGE': 'residential', 'GRANGE PARK': 'downtown', 'CHINATOWN': 'chinatown',
    'KENSINGTON MARKET': 'kensington', 'ALEXANDRA PARK': 'residential', 'TRINITY BELLWOODS': 'queen_west',
    'LITTLE PORTUGAL': 'little_italy', 'LITTLE ITALY': 'little_italy', 'DUFFERIN GROVE': 'park', 'PALMERSTON': 'residential',
    'DOWNTOWN': 'downtown', 'QUEENS PARK': 'park', 'RIVERDALE': 'residential', 'REGENT PARK': 'residential',
    'CABBAGETOWN': 'residential', 'MOSS PARK': 'downtown', 'GARDEN DISTRICT': 'downtown', 'YONGE-DUNDAS': 'yonge_dundas',
    'CHURCH-WELLESLEY': 'downtown', 'BLOOR-YONGE': 'downtown', 'BAY ST CORRIDOR': 'financial',
    'DISCOVERY DISTRICT': 'hospital', 'HANLANS POINT': 'waterfront', 'CENTRE ISLAND': 'waterfront',
    'WARDS ISLAND': 'waterfront', 'INNER HARBOUR': 'waterfront', 'HARBOURFRONT': 'waterfront', 'WATERFRONT': 'waterfront',
    'EAST BAYFRONT': 'waterfront', 'FORT YORK': 'park', 'ENTERTAINMENT DIST': 'entertainment', 'CITYPLACE': 'residential',
    'KING WEST': 'queen_west', 'LIBERTY VILLAGE': 'queen_west', 'WEST QUEEN WEST': 'queen_west',
    'SOUTH RIVERDALE': 'leslieville', 'DISTILLERY DISTRICT': 'entertainment', 'CORKTOWN': 'residential',
    'ST LAWRENCE': 'downtown', 'OLD TOWN': 'downtown', 'FINANCIAL DISTRICT': 'financial',
    'JUNCTION TRIANGLE': 'junction', 'HIGH PARK NORTH': 'residential', 'RONCESVALLES': 'residential',
    'BLOORDALE': 'residential', 'BROCKTON VILLAGE': 'residential', 'SUNNYSIDE': 'waterfront', 'PARKDALE': 'queen_west',
    'THE JUNCTION': 'junction', 'BLOOR WEST VILLAGE': 'residential', 'SWANSEA': 'residential', 'HIGH PARK': 'high_park',
    'GREEKTOWN': 'danforth', 'THE DANFORTH': 'danforth', 'CHINATOWN EAST': 'chinatown', 'RIVERSIDE': 'leslieville',
    'LESLIEVILLE': 'leslieville',
}
CELL = 128
COLS, ROWS = 8, 8
ENTRIES = 32
OUTDOOR_EXCLUDE = ('INDOOR', 'OFFICER')


def looks():
    data = json.loads((ROOT / 'content/people.json').read_text())
    items = data['looks'] if isinstance(data, dict) else data
    return sorted(items, key=lambda l: l['index'])


BLEND = 0.15


def table(items, tag, seed):
    """32 look indices in proportion to their weights for tag (largest
    remainder), shuffled so a route identity's hash spreads over them. Every
    crowd but the night's takes a little of the general foot traffic
    (downtown and residential), so no street is all one kind of person."""
    def weight(l):
        w = l.get('where', {})
        base = w.get(tag, 0)
        if tag == 'night':
            return base
        return base + BLEND * (w.get('downtown', 0) + w.get('residential', 0))
    weights = [(l['index'], weight(l)) for l in items if l['behave'] not in OUTDOOR_EXCLUDE]
    weights = [(i, w) for i, w in weights if w > 0]
    if not weights:
        return None
    total = sum(w for _, w in weights)
    shares = [(i, w * ENTRIES / total) for i, w in weights]
    counts = {i: int(s) for i, s in shares}
    left = ENTRIES - sum(counts.values())
    for i, s in sorted(shares, key=lambda x: (-(x[1] - int(x[1])), x[0]))[:left]:
        counts[i] += 1
    out = [i for i, c in sorted(counts.items()) for _ in range(c)]
    random.Random(f'toronto-dispatch crowd {seed}').shuffle(out)
    assert len(out) == ENTRIES
    return out


def grid():
    places = json.loads((ROOT / 'content/places.json').read_text())['scenes']
    out, unknown = [], set()
    for d in range(16):
        slug = world2x.scene_slug(d).removeprefix('toronto_')
        areas = places[slug]['areas']
        cells = []
        for r in range(ROWS):
            for c in range(COLS):
                cx, cy = c * CELL + CELL // 2, r * CELL + CELL // 2
                name = next((a['name'] for a in areas if a['rect'][0] <= cx < a['rect'][2] and a['rect'][1] <= cy < a['rect'][3]), None)
                if name is None:
                    # Nearest area by its rectangle's centre.
                    name = min(areas, key=lambda a: abs((a['rect'][0] + a['rect'][2]) / 2 - cx) + abs((a['rect'][1] + a['rect'][3]) / 2 - cy))['name']
                if name not in AREA_TAG:
                    unknown.add(name)
                cells.append(AREA_TAG.get(name, 'downtown' if d < 4 else 'residential'))
        out.append(cells)
    assert not unknown, f'neighbourhoods without a crowd: {sorted(unknown)}'
    return out


def build():
    items = looks()
    tables, used = {}, []
    for t in TAGS:
        tab = table(items, t, t)
        if tab:
            tables[t] = tab
    assert 'downtown' in tables, 'the downtown crowd needs looks'
    night = table(items, 'night', 'night') or tables['downtown']
    cells = grid()
    for scene in cells:
        for t in scene:
            if t not in tables:
                t = 'downtown'
            if t not in used:
                used.append(t)
    mix_id = {t: i for i, t in enumerate(used)}
    lines = ['/* Generated by scripts/create_people_mix.py from content/people.json and',
             ' * content/places.json: the crowd of each 128-pixel cell of every scene. */',
             '#ifndef TD_PEOPLE_MIX_H', '#define TD_PEOPLE_MIX_H', f'#define TD_MIXES {len(used)}',
             '#ifdef TD_PEOPLE_MIX_DATA']
    lines.append('static const UBYTE td_mix_grid[16][64]={')
    for d, scene in enumerate(cells):
        ids = [mix_id[t if t in tables else 'downtown'] for t in scene]
        lines.append('    {' + ','.join(map(str, ids)) + '},')
    lines.append('};')
    lines.append(f'static const UBYTE td_mix_looks[TD_MIXES][{ENTRIES}]={{')
    for t in used:
        lines.append('    {' + ','.join(map(str, tables[t])) + '}, /* ' + t + ' */')
    lines.append('};')
    lines.append(f'static const UBYTE td_mix_night[{ENTRIES}]={{' + ','.join(map(str, night)) + '};')
    lines += ['#endif', '#endif', '']
    return '\n'.join(lines), used


def main():
    text, used = build()
    if '--check' in sys.argv:
        assert OUT.read_text() == text, 'people mix is stale; run scripts/create_people_mix.py'
        print(f'People mix matches the neighbourhoods: {len(used)} crowds.')
        return
    OUT.write_text(text)
    print(f'Wrote {OUT.relative_to(ROOT)}: {len(used)} crowds ({", ".join(used)}).')


if __name__ == '__main__':
    main()
