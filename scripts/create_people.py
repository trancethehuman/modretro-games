"""Generate the NPC look library: people and animals of Toronto as small
original pixel-art sprites, with the text the gallery and the talk prompt
show for each one.

Every look is eight 8x16 OBJ tiles: four poses (side walking east, front,
back, and the look's own action) of two frames each. The engine streams a
look's 256 bytes into hardware sprite tiles when it needs it, so the whole
library costs no fixed VRAM. Drawings follow the walkers in sprite_art.py
(colour 1 light/skin, 2 clothing, 3 hair/shoes/outline; figures about ten
pixels tall standing on row 13) and use one of the seven people palettes
(an offset from the courier's palette). Props and hats tell types apart.

Writes:
  engine/include/td_people_data.h   look ids, poses, behaviours, palettes
  engine/src/td_people_tiles.c      the tiles (one bank)
  engine/src/td_people_text.c       names, homes, traits and talk lines
  content/people.json               the full metadata
`--check` verifies all four without writing; `--preview <png>` writes a
labelled review sheet (not committed).
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'project/plugins/toronto-driving/engine'
OUT_H = ENGINE / 'include/td_people_data.h'
OUT_TILES = ENGINE / 'src/td_people_tiles.c'
OUT_TEXT = ENGINE / 'src/td_people_text.c'
OUT_JSON = ROOT / 'content/people.json'

FONT = set(" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/.-+?<>$%#=,'!()&")
LINE = 18
NAME = 16
LOOK_BYTES = 256
POSES = ('SIDE', 'FRONT', 'BACK', 'ACT')
BEHAVES = ('WALK', 'SLOW', 'JOG', 'SKATE', 'PHOTO', 'SIT', 'SLEEP', 'BUSK', 'SIGN', 'BEG',
           'THIEF', 'TOUGH', 'RIVAL', 'RAGER', 'GOOSE', 'SCURRY', 'PIGEON', 'DOG', 'OFFICER', 'INDOOR')
WHERE = ('downtown', 'financial', 'chinatown', 'kensington', 'campus', 'waterfront', 'entertainment',
         'residential', 'park', 'queen_west', 'danforth', 'little_italy', 'leslieville', 'junction',
         'high_park', 'hospital', 'transit', 'yonge_dundas', 'night')
ROLES = ('museum', 'gallery', 'mall', 'store', 'electronics', 'cafe', 'restaurant', 'bakery', 'office',
         'pub', 'transit_hall', 'tower')
# People palettes: offsets from the courier uniform palette (create_sprites.py
# PALETTES[1..7]); light, mid and dark colours for the preview.
PALETTES = [
    ('courier orange', ('F8C8A0', 'E87820', '182030')),
    ('red', ('F0D0B8', 'C83028', '101018')),
    ('blue', ('D0E8F8', '3068C8', '101018')),
    ('yellow', ('F8F8E0', 'F0C020', '282010')),
    ('navy', ('F0C090', '2850B0', '101828')),
    ('teal', ('F0C090', '309878', '282030')),
    ('violet', ('C89070', '8858B8', '302018')),
]
NAVY = 4
ORANGE = 0


# ---------------------------------------------------------------- drawing
def blank():
    return [[0] * 8 for _ in range(16)]


def put(g, top, left, rows):
    """Paint rows at (top, left): '.' keeps, '0' clears, '1'..'3' paint."""
    for dy, row in enumerate(rows):
        for dx, ch in enumerate(row):
            if ch == '.':
                continue
            y, x = top + dy, left + dx
            assert 0 <= y < 16 and 0 <= x < 8, ('outside the tile', top, left, rows)
            g[y][x] = int(ch)
    return g


def shift(g, dy=0, dx=0):
    out = blank()
    for y in range(16):
        for x in range(8):
            if g[y][x] and 0 <= y + dy < 16 and 0 <= x + dx < 8:
                out[y + dy][x + dx] = g[y][x]
    return out


def copy(g):
    return [row[:] for row in g]


def art(rows, top=None):
    """An explicit drawing: rows of 8 characters, placed so the last row is
    row 13 unless `top` is given."""
    assert all(len(r) == 8 for r in rows), ('drawing rows must be 8 wide', rows)
    g = blank()
    if top is None:
        top = 14 - len(rows)
    return put(g, top, 0, rows)


class Size:
    """Rows of a figure."""
    def __init__(self, chin, hand, legs, torso_rows, leg_rows):
        self.chin, self.torso, self.hand, self.legs = chin, chin + 1, hand, legs
        self.torso_rows, self.leg_rows = torso_rows, leg_rows


ADULT = Size(6, 8, 11, (0, 1, 2, 3), (0, 1, 2))

# Heads: view -> rows ending on the chin row. 'right' faces east; an
# optional 'nape' adds rows below the chin, over the torso.
HEADS = {
    'short': {'down': ["..3333..", "..3113..", "..3113.."],
              'up': ["..3333..", "..3333..", "..3333.."],
              'right': ["..333...", "..3311..", "..3311.."]},
    'long': {'down': [".333333.", ".331133.", ".331133."],
             'up': [".333333.", ".333333.", ".333333."],
             'right': [".3333...", ".33311..", ".33311.."]},
    'bun': {'down': ["...33...", "..3333..", "..3113..", "..3113.."],
            'up': ["...33...", "..3333..", "..3333..", "..3333.."],
            'right': [".33.....", "..333...", "..3311..", "..3311.."]},
    'tail': {'down': ["..3333..", "..3113..", "..3113.."],
             'up': ["..3333..", "..3333..", "..3333.."],
             'right': ["..333...", ".33311..", ".3.311.."],
             'nape': {'up': ["...33..."]}},   # the ponytail over the collar
    'curly': {'down': ["..3333..", ".333333.", ".331133.", "..3113.."],
              'up': ["..3333..", ".333333.", ".333333.", "..3333.."],
              'right': ["..333...", ".33333..", ".33311..", "..3311.."]},
    'grey': {'down': ["..3113..", ".311113.", "..3113..", "..3113.."],
             'up': ["..3113..", ".311113.", "..3113..", "..3333.."],
             'right': ["..311...", ".31113..", "..3111..", "..3311.."]},
    'cap': {'down': ["..2222..", ".322223.", "..3113..", "..3113.."],
            'up': ["..2222..", "..2222..", "..3333..", "..3333.."],
            'right': ["..222...", "..22222.", "..3311..", "..3311.."]},
    'darkcap': {'down': ["..3333..", ".333333.", "..3113..", "..3113.."],
                'up': ["..3333..", "..3333..", "..3333..", "..3333.."],
                'right': ["..333...", "..33333.", "..3311..", "..3311.."]},
    'backcap': {'down': ["..2222..", "..3113..", "..3113.."],
                'up': ["..2222..", ".322223.", "..3333..", "..3333.."],
                'right': ["..222...", "22222...", "..3311..", "..3311.."]},
    'sunhat': {'down': ["..3223..", "32222223", "..3113..", "..3113.."],
               'up': ["..3223..", "32222223", "..3333..", "..3333.."],
               'right': ["..322...", "3222222.", "..3311..", "..3311.."]},
    'hardhat': {'down': ["..3223..", ".322223.", "33333333", "..3113..", "..3113.."],
                'up': ["..3223..", ".322223.", "33333333", "..3333..", "..3333.."],
                'right': ["..322...", ".32222..", "3333333.", "..3311..", "..3311.."]},
    'toque': {'down': [".311113.", ".311113.", "..3113..", "..3333..", "..3113..", "..3113.."],
              'up': [".311113.", ".311113.", "..3113..", "..3333..", "..3333..", "..3333.."],
              'right': [".31113..", ".31113..", "..3113..", "..3333..", "..3311..", "..3311.."]},
    'beret': {'down': ["..222...", ".22223..", "..3113..", "..3113.."],
              'up': ["..222...", ".22223..", "..3333..", "..3333.."],
              'right': ["..222...", ".22223..", "..3311..", "..3311.."]},
    'hood': {'down': ["..3223..", ".321123.", ".321123."],
             'up': ["..3223..", ".322223.", ".322223."],
             'right': ["..3222..", ".32211..", ".32211.."]},
    'headband': {'down': ["..3333..", "..2222..", "..3113..", "..3113.."],
                 'up': ["..3333..", "..2222..", "..3333..", "..3333.."],
                 'right': ["..333...", "..2222..", "..3311..", "..3311.."]},
    'helmet': {'down': ["..2332..", ".322223.", "..3113..", "..3113.."],
               'up': ["..2332..", ".322223.", "..3333..", "..3333.."],
               'right': [".2332...", ".322223.", "..3311..", "..3311.."]},
    'peaked': {'down': ["..2222..", ".321223.", ".333333.", "..3113..", "..3113.."],
               'up': ["..2222..", ".322223.", "..3333..", "..3333..", "..3333.."],
               'right': ["..2222..", ".322213.", "..33333.", "..3311..", "..3311.."]},
    'paperhat': {'down': ["..3113..", ".311113.", "..3113..", "..3113.."],
                 'up': ["..3113..", ".311113.", "..3333..", "..3333.."],
                 'right': ["..311...", ".31113..", "..3311..", "..3311.."]},
    'phones': {'down': ["..3333..", ".2.33.2.", ".231132.", "..3113.."],
               'up': ["..3333..", ".2.33.2.", ".233332.", "..3333.."],
               'right': ["..3332..", "..33.2..", "..3231..", "..3311.."]},
    'firehelm': {'down': ["..2222..", ".322223.", "33333333", "..3113..", "..3113.."],
                 'up': ["..2222..", ".322223.", "33333333", "..3333..", "..3333.."],
                 'right': ["..222...", ".32222..", "3333333.", "..3311..", "..3311.."]},
    'beanie': {'down': ["...33...", "..2222..", "..3113..", "..3113.."],
               'up': ["...33...", "..2222..", "..3333..", "..3333.."],
               'right': ["...3....", "..222...", "..3311..", "..3311.."]},
}
# Torsos: four rows (adult) from the shoulders to the hips.
TORSOS = {
    'shirt': {'down': [".322223.", ".122221.", ".322223.", "..3223.."],
              'up': [".322223.", ".122221.", ".322223.", "..3223.."],
              'right': ["..3223..", "..3213..", "..3223..", "..333..."]},
    'suit': {'down': [".321123.", ".123321.", ".323323.", "..3223.."],
             'up': [".322223.", ".122221.", ".322223.", "..3223.."],
             'right': ["..3213..", "..3213..", "..3223..", "..333..."]},
    'tee': {'down': [".322223.", ".122221.", ".133331.", "..3333.."],
            'up': [".322223.", ".122221.", ".133331.", "..3333.."],
            'right': ["..3223..", "..3213..", "..3333..", "..333..."]},
    'pack': {'down': [".332233.", ".132231.", ".322223.", "..3223.."],
             'up': [".333333.", ".133331.", ".333333.", "..3223.."],
             'right': [".33223..", ".33213..", ".33223..", "..333..."]},
    'longhair': {'down': [".332233.", ".122221.", ".322223.", "..3223.."],
                 'up': [".333333.", ".133331.", ".322223.", "..3223.."],
                 'right': [".33223..", "..3213..", "..3223..", "..333..."]},
    'hivis': {'down': [".322223.", ".111111.", ".322223.", "..3223.."],
              'up': [".322223.", ".111111.", ".322223.", "..3223.."],
              'right': ["..3223..", "..3111..", "..3223..", "..333..."]},
    'apron': {'down': [".332233.", ".132231.", ".322223.", "..3223.."],
              'up': [".333333.", ".133331.", ".323323.", "..3333.."],
              'right': ["..3322..", "..3312..", "..3322..", "..332..."]},
    'whites': {'down': [".311113.", ".112211.", ".311113.", "..3113.."],
               'up': [".311113.", ".111111.", ".311113.", "..3113.."],
               'right': ["..3112..", "..3111..", "..3111..", "..333..."]},
    'scrubs': {'down': [".321123.", ".122221.", ".322223.", "..3223.."],
               'up': [".322223.", ".122221.", ".322223.", "..3223.."],
               'right': ["..3221..", "..3213..", "..3223..", "..322..."]},
    'jersey': {'down': ["32222223", "31222213", "32111123", "..3223.."],
               'up': ["32222223", "31211213", "32111123", "..3223.."],
               'right': [".32223..", ".32213..", ".31113..", "..333..."]},
    'lanyard': {'down': [".321123.", ".122321.", ".322223.", "..3333.."],
                'up': [".322223.", ".122221.", ".322223.", "..3333.."],
                'right': ["..3213..", "..3213..", "..3223..", "..333..."]},
    'paint': {'down': [".322213.", ".122121.", ".312223.", "..3213.."],
              'up': [".312223.", ".122221.", ".322123.", "..3223.."],
              'right': ["..3123..", "..3213..", "..3221..", "..333..."]},
    'uniform': {'down': [".322223.", ".121121.", ".333333.", "..3223.."],
                'up': [".322223.", ".122221.", ".333333.", "..3223.."],
                'right': ["..3223..", "..3213..", "..3333..", "..333..."]},
    'police': {'down': [".322223.", ".123321.", ".333333.", "..3223.."],
               'up': [".322223.", ".122221.", ".333333.", "..3223.."],
               'right': ["..3223..", "..3313..", "..3333..", "..322..."]},
    'medic': {'down': [".333333.", ".111111.", ".322223.", "..3333.."],
              'up': [".333333.", ".111111.", ".322223.", "..3333.."],
              'right': ["..3333..", "..3111..", "..3223..", "..333..."]},
    'vest': {'down': [".322223.", ".132231.", ".333333.", "..3333.."],
             'up': [".322223.", ".132231.", ".333333.", "..3333.."],
             'right': ["..3323..", "..3313..", "..3333..", "..333..."]},
    'firecoat': {'down': [".322223.", ".111111.", ".322223.", ".322223."],
                 'up': [".322223.", ".111111.", ".322223.", ".322223."],
                 'right': ["..3223..", "..3111..", "..3223..", "..3223.."]},
    'hoodie': {'down': [".322223.", ".122221.", ".323323.", "..3223.."],
               'up': [".322223.", ".122221.", ".322223.", "..3223.."],
               'right': ["..3223..", "..3213..", "..3223..", "..333..."]},
    'blanket': {'down': [".322223.", ".322223.", ".322223.", ".322223."],
                'up': [".322223.", ".322223.", ".322223.", ".322223."],
                'right': [".32223..", ".32223..", ".32223..", ".3223..."]},
}
# Legs: colours of the three rows (upper, lower, shoes) and walk patterns.
LEG_COLOURS = {'dark': '333', 'mid': '223', 'shorts': '213', 'light': '113'}
LEG_PATTERNS = {
    'down': (["..x..x..", "..x..x..", "..x..x.."], ["..x..x..", "..x..x..", ".....x.."]),
    'up': (["..x..x..", "..x..x..", "..x..x.."], ["..x..x..", "..x..x..", "..x....."]),
    'right': (["..x.x...", ".x...x..", ".x...x.."], ["..xx....", "..xx....", "..x....."]),
}


def legs_rows(style, view, frame, size):
    pattern = LEG_PATTERNS[view][frame]
    colours = LEG_COLOURS[style]
    rows = [pattern[k].replace('x', colours[k]) for k in range(3)]
    return [rows[k] for k in size.leg_rows]


class Look:
    def __init__(self, key, name, home, trait, talk, pal, behave, where, roles=(),
                 head='short', torso='shirt', legs='dark', props=(), act=None, size=ADULT,
                 frames=None, board=False):
        self.key, self.name, self.home, self.trait, self.talk = key, name, home, trait, talk
        self.pal, self.behave, self.where, self.roles = pal, behave, dict(where), dict(roles)
        self.head, self.torso, self.legs, self.props, self.act = head, torso, legs, props, act
        self.size, self.custom, self.board = size, frames, board

    # ------------------------------------------------------------ drawing
    def body(self, view, frame, arms=None, head=None, torso=None, legs=None, props=None, anim=None,
             board=None):
        """One frame of the figure in a view ('right', 'down', 'up'): legs
        and props walk with `frame`; arm poses animate with `anim`. Props
        may draw behind the body (a backpack seen from the front)."""
        s = self.size
        g = blank()
        legs = legs or self.legs
        torso = torso or self.torso
        head = head or self.head
        patches = [p for name in (self.props if props is None else props) for p in PROPS[name](view, frame, s)]
        for p in patches:
            if len(p) == 4:
                put(g, *p[:3])
        if legs in LEG_SPECIAL:
            rows = LEG_SPECIAL[legs][view][frame]
            put(g, s.legs, 0, [rows[k] for k in s.leg_rows])
        elif legs != 'none':
            put(g, s.legs, 0, legs_rows(legs, view, frame, s))
        t = TORSOS[torso][view]
        put(g, s.torso, 0, [t[k] for k in s.torso_rows])
        h = HEADS[head][view]
        put(g, s.chin - len(h) + 1, 0, h)
        put(g, s.chin + 1, 0, HEADS[head].get('nape', {}).get(view, []))
        for p in patches:
            if len(p) == 3:
                put(g, *p)
        if arms:
            for top, left, rows in arms(view, frame if anim is None else anim, s):
                put(g, top, left, rows)
        if self.board if board is None else board:
            g = shift(g, -2)
            for top, left, rows in BOARD[view][frame]:
                put(g, top, left, rows)
        return g

    def walk(self, view):
        return [self.body(view, 0), self.body(view, 1)]

    def frames(self):
        """{pose: [A, B]} for SIDE, FRONT, BACK, ACT."""
        if self.custom:
            out = self.custom(self)
        else:
            out = {'SIDE': self.walk('right'), 'FRONT': self.walk('down'), 'BACK': self.walk('up')}
            out['ACT'] = self.act(self) if self.act else out['FRONT']
        return out


LEG_SPECIAL = {
    'skirt': {
        'down': ([".322223.", "..1..1..", "..3..3.."], [".322223.", "..1..1..", ".....3.."]),
        'up': ([".322223.", "..1..1..", "..3..3.."], [".322223.", "..1..1..", "..3....."]),
        'right': (["..32223.", ".1...1..", ".3...3.."], ["..3222..", "..11....", "..3....."]),
    },
    'skates': {  # in-line skates: coloured boots on a dark row of wheels
        'down': (["..3..3..", "..2..2..", "..3..3.."], ["..3..3..", "..2..2..", ".....3.."]),
        'up': (["..3..3..", "..2..2..", "..3..3.."], ["..3..3..", "..2..2..", "..3....."]),
        'right': (["..3.3...", ".22..22.", "33...33."], ["..33....", ".322....", ".333...."]),
    },
}
# A skateboard under a figure lifted two rows (side: deck and wheels; front
# and back: the deck end-on). Frame B of the side view pushes off.
BOARD = {
    'right': ([(12, 0, ["32222223"]), (13, 1, ["3....3"])],
              [(12, 0, ["32222223"]), (13, 1, ["3....3"]), (10, 0, ["3", "3"]), (13, 0, ["3"])]),
    'down': ([(12, 2, ["3223"]), (13, 2, ["3..3"])], [(12, 2, ["3223"]), (13, 2, ["3..3"])]),
    'up': ([(12, 2, ["3223"]), (13, 2, ["3..3"])], [(12, 2, ["3223"]), (13, 2, ["3..3"])]),
}


# ---------------------------------------------------------------- props
# Each prop returns patches (top, left, rows) for a view and walk frame;
# behind() patches are painted before the body. Held items are in the
# right hand: the viewer's left in the front view, the viewer's right in
# the back view and the near hand from the side.
def behind(top, left, rows):
    return (top, left, rows, 'behind')


def p_briefcase(v, f, s):
    h = s.hand
    if v == 'down':
        return [(h + 1, 0, ["3.", "33", "32"])]
    if v == 'up':
        return [(h + 1, 6, [".3", "33", "23"])]
    return [(h + 1, 4, ["3"]), (h + 2, 3, ["333", "323"])]


def p_coffee(v, f, s):
    h = s.hand
    if v == 'down':
        return [(h - 1, 0, ["3.", "13"]), (h, 1, ["3"])]
    if v == 'up':
        return [(h - 1, 6, [".3", "31"]), (h, 6, ["3"])]
    return [(h - 1, 5, ["3", "1"]), (h, 4, ["3"])]


def p_camera(v, f, s):
    t = s.torso
    if v == 'down':
        return [(t, 2, ["3..3"]), (t + 1, 3, ["33", "31"])]
    if v == 'up':
        return [(t, 2, ["3..3"])]
    return [(t + 1, 5, ["3"]), (t + 2, 4, ["33"])]


def p_cane(v, f, s):
    h = s.hand
    if v == 'down':
        return [(h, 0, ["31"]), (h + 1, 0, ["3", "3", "3", "3"][:13 - h])]
    if v == 'up':
        return [(h, 6, ["13"]), (h + 1, 7, ["3", "3", "3", "3"][:13 - h])]
    return [(h, 5, ["13"]), (h + 1, 6, ["3"] * (13 - h))]


def p_bags(v, f, s):
    h = s.hand
    bag = ["3.", "33", "21", "33"] if v != 'right' else None
    if v in ('down', 'up'):
        return [(h, 0, ["31"]), (h + 1, 0, ["33", "12", "33"]),
                (h, 6, ["13"]), (h + 1, 6, ["33", "21", "33"])]
    return [(h + 1, 3, ["333", "312", "333"])]


def p_backpack(v, f, s):
    t = s.torso
    if v == 'down':
        return [(t, 1, ["33..33"])]
    if v == 'up':
        return [(t, 1, ["333333", "322223", "322223"])]
    return [behind(t, 0, ["33", "32", "33"])]


def p_guitarcase(v, f, s):
    """A guitar case slung across the back, neck up over one shoulder."""
    t = s.torso
    if v == 'down':
        return [behind(t - 5, 5, ["33", "33", "33", "33"]), behind(t + 2, 0, ["33", "33"])]
    if v == 'up':
        return [(t - 5, 1, ["33", "33", "33"]), (t - 2, 1, [".3", "3223", "3223", "32223", ".3223", "..33"])]
    return [behind(t - 5, 1, ["3", "3", "3"]), behind(t - 2, 0, ["33", "32", "32", "32", "32", "33"])]


def p_sign(v, f, s):
    """A hand-painted sign on a stick, bobbing as they walk."""
    b = f
    sign = ["33333", "31213", "33333"]
    if v == 'down':
        return [(b, 3, sign), (3 + b, 6, ["3"] * (4 - b))]
    if v == 'up':
        return [(b, 0, ["33333", "33333", "33333"]), (3 + b, 1, ["3"] * (4 - b))]
    return [(b, 3, sign), (3 + b, 6, ["3"] * (4 - b)), (s.hand - 1, 5, ["1"])]


def p_satchel(v, f, s):
    """A shoulder bag: strap across the chest, bag at the hip."""
    t = s.torso
    if v == 'down':
        return [(t, 2, ["3"]), (t + 1, 3, ["3"]), (t + 2, 4, ["3"]), (t + 2, 6, ["33", "21", "33"])]
    if v == 'up':
        return [(t, 5, ["3"]), (t + 1, 4, ["3"]), (t + 2, 3, ["3"]), (t + 2, 0, ["33", "12", "33"])]
    return [(t, 3, ["3"]), (t + 2, 1, ["333", "312", "333"])]


def p_crate(v, f, s):
    t = s.torso
    if v == 'down':
        return [(t, 1, ["121121"]), (t + 1, 1, ["333333", "322223", "333333"])]
    if v == 'up':
        return [(t + 1, 0, ["1"]), (t + 1, 7, ["1"])]
    return [(t, 4, ["1211"]), (t + 1, 4, ["3333", "3223", "3333"])]


def p_records(v, f, s):
    t = s.torso
    if v == 'down':
        return [(t, 1, ["313133"]), (t + 1, 1, ["333333", "322223", "333333"])]
    if v == 'up':
        return [(t + 1, 0, ["1"]), (t + 1, 7, ["1"])]
    return [(t, 4, ["3133"]), (t + 1, 4, ["3333", "3223", "3333"])]


def p_mat(v, f, s):
    """A rolled yoga mat carried under the arm."""
    t = s.torso
    if v == 'down':
        return [(t, 6, ["33", "32", "32", "32", "33"])]
    if v == 'up':
        return [(t, 0, ["33", "23", "23", "23", "33"])]
    return [(t + 1, 1, ["33333", "32223", "33333"])]


def p_rod(v, f, s):
    h = s.hand
    if v == 'down':
        return [(1, 0, ["3"] * 7), (h, 0, ["31"])]
    if v == 'up':
        return [(1, 7, ["3"] * 7), (h, 6, ["13"])]
    return [behind(1, 0, ["3"]), behind(2, 1, ["3", "3"]), behind(4, 2, ["3"])]


def p_flag(v, f, s):
    h = s.hand
    wave = ["22", "2."] if f else ["22", "22"]
    if v == 'down':
        return [(h - 6, 0, ["3"] * 6), (h - 6, 1, wave), (h, 0, ["31"])]
    if v == 'up':
        return [(h - 6, 7, ["3"] * 6), (h - 6, 5, wave), (h, 6, ["13"])]
    return [(h - 6, 6, ["3"] * 6), (h - 6, 4, wave), (h, 5, ["1"])]


def p_rolling(v, f, s):
    h = s.hand
    if v == 'down':
        return [(h, 0, ["3", "3"]), (h + 2, 0, ["33", "32", "33"])]
    if v == 'up':
        return [(h, 7, ["3", "3"]), (h + 2, 6, ["33", "23", "33"])]
    return [(h + 1, 2, ["3"]), (h + 2, 0, ["33", "23", "33"])]


def p_medbag(v, f, s):
    h = s.hand
    if v == 'down':
        return [(h + 1, 0, ["33", "21", "33"])]
    if v == 'up':
        return [(h + 1, 6, ["33", "12", "33"])]
    return [(h + 1, 2 + f, ["333", "212"])]


def p_glove(v, f, s):
    h = s.hand
    if v == 'down':
        return [(h - 1, 6, ["33"]), (h, 6, ["33"])]
    if v == 'up':
        return [(h - 1, 0, ["33"]), (h, 0, ["33"])]
    return [(h, 4, ["33"])]


def p_book(v, f, s):
    h = s.hand
    if v == 'down':
        return [(h - 1, 5, ["33", "31"])]
    if v == 'up':
        return []
    return [(h - 1, 5, ["33"])]


def p_newspaper(v, f, s):
    h = s.hand
    if v == 'down':
        return [(h - 1, 6, ["31", "13", "31"])]
    if v == 'up':
        return [(h - 1, 0, ["13", "31", "13"])]
    return [(h - 1, 4, ["113", "131"])]


def p_redbox(v, f, s):
    """Rushly's insulated delivery box worn as a backpack."""
    t = s.torso
    if v == 'down':
        return [behind(t - 4, 1, ["333333", "322223", "322223", "322223"])]
    if v == 'up':
        return [(t - 4, 0, ["33333333", "32222223", "32111123", "32222223", "33333333"])]
    return [behind(t - 4, 0, ["3333", "3223", "3113", "3223", "3333"])]


