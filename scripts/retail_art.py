"""Dufferin Mall and BYTE BARN: original top-down pixel art for the core
district's retail landmarks (create_city_art.py calls it).

Dufferin Mall (900 Dufferin St) is drawn as a low mall on the west side of
Dufferin St opposite Dufferin Grove Park, with surface parking north and
south of it. BYTE BARN is a fictional electronics superstore (a parody
brand of this game, not any real chain): a big box in Liberty Village with
a barn-red ribbed roof, its name in large letters, barn-door entrances and
a parking lot. Colours come from the city palettes (create_city_art.py);
lettering uses the game's own fonts (ui_art.FONT and a 3 x 5 face here).
"""
from ui_art import FONT
from chinatown_art import SHADES, RED, GOLD, STONE, BLUE, palette, painter

# 3 x 5 capitals (M and N wider) for small signs.
MINI = {
    'A': [".#.", "#.#", "###", "#.#", "#.#"], 'D': ["##.", "#.#", "#.#", "#.#", "##."],
    'E': ["###", "#..", "##.", "#..", "###"], 'F': ["###", "#..", "##.", "#..", "#.."],
    'I': ["###", ".#.", ".#.", ".#.", "###"], 'L': ["#..", "#..", "#..", "#..", "###"],
    'M': ["#...#", "##.##", "#.#.#", "#...#", "#...#"], 'N': ["#..#", "##.#", "#.##", "#..#", "#..#"],
    'R': ["##.", "#.#", "##.", "#.#", "#.#"], 'U': ["#.#", "#.#", "#.#", "#.#", "###"],
    ' ': ["..", "..", "..", "..", ".."],
}


def text_width(text, face):
    return sum(len(face[ch][0]) + 1 for ch in text) - 1


def letter(d, text, x, y, face, shade):
    """Draw text with its top-left at (x, y) in one shade."""
    for ch in text:
        rows = face[ch]
        for r, row in enumerate(rows):
            for c, p in enumerate(row):
                if p == '#':
                    d.point((x + c, y + r), fill=SHADES[shade])
        x += len(rows[0]) + 1


def parking(img, x, y, w, h, rows_at, poles=()):
    """A surface lot: asphalt, painted stalls (an 8 px period, so stall
    tiles repeat) in rows starting at rows_at, and light poles."""
    d, box, frame = painter(img)
    box(x, y, w, h, 1)
    for ry in rows_at:
        d.line((x, ry, x + w - 1, ry), fill=SHADES[3])
        for sx in range(x, x + w, 8):
            d.line((sx, ry, sx, ry + 7), fill=SHADES[3])
    for px, py in poles:
        box(px - 1, py - 1, 3, 3, 0); d.point((px, py), fill=SHADES[3])


