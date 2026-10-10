"""Greenery for the district painters: tree species, bushes, hedges, fences,
planters and lawn patches. Original pixel art.

Everything here is decoration on finished district art (outer_art.py and
create_city_art.py call dress() after the water). It only replaces whole
tiles of plain lawn, or the four tiles of a standard round tree on lawn, so
collision never changes: cars and walkers still cross every lawn (user
direction, 2026-10-07) and trees keep their canopy priority.

Two palettes carry the greens: slot 6 (parks: cream, lawn, dark green, ink)
and slot 5 (gardens: pale lime, the same lawn, yellow-green, ink). One tile
design drawn in either slot gives two plants for one tile pattern. Designs
are mirror-symmetric where they can be, so the scene's flip-canonical tile
count grows by about one pattern per design. dress() keeps every scene within
city_kit.SCENE_TILE_BUDGET by dropping the least important kinds of greenery
in a scene that would exceed it.

Design rows use the source shades: '.' or '2' lawn, '0' ink, '1' deep (dark
green or yellow-green), '3' light (cream or pale lime).
"""
import hashlib
from PIL import Image, ImageDraw
import city_kit

GARDEN, PARK = city_kit.GARDEN_SLOT, city_kit.PARK_SLOT


def _mirror(half):
    """16-wide rows from their left halves."""
    return [row + row[::-1] for row in half]


# 16 x 16 trees lit from the north: round canopies seen from above, and a
# tiered spruce drawn like the buildings' faces, with a little of its side.
TREES = {
    'maple': _mirror([
        "......00", "...00011", "..011112", ".0111232", ".0112322", "01112221", "01211121", "01121111",
        "01111211", "01211111", ".0111121", ".0011111", "..001111", "...00011", "....0000", "........"]),
    'linden': _mirror([
        ".....000", "...00111", "..011133", ".0113331", ".0113111", "01133113", "01111131", "01131111",
        "01111111", "01113111", ".0111111", ".0111111", "..011111", "...00111", ".....000", "........"]),
    'spruce': _mirror([
        ".......0", "......01", ".....011", "....0112", "...01121", "..011211", "...00111", "...01112",
        "..011121", ".0111211", "01112111", "..000111", "..011112", ".0111121", "01112111", ".0000000"]),
    'blossom': _mirror([
        "......00", "....0033", "..003333", ".0333233", ".0332333", "03323332", "03333233", "03233333",
        "03332323", "02333332", ".0232323", ".0022222", "..001212", "...00011", "....0000", "........"]),
    'willow': _mirror([
        "........", "....0001", "..001111", ".0111131", ".0113111", "01131111", "01111131", "11311111",
        "1.111311", "1.1.1111", "1.1.1.11", "1.1.1.1.", "0.1.1.1.", "..0.1.0.", "....0...", "........"]),
}

