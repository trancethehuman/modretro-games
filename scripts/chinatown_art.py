"""Chinatown on Spadina Ave and Dundas St W, Dragon City and Chinatown
Centre: original top-down pixel art for the core district
(create_city_art.py calls it).

Drawn as the street stands: narrow nineteenth-century shops whose fronts
are covered in stacked signboards (large Chinese-style glyphs over a band of
smaller lettering, after the bilingual signs), red lanterns strung along the
shopfronts, produce stalls on the sidewalk, and over Spadina at Dundas
original red dragon gates that recall "Gateway" (Millie Chen, 1997,
commissioned by the TTC for the 510 Spadina right-of-way). Glyphs are
abstract strokes, not text; nothing is copied from a photograph, sign or
logo.

Designs are 8 x 8 tiles in the source shades: '0' ink, '1' deep, '2' the
palette's colour, '3' light ('.' is '2'). One design takes any palette, so a
board is red, gold or green by its tile's palette: slot 1 (terracotta) is
Chinatown red, slot 4 gold, slot 6 green. Designs are stamped on the tile
grid so every shop reuses the same few tiles (the core scenes have a tile
budget, city_kit.SCENE_TILE_BUDGET).
"""
from PIL import Image, ImageDraw

RED, GOLD, GREEN, STONE, BLUE, MAUVE = 1, 4, 6, 0, 2, 3
SHADES = ['#071821', '#306850', '#86c06c', '#e0f8cf']


def _rows(*rows):
    assert len(rows) == 8 and all(len(r) == 8 for r in rows), rows
    return [r.replace('.', '2') for r in rows]


# Glyphs: 5 x 5 strokes, light on the board.
GLYPHS = [
    ["33333", "..3..", "33333", ".3.3.", "3...3"],
    ["..3..", "33333", "3.3.3", "33333", "..3.."],
    ["3.333", "3.3.3", "33333", "3.3.3", "3.333"],
    ["33333", "3...3", "33333", ".3.3.", "33333"],
]


def _board(glyph, left):
    """Signboard tile: a big glyph on the board; the left end carries the
    board's edge (the right end is its mirror)."""
    g = GLYPHS[glyph]
    rows = ["00000000", "0......." if left else "........"]
    rows += [("0." + g[k] + ".") if left else ("." + g[k] + "..") for k in range(5)]
    rows.append("00000000")
    return _rows(*rows)


def _vboard(glyph, end):
    """Vertical signboard tile on a shop's Spadina wall: a glyph on the
    board; the top end carries the board's edge (the bottom end is its
    vertical mirror)."""
    rows = ["10" + r + "0" for r in GLYPHS[glyph]]
    rows = ["10000000", "10.....0"] + rows + ["10.....0"] if end else rows + ["10.....0"] * 3
    return _rows(*rows)


DESIGNS = {}
for _g in range(len(GLYPHS)):
    DESIGNS[f'board_l{_g}'] = _board(_g, True)
    DESIGNS[f'board_m{_g}'] = _board(_g, False)
    DESIGNS[f'vboard_t{_g}'] = _vboard(_g, True)
    DESIGNS[f'vboard_m{_g}'] = _vboard(_g, False)
