"""Drawing kit for the building interiors (create_interiors.py).

A Room is painted in real colours (hex strings) on a pixel canvas. Each
8x8 tile is then given one of the scene's background palettes that holds
every colour the tile uses, and the colour becomes that palette's shade
index (lightest..darkest = 0..3) in the four-shade source PNG. Floor
pixels may be painted "soft": a soft pixel falls back to a second colour in
a tile whose palette lacks the first (grout lines vanish under furniture
instead of forcing a palette clash).

Walls follow the city's interior convention: a dark cap on the border, a
visible north wall face 2-3 tiles tall, floors seen from straight above,
furniture with a short front face. Collision is per tile (0 open, 15
solid); tiles marked overhead carry CGB background priority and must keep
every non-overhead pixel at shade 0 so the player shows through.
"""
from collections import deque
from contextlib import contextmanager

from PIL import Image

# ------------------------------------------------------------------ colours
CREAM = 'E7DECC'   # city paper: shade 0 of nearly every interior palette
INK = '172B38'     # city ink: shade 3 of nearly every interior palette
SLATE = '526879'   # city slate
SKY = '8BBAD4'     # city glass blue
TEAL = '79AFAC'    # city lake / stone teal
TERRA = 'D69E72'   # city terracotta / wood
SAND = 'D2C895'
ROSE = 'C5A9A1'
LIME = 'D2E89A'
GRASS = '8FB56A'
PINE = '4D7A52'

# Interior palette library: name -> (title, colours lightest..darkest).
# Most keep the city's paper at shade 0 and ink at shade 3, so objects of
# any palette can stand on a cream floor or hang on a cream wall.
PALETTES = {
    'plaster': ('Interior plaster and bone', [CREAM, 'CDBB9E', '8C7A66', INK]),
    'wood': ('Interior wood', [CREAM, TERRA, '8E5A3A', INK]),
    'glass': ('Interior glass', [CREAM, SKY, SLATE, INK]),
    'stone': ('Interior stone', [CREAM, 'BFC1BA', '7C8590', INK]),
    'brass': ('Interior brass', [CREAM, 'D8A83C', '8A6424', INK]),
    'red': ('Interior lantern red', [CREAM, 'D4483A', '7E2A2A', INK]),
    'leaf': ('Interior plants', [CREAM, GRASS, PINE, INK]),
    'screen': ('Interior screens', ['E8F8F0', '6CCCEC', '2C64B0', INK]),
    'teal': ('Interior lake teal', [CREAM, TEAL, SLATE, INK]),
    'jade': ('Interior jade', [CREAM, '84C8A0', '2E7258', INK]),
    'violet': ('Interior violet', [CREAM, 'A890CA', '5C4A8A', INK]),
    'sunset': ('Interior sunset', [CREAM, 'E68A48', '8C3A58', INK]),
    'night': ('Interior night', [CREAM, '5C6EB2', '2A3468', INK]),
    'rose': ('Interior rose', [CREAM, ROSE, '7A5A62', INK]),
    'sand': ('Interior sand', [CREAM, SAND, '8E7C4C', INK]),
    'mint': ('Interior diner mint', [CREAM, '9CD4C4', '3E8A80', INK]),
    'dusk': ('Interior dusk grey', [CREAM, '9AA2B4', '4A5468', INK]),
    'walnut': ('Interior walnut', ['D8A878', '9A6640', '5A3424', INK]),
    'green': ('Interior bottle green', [CREAM, '6EA078', '2E5A44', INK]),
    'lift': ('Interior lift glass and gold', [CREAM, SKY, 'D8A83C', INK]),
    'aerial': ('Interior city far below', [CREAM, 'BFC1BA', GRASS, SLATE]),
    'lake': ('Interior lake far below', [CREAM, TEAL, GRASS, SLATE]),
    'city_b': ('Interior city roofs', [CREAM, ROSE, SKY, TERRA]),
    'gardens': ('Interior gardens', [LIME, GRASS, '5F9A3E', INK]),
    'sprite_a': ('Interior sprite swatches A', ['F0C020', 'E87820', '3068C8', '2850B0']),
    'sprite_b': ('Interior sprite swatches B', ['F8C8A0', '309878', '8858B8', 'C83028']),
}