def p_bag_loot(v, f, s):
    h = s.hand
    if v == 'down':
        return [(h, 6, ["13"]), (h + 1, 6, ["33", "22", "33"])]
    if v == 'up':
        return [(h, 0, ["31"]), (h + 1, 0, ["33", "22", "33"])]
    return [(h, 3, ["3"]), (h + 1, 3, ["333", "322", "333"])]


def p_clipboard(v, f, s):
    h = s.hand
    if v == 'down':
        return [(h - 1, 5, ["33", "13", "11"])]
    if v == 'up':
        return []
    return [(h - 1, 5, ["3", "1"])]


PROPS = {name[2:]: fn for name, fn in globals().items() if name.startswith('p_')}


# ---------------------------------------------------------------- arms
# Arm poses for actions, drawn after the body like props; `f` is the
# action frame (0, 1). Front view unless the action says otherwise.
def a_cheer(v, f, s):
    t, h = s.torso, s.hand
    top = t - 3 + f
    arm = ["1"] + ["3"] * (t - top)
    return [(h, 1, ["3"]), (h, 6, ["3"]), (top, 0, arm), (top, 7, arm)]


def a_wave(v, f, s):
    t, h = s.torso, s.hand
    if f == 0:
        return [(h, 6, ["3"]), (t - 3, 7, ["1", "3", "3", "3"])]
    return [(h, 6, ["3"]), (t - 3, 6, ["1"]), (t - 2, 7, ["3", "3", "3"])]