DESIGNS.update({
    # Lower signboard: smaller lettering (dark marks), then the shop's
    # striped awning.
    'band_l': _rows("0.......", "0.0.00.0", "0.00.0.0", "0.......", "00000000",
                    "3.3.3.3.", "3.3.3.3.", "00000000"),
    'band_m': _rows("........", "0.00.00.", ".00.0.00", "........", "00000000",
                    "3.3.3.3.", "3.3.3.3.", "00000000"),
    # Shopfronts under the awning (red tiles): a round red lantern hangs in
    # front of the window; roast ducks hang in a barbecue shop's window; the
    # door sits in the middle of the front.
    'lantern': _rows("11100111", "11022011", "10223201", "10222201", "11022011", "11100111",
                     "31311313", "00000000"),
    'bbq': _rows("10111101", "10111101", ".0.11.0.", ".3.11.3.", "...11...", "1.1111.1",
                 "31311313", "00000000"),
    'door': _rows("11100111", "13022031", "11000011", "13000031", "13000031", "11000011",
                  "11000011", "00000000"),
    # Produce stalls on the sidewalk: crates of greens, oranges or lychees
    # (the tile's palette), in two tiers.
    'stall': _rows("00000000", "03.3.3.0", "0.3.3.30", "0......0", "00000000", "0.3..3.0",
                   "01111110", "30000003"),
    # A rooftop air-handling unit on a tar roof.
    'roof_unit': _rows("11111111", "10000001", "10333301", "10300301", "10300301", "10333301",
                       "10000001", "11111111"),
    # The paved court between the malls, lanterns strung over it.
    'court': _rows("33333333", "00333300", "33000033", "333..333", "33.3..33", "333..333",
                   "33300333", "33333333"),
})
# The same stall beside a wall (its crates' backs against the wall).
DESIGNS['stall_v'] = ["".join(r[c] for r in DESIGNS['stall']) for c in range(8)]

