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
TITLE_ROWS = 9
COLOURS = {
    'paper': 'E7DECC', 'ink': '172B38', 'slate': '526879', 'blue': '8BBAD4', 'teal': '79AFAC',
    'amber': 'E89A3C', 'red': 'C83C34', 'glass': '9FC9D0',
}
# Which named colours each background slot provides at indices 0..3.
SLOT_COLOURS = {0: ['paper', 'teal', 'slate', 'ink'], 2: ['paper', 'blue', 'slate', 'ink'],
                5: ['paper', 'glass', 'slate', 'ink'], 7: ['paper', 'amber', 'red', 'ink']}


def title_image():
    """160x72 grid of colour names: dusk sky, CN Tower, logo, skyline, lake."""
    W, H = 160, 72
    g = [['blue'] * W for _ in range(H)]

    def put(x, y, c):
        if 0 <= x < W and 0 <= y < H:
            g[y][x] = c
    # Sunset band below the blue sky: cream glow, amber, then red at the horizon.
    for y in range(40, 64):
        for x in range(W):
            if y < 44:
                c = 'paper' if (x + y) % 2 == 0 or y >= 42 else 'amber'
            elif y < 46:
                c = 'paper' if (x + y) % 2 else 'amber'
            elif y < 56:
                c = 'amber'
            elif y < 60:
                c = 'amber' if (x + y) % 2 else 'red'
            else:
                c = 'red'
            g[y][x] = c
    # A few stars high in the sky.
    for x, y in ((50, 3), (118, 6), (150, 2), (88, 4), (138, 33), (60, 36)):
        put(x, y, 'paper')
    # Skyline silhouettes (x0, x1, top) with lit windows.
    buildings = [(26, 41, 54), (42, 49, 47), (50, 57, 51), (58, 69, 44), (70, 75, 50), (76, 87, 41),
                 (88, 93, 49), (94, 105, 45), (106, 111, 52), (112, 123, 43), (124, 129, 48),
                 (130, 141, 46), (142, 149, 53), (150, 159, 49)]
    for x0, x1, top in buildings:
        for x in range(x0, x1 + 1):
            for y in range(max(top, 40), 64):
                g[y][x] = 'ink'
        for y in range(max(top, 40) + 3, 62, 3):
            for x in range(x0 + 2, x1 - 1, 3):
                if (x * 7 + y * 3) % 5 < 3:
                    put(x, y, 'amber')
        put((x0 + x1) // 2, max(top, 40) - 1, 'red')
    # Rogers Centre dome beside the tower.
    for x in range(24, 42):
        d = abs(x - 33)
        for y in range(52 + d // 3, 64):
            g[y][x] = 'ink'
    # CN Tower: antenna, SkyPod, main pod, tapering shaft (x centre 15).
    cx = 15

    def tower_px(y):
        if y < 2:
            return []
        if y < 14:
            return [cx]                                    # antenna
        if y < 17:
            return list(range(cx - 2, cx + 3))             # SkyPod
        if y < 22:
            return list(range(cx - 1, cx + 2))
        if y < 28:
            return list(range(cx - 4, cx + 5))             # main pod
        w = 1 + (y - 28) // 9
        return list(range(cx - w, cx + w + 1))
    for y in range(H):
        for x in tower_px(y):
            put(x, y, 'slate' if y < 40 else 'ink')
        if 22 <= y < 28 and y in (24, 25):
            for x in (cx - 3, cx - 1, cx + 1, cx + 3):
                put(x, y, 'paper' if y < 40 else 'amber')
        if y < 40 and tower_px(y):
            put(tower_px(y)[0] - 1, y, 'ink')
            put(tower_px(y)[-1] + 1, y, 'ink')
    put(cx, 1, 'ink')
    # Lake: teal with slate ripples and light reflections.
    for y in range(64, 72):
        for x in range(W):
            c = 'teal'
            if (x // 3 + y * 5) % 11 == 0:
                c = 'slate'
            if (x % 12 in (4, 5)) and y in (65, 67, 69) and x > 24:
                c = 'paper'
            g[y][x] = c
    # Logo: "TORONTO" on rows 1-2, "DISPATCH" on rows 3-4.
    def letter(ch, X, Y):
        rows = LOGO[ch]
        fill = {(x, y) for y, r in enumerate(rows) for x, c in enumerate(r) if c == 'X'}
        out = {(x + dx, y + dy) for x, y in fill for dx in (-1, 0, 1) for dy in (-1, 0, 1)}
        for x, y in out:
            put(X + 1 + x + 1, Y + 1 + y + 1, 'slate')
        for x, y in out:
            put(X + 1 + x, Y + 1 + y, 'ink')
        for x, y in fill:
            put(X + 1 + x, Y + 1 + y, 'paper')
    for k, ch in enumerate("TORONTO"):
        letter(ch, 32 + 16 * k, 8)
    for k, ch in enumerate("DISPATCH"):
        letter(ch, 24 + 16 * k, 24)
    return g