def a_raise(v, f, s):
    t, h = s.torso, s.hand
    return [(h, 6, ["3"]), (t - 4 + f, 7, ["1"] + ["3"] * (4 - f))]


def a_phone(v, f, s):
    """Phone to the ear, elbow out; the free hand talks on frame B."""
    c, h = s.chin, s.hand
    out = [(h, 1, ["3"]), (c - 1, 1, ["1"]), (c, 0, ["3"])]
    if f:
        out += [(h, 6, ["3"]), (s.torso, 7, ["1"])]
    return out


def a_sip(v, f, s):
    """Coffee at the side, then raised to the lips."""
    c, h = s.chin, s.hand
    if f == 0:
        return [(h - 1, 0, ["3.", "13"]), (h, 1, ["3"])]
    return [(h, 1, ["3"]), (c - 1, 1, ["3", "1"]), (c, 2, ["1"])]


def a_cross(v, f, s):
    t = s.torso
    out = [(t + 1, 1, ["311113"]), (t + 2, 1, ["333333"])]
    if f:
        out.append((s.chin - 1, 3, ["31", "31"]))
    return out


def a_out(v, f, s):
    t, h = s.torso, s.hand
    if f == 0:
        return [(h, 1, ["3"]), (h, 6, ["3"]), (t, 0, ["1"]), (t, 7, ["1"])]
    return [(h, 1, ["3"]), (h, 6, ["3"]), (t, 0, ["1"]), (t - 4, 7, ["1", "3", "3", "3"])]


def a_side(v, f, s):
    """One arm out to the side: 'this way, please'."""
    t, h = s.torso, s.hand
    return [(h, 6, ["3"]), (t - f, 7, ["1"]), (t, 6, ["3"])]


def a_fists(v, f, s):
    c, h = s.chin, s.hand
    if f == 0:
        return [(h, 1, ["3"]), (h, 6, ["3"]), (c, 1, ["1"]), (c, 6, ["1"])]
    return [(h, 1, ["3"]), (h, 6, ["3"]), (c - 1, 1, ["1"]), (c + 1, 6, ["1"])]


def a_shake(v, f, s):
    t, h = s.torso, s.hand
    out = [(h, 6, ["3"]), (t - 4, 7 - f, ["1"]), (t - 3, 7, ["3", "3", "3"])]
    if f:
        out.append((s.chin - 1, 3, ["22", "22"]))   # red in the face
    return out