DMG = [(224, 248, 207), (134, 192, 108), (48, 104, 80), (7, 24, 33)]


def rgb(h):
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


# -------------------------------------------------------------------- fonts
# Original 3x5 capitals for small signs, boards and plaques.
MINI = {
    'A': ['.#.', '#.#', '###', '#.#', '#.#'], 'B': ['##.', '#.#', '##.', '#.#', '##.'],
    'C': ['.##', '#..', '#..', '#..', '.##'], 'D': ['##.', '#.#', '#.#', '#.#', '##.'],
    'E': ['###', '#..', '##.', '#..', '###'], 'F': ['###', '#..', '##.', '#..', '#..'],
    'G': ['.##', '#..', '#.#', '#.#', '.##'], 'H': ['#.#', '#.#', '###', '#.#', '#.#'],
    'I': ['###', '.#.', '.#.', '.#.', '###'], 'J': ['..#', '..#', '..#', '#.#', '.#.'],
    'K': ['#.#', '#.#', '##.', '#.#', '#.#'], 'L': ['#..', '#..', '#..', '#..', '###'],
    'M': ['#.#', '###', '###', '#.#', '#.#'], 'N': ['##.', '#.#', '#.#', '#.#', '#.#'],
    'O': ['.#.', '#.#', '#.#', '#.#', '.#.'], 'P': ['##.', '#.#', '##.', '#..', '#..'],
    'Q': ['.#.', '#.#', '#.#', '##.', '.##'], 'R': ['##.', '#.#', '##.', '#.#', '#.#'],
    'S': ['.##', '#..', '.#.', '..#', '##.'], 'T': ['###', '.#.', '.#.', '.#.', '.#.'],
    'U': ['#.#', '#.#', '#.#', '#.#', '###'], 'V': ['#.#', '#.#', '#.#', '#.#', '.#.'],
    'W': ['#.#', '#.#', '###', '###', '#.#'], 'X': ['#.#', '#.#', '.#.', '#.#', '#.#'],
    'Y': ['#.#', '#.#', '.#.', '.#.', '.#.'], 'Z': ['###', '..#', '.#.', '#..', '###'],
    '0': ['###', '#.#', '#.#', '#.#', '###'], '1': ['.#.', '##.', '.#.', '.#.', '###'],
    '2': ['##.', '..#', '.#.', '#..', '###'], '3': ['##.', '..#', '.#.', '..#', '##.'],
    '4': ['#.#', '#.#', '###', '..#', '..#'], '5': ['###', '#..', '##.', '..#', '##.'],
    '6': ['.##', '#..', '###', '#.#', '###'], '7': ['###', '..#', '.#.', '.#.', '.#.'],
    '8': ['###', '#.#', '###', '#.#', '###'], '9': ['###', '#.#', '###', '..#', '##.'],
    ':': ['.', '#', '.', '#', '.'], '.': ['.', '.', '.', '.', '#'], '-': ['...', '...', '###', '...', '...'],
    '/': ['..#', '..#', '.#.', '#..', '#..'], '>': ['#..', '.#.', '..#', '.#.', '#..'],
    '<': ['..#', '.#.', '#..', '.#.', '..#'], '+': ['...', '.#.', '###', '.#.', '...'],
    '$': ['.##', '##.', '.#.', '.##', '##.'], '!': ['#', '#', '#', '.', '#'], "'": ['#', '#', '.', '.', '.'],
    '&': ['.#.', '#.#', '.#.', '#.#', '.##'], '?': ['##.', '..#', '.#.', '...', '.#.'],
    ' ': ['..', '..', '..', '..', '..'],
}


def mini_width(text):
    return sum(len(MINI[ch][0]) + 1 for ch in text) - 1 if text else 0


def big_font():
    import ui_art
    return ui_art.FONT


