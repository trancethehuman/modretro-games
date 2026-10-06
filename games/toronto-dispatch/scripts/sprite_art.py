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

def _layers(spec, tyres=True):
    g = [[0] * W for _ in range(H)]

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
    spec = [('body', 1, 4, 14, 11, body),
            ('cut', 1, 4, 1, 4, 0), ('cut', 14, 4, 14, 4, 0), ('cut', 1, 11, 1, 11, 0), ('cut', 14, 11, 14, 11, 0),
            ('cabin', 4, 6, 10, 9, 3), ('roof', 5, 7, 8, 8, body)]
    if shine:
        spec += [('flank', 2, 5, 13, 5, 1), ('roofshine', 5, 7, 5, 7, 1)]
    if sign:
        spec += [('sign', 6, 7, 7, 8, 1)]
    if lightbar:
        spec += [('stripe', 2, 5, 13, 5, 2), ('stripe', 2, 10, 13, 10, 2), ('bar', 6, 6, 7, 9, 2)]
    spec += [('glint', 10, 7, 10, 7, 1),
             ('tyre', 3, 3, 4, 3, 3), ('tyre', 11, 3, 12, 3, 3), ('tyre', 3, 12, 4, 12, 3), ('tyre', 11, 12, 12, 12, 3)]
    return spec


def _truck_spec():
    return [('box', 0, 4, 9, 11, 1), ('panel', 1, 5, 8, 5, 2), ('panel', 1, 10, 8, 10, 2),
            ('rib', 3, 6, 3, 9, 2), ('rib', 6, 6, 6, 9, 2),
            ('cab', 10, 5, 14, 10, 2), ('divide', 10, 5, 10, 10, 3), ('screen', 12, 6, 12, 9, 3),
            ('glint', 12, 6, 12, 6, 1), ('flank', 11, 5, 13, 5, 1),
            ('tyre', 2, 3, 4, 3, 3), ('tyre', 2, 12, 4, 12, 3), ('tyre', 11, 4, 12, 4, 3), ('tyre', 11, 11, 12, 11, 3)]


def _outline(g, lamps=()):
    out = [row[:] for row in g]
    for y in range(H):
        for x in range(W):
            if g[y][x] in (1, 2):
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if not (0 <= nx < W and 0 <= ny < H) or g[ny][nx] == 0:
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


def _vehicle(spec, lamps):
    east = _outline(_layers(spec), lamps)
    # The diagonal leaves out tyres (they would read as bumps) and gains its
    # outline after rotation.
    bare = _outline(_layers(spec, tyres=False), lamps)
    se = _outline(_rotate45([[c if c != 3 or _inside(bare, x, y) else 0 for x, c in enumerate(row)]
                             for y, row in enumerate(bare)]))
    return eight_headings(east, transpose(east), se)


def _inside(g, x, y):
    """Dark pixels enclosed by body on both sides of an axis are detail."""
    return ((0 < x < W - 1 and g[y][x - 1] and g[y][x + 1]) or (0 < y < H - 1 and g[y - 1][x] and g[y + 1][x]))


CAR_LAMPS = ((14, 5), (14, 10))
TRUCK_LAMPS = ((14, 6), (14, 9))


def car_frames(taxi=False):
    return _vehicle(_sedan_spec(sign=taxi), CAR_LAMPS)


def van_frames():
    return _vehicle(_truck_spec(), TRUCK_LAMPS)


def door_frame():
    """The parked courier car, east-facing, with its door swung open."""
    g = [row[:] for row in car_frames()[0]]
    for x, y, c in ((6, 12, 3), (6, 13, 3), (7, 13, 2), (7, 14, 3), (8, 14, 3)):
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
    east = grid(SCOOTER_E)
    return eight_headings(east, transpose(east), raster(moto_zone(9.0, 2.0), 45))