def a_cap(v, f, s):
    t, h = s.torso, s.hand
    if f == 0:
        return [(h, 6, ["3"]), (t - 3, 6, ["1"]), (t - 2, 7, ["3", "3"])]
    return [(h, 6, ["3"]), (t - 4, 6, ["1"]), (t - 3, 7, ["3", "3", "3"])]


def a_hold(item):
    """Both hands hold an item in front of the chest; it lifts on frame B."""
    def fn(v, f, s):
        h = s.hand
        return [(h, 1, ["3"]), (h, 6, ["3"]), (h - f, 2, item)]
    return fn


def a_offer(item):
    """One hand holds an item out at the side; it lifts on frame B."""
    def fn(v, f, s):
        t, h = s.torso, s.hand
        return [(h, 6, ["3"]), (t - f - len(item) + 1, 6, item), (t + 1 - f, 7, ["1"])]
    return fn


def a_glove(v, f, s):
    """Glove up for a pop fly; the ball drops into it on frame B."""
    t, h = s.torso, s.hand
    out = [(h, 6, ["3"]), (h - 1, 6, ["0"]), (t - 5, 6, ["33", "33"]), (t - 3, 7, ["3", "3", "3"])]
    out.append((t - 7, 6, ["1"]) if f == 0 else (t - 5, 6, ["1"]))
    return out


def a_brush(v, f, s):
    t, h = s.torso, s.hand
    if f == 0:
        return [(h, 6, ["3"]), (t - 3, 7, ["3", "1", "3", "3"])]
    return [(h, 6, ["3"]), (t - 3, 6, ["3"]), (t - 2, 7, ["1", "3", "3"])]


def a_paddle(v, f, s):
    """A traffic paddle held up: orange face, then turned to the light one."""
    t, h = s.torso, s.hand
    face = [".33.", "3223", "3223", ".33."] if f == 0 else [".33.", "3113", "3113", ".33."]
    return [(h, 6, ["3"]), (0, 4, face), (4, 6, ["3", "3"]), (6, 6, ["1"])]


def a_flagup(v, f, s):
    t, h = s.torso, s.hand
    flag = ["22", "22"] if f == 0 else ["22", ".2"]
    return [(h, 6, ["3"]), (t - 6, 7, ["3", "3", "3"]), (t - 6, 5, flag), (t - 3, 7, ["1", "3", "3"])]


def a_book(v, f, s):
    t, h = s.torso, s.hand
    if f == 0:
        return [(h, 6, ["3"]), (h - 1, 5, ["00"]), (t - 6, 6, ["33", "31"]), (t - 4, 7, ["1", "3", "3", "3"])]
    return [(h, 6, ["3"]), (h - 1, 5, ["00"]), (t - 5, 6, ["33", "31"]), (t - 3, 7, ["1", "3", "3"])]


def a_phonecheck(v, f, s):
    h = s.hand
    return [(h, 1, ["3"]), (h, 6, ["3"]), (h - 1, 3, ["33"]), (h, 2, ["1" if f else "3", ".", ".", "1"])]


def a_camera(v, f, s):
    """Camera up to the eye with both hands; frame B flashes."""
    c, h = s.chin, s.hand
    out = [(h, 1, ["3"]), (h, 6, ["3"]), (c - 1, 1, ["133331", "133131"])]
    if f:
        out.append((c - 5, 5, [".1.", "1.1", ".1."]))
    return out


def a_signup(v, f, s):
    """A sign held overhead in both hands, pumped up and down."""
    h = s.hand
    arm = ["1"] + ["3"] * (3 - f)
    return [(h, 1, ["3"]), (h, 6, ["3"]), (f, 0, ["33333333", "31221213", "33333333"]),
            (3 + f, 1, arm), (3 + f, 6, arm)]


def a_behind(v, f, s):
    """Back view: hands clasped behind the back."""
    h = s.hand
    return [(h, 1, ["3"]), (h, 6, ["3"]), (h + 1, 3, ["11"])]


def act(view='down', arms=None, props=None, head=None, torso=None, legs=None, walk=False, after=None):
    """An action made from the look's own body: two frames of an arm pose."""
    def build(look):
        out = []
        for f in (0, 1):
            g = look.body(view, f if walk else 0, arms=arms, head=head, torso=torso, legs=legs,
                          props=props, anim=f)
            if after:
                g = after(g, f, look)
            out.append(g)
        return out
    return build


def head_patch(look, view, chin, dx=0, head=None):
    h = HEADS[head or look.head][view]
    return (chin - len(h) + 1, dx, h)


def drawn(rows_a, rows_b, view='down', chin=None, dx=0, top=None):
    """An explicit two-frame action; the look's head (hat and hair) is
    painted at `chin` so the action keeps the look's identity."""
    def build(look):
        out = []
        for rows in (rows_a, rows_b):
            g = art(rows, top)
            if chin is not None:
                put(g, *head_patch(look, view, chin, dx))
            out.append(g)
        return out
    return build


def nod(g, f, look):
    """Frame B dips the head one row (headphones on)."""
    if not f:
        return g
    out = copy(g)
    chin = look.size.chin
    for y in range(chin + 1, 0, -1):
        out[y] = [g[y - 1][x] or (g[y][x] if y == chin + 1 else 0) for x in range(8)]
    out[0] = [0] * 8
    return out


def look_up(g, f, look):
    """Back view, chin raised to a high painting (neck showing); frame B
    turns to the next one."""
    out = copy(g)
    chin = look.size.chin
    for y in range(0, chin + 1):
        out[y] = [0] * 8
    h = HEADS[look.head]['up']
    put(out, chin - len(h), f, h)
    put(out, chin, 3, ["11"])
    return out


def hop(g, f, look):
    return shift(g, -2) if f else g


# ---------------------------------------------------------------- explicit actions
SIT_BLANKET = drawn(
    [".322223.", "32222223", "32311323", "33333333"],
    [".322223.", "32311323", "32222223", "33333333"],
    chin=9)
LYING = drawn(
    ["...3333.", "33322223", "31122223", "33333333"],
    ["....333.", "33332223", "31122223", "33333333"])
SEATED_TABLE = drawn(
    [".322223.", ".122221.", "33333333", "31111113", ".3....3.", ".3....3."],
    [".322213.", ".12222..", "33333333", "31111113", ".3....3.", ".3....3."],
    chin=7)
READING = drawn(
    ["31111113", "31313113", "31313113", "33333333", ".3.33.3.", ".3....3."],
    ["........", "31111113", "31313113", "33333333", ".3.33.3.", ".3....3."],
    chin=7)
FISHING = drawn(
    [".......3", ".......3", "......3.", "......3.", "........", "......3.", "......3.", "..32213.",
     "..3223..", "..33333.", ".3223.3.", ".3223.3.", ".3333.33"],
    ["........", ".......3", "......33", "......3.", "........", "......3.", "......3.", "..32213.",
     "..3223..", "..33333.", ".3223.3.", ".3223.3.", ".3333.33"],
    view='right', chin=7)
SIGN_UP = act(arms=a_signup, props=())
GUITAR = drawn(
    [".322223.", "3112223.", "31333313", "3111123.", ".333.3..", "..3..3..", "..3..3.."],
    [".322223.", "3112223.", "31333313", "3111223.", ".3331.3.", "..3..3..", "..3..3.."],
    chin=6)
TREE_POSE = drawn(
    ["..1111..", ".3....3.", ".3....3.", ".3....3.", ".3....3.", ".322223.", ".322223.", ".322223.",
     "..3223..", "..3.3...", "..333...", "..3.....", "..3....."],
    ["...11...", "..3..3..", ".3....3.", ".3....3.", ".3....3.", ".322223.", ".322223.", ".322223.",
     "..3223..", "..3.3...", "..333...", "..3.....", "..3....."],
    chin=6, top=1)
SHOVE = drawn(
    ["..3223..", "..32211.", "..3223..", "..333...", "..3.3...", ".3...3..", ".3...3.."],
    ["...3223.", "...32211", "...3223.", "..333...", ".3..3...", "3...3...", "3....3.."],
    view='right', chin=6)
RUN_BAG = drawn(
    ["..3223..", "..32333.", "..33322.", "..333...", ".3..3...", "3....3..", ".....3.."],
    ["..3223..", "..32333.", "..33322.", "..333...", "..33....", "..3.3...", "..3..3.."],
    view='right', chin=6)


# ---------------------------------------------------------------- animals
def animal(side_a, side_b, front_a, front_b, back_a, back_b, act_a, act_b):
    def build(look):
        return {'SIDE': [art(side_a), art(side_b)], 'FRONT': [art(front_a), art(front_b)],
                'BACK': [art(back_a), art(back_b)], 'ACT': [art(act_a), art(act_b)]}
    return build


# Canada goose (yellow palette: cream body, dark neck and head, white
# chinstrap). The action hisses: neck low and forward, wings up.
GOOSE = animal(
    [".....33.", ".....313", ".....3..", ".....3..", ".33333..", "3111113.", "31111113", ".311113.", "..3333..", "...3.3.."],
    [".....33.", ".....313", ".....3..", ".....3..", ".33333..", "3111113.", "31111113", ".311113.", "..3333..", "....3..."],
    ["...33...", "..3113..", "...33...", "...33...", "...33...", "..3113..", ".311113.", ".311113.", "..3333..", "..3..3.."],
    ["...33...", "..3113..", "...33...", "...33...", "...33...", "..3113..", ".311113.", ".311113.", "..3333..", "..3....."],
    ["...33...", "...33...", "...33...", "...33...", "..3333..", ".311113.", "31111113", ".311113.", "..3113..", "..3..3.."],
    ["...33...", "...33...", "...33...", "...33...", "..3333..", ".311113.", "31111113", ".311113.", "..3113..", ".....3.."],
    ["3.......", "33......", "313.....", "3113....", "31113...", "311113..", ".3111333", "..311313", "..333...", "...3.3.."],
    ["........", "........", "........", "33......", "3113....", "311113..", ".3111333", "..311313", "..333...", "...3.3.."])