# 8 x 8 tiles.
TILES = {
    'bush': ["........", "...11...", "..1331..", ".112211.", ".121121.", ".111111.", "..0000..", "........"],
    'bloom': ["........", "...13...", "..1311..", ".131131.", ".113111.", ".131311.", "..0000..", "........"],
    # Hedges: a run along a sidewalk, capped at each end (east cap = flipped west cap).
    'hedge_h': ["........", "11111111", "21122112", "11211121", "11111111", "11111111", "00000000", "........"],
    'hedge_h_cap': ["........", "..111111", ".2112211", "11211121", "11111111", ".1111111", "..000000", "........"],
    'hedge_v': [".112110.", ".211210.", ".121110.", ".112210.", ".211110.", ".121210.", ".112110.", ".211110."],
    'hedge_v_cap': ["........", "..1111..", ".122110.", ".211210.", ".121110.", ".112210.", ".211110.", ".121210."],
    'hedge_v_end': [".112110.", ".211210.", ".121110.", ".112210.", ".111110.", "..11100.", "..0000..", "........"],
    # Fences: white pickets for front yards, iron railings round parks.
    'picket_h': ["........", "3.3.3.3.", "33333333", "3.3.3.3.", "3.3.3.3.", "33333333", "0.0.0.0.", "........"],
    'picket_v': ["..3.....", "..30....", "..33....", "..30....", "..3.....", "..30....", "..33....", "..30...."],
    'iron_h': ["........", "........", "0...0...", "00000000", "0...0...", "0...0...", "00000000", "........"],
    'iron_v': ["...0....", "..000...", "...0....", "...0....", "...0....", "..000...", "...0....", "...0...."],
    # A raised flower bed filling its tile (no lawn), so any palette colours
    # it: marigolds (terracotta), petunias (pink), daisies (khaki).
    'planter': ["00000000", "03233230", "02323320", "03212130", "02121210", "01212120", "01111110", "00000000"],
    # Lawn: clover and sun (gardens palette), long grass, wildflowers.
    'clover': ["3...3...", ".3.313..", "..3.3..3", "3.....3.", ".3.3....", "..313.3.", ".3.3..3.", "3......3"],
    'clover_b': [".3...3..", "313.3...", ".3....3.", "...3.3.3", "3.3.....", ".313..3.", "..3..3.3", "3...3..."],
    'meadow': [".1......", "1.1..3..", "..1..1.1", ".....1..", "...1....", "..1.1.3.", "3...1...", "........"],
    'meadow_b': ["....1...", "...1.1..", ".3....1.", ".1......", "......1.", ".1...1.1", "1.1.....", "........"],
    'flowers': ["........", "..3.....", ".313..1.", "..1...3.", "......1.", ".3......", ".1...3..", "........"],
}

# Greenery kinds, least important first: a scene over its tile budget drops
# them in this order, tree species one at a time (a dropped species shows
# the plain round tree again).
DROP_ORDER = ('flowers', 'meadow', 'clover', 'planter', 'iron', 'picket', 'bush', 'hedge',
              'species_willow', 'species_blossom', 'species_spruce', 'species_linden', 'species_maple')

# Which species grow where (weights). Slot 5 draws a maple as a honey locust.
MIX = {
    'park': [('maple', PARK, 4), ('maple', GARDEN, 2), ('linden', GARDEN, 3), ('spruce', PARK, 3), ('blossom', PARK, 1)],
    'high_park': [('maple', PARK, 5), ('spruce', PARK, 4), ('linden', GARDEN, 2), ('maple', GARDEN, 1), ('blossom', PARK, 1)],
    'yard': [('maple', PARK, 4), ('linden', GARDEN, 3), ('maple', GARDEN, 2), ('spruce', PARK, 2), ('blossom', PARK, 1)],
    'shore': [('willow', GARDEN, 3), ('maple', PARK, 1), ('blossom', PARK, 1)],
}
PLANTER_SLOTS = (1, 3, 4)


def seed(*values):
    return int.from_bytes(hashlib.sha256(repr(values).encode()).digest()[:4], 'big')


def _noise(salt, x, y, cell):
    """Smooth value noise in 0..1 on a `cell`-pixel lattice."""
    gx, gy = x // cell, y // cell
    fx, fy = (x % cell) / cell, (y % cell) / cell
    fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)

    def v(a, b):
        return seed(salt, a, b) / 0xFFFFFFFF
    top = v(gx, gy) + (v(gx + 1, gy) - v(gx, gy)) * fx
    bottom = v(gx, gy + 1) + (v(gx + 1, gy + 1) - v(gx, gy + 1)) * fx
    return top + (bottom - top) * fy


class Painter:
    def __init__(self, img, colors):
        self.img = img
        self.rgb = [tuple(bytes.fromhex(c[1:])) for c in colors]
        self.cache = {}

    def tile(self, rows, flip=0):
        """An 8 x 8 image of design rows (flip: 1 mirror, 2 upside down)."""
        key = (tuple(rows), flip)
        if key not in self.cache:
            t = Image.new('RGB', (8, 8))
            t.putdata([self.rgb[2 if c == '.' else int(c)] for row in rows for c in row])
            if flip & 1:
                t = t.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
            if flip & 2:
                t = t.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
            self.cache[key] = t
        return self.cache[key]


def tree_quarters(name):
    rows = TREES[name]
    return [[row[x:x + 8] for row in rows[y:y + 8]] for y in (0, 8) for x in (0, 8)]