# ---------------------------------------------------------------------- room
class Room:
    def __init__(self, key, tw, th, palettes, title='', kind='generic'):
        assert len(palettes) <= 7, key
        self.key, self.tw, self.th = key, tw, th
        self.title, self.kind = title, kind
        self.w, self.h = tw * 8, th * 8
        self.palette_names = list(palettes)
        self.pals = [PALETTES[p][1] for p in palettes]
        self.col = [[CREAM] * self.w for _ in range(self.h)]
        self.soft = [[None] * self.w for _ in range(self.h)]
        self.over = [[False] * self.w for _ in range(self.h)]
        self.solid = [[0] * tw for _ in range(th)]
        self.prio = [[False] * tw for _ in range(th)]
        self.points, self.npcs = [], []
        self.entrance = self.exit = None
        self.over_limit = None

    # ------------------------------------------------------------ overhead
    @contextmanager
    def overhead(self, below=None):
        """Context: everything painted inside (above pixel row below, if
        given) is overhead art with background priority."""
        self.over_limit = self.h if below is None else below
        try:
            yield
        finally:
            self.over_limit = None

    def _mark(self, x, y):
        if self.over_limit is not None and y < self.over_limit:
            self.over[y][x] = True
            self.prio[y // 8][x // 8] = True

    # ----------------------------------------------------------- painting
    def px(self, x, y, c, soft=None):
        if 0 <= x < self.w and 0 <= y < self.h and c is not None:
            self.col[y][x] = c
            self.soft[y][x] = soft
            self._mark(x, y)

    def rect(self, x, y, w, h, c, soft=None):
        for yy in range(max(0, y), min(self.h, y + h)):
            row, srow = self.col[yy], self.soft[yy]
            for xx in range(max(0, x), min(self.w, x + w)):
                row[xx] = c
                srow[xx] = soft
                self._mark(xx, yy)

    def hline(self, x, y, w, c):
        self.rect(x, y, w, 1, c)

    def vline(self, x, y, h, c):
        self.rect(x, y, 1, h, c)

    def frame(self, x, y, w, h, c):
        self.hline(x, y, w, c)
        self.hline(x, y + h - 1, w, c)
        self.vline(x, y, h, c)
        self.vline(x + w - 1, y, h, c)

    def fill(self, x, y, w, h, pattern, cmap, soft=None, ox=0, oy=0):
        """Tile a pattern (list of strings) over a rectangle, aligned to the
        canvas (plus an offset) so separate fills continue seamlessly.
        soft maps pattern chars to their fallback colour."""
        ph, pw = len(pattern), len(pattern[0])
        for yy in range(max(0, y), min(self.h, y + h)):
            prow = pattern[(yy - oy) % ph]
            for xx in range(max(0, x), min(self.w, x + w)):
                ch = prow[(xx - ox) % pw]
                c = cmap.get(ch)
                if c is None:
                    continue
                self.col[yy][xx] = c
                self.soft[yy][xx] = soft.get(ch) if soft else None
                self._mark(xx, yy)

    def stamp(self, x, y, rows, cmap):
        for j, row in enumerate(rows):
            for i, ch in enumerate(row):
                c = cmap.get(ch)
                if c is None:
                    continue
                xx, yy = x + i, y + j
                if 0 <= xx < self.w and 0 <= yy < self.h:
                    self.col[yy][xx] = c
                    self.soft[yy][xx] = None
                    self._mark(xx, yy)

    def text(self, x, y, s, c, font='mini'):
        """Draw text in the 3x5 sign font or the UI font ('big'); returns
        the width drawn."""
        glyphs = MINI if font == 'mini' else big_font()
        cx = x
        for ch in s:
            self.stamp(cx, y, glyphs[ch], {'#': c})
            cx += len(glyphs[ch][0]) + 1
        return cx - x - 1

    def text_center(self, cx, y, s, c, font='mini'):
        glyphs = MINI if font == 'mini' else big_font()
        w = sum(len(glyphs[ch][0]) + 1 for ch in s) - 1
        return self.text(cx - w // 2, y, s, c, font)

    # ---------------------------------------------------------- collision
    def block(self, tx, ty, tw=1, th=1, value=15):
        for y in range(ty, ty + th):
            for x in range(tx, tx + tw):
                if 0 <= x < self.tw and 0 <= y < self.th:
                    self.solid[y][x] = value

    # ----------------------------------------------------------- metadata
    def point(self, kind, tx, ty, text=None, dx=0):
        """A point of interest used standing on tile (tx, ty); dx shifts the
        feet position horizontally in pixels (8 = between two tiles)."""
        p = {'kind': kind, 'x': tx * 8 + 4 + dx, 'y': ty * 8 + 7}
        if text:
            p['text'] = text
        self.points.append(p)
        return p

    def npc(self, role, behave, tx, ty, face='s', area=None, path=None, dx=0):
        n = {'role': role, 'behave': behave, 'x': tx * 8 + 4 + dx, 'y': ty * 8 + 7, 'face': face}
        if area:
            ax, ay, aw, ah = area
            n['area'] = [ax * 8, ay * 8, (ax + aw) * 8 - 1, (ay + ah) * 8 - 1]
        if path:
            n['path'] = [[px * 8 + 4, py * 8 + 7] for px, py in path]
        self.npcs.append(n)
        return n

    # ------------------------------------------------------------ resolve
    def resolve(self):
        """Choose a palette per tile; returns (shade rows, attrs, problems)."""
        shades = [[0] * self.w for _ in range(self.h)]
        attrs, problems = [], []
        sets = [set(p) for p in self.pals]
        for ty in range(self.th):
            for tx in range(self.tw):
                cells = [(x, y) for y in range(ty * 8, ty * 8 + 8) for x in range(tx * 8, tx * 8 + 8)]
                hard = {self.col[y][x] for x, y in cells if self.soft[y][x] is None}
                soft = {(self.col[y][x], self.soft[y][x]) for x, y in cells if self.soft[y][x] is not None}
                # The first palette (scene order) holding every hard colour and
                # each soft colour or its fallback, preferring fewer fallbacks.
                best = None
                for i, have in enumerate(sets):
                    if not hard <= have or any(c not in have and s not in have for c, s in soft):
                        continue
                    misses = sum(c not in have for c, _ in soft)
                    if best is None or misses < best[1]:
                        best = (i, misses)
                if best is None:
                    problems.append((tx, ty, sorted(hard | {c for c, _ in soft})))
                    attrs.append(0)
                    continue
                i = best[0]
                pal = self.pals[i]
                for x, y in cells:
                    c = self.col[y][x]
                    shades[y][x] = pal.index(c if c in sets[i] else self.soft[y][x])
                prio = self.prio[ty][tx]
                if prio and any(not self.over[y][x] and shades[y][x] for x, y in cells):
                    problems.append((tx, ty, 'overhead tile hides the player'))
                attrs.append(i | (0x80 if prio else 0))
        return shades, attrs, problems

    def images(self):
        shades, attrs, problems = self.resolve()
        if problems:
            raise SystemExit(f'{self.key}: palette problems {problems[:12]} ({len(problems)} tiles)')
        src = Image.new('RGB', (self.w, self.h))
        src.putdata([DMG[s] for row in shades for s in row])
        colour = Image.new('RGB', (self.w, self.h))
        data = []
        for y in range(self.h):
            for x in range(self.w):
                pal = self.pals[attrs[(y // 8) * self.tw + x // 8] & 7]
                data.append(rgb(pal[shades[y][x]]))
        colour.putdata(data)
        return src, colour, attrs

    def collisions(self):
        return [self.solid[y][x] for y in range(self.th) for x in range(self.tw)]

    # --------------------------------------------------------- validation
    def reachable(self):
        """Open tiles reachable from the entrance tile."""
        ex, ey = self.entrance
        start = (ex // 8, ey // 8)
        seen, todo = {start}, deque([start])
        while todo:
            x, y = todo.popleft()
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if 0 <= nx < self.tw and 0 <= ny < self.th and (nx, ny) not in seen and not self.solid[ny][nx]:
                    seen.add((nx, ny))
                    todo.append((nx, ny))
        return seen