# Raccoon (blue palette: grey-blue fur, black mask, ringed tail); the
# action stands up on its hind legs, paws up.
RACCOON = animal(
    [".....3.3", "..333311", ".3111333", "31111131", "13111133", "3.3333..", "..3..3.."],
    [".....3.3", "..333311", ".3111333", "31111131", "13111133", "3.3333..", "...3.3.."],
    [".3....3.", ".311113.", ".333333.", ".313313.", "..3113..", ".311113.", ".311113.", ".3.33.3."],
    [".3....3.", ".311113.", ".333333.", ".313313.", "..3113..", ".311113.", ".311113.", "...33.3."],
    [".3....3.", ".311113.", ".311113.", "..3113..", ".311113.", ".311113.", "..3333..", "...13...", "...31...",
     "...13...", "...3...."],
    [".3....3.", ".311113.", ".311113.", "..3113..", ".311113.", ".311113.", "..3333..", "...13...", "...31...",
     "....13..", "....3..."],
    [".3....3.", ".311113.", ".333333.", ".313313.", "..3113..", "13111131", ".311113.", ".311113.", "..3113..",
     "3.3..3..", "13......"],
    [".3....3.", ".311113.", ".333333.", ".313313.", "1.3113.1", "33111133", ".311113.", ".311113.", "..3113..",
     "3.3..3..", "13......"])
# Rock pigeon (blue palette); the action takes off, wings up then down.
PIGEON = animal(
    [".....33.", ".....313", ".33332..", "3111123.", ".33333..", "..3.3..."],
    ["....33..", "....3133", ".33332..", "3111123.", ".33333..", "...33..."],
    ["...33...", "..3113..", "..3223..", ".311113.", "..3333..", "..3..3.."],
    ["...33...", "..3113..", "..3223..", ".311113.", "..3333..", "...33..."],
    ["...33...", "..3333..", "..3223..", ".311113.", "..3333..", "..3..3.."],
    ["...33...", "..3333..", "..3223..", ".311113.", "..3333..", "...33..."],
    ["3......3", "33....33", "313..313", ".313313.", "..3223..", "..3113..", "...33...", "........"],
    ["........", "........", "...33...", "..3223..", "33311333", "3.3113.3", "...33...", "........"])
# Black squirrel (blue palette: black fur, pale-rimmed bushy tail); the
# action sits up holding a nut.
SQUIRREL = animal(
    ["33......", "313.....", "3113....", "3113..3.", ".313.333", ".3333313", "..33333.", "..3..3.."],
    ["33......", "313.....", "3113....", "3113..3.", ".313.333", ".3333313", "..33333.", "...33..."],
    ["33......", "313.3.3.", "3113333.", "3113131.", "3113333.", ".313333.", "..33333.", "..3..3.."],
    ["33......", "313.3.3.", "3113333.", "3113131.", "3113333.", ".313333.", "..33333.", "...3.3.."],
    ["......33", ".3.3.313", ".3333113", ".3333113", ".3333113", ".333313.", ".33333..", ".3..3..."],
    ["......33", ".3.3.313", ".3333113", ".3333113", ".3333113", ".333313.", ".33333..", "..3.3..."],
    ["33..3.3.", "313.3333", "3113.313", "3113.33.", "3113311.", "3113333.", ".313333.", "..33333.", "...3.3.."],
    ["33..3.3.", "313.3333", "3113.313", "3113.33.", "31133.11", "3113333.", ".313333.", "..33333.", "...3.3.."])
# Golden dog (yellow palette); the action sits, tongue and tail going.
DOG = animal(
    [".....33.", "3...3223", ".3..3221", ".333333.", ".322223.", ".3.33.3.", ".3....3."],
    [".....33.", "3...3223", ".3..3221", ".333333.", ".322223.", "..3..3..", "..3..3.."],
    ["..3333..", ".322223.", "32322323", "32211223", "..3133..", "..3223..", ".322223.", ".32..23.", ".33..33."],
    ["..3333..", ".322223.", "32322323", "32211223", "..3133..", "..3223..", ".322223.", ".32..23.", "....33.."],
    ["..3333..", ".322223.", "32222223", "32222223", "..3223..", "..3223..", ".322223.", ".32..23.", ".33..33."],
    ["..3333..", ".322223.", "32222223", "32222223", "..3223..", "..3223..", ".322223.", ".32..23.", ".33....."],
    ["..3333..", ".322223.", "32322323", "32211223", "..3133..", "..3223..", ".322223.", "32211223", "33333333"],
    ["..3333..", ".322223.", "32322323", "32211223", "..3113..", "..3223..", ".322223.", "32211223", "33333333"])


