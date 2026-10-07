"""Shared original street furniture and roof detail for the district painters.

Everything here is decoration on top of the authored road, sidewalk and
building geometry: it never changes a collision value. Colour arguments are
indices into the native 4-shade source palette used by every generator:
0 dark, 1 asphalt / deep tone, 2 ground / body, 3 light.
"""
import hashlib
import re

ROAD_HALF, WALK_HALF = 24, 32


def intersections_from_routes(routes):
    """Centres where a horizontal and a vertical road segment meet (any
    crossing, T or corner). Routes are lists of cardinal points."""
    horizontal, vertical = [], []
    for points in routes:
        for (x1, y1), (x2, y2) in zip(points, points[1:]):
            if y1 == y2 and x1 != x2:
                horizontal.append((min(x1, x2), max(x1, x2), y1))
            elif x1 == x2 and y1 != y2:
                vertical.append((min(y1, y2), max(y1, y2), x1))
    centres = set()
    for hx1, hx2, hy in horizontal:
        for vy1, vy2, vx in vertical:
            if hx1 - ROAD_HALF <= vx <= hx2 + ROAD_HALF and vy1 - ROAD_HALF <= hy <= vy2 + ROAD_HALF:
                centres.add((vx, hy))
    return sorted(centres)


def _arms(is_road, is_walk, u, v):
    """Arms of the crossing at (u,v) that have asphalt continuing beyond the
    box and a sidewalk on both sides of the crossing band: those are the only
    places a pedestrian actually crosses that road."""
    h, w = ROAD_HALF, WALK_HALF
    mid = (h + w) // 2
    arms = []
    checks = {
        'n': ((u, v - mid), (u, v - w - 8), (u - mid, v - mid), (u + mid - 1, v - mid)),
        's': ((u, v + mid - 1), (u, v + w + 7), (u - mid, v + mid - 1), (u + mid - 1, v + mid - 1)),
        'w': ((u - mid, v), (u - w - 8, v), (u - mid, v - mid), (u - mid, v + mid - 1)),
        'e': ((u + mid - 1, v), (u + w + 7, v), (u + mid - 1, v - mid), (u + mid - 1, v + mid - 1)),
    }
    for arm, (band, beyond, side_a, side_b) in checks.items():
        if is_road(*band) and is_road(*beyond) and is_walk(*side_a) and is_walk(*side_b):
            arms.append(arm)
    return arms


def paint_crosswalks(box, is_road, is_walk, centres, light=3):
    """Zebra bars across every valid arm: 4px bars on an 8px period so each
    crossing reuses two tile patterns. Returns painted crossing rectangles."""
    h, w = ROAD_HALF, WALK_HALF
    painted = []
    for u, v in centres:
        if not is_road(u, v):
            continue
        for arm in _arms(is_road, is_walk, u, v):
            if arm in ('n', 's'):
                top = v - w if arm == 'n' else v + h
                for x in range(u - h + 2, u + h, 8):
                    box(x, top + 1, 4, 6, light)
                painted.append((u - h, top, 2 * h, w - h))
            else:
                left = u - w if arm == 'w' else u + h
                for y in range(v - h + 2, v + h, 8):
                    box(left + 1, y, 6, 4, light)
                painted.append((left, v - h, w - h, 2 * h))
    return painted


def near_crossing(x, y, centres, margin=8):
    """True inside an intersection box widened by its crossings, where lane
    dashes would otherwise be drawn straight across the junction."""
    lim = WALK_HALF + margin
    return any(abs(x - u) < lim and abs(y - v) < lim for u, v in centres)


def seed_of(*values):
    return int(hashlib.sha256(repr(values).encode()).hexdigest()[:8], 16)


