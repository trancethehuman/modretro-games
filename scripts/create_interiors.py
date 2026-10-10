"""Draw the building interiors as GB Studio scenes.

Each interior is a TORONTO scene of its own: a four-shade background PNG
with per-tile CGB palette attributes (assets/backgrounds/interior_*.png and
its .gbsres), a collision grid (0 open floor, 15 solid), seven background
palettes from the interior library (project/palettes/interior_*.gbsres)
and a scene resource. content/interiors.json describes, in scene pixels,
where the player arrives and leaves, the points of interest and the NPC
spots that the engine reads.

Rooms are drawn north-up like the city: floors straight from above, a
visible north wall face 2-3 tiles tall for windows, paintings, shelves and
signs, furniture with a short front face. All art is original; landmark
rooms are original interpretations of real places, and every shop name is
fictional.

  python3 scripts/create_interiors.py            write everything
  python3 scripts/create_interiors.py --check    verify the files match
  python3 scripts/create_interiors.py --preview DIR
                                                 also write 3x colour renders
"""
import json
import sys
import uuid
from pathlib import Path

from PIL import Image

import city_kit
import interior_kit as K
from interior_kit import CREAM, INK, SLATE, SKY, TEAL, TERRA, SAND, ROSE, GRASS, PINE

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'project'
SCENES = PROJECT / 'project/scenes'
BACKGROUNDS = PROJECT / 'assets/backgrounds'
PALETTE_DIR = PROJECT / 'project/palettes'
META = ROOT / 'content/interiors.json'
NAMESPACE = uuid.UUID('3b8e1f60-2c4d-5a9e-8f17-6d2c0b4e9a71')
TILE_BUDGET = city_kit.SCENE_TILE_BUDGET
CHARSET = set(" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/.-+?<>$%#=,'!()&")
ROLES = {'viewer', 'guard', 'docent', 'shopper', 'teen', 'clerk', 'cashier', 'barista', 'diner',
         'tourist', 'commuter', 'attendant', 'chef', 'patron', 'office', 'visitor'}
BEHAVES = {'wander', 'gaze', 'sit', 'counter', 'patrol', 'window'}
KINDS = {'elevator', 'desk', 'plaque', 'exhibit', 'window', 'arch_prev', 'arch_next', 'counter', 'painting', 'screen'}

# Shared colours of the library (interior_kit.PALETTES).
PLASTER, PLASTER_D = 'CDBB9E', '8C7A66'
WOOD_D = '8E5A3A'
STONE, STONE_D = 'BFC1BA', '7C8590'
GOLD, GOLD_D = 'D8A83C', '8A6424'
RED, RED_D = 'D4483A', '7E2A2A'
SCR_L, SCR, SCR_D = 'E8F8F0', '6CCCEC', '2C64B0'
JADE, JADE_D = '84C8A0', '2E7258'
VIO, VIO_D = 'A890CA', '5C4A8A'
SUN, PLUM = 'E68A48', '8C3A58'
NIGHT, NIGHT_D = '5C6EB2', '2A3468'
ROSE_D = '7A5A62'
SAND_D = '8E7C4C'
MINT, MINT_D = '9CD4C4', '3E8A80'
DUSK, DUSK_D = '9AA2B4', '4A5468'
WAL_L, WAL, WAL_D = 'D8A878', '9A6640', '5A3424'
BOTTLE, BOTTLE_D = '6EA078', '2E5A44'


# ===================================================================== shell
class Wall:
    """North wall face: base colour, baseboard trim, the cap's inner line,
    an optional wainscot (colour, height in px, rail colour) and an optional
    wallpaper pattern (rows, colour map)."""

    def __init__(self, base, trim, line, wainscot=None, paper=None):
        self.base, self.trim, self.line = base, trim, line
        self.wainscot, self.paper = wainscot, paper