# ---------------------------------------------------------------- the library
L = Look
LOOKS = [
    # The engine relies on the officer being look 0.
    L('officer', "POLICE OFFICER", "ON FOOT PATROL",
      ["WALKS THE BEAT,", "KNOWS EVERY ALLEY", "AND EVERY EXCUSE."],
      ["EASY ON THE GAS", "OUT THERE, OK?"],
      NAVY, 'OFFICER', dict(downtown=5, financial=4, entertainment=5, yonge_dundas=7, transit=3, night=6,
                            chinatown=2, kensington=2, queen_west=3, waterfront=3, park=2),
      dict(transit_hall=3, mall=2, museum=1, tower=1),
      head='peaked', torso='police', act=act(arms=a_out)),
    # --- civilians
    L('office', "OFFICE WORKER", "FINANCIAL DIST",
      ["BAY ST BY DAY,", "THE PATH BY", "WINTER."],
      ["LATE FOR THE 9:00", "AGAIN."],
      6, 'WALK', dict(financial=9, downtown=6, transit=4, yonge_dundas=3, entertainment=2),
      dict(office=6, cafe=3, transit_hall=3, tower=1),
      head='short', torso='suit', props=('briefcase',), act=act(arms=a_phone, props=())),
    L('coffee', "COFFEE COMMUTER", "KING ST WEST",
      ["THIRD CUP BY TEN.", "KNOWS EVERY", "BARISTA BY NAME."],
      ["CAN'T TALK.", "STILL WAKING UP."],
      5, 'WALK', dict(financial=7, downtown=6, queen_west=4, entertainment=4, transit=5, leslieville=2),
      dict(cafe=6, office=4, transit_hall=3),
      head='bun', torso='suit', legs='skirt', props=('coffee',), act=act(arms=a_sip, props=())),
    L('student', "U OF T STUDENT", "ST GEORGE CAMPUS",
      ["ESSAY DUE AT 9.", "LIVES IN ROBARTS.", "RUNS ON NOODLES."],
      ["IS IT FINALS", "ALREADY?"],
      1, 'WALK', dict(campus=9, kensington=5, chinatown=4, downtown=3, transit=3, queen_west=2, night=2),
      dict(museum=3, cafe=4, gallery=2, store=2, electronics=2),
      head='short', torso='pack', props=('backpack',), act=act(arms=a_phonecheck)),
    L('tourist', "TOURIST", "HARBOURFRONT",
      ["PHOTOGRAPHS EVERY", "STREETCAR. SAYS", "SORRY TO PIGEONS."],
      ["WHICH WAY IS THE", "TOWER? OH. THERE."],
      3, 'PHOTO', dict(waterfront=8, entertainment=7, downtown=5, yonge_dundas=6, chinatown=4, kensington=4,
                       financial=2, high_park=2),
      dict(museum=6, gallery=4, tower=8, mall=4, transit_hall=3),
      head='sunhat', torso='tee', legs='shorts', props=('camera',),
      act=act(arms=a_camera, props=())),
    L('jogger', "JOGGER", "WATERFRONT TRAIL",
      ["UP AT FIVE, RAIN", "OR SNOW. COUNTS", "STEPS ASLEEP."],
      ["ON YOUR LEFT.", "ALWAYS LEFT."],
      1, 'JOG', dict(waterfront=8, park=7, high_park=8, residential=4, leslieville=3, campus=2),
      head='headband', torso='tee', legs='shorts', act=act(arms=a_cheer)),
    L('skater', "SKATEBOARDER", "ANY SMOOTH CURB",
      ["SCUFFED SHOES,", "FRESH WAX, NEVER", "CHASES A BUS."],
      ["SAW MY KICKFLIP?", "NO? CLASSIC."],
      5, 'SKATE', dict(queen_west=6, downtown=4, waterfront=5, park=4, junction=4, campus=3, night=3),
      head='backcap', torso='tee', legs='mid', board=True, act=act(view='right', after=hop)),
    L('elder', "RETIREE", "LITTLE ITALY",
      ["FIFTY YEARS ON", "ONE STREET. KNOWS", "EVERY BAKER."],
      ["SLOW DOWN. LIFE", "IS NOT A RACE."],
      6, 'SLOW', dict(little_italy=8, danforth=7, chinatown=5, residential=6, park=5, kensington=3, junction=4),
      dict(cafe=4, bakery=4, museum=2, restaurant=3),
      head='grey', torso='shirt', props=('cane',), act=act(arms=a_cap)),
    L('shopper', "MARKET SHOPPER", "SPADINA AVE",
      ["HAGGLES FOR BOK", "CHOY AND WINS.", "KNOWS EVERY STALL."],
      ["LYCHEES, 3 FOR $5", "DON'T TELL."],
      1, 'SLOW', dict(chinatown=9, kensington=7, little_italy=3, danforth=3, residential=2),
      dict(store=5, bakery=3, mall=2),
      head='bun', torso='shirt', props=('bags',)),
    L('chef', "LINE COOK", "BEHIND THE KITCHEN",
      ["TEN-HOUR SHIFTS,", "FIVE-MINUTE", "BREAKS."],
      ["ORDER UP? NOT ME.", "I'M ON BREAK."],
      3, 'SLOW', dict(entertainment=6, chinatown=5, little_italy=6, danforth=5, queen_west=4, kensington=3, night=3),
      dict(restaurant=8, pub=3),
      head='toque', torso='whites', legs='dark', act=act(arms=a_cross)),
    L('builder', "BUILDER", "EVERY OTHER BLOCK",
      ["CRANES ARE THE", "CITY BIRD, SHE", "SAYS."],
      ["ROAD'S CLOSED.", "TRY THE NEXT ONE."],
      ORANGE, 'WALK', dict(downtown=6, financial=5, entertainment=5, waterfront=4, leslieville=3, junction=3,
                           queen_west=3, transit=2),
      head='hardhat', torso='hivis', legs='mid', act=act(arms=a_paddle)),
    L('nurse', "NURSE", "HOSPITAL ROW",
      ["TWELVE-HOUR", "SHIFTS, ENDLESS", "PATIENCE."],
      ["DRINK SOME WATER.", "YOU LOOK PALE."],
      5, 'WALK', dict(hospital=9, campus=3, downtown=3, transit=3, chinatown=2, night=3),
      dict(cafe=3, transit_hall=2),
      head='bun', torso='scrubs', legs='mid', props=('clipboard',), act=act(arms=a_hold(["33", "13"]))),
    L('paramedic', "PARAMEDIC", "UNIVERSITY AVE",
      ["CALM IN A CRISIS.", "KNOWS THE FASTEST", "WAY ANYWHERE."],
      ["KEEP THE LANE", "CLEAR, OK?"],
      1, 'WALK', dict(hospital=8, downtown=4, yonge_dundas=3, entertainment=3, night=3),
      head='short', torso='medic', props=('medbag',), act=act(arms=a_wave, props=('medbag',))),
    L('firefighter', "FIREFIGHTER", "THE FIRE HALL",
      ["COOKS CHILI FOR", "THE WHOLE CREW.", "RESCUES CATS TOO."],
      ["SMOKE ALARM", "TESTED? GOOD."],
      3, 'WALK', dict(downtown=3, residential=4, kensington=2, leslieville=3, junction=3, danforth=3, park=2),
      head='firehelm', torso='firecoat', act=act(arms=a_wave)),
    L('ttc', "TTC OPERATOR", "THE 504 KING",
      ["SAYS MOVE ALL THE", "WAY BACK FORTY", "TIMES A DAY."],
      ["STAND CLEAR OF", "THE DOORS, PLEASE."],
      1, 'WALK', dict(transit=9, downtown=4, queen_west=3, danforth=3, leslieville=2, junction=2),
      dict(transit_hall=6),
      head='cap', torso='uniform', act=act(arms=a_cap)),
    L('carrier', "LETTER CARRIER", "THE ANNEX",
      ["KNOWS EVERY DOG", "ON THE ROUTE BY", "NAME. MOST WAG."],
      ["NOTHING FOR YOU", "TODAY. SORRY."],
      1, 'WALK', dict(residential=9, little_italy=4, danforth=4, leslieville=4, junction=4, high_park=3, campus=2),
      dict(office=2, store=2),
      head='cap', torso='uniform', legs='shorts', props=('satchel',),
      act=act(arms=a_offer(["11", "13"]), props=('satchel',))),
    L('artist', "PAINTER", "QUEEN WEST",
      ["PAINT ON BOTH", "SLEEVES. SKETCHES", "STRANGERS."],
      ["HOLD STILL.", "YOU'RE PERFECT."],
      6, 'WALK', dict(queen_west=9, kensington=5, leslieville=4, junction=4, park=3, night=2),
      dict(gallery=7, cafe=3, museum=2),
      head='beret', torso='paint', legs='mid', act=act(arms=a_brush)),
    L('busker', "BUSKER", "SUBWAY STATIONS",
      ["THREE CHORDS,", "TEN THOUSAND", "SONGS."],
      ["ANY REQUESTS?", "NOT THAT ONE."],
      3, 'BUSK', dict(transit=7, yonge_dundas=7, kensington=6, queen_west=5, waterfront=4, entertainment=4, night=3),
      dict(transit_hall=6),
      head='curly', torso='shirt', legs='mid', props=('guitarcase',), act=GUITAR),
    L('protester', "PROTESTER", "QUEEN'S PARK",
      ["HAND-PAINTED", "SIGNS, STRONG", "OPINIONS, SNACKS."],
      ["SIGN THE PETITION.", "IT'S GOOD KARMA."],
      1, 'SIGN', dict(campus=6, downtown=6, yonge_dundas=5, financial=3),
      head='beanie', torso='shirt', legs='mid', props=('sign',), act=SIGN_UP),
    L('preacher', "STREET PREACHER", "YONGE & DUNDAS",
      ["SAME CORNER FOR", "YEARS. KNOWS THE", "REGULARS BY NAME."],
      ["HAVE A BLESSED", "AFTERNOON."],
      6, 'SIGN', dict(yonge_dundas=9, downtown=4, transit=3, financial=2),
      head='short', torso='suit', props=('book',), act=act(arms=a_book)),
    L('hockey', "HOCKEY FAN", "NEAR THE ARENA",
      ["BLUE JERSEY,", "ETERNAL HOPE.", "THIS IS THE YEAR."],
      ["DID WE WIN?", "DON'T TELL ME."],
      2, 'WALK', dict(entertainment=7, financial=4, waterfront=4, transit=4, downtown=4, night=5),
      dict(pub=8, transit_hall=3),
      head='short', torso='jersey', act=act(arms=a_cheer)),
    L('baseball', "BASEBALL FAN", "NEAR THE DOME",
      ["BRINGS A GLOVE TO", "EVERY GAME. HAS", "NEVER CAUGHT ONE."],
      ["BOTTOM OF THE", "NINTH, PAL."],
      5, 'WALK', dict(entertainment=8, waterfront=5, transit=4, downtown=3),
      dict(pub=6, tower=2),
      head='cap', torso='tee', legs='shorts', props=('glove',), act=act(arms=a_glove)),
    L('vendor', "MARKET VENDOR", "KENSINGTON MARKET",
      ["UP BEFORE DAWN,", "HAULS CRATES,", "KNOWS EVERY CAT."],
      ["FRESH TODAY.", "WELL, YESTERDAY."],
      5, 'WALK', dict(kensington=9, chinatown=7, little_italy=3, danforth=3),
      dict(store=4),
      head='darkcap', torso='apron', props=('crate',)),
    L('regular', "CAFE REGULAR", "THE DANFORTH",
      ["SAME TABLE SINCE", "FOREVER. READS", "THE PAPER TWICE."],
      ["SIT. HAVE A", "COFFEE. RELAX."],
      3, 'SIT', dict(danforth=9, little_italy=6, leslieville=4, queen_west=3, junction=4),
      dict(cafe=8, bakery=3, restaurant=2),
      head='grey', torso='shirt', props=('newspaper',), act=READING),
    L('birder', "BIRDWATCHER", "HIGH PARK",
      ["BINOCULARS UP,", "COUNTING SPARROWS", "SINCE SUNRISE."],
      ["SHH. A HAWK,", "TWO O'CLOCK."],
      3, 'SLOW', dict(high_park=8, park=7, waterfront=4, leslieville=3, residential=2),
      dict(museum=3),
      head='short', torso='shirt', legs='mid', props=('camera',),
      act=act(arms=a_camera, props=())),
    L('listener', "MUSIC LOVER", "YONGE STREET",
      ["HEADPHONES ON,", "WORLD OFF. KNOWS", "EVERY SHORTCUT."],
      ["WHAT?", "CAN'T HEAR YOU."],
      6, 'WALK', dict(yonge_dundas=8, downtown=5, queen_west=5, transit=5, kensington=4, night=3),
      dict(mall=7, electronics=4, store=3),
      head='phones', torso='hoodie', legs='mid', act=act(after=nod)),
    L('blader', "ROLLERBLADER", "THE BOARDWALK",
      ["KNEE PADS, NO", "BRAKES, BIG", "CONFIDENCE."],
      ["CAN'T STOP,", "CAN'T TALK, BYE."],
      3, 'SKATE', dict(waterfront=9, park=5, high_park=5, leslieville=3),
      head='helmet', torso='tee', legs='skates'),
    L('bikecourier', "BIKE COURIER", "DOWNTOWN CORE",
      ["RIVAL AND FRIEND.", "FASTER THAN YOU", "IN TRAFFIC."],
      ["STREETCAR TRACKS", "ARE THE ENEMY."],
      1, 'JOG', dict(downtown=7, financial=6, queen_west=5, kensington=4, entertainment=4, campus=3),
      dict(office=2, cafe=2),
      head='helmet', torso='tee', legs='shorts', props=('satchel',), act=act(arms=a_phonecheck)),
    L('yogi', "YOGA COMMUTER", "LESLIEVILLE",
      ["ROLLED MAT, OAT", "LATTE, UNSHAKEABLE", "CALM."],
      ["BREATHE IN.", "NOW DRIVE SLOWER."],
      6, 'WALK', dict(leslieville=9, park=5, high_park=5, queen_west=4, danforth=4, residential=3),
      dict(cafe=4),
      head='tail', torso='tee', legs='mid', props=('mat',), act=TREE_POSE),
    L('digger', "RECORD DIGGER", "THE JUNCTION",
      ["OWNS 4,000", "RECORDS AND", "ONE SPOON."],
      ["THAT'S A FIRST", "PRESSING, FRIEND."],
      1, 'SLOW', dict(junction=8, queen_west=7, kensington=6, leslieville=4),
      dict(store=5, cafe=2),
      head='beanie', torso='shirt', legs='mid', props=('records',)),
    L('angler', "ANGLER", "THE WATERFRONT",
      ["PATIENT AS THE", "LAKE. SWEARS HE", "SAW A PIKE ONCE."],
      ["SHH. THEY'RE", "BITING TODAY."],
      6, 'SIT', dict(waterfront=9, high_park=4, park=2),
      head='darkcap', torso='vest', legs='mid', props=('rod',), act=FISHING),
    # --- people experiencing homelessness (with dignity)
    L('vent', "STREET NEIGHBOUR", "A WARM VENT",
      ["KNOWS THE BLOCK", "BETTER THAN", "ANYONE."],
      ["SPARE A MINUTE?", "THANKS. TAKE CARE."],
      3, 'BEG', dict(downtown=6, yonge_dundas=5, queen_west=4, financial=3, transit=4, night=3),
      head='beanie', torso='blanket', legs='mid', act=SIT_BLANKET),
    L('doorway', "TIRED NEIGHBOUR", "A QUIET DOORWAY",
      ["WORKED NIGHTS,", "LOST THE LEASE.", "LET THEM REST."],
      ["MM. FIVE MORE", "MINUTES, OK?"],
      5, 'SLEEP', dict(downtown=6, yonge_dundas=4, queen_west=3, financial=3, night=6),
      head='hood', torso='hoodie', legs='mid', act=LYING),
    L('cardboard', "SIGN HOLDER", "QUEEN & SPADINA",
      ["CARDBOARD SIGN,", "NEAT LETTERS,", "STEADY SMILE."],
      ["ANYTHING HELPS.", "HAVE A GOOD ONE."],
      6, 'SIGN', dict(downtown=6, queen_west=5, yonge_dundas=5, chinatown=3, transit=3),
      head='darkcap', torso='shirt', legs='mid', act=act(arms=lambda v, f, s: [
          (s.hand, 1, ["3"]), (s.hand, 6, ["3"]), (s.hand - 1 - f, 1, ["333333", "311113", "313313", "333333"])])),
    # --- antagonists
    L('rival', "RUSHLY RIDER", "EVERYWHERE NOW",
      ["THREE APPS, TWO", "PHONES, ZERO", "MANNERS."],
      ["OUT OF MY LANE,", "INDIE COURIER."],
      5, 'RIVAL', dict(downtown=7, financial=7, entertainment=5, queen_west=4, yonge_dundas=4, campus=3),
      dict(cafe=2, office=2),
      head='backcap', torso='shirt', legs='mid', props=('redbox',),
      act=lambda look: [put(g, *p) for g, p in zip(SHOVE(look), [(4, 0, ["3333", "3223", "3113", "3223", "3333"])] * 2)]),
    L('tough', "STREET TOUGH", "BACK ALLEYS",
      ["HOODIE UP, CHIP", "ON SHOULDER, SOFT", "ON DOGS."],
      ["GOT A PROBLEM?", "THOUGHT NOT."],
      6, 'TOUGH', dict(night=8, downtown=4, entertainment=4, yonge_dundas=3, junction=2),
      head='hood', torso='hoodie', legs='mid', act=act(arms=a_fists)),
    L('pickpocket', "PICKPOCKET", "BUSY CROSSWALKS",
      ["LIGHT FINGERS,", "FAST FEET,", "NO ALIBI."],
      ["WHAT BAG?", "NEVER SEEN IT."],
      5, 'THIEF', dict(yonge_dundas=8, entertainment=6, transit=6, chinatown=4, waterfront=4, night=4),
      dict(mall=4, transit_hall=5),
      head='darkcap', torso='hoodie', legs='dark', props=('bag_loot',), act=RUN_BAG),
    L('rager', "ROAD RAGER", "THE GARDINER",
      ["HONKS AT GREEN", "LIGHTS. HONKS AT", "HONKING."],
      ["LEARN TO DRIVE!", "OR WALK!"],
      1, 'RAGER', dict(financial=5, downtown=5, entertainment=4, waterfront=3, residential=2),
      head='short', torso='suit', act=act(arms=a_shake)),
    # --- animals
    L('goose', "CANADA GOOSE", "THE LAKESHORE",
      ["FEARS NOTHING.", "OWNS EVERY PATH.", "HONKS FIRST."],
      ["HONK.", "HONK HONK."],
      3, 'GOOSE', dict(waterfront=9, park=7, high_park=8, leslieville=2), frames=GOOSE),
    L('raccoon', "RACCOON", "EVERY GREEN BIN",
      ["THE CITY'S TRUE", "MAYOR. OPENS ANY", "BIN. EVENTUALLY."],
      ["CHRRR?", "(STARES)"],
      2, 'SCURRY', dict(night=9, residential=7, park=5, high_park=5, kensington=4, leslieville=3, danforth=3),
      frames=RACCOON),
    L('pigeon', "PIGEON", "ANY LEDGE",
      ["RIDES STREETCAR", "ROOFS. PAYS", "NO FARE."],
      ["COO.", "COO COO."],
      2, 'PIGEON', dict(downtown=8, yonge_dundas=8, financial=6, chinatown=7, kensington=6, transit=6,
                        entertainment=5, waterfront=4),
      frames=PIGEON),
    L('squirrel', "BLACK SQUIRREL", "HIGH PARK",
      ["BURIES NUTS,", "FORGETS NUTS,", "GROWS TREES."],
      ["CHK CHK CHK.", "(BURIES A NUT)"],
      2, 'SCURRY', dict(park=9, high_park=9, campus=7, residential=5, leslieville=3),
      frames=SQUIRREL),
    L('dog', "GOOD DOG", "TRINITY BELLWOODS",
      ["GOLDEN, LOYAL,", "DEEPLY UNSURE OF", "SQUIRRELS."],
      ["WOOF.", "(WAGS HAPPILY)"],
      3, 'DOG', dict(park=8, high_park=7, residential=6, queen_west=4, leslieville=5, danforth=3),
      frames=DOG),
    # --- indoor roles (some also walk outside)
    L('artlover', "ART LOVER", "THE GALLERY",
      ["STANDS AT ONE", "PAINTING FOR AN", "HOUR. HAPPILY."],
      ["SEE THE BLUE?", "IT'S ALL BLUE."],
      6, 'PHOTO', dict(queen_west=4, campus=2, downtown=2),
      dict(gallery=9, museum=8, cafe=2),
      head='bun', torso='shirt', legs='skirt', act=act(view='up', arms=a_behind, after=look_up)),
    L('guide', "TOUR GUIDE", "MUSEUM HALLS",
      ["SMALL FLAG, LOUD", "VOICE, A FACT", "FOR EVERYTHING."],
      ["STAY WITH THE", "FLAG, PLEASE."],
      5, 'SLOW', dict(waterfront=4, entertainment=4, downtown=3, yonge_dundas=3),
      dict(museum=8, gallery=5, tower=7),
      head='short', torso='lanyard', props=('flag',), act=act(arms=a_flagup, props=())),
    L('teacher', "TEACHER", "HARBORD VILLAGE",
      ["MARKS ESSAYS ON", "THE STREETCAR,", "RED PEN IN HAND."],
      ["FIRST BELL IN", "TEN MINUTES."],
      5, 'WALK', dict(campus=3, park=3, waterfront=2, residential=3),
      dict(museum=7, gallery=4, tower=4),
      head='curly', torso='shirt', legs='skirt', props=('clipboard',), act=act(arms=a_raise)),
    L('guard', "SECURITY GUARD", "THE FRONT DOOR",
      ["SEES EVERYTHING,", "SAYS NOTHING,", "LOVES CROSSWORDS."],
      ["NO FLASH", "PHOTOGRAPHY."],
      2, 'INDOOR', dict(),
      dict(museum=8, gallery=7, mall=7, electronics=6, store=4, office=6, tower=5, transit_hall=4),
      head='darkcap', torso='uniform', act=act(arms=a_cross)),
    L('clerk', "STORE CLERK", "ELECTRONICS SHOP",
      ["KNOWS EVERY CABLE", "AND HAS OPINIONS", "ABOUT EACH ONE."],
      ["TRIED TURNING IT", "OFF AND ON?"],
      1, 'INDOOR', dict(),
      dict(electronics=9, store=6, mall=5),
      head='short', torso='lanyard', act=act(arms=a_hold(["3333", "3113"]))),
    L('cashier', "CASHIER", "THE CORNER STORE",
      ["COUNTS CHANGE", "FASTER THAN A", "CALCULATOR."],
      ["NEXT IN LINE,", "PLEASE."],
      3, 'INDOOR', dict(),
      dict(store=9, mall=5, bakery=3, electronics=3),
      head='tail', torso='apron', act=act(arms=a_wave)),
    L('barista', "BARISTA", "THE CAFE",
      ["LATTE ART:", "LEAVES, HEARTS,", "ONE RACCOON."],
      ["NAME FOR THE", "CUP?"],
      5, 'INDOOR', dict(),
      dict(cafe=9, bakery=3, transit_hall=2),
      head='bun', torso='apron', act=act(arms=a_hold(["..33", "..11"]))),
    L('baker', "BAKERY CLERK", "THE BAKERY",
      ["SMELLS LIKE", "FRESH BREAD", "ALL DAY LONG."],
      ["BUTTER TARTS", "JUST CAME OUT."],
      3, 'INDOOR', dict(),
      dict(bakery=9, cafe=3, store=2),
      head='paperhat', torso='apron', act=act(arms=a_hold(["3333", "2222"]))),
    L('diner', "DINER", "THE PATIO",
      ["ORDERS THE", "SPECIAL. ALWAYS", "THE SPECIAL."],
      ["TRY THE PEAMEAL", "SANDWICH."],
      6, 'SIT', dict(little_italy=7, danforth=6, chinatown=5, queen_west=4, entertainment=4, kensington=3),
      dict(restaurant=9, pub=6, cafe=4),
      head='short', torso='shirt', act=SEATED_TABLE),
    L('mallshopper', "MALL SHOPPER", "THE MALL",
      ["FIVE BAGS, ONE", "RECEIPT, NO", "REGRETS."],
      ["EVERYTHING WAS", "ON SALE. MOSTLY."],
      5, 'WALK', dict(yonge_dundas=7, downtown=5, queen_west=3),
      dict(mall=9, store=4, electronics=3),
      head='long', torso='longhair', legs='mid', props=('bags',)),
    L('attendant', "TOWER ATTENDANT", "THE CN TOWER",
      ["RIDES THE LIFT", "ALL DAY. LOVES", "THE GLASS FLOOR."],
      ["LOOK DOWN.", "IF YOU DARE."],
      1, 'INDOOR', dict(),
      dict(tower=9, museum=2),
      head='bun', torso='suit', legs='skirt', act=act(arms=a_side)),
    L('traveller', "TRAVELLER", "UNION STATION",
      ["ROLLING BAG,", "TIGHT TRANSFER,", "NO TIME AT ALL."],
      ["WHICH PLATFORM", "FOR MY TRAIN?"],
      6, 'WALK', dict(transit=9, financial=5, waterfront=3, downtown=3, entertainment=2),
      dict(transit_hall=9, tower=2, mall=2),
      head='short', torso='suit', props=('rolling',)),
    L('receptionist', "RECEPTIONIST", "THE LOBBY",
      ["KNOWS WHO IS IN,", "WHO IS OUT AND", "WHO IS LATE."],
      ["SIGN IN HERE,", "PLEASE."],
      6, 'INDOOR', dict(),
      dict(office=9, museum=2, gallery=2, tower=2),
      head='curly', torso='lanyard', act=act(arms=a_phone)),
]