# ---------------------------------------------------------------- people
# 1 skin, 2 clothing, 3 hair/shoes/outline.
PERSON = {
    'down': [[
        "................", "......3333......", ".....333333.....", ".....311113.....",
        ".....311113.....", "......3113......", ".....322223.....", "....32222223....",
        "....12222221....", "....32222223....", ".....333333.....", ".....33..33.....",
        ".....33..33.....", ".....3....3.....", "................", "................"],
        ["................", "......3333......", ".....333333.....", ".....311113.....",
         ".....311113.....", "......3113......", ".....322223.....", "....32222223....",
         "....12222221....", "....32222223....", ".....333333.....", "......33.33.....",
         "......3..33.....", "..........3.....", "................", "................"]],
    'up': [[
        "................", "......3333......", ".....333333.....", ".....333333.....",
        ".....333333.....", "......3113......", ".....322223.....", "....32222223....",
        "....12222221....", "....32222223....", ".....333333.....", ".....33..33.....",
        ".....33..33.....", ".....3....3.....", "................", "................"],
        ["................", "......3333......", ".....333333.....", ".....333333.....",
         ".....333333.....", "......3113......", ".....322223.....", "....32222223....",
         "....12222221....", "....32222223....", ".....333333.....", ".....33.33......",
         ".....33..3......", ".....3..........", "................", "................"]],
    'right': [[
        "................", "......3333......", ".....33333......", ".....333113.....",
        ".....331113.....", "......3113......", "......3223......", ".....322223.....",
        ".....321223.....", ".....322223.....", "......3333......", "......3..3......",
        ".....33..33.....", ".....3....3.....", "................", "................"],
        ["................", "......3333......", ".....33333......", ".....333113.....",
         ".....331113.....", "......3113......", "......3223......", ".....322223.....",
         ".....322123.....", ".....322223.....", "......3333......", "......3333......",
         "......33........", "......3.........", "................", "................"]],
}
# Long-haired / ponytail walker: same body, different head silhouette.
LONG_HAIR = {
    'down': ["................", "......3333......", ".....333333.....", "....33111133....",
             "....33111133....", "....33.11.33....", ".....322223....."],
    'up': ["................", "......3333......", ".....333333.....", ".....333333.....",
           "....33333333....", "....33.33.33....", ".....322223....."],
    'right': ["................", "......3333......", ".....333333.....", "....3333113.....",
              "....3331113.....", "....33.3113.....", "......3223......"],
}


def person_frames(variant=0):
    """[right0, right1, left0, left1, down0, down1, up0, up1] for one body."""
    def head(rows, d):
        if variant == 1:
            return LONG_HAIR[d] + rows[len(LONG_HAIR[d]):]
        return rows
    r = [grid(head(PERSON['right'][i], 'right')) for i in (0, 1)]
    d = [grid(head(PERSON['down'][i], 'down')) for i in (0, 1)]
    u = [grid(head(PERSON['up'][i], 'up')) for i in (0, 1)]
    return [r[0], r[1], flip_h(r[0]), flip_h(r[1]), d[0], d[1], u[0], u[1]]


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


def streetcar_zone(length=64.0, width=9.0, sections=5):
    """Five-section low-floor streetcar seen from above, cabs at both ends."""
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
    east = _outline(_layers(_sedan_spec(body=1, lightbar=True, shine=False)), CAR_LAMPS)
    south = transpose(east)
    west, north = flip_h(east), flip_v(south)
    return [east, east, south, south, west, west, north, north]


# Pursuit: the light bar flashes white over its front half [E, S, W, N]; the
# engine alternates these with the plain patrol car.
def police_flash_frames():
    spec = _sedan_spec(body=1, lightbar=True, shine=False) + [('flash', 5, 6, 8, 7, 1), ('blue', 6, 8, 7, 9, 2)]
    east = _outline(_layers(spec), CAR_LAMPS)
    south = transpose(east)
    return [east, south, flip_h(east), flip_v(south)]


# Officer: the ordinary walker body under a navy peaked cap (2 = uniform).
CAP = {
    'down': ["................", "......2222......", ".....222222.....", "....33333333....",
             ".....311113.....", "......3113......", ".....322223....."],
    'up': ["................", "......2222......", ".....222222.....", ".....333333.....",
           ".....333333.....", "......3113......", ".....322223....."],
    'right': ["................", "......2222......", ".....22222......", ".....3333333....",
              ".....331113.....", "......3113......", "......3223......"],
}


def officer_frames():
    """[right0, right1, left0, left1, down0, down1, up0, up1] with a cap."""
    def head(rows, d):
        return CAP[d] + rows[len(CAP[d]):]
    r = [grid(head(PERSON['right'][i], 'right')) for i in (0, 1)]
    d = [grid(head(PERSON['down'][i], 'down')) for i in (0, 1)]
    u = [grid(head(PERSON['up'][i], 'up')) for i in (0, 1)]
    return [r[0], r[1], flip_h(r[0]), flip_h(r[1]), d[0], d[1], u[0], u[1]]


