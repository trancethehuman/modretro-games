"""Original Game Boy Color menu art for Toronto Dispatch.

Each tile is 8x8 rows of: '.' paper (index 0), 'a' palette accent (index 1),
's' index 2 (slate in the city palettes, red in the UI palette) and '#' ink
(index 3). Palettes by name match the background slots that every district
loads: the seven city palettes share paper, slate and ink, so any tile can
sit beside any other without a seam.
"""

# Background palette slot of each named palette; 7 is the UI palette.
PALETTES = {
    'teal': 0, 'terra': 1, 'blue': 2, 'rose': 3, 'sand': 4, 'glass': 5, 'green': 6, 'ui': 7,
}
UI_COLORS = ['E7DECC', 'E89A3C', 'C83C34', '172B38']
# The atlas map was drawn for the original UI colours; it swaps them in while open.
MAP_COLORS = ['F8F8B8', '90C8C8', '486878', '082048']

# name: (palette, rows). Flipped variants are separate codes that reuse a tile.
TILES = {
    # Frame: ink outline with an amber inner line and rounded corners.
    'frame_tl': ('ui', [
        "........",
        "..######",
        ".#aaaaaa",
        ".#a.....",
        ".#a.....",
        ".#a.....",
        ".#a.....",
        ".#a.....",
    ]),
    'frame_t': ('ui', [
        "........",
        "########",
        "aaaaaaaa",
        "........",
        "........",
        "........",
        "........",
        "........",
    ]),
    'frame_l': ('ui', [
        ".#a.....",
        ".#a.....",
        ".#a.....",
        ".#a.....",
        ".#a.....",
        ".#a.....",
        ".#a.....",
        ".#a.....",
    ]),
    # Separator joining the frame sides.
    'frame_join': ('ui', [
        ".#a.....",
        ".#a.....",
        ".#a.....",
        ".#aaaaaa",
        ".#a.....",
        ".#a.....",
        ".#a.....",
        ".#a.....",
    ]),
    'rule': ('ui', [
        "........",
        "........",
        "........",
        "aaaaaaaa",
        "........",
        "........",
        "........",
        "........",
    ]),
    'rule_dot': ('ui', [
        "........",
        "........",
        "...##...",
        "aa#ss#aa",
        "...##...",
        "........",
        "........",
        "........",
    ]),
    # Menu cursor, two frames for a gentle bob.
    'cursor_0': ('ui', [
        "........",
        ".#......",
        ".##.....",
        ".#s#....",
        ".#ss#...",
        ".#s#....",
        ".##.....",
        ".#......",
    ]),
    'cursor_1': ('ui', [
        "........",
        "..#.....",
        "..##....",
        "..#s#...",
        "..#ss#..",
        "..#s#...",
        "..##....",
        "..#.....",
    ]),
    # Menu icons.
    'icon_resume': ('ui', [
        "..####..",
        ".#aaaa#.",
        "#aa#aaa#",
        "#aa##aa#",
        "#aa###a#",
        "#aa##aa#",
        ".#a#aa#.",
        "..####..",
    ]),
    'icon_map': ('green', [
        "########",
        "#aa.saa#",
        "#a..s.a#",
        "#ssssss#",
        "#.a.s.a#",
        "#aa.s..#",
        "#a..saa#",
        "########",
    ]),
    'icon_jobs': ('ui', [
        "..####..",
        ".#ssss#.",
        ".#....#.",
        ".#.##.#.",
        ".#....#.",
        ".#.##.#.",
        ".#....#.",
        ".######.",
    ]),
    'icon_park': ('blue', [
        "########",
        "#aaaaaa#",
        "#a...aa#",
        "#a.aa.a#",
        "#a...aa#",
        "#a.aaaa#",
        "#a.aaaa#",
        "########",
    ]),
    'icon_car': ('ui', [
        "........",
        "..####..",
        ".#a..a#.",
        "#aaaaaa#",
        "#aaaaaa#",
        "########",
        ".##..##.",
        "........",
    ]),
    'icon_transit': ('ui', [
        "..####..",
        ".#ssss#.",
        "#s####s#",
        "#ss##ss#",
        "#ss##ss#",
        "#ss##ss#",
        ".#ssss#.",
        "..####..",
    ]),
    'icon_save': ('blue', [
        "#######.",
        "#aaaaa##",
        "#a...a.#",
        "#a...a.#",
        "#aaaaaa#",
        "#a####a#",
        "#a#..#a#",
        "########",
    ]),
    'icon_cancel': ('ui', [
        "........",
        ".##..##.",
        ".#s##s#.",
        "..#ss#..",
        "..#ss#..",
        ".#s##s#.",
        ".##..##.",
        "........",
    ]),
    'icon_audio': ('ui', [
        "....#...",
        "...##.#.",
        "###a#..#",
        "#aaa#.#.",
        "#aaa#.#.",
        "###a#..#",
        "...##.#.",
        "....#...",
    ]),
    # HUD icons.
    'icon_coin': ('ui', [
        "..####..",
        ".#aaaa#.",
        "#aa..aa#",
        "#a.aaaa#",
        "#aa..aa#",
        "#aaaa.a#",
        ".#a..a#.",
        "..####..",
    ]),
    'icon_box': ('terra', [
        "........",
        ".######.",
        "#aa##aa#",
        "########",
        "#aa##aa#",
        "#aa##aa#",
        "#aaaaaa#",
        "########",
    ]),
    'icon_clock': ('ui', [
        "..####..",
        ".#....#.",
        "#...#..#",
        "#...#..#",
        "#...ss.#",
        "#......#",
        ".#....#.",
        "..####..",
    ]),
    'icon_clock_alert': ('ui', [
        "..####..",
        ".#ssss#.",
        "#sss#ss#",
        "#sss#ss#",
        "#sss..s#",
        "#ssssss#",
        ".#ssss#.",
        "..####..",
    ]),
    'icon_heart': ('ui', [
        "........",
        ".##.##..",
        "#ss#ss#.",
        "#s.sss#.",
        "#sssss#.",
        ".#sss#..",
        "..#s#...",
        "...#....",
    ]),
    'icon_pin': ('ui', [
        "..####..",
        ".#ssss#.",
        "#ss..ss#",
        "#ss..ss#",
        ".#ssss#.",
        "..#ss#..",
        "...##...",
        "...#....",
    ]),
    'icon_walk': ('ui', [
        "...##...",
        "...##...",
        "..####..",
        ".#.##.#.",
        "...##...",
        "..#..#..",
        "..#..#..",
        ".##..##.",
    ]),
    'icon_street': ('blue', [
        "########",
        "#aaaaaa#",
        "#a.a.aa#",
        "#aaaaaa#",
        "########",
        "...##...",
        "...##...",
        "...##...",
    ]),
    'icon_target': ('ui', [
        "..####..",
        ".#ssss#.",
        "#ss..ss#",
        "#s.##.s#",
        "#s.##.s#",
        "#ss..ss#",
        ".#ssss#.",
        "..####..",
    ]),
    'icon_star': ('ui', [
        "...#....",
        "..#a#...",
        "###a###.",
        "#aaaaa#.",
        ".#aaa#..",
        ".#a#a#..",
        "#a#.#a#.",
        "##...##.",
    ]),
    # Courier first aid: a red cross on a white tile edge.
    'icon_medic': ('ui', [
        "........",
        "..####..",
        "..#ss#..",
        "###ss###",
        "#ssssss#",
        "###ss###",
        "..#ss#..",
        "..####..",
    ]),
    # Pistol rounds: an amber cartridge.
    # Time of day in the pause menu: the sun (07:00-18:59) and the moon.
    'icon_sun': ('ui', [
        "...#....",
        "#.#a#.#.",
        ".#aaa#..",
        "#aaaaa#.",
        ".#aaa#..",
        "#.#a#.#.",
        "...#....",
        "........",
    ]),
    'icon_moon': ('ui', [
        "..####..",
        ".#aa#...",
        "#aa#....",
        "#aa#....",
        "#aa#....",
        "#aaa#..#",
        ".#aaa##.",
        "..####..",
    ]),
    'icon_ammo': ('ui', [
        "........",
        "...##...",
        "..#aa#..",
        "..#aa#..",
        "..####..",
        "..#ss#..",
        "..#ss#..",
        "..####..",
    ]),
    'icon_check': ('green', [
        "........",
        "......##",
        ".....#a#",
        "##..#a#.",
        "#a##a#..",
        ".#aa#...",
        "..##....",
        "........",
    ]),
    'icon_ferry': ('blue', [
        "........",
        "..####..",
        "..#..#..",
        "########",
        "#aaaaaa#",
        ".#aaaa#.",
        "..####..",
        "a.a.a.a.",
    ]),
    'icon_bus': ('ui', [
        ".######.",
        "#ssssss#",
        "#......#",
        "#ssssss#",
        "#ssssss#",
        "#a#ss#a#",
        "########",
        ".##..##.",
    ]),
    'icon_subway': ('sand', [
        "..####..",
        ".#aaaa#.",
        "#aa##aa#",
        "#a#a#aa#",
        "#aaa#aa#",
        "#aaa#aa#",
        ".#a###.#",
        "..####..",
    ]),
    # Compass arrows: north, east, north-east; flips give the other five.
    'arrow_n': ('ui', [
        "...##...",
        "..#ss#..",
        ".#ssss#.",
        "##s##s##",
        "..#ss#..",
        "..#ss#..",
        "..#ss#..",
        "..####..",
    ]),
    'arrow_e': ('ui', [
        "........",
        "....##..",
        "....#s#.",
        "#####ss#",
        "#sssssss",
        "#####ss#",
        "....#s#.",
        "....##..",
    ]),
    'arrow_ne': ('ui', [
        "........",
        "..######",
        "..#ssss#",
        "...##ss#",
        "..#ss#s#",
        ".#ss#.##",
        "#ss#...#",
        ".##.....",
    ]),
    # Buttons: GB-style A/B discs and SELECT/START pills.
    'btn_a': ('ui', [
        "..####..",
        ".#ssss#.",
        "#ss..ss#",
        "#s.ss.s#",
        "#s....s#",
        "#s.ss.s#",
        ".#ssss#.",
        "..####..",
    ]),
    'btn_b': ('ui', [
        "..####..",
        ".#ssss#.",
        "#s...ss#",
        "#s.ss.s#",
        "#s...ss#",
        "#s.ss.s#",
        ".#s..s#.",
        "..####..",
    ]),
    'dpad': ('ui', [
        "..###...",
        "..#a#...",
        "###a###.",
        "#aaaaa#.",
        "###a###.",
        "..#a#...",
        "..###...",
        "........",
    ]),
}