# ---------------------------------------------------------------- validation
def validate(looks, frames):
    keys = [l.key for l in looks]
    assert keys[0] == 'officer', 'the officer must be look 0'
    assert len(set(keys)) == len(keys), 'duplicate look keys'
    assert len(looks) * LOOK_BYTES <= 16384, 'the tile table must fit one ROM bank'
    for look in looks:
        k = look.key
        assert k.isidentifier() and k == k.lower(), k
        assert 0 <= look.pal <= 6, (k, 'palette')
        assert (look.pal == NAVY) == (k == 'officer'), (k, 'navy is for the police only')
        assert look.pal != ORANGE or k == 'builder', (k, 'orange is the courier\'s')
        assert look.behave in BEHAVES, (k, look.behave)
        assert set(look.where) <= set(WHERE), (k, set(look.where) - set(WHERE))
        assert set(look.roles) <= set(ROLES), (k, set(look.roles) - set(ROLES))
        assert all(isinstance(w, int) and 0 <= w <= 9 for w in [*look.where.values(), *look.roles.values()]), k
        assert look.where or look.roles, (k, 'appears nowhere')
        assert (look.behave == 'INDOOR') == (not look.where), (k, 'indoor looks have no street weights')
        texts = [(look.name, NAME), (look.home, LINE)] + [(t, LINE) for t in look.trait + look.talk]
        assert len(look.trait) == 3 and len(look.talk) == 2, (k, 'three trait and two talk lines')
        for text, width in texts:
            assert text and len(text) <= width and set(text) <= FONT, (k, text)
            assert '??' not in text, (k, text, 'no C trigraphs')
        poses = frames[k]
        assert list(poses) == list(POSES), k
        for pose, pair in poses.items():
            assert len(pair) == 2, (k, pose)
            for g in pair:
                assert len(g) == 16 and all(len(r) == 8 for r in g), (k, pose)
                assert all(v in (0, 1, 2, 3) for r in g for v in r), (k, pose)
                assert not any(g[14]) and not any(g[15]), (k, pose, 'rows 14-15 stay blank')
                assert any(any(r) for r in g), (k, pose, 'empty frame')
                if not look.custom and pose != 'ACT':
                    assert any(g[13]), (k, pose, 'people stand on row 13')
    exclaim = sum(1 for l in looks for t in l.trait + l.talk if '!' in t)
    assert exclaim <= 4, 'keep exclamation marks rare'