# A struck person tumbles through the air (four quarter turns made from one
# drawing by flips), then lies on the ground. Non-graphic: no blood.
TUMBLE = [
    "................",
    "................",
    "...333..........",
    "..31113.........",
    "..31113..33.....",
    "...333.3223.....",
    "....3322223.....",
    ".....322223.....",
    ".....3222233....",
    "......32223.....",
    ".......3333.....",
    "......33..33....",
    ".....33....33...",
    "................",
    "................",
    "................",
]
PRONE = [
    "................",
    "................",
    "................",
    "................",
    "................",
    "......33........",
    "..333..33.......",
    ".31113322222.33.",
    ".31113322222333.",
    ".31113322222.33.",
    "..333..33.......",
    "......33........",
    "................",
    "................",
    "................",
    "................",
]


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
# Courier action poses [right, left, down, up]: a punch (fist out) and the
# pistol held out. Each stays inside columns 4..11 (one 8x16 OBJ).
_PUNCH = {
    'right': {7: ".....3222221....", 8: ".....322223.....", 11: "......3...3.....",
              12: ".....33...33....", 13: ".....3.....3...."},
    'down': {8: "....12222222....", 9: "....32222232....", 10: ".....3333332....",
             11: ".....33..331...."},
    'up': {3: ".....3333331....", 4: ".....3333332....", 5: "......3113.2....",
           6: ".....3222232....", 8: "....12222223...."},
}
_SHOOT = {
    'right': {6: "......3223.3....", 7: ".....3222213....", 8: ".....322223....."},
    'down': {8: "....12222222....", 9: "....32222232....", 10: ".....3333331....",
             11: ".....33..333...."},
    'up': {2: ".....3333333....", 3: ".....3333331....", 4: ".....3333332....",
           5: "......3113.2....", 6: ".....3222232....", 8: "....12222223...."},
}


def _pose(changes, d):
    rows = list(PERSON[d][0])
    for r, row in changes.items():
        assert len(row) == 16, (d, r, row)
        rows[r] = row
    return grid(rows)


def action_frames(kind):
    """[right, left, down, up] punch ('punch') or pistol ('shoot') pose."""
    table = _PUNCH if kind == 'punch' else _SHOOT
    r = _pose(table['right'], 'right')
    return [r, flip_h(r), _pose(table['down'], 'down'), _pose(table['up'], 'up')]


def _centre8(rows8, top=4):
    """An 8-wide drawing in columns 4..11 starting at row `top`."""
    blank = "." * 16
    out = [blank] * top + ["...." + r + "...." for r in rows8]
    return grid(out + [blank] * (16 - len(out)))


# Tyre smoke and dust (traffic palette: 1 pale grey-blue, 3 near black): a
# dense fresh puff, a dithered cloud, then a thinning haze.
SMOKE = [
    _centre8(["........", "...33...", "..3113..", ".311113.", ".311113.", "..3113..", "...33...", "........"]),
    _centre8(["..1.1...", ".1.1.1.1", "1.1.1.1.", ".1.1.1.1", "1.1.1.1.", ".1.1.1..", "..1.1...", "........"]),
    _centre8(["...1....", ".1...1..", "....1..1", "1.1.....", "...1..1.", ".1...1..", "....1...", "........"]),
]
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
    The frame centre sits 14 px ahead of the vehicle centre; the lamps are
    2.5 px either side of its axis."""
    a = math.radians(angle_deg)
    ca, sa = math.cos(a), math.sin(a)
    g = [[0] * 16 for _ in range(16)]
    for y in range(16):
        for x in range(16):
            # Pixel centre relative to the vehicle centre, in its own frame.
            px, py = x + 0.5 - 8 + 14 * ca, y + 0.5 - 8 + 14 * sa
            f, s = px * ca + py * sa, -px * sa + py * ca
            if f < 7.5 or f > 20.5:
                continue
            near = min(abs(s - lamp) for lamp in (-2.5, 2.5))
            if near > 0.7 + (f - 7.5) * 0.3:
                continue
            if f < 11 and near < 1.0:
                lit = (x + y) % 2 == 0
            elif f < 15.5:
                lit = (x + y) % 2 == 0 and near < 0.6 + (f - 7.5) * 0.18
            else:
                lit = (x + 2 * y) % 4 == 0
            if lit:
                g[y][x] = 1
    return g


def beam_frames():
    """Engine heading order E, SE, S, SW, W, NW, N, NE (south is +y)."""
    e, se, s = _beam(0), _beam(45), _beam(90)
    return [e, se, s, flip_h(se), flip_h(e), flip_v(flip_h(se)), flip_v(s), flip_v(se)]