# The gate over Spadina: the west half of its lintel (five tiles; the east
# half mirrors it about Spadina's centre line) and the post under its end.
# A red dragon runs along the lintel from the post to the middle of the
# street, where it meets its mirror over a pearl.
GATE_LINTEL = [
    "3000000000000000000000000000000000000000",
    "0333333330333303000000022200000002200000",
    "0322222230322303000003222223000022220000",
    "0322222230322303203002223232003022322200",
    "0322222230322303322222300002222222222033",
    "0322222230322303022222000000322222220233",
    "0333333330333303003230000000023202200033",
    "0000000000000000000000000000000000000000",
]
GATE_POST = [
    "11033011", "11022011", "11023011", "11022011", "11022011", "11023011", "11022011", "11022011",
    "11022011", "11023011", "11022011", "11022011", "10022001", "10333301", "10000001", "33333333",
]
for _k in range(len(GATE_LINTEL[0]) // 8):
    DESIGNS[f'gate_{_k}'] = _rows(*[r[_k * 8:_k * 8 + 8] for r in GATE_LINTEL])
for _k in range(len(GATE_POST) // 8):
    DESIGNS[f'post_{_k}'] = _rows(*GATE_POST[_k * 8:_k * 8 + 8])

# A bilingual street-name sign on its post (drawn over a finished curb
# tile; '.' keeps the tile): a dark plate, a glyph line over a lettered line.
STREET_SIGN = ["........", ".000000.", ".033030.", ".000000.", ".030330.", ".000000.", "...0....", "...0...."]


# ------------------------------------------------------------------ helpers
def tile(rows, flip=False, vflip=False):
    rows = rows[::-1] if vflip else rows
    im = Image.new('RGB', (8, 8))
    im.putdata([tuple(bytes.fromhex(SHADES[int(c)][1:])) for r in rows for c in (r[::-1] if flip else r)])
    return im


def stamp(img, attrs, tw, name, x, y, slot, priority=True, flip=False, vflip=False):
    """One design on the tile at (x, y), in a palette slot (with BG priority
    unless it lies on open ground)."""
    assert x % 8 == 0 and y % 8 == 0, (name, x, y)
    img.paste(tile(DESIGNS[name], flip, vflip), (x, y))
    attrs[(y // 8) * tw + x // 8] = slot | (128 if priority else 0)


def palette(attrs, tw, x, y, w, h, slot, priority=True):
    for ty in range(y // 8, (y + h + 7) // 8):
        for tx in range(x // 8, (x + w + 7) // 8):
            attrs[ty * tw + tx] = slot | (128 if priority else 0)


def painter(img):
    d = ImageDraw.Draw(img)

    def box(x, y, w, h, c):
        d.rectangle((x, y, x + w - 1, y + h - 1), fill=SHADES[c])

    def frame(x, y, w, h, c=0):
        d.rectangle((x, y, x + w - 1, y + h - 1), outline=SHADES[c])
    return d, box, frame


# -------------------------------------------------------------------- shops
# Signboards, top to bottom: (top board or None, big board, lettered band).
SIGNS = [(None, RED, GOLD), (GREEN, GOLD, RED), (None, GREEN, RED), (GOLD, RED, GREEN), (None, GOLD, GREEN),
         (RED, GREEN, GOLD)]
STALL_COLOURS = (GREEN, GOLD, RED)


def sign_row(img, attrs, tw, x, y, n, colour, k, glyphs=len(GLYPHS)):
    """A big glyph signboard n tiles long at (x, y)."""
    for c in range(n):
        g = (k + c) % glyphs
        if c in (0, n - 1):
            stamp(img, attrs, tw, f'board_l{g}', x + c * 8, y, colour, flip=c == n - 1)
        else:
            stamp(img, attrs, tw, f'board_m{g}', x + c * 8, y, colour)


def front_row(img, attrs, tw, x, y, n, k, lean=False):
    """Shopfront tiles: lanterns (roast ducks in some windows) and the door
    in the middle tile (n is odd, so it sits where building() draws it)."""
    assert n % 2, n
    for c in range(n):
        front = 'door' if c == n // 2 else ('bbq' if (k + c) % 3 == 0 and not lean else 'lantern')
        stamp(img, attrs, tw, front, x + c * 8, y, RED)


def shop(img, attrs, tw, x, y, w, h, k, lean=False):
    """Chinatown shop front on the bottom tile rows of a building footprint:
    one or two big glyph signboards, a lettered band over the awning, and
    the shopfront with lanterns and the door in the middle. A lean shop
    uses fewer designs (a scene at its tile budget)."""
    glyphs = 2 if lean else len(GLYPHS)
    n = w // 8
    assert w % 8 == 0 and n % 2 and h >= 40 and (y + h) % 8 == 0, (x, y, w, h)
    top, board, band = SIGNS[k % len(SIGNS)]
    rows = [(y + h - 24, board)] + ([(y + h - 32, top)] if top else [])
    for r, (ty, colour) in enumerate(rows):
        sign_row(img, attrs, tw, x, ty, n, colour, (0 if lean else k) + 2 * r, glyphs)
    for c in range(n):
        stamp(img, attrs, tw, 'band_l' if c in (0, n - 1) else 'band_m', x + c * 8, y + h - 16, band, flip=c == n - 1)
    front_row(img, attrs, tw, x, y + h - 8, n, k, lean)


def tar_roof(img, x0, y0, x1, y1):
    """Flat tar roofs (the deep shade) instead of the painted roof colour."""
    px = img.load()
    accent, deep = tuple(bytes.fromhex(SHADES[2][1:])), tuple(bytes.fromhex(SHADES[1][1:]))
    for y in range(y0, y1):
        for x in range(x0, x1):
            if px[x, y] == accent:
                px[x, y] = deep


def wall_signs(img, attrs, tw, x, y0, y1, k, east):
    """Vertical signboards stacked along a shop's wall on Spadina (the
    tile column at x), from y0 down to y1: three-tile boards in turn."""
    colours = (RED, GOLD, GREEN)
    y, n = y0, 0
    while y + 24 <= y1:
        colour = colours[(k + n) % 3]
        g = (k + n) % 2
        stamp(img, attrs, tw, f'vboard_t{g}', x, y, colour, flip=not east)
        stamp(img, attrs, tw, f'vboard_m{1 - g}', x, y + 8, colour, flip=not east)
        stamp(img, attrs, tw, f'vboard_t{g}', x, y + 16, colour, flip=not east, vflip=True)
        y += 24; n += 1


def stalls(img, attrs, tw, x, y, w, k, plain):
    """Produce stalls on the sidewalk row in front of a shop (y: that row),
    either side of the door, on tiles plain(tx, ty) says are untouched
    sidewalk."""
    n = w // 8
    for c in range(n):
        if c == n // 2 or (c + k) % 2:
            continue
        tx = x + c * 8
        if plain(tx // 8, y // 8):
            stamp(img, attrs, tw, 'stall', tx, y, STALL_COLOURS[(k + c) % 3], priority=False)


def side_stalls(img, attrs, tw, x, y, n, k, plain, east):
    """Produce stalls in a column of sidewalk tiles beside a shop's wall on
    Spadina (east: the wall is to their west)."""
    for r in range(n):
        if plain(x // 8, (y + 8 * r) // 8):
            stamp(img, attrs, tw, 'stall_v', x, y + 8 * r, STALL_COLOURS[(k + r) % 3], priority=False, flip=not east)


# ------------------------------------------------------ the street features
def gateway(img, attrs, tw, cx, y, post_x):
    """The dragon gate over Spadina: the lintel's tile row at y, centred on
    cx (a tile boundary), and two posts two tiles tall under it at post_x
    from the centre. Returns the lintel's tile rectangle."""
    assert cx % 8 == 0 and y % 8 == 0 and post_x % 8 == 0
    half = len(GATE_LINTEL[0])
    for k in range(half // 8):
        stamp(img, attrs, tw, f'gate_{k}', cx - half + k * 8, y, RED)
        stamp(img, attrs, tw, f'gate_{k}', cx + half - 8 - k * 8, y, RED, flip=True)
    for k in range(len(GATE_POST) // 8):
        stamp(img, attrs, tw, f'post_{k}', cx - post_x, y + 8 + 8 * k, RED, priority=False)
        stamp(img, attrs, tw, f'post_{k}', cx + post_x - 8, y + 8 + 8 * k, RED, priority=False, flip=True)
    return (cx - half, y, 2 * half, 8)


def street_sign(img, x, y):
    """A bilingual street sign drawn over the sidewalk tile at (x, y)."""
    px = img.load()
    for r, row in enumerate(STREET_SIGN):
        for c, ch in enumerate(row):
            if ch != '.':
                px[x + c, y + r] = tuple(bytes.fromhex(SHADES[int(ch)][1:]))


# ------------------------------------------------------------ the two malls
def glass_pyramid(img, cx, cy, r):
    """A square glass roof rising to a point, seen from above: ridges from
    the corners, panes on a lattice, centred on a tile corner so its four
    quarters are flips of one."""
    px = img.load()
    for y in range(cy - r, cy + r):
        for x in range(cx - r, cx + r):
            a, b = abs(x + 0.5 - cx), abs(y + 0.5 - cy)
            if max(a, b) >= r - 1 or abs(a - b) < 0.8:
                c = 0
            elif (int(a) + int(b)) % 4 == 1:
                c = 1
            elif int(a) % 4 == 2 and int(b) % 4 == 2:
                c = 3
            else:
                c = 2
            px[x, y] = tuple(bytes.fromhex(SHADES[c][1:]))


def dragon_city(img, attrs, tw, x, y, w, h, lip):
    """Dragon City (280 Spadina Ave) at the south-west corner of Spadina
    and Dundas: a three-storey podium of shops, its glass-roofed public
    court at the corner, the residential tower set back from the
    intersection, and the signboards and lanterns of its south front."""
    d, box, frame = painter(img)
    n = w // 8
    roof_h = h - 16
    # Podium roof: a parapet round a paved terrace.
    box(x, y, w, roof_h, 2); frame(x, y, w, roof_h)
    d.rectangle((x + 2, y + 2, x + w - 3, y + roof_h - 1), outline=SHADES[1])
    for yy in range(y + 8, y + roof_h - 2, 8):
        d.line((x + 3, yy, x + w - 4, yy), fill=SHADES[3])
    palette(attrs, tw, x, y - lip, w, roof_h + lip, GOLD)
    # The glass court at the corner (north-east of the footprint).
    glass_pyramid(img, x + w - 16, y + 16, 16)
    palette(attrs, tw, x + w - 32, y, 32, 32, BLUE)
    # The tower, set back to the south-west of the podium: its roof and
    # crown, the balconies of its south face, its shadow on the terrace.
    tx0, ty0, tw0, th0 = x + 8, y + 8, 48, roof_h - 8
    box(tx0 + tw0, ty0 + 3, 3, th0 - 3, 1)
    box(tx0, ty0, tw0, th0, 2); frame(tx0, ty0, tw0, th0)
    box(tx0 + 8, ty0 + 4, tw0 - 16, 12, 1); frame(tx0 + 8, ty0 + 4, tw0 - 16, 12)
    box(tx0 + 12, ty0 + 8, 4, 4, 3); box(tx0 + tw0 - 16, ty0 + 8, 4, 4, 3)
    face = ty0 + th0 - 16
    d.line((tx0, face, tx0 + tw0 - 1, face), fill=SHADES[0])
    for yy in (face + 3, face + 9):
        for xx in range(tx0 + 3, tx0 + tw0 - 3, 4):
            box(xx, yy, 2, 3, 3)
        d.line((tx0 + 1, yy + 4, tx0 + tw0 - 2, yy + 4), fill=SHADES[1])
    palette(attrs, tw, tx0, ty0, tw0, th0, MAUVE)
    # South front: the signboard row and the shops with the door.
    sign_row(img, attrs, tw, x, y + h - 16, n - 4, RED, 1)
    sign_row(img, attrs, tw, x + w - 32, y + h - 16, 4, GOLD, 3)
    front_row(img, attrs, tw, x, y + h - 8, n, 1)


def chinatown_centre(img, attrs, tw, x, y, w, h):
    """Chinatown Centre (222 Spadina Ave), the large 1960s mall south of
    Dragon City: a broad flat roof with its long skylight over the
    concourse and plant rooms, precast concrete fins along the front,
    the signboards and the entrance."""
    d, box, frame = painter(img)
    n = w // 8
    roof_h = h - 24
    box(x, y, w, roof_h, 2); frame(x, y, w, roof_h)
    d.rectangle((x + 2, y + 2, x + w - 3, y + roof_h - 1), outline=SHADES[1])
    # The skylight along the concourse.
    sy = y + 16
    box(x + 8, sy, w - 16, 8, 1); frame(x + 8, sy, w - 16, 8)
    for xx in range(x + 10, x + w - 9, 4):
        d.line((xx, sy + 1, xx, sy + 6), fill=SHADES[3])
    palette(attrs, tw, x, y, w, roof_h, MAUVE)
    palette(attrs, tw, x + 8, sy, w - 16, 8, BLUE)
    # Plant rooms on the roof.
    for px in (x + 16, x + w - 40):
        box(px, y + 32, 24, 12, 1); frame(px, y + 32, 24, 12)
        for xx in range(px + 3, px + 21, 4):
            d.line((xx, y + 34, xx, y + 41), fill=SHADES[0])
    # Precast fins along the front, then the signboards and entrance.
    fy = y + roof_h
    box(x, fy, w, 8, 3); d.line((x, fy, x + w - 1, fy), fill=SHADES[0])
    for xx in range(x + 1, x + w - 1, 4):
        d.line((xx, fy + 1, xx, fy + 7), fill=SHADES[1])
        d.point((xx + 1, fy + 1), fill=SHADES[0])
    d.line((x, fy, x, fy + 7), fill=SHADES[0]); d.line((x + w - 1, fy, x + w - 1, fy + 7), fill=SHADES[0])
    palette(attrs, tw, x, fy, w, 8, STONE)
    sign_row(img, attrs, tw, x, y + h - 16, 5, GREEN, 2)
    sign_row(img, attrs, tw, x + 40, y + h - 16, n - 5, RED, 0)
    front_row(img, attrs, tw, x, y + h - 8, n, 2)


def court(img, attrs, tw, x, y, w, h):
    """The paved court between the malls, open to Spadina, with lanterns
    strung over it."""
    for tx in range(x, x + w, 8):
        for ty in range(y, y + h, 8):
            stamp(img, attrs, tw, 'court', tx, ty, RED, priority=False)