# ---------------------------------------------------------------- outputs
def tile32(g):
    """One 8x16 OBJ in Game Boy 2bpp: rows in order, low then high plane."""
    out = []
    for row in g:
        lo = hi = 0
        for x, v in enumerate(row):
            lo |= (v & 1) << (7 - x)
            hi |= (v >> 1) << (7 - x)
        out += [lo, hi]
    return out


def look_bytes(poses):
    data = []
    for pose in POSES:
        for g in poses[pose]:
            data += tile32(g)
    assert len(data) == LOOK_BYTES
    return data


def header(looks):
    lines = ['/* Generated by scripts/create_people.py: the NPC look library (people and',
             ' * animals of Toronto). Original pixel art. Do not edit by hand. */',
             '#ifndef TD_PEOPLE_DATA_H', '#define TD_PEOPLE_DATA_H',
             f'#define TD_LOOKS {len(looks)}']
    lines += [f'#define TD_LOOK_{l.key.upper()} {i}' for i, l in enumerate(looks)]
    lines += [f'#define TD_POSE_{p} {i}' for i, p in enumerate(POSES)]
    lines += [f'#define TD_BH_{b} {i}' for i, b in enumerate(BEHAVES)]
    lines.append(f'#define TD_LOOK_BYTES {LOOK_BYTES}')
    lines += ['/* Look l, pose p, frame f: td_look_tiles[((l*4+p)*2+f)*32], one 8x16 OBJ. */',
              '#ifdef TD_PEOPLE_DATA',
              '/* People palette offset from the courier uniform palette (0..6). */',
              'static const UBYTE td_look_pal[TD_LOOKS]={' + ','.join(str(l.pal) for l in looks) + '};',
              'static const UBYTE td_look_behave[TD_LOOKS]={' +
              ','.join(f'TD_BH_{l.behave}' for l in looks) + '};',
              '#endif', '#endif', '']
    return '\n'.join(lines)


def tiles_c(looks, frames):
    lines = ['/* Generated by scripts/create_people.py: NPC look tiles, eight 8x16 OBJs',
             ' * per look (side, front, back, action; two frames each). */',
             '#pragma bank 255', '#include <gbdk/platform.h>', '#include "td_people_data.h"', '',
             'BANKREF(td_look_tiles)', 'const UBYTE td_look_tiles[TD_LOOKS*256]={']
    for look in looks:
        data = look_bytes(frames[look.key])
        lines.append(f'    /* {look.key} */')
        for k in range(0, LOOK_BYTES, 32):
            lines.append('    ' + ','.join(f'0x{b:02X}' for b in data[k:k + 32]) + ',')
    lines += ['};', '']
    return '\n'.join(lines)


def text_c(looks):
    fields = ('name', 'home', 'trait0', 'trait1', 'trait2', 'talk0', 'talk1')
    lines = ['/* Generated by scripts/create_people.py: gallery and talk text for every',
             ' * NPC look. Field 0 name, 1 home, 2-4 trait lines, 5-6 talk lines. */',
             '#pragma bank 255', '#include <string.h>', '#include <gbdk/platform.h>',
             '#include "td_people_data.h"', '']
    lines.append(f'static const char td_people_strings[TD_LOOKS][7][{LINE + 1}]={{')
    for look in looks:
        texts = [look.name, look.home] + look.trait + look.talk
        assert len(texts) == len(fields)
        lines.append('    {' + ','.join('"' + t + '"' for t in texts) + '},' + f' /* {look.key} */')
    lines += ['};', '',
              'void td_people_text(UBYTE look,UBYTE field,char *dest) BANKED',
              '{',
              '    if (look >= TD_LOOKS || field >= 7) {',
              '        dest[0] = 0;',
              '        return;',
              '    }',
              '    strcpy(dest, td_people_strings[look][field]);',
              '}', '']
    return '\n'.join(lines)


def metadata(looks):
    out = {'note': 'Generated by scripts/create_people.py. NPC looks: people and animals of '
                   'Toronto, original pixel art. palette is the offset from the courier uniform '
                   'OBJ palette; where and roles are 0-9 weights.',
           'poses': list(POSES), 'behaviours': list(BEHAVES), 'where_tags': list(WHERE),
           'role_tags': list(ROLES),
           'palettes': [{'offset': i, 'name': n, 'colours': list(c)} for i, (n, c) in enumerate(PALETTES)],
           'looks': []}
    for i, l in enumerate(looks):
        out['looks'].append({'key': l.key, 'index': i, 'name': l.name, 'home': l.home, 'trait': l.trait,
                             'talk': l.talk, 'palette': l.pal, 'behave': l.behave,
                             'where': {k: l.where[k] for k in WHERE if k in l.where},
                             'roles': {k: l.roles[k] for k in ROLES if k in l.roles}})
    return json.dumps(out, indent=2) + '\n'


def build():
    frames = {l.key: l.frames() for l in LOOKS}
    validate(LOOKS, frames)
    files = {OUT_H: header(LOOKS), OUT_TILES: tiles_c(LOOKS, frames), OUT_TEXT: text_c(LOOKS),
             OUT_JSON: metadata(LOOKS)}
    return files, frames


# ---------------------------------------------------------------- preview
def preview(path, frames, scale=4):
    """Every look's eight frames at `scale` in its own palette on a light
    background, with its index, key and name; then the front views at 2x on
    a pavement tone for a sense of the crowd at near game size."""
    from PIL import Image, ImageDraw
    cols = 3
    cell_w, cell_h = 8 * scale * 8 + 7 * 4 + 16, 16 * scale + 22
    rows = (len(LOOKS) + cols - 1) // cols
    crowd_h = 16 * 2 * ((len(LOOKS) + 23) // 24) + 40
    sheet = Image.new('RGB', (cols * cell_w + 16, rows * cell_h + 16 + crowd_h), (236, 232, 222))
    d = ImageDraw.Draw(sheet)

    def rgb(hexstr):
        return tuple(bytes.fromhex(hexstr))

    def stamp(g, pal, x0, y0, sc):
        colours = [None] + [rgb(c) for c in PALETTES[pal][1]]
        for y in range(16):
            for x in range(8):
                v = g[y][x]
                if v:
                    d.rectangle([x0 + x * sc, y0 + y * sc, x0 + (x + 1) * sc - 1, y0 + (y + 1) * sc - 1],
                                fill=colours[v])

    for i, look in enumerate(LOOKS):
        cx, cy = 8 + (i % cols) * cell_w, 8 + (i // cols) * cell_h
        d.text((cx, cy), f'{i:2d} {look.key}  {look.name}  [{look.behave} p{look.pal}]', fill=(40, 40, 40))
        k = 0
        for pose in POSES:
            for g in frames[look.key][pose]:
                x0 = cx + k * (8 * scale + 4) + (8 if k >= 6 else 0)
                d.rectangle([x0, cy + 14, x0 + 8 * scale - 1, cy + 14 + 16 * scale - 1], fill=(250, 248, 240))
                stamp(g, look.pal, x0, cy + 14, scale)
                k += 1
    y = 16 + rows * cell_h
    d.rectangle([0, y, sheet.width, sheet.height], fill=(150, 150, 140))
    d.text((8, y + 4), 'front and side views at 2x on pavement', fill=(20, 20, 20))
    for i, look in enumerate(LOOKS):
        x0, y0 = 8 + (i % 24) * 40, y + 20 + (i // 24) * 34
        stamp(frames[look.key]['FRONT'][0], look.pal, x0, y0, 2)
        stamp(frames[look.key]['SIDE'][0], look.pal, x0 + 18, y0, 2)
    sheet.save(path)


def main():
    files, frames = build()
    if '--preview' in sys.argv:
        out = Path(sys.argv[sys.argv.index('--preview') + 1])
        preview(out, frames)
        print(f'Wrote the people preview to {out}.')
        return
    stale = [p for p, text in files.items() if not p.exists() or p.read_text() != text]
    summary = f'{len(LOOKS)} looks, {len(LOOKS) * 8} sprite tiles ({len(LOOKS) * LOOK_BYTES} bytes)'
    if '--check' in sys.argv:
        assert not stale, 'People outputs are stale; run scripts/create_people.py: ' + \
            ', '.join(str(p.relative_to(ROOT)) for p in stale)
        print(f'NPC looks match their original designs: {summary}.')
        return
    for path in stale:
        path.write_text(files[path])
    print(f'Wrote {len(stale)} people output(s): {summary}.')


if __name__ == '__main__':
    main()