def roof_details(d, box, x, y, w, h, roof, seed, colors, aligned=False):
    """Varied rooftop equipment inside the existing roof outline. Picks one or
    two features from the seed: water tank, HVAC units, skylight strip, solar
    grid or a stair bulkhead. Never draws outside (x+5..x+w-6, y+5..).
    aligned: one of four features in a fixed 8x5 spot measured from the
    roof's top-right corner, so buildings of a style share tiles (the core
    scene, where the tile budget is tight)."""
    if aligned:
        if w < 32 or h - roof < 14:
            return
        l, t = x + w - 15, y + 6
        kind = seed % 4
        if kind == 0:      # water tank
            d.ellipse((l + 1, t, l + 6, t + 4), fill=colors[3], outline=colors[0])
        elif kind == 1:    # two HVAC units
            box(l, t, 3, 4, 3); box(l + 4, t, 3, 4, 3)
            d.rectangle((l, t, l + 2, t + 3), outline=colors[0]); d.rectangle((l + 4, t, l + 6, t + 3), outline=colors[0])
        elif kind == 2:    # skylight strip
            d.rectangle((l, t, l + 7, t + 4), outline=colors[0], fill=colors[3])
            for sx in (l + 2, l + 4, l + 6):
                d.line((sx, t + 1, sx, t + 3), fill=colors[0])
        else:              # stair bulkhead
            box(l + 1, t, 6, 5, 1); d.rectangle((l + 1, t, l + 6, t + 4), outline=colors[0]); box(l + 3, t + 4, 2, 1, 3)
        return
        l, t = x + w - 15, y + 6
        kind = seed % 6
        if kind == 0:      # water tank
            d.ellipse((l + 1, t, l + 6, t + 4), fill=colors[3], outline=colors[0])
        elif kind == 1:    # two HVAC units
            box(l, t, 3, 4, 3); box(l + 4, t, 3, 4, 3)
            d.rectangle((l, t, l + 2, t + 3), outline=colors[0]); d.rectangle((l + 4, t, l + 6, t + 3), outline=colors[0])
        elif kind == 2:    # skylight strip
            d.rectangle((l, t, l + 7, t + 4), outline=colors[0], fill=colors[3])
            for sx in (l + 2, l + 4, l + 6):
                d.line((sx, t + 1, sx, t + 3), fill=colors[0])
        elif kind == 3:    # solar panels
            d.rectangle((l, t, l + 7, t + 4), fill=colors[0])
            for sx in (l + 1, l + 3, l + 5):
                d.line((sx, t + 1, sx, t + 3), fill=colors[1])
        elif kind == 4:    # stair bulkhead
            box(l + 1, t, 6, 5, 1); d.rectangle((l + 1, t, l + 6, t + 4), outline=colors[0]); box(l + 3, t + 4, 2, 1, 3)
        else:              # gravel ballast
            for py in (t, t + 2, t + 4):
                for px in range(l + (py - t) // 2 % 2, l + 8, 3):
                    d.point((px, py), fill=colors[0])
        return
    # Right half only: the base styles already place their own roof unit left.
    left, top = x + w // 2 + 1, y + 6
    right, bottom = x + w - 7, y + h - roof - 4
    if right - left < 6 or bottom - top < 4:
        return
    kind = seed % 6
    if kind == 0 and right - left >= 10:
        # Toronto-style rooftop water tank on legs.
        cx = right - 5
        d.ellipse((cx - 4, top, cx + 3, top + 7), fill=colors[3], outline=colors[0])
        d.line((cx - 2, top + 3, cx + 1, top + 3), fill=colors[0])
    elif kind == 1:
        for i, hx in enumerate(range(left, right - 4, 9)):
            if i > 2:
                break
            box(hx, top, 5, 4, 3)
            d.rectangle((hx, top, hx + 4, top + 3), outline=colors[0])
    elif kind == 2 and bottom - top >= 6:
        d.rectangle((left, top, right, top + 4), outline=colors[0], fill=colors[3])
        for sx in range(left + 3, right, 4):
            d.line((sx, top + 1, sx, top + 3), fill=colors[0])
    elif kind == 3 and right - left >= 12 and bottom - top >= 6:
        d.rectangle((left, top, right - 2, bottom), fill=colors[0])
        for sx in range(left + 2, right - 2, 3):
            d.line((sx, top + 1, sx, bottom - 1), fill=colors[1])
    elif kind == 4:
        box(left, top, 6, 5, 1)
        d.rectangle((left, top, left + 5, top + 4), outline=colors[0])
        box(left + 2, top + 4, 2, 1, 3)
    else:
        # Gravel ballast speckle.
        for py in range(top, bottom + 1, 3):
            for px in range(left + (py // 3) % 2, right + 1, 4):
                d.point((px, py), fill=colors[0])
    if (seed >> 5) % 3 == 0 and right - left >= 14:
        # Antenna mast on the opposite corner.
        d.line((left + 1, bottom - 4, left + 1, bottom), fill=colors[0])
        d.point((left + 1, bottom - 5), fill=colors[3])


def gable_house(d, box, x, y, w, h, roof, colors):
    """Detached house seen from above: lit and shaded roof slopes with a dark
    ridge along the longer side, shingle courses, a chimney, and a front wall
    (the bottom `roof` pixels) with a porch door and windows."""
    bottom = y + h - roof
    if w <= h - roof:  # ridge runs north-south along the deep side
        mid = x + w // 2
        box(x + 1, y + 1, mid - x - 1, bottom - y - 1, 3)
        box(mid, y + 1, x + w - 1 - mid, bottom - y - 1, 2)
        for yy in range(y + 4, bottom - 1, 4):
            d.line((x + 2, yy, mid - 2, yy), fill=colors[2])
            d.line((mid + 1, yy, x + w - 3, yy), fill=colors[1])
        d.line((mid, y + 1, mid, bottom - 1), fill=colors[0])
    else:  # ridge runs east-west: north slope lit, south slope shaded
        mid = y + (bottom - y) // 2
        box(x + 1, y + 1, w - 2, mid - y - 1, 3)
        box(x + 1, mid, w - 2, bottom - mid, 2)
        for xx in range(x + 4, x + w - 2, 4):
            d.line((xx, y + 2, xx, mid - 2), fill=colors[2])
            d.line((xx, mid + 1, xx, bottom - 2), fill=colors[1])
        d.line((x + 1, mid, x + w - 2, mid), fill=colors[0])
    box(x + w - 7, y + 3, 3, 4, 0)
    box(x, bottom, w, roof, 1)
    d.line((x, bottom, x + w - 1, bottom), fill=colors[0])
    box(x + w // 2 - 2, y + h - 6, 4, 6, 0)
    box(x + w // 2 - 3, y + h - 1, 6, 1, 3)
    if w >= 16:
        box(x + 2, bottom + 2, 3, 3, 3)
        box(x + w - 5, bottom + 2, 3, 3, 3)


def tower_crown(d, box, x, y, w, lip, style, colors):
    """Upper floors of a tall building drawn `lip` pixels north of its
    footprint at y. They overhang the street and carry BG priority, so
    walkers and cars on that side pass behind the building. The lightest
    shade is BG colour 0, which never covers sprites, so it is avoided."""
    box(x, y - lip, w, lip, 2)
    d.line((x, y - lip, x + w - 1, y - lip), fill=colors[0])
    d.line((x, y - lip, x, y - 1), fill=colors[0])
    d.line((x + w - 1, y - lip, x + w - 1, y - 1), fill=colors[0])
    if style == 4:  # glass curtain wall: floor lines and mullions
        for yy in range(y - lip + 4, y, 4):
            d.line((x + 1, yy, x + w - 2, yy), fill=colors[1])
        for xx in range(x + 4, x + w - 1, 4):
            d.line((xx, y - lip + 1, xx, y - 1), fill=colors[1])
    else:  # Art Deco setback crown and piers
        box(x + 4, y - lip + 1, w - 8, 5, 1)
        d.line((x + 6, y - lip + 3, x + w - 7, y - lip + 3), fill=colors[0])
        for xx in range(x + 3, x + w - 2, 6):
            d.line((xx, y - lip + 7, xx, y - 1), fill=colors[1])


def tree(d, x, y, colors, variant=0):
    """16x16 canopy seen from above; tile-aligned so trees share patterns."""
    d.ellipse((x, y, x + 15, y + 15), fill=colors[1], outline=colors[0])
    if variant == 0:
        d.ellipse((x + 3, y + 3, x + 10, y + 10), fill=colors[2])
        d.point((x + 5, y + 5), fill=colors[3])
    else:
        for px, py in ((x + 4, y + 4), (x + 9, y + 5), (x + 6, y + 9), (x + 10, y + 10)):
            d.rectangle((px, py, px + 1, py + 1), fill=colors[2])


def lawn_texture(d, x, y, w, h, colors):
    """Sparse grass tufts on an 8px lattice (one repeating tile)."""
    for py in range((y + 7) // 8 * 8 + 3, y + h - 2, 8):
        for px in range((x + 7) // 8 * 8 + 2, x + w - 2, 8):
            d.point((px, py), fill=colors[1])
            d.point((px + 1, py - 1), fill=colors[1])


def parking_lot(d, box, x, y, w, h, colors):
    """Asphalt lot with painted stalls on an 8px period; returns its rectangle."""
    box(x, y, w, h, 1)
    d.rectangle((x, y, x + w - 1, y + h - 1), outline=colors[0])
    for px in range(x + 8, x + w - 1, 8):
        d.line((px, y + 1, px, y + 6), fill=colors[3])
        if h >= 24:
            d.line((px, y + h - 7, px, y + h - 2), fill=colors[3])
    return (x, y, w, h)


def plaza(d, box, x, y, w, h, colors):
    """Light paving with a square grid every 8px."""
    box(x, y, w, h, 3)
    for px in range(x, x + w, 8):
        d.line((px, y, px, y + h - 1), fill=colors[2])
    for py in range(y, y + h, 8):
        d.line((x, py, x + w - 1, py), fill=colors[2])


def flowerbed(d, box, x, y, w, h, colors):
    box(x, y, w, h, 1)
    d.rectangle((x, y, x + w - 1, y + h - 1), outline=colors[0])
    for py in range(y + 2, y + h - 1, 3):
        for px in range(x + 2 + (py % 2), x + w - 1, 3):
            d.point((px, py), fill=colors[3])


def dress_lots(d, box, tw, th, lot, set_attr, colors, salt, parks=()):
    """Decorate undecorated walkable ground tile rectangles (lot(tx,ty)).
    Themes by size and seed: parking (stone palette asphalt), plaza (stone
    paving), or lawn with canopy trees. Park rectangles only get lawn and
    trees. Returns canopy rectangles. Collision is never touched."""
    used = [[False] * tw for _ in range(th)]
    canopies = []

    def in_park(px, py):
        return any(x <= px < x + w and y <= py < y + h for x, y, w, h in parks)

    for ty in range(th):
        for tx in range(tw):
            if used[ty][tx] or not lot(tx, ty):
                continue
            w = 0
            while tx + w < tw and w < 12 and not used[ty][tx + w] and lot(tx + w, ty):
                w += 1
            h = 1
            while ty + h < th and h < 10 and all(not used[ty + h][tx + k] and lot(tx + k, ty + h) for k in range(w)):
                h += 1
            for yy in range(ty, ty + h):
                for xx in range(tx, tx + w):
                    used[yy][xx] = True
            X, Y, W, H = tx * 8, ty * 8, w * 8, h * 8
            seed = seed_of(salt, tx, ty)
            park = in_park(X + 4, Y + 4)
            lawn_texture(d, X, Y, W, H, colors)
            if not park and w >= 6 and h >= 4 and seed % 3 == 0:
                lx, ly, lw, lh = X + 8, Y + 8, W - 16, H - 16
                parking_lot(d, box, lx, ly, lw, lh, colors)
                for yy in range(ly // 8, (ly + lh) // 8):
                    for xx in range(lx // 8, (lx + lw) // 8):
                        set_attr(xx, yy, 0)
                continue
            if not park and w >= 4 and h >= 4 and seed % 3 == 1:
                px, py, pw, ph = X + 8, Y + 8, W - 16, H - 16
                plaza(d, box, px, py, pw, ph, colors)
                for yy in range(py // 8, (py + ph) // 8):
                    for xx in range(px // 8, (px + pw) // 8):
                        set_attr(xx, yy, 0)
                if pw >= 16 and ph >= 16:
                    cx, cy = px + (pw // 16 - 1) * 8, py + (ph // 16 - 1) * 8
                    tree(d, cx, cy, colors, seed & 1)
                    for yy in (cy // 8, cy // 8 + 1):
                        for xx in (cx // 8, cx // 8 + 1):
                            set_attr(xx, yy, 6 | 128)
                    canopies.append([cx, cy, 16, 16])
                continue
            if w >= 2 and h >= 2:
                step = 24 if seed % 2 else 32
                for cy in range(Y + 4 - (Y + 4) % 8 + (8 if H > 24 else 0), Y + H - 16 + 1, step):
                    for cx in range(X + 4 - (X + 4) % 8 + (8 if W > 24 else 0), X + W - 16 + 1, step):
                        cx8, cy8 = cx // 8 * 8, cy // 8 * 8
                        if cx8 < X or cy8 < Y or cx8 + 16 > X + W or cy8 + 16 > Y + H:
                            continue
                        tree(d, cx8, cy8, colors, seed_of(salt, cx8, cy8) & 1)
                        for yy in (cy8 // 8, cy8 // 8 + 1):
                            for xx in (cx8 // 8, cx8 // 8 + 1):
                                set_attr(xx, yy, 6 | 128)
                        canopies.append([cx8, cy8, 16, 16])
    return canopies


# Open water: a 32 x 32 px ripple texture (16 tiles) repeated on a global grid
# so every scene shares the same water tiles, and four frames of drifting
# ripples and twinkling glints that the engine swaps into those tiles
# (td_scenery.c). Colour indices are into COLORS: 3 cream, 2 teal (the water),
# 1 slate, 0 dark.
WATER_SIZE = 32
WATER_FRAMES = 4
# Ripples: left x, row, length, drift direction, frames with a glint.
_RIPPLES = [(2, 3, 7, 1, (0,)), (19, 5, 5, -1, (2,)), (11, 10, 4, 1, ()), (25, 12, 8, -1, (1,)),
            (4, 18, 6, -1, (3,)), (16, 21, 7, 1, (0, 2)), (28, 25, 4, 1, ()), (8, 28, 8, -1, (1,))]
_SPECKS = [(14, 1), (30, 8), (6, 14), (21, 16), (1, 23), (23, 29)]
# Steady dots: every tile carries a mark, so no water tile equals plain ground.
_DOTS = [(27, 4), (21, 9), (13, 19), (30, 21), (3, 26), (18, 26)]
_DRIFT = [0, 1, 2, 1]


def water_pattern(frame):
    """32 x 32 COLORS indices of the open-water texture in one frame."""
    g = [[2] * WATER_SIZE for _ in range(WATER_SIZE)]
    for x, y in _SPECKS:
        if (x + y + frame) % 4:
            g[y][x] = 1
    for x, y in _DOTS:
        g[y][x] = 1
    for x0, row, length, sign, glints in _RIPPLES:
        x = x0 + sign * _DRIFT[frame]
        for i in range(length):
            px = (x + i) % WATER_SIZE
            if i in (0, length - 1):
                g[row - 1][px] = 1
            else:
                g[row][px] = 1
        if frame in glints:
            g[row - 1][(x + 2) % WATER_SIZE] = 3
            if length > 5:
                g[row - 1][(x + 3) % WATER_SIZE] = 3
    return g


def water_tiles():
    """[frame][tile] -> 64 COLORS indices, tile = (y // 8) * 4 + x // 8."""
    out = []
    for f in range(WATER_FRAMES):
        g = water_pattern(f)
        out.append([[g[ty * 8 + y][tx * 8 + x] for y in range(8) for x in range(8)]
                    for ty in range(4) for tx in range(4)])
    return out


def texture_water(img, attrs, tw, is_water, colors):
    """Repaint open water with the shared texture and a foam edge.

    A pixel is water when is_water(x, y) holds, its tile uses palette slot 0
    and it still has the plain water colour (COLORS[2]) after everything else
    was drawn. Whole-water tiles take the texture's first frame exactly (the
    engine animates those); tiles on a shore are calm shallows (plain water)
    with a cream foam line along the land and a broken second line, so a
    shoreline adds few tile patterns; grass or ground beside the water gets a
    sand line. Returns the number of whole-water tiles."""
    px = img.load(); w, h = img.size
    base = tuple(int(colors[2][i:i + 2], 16) for i in (1, 3, 5))
    rgb = [tuple(int(c[i:i + 2], 16) for i in (1, 3, 5)) for c in colors]
    wet = bytearray(w * h)
    for y in range(h):
        row = (y // 8) * tw
        for x in range(w):
            if attrs[row + x // 8] & 7 == 0 and px[x, y] == base and is_water(x, y):
                wet[y * w + x] = 1
    pattern = water_pattern(0)
    whole = 0
    # Grass and ground meeting the water get a sand line (land side), so a
    # tile-aligned shore shows an edge without breaking whole-water tiles.
    sand = []
    for y in range(h):
        for x in range(w):
            if not wet[y * w + x]:
                continue
            for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                if 0 <= nx < w and 0 <= ny < h and not wet[ny * w + nx] and \
                        attrs[(ny // 8) * tw + nx // 8] & 7 == 6 and px[nx, ny] == base:
                    sand.append((nx, ny))
    for x, y in sand:
        px[x, y] = rgb[3]

    def dry(x, y):
        return 0 <= x < w and 0 <= y < h and not wet[y * w + x]
    for ty in range(h // 8):
        for tx in range(w // 8):
            cells = [(tx * 8 + x, ty * 8 + y) for y in range(8) for x in range(8)]
            full = all(wet[y * w + x] for x, y in cells)
            whole += full
            for x, y in cells:
                if not wet[y * w + x]:
                    continue
                c = pattern[y % WATER_SIZE][x % WATER_SIZE] if full else 2
                if not full:
                    edge = dry(x - 1, y) or dry(x + 1, y) or dry(x, y - 1) or dry(x, y + 1)
                    near = dry(x - 2, y) or dry(x + 2, y) or dry(x, y - 2) or dry(x, y + 2)
                    if edge or (near and (x + y) % 3 == 0):
                        c = 3
                px[x, y] = rgb[c]
    return whole


# Video screens on the roofs at Yonge and Dundas: 32 x 16 px (eight tiles),
# four frames that the engine cycles like the water. Colour indices into
# COLORS: 3 light, 2 the building's accent, 1 deep, 0 dark frame.
SCREEN_W, SCREEN_H = 32, 16
_LANTERN = ["..XX..", ".XXXX.", "XXXXXX", "XXXXXX", ".XXXX.", "..XX.."]
_PARCEL = ["XXXXXXX", "X..X..X", "XXXXXXX", "X..X..X", "X..X..X", "XXXXXXX"]


def screen_pattern(frame):
    """32 x 16 COLORS indices of the screen in one frame."""
    w, h = SCREEN_W, SCREEN_H
    g = [[1] * w for _ in range(h)]
    if frame == 0:          # Lakelight: a lantern and its glow
        for y, row in enumerate(_LANTERN):
            for x, c in enumerate(row):
                if c == 'X':
                    g[4 + y][4 + x] = 3
        for x in range(13, 29, 3):
            g[7][x] = 2; g[9][x + 1] = 2
    elif frame == 1:        # stripes sweeping across
        for y in range(h):
            for x in range(w):
                if (x + y) % 8 < 3:
                    g[y][x] = 2
    elif frame == 2:        # a parcel: the depot's ad
        for y, row in enumerate(_PARCEL):
            for x, c in enumerate(row):
                if c == 'X':
                    g[4 + y][12 + x] = 3
        for x in range(3, 9):
            g[13][x] = 2; g[13][w - 1 - x] = 2
    else:                   # fireworks over the lake
        for x, y in ((6, 4), (12, 9), (19, 5), (25, 10), (9, 12), (22, 3)):
            g[y][x] = 3
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                g[y + dy][x + dx] = 2
    for x in range(w):
        g[0][x] = g[h - 1][x] = 0
    for y in range(h):
        g[y][0] = g[y][w - 1] = 0
    return g


def screen_tiles():
    """[frame][tile] -> 64 COLORS indices, tile = (y // 8) * 4 + x // 8."""
    out = []
    for f in range(4):
        g = screen_pattern(f)
        out.append([[g[ty * 8 + y][tx * 8 + x] for y in range(8) for x in range(8)]
                    for ty in range(SCREEN_H // 8) for tx in range(SCREEN_W // 8)])
    return out


def paint_screen(img, x0, y0, colors):
    """Draws the screen's first frame at a tile-aligned (x0, y0)."""
    assert x0 % 8 == 0 and y0 % 8 == 0
    px = img.load(); g = screen_pattern(0)
    rgb = [tuple(int(c[i:i + 2], 16) for i in (1, 3, 5)) for c in colors]
    for y in range(SCREEN_H):
        for x in range(SCREEN_W):
            px[x0 + x, y0 + y] = rgb[g[y][x]]


# Spray bays: one body shop per scene. The bay is painted in a road lane and
# the shop's roll-up door on the building behind it. The engine keeps their
# centres in TORONTO.c (td_spray_at, by scene); each art script checks it.
SPRAY_BAY_W, SPRAY_BAY_H = 32, 16


def paint_spray_bay(d, box, colors, bx, by, door_bottom, door_h=20):
    """Hazard-striped bay at (bx, by) and a roll-up door ending at door_bottom."""
    box(bx, by, SPRAY_BAY_W, SPRAY_BAY_H, 0)
    for x in range(bx + 2, bx + SPRAY_BAY_W - 2, 4):
        box(x, by + 2, 2, SPRAY_BAY_H - 4, 3)
    d.rectangle((bx, by, bx + SPRAY_BAY_W - 1, by + SPRAY_BAY_H - 1), outline=colors[3])
    box(bx + 2, door_bottom - door_h, SPRAY_BAY_W - 4, door_h, 0)
    for y in range(door_bottom - door_h + 2, door_bottom, 3):
        d.line((bx + 4, y, bx + SPRAY_BAY_W - 5, y), fill=colors[1])
    box(bx + 10, door_bottom - door_h - 4, 12, 3, 3)


def spray_bay_registered(engine_text, scene, bx, by):
    """True when TORONTO.c's td_spray_at row for this scene is the bay's centre."""
    table = re.search(r'td_spray_at\[TD_SPRAY_BAYS\]\[2\]=\{(.*?)\};', engine_text)
    rows = re.findall(r'\{(\d+),(\d+)\}', table.group(1)) if table else []
    return scene < len(rows) and (int(rows[scene][0]), int(rows[scene][1])) == (bx + SPRAY_BAY_W // 2, by + SPRAY_BAY_H // 2)


# Park features drawn from the layouts' park entries (West and East scenes):
# each returns the rectangles to make solid and (rect, palette slot) pairs.
# Lines and surfaces use the park palette unless noted, so a feature never
# shares a tile with another palette.
def paint_park_feature(d, box, colors, kind, x, y, w, h):
    solid, slots = [], []
    if kind == 'pitch':        # soccer pitch, long axis east-west
        cx, cy = x + w // 2, y + h // 2
        d.rectangle((x, y, x + w - 1, y + h - 1), outline=colors[3])
        d.line((cx, y, cx, y + h - 1), fill=colors[3])
        d.ellipse((cx - 6, cy - 6, cx + 6, cy + 6), outline=colors[3])
        for gx, sgn in ((x, 1), (x + w - 1, -1)):
            d.rectangle((min(gx, gx + sgn * 11), cy - 10, max(gx, gx + sgn * 11), cy + 10), outline=colors[3])
            box(gx - (2 if sgn > 0 else -1), cy - 4, 2, 8, 0)
    elif kind == 'diamond':    # baseball diamond, home plate to the south
        cx, cy, r = x + w // 2, y + h // 2, min(w, h) // 2 - 4
        for k in range(3):
            d.polygon([(cx, cy + r - k), (cx + r - k, cy), (cx, cy - r + k), (cx - r + k, cy)], outline=colors[3])
        d.line((cx, cy + r, x, cy + r - (cx - x)), fill=colors[3]); d.line((cx, cy + r, x + w - 1, cy + r - (x + w - 1 - cx)), fill=colors[3])
        for bx, by in ((cx, cy + r), (cx + r, cy), (cx, cy - r), (cx - r, cy)):
            box(bx - 1, by - 1, 3, 3, 3)
        box(cx - 1, cy - 1, 3, 3, 3)
        d.arc((cx - 6, cy + r - 2, cx + 6, cy + r + 6), 0, 180, fill=colors[0])
    elif kind == 'rink':       # outdoor rink: ice, boards and lines
        box(x, y, w, h, 3); d.rectangle((x, y, x + w - 1, y + h - 1), outline=colors[0])
        for px, py in ((x, y), (x + w - 1, y), (x, y + h - 1), (x + w - 1, y + h - 1)):
            d.point((px, py), fill=colors[2])
        d.line((x + w // 2, y + 1, x + w // 2, y + h - 2), fill=colors[1])
        for lx in (x + w // 3, x + w - 1 - w // 3):
            d.line((lx, y + 1, lx, y + h - 2), fill=colors[0])
    elif kind == 'pool':       # outdoor pool: deck, lanes (stone and water palette)
        box(x, y, w, h, 3); box(x + 3, y + 3, w - 6, h - 6, 2)
        d.rectangle((x + 3, y + 3, x + w - 4, y + h - 4), outline=colors[0])
        for ly in range(y + 7, y + h - 4, 4):
            d.line((x + 4, ly, x + w - 5, ly), fill=colors[1])
        solid.append((x + 3, y + 3, w - 6, h - 6)); slots.append(((x, y, w, h), 0))
    elif kind == 'paddock':    # zoo paddock: post-and-rail fence and a bison
        d.rectangle((x, y, x + w - 1, y + h - 1), outline=colors[0])
        for px in range(x, x + w, 4):
            d.point((px, y + 1), fill=colors[0]); d.point((px, y + h - 2), fill=colors[0])
        bx, by = x + w // 3, y + h // 2 - 2
        box(bx, by, 6, 4, 0); box(bx + 6, by + 1, 2, 2, 0); box(bx + 1, by + 4, 1, 1, 0); box(bx + 4, by + 4, 1, 1, 0)
        solid.append((x, y, w, h))
    elif kind != 'meadow':
        raise ValueError(kind)
    return solid, slots