MINI = {  # 3x5 capitals for button pills
    'S': ["###", "#..", "###", "..#", "###"], 'E': ["###", "#..", "##.", "#..", "###"],
    'L': ["#..", "#..", "#..", "#..", "###"], 'T': ["###", ".#.", ".#.", ".#.", ".#."],
    'A': ["###", "#.#", "###", "#.#", "#.#"], 'R': ["##.", "#.#", "##.", "#.#", "#.#"],
    'C': ["###", "#..", "#..", "#..", "###"],
}


def pill(text, tiles):
    """Ink capsule with paper mini letters, split into 8x8 tiles."""
    w = tiles * 8
    grid = [['.'] * w for _ in range(8)]
    for y in range(1, 8):
        for x in range(w):
            corner = (y in (1, 7)) and (x in (0, w - 1))
            if not corner:
                grid[y][x] = '#'
    tw = len(text) * 4 - 1
    x0 = (w - tw) // 2
    for k, ch in enumerate(text):
        for dy, row in enumerate(MINI[ch]):
            for dx, c in enumerate(row):
                if c == '#':
                    grid[2 + dy][x0 + 4 * k + dx] = '.'
    return [[''.join(grid[y][t * 8:t * 8 + 8]) for y in range(8)] for t in range(tiles)]


def emblem():
    """16x16 badge: CN Tower against an amber sky over an ink skyline."""
    g = [['.'] * 16 for _ in range(16)]
    for y in range(16):
        for x in range(16):
            d = ((x - 7.5) ** 2 + (y - 7.5) ** 2) ** 0.5
            if d <= 7.6:
                g[y][x] = '#' if d > 6.6 else 'a'
    tower = {1: (7, 8), 2: (7, 8), 3: (7, 8), 4: (6, 9), 5: (7, 8), 6: (5, 10), 7: (5, 10),
             8: (6, 9), 9: (7, 8), 10: (7, 8), 11: (6, 9), 12: (6, 9), 13: (6, 9), 14: (5, 10)}
    for y, (a, b) in tower.items():
        for x in range(a, b + 1):
            if g[y][x] != '.':
                g[y][x] = '#'
    g[7][6] = g[7][9] = 's'  # lit restaurant windows
    skyline = [(1, 3, 11), (3, 5, 10), (11, 12, 9), (12, 14, 11)]
    for x0, x1, top in skyline:
        for x in range(x0, x1 + 1):
            for y in range(top, 16):
                if g[y][x] == 'a':
                    g[y][x] = '#'
    for x, y in ((2, 12), (4, 11), (13, 12), (12, 10)):
        if g[y][x] == '#':
            g[y][x] = 'a'
    rows = [''.join(r) for r in g]
    return [[r[0:8] for r in rows[0:8]], [r[8:16] for r in rows[0:8]],
            [r[0:8] for r in rows[8:16]], [r[8:16] for r in rows[8:16]]]


