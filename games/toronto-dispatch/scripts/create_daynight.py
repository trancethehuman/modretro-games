"""Generate the day/night palette tables from the scene palettes.

The seven background palettes and eight sprite palettes registered on the
Toronto scenes are the daytime look. Golden hour, dusk, night and dawn are
derived from them (night background colours are chosen by hand: navy roads,
sodium-lit sidewalks, dimmed facades) and interpolated across a 64-step day.
The UI palette (BG slot 7) is never tinted, so the HUD and menus keep their
colours.

Writes:
  engine/include/td_daynight_data.h   step -> palette-set map, unique RGB555
                                      background and sprite palette sets
`--check` verifies the output without writing.
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'project'
ENGINE = PROJECT / 'plugins/toronto-driving/engine'
OUT = ENGINE / 'include/td_daynight_data.h'

DAY_SECONDS = 1024          # one game day per 1024 play seconds (about 17 minutes)
STEPS = 64                  # a palette step every 16 seconds (22.5 game minutes)
START_HOUR = 8              # a new game starts at 08:00
LIGHTS = (19.0, 6.5)        # vehicle headlamps from 19:00 until 06:30
MAX_SETS = 40

# Night background colour for each daytime colour. Lifted on 2026-10-10 so
# streets, people and buildings stay readable on the handheld at night: navy
# roads, warm sodium-lit sidewalks, facades at about two thirds.
NIGHT_BG = {
    'E7DECC': 'B4A490',  # paper: sidewalks and markings under sodium lamps
    '526879': '36445F',  # slate roads: navy
    '172B38': '101828',  # ink
    '79AFAC': '2F6680',  # lake and stone
    'D69E72': 'B07258', '8BBAD4': '5C82AE', 'C5A9A1': '8E7090',
    'D2C895': 'A49468', '9FC9D0': '6894AC',  # facades
    '8FB56A': '4A6E50', '4D7A52': '2A4A3E',  # parks
}
# Sprites dim less than the city so traffic and people stay readable; the
# beacon/pickup/taxi yellow never dims. Player palettes dim least.
SPRITE_STRENGTH = {'courier_vehicle': 0.3, 'courier_person': 0.35, 'signal_yellow': 0.0,
                   'police': 0.8}
# (hour, keyframe) along the day; 29 = 05:00 the next morning.
TIMELINE = ((4.5, 'night'), (5.75, 'dawn'), (7.25, 'day'), (17.5, 'day'), (19.0, 'golden'),
            (20.25, 'dusk'), (21.5, 'night'), (28.5, 'night'))


def rgb(h):
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def clamp(c):
    return tuple(max(0.0, min(255.0, v)) for v in c)


def mix(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))


def tint(c, mul, add=(0, 0, 0)):
    return clamp(tuple(v * m + d for v, m, d in zip(c, mul, add)))


def luma(c):
    return (0.3 * c[0] + 0.59 * c[1] + 0.11 * c[2]) / 255


def golden(c):
    """Warm highlights, cool shadows."""
    L = luma(c)
    mul = mix((0.86, 0.86, 1.0), (1.05, 0.86, 0.70), L)
    return tint(c, mul, (6, 0, 0))


def dusk(c, night):
    return mix(tint(c, (0.90, 0.66, 0.82), (6, 0, 16)), night, 0.40)


def dawn(c, night):
    return mix(tint(c, (0.92, 0.84, 0.96), (10, 2, 14)), night, 0.30)


def sprite_night(c, role):
    if role == 'light':
        return tint(c, (0.82, 0.82, 0.86), (0, 0, 6))
    if role == 'body':
        return tint(c, (0.62, 0.64, 0.80), (6, 8, 24))
    return mix(c, rgb('0C1222'), 0.4)


def palettes():
    by_id = {}
    for f in (PROJECT / 'project/palettes').glob('*.gbsres'):
        d = json.loads(f.read_text())
        by_id[d['id']] = d
    scenes = sorted((PROJECT / 'project/scenes').glob('toronto_*/scene.gbsres'))
    assert scenes, 'no Toronto scenes'
    first = json.loads(scenes[0].read_text())
    for s in scenes[1:]:
        d = json.loads(s.read_text())
        assert d['paletteIds'] == first['paletteIds'] and d['spritePaletteIds'] == first['spritePaletteIds'], \
            f'{s.parent.name} must share the Toronto palettes'
    bkg = [by_id[i]['colors'] for i in first['paletteIds'][:7]]
    spr = [(by_id[i]['name'], by_id[i]['colors']) for i in first['spritePaletteIds']]
    assert len(bkg) == 7 and len(spr) == 8
    return bkg, spr


def sprite_key(name):
    return {'Courier vehicle': 'courier_vehicle', 'Courier uniform': 'courier_person',
            'Taxi and beacon yellow': 'signal_yellow', 'Police navy': 'police'}.get(name, name)


def keyframes(bkg, spr):
    """keyframe -> (7 background palettes, 8 sprite palettes) of float RGB."""
    missing = {c for p in bkg for c in p} - set(NIGHT_BG)
    assert not missing, f'no night colour for {sorted(missing)}'
    day_b = [[rgb(c) for c in p] for p in bkg]
    night_b = [[rgb(NIGHT_BG[c]) for c in p] for p in bkg]
    out = {
        'day': day_b,
        'night': night_b,
        'golden': [[golden(c) for c in p] for p in day_b],
        'dusk': [[dusk(c, n) for c, n in zip(p, q)] for p, q in zip(day_b, night_b)],
        'dawn': [[dawn(c, n) for c, n in zip(p, q)] for p, q in zip(day_b, night_b)],
    }
    sprites = {k: [] for k in out}
    for name, colours in spr:
        strength = SPRITE_STRENGTH.get(sprite_key(name), 1.0)
        # Hardware order: transparent, light, mid, dark from resource [0, 0, 1, 3].
        day = [rgb(colours[0]), rgb(colours[0]), rgb(colours[1]), rgb(colours[3])]
        roles = ('light', 'light', 'body', 'dark')
        night = [mix(c, sprite_night(c, r), strength) for c, r in zip(day, roles)]
        soft = 0.6 * strength
        sprites['day'].append(day)
        sprites['night'].append(night)
        sprites['golden'].append([mix(c, golden(c), soft) for c in day])
        sprites['dusk'].append([mix(c, dusk(c, n), soft) for c, n in zip(day, night)])
        sprites['dawn'].append([mix(c, dawn(c, n), soft) for c, n in zip(day, night)])
    return {k: (out[k], sprites[k]) for k in out}


def at_hour(keys, hour):
    if hour < TIMELINE[0][0]:
        hour += 24
    for (h0, a), (h1, b) in zip(TIMELINE, TIMELINE[1:]):
        if h0 <= hour <= h1:
            t = (hour - h0) / (h1 - h0)
            return tuple([[mix(x, y, t) for x, y in zip(p, q)] for p, q in zip(ka, kb)]
                         for ka, kb in zip(keys[a], keys[b]))
    raise AssertionError(hour)


def rgb15(c):
    r, g, b = (int(round(v)) >> 3 for v in c)
    return r | g << 5 | b << 10


def quantise(sets):
    bkg, spr = sets
    return (tuple(rgb15(c) for p in bkg for c in p), tuple(rgb15(c) for p in spr for c in p))


def build():
    bkg, spr = palettes()
    keys = keyframes(bkg, spr)
    sets, index, step_set = [], {}, []
    for step in range(STEPS):
        q = quantise(at_hour(keys, step * 24 / STEPS))
        if q not in index:
            index[q] = len(sets)
            sets.append(q)
        step_set.append(index[q])
    day = quantise((keys['day'][0], keys['day'][1]))
    night = quantise((keys['night'][0], keys['night'][1]))
    assert day in index and night in index, 'the day and night looks must both be reached'
    # The day set reproduces the registered scene palettes exactly.
    assert day[0] == tuple(rgb15(rgb(c)) for p in bkg for c in p)
    assert len(sets) <= MAX_SETS, f'{len(sets)} palette sets exceed the {MAX_SETS}-set budget'
    # Night readability: roads (slate) stay clearly darker than sidewalks (paper).
    paper, slate = rgb(NIGHT_BG['E7DECC']), rgb(NIGHT_BG['526879'])
    assert luma(paper) - luma(slate) > 0.25, 'night roads and sidewalks need contrast'
    start = round(START_HOUR * DAY_SECONDS / 24 + 0.5)
    on, off = (round(h * DAY_SECONDS / 24) for h in LIGHTS)

    def table(rows):
        return ',\n    '.join('{' + ','.join(f'0x{v:04X}' for v in row) + '}' for row in rows)
    text = (
        '/* Generated by scripts/create_daynight.py from the Toronto scene palettes. */\n'
        '#ifndef TD_DAYNIGHT_DATA_H\n#define TD_DAYNIGHT_DATA_H\n'
        f'#define TD_DN_DAY_SECONDS {DAY_SECONDS}\n'
        f'#define TD_DN_START {start} /* play second 0 is {START_HOUR:02d}:00 */\n'
        f'#define TD_DN_STEPS {STEPS}\n'
        f'#define TD_DN_SETS {len(sets)}\n'
        f'#define TD_DN_DAY_SET {index[day]}\n'
        f'#define TD_DN_NIGHT_SET {index[night]}\n'
        f'#define TD_DN_LIGHTS_ON {on} /* {LIGHTS[0]:g}h */\n'
        f'#define TD_DN_LIGHTS_OFF {off} /* {LIGHTS[1]:g}h */\n'
        'static const UBYTE td_dn_step_set[TD_DN_STEPS]={' + ','.join(map(str, step_set)) + '};\n'
        '/* Background palettes 0..6, four RGB555 colours each. */\n'
        'static const UWORD td_dn_bkg[TD_DN_SETS][28]={\n    ' + table(s[0] for s in sets) + '};\n'
        '/* Sprite palettes 0..7 (colour 0 is transparent). */\n'
        'static const UWORD td_dn_spr[TD_DN_SETS][32]={\n    ' + table(s[1] for s in sets) + '};\n'
        '#endif\n')
    return text, len(sets), step_set


def main():
    text, count, steps = build()
    if '--check' in sys.argv:
        if not OUT.exists() or OUT.read_text() != text:
            raise SystemExit(f'{OUT.relative_to(ROOT)} is stale; run scripts/create_daynight.py')
        print(f'Day/night palettes match the scene palettes: {count} sets over {len(steps)} steps.')
        return
    OUT.write_text(text)
    print(f'Wrote {OUT.relative_to(ROOT)}: {count} palette sets over {len(steps)} steps.')


if __name__ == '__main__':
    main()
