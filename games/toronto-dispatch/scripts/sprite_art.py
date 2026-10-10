"""Original top-down sprite designs for Toronto Dispatch.

Every design is a grid of colour indices: 0 transparent, 1 light, 2 mid,
3 dark. Cardinal views are hand-drawn pixel art (east-facing; south views are
their transpose). Diagonals are rasterized from the matching vehicle shape with
4x4 supersampling and a one-pixel outline, then mirrored for the remaining
headings. No external imagery is used.
"""
import math

W = H = 16


def grid(rows):
    """ASCII rows ('.123') to a list of integer rows."""
    return [[0 if ch == '.' else int(ch) for ch in row] for row in rows]


def transpose(g):
    return [list(col) for col in zip(*g)]


def flip_h(g):
    return [row[::-1] for row in g]


def flip_v(g):
    return [row[:] for row in g[::-1]]


def raster(zone, heading_deg, size=(W, H), outline=True, ss=4):
    w, h = size
    cx, cy = w / 2, h / 2
    a = math.radians(heading_deg)
    ca, sa = math.cos(a), math.sin(a)
    out = [[0] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            votes = {}
            for sy in range(ss):
                for sx in range(ss):
                    px = x + (sx + 0.5) / ss - cx
                    py = y + (sy + 0.5) / ss - cy
                    f = px * ca + py * sa
                    s = -px * sa + py * ca
                    c = zone(f, s)
                    votes[c] = votes.get(c, 0) + 1
            colour, count = max(votes.items(), key=lambda kv: (kv[1], kv[0] != 0))
            out[y][x] = colour if colour == 0 or count >= ss * ss * 0.45 else 0
    if not outline:
        return out
    edged = [row[:] for row in out]
    for y in range(h):
        for x in range(w):
            if out[y][x] in (1, 2):
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if not (0 <= nx < w and 0 <= ny < h) or out[ny][nx] == 0:
                        edged[y][x] = 3
                        break
    return edged


def eight_headings(east, south, south_east):
    """Frames in engine heading order: E, SE, S, SW, W, NW, N, NE."""
    return [east, south_east, south, flip_h(south_east), flip_h(east),
            flip_v(flip_h(south_east)), flip_v(south), flip_v(south_east)]


# ---------------------------------------------------------------- cars
# Road vehicles are drawn east-facing on a 16x16 grid from simple layers:
# body, a dark cabin ring (rear window, side glass, windscreen), a light
# upper flank (light from the top left), headlamps and tyres. South is the
# transpose; the 45-degree view rotates the tyre-less drawing with 5x5
# supersampling and redraws the outline, so all eight headings share one
# design. Colours: 1 light, 2 body, 3 dark.

def _layers(spec, tyres=True, w=W, h=H):
    g = [[0] * w for _ in range(h)]

    def rect(x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                g[y][x] = c
    for op in spec:
        if op[0] == 'tyre' and not tyres:
            continue
        rect(*op[1:5], op[5])
    return g


def _sedan_spec(body=2, sign=False, lightbar=False, shine=True):
    """Sedan filling the 16-pixel frame: 16 long, 10 wide plus tyres."""
    spec = [('body', 0, 3, 15, 12, body),
            ('cut', 0, 3, 0, 3, 0), ('cut', 15, 3, 15, 3, 0), ('cut', 0, 12, 0, 12, 0), ('cut', 15, 12, 15, 12, 0),
            ('cabin', 4, 5, 11, 10, 3), ('roof', 5, 6, 9, 9, body)]
    if shine:
        spec += [('flank', 1, 4, 14, 4, 1), ('roofshine', 5, 6, 6, 6, 1), ('trunk', 1, 7, 1, 8, 1)]
    if sign:
        spec += [('sign', 6, 7, 8, 8, 1)]
    if lightbar:
        spec += [('stripe', 1, 4, 14, 4, 2), ('stripe', 1, 11, 14, 11, 2), ('bar', 6, 6, 7, 9, 2)]
    spec += [('glint', 11, 6, 11, 6, 1),
             ('tyre', 2, 2, 4, 2, 3), ('tyre', 11, 2, 13, 2, 3), ('tyre', 2, 13, 4, 13, 3), ('tyre', 11, 13, 13, 13, 3)]
    return spec


def _compact_spec():
    """Hatchback: shorter (13 long), tall glasshouse, stubby hood."""
    return [('body', 1, 3, 13, 12, 2),
            ('cut', 1, 3, 1, 3, 0), ('cut', 13, 3, 13, 3, 0), ('cut', 1, 12, 1, 12, 0), ('cut', 13, 12, 13, 12, 0),
            ('cabin', 3, 5, 10, 10, 3), ('roof', 4, 6, 8, 9, 2),
            ('flank', 2, 4, 12, 4, 1), ('roofshine', 4, 6, 5, 6, 1), ('glint', 10, 6, 10, 6, 1),
            ('tyre', 2, 2, 4, 2, 3), ('tyre', 10, 2, 12, 2, 3), ('tyre', 2, 13, 4, 13, 3), ('tyre', 10, 13, 12, 13, 3)]


def _pickup_spec():
    """Pickup truck: open bed with dark floor and ribs, then the cab."""
    return [('body', 0, 3, 15, 12, 2),
            ('cut', 0, 3, 0, 3, 0), ('cut', 15, 3, 15, 3, 0), ('cut', 0, 12, 0, 12, 0), ('cut', 15, 12, 15, 12, 0),
            ('bed', 1, 5, 7, 10, 3), ('floor', 2, 6, 7, 9, 2), ('load', 3, 7, 5, 8, 1),
            ('cab', 9, 5, 12, 10, 3), ('cabroof', 10, 6, 11, 9, 2), ('glint', 12, 6, 12, 6, 1),
            ('flank', 9, 4, 14, 4, 1), ('rail', 1, 4, 7, 4, 1),
            ('tyre', 2, 2, 4, 2, 3), ('tyre', 11, 2, 13, 2, 3), ('tyre', 2, 13, 4, 13, 3), ('tyre', 11, 13, 13, 13, 3)]


def _sports_spec():
    """Low coupe: long hood with twin stripes, small cabin, rear spoiler."""
    return [('body', 0, 3, 15, 12, 2),
            ('cut', 0, 3, 0, 3, 0), ('cut', 15, 3, 15, 3, 0), ('cut', 0, 12, 0, 12, 0), ('cut', 15, 12, 15, 12, 0),
            ('cabin', 4, 5, 9, 10, 3), ('roof', 5, 6, 7, 9, 2),
            ('stripe', 10, 6, 15, 6, 1), ('stripe', 10, 9, 15, 9, 1), ('stripe', 1, 6, 3, 6, 1), ('stripe', 1, 9, 3, 9, 1),
            ('spoiler', 0, 4, 0, 11, 3), ('glint', 9, 6, 9, 6, 1),
            ('tyre', 2, 2, 4, 2, 3), ('tyre', 11, 2, 13, 2, 3), ('tyre', 2, 13, 4, 13, 3), ('tyre', 11, 13, 13, 13, 3)]


def _truck_spec():
    """Box van: cargo box (light) with panels and ribs, then the cab."""
    return [('box', 0, 3, 10, 12, 1), ('panel', 1, 4, 9, 4, 2), ('panel', 1, 11, 9, 11, 2),
            ('rib', 3, 5, 3, 10, 2), ('rib', 6, 5, 6, 10, 2), ('rib', 9, 5, 9, 10, 2),
            ('cab', 11, 4, 15, 11, 2), ('divide', 11, 4, 11, 11, 3), ('screen', 13, 5, 13, 10, 3),
            ('glint', 13, 5, 13, 5, 1), ('flank', 12, 4, 14, 4, 1),
            ('tyre', 2, 2, 4, 2, 3), ('tyre', 2, 13, 4, 13, 3), ('tyre', 12, 3, 13, 3, 3), ('tyre', 12, 12, 13, 12, 3)]


def _outline(g, lamps=()):
    out = [row[:] for row in g]
    h, w = len(g), len(g[0])
    for y in range(h):
        for x in range(w):
            if g[y][x] in (1, 2):
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if not (0 <= nx < w and 0 <= ny < h) or g[ny][nx] == 0:
                        out[y][x] = 3
                        break
    for x, y in lamps:
        out[y][x] = 1
    return out


def _rotate45(g, ss=5):
    """Rotate an east-facing drawing 45 degrees clockwise (to south-east)."""
    a = math.radians(45)
    ca, sa = math.cos(a), math.sin(a)
    out = [[0] * W for _ in range(H)]
    for y in range(H):
        for x in range(W):
            votes = {}
            for sy in range(ss):
                for sx in range(ss):
                    px, py = x + (sx + 0.5) / ss - 8, y + (sy + 0.5) / ss - 8
                    fx, fy = px * ca + py * sa + 8, -px * sa + py * ca + 8
                    ix, iy = int(math.floor(fx)), int(math.floor(fy))
                    c = g[iy][ix] if 0 <= ix < W and 0 <= iy < H else 0
                    votes[c] = votes.get(c, 0) + 1
            if ss * ss - votes.get(0, 0) < ss * ss * 0.5:
                continue
            body = {c: v for c, v in votes.items() if c}
            out[y][x] = 3 if body.get(3, 0) >= ss * ss * 0.4 else max(body, key=lambda c: (body[c], c))
    return out


CAR_LAMPS = ((15, 4), (15, 11))
TRUCK_LAMPS = ((15, 5), (15, 10))
COMPACT_LAMPS = ((13, 4), (13, 11))

# Full-size road vehicles: 20 long and 14 wide (12 of body plus tyres). The
# east view sits in a 24x16 frame and the south view (its transpose) in a
# centred 16x32 frame, so west and north are tile flips of them. The 45
# degree views keep the 16x16 drawing, whose diagonal already spans about
# 20 pixels. Compact cars and coupes stay 16 long.
LONG = 24
LONG_CAR_LAMPS = ((21, 3), (21, 12))
LONG_TRUCK_LAMPS = ((21, 4), (21, 11))
_CORNERS = [('cut', 2, 2, 2, 2, 0), ('cut', 21, 2, 21, 2, 0), ('cut', 2, 13, 2, 13, 0), ('cut', 21, 13, 21, 13, 0)]
_LONG_TYRES = [('tyre', 4, 1, 7, 1, 3), ('tyre', 16, 1, 19, 1, 3), ('tyre', 4, 14, 7, 14, 3), ('tyre', 16, 14, 19, 14, 3)]


def _sedan_long(body=2, sign=False, lightbar=False, shine=True, flash=False):
    spec = [('body', 2, 2, 21, 13, body)] + _CORNERS + [('cabin', 7, 4, 16, 11, 3), ('roof', 8, 5, 14, 10, body)]
    if shine:
        spec += [('flank', 3, 3, 20, 3, 1), ('roofshine', 8, 5, 10, 5, 1), ('trunk', 3, 6, 3, 9, 1), ('hood', 18, 5, 18, 10, 1)]
    if sign:
        spec += [('sign', 10, 7, 12, 8, 1)]
    if lightbar:
        spec += [('stripe', 3, 3, 20, 3, 2), ('stripe', 3, 12, 20, 12, 2), ('bar', 10, 5, 11, 10, 2)]
    if flash:
        spec += [('flash', 10, 5, 11, 7, 1), ('blue', 10, 8, 11, 10, 2)]
    return spec + [('glint', 16, 5, 16, 5, 1)] + _LONG_TYRES


def _truck_long():
    return [('box', 2, 2, 15, 13, 1), ('panel', 3, 3, 14, 3, 2), ('panel', 3, 12, 14, 12, 2),
            ('rib', 5, 4, 5, 11, 2), ('rib', 9, 4, 9, 11, 2), ('rib', 13, 4, 13, 11, 2),
            ('cab', 16, 3, 21, 12, 2), ('divide', 16, 3, 16, 12, 3), ('screen', 19, 4, 19, 11, 3),
            ('glint', 19, 4, 19, 4, 1), ('flank', 17, 3, 20, 3, 1),
            ('tyre', 4, 1, 7, 1, 3), ('tyre', 4, 14, 7, 14, 3), ('tyre', 17, 2, 19, 2, 3), ('tyre', 17, 13, 19, 13, 3)]


def _pickup_long():
    return [('body', 2, 2, 21, 13, 2)] + _CORNERS + [
            ('bed', 3, 4, 11, 11, 3), ('floor', 4, 5, 11, 10, 2), ('load', 5, 6, 8, 9, 1),
            ('cab', 13, 4, 17, 11, 3), ('cabroof', 14, 5, 16, 10, 2), ('glint', 17, 5, 17, 5, 1),
            ('flank', 13, 3, 20, 3, 1), ('rail', 3, 3, 11, 3, 1)] + _LONG_TYRES


def _long_east(spec, lamps):
    return _outline(_layers(spec, w=LONG), lamps)


def _south32(east):
    """South view of a 24-long east view, centred in a 16x32 frame."""
    south = transpose(east)
    blank = [[0] * len(south[0]) for _ in range(4)]
    return blank + south + [row[:] for row in blank]


def _diagonal(spec, lamps):
    bare = _outline(_layers(spec, tyres=False), lamps)
    return _outline(_rotate45([[c if c != 3 or _inside(bare, x, y) else 0 for x, c in enumerate(row)]
                               for y, row in enumerate(bare)]))


def _vehicle(spec, lamps):
    east = _outline(_layers(spec), lamps)
    # The diagonal leaves out tyres (they would read as bumps) and gains its
    # outline after rotation.
    return eight_headings(east, transpose(east), _diagonal(spec, lamps))


def _long_vehicle(long_spec, long_lamps, spec, lamps):
    east = _long_east(long_spec, long_lamps)
    return eight_headings(east, _south32(east), _diagonal(spec, lamps))


def _inside(g, x, y):
    """Dark pixels enclosed by body on both sides of an axis are detail."""
    return ((0 < x < W - 1 and g[y][x - 1] and g[y][x + 1]) or (0 < y < H - 1 and g[y - 1][x] and g[y + 1][x]))


def car_frames(taxi=False):
    return _long_vehicle(_sedan_long(sign=taxi), LONG_CAR_LAMPS, _sedan_spec(sign=taxi), CAR_LAMPS)


def van_frames():
    return _long_vehicle(_truck_long(), LONG_TRUCK_LAMPS, _truck_spec(), TRUCK_LAMPS)


def _cardinal_vehicle(spec, lamps):
    """Traffic-only designs drive cardinally: diagonal slots repeat the
    nearest cardinal view, so a design costs only its east and south tiles."""
    east = _outline(_layers(spec), lamps)
    south = transpose(east)
    return [east, east, south, south, flip_h(east), flip_h(east), flip_v(south), flip_v(south)]


def _cardinal_long(spec, lamps):
    east = _long_east(spec, lamps)
    south = _south32(east)
    return [east, east, south, south, flip_h(east), flip_h(east), flip_v(south), flip_v(south)]


def compact_frames():
    return _cardinal_vehicle(_compact_spec(), COMPACT_LAMPS)


def pickup_frames():
    return _cardinal_long(_pickup_long(), LONG_CAR_LAMPS)


def sports_frames():
    return _cardinal_vehicle(_sports_spec(), CAR_LAMPS)


def door_frame():
    """The parked courier car, east-facing, with its driver door swung open."""
    g = [row[:] for row in car_frames()[0]]
    for x, y, c in ((10, 14, 3), (10, 15, 3), (11, 15, 2), (12, 15, 3)):
        g[y][x] = c
    return g


# ---------------------------------------------------------------- two-wheelers
MOTO_E = [
    "................",
    "................",
    "................",
    "................",
    "................",
    "......333..3....",
    ".....32223.3....",
    "..33322112333...",
    "..33322112333...",
    ".....32223.3....",
    "......333..3....",
    "................",
    "................",
    "................",
    "................",
    "................",
]
SCOOTER_E = [
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "......333.3.....",
    "....33211233....",
    "....33211233....",
    "......333.3.....",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
]


def moto_zone(length=11.0, rider=2.3):
    hl = length / 2

    def zone(f, s):
        a = abs(s)
        if abs(f) > hl or a > 3.0:
            return 0
        if a <= 1.0 and (f > hl - 2.3 or f < -hl + 2.3):
            return 3
        if abs(f - (hl - 3.0)) < 0.6 and a < 2.9:
            return 3
        if (f + 0.4) ** 2 + s * s < rider * rider:
            return 1 if f * f + s * s < 1.3 else 2
        if a <= 1.4:
            return 2
        return 0
    return zone


def moto_frames():
    east = grid(MOTO_E)
    return eight_headings(east, transpose(east), raster(moto_zone(), 45))


def scooter_frames():
    """The scooter shares the motorcycle's drawing (and tiles); it differs in
    handling, not looks, so full-size cars fit the sprite tile budget."""
    return moto_frames()


# ---------------------------------------------------------------- people
# People are about 10 px tall (feet on row 13), so vehicles read about
# one and a half times their length. Each figure is drawn inside columns
# 4..11 (one 8x16 OBJ) from 8-wide rows: 1 skin, 2 clothing, 3 hair,
# shoes and outline. Every person uses the courier's palette; an actor's
# palette offset chooses the clothing colour at run time, so one design
# serves the courier, civilians in five colours and the police.
_HEAD = {  # design -> view -> rows ending on row 6 (the chin)
    'short': {'down': ["..3333..", "..3113..", "..3113.."],
              'up': ["..3333..", "..3333..", "..3333.."],
              'right': ["..333...", "..3311..", "..3311.."]},
    'long': {'down': [".333333.", ".331133.", ".331133."],
             'up': [".333333.", ".333333.", ".333333."],
             'right': [".3333...", ".33311..", ".33311.."]},
    'cap': {'down': ["..2222..", ".322223.", "..3113..", "..3113.."],
            'up': ["..2222..", "..2222..", "..3333..", "..3333.."],
            'right': ["..222...", "..22222.", "..3311..", "..3311.."]},
    'bun': {'down': ["...33...", "..3333..", "..3113..", "..3113.."],
            'up': ["...33...", "..3333..", "..3333..", "..3333.."],
            'right': [".33.....", "..333...", "..3311..", "..3311.."]},
    'pack': None,  # short hair; the backpack changes the torso
    'umbrella': {'down': ["..3333..", ".322123.", "32222223", "32222223", ".3.33.3.", "..3113.."],
                 'up': ["..3333..", ".321223.", "32222223", "32222223", ".3.33.3.", "..3333.."],
                 'right': ["..3333..", ".322123.", "32222223", "32222223", ".3.33.3.", "..3311.."]},
}
_TORSO = {
    'down': [".322223.", ".122221.", ".322223.", "..3223.."],
    'up': [".322223.", ".122221.", ".322223.", "..3223.."],
    'right': ["..3223..", "..3213..", "..3223..", "..333..."],
}
_TORSO_PACK = {
    'down': [".332233.", ".132231.", ".322223.", "..3223.."],
    'up': [".333333.", ".133331.", ".333333.", "..3223.."],
    'right': [".33223..", ".33213..", ".33223..", "..333..."],
}
_TORSO_LONG = {  # hair falls over the shoulders
    'down': [".332233.", ".122221.", ".322223.", "..3223.."],
    'up': [".333333.", ".133331.", ".322223.", "..3223.."],
    'right': [".33223..", "..3213..", "..3223..", "..333..."],
}
_LEGS = {
    'down': (["..3..3..", "..3..3..", "..3..3.."], ["..3..3..", "..3..3..", ".....3.."]),
    'up': (["..3..3..", "..3..3..", "..3..3.."], ["..3..3..", "..3..3..", "..3....."]),
    'right': (["..3.3...", ".3...3..", ".3...3.."], ["..33....", "..33....", "..3....."]),
}
PEOPLE_DESIGNS = ('short', 'long', 'cap', 'bun', 'pack', 'umbrella')


def _figure(head, torso, legs):
    rows = head + torso + legs
    top = 14 - len(rows)
    blank = "." * 16
    out = [blank] * top + ["...." + r + "...." for r in rows] + [blank] * 2
    assert len(out) == 16, len(out)
    return grid(out)


def person_frames(design='short'):
    """[right0, right1, left0, left1, down0, down1, up0, up1] for a design."""
    heads = _HEAD[design] or _HEAD['short']
    torsos = _TORSO_PACK if design == 'pack' else _TORSO_LONG if design == 'long' else _TORSO
    view = {d: [_figure(heads[d], torsos[d], _LEGS[d][i]) for i in (0, 1)] for d in ('right', 'down', 'up')}
    r = view['right']
    return [r[0], r[1], flip_h(r[0]), flip_h(r[1]), view['down'][0], view['down'][1], view['up'][0], view['up'][1]]


# ---------------------------------------------------------------- markers / props
BEACON = [
    "................",
    ".....333333.....",
    "....32222223....",
    "...3221111223...",
    "...3213333123...",
    "...3213113123...",
    "...3213333123...",
    "...3221111223...",
    "....32222223....",
    ".....322223.....",
    "......3223......",
    ".......33.......",
    "................",
    "................",
    "................",
    "................",
]
BEACON_PULSE = [
    "................",
    "................",
    ".....333333.....",
    "....32222223....",
    "...3221111223...",
    "...3213333123...",
    "...3213113123...",
    "...3213333123...",
    "...3221111223...",
    "....32222223....",
    ".....322223.....",
    "......3223......",
    ".......33.......",
    "................",
    "................",
    "................",
]
# Sidewalk pickups, centred in columns 4..11 so each needs one 8x16 OBJ.
# The second frame of each adds a highlight that blinks.
PICKUP_CASH = [  # gold coin stamped with a dollar sign (beacon yellow)
    "................", "................", "................", "................",
    "......3333......", ".....322223.....", "....32211123....", "....32122223....",
    "....32211223....", "....32222123....", "....32111223....", ".....322223.....",
    "......3333......", "................", "................", "................"]
PICKUP_CASH_GLINT = [
    "................", "................", "................", "................",
    "......3333......", ".....312223.....", "....31211123....", "....32122223....",
    "....32211223....", "....32222123....", "....32111223....", ".....322223.....",
    "......3333......", "................", "................", "................"]
PICKUP_FIRST_AID = [  # white case with a red cross and a handle (traffic red)
    "................", "................", "................", "......3333......",
    ".....33..33.....", "....33333333....", "....31111113....", "....31122113....",
    "....31222213....", "....31222213....", "....31122113....", "....31111113....",
    "....33333333....", "................", "................", "................"]
PICKUP_FIRST_AID_GLINT = [
    "................", "................", "................", "......3333......",
    ".....33..33.....", "....33333333....", "....3.111113....", "....31122113....",
    "....31222213....", "....31222213....", "....31122113....", "....31111113....",
    "....33333333....", "................", "................", "................"]
PICKUP_AMMO = [  # navy ammunition box with brass rounds (police palette)
    "................", "................", "................", ".....3.3.3......",
    "....313131 3....", "....31313133....", "....33333333....", "....32222223....",
    "....32111123....", "....32222223....", "....32222223....", "....32222223....",
    "....33333333....", "................", "................", "................"]
PICKUP_AMMO = [row.replace(' ', '.') for row in PICKUP_AMMO]
PICKUP_AMMO_GLINT = [
    "................", "................", "................", ".....3.3.3......",
    "....3.3131.3....", "....31313133....", "....33333333....", "....32222223....",
    "....32111123....", "....32222223....", "....32222223....", "....32222223....",
    "....33333333....", "................", "................", "................"]


# ---------------------------------------------------------------- transit
def _ttc_body(f, s, hl, pane=5.0):
    """Shared TTC livery: red skirt along both sides, white roof."""
    return 2 if abs(s) > 2.6 else 1


def bus_zone(length=38.0, width=9.0):
    """TTC-style low-floor bus seen from above."""
    hl, hw = length / 2, width / 2

    def zone(f, s):
        a = abs(s)
        if abs(f) > hl or a > hw:
            return 0
        if f > hl - 1.0:
            return 1 if 2.0 < a < 3.4 else 3  # bumper with headlamps
        if f > hl - 3.6:
            return 3 if a < 3.4 else 2  # windscreen
        if f < -hl + 1.5:
            return 3  # engine deck
        body = _ttc_body(f, s, hl)
        if body == 1 and -hl * 0.62 < f < -hl * 0.22 and a < 1.5:
            return 3  # rooftop air conditioning
        if body == 1 and abs(f - hl * 0.25) < 1.0:
            return 2  # roof hatch
        return body
    return zone


def streetcar_zone(length=48.0, width=9.0, sections=4):
    """Four-section low-floor streetcar seen from above, cabs at both ends."""
    hl, hw = length / 2, width / 2
    seg = length / sections

    def zone(f, s):
        a = abs(s)
        if abs(f) > hl or a > hw:
            return 0
        if abs(f) > hl - 1.0:
            return 1 if 2.0 < a < 3.4 else 3
        if abs(f) > hl - 3.6:
            return 3 if a < 3.4 else 2
        pos = f + hl
        k = round(pos / seg)
        if 0 < k < sections and abs(pos - k * seg) < 0.6:
            return 3  # articulation joints
        body = _ttc_body(f, s, hl, pane=4.0)
        if body == 1 and abs(pos - 2.5 * seg) < 2.6:
            return 3  # pantograph
        if body == 1 and (abs(pos - 1.5 * seg) < 1.6 or abs(pos - 3.5 * seg) < 1.6) and a < 1.5:
            return 3  # roof equipment pods
        return body
    return zone


def ferry_zone(length=40.0, width=14.0):
    """Island ferry: blue hull, white deck, benches and a wheelhouse."""
    hl, hw = length / 2, width / 2

    def zone(f, s):
        a = abs(s)
        lim = hw if f < hl - 7 else hw * max(0.0, hl - f) / 7.0
        if abs(f) > hl or a > lim:
            return 0
        if a > lim - 1.3 or f < -hl + 2.0:
            return 2
        if abs(f - (hl - 13)) < 3.0 and a < hw - 3.0:
            return 3 if abs(f - (hl - 13)) > 2.2 or a > hw - 3.8 else 2
        if a < hw - 2.6 and abs(((f + 40) % 6.0) - 3.0) < 0.5 and f < hl - 17:
            return 2
        return 1
    return zone


def big_frame(zone, heading, size):
    return raster(zone, heading, size)


# ---------------------------------------------------------------- police and street life
# Patrol car: the sedan in white (1) with a blue (2) roof light bar and
# dark glass. Patrol units drive cardinally; diagonal slots repeat the
# nearest cardinal view so they cost no extra tiles.
def police_frames():
    return _cardinal_long(_sedan_long(body=1, lightbar=True, shine=False), LONG_CAR_LAMPS)


# Pursuit: the engine flashes the light bar and stripes by alternating the
# patrol car's palette between blue and red; no extra tiles.
# A struck person tumbles through the air (four quarter turns made from one
# drawing by flips), then lies on the ground. Non-graphic: no blood. Drawn
# in the people palette, so the actor's clothing colour carries over.
TUMBLE = [
    "................", "................", "................", "................",
    "................", ".....33.........", "....3113........", ".....3332.......",
    ".......3223.....", "........323.....", ".......3..3.....", "......3....3....",
    "................", "................", "................", "................"]
PRONE = [
    "................", "................", "................", "................",
    "................", "................", "................", "................",
    ".....3....3.....", "....31132223....", "....31132223....", ".....3....3.....",
    "................", "................", "................", "................"]


def knockdown_frames():
    """[tumble x4 (quarter turns), prone, prone facing the other way]."""
    t = grid(TUMBLE)
    p = grid(PRONE)
    return [t, flip_h(t), flip_v(flip_h(t)), flip_v(t), p, flip_h(p)]


# Effects drawn inside the left 8-pixel column so each needs one OAM object.
SPARK = [
    "................", "................", "................", "................",
    "...3............", ".3.1.3..........", "..111...........", "31111 13........",
    "..111...........", ".3.1.3..........", "...3............", "................",
    "................", "................", "................", "................"]
SPARK = [row.replace(' ', '1') for row in SPARK]
BULLET = [
    "................", "................", "................", "................",
    "................", "................", "................", "...33...........",
    "...31...........", "................", "................", "................",
    "................", "................", "................", "................"]
# Objective pointers, 8x8 in the left 8-pixel column (rows 4-11): east,
# south-east and south are drawn; the other five directions are flips.
_ARROW_E8 = ["33......", "3233....", "322233..", "32222233", "32222233", "322233..", "3233....", "33......"]
_ARROW_SE8 = [".......3", "......33", ".....323", "....3223", "...32223", "..322223", ".3222223", "33333333"]


def _left_column(rows8):
    blank = "." * 16
    return [blank] * 4 + [r + "." * 8 for r in rows8] + [blank] * 4


ARROW_E = _left_column(_ARROW_E8)
ARROW_SE = _left_column(_ARROW_SE8)
ARROW_S = _left_column(["".join(col) for col in zip(*_ARROW_E8)])


def arrow_frames():
    """Engine heading order E, SE, S, SW, W, NW, N, NE."""
    e, se, s = grid(ARROW_E), grid(ARROW_SE), grid(ARROW_S)
    return [e, se, s, flip_h_col(se), flip_h_col(e), flip_v(flip_h_col(se)), flip_v(s), flip_v(se)]


def flip_h_col(g):
    """Mirror a left-column drawing about that 8-pixel column."""
    return [row[:8][::-1] + row[8:] for row in g]


# ---------------------------------------------------------------- animation
# Courier action poses [right, left, down, up] on the short-haired figure:
# a punch (fist out) and the pistol held out (3 = the gun).
_ACTION = {
    'punch': {'right': (None, ["..3223..", "..32221.", "..3223..", "..333..."]),
              'down': (None, [".322223.", ".122222.", ".322232.", "..3223.1"]),
              'up': (["..3333.1", "..33332.", "..33332."], [".322232.", ".12222..", ".322223.", "..3223.."])},
    'shoot': {'right': (None, ["..3223.3", "..322213", "..3223..", "..333..."]),
              'down': (None, [".322223.", ".122222.", ".322232.", "..3223.3"]),
              'up': (["..3333.3", "..33331.", "..33332."], [".322232.", ".12222..", ".322223.", "..3223.."])},
}


def action_frames(kind):
    """[right, left, down, up] punch ('punch') or pistol ('shoot') pose."""
    out = {}
    for d, (head, torso) in _ACTION[kind].items():
        out[d] = _figure(head or _HEAD['short'][d], torso, _LEGS[d][0])
    return [out['right'], flip_h(out['right']), out['down'], out['up']]


def _centre8(rows8, top=4):
    """An 8-wide drawing in columns 4..11 starting at row `top`."""
    blank = "." * 16
    out = [blank] * top + ["...." + r + "...." for r in rows8]
    return grid(out + [blank] * (16 - len(out)))


# Tyre smoke and dust (traffic palette: 1 pale grey-blue, 3 near black): a
# dense fresh puff, then a dithered cloud that shimmers (its mirror image
# shares the tile).
SMOKE = [
    _centre8(["........", "...33...", "..3113..", ".311113.", ".311113.", "..3113..", "...33...", "........"]),
    _centre8(["..1.1...", ".1.1.1.1", "1.1.1.1.", ".1.1.1.1", "1.1.1.1.", ".1.1.1..", "..1.1...", "........"]),
]
SMOKE.append(flip_h(SMOKE[1]))
# A courier parcel that pops up when it is collected (courier palette).
PARCEL = _centre8([".333333.", "32221223", "32221223", "31111113", "32221223", "32221223", ".333333."], 5)
# Delivery sparkle (beacon yellow): a four-point star, then a burst.
SPARKLE = [
    _centre8(["...1....", "...1....", "..212...", "1122211.", "..212...", "...1....", "...1....", "........"]),
    _centre8(["1..1..1.", ".2.1.2..", "..2.2...", "111.111.", "..2.2...", ".2.1.2..", "1..1..1.", "........"]),
]


def _beam(angle_deg):
    """Night headlamp light ahead of the vehicle, dithered so the road shows
    through: a bright spot at each lamp, then two soft cones that thin out.
    The frame centre sits 16 px ahead of the vehicle centre (14 on the
    diagonals) and the light starts just past the bumper; the lamps are 2.5 px either
    side of its axis."""
    a = math.radians(angle_deg)
    ca, sa = math.cos(a), math.sin(a)
    g = [[0] * 16 for _ in range(16)]
    for y in range(16):
        for x in range(16):
            # Pixel centre relative to the vehicle centre, in its own frame.
            px, py = x + 0.5 - 8 + 14 * ca, y + 0.5 - 8 + 14 * sa
            f, s = px * ca + py * sa, -px * sa + py * ca
            if f < 8.5 or f > 21.5:
                continue
            near = min(abs(s - lamp) for lamp in (-2.5, 2.5))
            if near > 0.7 + (f - 8.5) * 0.3:
                continue
            yy = min(y, 15 - y) if angle_deg == 0 else y
            if f < 12 and near < 1.0:
                lit = (x + yy) % 2 == 0
            elif f < 16.5:
                lit = (x + yy) % 2 == 0 and near < 0.6 + (f - 8.5) * 0.18
            else:
                lit = (x + 2 * yy) % 4 == 0
            if lit:
                g[y][x] = 1
    return g


def beam_frames():
    """Engine heading order E, SE, S, SW, W, NW, N, NE (south is +y). East is
    mirror-symmetric about its axis, so south (its transpose) shares tiles."""
    e, se = _beam(0), _beam(45)
    s = transpose(e)
    return [e, se, s, flip_h(se), flip_h(e), flip_v(flip_h(se)), flip_v(s), flip_v(se)]


# Rounds in flight (beacon yellow: 1 white-hot, 2 gold): a fat white-hot head
# with a glow and a gold trail that thins behind it, 16 px long, so a shot
# reads at a glance without covering the street. East is drawn, south is its
# transpose (one 8-pixel column) and south-east a diagonal; the other
# headings are flips, so all eight cost five tiles and at most two hardware
# sprites. Each frame reports the pixel of its head.
def _shot_e():
    g = [[0] * 16 for _ in range(16)]
    for x in (0, 2, 4):
        g[7][x] = 2
    for x in range(5, 13):
        g[7][x] = 2
    for x in range(8, 13):
        g[8][x] = 2
    for x in (13, 14, 15):
        g[7][x] = g[8][x] = 1
    for x in (13, 14):
        g[6][x] = g[9][x] = 2
    return g


def _shot_se():
    g = [[0] * 16 for _ in range(16)]
    for k in (0, 2, 4):
        g[k][k] = 2
    for k in range(5, 13):
        g[k][k] = 2
        if k >= 8:
            g[k][k - 1] = 2
    for y, x in ((13, 13), (14, 14), (15, 15), (14, 15), (15, 14), (13, 14), (14, 13)):
        g[y][x] = 1
    for y, x in ((12, 14), (14, 12), (15, 13), (13, 15)):
        g[y][x] = 2
    return g


def shot_frames():
    """[(grid, size, head pixel)] in engine heading order E, SE, S, SW, W, NW, N, NE."""
    e, se = _shot_e(), _shot_se()
    s = transpose(e)
    sq = (16, 16)
    return [(e, sq, (15, 7)), (se, sq, (15, 15)), (s, sq, (7, 15)), (flip_h(se), sq, (0, 15)),
            (flip_h(e), sq, (0, 7)), (flip_v(flip_h(se)), sq, (0, 0)), (flip_v(s), sq, (7, 0)),
            (flip_v(se), sq, (15, 0))]


# Lock-on marker (traffic red, dark tips): corner brackets framing the
# whole target walker, inside one 8x16 tile.
RETICLE = grid(["................", "....22....22....", "....3......3....", "................",
                "................", "................", "................", "................",
                "................", "................", "................", "................",
                "................", "....3......3....", "....22....22....", "................"])