for _name, _rows in zip(('btn_sel_0', 'btn_sel_1'), pill('SEL', 2)):
    TILES[_name] = ('ui', _rows)
for _name, _rows in zip(('btn_start_0', 'btn_start_1', 'btn_start_2'), pill('START', 3)):
    TILES[_name] = ('ui', _rows)
for _name, _rows in zip(('emblem_tl', 'emblem_tr', 'emblem_bl', 'emblem_br'), emblem()):
    TILES[_name] = ('ui', _rows)

# Glyph codes 0x80.. in this order; (tile, xflip, yflip).
CODES = [
    ('FRAME_TL', 'frame_tl', 0, 0), ('FRAME_TR', 'frame_tl', 1, 0),
    ('FRAME_BL', 'frame_tl', 0, 1), ('FRAME_BR', 'frame_tl', 1, 1),
    ('FRAME_T', 'frame_t', 0, 0), ('FRAME_B', 'frame_t', 0, 1),
    ('FRAME_L', 'frame_l', 0, 0), ('FRAME_R', 'frame_l', 1, 0),
    ('JOIN_L', 'frame_join', 0, 0), ('JOIN_R', 'frame_join', 1, 0),
    ('RULE', 'rule', 0, 0), ('RULE_DOT', 'rule_dot', 0, 0),
    ('CURSOR', 'cursor_0', 0, 0), ('CURSOR_ALT', 'cursor_1', 0, 0),
    ('RESUME', 'icon_resume', 0, 0), ('MAP', 'icon_map', 0, 0), ('JOBS', 'icon_jobs', 0, 0),
    ('PARK', 'icon_park', 0, 0), ('CAR', 'icon_car', 0, 0), ('TRANSIT', 'icon_transit', 0, 0),
    ('SAVE', 'icon_save', 0, 0), ('CANCEL', 'icon_cancel', 0, 0), ('AUDIO', 'icon_audio', 0, 0),
    ('COIN', 'icon_coin', 0, 0), ('BOX', 'icon_box', 0, 0), ('CLOCK', 'icon_clock', 0, 0), ('CLOCK_ALERT', 'icon_clock_alert', 0, 0),
    ('HEART', 'icon_heart', 0, 0), ('PIN', 'icon_pin', 0, 0), ('WALK', 'icon_walk', 0, 0),
    ('STREET', 'icon_street', 0, 0), ('TARGET', 'icon_target', 0, 0), ('STAR', 'icon_star', 0, 0),
    ('CHECK', 'icon_check', 0, 0), ('FERRY', 'icon_ferry', 0, 0), ('BUS', 'icon_bus', 0, 0),
    ('SUBWAY', 'icon_subway', 0, 0),
    ('ARROW_N', 'arrow_n', 0, 0), ('ARROW_S', 'arrow_n', 0, 1),
    ('ARROW_E', 'arrow_e', 0, 0), ('ARROW_W', 'arrow_e', 1, 0),
    ('ARROW_NE', 'arrow_ne', 0, 0), ('ARROW_NW', 'arrow_ne', 1, 0),
    ('ARROW_SE', 'arrow_ne', 0, 1), ('ARROW_SW', 'arrow_ne', 1, 1),
    ('BTN_A', 'btn_a', 0, 0), ('BTN_B', 'btn_b', 0, 0),
    ('BTN_SEL_0', 'btn_sel_0', 0, 0), ('BTN_SEL_1', 'btn_sel_1', 0, 0),
    ('BTN_START_0', 'btn_start_0', 0, 0), ('BTN_START_1', 'btn_start_1', 0, 0), ('BTN_START_2', 'btn_start_2', 0, 0),
    ('DPAD', 'dpad', 0, 0),
    ('EMBLEM_TL', 'emblem_tl', 0, 0), ('EMBLEM_TR', 'emblem_tr', 0, 0),
    ('EMBLEM_BL', 'emblem_bl', 0, 0), ('EMBLEM_BR', 'emblem_br', 0, 0),
    ('MEDIC', 'icon_medic', 0, 0), ('AMMO', 'icon_ammo', 0, 0),
    ('SUN', 'icon_sun', 0, 0), ('MOON', 'icon_moon', 0, 0),
]