def dufferin_mall(img, attrs, tw, x, y, w, h, lip):
    """Dufferin Mall: a long low roof over the concourse skylight, plant
    rooms and roof hatches, the mall's name over the south entrance."""
    d, box, frame = painter(img)
    assert (w // 8) % 2, 'the door sits in the middle tile'
    roof_h = h - 16
    box(x, y, w, roof_h, 2); frame(x, y, w, roof_h)
    d.rectangle((x + 2, y + 2, x + w - 3, y + roof_h - 1), outline=SHADES[1])
    palette(attrs, tw, x, y - lip, w, roof_h + lip, STONE)
    # The concourse skylight down the middle, with a crossing at its centre
    # court; panes on a 4 px rhythm so the strip repeats one tile.
    cx = x + w // 2
    box(cx - 8, y + 8, 16, roof_h - 16, 1); frame(cx - 8, y + 8, 16, roof_h - 16)
    for yy in range(y + 10, y + roof_h - 9, 4):
        d.line((cx - 7, yy, cx + 6, yy), fill=SHADES[3])
    d.line((cx - 1, y + 9, cx - 1, y + roof_h - 10), fill=SHADES[0]); d.line((cx, y + 9, cx, y + roof_h - 10), fill=SHADES[0])
    palette(attrs, tw, cx - 8, y + 8, 16, roof_h - 16, BLUE)
    # Plant rooms and hatches either side of the skylight.
    for px in (x + 8, x + w - 24):
        for py in range(y + 16, y + roof_h - 24, 48):
            box(px, py, 16, 16, 1); frame(px, py, 16, 16)
            for xx in range(px + 3, px + 14, 4):
                d.line((xx, py + 3, xx, py + 12), fill=SHADES[0])
    # South front: the name on its band, glass doors under a canopy.
    sy = y + h - 16
    box(x, sy, w, 8, 0)
    name = 'DUFFERIN MALL'
    letter(d, name, x + (w - text_width(name, MINI)) // 2, sy + 2, MINI, 3)
    palette(attrs, tw, x, sy, w, 8, GOLD)
    fy = y + h - 8
    box(x, fy, w, 8, 1); d.line((x, fy, x + w - 1, fy), fill=SHADES[0])
    for xx in range(x + 2, x + w - 2, 4):
        box(xx, fy + 2, 2, 4, 3)
    box(x + w // 2 - 6, fy + 1, 12, 7, 0)
    box(x + w // 2 - 2, fy + 3, 4, 5, 3)
    d.line((x + w // 2, fy + 3, x + w // 2, fy + 7), fill=SHADES[0])
    d.line((x, fy + 7, x + w - 1, fy + 7), fill=SHADES[0])
    palette(attrs, tw, x, fy, w, 8, STONE)


def byte_barn(img, attrs, tw, x, y, w, h):
    """BYTE BARN: a barn-red ribbed metal roof, a gambrel barn mark and the
    name in large letters along the front, glass and barn-door entrances."""
    d, box, frame = painter(img)
    roof_h = h - 24
    box(x, y, w, roof_h, 2); frame(x, y, w, roof_h)
    for xx in range(x + 4, x + w - 3, 4):
        d.line((xx, y + 1, xx, y + roof_h - 2), fill=SHADES[1])
    palette(attrs, tw, x, y, w, roof_h, RED)
    # Roof units in a row, and the sign band: a gambrel barn mark and the
    # name, light on ink.
    for px in range(x + 16, x + w - 16, 40):
        box(px, y + 12, 16, 12, 3); frame(px, y + 12, 16, 12)
        d.line((px + 2, y + 18, px + 13, y + 18), fill=SHADES[0])
    sy = y + roof_h
    box(x, sy, w, 16, 0)
    name = 'BYTE BARN'
    text_w = text_width(name, FONT)
    mark = 14
    lx = x + (w - text_w - mark - 4) // 2
    # Gambrel barn mark: roof, walls and an X-braced door.
    bx, by = lx, sy + 3
    for k, (a, b) in enumerate(((5, 8), (2, 11), (0, 13), (0, 13))):
        d.line((bx + a, by + k, bx + b, by + k), fill=SHADES[2])
    box(bx + 1, by + 4, 12, 7, 2)
    frame(bx + 4, by + 6, 6, 5, 3)
    d.line((bx + 4, by + 6, bx + 9, by + 10), fill=SHADES[3]); d.line((bx + 9, by + 6, bx + 4, by + 10), fill=SHADES[3])
    letter(d, name, lx + mark + 4, sy + 4, FONT, 3)
    palette(attrs, tw, x, sy, w, 16, RED)
    # The front: glass, and barn doors at the entrance in the middle.
    fy = y + h - 8
    box(x, fy, w, 8, 1); d.line((x, fy, x + w - 1, fy), fill=SHADES[0])
    for xx in range(x + 2, x + w - 2, 4):
        box(xx, fy + 2, 2, 4, 3)
    cx = x + w // 2
    for ex in (cx - 12, cx + 4):
        box(ex, fy + 1, 8, 7, 2); frame(ex, fy + 1, 8, 7)
        d.line((ex, fy + 1, ex + 7, fy + 7), fill=SHADES[0]); d.line((ex + 7, fy + 1, ex, fy + 7), fill=SHADES[0])
    box(cx - 4, fy + 1, 8, 7, 0); box(cx - 2, fy + 3, 4, 5, 3)
    d.line((x, fy + 7, x + w - 1, fy + 7), fill=SHADES[0])
    palette(attrs, tw, x, fy, w, 8, RED)