def shell(r, wall, wall_rows, floor, mat=(SLATE, INK), door=(CREAM, INK)):
    """Border caps, north wall face, floor, south doorway and door mat;
    sets the entrance and exit. Returns the first floor pixel row."""
    w, h = r.w, r.h
    r.rect(0, 0, w, h, INK)
    face_h = wall_rows * 8
    fy = 8 + face_h
    # Wall face.
    r.rect(8, 8, w - 16, face_h, wall.base)
    if wall.paper:
        rows, cmap = wall.paper
        r.fill(8, 10, w - 16, face_h - 4, rows, cmap)
    if wall.wainscot:
        c, ht, rail = wall.wainscot
        r.rect(8, fy - ht, w - 16, ht, c)
        r.hline(8, fy - ht, w - 16, rail)
    r.hline(8, 8, w - 16, INK)
    r.rect(8, fy - 3, w - 16, 2, wall.trim)
    r.hline(8, fy - 1, w - 16, INK)
    # Caps: the top of the walls seen from above, a light inner edge.
    r.rect(0, 5, w, 2, wall.line)
    r.rect(5, 5, 2, h - 10, wall.line)
    r.rect(w - 7, 5, 2, h - 10, wall.line)
    r.rect(5, h - 7, w - 10, 2, wall.line)
    # Floor.
    floor(r, 8, fy, w - 16, h - 8 - fy)
    r.block(0, 0, r.tw, wall_rows + 1)
    r.block(0, 0, 1, r.th)
    r.block(r.tw - 1, 0, 1, r.th)
    r.block(0, r.th - 1, r.tw, 1)
    # South doorway (solid: the exit is the mat inside it).
    dx = (r.tw // 2 - 1) * 8
    light, line = door
    r.rect(dx, h - 8, 16, 8, light)
    r.hline(dx, h - 4, 16, line)
    r.hline(dx, h - 1, 16, line)
    r.rect(dx - 2, h - 8, 2, 8, INK)
    r.rect(dx + 16, h - 8, 2, 8, INK)
    door_mat(r, mat)
    return fy


def door_mat(r, mat=(SLATE, INK)):
    """The exit mat inside the south doorway (covers its two tiles); sets
    the exit rectangle and the entrance just north of it."""
    dx, my = (r.tw // 2 - 1) * 8, r.h - 16
    m, md = mat
    r.rect(dx, my, 16, 8, m)
    r.frame(dx, my, 16, 8, md)
    for x in range(dx + 3, dx + 14, 2):
        r.vline(x, my + 2, 4, md)
    r.exit = [dx, my, dx + 15, my + 7]
    r.entrance = [dx + 8, my - 1]


# ------------------------------------------------------------------- floors
def planks(dark=WOOD_D, base=TERRA):
    """Long boards 7 px wide with a seam, joints staggered, sparse grain."""
    a = 'd' + 'b' * 31
    b = 'b' * 16 + 'd' + 'b' * 15
    pat = [a, 'd' + 'b' * 9 + 'gg' + 'b' * 20, a, a[:24] + 'ggg' + a[27:], a, a, a, 'D' * 32,
           b, b[:4] + 'ggg' + b[7:], b, b, b[:22] + 'gg' + b[24:], b, b, 'D' * 32]

    def paint(r, x, y, w, h):
        r.fill(x, y, w, h, pat, {'b': base, 'd': dark, 'g': dark, 'D': dark}, soft={'d': base, 'g': base, 'D': base})
    return paint


def checker(a, b, size=8):
    pat = ['a' * size + 'b' * size] * size + ['b' * size + 'a' * size] * size

    def paint(r, x, y, w, h):
        r.fill(x, y, w, h, pat, {'a': a, 'b': b})
    return paint


def slabs(base, grout, speck=None, size=16):
    pat = []
    for j in range(size):
        row = ''
        for i in range(size):
            if i == 0 or j == 0:
                row += 'g'
            elif speck and (i * 7 + j * 3) % 23 == 0:
                row += 's'
            else:
                row += 'b'
        pat.append(row)

    def paint(r, x, y, w, h):
        cmap = {'b': base, 'g': grout, 's': speck or base}
        r.fill(x, y, w, h, pat, cmap, soft={'g': base, 's': base})
    return paint


def pattern_floor(rows, cmap, soft=None):
    def paint(r, x, y, w, h):
        r.fill(x, y, w, h, rows, cmap, soft=soft)
    return paint


# ---------------------------------------------------------------- furniture
def counter(r, x, y, w, h, top, edge_c, face, face_d, front=6):
    """A counter seen from above with its front face (front px tall)."""
    r.rect(x, y, w, h, top)
    r.frame(x, y, w, h - front + 1, INK)
    r.rect(x, y + h - front, w, front, face)
    r.hline(x, y + h - front, w, edge_c)
    r.hline(x, y + h - 1, w, INK)
    r.vline(x, y, h, INK)
    r.vline(x + w - 1, y, h, INK)
    for xx in range(x + 4, x + w - 2, 8):
        r.vline(xx, y + h - front + 2, front - 3, face_d)


def stool(r, cx, cy, seat, dark):
    r.stamp(cx - 3, cy - 3, ['.###.', '#sss#', '#sds#', '#sss#', '.###.'], {'#': INK, 's': seat, 'd': dark})


def plant_box(r, x, y, w, h, box=WOOD_D, rim=INK):
    """A square planter of leafy plants covering whole tiles."""
    r.rect(x, y, w, h, PINE)
    for j in range(h):
        for i in range(w):
            v = (i * 5 + j * 3 + (i * j) % 4) % 7
            if v in (0, 3):
                r.px(x + i, y + j, GRASS)
            elif v == 5 and (i + j) % 2:
                r.px(x + i, y + j, GRASS)
    for j in range(2, h - 2, 4):
        for i in range(2 + (j // 4) % 2 * 2, w - 2, 4):
            r.stamp(x + i - 1, y + j - 1, ['.l.', 'lgl', '.l.'], {'l': GRASS, 'g': CREAM})
    r.frame(x, y, w, h, rim)


def table_round(r, cx, cy, top, edge=INK, shade=None):
    rows = ['..######..',
            '.#tttttt#.',
            '#tttttttt#',
            '#tttttttt#',
            '#tttttttt#',
            '#ssssssss#',
            '.#ssssss#.',
            '..######..']
    r.stamp(cx - 5, cy - 4, rows, {'#': edge, 't': top, 's': shade or top})


def chair(r, x, y, seat, dark, facing='s'):
    """7x7 chair seen from above; the back is on the side away from facing."""
    rows = ['#######', '#ddddd#', '#######', '#sssss#', '#sssss#', '#sssss#', '.#####.']
    if facing == 'n':
        rows = rows[::-1]
    elif facing in 'ew':
        rows = [''.join(row[i] for row in rows) for i in range(7)]
        if facing == 'w':
            rows = [row[::-1] for row in rows]
    r.stamp(x, y, rows, {'#': INK, 's': seat, 'd': dark})


# ======================================================================= rooms
def cafe():
    """Generic cafe: back bar with an espresso machine, front counter with a
    pastry case, chalkboard menus, small tables, a window bar, plants."""
    r = K.Room('cafe', 20, 18, ['wood', 'stone', 'green', 'leaf', 'brass', 'glass'], 'CAFE')
    wall = Wall(CREAM, WOOD_D, TERRA, wainscot=(TERRA, 8, WOOD_D))
    shell(r, wall, 3, planks(), mat=(WOOD_D, INK))
    for x in range(16, 152, 16):
        r.vline(x, 26, 3, WOOD_D)
    # Chalkboard menus.
    chalkboard(r, 16, 9, 32, 14, 'MENU')
    chalkboard(r, 56, 9, 16, 14, None)
    r.stamp(59, 12, ['..#..#..', '.#..#...', '..#..#..', '#########', '#.......#', '#.......##', '#.......#.', '.#.....#..', '..#####...'][:9],
            {'#': CREAM})
    # Shelf of coffee bags and cups.
    wall_shelf(r, 96, 18, 32, ['bag', 'bag', 'cup', 'cup', 'bag'])
    clock(r, 140, 12)
    r.rect(78, 10, 14, 12, INK)
    r.rect(79, 11, 12, 10, GOLD)
    r.stamp(80, 13, ['.#..#.', '..#..#', '######', '#cccc##', '#cccc#.', '.####.'], {'#': GOLD_D, 'c': CREAM})
    # Back bar: steel counter, espresso machine, grinder, cups.
    r.rect(8, 32, 80, 16, STONE)
    r.hline(8, 32, 80, CREAM)
    r.rect(8, 42, 80, 6, STONE_D)
    r.hline(8, 42, 80, INK)
    r.hline(8, 47, 80, INK)
    r.vline(87, 32, 16, INK)
    for xx in range(12, 88, 8):
        r.vline(xx, 43, 4, STONE)
    espresso(r, 16, 22)
    grinder(r, 52, 33)
    for x in (66, 72, 78):
        r.stamp(x, 37, ['.##.', '#cc#', '#cc#', '.##.'], {'#': INK, 'c': CREAM})
    r.block(1, 4, 10, 2)
    # Front counter: marble top, panelled front, register, pastry case.
    r.rect(8, 56, 40, 8, CREAM)
    for x in (12, 26, 38):
        r.stamp(x, 58, ['.s..', 's.s.', '...s'], {'s': STONE})
    r.hline(8, 56, 40, INK)
    r.hline(8, 63, 40, STONE_D)
    r.rect(8, 64, 40, 8, TERRA)
    r.hline(8, 64, 40, WOOD_D)
    r.hline(8, 71, 40, INK)
    for x in range(14, 48, 8):
        r.rect(x, 66, 5, 4, WOOD_D)
        r.rect(x + 1, 67, 3, 2, TERRA)
    register(r, 18, 56)
    pastry_case(r, 48, 56, 32, 16)
    r.vline(87, 56, 16, INK)
    r.rect(80, 56, 8, 8, CREAM)
    r.hline(80, 56, 8, INK)
    r.hline(80, 63, 8, STONE_D)
    r.vline(87, 56, 16, INK)
    r.rect(80, 64, 8, 8, TERRA)
    r.hline(80, 64, 7, WOOD_D)
    r.hline(80, 71, 8, INK)
    r.vline(87, 56, 16, INK)
    r.stamp(81, 57, ['.##.', '#cc#', '#cc#', '.##.'], {'#': INK, 'c': CREAM})
    r.block(1, 7, 10, 2)
    # Tables with chairs (chairs stay open: seated NPCs use them).
    for cx, cy in ((32, 92), (32, 116), (120, 116)):
        bistro_set(r, cx, cy)
    bistro_set(r, 120, 60)
    # Window bar along the east wall with stools.
    r.rect(144, 80, 8, 40, TERRA)
    r.frame(144, 80, 8, 40, INK)
    r.vline(145, 81, 38, CREAM)
    for y in range(80, 120, 8):
        r.stamp(146, y + 2, ['c.', '##'], {'c': CREAM, '#': WOOD_D})
    r.block(18, 10, 1, 5)
    for y in (84, 100, 112):
        stool(r, 138, y, TERRA, WOOD_D)
    side_window(r, 'e', 10, 5)
    side_window(r, 'w', 11, 4)
    plant_box(r, 136, 32, 16, 16)
    r.block(17, 4, 2, 2)
    plant_box(r, 8, 120, 16, 16)
    r.block(1, 15, 2, 2)
    r.point('counter', 2, 9, ['CAFE', 'ESPRESSO, TEA,', 'BUNS FROM $3'], dx=4)
    r.point('screen', 6, 9, ['MENU', 'OAT MILK? SURE.', 'DECAF? WHY.'], dx=4)
    r.npc('barista', 'counter', 3, 6, 's', area=(1, 6, 10, 1))
    r.npc('patron', 'sit', 2, 11, 'e')
    r.npc('patron', 'sit', 16, 7, 'w')
    r.npc('patron', 'sit', 17, 12, 'e', dx=-2)
    r.npc('patron', 'wander', 8, 12, 'n', area=(7, 10, 5, 5))
    return r


def chalkboard(r, x, y, w, h, title):
    r.rect(x, y, w, h, BOTTLE_D)
    r.frame(x, y, w, h, INK)
    r.frame(x + 1, y + 1, w - 2, h - 2, BOTTLE)
    if title:
        r.text_center(x + w // 2, y + 3, title, CREAM)
        for yy in (y + 10,):
            for xx in range(x + 4, x + w - 4, 2):
                r.px(xx, yy, BOTTLE)
            r.px(x + w - 6, yy, CREAM)
            r.px(x + w - 5, yy, CREAM)


def wall_shelf(r, x, y, w, items):
    r.rect(x, y, w, 2, TERRA)
    r.hline(x, y + 2, w, WOOD_D)
    cx = x + 1
    for it in items:
        if it == 'bag':
            r.stamp(cx, y - 7, ['.##.', '#bb#', '#bb#', '#lb#', '#bb#', '#bb#', '####'], {'#': INK, 'b': WOOD_D, 'l': CREAM})
            cx += 6
        else:
            r.stamp(cx, y - 4, ['###.', '#c##', '#c#.', '###.'], {'#': INK, 'c': CREAM})
            cx += 6


def clock(r, x, y):
    r.stamp(x, y, ['.####.', '#cccc#', '#c#cc#', '#c##c#', '#cccc#', '.####.'], {'#': INK, 'c': CREAM})


def espresso(r, x, y):
    """24 x 24 espresso machine against the wall (covers its wall tiles)."""
    rows = ['....##...##...##........',
            '...#cc#.#cc#.#cc#.......',
            '########################',
            '#llllllllllllllllllllll#',
            '#ssssssssssssssssssssss#',
            '########################',
            '#dddddddddddddddddddddd#',
            '#dd##dddddddddddddd##dd#',
            '#d#cc#ddd#ddd#dddd#cc#d#',
            '#d#c##ddd#ddd#dddd#c##d#',
            '#dd##dddddddddddddd##dd#',
            '#dddddddddddddddddddddd#',
            '#ll####llllllllll####ll#',
            '#lll##llllllllllll##lll#',
            '#llll#lllllllllll#lllll#',
            '#llll##llllllllll##llll#',
            '#llllllllllllllllllllll#',
            '#lll#c#lllllllllll#c#ll#',
            '#lll#c#lllllllllll#c#ll#',
            '#lll###lllllllllll###ll#',
            '#dddddddddddddddddddddd#',
            '#d#d#d#d#d#d#d#d#d#d#dd#',
            '########################',
            'ssssssssssssssssssssssss']
    r.stamp(x, y, rows, {'#': INK, 'l': STONE, 's': STONE_D, 'd': STONE_D, 'c': CREAM})


def grinder(r, x, y):
    r.stamp(x, y, ['.####.', '#ssss#', '.#ss#.', '..##..', '.####.', '#dddd#', '#dccd#', '#dddd#', '######'],
            {'#': INK, 's': STONE_D, 'd': STONE, 'c': CREAM})


def register(r, x, y):
    r.stamp(x, y, ['.######.', '#dddddd#', '#dccccd#', '#dddddd#', '########', '.#ssss#.', '.######.'],
            {'#': INK, 'd': STONE_D, 'c': CREAM, 's': STONE})


def pastry_case(r, x, y, w, h):
    """A warm-lit glass case of buns and tarts (brass palette, whole tiles)."""
    r.rect(x, y, w, h, CREAM)
    r.frame(x, y, w, 9, INK)
    r.rect(x, y + 9, w, h - 9, GOLD_D)
    r.frame(x, y + 8, w, h - 8, INK)
    r.rect(x + 2, y + 10, w - 4, h - 13, CREAM)
    r.hline(x + 1, y + 1, w - 2, CREAM)
    buns = ['.##.', '#gg#', '#gG#', '.##.']
    tart = ['####', '#gg#', '####']
    for i, xx in enumerate(range(x + 2, x + w - 4, 5)):
        r.stamp(xx, y + 2, buns if i % 2 == 0 else tart, {'#': GOLD_D, 'g': GOLD, 'G': GOLD_D})
        r.stamp(xx, y + 11, tart if i % 2 == 0 else buns[:3], {'#': GOLD_D, 'g': GOLD, 'G': GOLD_D})
    r.hline(x, y + h - 1, w, INK)


def bistro_set(r, cx, cy):
    """Round marble table centred on a tile corner, a chair each side; the
    table's tiles are solid, the chairs open."""
    table_round(r, cx, cy, CREAM)
    r.hline(cx - 3, cy + 1, 6, 'E7DECC')
    chair(r, cx - 16, cy - 3, TERRA, WOOD_D, 'e')
    chair(r, cx + 9, cy - 3, TERRA, WOOD_D, 'w')
    r.block((cx - 5) // 8, (cy - 4) // 8, 2, 1)


def side_window(r, side, ty, n):
    """A window in a side wall cap (seen from above: glass in the wall)."""
    x = r.w - 8 if side == 'e' else 0
    for j in range(n):
        y = (ty + j) * 8
        r.rect(x, y, 8, 8, INK)
        r.rect(x + 2, y, 4, 8, SKY)
        r.vline(x + 3, y, 8, CREAM)
        r.hline(x + 2, y, 4, SLATE)
    r.hline(x + 2, (ty + n) * 8 - 1, 4, SLATE)


# ------------------------------------------------------------ shared pieces
def clear_over(r, tx, ty, tw, th, c=CREAM):
    """Plain floor under overhead art (priority tiles show the player
    through shade 0 only)."""
    r.rect(tx * 8, ty * 8, tw * 8, th * 8, c)


def south_glass(r, tx, n):
    """Glass panes in the south wall cap beside the door."""
    y = r.h - 8
    for i in range(n):
        x = (tx + i) * 8
        r.rect(x, y, 8, 8, INK)
        r.rect(x, y + 2, 8, 4, SKY)
        r.hline(x, y + 3, 8, CREAM)
        r.vline(x, y + 2, 4, SLATE)


def elevator(r, x, y, up=True):
    """A 24 x 24 elevator portal on the north wall face: gold trim, glass
    doors, a floor indicator (covers its nine tiles)."""
    r.rect(x, y, 24, 24, GOLD)
    r.hline(x, y, 24, INK)
    r.vline(x, y, 24, INK)
    r.vline(x + 23, y, 24, INK)
    r.hline(x + 1, y + 1, 22, CREAM)
    r.rect(x + 8, y + 2, 8, 4, INK)
    r.stamp(x + 9, y + 3, ['.c.', 'ccc'] if up else ['ccc', '.c.'], {'c': CREAM})
    r.px(x + 14, y + 3, GOLD)
    r.rect(x + 3, y + 7, 18, 17, INK)
    r.rect(x + 4, y + 8, 16, 15, SKY)
    r.vline(x + 11, y + 8, 15, INK)
    r.vline(x + 12, y + 8, 15, INK)
    for i in range(5):
        r.px(x + 5 + i, y + 16 - i * 2, CREAM)
        r.px(x + 5 + i, y + 17 - i * 2, CREAM)
        r.px(x + 14 + i, y + 18 - i * 2, CREAM)
    r.rect(x + 4, y + 8, 16, 1, CREAM)
    r.hline(x, y + 23, 24, INK)
    r.rect(x + 1, y + 6, 22, 1, INK)


def stanchions(r, x0, x1, y, rope=RED, rope_d=RED_D, post=INK):
    """A rope rail across a tile row (posts every 16 px)."""
    for x in range(x0, x1 + 1, 16):
        r.stamp(x - 1, y + 1, ['.c.', '#c#', '.#.', '.#.', '.#.', '###'], {'#': post, 'c': CREAM})
    for x in range(x0 + 2, x1 - 1):
        k = (x - x0) % 16
        sag = 1 if 4 <= k <= 11 else 0
        r.px(x, y + 2 + sag, rope)
        r.px(x, y + 3 + sag, rope_d)


def stanchions_v(r, x, y0, y1, rope=RED, rope_d=RED_D, post=INK):
    for y in range(y0, y1 + 1, 16):
        r.stamp(x - 1, y - 4, ['.c.', '#c#', '.#.', '.#.', '###'], {'#': post, 'c': CREAM})
    for y in range(y0 + 1, y1):
        if (y - y0) % 16 in (0, 15, 14, 13, 12):
            continue
        r.px(x - 1, y, rope)
        r.px(x, y, rope_d)


TOWER = ['.......#.......',
         '.......#.......',
         '.......#.......',
         '.......#.......',
         '......#s#......',
         '......#s#......',
         '.....#####.....',
         '.....#lsd#.....',
         '.....#####.....',
         '......#s#......',
         '......#s#......',
         '......#s#......',
         '...#########...',
         '..#lllssssdd#..',
         '.#llsssssssdd#.',
         '###############',
         '#c#c#c#c#c#c#c#',
         '###############',
         '.#llsssssssdd#.',
         '..##sssssss##..',
         '....#######....',
         '.....#lsd#.....',
         '.....#lsd#.....',
         '.....#lsd#.....',
         '.....#lsd#.....',
         '....#llsdd#....',
         '....#llsdd#....',
         '....#llsdd#....',
         '....#llsdd#....',
         '...#lllsddd#...',
         '...#lllsddd#...',
         '...#lllsddd#...',
         '..#llllsdddd#..',
         '..#llll#dddd#..',
         '.#llll#s#dddd#.',
         '#llll#.#.#dddd#',
         '#####.###.#####']
TOWER_ICON = ['...#...', '...#...', '...#...', '..###..', '...#...', '...#...', '.#####.', '#######',
              '.#####.', '...#...', '...#...', '...#...', '..###..', '..###..', '.##.##.', '##...##']


def tower_model(r, cx, base_y):
    """An original miniature of the tower: mast, upper pod, main pod with
    its window band, tapering shaft and three splayed legs (37 px)."""
    r.stamp(cx - 7, base_y - len(TOWER) + 1, TOWER, {'#': INK, 'l': CREAM, 's': STONE, 'd': STONE_D, 'c': CREAM})


def plinth(r, x, y, w, h, top=CREAM, face=STONE, edge=STONE_D, plaque=True):
    """A plinth seen from above with its front face (h px, face 5 px)."""
    r.rect(x, y, w, h, top)
    r.frame(x, y, w, h, INK)
    r.rect(x + 1, y + h - 6, w - 2, 5, face)
    r.hline(x + 1, y + h - 6, w - 2, INK)
    r.hline(x + 1, y + h - 5, w - 2, edge)
    if plaque:
        r.rect(x + w // 2 - 3, y + h - 4, 6, 2, CREAM)
        r.hline(x + w // 2 - 2, y + h - 3, 4, edge)


def planter(r, x, y, w=16, h=16, tree=True):
    """A square planter with a small tree or shrubs, seen from above."""
    r.rect(x, y, w, h, INK)
    r.rect(x + 1, y + 1, w - 2, h - 2, PINE)
    for j in range(1, h - 1):
        for i in range(1, w - 1):
            if (i * 3 + j * 5 + i * j) % 7 in (0, 4):
                r.px(x + i, y + j, GRASS)
    if tree:
        r.stamp(x + w // 2 - 6, y + h // 2 - 6, ['...######...', '..#gggglg#..', '.#ggllgggg#.', '#gglgggglgg#',
                                                  '#gggggllggg#', '#glgggggggg#', '#ggggglgglg#', '#gglgggggg##',
                                                  '.#gggglggg#.', '.##gggggg##.', '...######...'],
                {'#': INK, 'g': GRASS, 'l': CREAM})


def bench(r, x, y, w, seat=TERRA, dark=WOOD_D, legs=INK):
    """A slatted bench seen from above (w x 8 px)."""
    r.rect(x, y + 1, w, 6, seat)
    r.frame(x, y + 1, w, 6, legs)
    r.hline(x + 1, y + 3, w - 2, dark)
    r.hline(x + 1, y + 5, w - 2, dark)
    r.hline(x, y + 7, w, dark)


def cn_base():
    """CN Tower base lobby: glass entrance doors, ticket desk, security arch
    and rope rails, three glass elevators with gold trim, a gift shop of
    tiny towers, a model of the tower on a plinth."""
    r = K.Room('cn_base', 24, 18, ['stone', 'lift', 'red', 'dusk', 'glass', 'leaf', 'wood'], 'CN TOWER', 'landmark')
    wall = Wall(STONE, STONE_D, STONE_D)
    shell(r, wall, 3, slabs(CREAM, STONE), mat=(DUSK_D, INK))
    for x in range(8, 184, 16):
        r.vline(x, 9, 19, STONE_D)
    r.rect(8, 9, 176, 1, CREAM)
    south_glass(r, 7, 4)
    south_glass(r, 13, 4)
    # Elevators and call buttons.
    for tx in (7, 11, 15):
        elevator(r, tx * 8, 8)
    for tx in (10, 14):
        r.stamp(tx * 8 + 2, 15, ['####', '#cc#', '#..#', '#cc#', '####'], {'#': INK, 'c': CREAM, '.': STONE_D})
    # Sign to the LookOut (dusk panel) and an elevation drawing of the tower.
    r.rect(16, 8, 40, 16, DUSK_D)
    r.frame(16, 8, 40, 16, INK)
    r.text(19, 11, 'LOOKOUT', CREAM)
    r.text(19, 17, '346 M', DUSK)
    r.stamp(45, 17, ['..#..', '.###.', '#####'], {'#': CREAM})
    r.rect(144, 9, 32, 20, CREAM)
    r.frame(144, 9, 32, 20, INK)
    r.stamp(148, 11, TOWER_ICON, {'#': INK})
    for y, label, ly in ((11, '553', 11), (18, '346', 18)):
        r.hline(156, ly + 2, 3, STONE)
        r.text(160, y, label, INK)
    r.text(160, 22, 'M', STONE_D)
    # Rope rail with a security arch at the opening.
    stanchions(r, 12, 76, 48)
    stanchions(r, 116, 180, 48)
    r.block(1, 6, 10, 1)
    r.block(13, 6, 10, 1)
    clear_over(r, 11, 6, 2, 1)
    for x in (81, 104):
        r.rect(x, 48, 7, 8, DUSK)
        r.frame(x, 48, 7, 8, INK)
        r.vline(x + 1, 49, 6, CREAM)
        r.px(x + 3, 51, DUSK_D)
    with r.overhead():
        r.stamp(88, 49, ['#' * 16, 'd' * 16, 'D' * 16, '#' * 16], {'#': INK, 'd': DUSK, 'D': DUSK_D})
    # Queue funnel.
    stanchions_v(r, 76, 60, 76)
    stanchions_v(r, 124, 60, 76)
    r.block(9, 7, 1, 2)
    r.block(15, 7, 1, 2)
    # Ticket desk.
    r.rect(16, 64, 48, 16, DUSK)
    r.frame(16, 64, 48, 9, INK)
    r.rect(16, 72, 48, 8, DUSK_D)
    r.frame(16, 72, 48, 8, INK)
    r.text_center(40, 74, 'TICKETS', CREAM)
    for x in (22, 38, 52):
        r.stamp(x, 65, ['######', '#cccc#', '#cccc#', '######', '..##..'], {'#': INK, 'c': CREAM})
    r.block(2, 8, 6, 2)
    # Model of the tower on its plinth (the player walks behind it).
    plinth(r, 88, 96, 16, 16)
    clear_over(r, 11, 8, 2, 4)
    tower_model_over(r, 96, 102)
    r.block(11, 12, 2, 2)
    # Gift shop: wall shelf, glass island of tiny towers, counter.
    gift_shelf(r, 168, 64, 16, 48)
    r.block(21, 8, 2, 6)
    glass_island(r, 128, 72, 24, 16)
    r.block(16, 9, 3, 2)
    r.rect(128, 104, 32, 16, TERRA)
    r.frame(128, 104, 32, 9, INK)
    r.rect(128, 112, 32, 8, WOOD_D)
    r.frame(128, 112, 32, 8, INK)
    r.text_center(144, 114, 'GIFTS', CREAM)
    r.stamp(132, 105, ['######', '#cccc#', '######'], {'#': INK, 'c': CREAM})
    r.stamp(146, 105, ['.#.', '#c#', '.#.', '#c#', '###'], {'#': INK, 'c': CREAM})
    r.block(16, 13, 4, 2)
    # Benches, planters.
    bench(r, 16, 104, 32)
    r.block(2, 13, 4, 1)
    planter(r, 64, 120)
    planter(r, 112, 120)
    r.block(8, 15, 2, 2)
    r.block(14, 15, 2, 2)
    # Points and people.
    for tx in (8, 12, 16):
        r.point('elevator', tx, 4, ['LOOKOUT 346 M', 'GLASS ELEVATOR', '58 SECONDS UP'])
    r.point('desk', 4, 10, ['TICKETS', 'LOOKOUT LEVEL', 'AND GLASS FLOOR'])
    r.point('exhibit', 11, 14, ['TOWER MODEL', '553 M TALL', 'BUILT 1973-76'], dx=4)
    r.point('counter', 17, 15, ['GIFT SHOP', 'TINY TOWERS', '$12 EACH'], dx=4)
    r.point('plaque', 19, 4, ['THE TOWER', 'LOOKOUT AT 346 M', 'MAST TO 553 M'])
    r.npc('attendant', 'counter', 4, 7, 's', area=(2, 7, 6, 1))
    r.npc('guard', 'patrol', 10, 10, 's', path=[(10, 10), (13, 10), (13, 9), (10, 9)])
    r.npc('clerk', 'counter', 17, 12, 's', area=(16, 12, 4, 1))
    r.npc('tourist', 'gaze', 13, 14, 'n')
    r.npc('tourist', 'wander', 6, 11, 'n', area=(1, 10, 9, 5))
    r.npc('visitor', 'gaze', 7, 11, 'e')
    r.npc('tourist', 'wander', 4, 4, 'n', area=(1, 4, 22, 2))
    r.npc('shopper', 'wander', 19, 11, 'e', area=(15, 11, 6, 1))
    return r


def tower_model_over(r, cx, base_y):
    """The tower model drawn as overhead art (priority) above its plinth
    tiles; within the plinth tiles it is ordinary background."""
    with r.overhead((base_y // 8) * 8):
        tower_model(r, cx, base_y)


def gift_shelf(r, x, y, w, h):
    """A white display unit along a side wall with rows of tiny towers."""
    r.rect(x, y, w, h, CREAM)
    r.frame(x, y, w, h, INK)
    r.vline(x + 1, y + 1, h - 2, STONE)
    for yy in range(y + 3, y + h - 4, 8):
        r.hline(x + 2, yy + 5, w - 3, STONE_D)
        for xx in range(x + 3, x + w - 2, 4):
            r.stamp(xx, yy, ['.#.', '.#.', '#s#', '.#.', '###'], {'#': INK, 's': STONE})


def glass_island(r, x, y, w, h):
    r.rect(x, y, w, h, SKY)
    r.frame(x, y, w, h, INK)
    r.rect(x, y + h - 4, w, 4, SLATE)
    r.frame(x, y + h - 4, w, 4, INK)
    for xx in range(x + 3, x + w - 3, 5):
        r.stamp(xx, y + 2, ['.#.', '.#.', '#c#', '.#.', '.#.', '###'], {'#': INK, 'c': CREAM})
    for i in range(3):
        r.px(x + 2 + i, y + h - 6 - i, CREAM)


def aerial_px(x, y, shore=174):
    """The city 346 m below, as one continuous original map around the
    pod: downtown blocks to the north, the rail corridor, the waterfront,
    the lake and the islands to the south."""
    if y >= shore:
        for x0, x1, y0, y1 in ((14, 74, 181, 191), (92, 156, 179, 189), (176, 246, 182, 192)):
            cx, cy, rx, ry = (x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0) / 2, (y1 - y0) / 2
            d = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
            if d <= 1:
                if d > 0.72:
                    return CREAM
                if x0 == 14 and y == 186 and 22 <= x <= 66:
                    return CREAM
                if x0 == 92 and 0.12 < d < 0.3 and x > 120:
                    return TEAL
                return GRASS if (x + y) % 9 else SLATE
        if (x * 7 + y * 13) % 41 == 0 or ((x * 3 + y * 5) % 53 == 0 and y > shore + 3):
            return CREAM
        if y == shore:
            return SLATE
        return TEAL
    if y >= shore - 4:
        return GRASS if (x % 11) else CREAM
    if y >= shore - 8:
        return CREAM if y != shore - 6 or x % 6 else SLATE
    if shore - 20 <= y < shore - 8:
        k = y - (shore - 20)
        if k in (0, 11):
            return SLATE
        return SLATE if k in (2, 5, 8) or (k in (3, 6, 9) and x % 4 == 0) else 'BFC1BA'
    # Tiny blocks: 7 px with 1 px streets, avenues every 32 px.
    if x % 32 in (0, 1) or (y + 4) % 32 in (0, 1):
        return CREAM
    bx, by = x // 8, (y + 4) // 8
    ix, iy = x % 8, (y + 4) % 8
    if ix == 0 or iy == 0:
        return CREAM
    kind = (bx * 7 + by * 3 + (bx * by) % 5) % 7
    if kind == 0:
        return SLATE if (ix, iy) in ((3, 3), (5, 5)) else GRASS
    shadow = ix == 7 or iy == 7
    if kind in (1, 2):
        return SLATE if shadow else 'BFC1BA'
    if kind == 3:
        return SLATE if shadow or ix == 4 else ('BFC1BA' if ix < 4 else CREAM)
    if kind == 4:
        return SLATE if shadow or (ix >= 6 or iy >= 6) else ('BFC1BA' if 2 <= ix <= 4 and 2 <= iy <= 4 else CREAM)
    if kind == 5:
        return SLATE if shadow else ('BFC1BA' if iy < 4 else CREAM)
    return SLATE if shadow else ('BFC1BA' if (ix + iy) % 4 else CREAM)


def lookout_floor(x, y):
    """Inside the octagonal pod (pixel coordinates)."""
    x0, y0, x1, y1, cut = 24, 24, 231, 167, 24
    if not (x0 <= x <= x1 and y0 <= y <= y1):
        return False
    return ((x - x0) + (y - y0) >= cut and (x1 - x) + (y - y0) >= cut and
            (x - x0) + (y1 - y) >= cut and (x1 - x) + (y1 - y) >= cut)


def cn_lookout():
    """The LookOut level 346 m up: floor-to-ceiling windows on all sides
    over a tiny city, a glass floor, the elevator core, the SKY DESK host
    stand, benches and coin binoculars."""
    r = K.Room('cn_lookout', 32, 24, ['dusk', 'aerial', 'lake', 'stone', 'lift', 'wood', 'brass'],
               'CN TOWER LOOKOUT', 'landmark')
    carpet = ['bbbbbbbb', 'bbbbbbbb', 'bbdbbbbb', 'bbbbbbbb', 'bbbbbbbb', 'bbbbbbbb', 'bbbbbbdb', 'bbbbbbbb']

    def tile_class(tx, ty):
        n = sum(lookout_floor(tx * 8 + dx, ty * 8 + dy) for dx in range(8) for dy in range(8))
        return 'in' if n == 64 else ('out' if n == 0 else 'edge')
    classes = {(tx, ty): tile_class(tx, ty) for ty in range(r.th) for tx in range(r.tw)}
    for y in range(r.h):
        for x in range(r.w):
            cls = classes[(x // 8, y // 8)]
            if lookout_floor(x, y):
                ch = carpet[y % 8][x % 8]
                r.px(x, y, DUSK_D if ch == 'd' else DUSK, soft=DUSK if ch == 'd' else None)
            elif cls == 'edge':
                near = any(lookout_floor(x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1))
                near2 = any(lookout_floor(x + dx, y + dy) for dx in (-2, 0, 2) for dy in (-2, 0, 2))
                r.px(x, y, INK if near or not near2 else CREAM)
            else:
                r.px(x, y, aerial_px(x, y))
    # Window frames: a sill along the glass, mullions across the view.
    for y in range(r.h):
        for x in range(r.w):
            if classes[(x // 8, y // 8)] != 'out':
                continue
            near = [classes.get(((x + dx) // 8, (y + dy) // 8)) != 'out' and 0 <= x + dx < r.w and 0 <= y + dy < r.h
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))]
            near2 = [classes.get(((x + dx) // 8, (y + dy) // 8)) != 'out' and 0 <= x + dx < r.w and 0 <= y + dy < r.h
                     for dx, dy in ((2, 0), (-2, 0), (0, 2), (0, -2))]
            if any(near):
                r.px(x, y, SLATE)
            elif any(near2):
                r.px(x, y, CREAM)
            elif ((y < 24 or y > 167) and x % 32 == 16) or (24 <= y <= 167 and y % 32 == 8):
                r.px(x, y, SLATE)
    # Glints on the glass.
    for gx, gy in ((40, 4), (136, 2), (200, 5), (4, 60), (250, 100), (60, 178), (180, 174)):
        for i in range(4):
            r.px(gx + i, gy + 3 - i, CREAM)
    for (tx, ty), cls in classes.items():
        if cls != 'in':
            r.block(tx, ty)
    # Stairs down (the exit) through the south glass.
    r.rect(104, 168, 48, 24, INK)
    r.rect(106, 168, 44, 2, STONE)
    r.rect(120, 168, 16, 24, STONE)
    for y in range(170, 192, 3):
        r.hline(120, y, 16, STONE_D)
    r.vline(119, 168, 24, INK)
    r.vline(136, 168, 24, INK)
    r.text(106, 174, 'EXIT', CREAM)
    r.text(139, 174, 'EXIT', CREAM)
    r.rect(120, 160, 16, 8, DUSK_D)
    r.frame(120, 160, 16, 8, INK)
    for x in range(123, 134, 2):
        r.vline(x, 162, 4, INK)
    r.exit = [120, 160, 135, 167]
    r.entrance = [128, 159]
    # The elevator core: concrete cap, two glass elevators in its face.
    r.rect(96, 48, 64, 40, INK)
    r.rect(98, 50, 60, 2, STONE)
    r.rect(98, 50, 2, 14, STONE)
    r.rect(156, 50, 2, 14, STONE)
    r.rect(96, 64, 64, 24, STONE)
    r.hline(96, 64, 64, INK)
    r.vline(96, 64, 24, INK)
    r.vline(159, 64, 24, INK)
    r.hline(96, 87, 64, INK)
    for x in (100, 108, 148, 156):
        r.vline(x, 66, 20, STONE_D)
    elevator(r, 104, 64)
    elevator(r, 128, 64)
    r.text(104 + 44 - 12, 54, '346', STONE)
    r.block(12, 6, 8, 5)
    # SKY DESK host stand.
    r.rect(176, 56, 40, 16, GOLD)
    r.frame(176, 56, 40, 9, INK)
    r.hline(177, 57, 38, CREAM)
    r.rect(176, 64, 40, 8, GOLD_D)
    r.frame(176, 64, 40, 8, INK)
    r.text_center(196, 66, 'SKY DESK', CREAM)
    r.stamp(181, 58, ['#####', '#ccc#', '#####'], {'#': INK, 'c': CREAM})
    r.stamp(204, 58, ['.#.', '#c#', '###'], {'#': INK, 'c': CREAM})
    r.block(22, 7, 5, 2)
    # Glass floor: the ground straight below.
    glass_floor(r, 40, 104, 48, 48)
    # Benches by the windows, coin binoculars.
    for x, y, w, h in ((32, 64, 8, 24), (216, 96, 8, 24)):
        r.rect(x, y, w, h, TERRA)
        r.frame(x, y, w, h, INK)
        for yy in range(y + 2, y + h - 1, 2):
            r.hline(x + 1, yy, w - 2, WOOD_D)
        r.block(x // 8, y // 8, w // 8, h // 8)
    r.rect(160, 152, 32, 8, TERRA)
    r.frame(160, 152, 32, 8, INK)
    r.hline(161, 154, 30, WOOD_D)
    r.hline(161, 156, 30, WOOD_D)
    r.block(20, 19, 4, 1)
    for tx, ty in ((9, 3), (24, 3), (3, 12), (28, 9), (8, 20)):
        binocular(r, tx * 8, ty * 8)
        r.block(tx, ty)
    # Points.
    r.point('elevator', 14, 11, ['DOWN TO THE BASE', '58 SECONDS', 'MIND THE EARS'])
    r.point('elevator', 17, 11, ['DOWN TO THE BASE', '58 SECONDS', 'MIND THE EARS'])
    r.point('desk', 24, 9, ['SKY DESK', 'DELIVERIES AND', 'RESERVATIONS'], dx=4)
    r.point('window', 9, 4, ['DOWNTOWN', 'BLOCKS LIKE', 'CIRCUIT BOARDS'])
    r.point('window', 24, 4, ['NORTH TO BLOOR', 'ON A CLEAR DAY', 'YOU SEE TOMORROW'])
    r.point('window', 4, 12, ['WEST: THE RAIL', 'CORRIDOR AND', 'THE GARDINER'])
    r.point('window', 27, 9, ['EAST: THE DON', 'AND THE PORT', 'LANDS'])
    r.point('window', 8, 19, ['LAKE ONTARIO', 'AND THE ISLANDS', 'FERRIES BELOW'])
    r.point('exhibit', 7, 15, ['GLASS FLOOR', 'HOLDS 14 HIPPOS', 'PLEASE BRING 1'], dx=4)
    r.npc('attendant', 'counter', 24, 6, 's', area=(22, 6, 5, 1))
    r.npc('tourist', 'window', 4, 14, 'w', area=(4, 11, 1, 6))
    r.npc('tourist', 'window', 12, 19, 's', area=(10, 19, 4, 1))
    r.npc('tourist', 'wander', 7, 15, 's', area=(5, 13, 6, 6))
    r.npc('tourist', 'gaze', 11, 16, 'w')
    r.npc('visitor', 'gaze', 12, 14, 'w')
    r.npc('attendant', 'patrol', 8, 4, 's', path=[(6, 4), (25, 4), (25, 12), (6, 12)])
    r.npc('tourist', 'window', 27, 16, 'e', area=(27, 15, 1, 3))
    return r


def glass_floor(r, x, y, w, h):
    """A walkable glass panel: the ground straight below at the same tiny
    scale as the windows (the tower's legs, a domed stadium, the rail
    corridor, blocks), with panel joints."""
    for yy in range(y, y + h):
        for xx in range(x, x + w):
            i, j = xx - x, yy - y
            c = aerial_px(xx + 96, yy - 64)
            if 30 <= j <= 37:
                k = j - 30
                c = SLATE if k in (0, 7) or (k in (2, 5)) else ('BFC1BA' if (i + k) % 3 else SLATE)
            d = (i - 34) ** 2 + (j - 14) ** 2
            if d <= 64:
                c = SLATE if d > 49 else (CREAM if d <= 36 or (i + j) % 2 else 'BFC1BA')
                if d <= 36 and (i - 34) in (-3, 0, 3) and j < 18:
                    c = 'BFC1BA'
            if i % 16 == 15 or j % 16 == 15:
                c = SLATE
            r.px(xx, yy, c)
    r.stamp(x + 8, y + 10, ['..#.#..', '..###..', '###.###', '..#.#..'], {'#': SLATE})
    r.frame(x, y, w, h, SLATE)
    r.frame(x + 1, y + 1, w - 2, h - 2, CREAM)


def binocular(r, x, y):
    r.stamp(x, y, ['.######.', '#cDDDDc#', '#DDDDDD#', '.######.', '...##...', '..#DD#..', '.##DD##.', '.######.'],
            {'#': INK, 'c': CREAM, 'D': DUSK_D})


def dais(r, x, y, w, h):
    """A low pale display platform covering whole tiles."""
    r.rect(x, y, w, h, CREAM)
    r.frame(x, y, w, h, INK)
    r.frame(x + 1, y + 1, w - 2, h - 2, STONE)
    r.hline(x + 1, y + h - 2, w - 2, STONE_D)


# ------------------------------------------------------------- paintings
def painting(r, x, y, w, h, art, edge=None):
    """A framed original painting (art(r, x, y, w, h) fills the canvas);
    edge is the inner frame colour. A small label hangs below."""
    r.rect(x, y, w, h, INK)
    if edge:
        r.frame(x + 1, y + 1, w - 2, h - 2, edge)
    art(r, x + 2, y + 2, w - 4, h - 4)
    r.rect(x + w // 2 - 3, y + h + 1, 6, 2, INK)


def art_sunset_lake(r, x, y, w, h):
    """Pines on a northern lake at sunset (original)."""
    sun_x = x + w * 2 // 3
    for j in range(h):
        for i in range(w):
            if j < 2:
                c = PLUM
            elif j == 2:
                c = SUN if i % 2 else PLUM
            elif j < 7:
                c = SUN
            elif j == 7:
                c = INK if (i * 7) % 13 < 5 else PLUM
            else:
                c = PLUM
                if abs(x + i - sun_x) <= 2 - (j - 8) // 3 and j % 2 == 0:
                    c = SUN
                if (i * 5 + j * 3) % 23 == 0:
                    c = SUN
            r.px(x + i, y + j, c)
    r.stamp(sun_x - 2, y + 4, ['.###.', '#####', '#####'], {'#': CREAM})
    tree = ['..#..', '..#..', '.###.', '..#..', '.###.', '#####', '.###.', '#####', '..#..', '..#..']
    for tx_, ty_ in ((x, y), (x + 4, y + 2), (x + w - 5, y + 1), (x + w - 9, y + 3)):
        r.stamp(tx_, ty_, tree[:y + 10 - ty_], {'#': INK})


def art_portrait(r, x, y, w, h):
    r.rect(x, y, w, h, ROSE)
    for j in range(h):
        for i in range(w):
            if ((i - w / 2) ** 2) / (w * w / 4) + ((j - h / 2) ** 2) / (h * h / 4) > 1.0 and (i + j) % 2:
                r.px(x + i, y + j, ROSE_D)
    head = ['....####....',
            '..########..',
            '.#########..',
            '##########..',
            '###########.',
            '#########...',
            '##########..',
            '.#########..',
            '..#######...',
            '...######...',
            '....####....',
            '..########..',
            '############']
    r.stamp(x + w // 2 - 6, y + h - len(head), head, {'#': INK})


def art_colour_field(r, x, y, w, h):
    r.rect(x, y, w, h, SUN)
    for j in range(h):
        for i in range(w):
            top = 1 <= j <= h // 2 - 1 and 2 <= i <= w - 3
            bot = h // 2 + 1 <= j <= h - 2 and 2 <= i <= w - 3
            edge = i in (2, w - 3) or j in (1, h // 2 - 1, h // 2 + 1, h - 2)
            if top and not (edge and (i + j) % 2):
                r.px(x + i, y + j, PLUM)
            elif bot and not (edge and (i + j) % 2):
                r.px(x + i, y + j, CREAM)


def art_city_night(r, x, y, w, h):
    r.rect(x, y, w, h, NIGHT)
    for i, j in ((3, 1), (11, 3), (19, 0), (27, 2), (36, 1), (41, 4), (7, 5)):
        if i < w and j < h:
            r.px(x + i, y + j, CREAM)
    r.stamp(x + w - 9, y + 1, ['.##', '#..', '#..', '.##'], {'#': CREAM})
    heights = [6, 9, 7, 11, 8, 5, 10, 12, 7, 9, 6, 8]
    bx = x
    k = 0
    while bx < x + w:
        bw = min(3 + k % 3, x + w - bx)
        bh = heights[k % len(heights)]
        top = y + h - 3 - bh
        r.rect(bx, top, bw, bh, NIGHT_D if k % 2 else INK)
        for wy in range(top + 1, y + h - 3, 2):
            for wx in range(bx + 1, bx + bw - 1, 2):
                if (wx * 3 + wy * 5 + k) % 4 == 0:
                    r.px(wx, wy, CREAM)
        bx += bw
        k += 1
    tx = x + w // 3
    r.vline(tx, y + 1, h - 4, INK)
    r.stamp(tx - 1, y + 5, ['###', '#c#', '###'], {'#': INK, 'c': CREAM})
    r.rect(x, y + h - 3, w, 3, NIGHT_D)
    for i in range(0, w, 3):
        r.px(x + i + (i // 3) % 2, y + h - 2, CREAM if (i // 3) % 3 == 0 else NIGHT)


def art_bay_ice(r, x, y, w, h):
    for j in range(h):
        for i in range(w):
            if j < 4:
                c = CREAM if (i + j) % 4 else TEAL
            elif j == 4:
                c = SLATE
            else:
                c = TEAL
            r.px(x + i, y + j, c)
    for fx, fy, fw in ((2, 6, 7), (12, 7, 9), (4, 10, 6), (16, 11, 8), (22, 6, 4)):
        if fx + fw <= w:
            r.rect(x + fx, y + fy, fw, 2, CREAM)
            r.hline(x + fx + 1, y + fy + 2, fw - 1, SLATE)
            r.px(x + fx + fw // 2, y + fy, INK)


def art_streetcar(r, x, y, w, h):
    r.rect(x, y, w, h, CREAM)
    r.hline(x, y + 1, w, STONE_D)
    r.vline(x + w - 4, y + 1, h - 2, STONE_D)
    car = ['....#............#...',
           '.....#..........#....',
           '.##################..',
           '#cccccccccccccccccc#.',
           '#c##c##c##c##c##c#c#.',
           '#c##c##c##c##c##c#c#.',
           '#cccccccccccccccccc#.',
           '#ssssssssssssssssss#.',
           '.##################..',
           '..##..........##.....']
    r.stamp(x + 2, y + 2, car, {'#': INK, 'c': CREAM, 's': STONE})
    r.hline(x, y + h - 1, w, STONE)


def sculpture_loop(r, cx, base_y):
    rows = ['....#####.....',
            '..##ooooo##...',
            '.#oo#####oo#..',
            '#oo#.....#oO#.',
            '#o#.......#O#.',
            '#o#..###..#O#.',
            '#oo##oOo#.#O#.',
            '.#ooooO#.#oO#.',
            '..######.####.']
    r.stamp(cx - 7, base_y - len(rows) + 1, rows, {'#': INK, 'o': STONE, 'O': STONE_D})


def sculpture_tall(r, cx, base_y):
    rows = ['..###..', '.#oO#..', '.#oO#..', '..#O#..', '.#oOO#.', '#ooOO#.', '#oOOO#.', '.#oO#..',
            '.#oO#..', '#ooOO#.', '#oOOO#.', '.####..']
    r.stamp(cx - 3, base_y - len(rows) + 1, rows, {'#': INK, 'o': STONE, 'O': STONE_D})


def ago():
    """Art gallery (an original take on the AGO): two rooms joined by an
    opening, original paintings, bronze sculptures on plinths, benches, a
    guard post, wooden ribs over a south galleria walk."""
    r = K.Room('ago', 40, 24, ['plaster', 'stone', 'wood', 'sunset', 'night', 'rose', 'teal'], 'ART GALLERY', 'landmark')
    wall = Wall(CREAM, CREAM, TERRA)
    shell(r, wall, 3, planks(PLASTER_D, PLASTER), mat=(WOOD_D, INK), door=(CREAM, STONE_D))
    planks(PLASTER, CREAM)(r, 8, 152, r.w - 16, 32)
    r.hline(8, 152, r.w - 16, PLASTER_D)
    door_mat(r, (WOOD_D, INK))
    r.rect(8, 29, r.w - 16, 2, CREAM)
    # Partition with an opening between the rooms.
    r.rect(152, 32, 16, 48, INK)
    r.rect(154, 32, 2, 40, TERRA)
    r.rect(164, 32, 2, 40, TERRA)
    r.rect(152, 72, 16, 8, CREAM)
    r.hline(152, 72, 16, INK)
    r.hline(152, 79, 16, INK)
    r.rect(152, 112, 16, 40, INK)
    r.rect(154, 114, 2, 38, TERRA)
    r.rect(164, 114, 2, 38, TERRA)
    r.hline(154, 114, 12, TERRA)
    r.block(19, 4, 2, 6)
    r.block(19, 14, 2, 5)
    # Paintings on the north wall faces.
    works = [(16, 48, art_sunset_lake, SUN, ['LAKE AT DUSK', 'OIL ON CANVAS', 'LOONS NOT SHOWN']),
             (72, 24, art_portrait, ROSE_D, ['PORTRAIT IN ROSE', 'THE SITTER LEFT', 'EARLY FOR LUNCH']),
             (104, 40, art_colour_field, PLUM, ['FIELD NO. 3', 'TWO RECTANGLES', 'MANY FEELINGS']),
             (176, 48, art_city_night, NIGHT_D, ['CITY AT NIGHT', 'SOMEONE LEFT', 'THE LIGHTS ON']),
             (232, 32, art_bay_ice, SLATE, ['BAY ICE, MARCH', 'COLD BUT HOPEFUL', 'LIKE A COMMUTE']),
             (272, 32, art_streetcar, STONE_D, ['THE 501, SKETCH', 'CHARCOAL', 'STILL DUE IN 5'])]
    for x, w, art, edge, text in works:
        painting(r, x, 10, w, 17, art, edge)
        cx = x + w // 2
        r.point('painting', cx // 8, 4, text, dx=cx % 8 - 4)
    # Sculptures on plinths (bronze rises over the floor behind them).
    for px_, sc, text in ((64, sculpture_loop, ['LOOP', 'BRONZE', 'NO BEGINNING']),
                          (232, sculpture_tall, ['TALL THOUGHT', 'BRONZE', 'IT IS THINKING'])):
        dais(r, px_ - 8, 80, 32, 32)
        plinth(r, px_, 96, 16, 16)
        sc(r, px_ + 8, 100)
        r.block(px_ // 8 - 1, 10, 4, 4)
        r.point('exhibit', px_ // 8, 14, text, dx=4)
    # Benches facing the paintings.
    for x in (32, 200, 280):
        r.rect(x, 64, 32, 8, TERRA)
        r.frame(x, 64, 32, 8, INK)
        r.hline(x + 1, 66, 30, WOOD_D)
        r.hline(x + 1, 68, 30, WOOD_D)
        r.hline(x, 71, 32, INK)
        pass  # seats stay open
    # Guard post by the opening.
    r.rect(128, 112, 16, 16, CREAM)
    r.frame(128, 112, 16, 9, INK)
    r.rect(128, 120, 16, 8, STONE)
    r.frame(128, 120, 16, 8, INK)
    r.stamp(131, 114, ['####', '#cc#', '####'], {'#': INK, 'c': STONE})
    r.text(134, 122, 'G', INK)
    r.block(16, 14, 2, 2)
    r.point('desk', 16, 16, ['GUARD POST', 'PLEASE DO NOT', 'LICK THE ART'], dx=4)
    # The galleria walk: wooden ribs overhead along the south wall.
    for tx in list(range(1, 17)) + list(range(23, 39)):
        clear_over(r, tx, 21, 1, 2, CREAM)
    with r.overhead():
        for x in list(range(8, 136, 8)) + list(range(184, 312, 8)):
            for y in range(168, 184):
                k = y - 168
                bow = (3 if 4 <= k <= 11 else (2 if 2 <= k <= 13 else 1)) if (x // 8) % 2 else 0
                r.px(x + 2 + bow, y, WOOD_D)
                r.px(x + 3 + bow, y, TERRA)
            r.hline(x, 168, 8, WOOD_D)
    r.point('plaque', 6, 19, ['THE GALLERIA', 'WOODEN RIBS LIKE', 'A SHIP INSIDE OUT'])
    r.npc('viewer', 'gaze', 5, 6, 'n')
    r.npc('viewer', 'gaze', 23, 6, 'n')
    r.npc('guard', 'patrol', 18, 11, 's', path=[(14, 11), (25, 11), (25, 17), (14, 17)])
    r.npc('docent', 'wander', 30, 15, 's', area=(22, 14, 15, 4))
    r.npc('viewer', 'gaze', 31, 15, 'n')
    r.npc('visitor', 'sit', 26, 8, 'n')
    r.npc('visitor', 'wander', 8, 16, 'n', area=(2, 15, 14, 4))
    r.npc('viewer', 'gaze', 13, 6, 'n')
    return r


def bezier(p0, p1, p2, n):
    out = []
    for i in range(n + 1):
        t = i / n
        x = (1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0]
        y = (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1]
        out.append((x, y))
    return out


def disc(r, cx, cy, rad, c):
    rr = rad * rad + rad * 0.8
    for y in range(int(cy - rad - 1), int(cy + rad + 2)):
        for x in range(int(cx - rad - 1), int(cx + rad + 2)):
            if (x - cx) ** 2 + (y - cy) ** 2 <= rr:
                r.px(x, y, c)


def bones(r, segments):
    """Bones as chains of discs: segments are (points, radius at start,
    radius at end); drawn with an ink outline, bone face and shade."""
    chain = []
    for pts, r0, r1 in segments:
        for i, (x, y) in enumerate(pts):
            t = i / max(1, len(pts) - 1)
            chain.append((x, y, r0 + (r1 - r0) * t))
    for x, y, rad in chain:
        disc(r, x, y, rad + 1, INK)
    for x, y, rad in chain:
        disc(r, x, y, rad, CREAM)
    for x, y, rad in chain:
        if rad >= 1:
            r.px(int(round(x)), int(round(y + rad)), PLASTER)


def sauropod(r, ox, oy):
    """An original long-necked skeleton facing west; (ox, oy) is the
    front of the skull, the feet stand about 50 px lower."""
    neck = bezier((ox + 6, oy + 3), (ox + 14, oy + 22), (ox + 44, oy + 28), 26)
    back = bezier((ox + 44, oy + 28), (ox + 66, oy + 22), (ox + 86, oy + 26), 22)
    tail = bezier((ox + 86, oy + 26), (ox + 114, oy + 32), (ox + 130, oy + 50), 30)
    legs = []
    for hx, fx, d in ((ox + 50, ox + 47, 0), (ox + 56, ox + 55, 1), (ox + 80, ox + 79, 0), (ox + 86, ox + 88, 1)):
        hy = oy + 30
        legs.append((bezier((hx, hy), (hx + 2, hy + 10), (fx, oy + 49), 10), 2.2 - d * 0.4, 1.2))
    for leg in legs[1::2]:
        bones(r, [leg])
    # Ribs hang under the back.
    for i, x in enumerate(range(ox + 48, ox + 84, 4)):
        depth = 12 - abs(i - 4) * 1.4
        pts = bezier((x, oy + 28), (x - 3, oy + 28 + depth * 0.6), (x + 1, oy + 28 + depth), 8)
        for px_, py_ in pts:
            r.px(int(round(px_)), int(round(py_)), INK)
            r.px(int(round(px_)) + 1, int(round(py_)), CREAM)
    bones(r, [(neck, 1.2, 2.0), (back, 2.6, 2.4), (tail, 2.2, 0.4)])
    for leg in legs[0::2]:
        bones(r, [leg])
    # Hip and shoulder blades.
    bones(r, [(bezier((ox + 78, oy + 22), (ox + 84, oy + 20), (ox + 90, oy + 30), 8), 2.5, 1.5)])
    bones(r, [(bezier((ox + 46, oy + 24), (ox + 52, oy + 26), (ox + 54, oy + 36), 8), 2.0, 1.0)])
    # Spines along the back and tail.
    for x, y in back[::3] + tail[:18:3]:
        r.px(int(round(x)), int(round(y)) - 4, INK)
        r.px(int(round(x)), int(round(y)) - 3, CREAM)
    # Feet.
    for hx, fx, d in ((0, ox + 47, 0), (0, ox + 55, 1), (0, ox + 79, 0), (0, ox + 88, 1)):
        r.stamp(fx - 3, oy + 49, ['#######', '#ccccc#' if not d else '#ppppp#', '#######'], {'#': INK, 'c': CREAM, 'p': PLASTER})
    # Skull.
    r.stamp(ox - 2, oy - 2, ['..######..', '.#cccccc#.', '#cc#ccccc#', '#cccccccp#', '.#cpcpcp#.', '..######..'],
            {'#': INK, 'c': CREAM, 'p': PLASTER})


def shards(r, x0, y0, x1, y1, polys):
    """Angular light on the floor from a crystal skylight: polygons with
    45-degree and square edges, dithered light inside a bright rim."""
    def inside(poly, x, y):
        n, c = len(poly), False
        for i in range(n):
            (ax, ay), (bx, by) = poly[i], poly[(i + 1) % n]
            if (ay > y) != (by > y) and x < ax + (y - ay) * (bx - ax) / (by - ay):
                c = not c
        return c
    for poly in polys:
        for y in range(y0, y1):
            for x in range(x0, x1):
                if inside(poly, x + 0.5, y + 0.5):
                    edge = not all(inside(poly, x + 0.5 + dx, y + 0.5 + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
                    if edge:
                        r.px(x, y, CREAM)
                    elif (x + y) % 2 == 0:
                        r.px(x, y, CREAM, soft=DUSK)
                    else:
                        r.px(x, y, DUSK)


def vitrine(r, x, y, w, h, kind):
    """A free-standing case (whole tiles): glass with fossils, or dark
    velvet with crystals (violet amethyst, jade)."""
    if kind == 'fossil':
        top, face, hi, item = SKY, SLATE, CREAM, CREAM
    elif kind == 'amethyst':
        top, face, hi, item = VIO_D, VIO_D, CREAM, VIO
    else:
        top, face, hi, item = JADE_D, JADE_D, CREAM, JADE
    r.rect(x, y, w, h, top)
    r.frame(x, y, w, h - 5, INK)
    r.rect(x, y + h - 6, w, 6, face)
    r.frame(x, y + h - 6, w, 6, INK)
    r.hline(x + 1, y + h - 5, w - 2, hi if kind == 'fossil' else item)
    for i in range(3):
        r.px(x + 2 + i, y + h - 9 - i, hi)
    if kind == 'fossil':
        for k, xx in enumerate(range(x + 4, x + w - 6, 9)):
            if k % 2 == 0:
                r.stamp(xx, y + 2, ['.###.', '#c#c#', '#c##c', '#ccc#', '.###.'], {'#': SLATE, 'c': CREAM})
            else:
                r.stamp(xx, y + 2, ['.#.', '#c#', '###', '#c#', '###', '#c#', '.#.'], {'#': SLATE, 'c': CREAM})
    else:
        for k, xx in enumerate(range(x + 3, x + w - 5, 7)):
            r.stamp(xx, y + 2, ['..#..', '.#i#.', '#iiI#', '#iII#', '.###.'][:5], {'#': INK, 'i': item, 'I': CREAM if k % 2 else item})


def rom():
    """Dinosaur hall (an original take on the ROM): a long-necked
    skeleton on a gravel dais, fossil and mineral cases, crystal light on
    the floor, benches and a meteorite on red velvet."""
    r = K.Room('rom', 40, 28, ['dusk', 'plaster', 'glass', 'violet', 'jade', 'wood', 'red'], 'ROYAL ONT MUSEUM', 'landmark')
    wall = Wall(DUSK_D, INK, DUSK)
    shell(r, wall, 3, slabs(DUSK, DUSK_D, size=16), mat=(DUSK_D, INK), door=(CREAM, DUSK))
    shards(r, 8, 32, 312, 216, [
        [(24, 40), (72, 40), (104, 72), (56, 72)],
        [(232, 32), (288, 32), (312, 56), (312, 80), (264, 80)],
        [(16, 152), (48, 120), (80, 120), (48, 152)],
        [(248, 120), (296, 120), (312, 136), (312, 152), (280, 152)],
        [(120, 168), (176, 168), (200, 192), (144, 192)],
    ])
    door_mat(r, (DUSK_D, INK))
    # Wall: title, banners, wall cases of fossils.
    r.text_center(160, 13, 'DINOSAURS', CREAM, font='big')
    r.hline(124, 22, 72, DUSK)
    for bx in (104, 200):
        r.rect(bx, 8, 16, 24, RED)
        r.frame(bx, 8, 16, 24, INK)
        r.rect(bx + 1, 9, 14, 2, RED_D)
        r.stamp(bx + 2, 14, ['.......##...', '......####..', '.....##.....', '....##......', '#######.....', '.######.....',
                             '.#.#..#.#...'], {'#': CREAM})
        r.hline(bx, 31, 16, INK)
    for wx in (24, 248):
        r.rect(wx, 8, 48, 24, SLATE)
        r.frame(wx, 8, 48, 24, INK)
        r.rect(wx + 2, 10, 44, 18, SKY)
        r.hline(wx + 2, 19, 44, SLATE)
        for k, xx in enumerate(range(wx + 5, wx + 44, 8)):
            r.stamp(xx, 12 if k % 2 else 21, ['.###.', '#c#c#', '#cc#c', '.###.'] if k % 2 else ['#c#', 'ccc', '#c#'],
                    {'#': SLATE, 'c': CREAM})
        for i in range(4):
            r.px(wx + 3 + i, 17 - i, CREAM)
        r.hline(wx, 31, 48, INK)
    r.point('plaque', 20, 4, ['DINOSAURS', 'THEY WERE HERE', 'BEFORE THE TTC'], dx=0)
    r.point('exhibit', 5, 4, ['AMMONITES', 'SPIRAL SHELLS', 'OF ANCIENT SEAS'], dx=4)
    r.point('exhibit', 33, 4, ['TRILOBITES', 'ANCIENT BUGS', 'VERY SMALL TEETH'], dx=4)
    # The dais and its skeleton.
    r.rect(88, 64, 144, 80, PLASTER_D)
    for y in range(66, 140):
        for x in range(90, 230):
            v = (x * 13 + y * 7 + (x * y) % 5) % 37
            if v == 0:
                r.px(x, y, INK)
            elif v in (11, 23):
                r.px(x, y, PLASTER)
    r.frame(88, 64, 144, 80, INK)
    r.frame(89, 65, 142, 78, PLASTER)
    r.rect(88, 136, 144, 8, PLASTER)
    r.hline(88, 136, 144, INK)
    r.hline(88, 143, 144, INK)
    r.vline(88, 136, 8, INK)
    r.vline(231, 136, 8, INK)
    for x in range(96, 228, 16):
        r.vline(x, 137, 6, PLASTER_D)
    sauropod(r, 98, 74)
    r.rect(152, 137, 16, 5, CREAM)
    r.frame(152, 137, 16, 5, INK)
    r.block(11, 8, 18, 10)
    r.point('exhibit', 19, 18, ['LONG NECK', 'PLANT EATER', 'NEVER LATE'], dx=4)
    # Cases of fossils and minerals.
    for x, y, kind, text in ((16, 64, 'fossil', ['FOSSIL BONES', 'PLEASE DO NOT', 'FEED THEM']),
                             (16, 120, 'fossil', ['FOSSIL EGGS', 'DID NOT HATCH', 'NO REFUNDS']),
                             (272, 88, 'amethyst', ['AMETHYST GEODE', 'A ROCK THAT', 'LOOKED INSIDE']),
                             (272, 136, 'jade', ['JADE', 'GREEN STONE', 'POLISHED BY TIME'])):
        vitrine(r, x, y, 32, 16, kind)
        r.block(x // 8, y // 8, 4, 2)
        r.point('exhibit', x // 8 + 1, y // 8 + 2, text, dx=4)
    # Benches facing the skeleton.
    for x in (96, 192):
        r.rect(x, 160, 32, 8, TERRA)
        r.frame(x, 160, 32, 8, INK)
        r.hline(x + 1, 162, 30, WOOD_D)
        r.hline(x + 1, 164, 30, WOOD_D)
    # A meteorite on a red velvet plinth.
    r.rect(264, 176, 32, 24, RED)
    r.frame(264, 176, 32, 16, INK)
    r.frame(265, 177, 30, 14, RED_D)
    r.stamp(270, 178, ['....####....', '..##RRRR##..', '.#RR#RRRR##.', '#RRRRRR#RR#.', '#R#RRRRRRRR#',
                       '.##RRR#RRR#.', '...######...'], {'#': INK, 'R': RED_D})
    r.px(276, 181, CREAM)
    r.rect(264, 192, 32, 8, RED_D)
    r.frame(264, 192, 32, 8, INK)
    r.rect(276, 194, 8, 3, CREAM)
    r.block(33, 22, 4, 3)
    r.point('exhibit', 34, 25, ['METEORITE', 'IRON FROM SPACE', 'ARRIVED EARLY'], dx=4)
    r.npc('visitor', 'wander', 30, 20, 'w', area=(28, 19, 10, 2))
    r.npc('viewer', 'gaze', 32, 25, 'n')
    r.npc('visitor', 'wander', 24, 24, 'n', area=(21, 23, 9, 3))
    r.npc('guard', 'patrol', 6, 20, 's', path=[(6, 20), (6, 6), (33, 6), (33, 20)])
    r.npc('viewer', 'gaze', 15, 19, 'n')
    r.npc('visitor', 'sit', 25, 20, 'n')
    r.npc('docent', 'wander', 9, 12, 'e', area=(7, 9, 3, 9))
    return r


def terrazzo(base=CREAM, a=STONE, b=STONE_D):
    rows = []
    for j in range(16):
        row = ''
        for i in range(16):
            v = (i * 7 + j * 11 + (i * j) % 7) % 29
            row += 'a' if v == 0 else ('b' if v == 13 else '.')
        rows.append(row)

    def paint(r, x, y, w, h):
        r.rect(x, y, w, h, base)
        r.fill(x, y, w, h, rows, {'a': a, 'b': b}, soft={'a': base, 'b': base})
    return paint


def figure(r, x, y, shirt, hair, back=False):
    """A tiny seated adult drawn into the background (6 x 7): seen from the
    front (a face under the hair) or from behind."""
    rows = (['.####.', '#hhhh#', '#hhhh#', '.#hh#.', '##ss##', '#ssss#', '#ssss#'] if back else
            ['.####.', '#hhhh#', '#hffh#', '.#ff#.', '##ss##', '#ssss#', '#ssss#'])
    r.stamp(x, y, rows, {'#': INK, 'h': hair, 'f': CREAM, 's': shirt})


def table_set(r, x, y, colour, colour_d, diners=''):
    """A 24 x 16 food-court table (whole tiles at x, y) with two seats north
    and two south; diners lists the seats taken ('n' north-west, 'N'
    north-east, 's' south-west, 'S' south-east)."""
    r.rect(x + 3, y + 6, 18, 6, CREAM)
    r.frame(x + 3, y + 6, 18, 6, INK)
    r.hline(x + 4, y + 10, 16, colour)
    for tx in (x + 7, x + 14):
        r.stamp(tx, y + 7, ['##', '#.'], {'#': colour_d, '.': CREAM})
    seats = (('n', x + 3, y, False), ('N', x + 15, y, False), ('s', x + 3, y + 11, True), ('S', x + 15, y + 11, True))
    for key, sx, sy, back in seats:
        if key in diners:
            figure(r, sx, sy - 1 if not back else sy - 2, colour, colour_d if colour != CREAM else INK, back)
        else:
            r.stamp(sx, sy + (0 if not back else 1), ['######', '#cccc#', '#cccc#', '#dddd#', '######'][:5],
                    {'#': INK, 'c': colour, 'd': colour_d})


def shopfront(r, x, w, sign, sign_d, icon, goods):
    """A mall shopfront on the north wall face: a sign band with an icon,
    a display window of goods and an open doorway."""
    r.rect(x, 8, w, 9, sign)
    r.hline(x, 8, w, INK)
    r.hline(x, 16, w, INK)
    r.hline(x, 9, w, sign_d)
    r.stamp(x + w // 2 - len(icon[0]) // 2, 10, icon, {'#': CREAM, 'd': sign_d})
    r.rect(x, 17, w, 15, INK)
    r.rect(x + 1, 18, w - 18, 12, SKY)
    r.hline(x + 1, 29, w - 18, SLATE)
    for i in range(3):
        r.px(x + 2 + i, 22 - i, CREAM)
    gx = x + 6
    for g in goods:
        r.stamp(gx, 30 - len(g), g, {'#': INK, 'c': CREAM, 's': SLATE})
        gx += len(g[0]) + 2
    r.rect(x + w - 16, 18, 15, 13, SLATE)
    r.rect(x + w - 15, 19, 13, 12, '172B38')
    r.hline(x + w - 15, 31, 13, SLATE)
    r.hline(x, 31, w, INK)


ICON_SHOE = ['..........', '.##.......', '.#d##.....', '.#ddd###..', '.########.', '..........']
ICON_SHIRT = ['.##..##.', '########', '.######.', '..####..', '..####..', '..####..']
ICON_BOOK = ['.###.###.', '#ddd#ddd#', '#ddd#ddd#', '#ddd#ddd#', '.###.###.', '....#....']
ICON_NOTE = ['...####', '...#..#', '...#..#', '.###.##', '####.##', '.##....']
ICON_GEM = ['.#####.', '#d#d#d#', '.#ddd#.', '..#d#..', '...#...', '.......']
GOODS_SHOE = [['......', '##....', '#c##..', '######']]
GOODS_SHIRT = [['#..#', '####', '.##.', '.##.'], ['#..#', '####', '.##.', '.##.']]
GOODS_BOOK = [['##', '#c', '#c', '##'], ['##', 'c#', 'c#', '##'], ['##', '#c', '#c', '##']]


def escalators(r, x, y, w):
    """Two escalators side by side running east-west, seen from above
    (16 px each, glass balustrades), covering whole tiles."""
    r.rect(x, y, w, 32, INK)
    for k, yy in enumerate((y, y + 16)):
        r.rect(x, yy + 1, w, 2, SKY)
        r.hline(x, yy + 3, w, INK)
        r.rect(x, yy + 12, w, 2, SKY)
        r.hline(x, yy + 11, w, INK)
        r.rect(x + 6, yy + 4, w - 12, 7, SLATE)
        for xx in range(x + 7, x + w - 6, 3):
            r.vline(xx, yy + 4, 7, INK if (xx // 3) % 2 else CREAM)
        r.rect(x, yy + 4, 6, 7, CREAM)
        r.rect(x + w - 6, yy + 4, 6, 7, CREAM)
        for xx in (x + 1, x + 3, x + w - 4, x + w - 2):
            r.vline(xx, yy + 5, 5, SLATE)
        ay = yy + 6
        if k == 0:
            r.stamp(x + w // 2 - 2, ay, ['#...', '##..', '###.', '##..', '#...'], {'#': CREAM})
        else:
            r.stamp(x + w // 2 - 2, ay, ['...#', '..##', '.###', '..##', '...#'], {'#': CREAM})


def eaton():
    """A downtown mall concourse (an original take on the Eaton Centre):
    shopfronts with icon signs, a glass-roof pattern down the galleria,
    escalators, planters, benches and a food court with diners."""
    r = K.Room('eaton', 48, 24, ['stone', 'glass', 'leaf', 'wood', 'red', 'jade', 'violet'], 'EATON CENTRE', 'landmark')
    wall = Wall(STONE, STONE_D, STONE_D)
    shell(r, wall, 3, terrazzo(), mat=(SLATE, INK), door=(CREAM, SLATE))
    south_glass(r, 19, 4)
    south_glass(r, 25, 4)
    # The glass roof's lattice of light down the middle of the galleria.
    for y in range(80, 120):
        for x in range(8, 376):
            k = y - 80
            if k in (0, 39):
                r.px(x, y, STONE_D, soft=CREAM)
            elif k in (1, 38):
                r.px(x, y, CREAM)
            elif (x % 16 == 0) or k % 13 == 6:
                r.px(x, y, STONE, soft=CREAM)
            elif ((x - y) % 16 == 0 or (x + y) % 16 == 0) and 3 < k < 36:
                r.px(x, y, STONE, soft=CREAM)
            else:
                r.px(x, y, CREAM)
    door_mat(r, (SLATE, INK))
    # Food court stalls on the west wall face.
    for x, name, sign, sign_d, icon in ((16, 'NOODLES', RED, RED_D, ['#########', '.#ddddd#.', '..#####..']),
                                        (56, 'PIZZA', JADE, JADE_D, ['#######', '.#ddd#.', '..#d#..', '...#...'])):
        r.rect(x, 8, 32, 24, sign)
        r.hline(x, 8, 32, INK)
        r.frame(x, 8, 32, 24, INK)
        r.text_center(x + 16, 11, name, CREAM)
        r.stamp(x + 16 - len(icon[0]) // 2, 19, icon, {'#': CREAM, 'd': sign_d})
        r.hline(x, 31, 32, INK)
        r.rect(x, 32, 32, 16, sign)
        r.frame(x, 32, 32, 9, INK)
        r.rect(x + 1, 33, 30, 7, CREAM)
        r.rect(x, 40, 32, 8, sign_d)
        r.frame(x, 40, 32, 8, INK)
        for xx in range(x + 3, x + 29, 7):
            r.stamp(xx, 34, ['.##.', '#cc#', '.##.'], {'#': sign_d, 'c': sign})
        r.block(x // 8, 4, 4, 2)
    r.point('counter', 3, 6, ['NOODLES', 'HAND PULLED', 'SINCE TUESDAY'], dx=4)
    r.point('counter', 8, 6, ['PIZZA', 'BY THE SLICE', 'OR BY THE TON'], dx=4)
    # Shopfronts.
    shops = [(104, RED, RED_D, ICON_SHOE, GOODS_SHOE, ['STEP RIGHT', 'SHOES FOR FEET', 'THAT WALK A LOT']),
             (152, VIO, VIO_D, ICON_SHIRT, GOODS_SHIRT, ['THREADS', 'SHIRTS ON SALE', 'SLEEVES EXTRA']),
             (200, JADE, JADE_D, ICON_BOOK, GOODS_BOOK, ['PAGE TURNER', 'BOOKS AND MAPS', 'OF EVERYWHERE']),
             (248, RED, RED_D, ICON_NOTE, GOODS_BOOK[:1], ['LOUD RECORDS', 'NEW AND USED', 'MOSTLY LOUD']),
             (296, VIO, VIO_D, ICON_GEM, [], ['SPARKLE CO.', 'RINGS AND THINGS', 'MOSTLY THINGS'])]
    for x, sign, sign_d, icon, goods, text in shops:
        shopfront(r, x, 40, sign, sign_d, icon, goods)
        r.point('window', (x + 12) // 8, 4, text, dx=0)
    for x in (96, 144, 192, 240, 288, 336):
        r.rect(x, 8, 8, 24, STONE)
        r.frame(x, 8, 8, 24, INK)
        r.vline(x + 2, 9, 22, CREAM)
    # Escalators to the upper level.
    escalators(r, 328, 72, 48)
    r.block(41, 9, 6, 4)
    r.point('elevator', 40, 10, ['ESCALATOR', 'LEVEL 2 SHOPS', 'ALWAYS MOVING'], dx=0)
    r.text(344, 18, 'LEVEL 2', CREAM)
    r.stamp(352, 24, ['..#..', '.###.', '#####'], {'#': CREAM})
    # Planters and benches along the galleria.
    for x in (120, 216, 280):
        planter(r, x, 88, 16, 16)
        r.block(x // 8, 11, 2, 2)
    for x in (144, 240):
        bench(r, x, 96, 24)
    # Food court tables with diners.
    sets = [(16, 56, RED, RED_D, 'nS'), (48, 56, JADE, JADE_D, 'N'), (80, 56, VIO, VIO_D, 'ns'),
            (16, 128, JADE, JADE_D, 's'), (48, 128, VIO, VIO_D, 'nN'), (80, 128, RED, RED_D, ''),
            (16, 160, VIO, VIO_D, 'S'), (48, 160, RED, RED_D, 'nsS'), (80, 160, JADE, JADE_D, 'N')]
    for x, y, colour, colour_d, diners in sets:
        table_set(r, x, y, colour, colour_d, diners)
        r.block(x // 8, y // 8, 3, 2)
    # Kiosk carts on the concourse.
    for x, colour, colour_d, text in ((144, RED, RED_D, ['PRETZEL CART', 'TWISTED SINCE', 'THE MORNING']),
                                      (288, JADE, JADE_D, ['CASE CART', 'FITS EVERY PHONE', 'EXCEPT YOURS'])):
        r.rect(x, 136, 32, 8, CREAM)
        for xx in range(x, x + 32, 8):
            r.rect(xx, 136, 4, 8, colour)
        r.hline(x, 136, 32, INK)
        r.hline(x, 143, 32, colour_d)
        r.vline(x, 136, 8, INK)
        r.vline(x + 31, 136, 8, INK)
        r.rect(x, 144, 32, 8, TERRA)
        r.frame(x, 144, 32, 8, INK)
        r.hline(x + 1, 148, 30, WOOD_D)
        for xx in (x + 4, x + 26):
            r.stamp(xx, 145, ['##', '##'], {'#': CREAM})
        r.block(x // 8, 17, 4, 2)
        r.point('counter', x // 8 + 1, 19, text, dx=4)
    for x in (224, 352):
        planter(r, x, 136, 16, 16)
        r.block(x // 8, 17, 2, 2)
    r.point('screen', 30, 21, ['MALL DIRECTORY', 'YOU ARE HERE.', 'SO IS EVERYONE.'], dx=4)
    r.rect(240, 152, 16, 8, INK)
    r.rect(241, 153, 14, 5, SKY)
    r.stamp(243, 154, ['#.#', '.#.', '#.#'], {'#': INK})
    r.rect(246, 160, 4, 4, SLATE)
    r.block(30, 19, 2, 2)
    r.npc('shopper', 'wander', 26, 7, 's', area=(13, 5, 26, 3))
    r.npc('teen', 'wander', 22, 16, 'n', area=(22, 15, 12, 2))
    r.npc('teen', 'gaze', 23, 16, 'n')
    r.npc('guard', 'patrol', 13, 14, 'e', path=[(13, 14), (38, 14), (38, 9), (13, 9)])
    r.npc('diner', 'sit', 31, 12, 'n')
    r.npc('shopper', 'window', 33, 5, 'n', area=(31, 4, 6, 2))
    r.npc('visitor', 'sit', 18, 12, 'n')
    r.npc('shopper', 'wander', 40, 17, 'w', area=(34, 14, 12, 7))
    return r


def arched_window(r, x, y, w, h):
    """A tall arched window on a wall face (glass palette, whole tiles)."""
    r.rect(x, y, w, h, CREAM)
    for j in range(h):
        for i in range(w):
            cx = (w - 1) / 2
            arch = j < w / 2 and (i - cx) ** 2 + (j - w / 2) ** 2 > (w / 2 - 1) ** 2
            if arch:
                continue
            edge = i in (1, w - 2) or j == h - 2 or (j < w / 2 and (i - cx) ** 2 + (j - w / 2) ** 2 > (w / 2 - 2.2) ** 2)
            if i == 0 or i == w - 1 or j == h - 1:
                continue
            c = INK if edge else (SLATE if i == w // 2 or (j - 2) % 6 == 0 else SKY)
            r.px(x + i, y + j, c)
    for k in range(3):
        r.px(x + 3 + k, y + h - 6 - k, CREAM)


def column(r, x, y):
    """A round stone column seen from above on a square base (16 x 16)."""
    r.stamp(x, y, ['################',
                   '#cccccccccccccs#',
                   '#cc...####...ss#',
                   '#c..##cccc##..s#',
                   '#c.#ccccccss#.s#',
                   '#c#cccccccsss#s#',
                   '#c#ccccccssss#s#',
                   '#c#cccccsssss#s#',
                   '#c#ccccssssss#s#',
                   '#c#cccsssssss#s#',
                   '#c.#ssssssss#.s#',
                   '#c..##ssss##..s#',
                   '#cs...####...ss#',
                   '#ssssssssssssss#',
                   '#dddddddddddddd#',
                   '################'], {'#': INK, 'c': CREAM, 's': PLASTER, 'd': PLASTER_D, '.': PLASTER_D})


def union():
    """Union Station's great hall (original take): tall arched windows,
    a departures board, ticket counters, benches, columns, stone floor."""
    r = K.Room('union', 40, 22, ['plaster', 'glass', 'brass', 'wood', 'red', 'leaf'], 'UNION STATION', 'landmark')
    wall = Wall(PLASTER, PLASTER_D, PLASTER_D)
    shell(r, wall, 3, slabs(CREAM, PLASTER, size=16), mat=(RED_D, INK), door=(CREAM, PLASTER_D))
    # A border of darker stone around the hall floor.
    for x0, y0, w, h in ((8, 32, 304, 4), (8, 164, 304, 4), (8, 32, 4, 136), (308, 32, 4, 136)):
        r.rect(x0, y0, w, h, PLASTER)
    for x0, y0, w, h in ((8, 36, 304, 1), (8, 163, 304, 1), (12, 36, 1, 128), (307, 36, 1, 128)):
        r.rect(x0, y0, w, h, PLASTER_D)
    door_mat(r, (RED_D, INK))
    for y in (14, 22):
        r.hline(8, y, r.w - 16, PLASTER_D)
    south_glass(r, 16, 3)
    south_glass(r, 21, 3)
    for tx in (2, 6, 10, 27, 31, 35):
        arched_window(r, tx * 8, 8, 24, 24)
    # Departures board with a clock.
    r.rect(128, 8, 64, 24, INK)
    r.frame(129, 9, 62, 22, GOLD_D)
    r.text(132, 11, 'DEPARTURES', CREAM)
    r.stamp(178, 10, ['.###.', '#c#c#', '#c##c', '#ccc#', '.###.'], {'#': GOLD, 'c': CREAM})
    r.text(132, 17, '0715 NIAGARA', GOLD)
    r.text(132, 23, '0742 KINGSTON', GOLD)
    r.hline(128, 31, 64, INK)
    r.point('screen', 19, 4, ['0715 NIAGARA', '0730 BARRIE', '0742 KINGSTON'], dx=4)
    # Ticket counters with brass grilles.
    r.rect(16, 40, 64, 16, TERRA)
    r.frame(16, 40, 64, 9, INK)
    r.hline(17, 41, 62, CREAM)
    r.rect(16, 48, 64, 8, WOOD_D)
    r.frame(16, 48, 64, 8, INK)
    for x in range(18, 78, 16):
        r.stamp(x + 1, 49, ['############', '#tttttttttt#', '#t#t#t#t#tt#', '############'], {'#': INK, 't': TERRA})
    r.text_center(48, 42, 'TICKETS', WOOD_D)
    r.block(2, 5, 8, 2)
    r.point('counter', 5, 7, ['TICKETS', 'GO AND VIA', 'ONE WAY, PLEASE'], dx=4)
    # Columns.
    for tx, ty in ((14, 6), (24, 6), (14, 15), (24, 15)):
        column(r, tx * 8, ty * 8)
        r.block(tx, ty, 2, 2)
    # Medallion in the floor.
    r.stamp(144, 88, ['.......##.......',
                      '......#cc#......',
                      '.....#cssc#.....',
                      '..##.#cssc#.##..',
                      '..#c##cssc##c#..',
                      '...#ccsssscc#...',
                      '.###csssssssc###',
                      '#ccsssssssssssc#',
                      '#ccsssssssssssc#',
                      '.###csssssssc###',
                      '...#ccsssscc#...',
                      '..#c##cssc##c#..',
                      '..##.#cssc#.##..',
                      '.....#cssc#.....',
                      '......#cc#......',
                      '.......##.......'], {'#': PLASTER_D, 'c': CREAM, 's': PLASTER})
    # Benches.
    for x, y in ((48, 96), (48, 104), (88, 96), (88, 104), (200, 96), (200, 104), (248, 96), (248, 104)):
        bench(r, x, y, 32)
    # The flag hanging over the middle of the hall (the player walks under).
    clear_over(r, 17, 7, 6, 3)
    r.frame(135, 55, 50, 26, PLASTER_D)
    r.frame(133, 53, 54, 30, PLASTER)
    with r.overhead():
        r.hline(140, 57, 40, INK)
        r.vline(140, 56, 3, INK)
        r.vline(179, 56, 3, INK)
        r.rect(144, 58, 32, 18, CREAM)
        r.frame(144, 58, 32, 18, INK)
        r.rect(145, 59, 8, 16, RED)
        r.rect(167, 59, 8, 16, RED)
        r.stamp(155, 62, ['....#....', '..#.#.#..', '#.#####.#', '#########', '.#######.', '..#####..',
                          '.###.###.', '....#....', '....#....'], {'#': RED})
        r.hline(145, 74, 30, RED_D)
    # Planters.
    for x in (16, 288):
        planter(r, x, 144, 16, 16)
        r.block(x // 8, 18, 2, 2)
    r.point('plaque', 30, 4, ['UNION STATION', 'OPENED 1927', 'RUNNING LATE SINCE'], dx=0)
    r.npc('clerk', 'counter', 5, 4, 's', area=(2, 4, 8, 1))
    r.npc('commuter', 'patrol', 10, 10, 'e', path=[(4, 10), (34, 10), (34, 17), (4, 17)])
    r.npc('commuter', 'wander', 20, 18, 'n', area=(12, 13, 16, 7))
    r.npc('tourist', 'gaze', 20, 5, 'n')
    r.npc('commuter', 'sit', 7, 12, 'n')
    r.npc('guard', 'patrol', 30, 8, 'w', path=[(30, 8), (35, 8), (35, 17), (30, 17)])
    r.npc('commuter', 'wander', 11, 12, 'e', area=(4, 11, 9, 4))
    r.npc('commuter', 'sit', 26, 13, 's')
    return r


def lantern_string(r, x0, x1, y, step=24, start=12):
    """Red paper lanterns on a string overhead (priority): the player walks
    under them. The floor of their tiles is cleared to plain cream."""
    for tx in range(x0 // 8, (x1 + 7) // 8):
        clear_over(r, tx, y // 8, 1, 2)
    with r.overhead():
        for x in range(x0, x1):
            r.px(x, y + 1 + (1 if (x - x0) % step in range(6, 18) else 0), INK)
        for x in range(x0 + start, x1 - 6, step):
            r.stamp(x - 3, y + 2, ['..##..', '.#rr#.', '#rRrr#', '#rRrr#', '#rRrr#', '.#rr#.', '..##..', '...#..', '..#R..'],
                    {'#': INK, 'r': RED, 'R': RED_D})


def stall_front(r, x, w, name, colour, colour_d, goods):
    """A small mall stall on the north wall face: a sign board with its
    name, shelves of goods behind (goods(r, x, w) draws them)."""
    r.rect(x, 8, w, 9, colour)
    r.frame(x, 8, w, 9, INK)
    r.hline(x + 1, 9, w - 2, colour_d)
    r.text_center(x + w // 2, 10, name, CREAM)
    r.rect(x, 17, w, 15, colour_d)
    r.hline(x, 31, w, INK)
    r.vline(x, 17, 15, INK)
    r.vline(x + w - 1, 17, 15, INK)
    goods(r, x, w)


def goods_jade(r, x, w):
    for yy in (18, 25):
        r.hline(x + 1, yy + 6, w - 2, CREAM)
        for k, xx in enumerate(range(x + 3, x + w - 5, 6)):
            if (k + yy) % 2:
                r.stamp(xx, yy + 1, ['.##.', '#..#', '#..#', '.##.'], {'#': JADE})
            else:
                r.stamp(xx, yy, ['.#..', '###.', '.#..', '###.', '#.#.'], {'#': JADE})


def goods_phones(r, x, w):
    for yy in (18, 25):
        r.hline(x + 1, yy + 6, w - 2, CREAM)
        for xx in range(x + 3, x + w - 4, 5):
            r.stamp(xx, yy, ['###', '#s#', '#s#', '#s#', '###'], {'#': INK, 's': SKY})


def goods_bakery(r, x, w):
    r.rect(x + 1, 18, w - 2, 13, CREAM)
    for yy in (19, 25):
        r.hline(x + 1, yy + 5, w - 2, GOLD_D)
        for k, xx in enumerate(range(x + 3, x + w - 5, 5)):
            r.stamp(xx, yy, ['.##.', '#gg#', '#gG#', '.##.'] if k % 2 else ['####', '#gg#', '####'], {'#': GOLD_D, 'g': GOLD, 'G': GOLD_D})


def goods_herbs(r, x, w):
    r.rect(x + 1, 18, w - 2, 13, TERRA)
    for yy in range(18, 31, 4):
        r.hline(x + 1, yy, w - 2, WOOD_D)
    for xx in range(x + 1, x + w - 1, 6):
        r.vline(xx, 18, 13, WOOD_D)
        for yy in range(20, 31, 4):
            r.px(xx + 3, yy, CREAM)


def escalator_ns(r, x, y, h):
    """One escalator running north-south (16 px wide), whole tiles."""
    r.rect(x, y, 16, h, INK)
    r.rect(x + 1, y, 2, h, SKY)
    r.rect(x + 13, y, 2, h, SKY)
    r.rect(x + 4, y + 6, 8, h - 12, SLATE)
    for yy in range(y + 7, y + h - 6, 3):
        r.hline(x + 4, yy, 8, CREAM if (yy // 3) % 2 else INK)
    r.rect(x + 4, y, 8, 6, CREAM)
    r.rect(x + 4, y + h - 6, 8, 6, CREAM)
    for yy in (y + 1, y + 3, y + h - 4, y + h - 2):
        r.hline(x + 5, yy, 6, SLATE)
    r.stamp(x + 6, y + h // 2 - 2, ['.##.', '####', '.##.', '.##.'], {'#': CREAM})


def dragon_city():
    """A Chinatown mall (an original take on Dragon City at Spadina and
    Dundas): a glass entrance court, stalls for jade and gifts, phones, a
    bakery counter, an herbal shop with drawers and a tea island, red
    lanterns overhead and an escalator."""
    r = K.Room('dragon_city', 32, 22, ['stone', 'red', 'brass', 'jade', 'glass', 'wood', 'leaf'], 'DRAGON CITY', 'landmark')
    wall = Wall(RED_D, INK, RED)
    shell(r, wall, 3, terrazzo(), mat=(RED, RED_D), door=(CREAM, SLATE))
    south_glass(r, 11, 4)
    south_glass(r, 17, 4)
    for side in ('w', 'e'):
        side_window(r, side, 15, 5)
    # The glass court: lighter floor under a lattice roof.
    for y in range(128, 168):
        for x in range(8, 248):
            k = y - 128
            if k == 0:
                c = STONE_D
            elif x % 16 == 8 or k % 16 == 8:
                c = STONE
            else:
                c = CREAM
            r.px(x, y, c, soft=CREAM if c != CREAM else None)
    door_mat(r, (RED, RED_D))
    # Stalls on the north wall.
    stalls = [(8, 56, 'JADE', JADE, JADE_D, goods_jade, ['JADE AND GIFTS', 'BANGLES, CATS', 'AND GOOD LUCK'], 'jade'),
              (72, 56, 'PHONES', SLATE, INK, goods_phones, ['PHONE REPAIR', 'CASES, CABLES,', 'CRACKS FIXED'], 'glass'),
              (136, 48, 'BAKERY', GOLD, GOLD_D, goods_bakery, ['BAKERY COUNTER', 'BUNS STILL WARM', 'TARTS GOING FAST'], 'brass'),
              (192, 56, 'HERBAL', TERRA, WOOD_D, goods_herbs, ['HERBAL SHOP', 'A HUNDRED DRAWERS', 'ONE FOR YOU'], 'wood')]
    for x, w, name, c, cd, goods, text, pal in stalls:
        stall_front(r, x, w, name, c, cd, goods)
        r.rect(x, 40, w, 16, c)
        r.frame(x, 40, w, 9, INK)
        r.hline(x + 1, 41, w - 2, CREAM)
        r.rect(x, 48, w, 8, cd)
        r.frame(x, 48, w, 8, INK)
        for xx in range(x + 4, x + w - 3, 8):
            r.px(xx, 51, CREAM)
        r.block(x // 8, 5, w // 8, 2)
        r.point('counter', (x + w // 2) // 8, 7, text, dx=(x + w // 2) % 8 - 4)
    for x in (64, 128, 184):
        r.rect(x, 8, 8, 24, RED)
        r.frame(x, 8, 8, 24, INK)
        r.vline(x + 2, 9, 22, CREAM)
    # Tea island in the middle.
    r.rect(80, 80, 64, 16, JADE_D)
    r.frame(80, 80, 64, 16, INK)
    r.rect(82, 82, 60, 12, CREAM)
    for k, xx in enumerate(range(85, 139, 9)):
        if k % 2:
            r.stamp(xx, 84, ['.##..', '#jj##', '#jj#.', '.##..'], {'#': INK, 'j': JADE_D})
        else:
            r.stamp(xx, 83, ['####', '#jj#', '#jj#', '#jj#', '####'], {'#': INK, 'j': JADE})
    r.rect(80, 96, 64, 8, WOOD_D)
    r.frame(80, 96, 64, 8, INK)
    r.text_center(112, 98, 'TEA', CREAM)
    r.block(10, 10, 8, 3)
    r.point('counter', 13, 13, ['TEA ISLAND', 'OOLONG, PUERH,', 'BUBBLE: SOLD OUT'], dx=4)
    # Escalator to the upper floors.
    escalator_ns(r, 216, 72, 48)
    r.block(27, 9, 2, 6)
    r.point('elevator', 26, 11, ['ESCALATOR', 'MORE SHOPS ABOVE', 'DIM SUM AT TOP'], dx=0)
    # Lucky bamboo planters in the court.
    for x in (24, 216):
        planter(r, x, 136, 16, 16)
        r.block(x // 8, 17, 2, 2)
    # Lanterns overhead.
    lantern_string(r, 8, 248, 64)
    lantern_string(r, 8, 200, 112, start=20)
    r.npc('clerk', 'counter', 3, 4, 's', area=(1, 4, 7, 1))
    r.npc('clerk', 'counter', 18, 4, 's', area=(17, 4, 6, 1))
    r.npc('shopper', 'wander', 8, 15, 'n', area=(2, 14, 22, 2))
    r.npc('shopper', 'window', 11, 7, 'n', area=(9, 7, 7, 1))
    r.npc('teen', 'wander', 20, 9, 'w', area=(19, 8, 6, 6))
    r.npc('tourist', 'gaze', 15, 18, 'n')
    r.npc('tourist', 'gaze', 16, 18, 'n')
    r.npc('clerk', 'counter', 12, 9, 's', area=(10, 9, 8, 1))
    return r


def bbq_window(r, x, w):
    """A steamy shop window with roast ducks and char siu on hooks (red
    palette, the wall face rows)."""
    r.rect(x, 8, w, 24, CREAM)
    r.frame(x, 8, w, 24, INK)
    r.hline(x + 1, 11, w - 2, INK)
    for xx in range(x + 4, x + w - 6, 10):
        r.vline(xx + 3, 12, 2, INK)
        r.stamp(xx, 14, ['..#...', '.#rr#.', '#rRRr#', '#rRRr#', '#rRRr#', '.#rr#.', '..##..', '..#...'], {'#': INK, 'r': RED, 'R': RED_D})
    for xx in range(x + 9, x + w - 6, 10):
        r.vline(xx, 12, 2, INK)
        r.rect(xx - 1, 14, 3, 9, RED_D)
        r.vline(xx - 1, 14, 9, RED)
    r.rect(x, 26, w, 6, RED_D)
    r.hline(x, 26, w, INK)
    r.hline(x + 1, 28, w - 2, RED)


def menu_board(r, x, w, title, rows):
    r.rect(x, 8, w, 24, RED)
    r.frame(x, 8, w, 24, INK)
    r.frame(x + 1, 9, w - 2, 22, RED_D)
    r.text_center(x + w // 2, 11, title, CREAM)
    for k, (icon, price) in enumerate(rows):
        yy = 18 + k * 6
        r.stamp(x + 4, yy, icon, {'#': CREAM})
        r.text(x + w - 4 - K.mini_width(price), yy, price, CREAM)
        for xx in range(x + 10, x + w - 6 - K.mini_width(price), 2):
            r.px(xx, yy + 4, RED_D)


def chinatown_bakery():
    """A Spadina bakery and barbecue shop: cases of buns and tarts, a
    window of hanging roast meats, a counter, small tables, menu boards."""
    r = K.Room('chinatown_bakery', 24, 18, ['stone', 'red', 'brass', 'wood', 'glass', 'leaf'], 'GOLDEN BUN BBQ', 'landmark')
    wall = Wall(CREAM, STONE_D, RED)
    small_checks = ['aaaabbbb'] * 4 + ['bbbbaaaa'] * 4
    shell(r, wall, 3, pattern_floor(small_checks, {'a': CREAM, 'b': STONE}, soft={'b': CREAM}), mat=(RED, RED_D),
          door=(CREAM, STONE_D))
    bbq_window(r, 16, 56)
    menu_board(r, 80, 40, 'BAKERY', [(['.##.', '#..#', '.##.'], '$1'), (['####', '#..#', '####'], '$2')])
    menu_board(r, 128, 48, 'BBQ', [(['.###', '#..#', '###.'], '$9'), (['###.', '####', '###.'], '$12')])
    # BBQ counter with a chopping block under the window.
    r.rect(16, 40, 56, 16, TERRA)
    r.frame(16, 40, 56, 9, INK)
    r.rect(24, 41, 16, 7, CREAM)
    r.frame(24, 41, 16, 7, WOOD_D)
    r.stamp(44, 42, ['######..', '#cccc##.', '######..', '....##..'], {'#': INK, 'c': CREAM})
    r.rect(16, 48, 56, 8, WOOD_D)
    r.frame(16, 48, 56, 8, INK)
    r.text_center(44, 50, 'BBQ', CREAM)
    r.block(2, 5, 7, 2)
    # Cases of buns and tarts and the till.
    pastry_case(r, 88, 48, 48, 16)
    pastry_case(r, 136, 48, 32, 16)
    r.rect(168, 48, 16, 16, CREAM)
    r.frame(168, 48, 16, 9, INK)
    r.rect(168, 56, 16, 8, STONE)
    r.frame(168, 56, 16, 8, INK)
    r.stamp(170, 49, ['.######.', '#ssssss#', '#scccs##', '########'], {'#': INK, 's': STONE_D, 'c': CREAM})
    r.block(11, 6, 10, 2)
    r.point('counter', 21, 8, ['GOLDEN BUN BBQ', 'PINEAPPLE BUNS', 'CONTAIN NO PINE'], dx=0)
    r.point('window', 5, 7, ['ROAST DUCK', 'CHAR SIU, CRISPY', 'PORK: ASK SOON'], dx=4)
    r.point('screen', 13, 8, ['MENU', 'BUNS FROM $1', 'TARTS STILL WARM'], dx=4)
    # Small tables with stools.
    for cx, cy in ((40, 104), (88, 120), (152, 104), (168, 128)):
        table_round(r, cx, cy, STONE_D, shade=STONE)
        r.hline(cx - 3, cy - 2, 6, CREAM)
        stool(r, cx - 10, cy, STONE_D, INK)
        stool(r, cx + 10, cy, STONE_D, INK)
        r.block((cx - 5) // 8, (cy - 4) // 8, 2, 1)
    planter(r, 8, 120, 16, 16)
    r.block(1, 15, 2, 2)
    # A drinks fridge against the east wall: its top, and the glass door
    # facing into the shop.
    r.rect(168, 80, 16, 32, SLATE)
    r.frame(168, 80, 16, 32, INK)
    r.rect(169, 82, 6, 28, SKY)
    r.vline(175, 81, 30, INK)
    for y in range(84, 108, 6):
        r.hline(169, y + 4, 6, SLATE)
        r.stamp(170, y, ['#.#', '#.#', '###', '#c#'][:4], {'#': INK, 'c': CREAM})
    r.vline(170, 83, 26, CREAM)
    r.rect(177, 82, 6, 28, CREAM)
    r.block(21, 10, 2, 4)
    r.point('window', 20, 11, ['DRINKS FRIDGE', 'SOY MILK, TEA,', 'GRASS JELLY'])
    r.npc('cashier', 'counter', 21, 5, 's', area=(19, 5, 3, 1))
    r.npc('chef', 'counter', 4, 4, 's', area=(2, 4, 7, 1))
    r.npc('diner', 'sit', 3, 13, 'e')
    r.npc('diner', 'sit', 20, 16, 'w')
    r.npc('shopper', 'wander', 14, 10, 'n', area=(10, 9, 8, 3))
    r.npc('shopper', 'gaze', 15, 10, 'n')
    return r


def tv(r, x, y, w, h, scene):
    """A television on the wall: ink bezel, a bright screen with an image."""
    r.rect(x, y, w, h, INK)
    sx, sy, sw, sh = x + 1, y + 1, w - 2, h - 3
    r.rect(sx, sy, sw, sh, SCR)
    if scene == 'sky':
        r.rect(sx, sy + sh * 2 // 3, sw, sh - sh * 2 // 3, SCR_D)
        r.stamp(sx + 2, sy + 1, ['.##.', '####'], {'#': SCR_L})
    elif scene == 'rink':
        r.rect(sx, sy, sw, sh, SCR_L)
        r.vline(sx + sw // 2, sy, sh, SCR)
        r.frame(sx + 1, sy + 1, sw - 2, sh - 2, SCR_D)
        r.px(sx + sw // 2 + 2, sy + sh // 2, INK)
    elif scene == 'game':
        r.rect(sx, sy + sh - 3, sw, 3, SCR_D)
        for i in range(0, sw - 2, 4):
            r.px(sx + i + 1, sy + sh - 4, SCR_D)
        r.stamp(sx + 3, sy + sh - 7, ['#.', '##', '##', '#.'], {'#': SCR_L})
        r.stamp(sx + sw - 6, sy + 2, ['##', '##'], {'#': SCR_L})
    elif scene == 'bars':
        for i in range(sw):
            r.vline(sx + i, sy, sh, (SCR_L, SCR, SCR_D, INK)[(i * 4) // sw])
    elif scene == 'city':
        for i in range(0, sw, 3):
            hh = 2 + (i * 7) % (sh - 1)
            r.rect(sx + i, sy + sh - hh, 2, hh, SCR_D)
    elif scene == 'face':
        r.stamp(sx + sw // 2 - 3, sy + 1, ['.####.', '#llll#', '#l#l##', '#llll#', '.#ll#.'][:sh], {'#': SCR_D, 'l': SCR_L})
    r.hline(x, y + h - 2, w, INK)
    r.hline(x + w // 2 - 2, y + h - 1, 4, INK)


def aisle(r, x, y, w, h, box, box_d):
    """A shelving run seen from above with boxes on it (whole tiles)."""
    r.rect(x, y, w, h, CREAM)
    r.frame(x, y, w, h, INK)
    r.vline(x + w // 2, y + 1, h - 2, box_d)
    r.hline(x + 1, y + h - 2, w - 2, box_d)
    for yy in range(y + 2, y + h - 4, 6):
        for xx in (x + 2, x + w // 2 + 2):
            r.rect(xx, yy, w // 2 - 4, 4, box)
            r.frame(xx, yy, w // 2 - 4, 4, box_d)


def electronics():
    """BYTE BARN, a fictional big-box electronics store: a wall of TVs,
    aisles of shelves, a game demo station, a phones counter and checkouts
    by the doors. Barn red and cream are its own colours."""
    r = K.Room('electronics', 40, 26, ['stone', 'red', 'screen', 'dusk', 'glass', 'wood', 'jade'], 'BYTE BARN', 'landmark')
    wall = Wall(DUSK_D, INK, DUSK)
    shell(r, wall, 3, slabs(CREAM, STONE, size=16), mat=(RED, RED_D), door=(CREAM, SLATE))
    south_glass(r, 16, 3)
    south_glass(r, 21, 3)
    # The wall of TVs.
    r.rect(8, 8, 192, 24, INK)
    scenes = ['sky', 'game', 'rink', 'city', 'bars', 'face', 'sky', 'game', 'rink', 'city', 'face', 'bars']
    k = 0
    for x in range(8, 200, 32):
        tv(r, x + 1, 9, 30, 14, scenes[k % 12])
        k += 1
    for x in range(8, 200, 16):
        tv(r, x + 1, 24, 14, 8, scenes[k % 12])
        k += 1
    r.point('screen', 13, 4, ['TV WALL', 'FORTY CHANNELS', 'ALL THE SAME GAME'], dx=0)
    # The BYTE BARN sign: a barn with a pixel in its loft.
    r.rect(216, 8, 96, 24, RED)
    r.frame(216, 8, 96, 24, INK)
    r.frame(217, 9, 94, 22, CREAM)
    r.stamp(222, 11, ['....##....', '...#cc#...', '..#cccc#..', '.#cc##cc#.', '#ccc##ccc#', '.#cccccc#.', '.#c#cc#c#.',
                      '.#c#cc#c#.', '.#cc##cc#.', '.########.'], {'#': CREAM, 'c': RED_D})
    r.text(236, 12, 'BYTE', CREAM, font='big')
    r.text(272, 12, 'BARN', CREAM, font='big')
    r.text(237, 23, 'GADGETS AND MORE', CREAM)
    # Aisles of shelves.
    for x, box, box_d in ((32, RED, RED_D), (72, JADE, JADE_D), (112, CREAM, DUSK_D), (152, RED, RED_D)):
        aisle(r, x, 64, 16, 80, box, box_d)
        r.block(x // 8, 8, 2, 10)
    # Game demo station.
    r.rect(232, 56, 48, 24, JADE_D)
    r.frame(232, 56, 48, 24, INK)
    r.rect(236, 58, 40, 4, JADE)
    r.rect(240, 62, 32, 12, INK)
    r.rect(241, 63, 30, 9, JADE)
    r.rect(241, 69, 30, 3, JADE_D)
    r.stamp(245, 65, ['#.', '##', '#.'], {'#': CREAM})
    r.stamp(262, 64, ['##', '##'], {'#': CREAM})
    r.stamp(250, 75, ['.######.', '#c#cc#c#', '.######.'], {'#': INK, 'c': CREAM})
    r.block(29, 7, 6, 3)
    r.point('screen', 32, 10, ['GAME DEMO', 'PLAY ONE LEVEL', 'THEN ONE MORE'], dx=0)
    # Phones counter.
    r.rect(232, 112, 64, 16, SKY)
    r.frame(232, 112, 64, 10, INK)
    r.rect(232, 121, 64, 7, SLATE)
    r.frame(232, 121, 64, 7, INK)
    for xx in range(237, 292, 7):
        r.stamp(xx, 114, ['###', '#c#', '#c#', '###'], {'#': INK, 'c': CREAM})
    for i in range(3):
        r.px(234 + i, 120 - i, CREAM)
    r.text(240, 123, 'PHONES', CREAM)
    r.block(29, 14, 8, 2)
    r.point('counter', 33, 16, ['PHONES', 'NEW, USED AND', 'DROPPED ONCE'], dx=0)
    # Laptop table, sale endcaps and a big TV on a stand.
    r.rect(176, 88, 32, 16, DUSK)
    r.frame(176, 88, 32, 16, INK)
    r.rect(176, 99, 32, 5, DUSK_D)
    r.hline(176, 99, 32, INK)
    for xx in (179, 193):
        r.stamp(xx, 89, ['##########', '#cccccccc#', '#cccccccc#', '#cccccccc#', '##########', '#dddddddd#',
                         '##########'], {'#': INK, 'c': CREAM, 'd': DUSK_D})
    r.block(22, 11, 4, 2)
    r.point('counter', 23, 13, ['LAPTOPS', 'THIN, LIGHT,', 'ALREADY OUTDATED'], dx=4)
    for x in (32, 152):
        r.rect(x, 144, 16, 8, RED)
        r.frame(x, 144, 16, 8, INK)
        r.text_center(x + 8, 145, 'SALE', CREAM)
        r.hline(x + 1, 150, 14, RED_D)
        r.block(x // 8, 18, 2, 1)
    r.rect(184, 128, 32, 16, INK)
    tv(r, 185, 128, 30, 13, 'sky')
    r.rect(196, 141, 8, 3, INK)
    r.block(23, 16, 4, 2)
    r.point('screen', 25, 18, ['BIG SCREEN', 'SO BIG YOU CAN', 'SEE YOUR HOUSE'], dx=0)
    # Checkouts by the doors.
    for x in (64, 216):
        r.rect(x, 160, 48, 16, TERRA)
        r.frame(x, 160, 48, 9, INK)
        r.rect(x + 2, 162, 28, 5, INK)
        for xx in range(x + 3, x + 29, 3):
            r.vline(xx, 163, 3, WOOD_D)
        r.rect(x, 168, 48, 8, WOOD_D)
        r.frame(x, 168, 48, 8, INK)
        r.stamp(x + 34, 161, ['######', '#cccc#', '######', '..##..'], {'#': INK, 'c': CREAM})
        r.block(x // 8, 20, 6, 2)
    r.point('counter', 10, 22, ['CHECKOUT', 'RECEIPT IS LONGER', 'THAN THE CABLE'], dx=4)
    r.npc('clerk', 'counter', 33, 13, 's', area=(30, 13, 6, 1))
    r.npc('cashier', 'counter', 11, 19, 's', area=(9, 19, 4, 1))
    r.npc('teen', 'gaze', 32, 11, 'n')
    r.npc('shopper', 'gaze', 33, 11, 'n')
    r.npc('shopper', 'wander', 7, 12, 'n', area=(6, 8, 3, 10))
    r.npc('shopper', 'wander', 16, 14, 's', area=(16, 8, 3, 10))
    r.npc('clerk', 'patrol', 23, 9, 'w', path=[(23, 6), (23, 18), (6, 18), (6, 6)])
    r.npc('shopper', 'window', 8, 5, 'n', area=(2, 4, 22, 2))
    return r


def wall_goods(r, x, y, w, h, colour, colour_d, kind):
    """Shelving on the north wall face filled with goods (whole tiles)."""
    r.rect(x, y, w, h, colour_d)
    r.frame(x, y, w, h, INK)
    shelf_h = h // 2
    for k in range(2):
        sy = y + k * shelf_h
        r.hline(x + 1, sy + shelf_h - 1, w - 2, INK if k else CREAM)
        for i, xx in enumerate(range(x + 2, x + w - 3, 4)):
            if kind == 'cans':
                r.stamp(xx, sy + shelf_h - 6, ['###', '#c#', '#c#', '###', '###'][:5], {'#': colour, 'c': CREAM})
            elif kind == 'chips':
                r.stamp(xx, sy + shelf_h - 7, ['.#.', '###', '#c#', '###', '###', '.#.'][:6], {'#': colour, 'c': CREAM})
            else:
                hh = 5 + (i * 3 + k) % 3
                r.rect(xx, sy + shelf_h - 1 - hh, 3, hh, colour if (i + k) % 3 else CREAM)
                r.vline(xx + 3, sy + shelf_h - 1 - hh, hh, INK)


def fridge_wall(r, x, w):
    """Drinks fridges with glass doors on the north wall face."""
    r.rect(x, 8, w, 24, SLATE)
    r.frame(x, 8, w, 24, INK)
    r.rect(x + 1, 9, w - 2, 6, CREAM)
    r.text_center(x + w // 2, 10, 'COLD DRINKS', INK)
    for dx in range(x + 1, x + w - 1, 16):
        r.rect(dx, 15, 15, 16, SKY)
        r.frame(dx, 15, 15, 16, INK)
        for sy in (18, 24):
            r.hline(dx + 1, sy + 5, 13, SLATE)
            for bx in range(dx + 2, dx + 13, 3):
                r.stamp(bx, sy, ['.#', '##', '#c', '#c', '##'], {'#': INK, 'c': CREAM if (bx // 3) % 2 else SLATE})
        r.vline(dx + 13, 17, 12, CREAM)


def shelf_unit(r, x, y, w, h, colours):
    """A freestanding shelving unit seen from above (whole tiles): white
    shelving, goods in rows, colours cycle (all in one palette)."""
    r.rect(x, y, w, h, CREAM)
    r.frame(x, y, w, h, INK)
    r.vline(x + w // 2, y + 1, h - 2, colours[-1])
    for k, yy in enumerate(range(y + 2, y + h - 3, 5)):
        for side in (x + 2, x + w // 2 + 2):
            c = colours[(k + side // 8) % (len(colours) - 1)]
            r.rect(side, yy, w // 2 - 4, 3, c)
            r.hline(side, yy + 3, w // 2 - 4, colours[-1])


def corner_store():
    """Generic corner store: wall shelves of goods, a wall of drinks
    fridges, two shelving runs, a counter by the door with candy and
    lottery tickets, an ice chest."""
    r = K.Room('corner_store', 20, 18, ['stone', 'glass', 'red', 'wood', 'brass', 'jade'], 'CORNER STORE')
    wall = Wall(CREAM, STONE_D, STONE_D)
    shell(r, wall, 3, slabs(CREAM, STONE, size=16), mat=(RED, RED_D), door=(CREAM, STONE_D))
    wall_goods(r, 8, 8, 24, 24, RED, RED_D, 'cans')
    wall_goods(r, 32, 8, 32, 24, GOLD, GOLD_D, 'chips')
    fridge_wall(r, 72, 80)
    r.point('window', 13, 4, ['COLD DRINKS', 'POP, JUICE,', 'MYSTERY FLAVOUR'], dx=0)
    shelf_unit(r, 32, 56, 16, 48, [RED, CREAM, RED_D])
    shelf_unit(r, 72, 56, 16, 48, [JADE, CREAM, JADE_D])
    r.block(4, 7, 2, 6)
    r.block(9, 7, 2, 6)
    # Counter by the door: candy rack, register and lottery screen.
    r.rect(112, 104, 40, 16, TERRA)
    r.frame(112, 104, 40, 9, INK)
    r.hline(113, 105, 38, CREAM)
    r.rect(112, 112, 40, 8, WOOD_D)
    r.frame(112, 112, 40, 8, INK)
    for xx in range(115, 131, 4):
        r.stamp(xx, 106, ['##', '#c', '##'], {'#': INK, 'c': CREAM})
    r.stamp(135, 105, ['########', '#cccccc#', '#cccccc#', '########', '...##...'], {'#': INK, 'c': CREAM})
    r.block(14, 13, 5, 2)
    r.point('counter', 16, 15, ['CORNER STORE', 'MILK, BREAD,', 'LOTTERY HOPE'], dx=0)
    # Newspaper rack and ice chest.
    r.rect(16, 112, 16, 16, SKY)
    r.frame(16, 112, 16, 9, INK)
    r.rect(17, 113, 14, 7, CREAM)
    r.text(18, 114, 'ICE', SLATE)
    r.rect(16, 120, 16, 8, SLATE)
    r.frame(16, 120, 16, 8, INK)
    r.block(2, 14, 2, 2)
    r.rect(48, 120, 16, 8, CREAM)
    r.frame(48, 120, 16, 8, INK)
    for xx in range(50, 62, 3):
        r.rect(xx, 121, 2, 5, STONE_D)
    r.block(6, 15, 2, 1)
    r.npc('cashier', 'counter', 16, 12, 's', area=(14, 12, 5, 1))
    r.npc('shopper', 'wander', 7, 10, 'n', area=(6, 7, 3, 6))
    r.npc('teen', 'window', 12, 5, 'n', area=(9, 4, 9, 2))
    r.npc('shopper', 'wander', 3, 9, 's', area=(1, 6, 3, 6))
    return r


def booth(r, x, y, seat, seat_d, table=CREAM, table_d=STONE):
    """A diner booth seen from above: seat, table, seat (24 x 24 px). The
    seats stay open for sitting NPCs; the table is solid."""
    for sy, back in ((y, y), (y + 16, y + 22)):
        r.rect(x, sy, 24, 8, seat)
        r.frame(x, sy, 24, 8, INK)
        r.rect(x + 1, back if back == y else back - 1, 22, 2, seat_d)
    r.rect(x + 2, y + 8, 20, 8, table)
    r.frame(x + 2, y + 8, 20, 8, INK)
    r.hline(x + 3, y + 14, 18, table_d)
    r.stamp(x + 6, y + 10, ['##', '#c'], {'#': INK, 'c': CREAM})
    r.block(x // 8, y // 8 + 1, 3, 1)


def diner():
    """Generic diner: a counter with stools before the kitchen pass, red
    booths, a pie case, a jukebox, black and white floor."""
    r = K.Room('diner', 24, 18, ['red', 'mint', 'stone', 'brass', 'wood', 'glass'], 'DINER')
    wall = Wall(MINT, MINT_D, MINT_D, wainscot=(CREAM, 8, MINT_D))
    shell(r, wall, 3, checker(CREAM, INK, 8), mat=(RED, RED_D), door=(CREAM, INK))
    # Kitchen pass: steel shelf, order tickets, a bell; the kitchen beyond.
    r.rect(24, 8, 64, 16, INK)
    r.frame(24, 8, 64, 16, STONE_D)
    r.rect(25, 20, 62, 4, STONE)
    r.hline(25, 20, 62, CREAM)
    r.hline(24, 23, 64, STONE_D)
    for xx in range(30, 82, 9):
        r.stamp(xx, 9, ['####', '#cc#', '#c.#', '#cc#', '####'], {'#': STONE_D, 'c': CREAM, '.': STONE})
    r.stamp(40, 16, ['.##.', '####'], {'#': CREAM})
    r.stamp(72, 18, ['.#.', '###'], {'#': STONE_D})
    r.point('window', 6, 8, ['ORDER UP!', 'TWO OVER EASY', 'HOLD THE DRAMA'], dx=4)
    # Menu board and clock.
    r.rect(104, 8, 48, 16, RED)
    r.frame(104, 8, 48, 16, INK)
    r.text_center(128, 10, 'EAT', CREAM, font='big')
    r.stamp(160, 10, ['.####.', '#c#cc#', '#c##c#', '#cccc#', '.####.'], {'#': INK, 'c': CREAM})
    # Counter with stools.
    r.rect(16, 40, 104, 16, MINT)
    r.frame(16, 40, 104, 9, INK)
    r.hline(17, 41, 102, CREAM)
    r.rect(16, 48, 104, 8, MINT_D)
    r.frame(16, 48, 104, 8, INK)
    for xx in range(20, 118, 12):
        r.hline(xx, 51, 6, CREAM)
    r.stamp(96, 41, ['.######.', '#cccccc#', '#cGcGcG#', '########'], {'#': INK, 'c': CREAM, 'G': MINT_D})
    r.block(2, 5, 13, 2)
    for xx in (24, 40, 56, 72, 88, 104):
        stool(r, xx, 60, RED, RED_D)
    r.point('counter', 7, 7, ['DINER COUNTER', 'COFFEE IS BOTTOM', 'LESS. LIKE TIME.'], dx=4)
    # Booths along the east wall and the south.
    for y in (40, 72, 104):
        booth(r, 160, y, RED, RED_D)
    for x in (24, 64):
        booth(r, x, 96, RED, RED_D)
    # Jukebox.
    r.stamp(136, 112, ['..######..', '.#gggggg#.', '#gccccccg#', '#gc####cg#', '#gc#rr#cg#',
                       '#gc####cg#', '#gccccccg#', '#gggggggg#', '#g#g##g#g#', '##########'],
            {'#': INK, 'g': RED, 'c': CREAM, 'r': RED_D})
    r.block(17, 14, 2, 2)
    r.point('screen', 17, 16, ['JUKEBOX', 'B4: A SONG ABOUT', 'A DIFFERENT DINER'], dx=4)
    r.npc('chef', 'counter', 8, 4, 's', area=(2, 4, 12, 1))
    r.npc('diner', 'sit', 5, 7, 'n', dx=-1)
    r.npc('diner', 'sit', 9, 7, 'n', dx=-1)
    r.npc('diner', 'sit', 21, 5, 's')
    r.npc('diner', 'sit', 4, 14, 'n')
    r.npc('clerk', 'patrol', 18, 8, 's', path=[(17, 8), (17, 16), (14, 16), (14, 8)])
    r.npc('patron', 'wander', 11, 11, 'n', area=(8, 9, 7, 4))
    return r


def book_wall(r, x, w, palette_colours):
    """Floor-to-ceiling bookshelves on the north wall face."""
    c, cd = palette_colours
    r.rect(x, 8, w, 24, WOOD_D if c == TERRA else cd)
    r.frame(x, 8, w, 24, INK)
    for sy in (9, 17, 25):
        k = sy
        xx = x + 2
        while xx < x + w - 3:
            bw = 2 + (xx * 7 + sy) % 2
            hh = 5 + (xx * 3 + sy) % 2
            col = (c, CREAM, cd)[(xx * 5 + sy) % 3]
            r.rect(xx, sy + 6 - hh, bw, hh, col)
            r.vline(xx + bw, sy + 6 - hh, hh, INK)
            xx += bw + 1
        r.hline(x + 1, sy + 6, w - 2, INK)


def record_bin(r, x, y, sleeves):
    """A crate of records seen from above (16 x 16)."""
    r.rect(x, y, 16, 16, sleeves[-1])
    r.frame(x, y, 16, 16, INK)
    r.rect(x + 2, y + 2, 12, 10, INK)
    for k, yy in enumerate(range(y + 3, y + 12, 2)):
        r.hline(x + 3, yy, 10, sleeves[k % len(sleeves)])
    r.hline(x + 1, y + 13, 14, CREAM)


def shop():
    """Generic book and record shop: bookshelves along the walls, a
    freestanding case, record bins, a counter with a turntable, a reading
    nook."""
    r = K.Room('shop', 20, 18, ['wood', 'red', 'violet', 'jade', 'brass', 'teal', 'leaf'], 'BOOKS & RECORDS')
    wall = Wall(TERRA, WOOD_D, WOOD_D)
    shell(r, wall, 3, planks(), mat=(WOOD_D, INK))
    book_wall(r, 8, 48, (RED, RED_D))
    book_wall(r, 56, 32, (JADE, JADE_D))
    book_wall(r, 88, 32, (VIO, VIO_D))
    book_wall(r, 120, 32, (GOLD, GOLD_D))
    r.point('plaque', 4, 4, ['FICTION', 'A TO Z, MOSTLY', 'SOME Q MISSING'], dx=0)
    # Freestanding bookcase.
    r.rect(24, 56, 48, 16, RED_D)
    r.frame(24, 56, 48, 16, INK)
    for xx in range(26, 70, 3):
        r.vline(xx, 58, 5, (RED, CREAM)[(xx // 3) % 2])
        r.vline(xx, 65, 5, (CREAM, RED)[(xx // 3) % 2])
    r.hline(25, 64, 46, INK)
    r.block(3, 7, 6, 2)
    # Record bins.
    for x, y, sl in ((24, 96, (VIO, CREAM, VIO_D)), (48, 96, (JADE, CREAM, JADE_D)), (24, 120, (RED, CREAM, RED_D)),
                     (48, 120, (VIO, CREAM, VIO_D))):
        record_bin(r, x, y, sl)
        r.block(x // 8, y // 8, 2, 2)
    r.point('exhibit', 5, 11, ['RECORD BINS', 'DIG FOR GOLD,', 'FIND POLKA'], dx=0)
    # Counter with a turntable.
    r.rect(104, 64, 40, 16, TERRA)
    r.frame(104, 64, 40, 9, INK)
    r.hline(105, 65, 38, CREAM)
    r.stamp(108, 65, ['.#####.', '#ccccc#', '#c###c#', '#ccccc#', '.#####.'][:5], {'#': INK, 'c': WOOD_D})
    r.stamp(124, 66, ['######', '#cccc#', '######'], {'#': INK, 'c': CREAM})
    r.rect(104, 72, 40, 8, WOOD_D)
    r.frame(104, 72, 40, 8, INK)
    r.block(13, 8, 5, 2)
    r.point('counter', 15, 10, ['BOOKS & RECORDS', 'NOW PLAYING:', 'SIDE B, FOREVER'], dx=4)
    # Reading nook: rug, armchair, lamp, plant.
    r.rect(104, 104, 40, 24, TEAL)
    r.frame(104, 104, 40, 24, INK)
    r.frame(106, 106, 36, 20, CREAM)
    r.stamp(112, 108, ['.########.', '##ssssss##', '#sssssss##', '#ssssssss#', '##ssssss##', '.########.'],
            {'#': INK, 's': SLATE})
    r.stamp(132, 110, ['.##.', '#cc#', '#cc#', '.##.'], {'#': INK, 'c': CREAM})
    plant_box(r, 136, 32, 16, 16)
    r.block(17, 4, 2, 2)
    r.npc('clerk', 'counter', 15, 7, 's', area=(13, 7, 5, 1))
    r.npc('shopper', 'wander', 2, 12, 'n', area=(1, 10, 2, 6))
    r.npc('teen', 'gaze', 5, 11, 'n')
    r.npc('patron', 'sit', 15, 13, 's')
    r.npc('shopper', 'window', 6, 4, 'n', area=(1, 4, 15, 2))
    return r


def office_lobby():
    """Generic office lobby: marble floor, a security desk, glass gates, a
    row of staff-only elevators, plants and lounge seating."""
    r = K.Room('office_lobby', 24, 18, ['stone', 'glass', 'leaf', 'dusk', 'teal', 'brass'], 'OFFICE LOBBY')
    wall = Wall(STONE, STONE_D, STONE_D)
    shell(r, wall, 3, slabs(CREAM, STONE, speck=STONE_D, size=16), mat=(DUSK_D, INK), door=(CREAM, STONE_D))
    south_glass(r, 8, 3)
    south_glass(r, 13, 3)
    # Staff-only elevators: brushed steel doors.
    for tx in (5, 9, 13, 17):
        x = tx * 8
        r.rect(x, 8, 24, 24, STONE_D)
        r.frame(x, 8, 24, 24, INK)
        r.rect(x + 3, 13, 18, 19, INK)
        r.rect(x + 4, 14, 16, 18, STONE)
        r.vline(x + 11, 14, 18, INK)
        r.vline(x + 12, 14, 18, INK)
        r.vline(x + 5, 15, 16, CREAM)
        r.vline(x + 14, 15, 16, CREAM)
        r.rect(x + 8, 9, 8, 3, INK)
        r.px(x + 11, 10, CREAM)
        r.point('elevator', tx + 1, 4, ['STAFF ONLY', 'PASS REQUIRED', 'NICE TRY THOUGH'], dx=0)
    r.rect(10, 10, 26, 16, CREAM)
    r.frame(10, 10, 26, 16, INK)
    r.text_center(23, 12, 'STAFF', INK)
    r.text_center(23, 18, 'ONLY', INK)
    # Glass security gates in a row with the desk.
    for x in (40, 64, 112, 136):
        r.rect(x, 64, 8, 8, SKY)
        r.frame(x, 64, 8, 8, INK)
        r.vline(x + 2, 65, 6, CREAM)
    r.block(5, 8, 1, 1)
    r.block(8, 8, 1, 1)
    r.block(14, 8, 1, 1)
    r.block(17, 8, 1, 1)
    for x0, x1 in ((8, 40), (144, 184)):
        r.rect(x0, 64, x1 - x0, 8, SKY)
        r.frame(x0, 64, x1 - x0, 8, INK)
        r.hline(x0 + 1, 66, x1 - x0 - 2, CREAM)
    r.block(1, 8, 4, 1)
    r.block(18, 8, 5, 1)
    # Security desk in the middle of the gate line.
    r.rect(72, 56, 40, 24, DUSK)
    r.frame(72, 56, 40, 16, INK)
    r.rect(74, 58, 36, 4, CREAM)
    r.stamp(80, 62, ['######', '#cccc#', '######'], {'#': INK, 'c': CREAM})
    r.rect(72, 72, 40, 8, DUSK_D)
    r.frame(72, 72, 40, 8, INK)
    r.text_center(92, 74, 'SECURITY', CREAM)
    r.block(9, 7, 5, 3)
    r.point('desk', 11, 10, ['SECURITY DESK', 'SIGN IN, SMILE,', 'WAIT FOREVER'], dx=4)
    # Lounge: sofa, chairs, coffee table, plants.
    r.rect(24, 104, 40, 8, TEAL)
    r.frame(24, 104, 40, 8, INK)
    r.hline(25, 105, 38, SLATE)
    r.rect(32, 116, 24, 8, CREAM)
    r.frame(32, 116, 24, 8, INK)
    r.stamp(36, 117, ['####', '#cc#'], {'#': INK, 'c': SLATE})
    r.block(4, 14, 3, 1)
    for x in (144, 168):
        r.stamp(x, 104, ['################', '#ssssssssssssss#', '#ssssssssssssss#', '#s############s#',
                         '#s#tttttttttt#s#', '#s#tttttttttt#s#', '#s#tttttttttt#s#', '#s#tttttttttt#s#',
                         '#s#tttttttttt#s#', '#s############s#', '#ssssssssssssss#', '################'],
                {'#': INK, 's': SLATE, 't': TEAL})
    for x, y in ((8, 32), (168, 32), (8, 120), (168, 128)):
        planter(r, x, y, 16, 16)
        r.block(x // 8, y // 8, 2, 2)
    r.npc('guard', 'counter', 11, 6, 's', area=(9, 6, 5, 1))
    r.npc('office', 'patrol', 6, 5, 'e', path=[(6, 5), (20, 5)])
    r.npc('office', 'wander', 16, 11, 's', area=(14, 10, 8, 3))
    r.npc('visitor', 'sit', 5, 13, 's')
    r.npc('office', 'wander', 7, 12, 'n', area=(2, 10, 6, 3))
    r.npc('visitor', 'sit', 19, 13, 's')
    return r


def pub():
    """Generic pub: a long bar with taps and stools before a back bar of
    bottles, green leather booths, a pool table, a TV, a dartboard."""
    r = K.Room('pub', 24, 18, ['wood', 'green', 'brass', 'glass', 'red', 'screen', 'walnut'], 'PUB')
    wall = Wall(BOTTLE_D, INK, WOOD_D, wainscot=(WOOD_D, 8, INK))
    shell(r, wall, 3, planks(INK, WOOD_D), mat=(RED_D, INK), door=(CREAM, INK))
    # Back bar: shelves of bottles and a mirror.
    r.rect(16, 8, 96, 24, INK)
    r.rect(18, 10, 92, 20, SLATE)
    for sy in (11, 19):
        r.hline(18, sy + 7, 92, INK)
        for k, bx in enumerate(range(20, 108, 4)):
            r.stamp(bx, sy, ['.#.', '.#.', '#s#', '#c#', '#s#', '###', '###'][:7], {'#': INK, 's': SKY, 'c': CREAM if k % 3 else SKY})
    r.rect(56, 10, 16, 8, SKY)
    r.frame(56, 10, 16, 8, INK)
    for i in range(3):
        r.px(58 + i, 15 - i, CREAM)
    # TV and dartboard.
    tv(r, 128, 9, 24, 16, 'rink')
    r.point('screen', 17, 4, ['THE GAME', 'TIED IN THE THIRD', 'NOBODY BLINKS'], dx=0)
    r.rect(160, 8, 16, 16, INK)
    r.stamp(164, 12, ['..####..', '.#rccr#.', '#rc##cr#', '#c#rr#c#', '#c#rr#c#', '#rc##cr#', '.#rccr#.', '..####..'],
            {'#': INK, 'r': RED, 'c': CREAM})
    r.point('exhibit', 20, 4, ['DARTBOARD', 'HOUSE RECORD:', 'ONE BULLSEYE'], dx=0)
    # The bar with brass taps.
    r.rect(16, 40, 96, 16, WAL)
    r.frame(16, 40, 96, 9, INK)
    r.hline(17, 41, 94, WAL_L)
    r.rect(16, 48, 96, 8, WAL_D)
    r.frame(16, 48, 96, 8, INK)
    for xx in range(20, 108, 16):
        r.rect(xx, 50, 8, 4, WAL)
    for xx in (40, 48, 56, 64):
        r.stamp(xx, 39, ['.#.', '#g#', '.#.', '.#.'], {'#': INK, 'g': WAL_L})
    r.block(2, 5, 12, 2)
    for xx in (24, 40, 56, 72, 88, 104):
        stool(r, xx, 60, TERRA, WOOD_D)
    r.point('counter', 7, 7, ['THE BAR', 'TAPS: SIX', 'OPINIONS: MORE'], dx=4)
    # Booths of green leather along the east wall.
    for y in (40, 72, 104):
        booth(r, 160, y, BOTTLE, BOTTLE_D, table=TERRA, table_d=WOOD_D)
    # Pool table.
    r.rect(32, 96, 56, 32, BOTTLE_D)
    r.frame(32, 96, 56, 32, INK)
    r.hline(33, 97, 54, CREAM)
    r.rect(36, 100, 48, 24, BOTTLE)
    r.frame(36, 100, 48, 24, INK)
    for px_, py_ in ((36, 100), (58, 100), (82, 100), (36, 122), (58, 122), (82, 122)):
        r.rect(px_, py_, 2, 2, INK)
    r.stamp(50, 108, ['.#.', '###', '.#.'], {'#': CREAM})
    r.px(68, 111, CREAM)
    r.block(4, 12, 7, 4)
    r.point('exhibit', 7, 16, ['POOL TABLE', 'BALLS: MOST', 'CHALK: NONE'], dx=0)
    r.npc('clerk', 'counter', 7, 4, 's', area=(2, 4, 12, 1))
    r.npc('patron', 'sit', 3, 7, 'n', dx=-1)
    r.npc('patron', 'sit', 11, 7, 'n', dx=-1)
    r.npc('patron', 'sit', 21, 5, 's')
    r.npc('patron', 'sit', 21, 15, 'n')
    r.npc('patron', 'gaze', 16, 6, 'n')
    r.npc('patron', 'wander', 13, 12, 'w', area=(12, 9, 6, 6))
    return r


def side_arch(r, side, ty, n=3, floor=CREAM, line=STONE):
    """An archway through a side wall to the next room: the floor runs out
    into the dark under the arch between two piers (whole tiles; the wall
    stays solid, the arch point stands in front of it)."""
    x = 0 if side == 'w' else r.w - 8
    y0, y1 = ty * 8, (ty + n) * 8
    for y in range(y0, y1):
        for i in range(8):
            d = i if side == 'w' else 7 - i
            r.px(x + i, y, floor if d >= 4 else (line if d == 3 else (INK if d < 2 or (y // 2) % 2 else line)))
    for py in (y0 - 8, y1):
        r.rect(x, py, 8, 8, INK)
        r.rect(x + 1, py + 1, 6, 6, line)
        r.rect(x + 2, py + 2, 4, 4, floor)
    ax = x + 4 if side == 'w' else x + 1
    r.stamp(ax, y0 + (y1 - y0) // 2 - 2, ['..#', '.#.', '#..', '.#.', '..#'] if side == 'w' else
            ['#..', '.#.', '..#', '.#.', '#..'], {'#': line})


def gallery_plinth(r, cx, y):
    """A 16 x 8 marble plinth with a brass plaque; the exhibited sprite
    stands on its top (feet at y + 3)."""
    x = cx - 8
    r.rect(x, y, 16, 4, CREAM)
    r.frame(x, y, 16, 4, INK)
    r.hline(x + 1, y + 1, 14, CREAM)
    r.rect(x, y + 3, 16, 5, STONE)
    r.frame(x, y + 3, 16, 5, INK)
    r.rect(x + 5, y + 4, 6, 3, CREAM)
    r.hline(x + 6, y + 5, 4, STONE_D)


def gallery_hall():
    """The pause-menu museum hall: a title banner on the north wall, eight
    plinths for live exhibits, archways to the previous and next rooms, a
    red runner to the south exit."""
    r = K.Room('gallery_hall', 32, 22, ['stone', 'plaster', 'brass', 'red', 'leaf'], 'GALLERY', 'gallery')
    wall = Wall(PLASTER, PLASTER_D, PLASTER_D)
    shell(r, wall, 3, slabs(CREAM, STONE, size=16), mat=(RED, RED_D), door=(CREAM, PLASTER_D))
    for x in range(16, 248, 32):
        if 72 <= x <= 176:
            continue
        r.rect(x, 9, 8, 21, CREAM)
        r.vline(x, 9, 21, PLASTER_D)
        r.vline(x + 7, 9, 21, PLASTER_D)
        r.stamp(x + 2, 12, ['.##.', '#gg#', '.##.', '..#.'][:3], {'#': PLASTER_D, 'g': PLASTER})
    # Title banner: brass frame, one plain row for the engine's title text.
    r.rect(80, 8, 96, 24, GOLD)
    r.frame(80, 8, 96, 24, INK)
    r.frame(81, 9, 94, 22, GOLD_D)
    r.hline(82, 10, 92, CREAM)
    for x in (84, 168):
        r.stamp(x, 11, ['.##.', '#gg#', '#gg#', '.##.'], {'#': GOLD_D, 'g': CREAM})
    r.rect(88, 15, 80, 10, INK)
    r.rect(88, 16, 80, 8, CREAM)
    r.stamp(124, 26, ['########', '.######.'], {'#': GOLD_D})
    # Runner from the door to the banner.
    r.rect(120, 32, 16, 128, RED)
    r.vline(120, 32, 128, RED_D)
    r.vline(135, 32, 128, RED_D)
    for y in range(36, 160, 8):
        r.hline(123, y, 10, RED_D)
    door_mat(r, (RED_D, INK))
    # Archways to the previous and next rooms.
    side_arch(r, 'w', 9)
    side_arch(r, 'e', 9)
    r.point('arch_prev', 1, 10, ['PREVIOUS ROOM'])
    r.point('arch_next', 30, 10, ['NEXT ROOM'])
    # Eight plinths in two rows of four.
    for row_y in (72, 120):
        for cx in (40, 88, 168, 216):
            gallery_plinth(r, cx, row_y)
            r.block(cx // 8 - 1, row_y // 8, 2, 1)
            p = r.point('exhibit', cx // 8, row_y // 8 + 1, dx=-4)
            p['plinth'] = [cx, row_y + 3]
    for x, y in ((8, 32), (232, 32), (8, 144), (232, 144)):
        planter(r, x, y, 16, 16)
        r.block(x // 8, y // 8, 2, 2)
    r.banner = [88, 16, 167, 23]
    return r


def city_palette_colours():
    """The city's seven background palettes, read from their resources."""
    names = ['toronto_stone_and_water'] + [f'toronto_architecture_{i}' for i in range(1, 7)]
    return [json.loads((PALETTE_DIR / f'{n}.gbsres').read_text())['colors'] for n in names]


def sprite_palette_colours():
    ids = sprite_palettes()[0]
    by_id = {}
    for f in PALETTE_DIR.glob('td_sprite_*.gbsres'):
        d = json.loads(f.read_text())
        by_id[d['id']] = d['colors']
    return [by_id[i] for i in ids]


def gallery_art():
    """The pause-menu art direction room: the city's background palettes as
    swatches, a street kit at 1x, a miniature tower and skyline, the sprite
    palettes, models on plinths."""
    r = K.Room('gallery_art', 32, 22, ['teal', 'city_b', 'leaf', 'sand', 'gardens', 'sprite_a', 'sprite_b'],
               'ART DIRECTION', 'gallery')
    wall = Wall(CREAM, CREAM, SLATE)
    shell(r, wall, 3, planks(SAND_D, SAND), mat=(SLATE, INK), door=(CREAM, SLATE))
    # (a) The seven city palettes, one column each, four swatches tall.
    city = city_palette_colours()
    have = {c for p in r.pals for c in p}
    missing = {c for p in city for c in p} - have
    assert not missing, f'gallery_art palettes lack city colours {sorted(missing)}'
    r.rect(8, 8, 72, 24, CREAM)
    for k, colours in enumerate(city):
        x = 16 + k * 8
        for j, c in enumerate(colours):
            r.rect(x, 8 + j * 4, 8, 4, c)
    r.rect(8, 8, 8, 16, INK)
    r.rect(72, 8, 8, 16, INK)
    r.hline(8, 24, 72, INK)
    r.text(12, 26, 'BACKGROUNDS', SLATE)
    r.point('plaque', 5, 4, ['CITY PALETTES', 'SEVEN SETS OF', 'FOUR COLOURS'], dx=4)
    # (b) Street kit at 1x: roofs, sidewalk, kerb, road, lane marks, crosswalk.
    x0 = 88
    for x, w, roof, ridge in ((x0, 16, TERRA, ROSE), (x0 + 16, 8, SKY, CREAM), (x0 + 24, 16, ROSE, TERRA)):
        r.rect(x, 8, w, 8, roof)
        r.hline(x, 11, w, ridge)
        r.vline(x + w - 1, 8, 8, ridge)
    r.rect(x0, 16, 40, 16, SLATE)
    r.rect(x0, 16, 40, 2, CREAM)
    r.hline(x0, 18, 40, INK)
    for x in range(x0 + 2, x0 + 24, 6):
        r.hline(x, 22, 3, CREAM)
    for y in range(19, 28, 2):
        r.hline(x0 + 26, y, 10, CREAM)
    r.rect(x0, 28, 40, 3, CREAM)
    r.hline(x0, 31, 40, INK)
    r.point('plaque', 13, 4, ['STREET KIT', 'ROADS, KERBS,', 'ROOFS AT 1X'], dx=0)
    # (c) A miniature tower over the skyline and the lake.
    x0 = 136
    r.rect(x0, 8, 40, 24, INK)
    r.rect(x0 + 1, 9, 38, 22, TEAL)
    for i, hh in enumerate((6, 9, 5, 11, 7, 4, 8, 6, 10, 5)):
        r.rect(x0 + 2 + i * 4, 26 - hh, 3, hh, SLATE)
        r.px(x0 + 3 + i * 4, 27 - hh + 2, CREAM)
    r.stamp(x0 + 22, 10, TOWER_ICON, {'#': INK})
    r.rect(x0 + 1, 26, 38, 5, SLATE)
    for x in range(x0 + 3, x0 + 38, 5):
        r.px(x, 28, CREAM)
    r.point('plaque', 19, 4, ['THE SKYLINE', 'ONE TOWER,', 'MANY ROOFS'], dx=0)
    # (d) The eight sprite palettes: body colour over light colour.
    sprites = sprite_palette_colours()
    x0 = 184
    for k, colours in enumerate(sprites):
        body = colours[1]
        assert body in have, f'gallery_art lacks sprite colour {body}'
        r.rect(x0 + k * 8, 8, 8, 16, body)
    r.hline(x0, 24, 64, INK)
    r.text(x0 + 2, 26, 'SPRITES', SLATE)
    r.point('plaque', 26, 4, ['SPRITE PALETTES', 'EIGHT COLOURS', 'FOR EVERYONE'], dx=0)
    # Models on plinths.
    for cx, model, text in ((64, 'tree', ['PARK TREE', 'ONE TILE OF', 'SHADE']),
                            (128, 'tower', ['THE TOWER', 'SIXTEEN PIXELS', 'OF CONCRETE']),
                            (192, 'car', ['TEAL SEDAN', 'TRAFFIC LOOP 4', 'NEVER LATE'])):
        x = cx - 16
        r.rect(x, 80, 32, 24, CREAM)
        r.frame(x, 80, 32, 24, INK)
        r.rect(x + 1, 96, 30, 7, SAND_D)
        r.hline(x + 1, 96, 30, INK)
        r.hline(x + 1, 97, 30, SAND)
        r.rect(cx - 3, 99, 6, 3, CREAM)
        r.block(x // 8, 10, 4, 3)
        r.point('exhibit', cx // 8, 13, text, dx=-4)
    r.stamp(52, 82, ['....######......', '..##gggglg##....', '.#gglggggggg#...', '#ggggglgggggg#..', '#gglggggglggg#..',
                     '#ggggggggggg#...', '.##ggglggg##....', '...#########....', '......##........'],
            {'#': INK, 'g': GRASS, 'l': CREAM})
    r.stamp(125, 81, TOWER_ICON[2:], {'#': SLATE})
    r.stamp(181, 84, ['..##########......', '.#tttttttttt###...', '#tt#ss#tttt#ss#t#.', '#tttttttttttttttt#',
                      '#tt##tttttttt##tt#', '.##..########..##.'], {'#': INK, 't': TEAL, 's': CREAM})
    side_arch(r, 'w', 9, floor=SAND, line=SAND_D)
    side_arch(r, 'e', 9, floor=SAND, line=SAND_D)
    r.point('arch_prev', 1, 10, ['PREVIOUS ROOM'])
    r.point('arch_next', 30, 10, ['NEXT ROOM'])
    for x, y in ((8, 144), (232, 144)):
        planter(r, x, y, 16, 16)
        r.block(x // 8, y // 8, 2, 2)
    r.npc('docent', 'wander', 12, 16, 's', area=(4, 15, 24, 3))
    r.npc('viewer', 'gaze', 10, 5, 'n')
    return r


ROOMS = [cn_base, cn_lookout, ago, cafe, rom, eaton, union, dragon_city, chinatown_bakery, electronics, corner_store, diner, shop, office_lobby, pub, gallery_hall, gallery_art]


# ===================================================================== output
def ident(kind, name):
    return str(uuid.uuid5(NAMESPACE, kind + '/' + name))


def palette_id(name):
    return ident('palette', name)


def sprite_palettes():
    path = SCENES / 'toronto_core_nw/scene.gbsres'
    data = json.loads(path.read_text())
    return data['spritePaletteIds'], data['playerSpriteSheetId']


def build():
    rooms = [f() for f in ROOMS]
    sprite_ids, player = sprite_palettes()
    texts, images, previews, meta, report = {}, {}, {}, [], []
    used = []
    for n, r in enumerate(rooms):
        slug = 'interior_' + r.key
        src, colour, attrs = r.images()
        tiles = city_kit.flip_canonical_count(src)
        assert tiles <= TILE_BUDGET, (slug, 'tiles', tiles)
        collisions = r.collisions()
        validate(r)
        for p in r.palette_names:
            if p not in used:
                used.append(p)
        ids = [palette_id(p) for p in r.palette_names]
        while len(ids) < 7:
            ids.append(ids[len(ids) % len(r.palette_names)])
        name = r.title
        background = {
            '_resourceType': 'background', 'id': ident('background', slug), 'name': 'Interior ' + name.title(),
            'symbol': f'bg_{slug}', 'tileColors': compress(attrs), 'filename': f'{slug}.png',
            'width': r.tw, 'height': r.th, 'imageWidth': r.w, 'imageHeight': r.h, 'autoColor': False,
        }
        scene = {
            '_resourceType': 'scene', 'id': ident('scene', slug), '_index': 17 + n, 'type': 'TORONTO',
            'name': 'Interior ' + name.title(), 'symbol': f'scene_{slug}',
            'x': 300 + (n % 6) * 300, 'y': 1000 + (n // 6) * 300,
            'width': r.tw, 'height': r.th, 'backgroundId': ident('background', slug), 'tilesetId': '',
            'colorModeOverride': 'none', 'paletteIds': ids, 'spritePaletteIds': sprite_ids,
            'autoFadeSpeed': 1, 'script': [], 'playerHit1Script': [], 'playerHit2Script': [], 'playerHit3Script': [],
            'collisions': compress(collisions), 'playerSpriteSheetId': player,
        }
        texts[BACKGROUNDS / f'{slug}.png.gbsres'] = json.dumps(background, indent=2) + '\n'
        texts[SCENES / slug / 'scene.gbsres'] = json.dumps(scene, indent=2) + '\n'
        texts[PROJECT / f'original-art/{slug}_attributes.json'] = json.dumps(attrs) + '\n'
        images[BACKGROUNDS / f'{slug}.png'] = src
        previews[slug] = colour
        entry = {'key': r.key, 'slug': slug, 'name': name, 'kind': r.kind, 'width': r.tw, 'height': r.th,
                 'entrance': r.entrance, 'exit': r.exit, 'points': r.points, 'npc_spots': r.npcs,
                 'palettes': [palette_id(p) for p in r.palette_names]}
        if getattr(r, 'banner', None):
            entry['banner'] = r.banner
        meta.append(entry)
        report.append((slug, tiles, r.palette_names))
    for p in used:
        title, colours = K.PALETTES[p]
        texts[PALETTE_DIR / f'interior_{p}.gbsres'] = json.dumps(
            {'_resourceType': 'palette', 'id': palette_id(p), 'name': title, 'colors': colours,
             'defaultName': title, 'defaultColors': colours}, indent=2) + '\n'
    doc = {'note': 'Generated by scripts/create_interiors.py. Pixel coordinates within each scene, origin top-left; '
                   'x is the centre column and y the bottom row (feet) of an 8x16 sprite. Exit rectangles are '
                   'inclusive [x0, y0, x1, y1]. Landmark rooms are original interpretations; shop names are fictional.',
           'interiors': meta}
    texts[META] = json.dumps(doc, indent=1) + '\n'
    return texts, images, previews, report


def compress(values):
    output, last, count = [], None, 0
    for value in list(values) + [None]:
        if value != last:
            if count:
                output.append(f'{last:02x}' + ('!' if count == 1 else f'{count:x}+'))
            last, count = value, 0
        count += 1
    return ''.join(output)


def validate(r):
    key = r.key
    assert r.tw >= 20 and r.th >= 18, (key, 'smaller than one screen')
    assert len(r.title) <= 16 and set(r.title) <= CHARSET, (key, r.title)
    for x in range(r.tw):
        assert r.solid[0][x] and r.solid[r.th - 1][x], (key, 'border', x)
    for y in range(r.th):
        assert r.solid[y][0] and r.solid[y][r.tw - 1], (key, 'border', y)
    seen = r.reachable()
    x0, y0, x1, y1 = r.exit
    assert all(not r.solid[y // 8][x // 8] for x in (x0, x1) for y in (y0, y1)), (key, 'exit on solid tiles')
    assert (x0 // 8, y0 // 8) in seen, (key, 'exit unreachable')
    ex, ey = r.entrance
    assert not (x0 <= ex <= x1 and y0 <= ey <= y1), (key, 'entrance inside the exit')
    for p in r.points:
        assert p['kind'] in KINDS, (key, p)
        assert (p['x'] // 8, p['y'] // 8) in seen, (key, 'point unreachable', p)
        for line in p.get('text', []):
            assert len(line) <= 18 and set(line) <= CHARSET, (key, line)
    assert len(r.npcs) <= 8, (key, 'too many NPC spots')
    for n in r.npcs:
        assert n['role'] in ROLES and n['behave'] in BEHAVES, (key, n)
        assert (n['x'] // 8, n['y'] // 8) in seen, (key, 'NPC spot unreachable', n)
        if 'area' in n:
            ax0, ay0, ax1, ay1 = n['area']
            assert ax0 <= n['x'] <= ax1 and ay0 <= n['y'] <= ay1, (key, 'NPC outside its area', n)
        for px, py in n.get('path', []):
            assert (px // 8, py // 8) in seen, (key, 'patrol point unreachable', n)


def leftovers(texts, images):
    """Interior files on disk that the generator no longer writes (a room
    or palette that was removed or renamed)."""
    keep = set(texts) | set(images)
    found = (list(PALETTE_DIR.glob('interior_*.gbsres')) + list(BACKGROUNDS.glob('interior_*')) +
             list(SCENES.glob('interior_*/scene.gbsres')) + list((PROJECT / 'original-art').glob('interior_*')))
    return sorted(path for path in found if path not in keep)


def main(argv):
    texts, images, previews, report = build()
    if '--preview' in argv:
        out = Path(argv[argv.index('--preview') + 1])
        out.mkdir(parents=True, exist_ok=True)
        for slug, im in previews.items():
            im.resize((im.width * 3, im.height * 3), Image.NEAREST).save(out / f'{slug}.png')
    check = '--check' in argv
    if check:
        stale = []
        for path, text in texts.items():
            if not path.exists() or path.read_text() != text:
                stale.append(str(path.relative_to(ROOT)))
        for path, im in images.items():
            if not path.exists():
                stale.append(str(path.relative_to(ROOT)))
                continue
            with Image.open(path) as current:
                if current.convert('RGB').tobytes() != im.tobytes():
                    stale.append(str(path.relative_to(ROOT)))
        stale += [str(path.relative_to(ROOT)) for path in leftovers(texts, images)]
        assert not stale, f'stale interior files: {stale}'
    else:
        for path, im in images.items():
            path.parent.mkdir(parents=True, exist_ok=True)
            im.save(path)
        for path, text in texts.items():
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
        for path in leftovers(texts, images):
            path.unlink()
            if path.name == 'scene.gbsres' and not any(path.parent.iterdir()):
                path.parent.rmdir()
    tiles = ', '.join(f'{s.removeprefix("interior_")} {t}' for s, t, _ in report)
    verb = 'match their generator' if check else 'written'
    print(f'{len(report)} interiors {verb}; tiles per scene: {tiles}.')


if __name__ == '__main__':
    main(sys.argv[1:])