# Original bold UI font: 2-pixel stems, 6 pixels wide (M and W use 7), seven
# rows tall with a blank eighth row. Drawn from column 1 so every glyph keeps a
# clear pixel from the icon or frame on its left.
FONT_CHARS = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/.-+?<>$%#=,'!()"
FONT = {
    ' ': ["......"] * 7,
    'A': [".####.", "##..##", "##..##", "######", "##..##", "##..##", "##..##"],
    'B': ["#####.", "##..##", "##..##", "#####.", "##..##", "##..##", "#####."],
    'C': [".####.", "##..##", "##....", "##....", "##....", "##..##", ".####."],
    'D': ["####..", "##.##.", "##..##", "##..##", "##..##", "##.##.", "####.."],
    'E': ["######", "##....", "##....", "#####.", "##....", "##....", "######"],
    'F': ["######", "##....", "##....", "#####.", "##....", "##....", "##...."],
    'G': [".####.", "##..##", "##....", "##.###", "##..##", "##..##", ".#####"],
    'H': ["##..##", "##..##", "##..##", "######", "##..##", "##..##", "##..##"],
    'I': ["######", "..##..", "..##..", "..##..", "..##..", "..##..", "######"],
    'J': ["...###", "....##", "....##", "....##", "##..##", "##..##", ".####."],
    'K': ["##..##", "##.##.", "####..", "###...", "####..", "##.##.", "##..##"],
    'L': ["##....", "##....", "##....", "##....", "##....", "##....", "######"],
    'M': ["##...##", "###.###", "#######", "##.#.##", "##...##", "##...##", "##...##"],
    'N': ["##..##", "###.##", "######", "##.###", "##..##", "##..##", "##..##"],
    'O': [".####.", "##..##", "##..##", "##..##", "##..##", "##..##", ".####."],
    'P': ["#####.", "##..##", "##..##", "#####.", "##....", "##....", "##...."],
    'Q': [".####.", "##..##", "##..##", "##..##", "##.###", "##..##", ".###.#"],
    'R': ["#####.", "##..##", "##..##", "#####.", "####..", "##.##.", "##..##"],
    'S': [".####.", "##..##", "##....", ".####.", "....##", "##..##", ".####."],
    'T': ["######", "..##..", "..##..", "..##..", "..##..", "..##..", "..##.."],
    'U': ["##..##", "##..##", "##..##", "##..##", "##..##", "##..##", ".####."],
    'V': ["##..##", "##..##", "##..##", "##..##", "##..##", ".####.", "..##.."],
    'W': ["##...##", "##...##", "##...##", "##.#.##", "#######", "###.###", "##...##"],
    'X': ["##..##", "##..##", ".####.", "..##..", ".####.", "##..##", "##..##"],
    'Y': ["##..##", "##..##", "##..##", ".####.", "..##..", "..##..", "..##.."],
    'Z': ["######", "....##", "...##.", "..##..", ".##...", "##....", "######"],
    '0': [".####.", "##..##", "##.###", "######", "###.##", "##..##", ".####."],
    '1': ["..##..", ".###..", "..##..", "..##..", "..##..", "..##..", ".####."],
    '2': [".####.", "##..##", "....##", "..###.", ".##...", "##....", "######"],
    '3': [".####.", "##..##", "....##", "..###.", "....##", "##..##", ".####."],
    '4': ["...##.", "..###.", ".####.", "##.##.", "######", "...##.", "...##."],
    '5': ["######", "##....", "#####.", "....##", "....##", "##..##", ".####."],
    '6': [".####.", "##....", "##....", "#####.", "##..##", "##..##", ".####."],
    '7': ["######", "....##", "...##.", "..##..", "..##..", "..##..", "..##.."],
    '8': [".####.", "##..##", "##..##", ".####.", "##..##", "##..##", ".####."],
    '9': [".####.", "##..##", "##..##", ".#####", "....##", "....##", ".####."],
    ':': ["......", "..##..", "..##..", "......", "..##..", "..##..", "......"],
    '/': ["....##", "....##", "...##.", "..##..", ".##...", "##....", "##...."],
    '.': ["......", "......", "......", "......", "......", "..##..", "..##.."],
    '-': ["......", "......", "......", ".####.", "......", "......", "......"],
    '+': ["......", "..##..", "..##..", "######", "..##..", "..##..", "......"],
    '?': [".####.", "##..##", "....##", "...##.", "..##..", "......", "..##.."],
    '<': ["....##", "...##.", "..##..", ".##...", "..##..", "...##.", "....##"],
    '>': ["##....", ".##...", "..##..", "...##.", "..##..", ".##...", "##...."],
    '$': ["..##..", ".#####", "##.#..", ".####.", "..#.##", "#####.", "..##.."],
    '%': ["##..##", "##.##.", "...##.", "..##..", ".##...", ".##.##", "##..##"],
    '#': [".#..#.", "######", ".#..#.", ".#..#.", "######", ".#..#.", "......"],
    '=': ["......", "......", "######", "......", "######", "......", "......"],
    ',': ["......", "......", "......", "......", "..##..", "..##..", ".##..."],
    "'": ["..##..", "..##..", ".##...", "......", "......", "......", "......"],
    '!': ["..##..", "..##..", "..##..", "..##..", "..##..", "......", "..##.."],
    '(': ["...##.", "..##..", ".##...", ".##...", ".##...", "..##..", "...##."],
    ')': [".##...", "..##..", "...##.", "...##.", "...##.", "..##..", ".##...."[:6]],
}