def reference_trees(colors):
    """Pixels of the painters' standard 16 x 16 round trees on plain lawn."""
    refs = set()
    for kind in ('kit0', 'kit1', 'park'):
        im = Image.new('RGB', (16, 16), colors[2])
        d = ImageDraw.Draw(im)
        if kind == 'park':
            d.ellipse((0, 0, 15, 15), fill=colors[1], outline=colors[0])
            d.rectangle((4, 4, 11, 11), fill=colors[2])
        else:
            city_kit.tree(d, 0, 0, colors, int(kind[-1]))
        refs.add(im.tobytes())
    return refs


def dress(img, attrs, collisions, tw, canopies, colors, salt, old_id, kind_at, park_at, wet):
    """Add greenery to a finished district (see the module notes).

    kind_at(x, y): 'park', 'houses', 'shops', 'towers', 'works' or None.
    park_at(x, y): the park's name or None. wet(x, y): open water.
    Returns {kind: tiles} for the metadata."""
    th = len(attrs) // tw
    assert not any(a & 7 == GARDEN for a in attrs), 'palette slot 5 is reserved for greenery'
    paint = Painter(img, colors)
    lawn_rgb, tuft_rgb = paint.rgb[2], paint.rgb[1]
    cream = paint.rgb[3]
    orig = {}
    placed = []          # (kind, tile index)

    def pixels(tx, ty):
        return img.crop((tx * 8, ty * 8, tx * 8 + 8, ty * 8 + 8)).tobytes()

    def is_lawn(tx, ty):
        if not (0 <= tx < tw and 0 <= ty < th):
            return False
        i = ty * tw + tx
        if attrs[i] != PARK or collisions[i] != 16:
            return False
        px = pixels(tx, ty)
        tufts = 0
        for k in range(0, 192, 3):
            p = (px[k], px[k + 1], px[k + 2])
            if p == tuft_rgb:
                tufts += 1
            elif p != lawn_rgb:
                return False
        return tufts <= 4

    lawn = {(tx, ty) for ty in range(th) for tx in range(tw) if is_lawn(tx, ty)}

    def pavement(tx, ty):
        if not (0 <= tx < tw and 0 <= ty < th):
            return False
        i = ty * tw + tx
        if collisions[i] != 16 or attrs[i] & 7 not in (0, city_kit.SIDEWALK_SLOT) or attrs[i] & 128:
            return False
        px = pixels(tx, ty)
        return sum((px[k], px[k + 1], px[k + 2]) == cream for k in range(0, 192, 3)) >= 32

    def stamp(kind, tx, ty, rows, slot, flip=0, priority=False):
        i = ty * tw + tx
        if i not in orig:
            orig[i] = (img.crop((tx * 8, ty * 8, tx * 8 + 8, ty * 8 + 8)), attrs[i])
        img.paste(paint.tile(rows, flip), (tx * 8, ty * 8))
        attrs[i] = slot | (128 if priority else 0)
        placed.append((kind, i))
        lawn.discard((tx, ty))

    def pick(options, s):
        total = sum(w for *_, w in options)
        s %= total
        for *choice, w in options:
            if s < w:
                return choice
            s -= w

    # Tree species: standard round trees on lawn take a species for where
    # they grow (shore, High Park, parks, yards).
    refs = reference_trees(colors)
    for x, y, w, h in canopies:
        if (w, h) != (16, 16) or x % 8 or y % 8:
            continue
        tx, ty = x // 8, y // 8
        if any(attrs[(ty + b) * tw + tx + a] != PARK | 128 for a in (0, 1) for b in (0, 1)):
            continue
        if img.crop((x, y, x + 16, y + 16)).tobytes() not in refs:
            continue
        park = park_at(x + 8, y + 8)
        near_water = any(wet(x + dx, y + dy) for dx in (-24, 8, 40) for dy in (-24, 8, 40)
                         if 0 <= x + dx < img.width and 0 <= y + dy < img.height)
        mix = 'shore' if near_water else 'high_park' if park and 'HIGH PARK' in park.upper() else 'park' if park else 'yard'
        name, slot = pick(MIX[mix], seed(salt, 'tree', x, y))
        for k, rows in enumerate(tree_quarters(name)):
            stamp('species_' + name, tx + k % 2, ty + k // 2, rows, slot, priority=True)

    # Sidewalk edges of lawns: front-yard hedges and white pickets, iron
    # railings round parks, flower beds by shops. A run is cut into
    # segments with a gap (a gate) between them.
    def runs(axis):
        out, seen = [], set()
        for ty in range(th):
            for tx in range(tw):
                if (tx, ty) in seen or (tx, ty) not in lawn:
                    continue
                for side in ((0, 1), (0, -1)) if axis == 'h' else ((1, 0), (-1, 0)):
                    if not pavement(tx + side[0], ty + side[1]):
                        continue
                    step = (1, 0) if axis == 'h' else (0, 1)
                    run = []
                    cx, cy = tx, ty
                    while (cx, cy) in lawn and (cx, cy) not in seen and pavement(cx + side[0], cy + side[1]):
                        run.append((cx, cy))
                        cx, cy = cx + step[0], cy + step[1]
                    seen.update(run)
                    if len(run) >= 3:
                        out.append((run, side))
                    break
        return out

    def treatment(run):
        x, y = run[len(run) // 2][0] * 8 + 4, run[len(run) // 2][1] * 8 + 4
        kind = kind_at(x, y)
        s = seed(salt, 'edge', x, y)
        if kind == 'park':
            return 'iron'
        if kind == 'houses':
            return ('hedge', 'picket', 'hedge')[s % 3]
        if kind == 'shops':
            return 'planter' if s % 3 == 0 else None
        if kind in ('towers', 'works'):
            return 'hedge' if s % 2 else None
        return 'hedge' if s % 3 == 0 else None

    for axis in ('h', 'v'):
        for run, side in runs(axis):
            kind = treatment(run)
            if not kind:
                continue
            s = seed(salt, 'gates', run[0])
            segments, k = [], 0
            while k < len(run):
                length = 4 + (s >> (k % 24)) % 6
                segments.append(run[k:k + length])
                k += length + 1
            slot = GARDEN if kind == 'hedge' and s % 3 == 0 else PARK
            for seg in segments:
                if kind == 'planter' and len(seg) > 3:
                    seg = seg[:3]
                for j, (tx, ty) in enumerate(seg):
                    first, last = j == 0, j == len(seg) - 1
                    if kind == 'hedge':
                        if axis == 'h':
                            rows, flip = (TILES['hedge_h_cap'], 0 if first else 1) if first or last else (TILES['hedge_h'], 0)
                            if len(seg) == 1:
                                rows, flip = TILES['bush'], 0
                        else:
                            rows = TILES['hedge_v_cap'] if first else TILES['hedge_v_end'] if last else TILES['hedge_v']
                            flip = 0
                        stamp('hedge', tx, ty, rows, slot, flip)
                    elif kind == 'picket':
                        stamp('picket', tx, ty, TILES['picket_h' if axis == 'h' else 'picket_v'], PARK,
                              1 if axis == 'v' and side[0] < 0 else 0)
                    elif kind == 'iron':
                        stamp('iron', tx, ty, TILES['iron_h' if axis == 'h' else 'iron_v'], PARK)
                    else:
                        stamp('planter', tx, ty, TILES['planter'], PLANTER_SLOTS[seed(salt, 'bed', tx // 4, ty) % 3])

    # Bushes: beside buildings (foundation planting) and along park paths.
    def building_side(tx, ty):
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = tx + dx, ty + dy
            if 0 <= nx < tw and 0 <= ny < th:
                a = attrs[ny * tw + nx] & 7
                if collisions[ny * tw + nx] != 0 and a not in (PARK, GARDEN) and not pavement(nx, ny):
                    return True
        return False

    def path_side(tx, ty):
        return any(pavement(tx + dx, ty + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
    for tx, ty in sorted(lawn):
        x, y = tx * 8 + 4, ty * 8 + 4
        s = seed(salt, 'bush', tx, ty)
        cluster = seed(salt, 'cluster', tx // 3, ty // 3) % 100
        if building_side(tx, ty):
            chance = 30 if cluster < 45 else 6
        elif park_at(x, y) and path_side(tx, ty):
            chance = 18 if cluster < 40 else 4
        else:
            continue
        if s % 100 >= chance:
            continue
        r = (s >> 8) % 10
        rows, slot = (TILES['bush'], PARK) if r < 5 else (TILES['bush'], GARDEN) if r < 8 else (TILES['bloom'], PARK)
        stamp('bush', tx, ty, rows, slot, (s >> 12) & 1)

    # Lawn patches: clover and sun in the gardens palette, long grass in
    # High Park and on wide wild ground, wildflowers in parks.
    for tx, ty in sorted(lawn):
        x, y = tx * 8 + 4, ty * 8 + 4
        s = seed(salt, 'lawn', tx, ty)
        park = park_at(x, y)
        flip = (s >> 4) & 3
        if park and 'HIGH PARK' in park.upper() and _noise((salt, 'meadow'), x, y, 56) > 0.58:
            stamp('meadow', tx, ty, TILES['meadow' if s & 1 else 'meadow_b'], GARDEN, flip)
        elif _noise((salt, 'clover'), x, y, 40) > 0.66:
            stamp('clover', tx, ty, TILES['clover' if s & 1 else 'clover_b'], GARDEN, flip)
        elif park and s % 100 < 4:
            stamp('flowers', tx, ty, TILES['flowers'], PARK, flip)

    # Keep each scene within its tile budget.
    import world2x
    dropped = []
    for q in range(4):
        ox, oy = world2x.scene_origin(old_id * 4 + q)
        box = (ox, oy, ox + world2x.SCENE_W, oy + world2x.SCENE_H)
        inside = lambda i: box[0] <= i % tw * 8 < box[2] and box[1] <= i // tw * 8 < box[3]
        for kind in DROP_ORDER:
            if city_kit.flip_canonical_count(img.crop(box)) <= city_kit.SCENE_TILE_BUDGET:
                break
            for k, i in [(k, i) for k, i in placed if k == kind and inside(i)]:
                tile, attr = orig[i]
                img.paste(tile, (i % tw * 8, i // tw * 8))
                attrs[i] = attr
            placed = [(k, i) for k, i in placed if not (k == kind and inside(i))]
            dropped.append((q, kind))
        assert city_kit.flip_canonical_count(img.crop(box)) <= city_kit.SCENE_TILE_BUDGET, (old_id, q)
    counts = {}
    for k, _ in placed:
        k = 'species' if k.startswith('species_') else k
        counts[k] = counts.get(k, 0) + 1
    return {'tiles': counts, 'dropped': [f'{world2x.scene_slug(old_id * 4 + q)}:{k}' for q, k in dropped]}


def preview(path, scale=6):
    """A contact sheet of every design in its palettes (true colours)."""
    import json
    from pathlib import Path
    root = Path(__file__).resolve().parents[1] / 'project/project/palettes'
    pal = {}
    for name, slot in (('toronto_architecture_6', PARK), ('toronto_architecture_5', GARDEN),
                       ('toronto_architecture_1', 1), ('toronto_architecture_3', 3), ('toronto_architecture_4', 4)):
        pal[slot] = ['#' + c for c in json.loads((root / f'{name}.gbsres').read_text())['colors']]
    designs = [(n, r) for n, r in TREES.items()] + [(n, r) for n, r in TILES.items()]
    cols = len(designs)
    sheet = Image.new('RGB', (cols * 20 * scale, 3 * 20 * scale), '#222')
    for row, slots in enumerate(((PARK,), (GARDEN,), (1, 3, 4))):
        for k, (name, rows) in enumerate(designs):
            slot = slots[k % len(slots)]
            # Source shade 3 is the palette's first colour (lightest).
            colors = [pal[slot][3], pal[slot][2], pal[slot][1], pal[slot][0]]
            w = len(rows[0])
            im = Image.new('RGB', (w, len(rows)))
            im.putdata([tuple(bytes.fromhex(colors[2 if c == '.' else int(c)][1:])) for r in rows for c in r])
            bg = Image.new('RGB', (20, 20), colors[2])
            bg.paste(im, (2, 2))
            sheet.paste(bg.resize((20 * scale, 20 * scale), Image.NEAREST), (k * 20 * scale, row * 20 * scale))
    sheet.save(path)