def font_rows(ch):
    """8x8 rows ('#' ink) for a font character."""
    rows = FONT[ch]
    return [('.' + r).ljust(8, '.')[:8] for r in rows] + ['........']


# ---- Title illustration (rows 0..8 of the title screen, 160 x 72) ----
LOGO = {
    'T': ["XXXXXXXXXXXX", "XXXXXXXXXXXX", "....XXXX....", "....XXXX....", "....XXXX....", "....XXXX....",
          "....XXXX....", "....XXXX....", "....XXXX....", "....XXXX....", "....XXXX....", "....XXXX...."],
    'O': ["..XXXXXXXX..", ".XXXXXXXXXX.", "XXXX....XXXX", "XXX......XXX", "XXX......XXX", "XXX......XXX",
          "XXX......XXX", "XXX......XXX", "XXX......XXX", "XXXX....XXXX", ".XXXXXXXXXX.", "..XXXXXXXX.."],
    'R': ["XXXXXXXXXX..", "XXXXXXXXXXX.", "XXX.....XXXX", "XXX......XXX", "XXX.....XXXX", "XXXXXXXXXXX.",
          "XXXXXXXXXX..", "XXX...XXXX..", "XXX....XXXX.", "XXX.....XXX.", "XXX.....XXXX", "XXX......XXX"],
    'N': ["XXX......XXX", "XXXX.....XXX", "XXXXX....XXX", "XXXXXX...XXX", "XXX.XXX..XXX", "XXX..XXX.XXX",
          "XXX...XXXXXX", "XXX....XXXXX", "XXX.....XXXX", "XXX......XXX", "XXX......XXX", "XXX......XXX"],
    'D': ["XXXXXXXXX...", "XXXXXXXXXX..", "XXX....XXXX.", "XXX.....XXX.", "XXX......XXX", "XXX......XXX",
          "XXX......XXX", "XXX......XXX", "XXX.....XXX.", "XXX....XXXX.", "XXXXXXXXXX..", "XXXXXXXXX..."],
    'I': [".XXXXXXXXXX.", ".XXXXXXXXXX.", "....XXXX....", "....XXXX....", "....XXXX....", "....XXXX....",
          "....XXXX....", "....XXXX....", "....XXXX....", "....XXXX....", ".XXXXXXXXXX.", ".XXXXXXXXXX."],
    'S': ["..XXXXXXXXX.", ".XXXXXXXXXXX", "XXXX........", "XXX.........", "XXXX........", ".XXXXXXXXX..",
          "..XXXXXXXXX.", "........XXXX", ".........XXX", "........XXXX", "XXXXXXXXXXX.", ".XXXXXXXXX.."],
    'P': ["XXXXXXXXXX..", "XXXXXXXXXXX.", "XXX.....XXXX", "XXX......XXX", "XXX.....XXXX", "XXXXXXXXXXX.",
          "XXXXXXXXXX..", "XXX.........", "XXX.........", "XXX.........", "XXX.........", "XXX........."],
    'A': ["...XXXXXX...", "..XXXXXXXX..", ".XXXX..XXXX.", "XXXX....XXXX", "XXX......XXX", "XXX......XXX",
          "XXXXXXXXXXXX", "XXXXXXXXXXXX", "XXX......XXX", "XXX......XXX", "XXX......XXX", "XXX......XXX"],
    'C': ["..XXXXXXXXX.", ".XXXXXXXXXXX", "XXXX.....XXX", "XXX.........", "XXX.........", "XXX.........",
          "XXX.........", "XXX.........", "XXX.........", "XXXX.....XXX", ".XXXXXXXXXXX", "..XXXXXXXXX."],
    'H': ["XXX......XXX", "XXX......XXX", "XXX......XXX", "XXX......XXX", "XXX......XXX", "XXXXXXXXXXXX",
          "XXXXXXXXXXXX", "XXX......XXX", "XXX......XXX", "XXX......XXX", "XXX......XXX", "XXX......XXX"],
}
TITLE_ROWS = 11
# Title palettes: slots 0..6 are swapped in while the title is shown (the
# scene's own palettes come back afterwards); slot 7 is the UI palette, also
# used by the lake, the shoreline and the controls card. Index 0 of each
# palette is the sky colour of its band.
INK, CREAM, GOLD, SUN = '172B38', 'FFF1D0', 'F9B544', 'FFE69A'
SKY = ['1E2453', '2B2B67', '453279', '6C3A84', '9B4683', 'C95876', 'EB7A5F']
TITLE_PALETTES = [
    [SKY[0], CREAM, GOLD, INK],   # rows 0-1: night sky, stars, TORONTO
    [SKY[1], CREAM, GOLD, INK],   # row 2
    [SKY[2], CREAM, GOLD, INK],   # row 3: DISPATCH
    [SKY[3], CREAM, GOLD, INK],   # row 4
    [SKY[4], SUN, GOLD, INK],     # row 5: towers and the setting sun
    [SKY[5], SUN, GOLD, INK],     # row 6
    [SKY[6], SUN, GOLD, INK],     # row 7
    list(UI_COLORS),              # rows 8-10: lake and shoreline (paper, amber, red, ink)
]
TITLE_ROW_PALETTE = [0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 7]
TITLE_SKY_ROW = [0, 0, 1, 2, 3, 4, 5, 6]


def title_image():
    """160x88 grid of RGB hex colours: Toronto skyline at dusk from the lake,
    the CN Tower, a setting sun and the courier car on the shore road."""
    W, H = 160, 88
    PAPER, AMBER, RED = UI_COLORS[0], UI_COLORS[1], UI_COLORS[2]
    g = [[SKY[TITLE_SKY_ROW[y // 8]] if y < 64 else INK for _ in range(W)] for y in range(H)]

    def put(x, y, c):
        if 0 <= x < W and 0 <= y < H:
            g[y][x] = c

    def sky(y):
        return SKY[TITLE_SKY_ROW[y // 8]]
    # Stars in the upper sky.
    for x, y in ((36, 2), (61, 1), (97, 3), (128, 1), (151, 4), (23, 9), (84, 18), (143, 19), (6, 33), (153, 36),
                 (118, 17), (48, 19), (70, 37), (27, 27)):
        put(x, y, CREAM)
    put(152, 4, CREAM); put(151, 3, CREAM); put(150, 4, CREAM); put(151, 5, CREAM)   # one twinkling star
    # Setting sun behind the skyline, cut by retro bands near the horizon.
    cx, cy, r = 112, 60, 17
    for y in range(40, 64):
        for x in range(cx - r, cx + r + 1):
            d2 = (x - cx) ** 2 + (y - cy) ** 2
            if d2 <= r * r:
                if y in (51, 55, 56, 59, 60, 61):
                    continue
                put(x, y, GOLD if d2 > (r - 2) ** 2 or y > 57 else SUN)
    # Skyline silhouettes (x0, x1, top) with lit windows.
    towers = [(42, 49, 50), (50, 57, 45), (58, 63, 52), (64, 73, 42), (74, 79, 49), (80, 86, 46),
              (88, 92, 54), (93, 98, 57), (126, 131, 55), (132, 141, 44), (142, 147, 50), (148, 155, 41), (156, 159, 47)]
    for x0, x1, top in towers:
        for x in range(x0, x1 + 1):
            for y in range(top, 64):
                put(x, y, INK)
        for y in range(top + 2, 62, 3):
            for x in range(x0 + 1, x1, 2):
                if (x * 5 + y * 3 + x0) % 7 < 3:
                    put(x, y, GOLD if (x + y) % 3 else SUN)
        if x1 - x0 >= 8:
            put((x0 + x1) // 2, top - 1, INK); put((x0 + x1) // 2, top - 2, GOLD)   # rooftop beacon
    # Waterfront lights along the foot of the skyline.
    for x in range(20, W):
        if g[62][x] == INK and x % 4 != 1:
            put(x, 63, GOLD if x % 4 else SUN)
    # Rogers Centre dome beside the tower.
    for x in range(20, 42):
        h = int(10 * (1 - ((x - 31) / 11.5) ** 2) ** 0.5)
        for y in range(64 - 3 - h, 64):
            put(x, y, INK)
        if 2 < h:
            put(x, 64 - 3 - h, GOLD if x % 3 else SUN)
    # CN Tower (centre x 13): antenna, SkyPod, main pod with lit windows,
    # tapering concrete shaft and a red-free night look.
    tx = 13
    for y in range(2, 64):
        if y < 14:
            xs = [tx]
        elif y < 17:
            xs = range(tx - 1, tx + 2)
        elif y < 21:
            xs = [tx]
        elif y < 23:
            xs = range(tx - 3, tx + 4)
        elif y < 27:
            xs = range(tx - 4, tx + 5)
        elif y < 29:
            xs = range(tx - 2, tx + 3)
        else:
            w = 1 + (y - 29) // 12
            xs = range(tx - w, tx + w + 1)
        for x in xs:
            put(x, y, INK)
    put(tx, 1, GOLD)                                           # beacon
    put(tx, 15, CREAM)                                         # SkyPod window
    for x in range(tx - 3, tx + 4, 2):
        put(x, 24, GOLD if x != tx else SUN)                  # restaurant ring
    for x in range(tx - 2, tx + 3, 2):
        put(x, 25, CREAM)
    # Logo: TORONTO over DISPATCH in cream and gold with an ink outline and
    # drop shadow, clear of the tower.
    def letter(ch, X, Y):
        rows = LOGO[ch]
        fill = {(x, y) for y, r in enumerate(rows) for x, c in enumerate(r) if c == 'X'}
        out = {(x + dx, y + dy) for x, y in fill for dx in (-1, 0, 1) for dy in (-1, 0, 1)}
        for x, y in out:
            put(X + x + 1, Y + y + 1, INK)      # shadow
            put(X + x, Y + y, INK)              # outline
        for x, y in fill:
            put(X + x, Y + y, CREAM if y < 6 else GOLD)
    for k, ch in enumerate("TORONTO"):
        letter(ch, 38 + 16 * k, 3)
    for k, ch in enumerate("DISPATCH"):
        letter(ch, 30 + 16 * k, 21)
    # Lake: the sun's reflection breaks into amber and red dashes; city
    # lights shimmer under the towers.
    for k, y in enumerate(range(65, 80, 2)):
        half = 15 - 2 * k
        for x in range(cx - half, cx + half + 1):
            if (x - cx + 64) % (6 + k) < 4 + k // 2:
                put(x + (k % 2), y, AMBER if k < 4 else RED)
    for x0, x1, top in towers:
        for x in range(x0 + 1, x1, 3):
            for y in range(65 + (x % 2), 78, 4):
                if abs(x - cx) > 16 and (x * 7 + y) % 5 < 2:
                    put(x, y, AMBER)
    for x, y in ((8, 70), (30, 67), (57, 74), (150, 66), (139, 75), (20, 77), (86, 69)):
        put(x, y, PAPER)
    # Shore road: promenade kerb, lane marks and the courier's orange car.
    for x in range(W):
        put(x, 80, PAPER)
        if (x // 4) % 3 == 0:
            put(x, 84, PAPER)
    car = ["..aaaaaa....",
           ".aa..#a.aa..",
           "aaaa.#aaaaar",
           "aaaaaaaaaaar",
           ".##.....##.."]
    cmap = {'a': AMBER, 'r': RED, '.': None, '#': INK}
    for y, row in enumerate(car):
        for x, c in enumerate(row):
            col = cmap[c]
            if col:
                put(46 + x, 82 + y, col)
    for x, y in ((48, 83), (49, 83), (53, 83), (54, 83)):
        put(x, y, PAPER)                                       # windows
    for x, y in ((59, 84), (60, 84), (61, 84), (62, 85), (63, 85), (60, 85), (61, 85)):
        put(x, y, PAPER)                                       # headlamp beam
    return g
