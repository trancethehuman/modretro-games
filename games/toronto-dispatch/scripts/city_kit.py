"""Shared original street furniture and roof detail for the district painters.

Everything here is decoration on top of the authored road, sidewalk and
building geometry: it never changes a collision value. Colour arguments are
indices into the native 4-shade source palette used by every generator:
0 dark, 1 asphalt / deep tone, 2 ground / body, 3 light.
"""
import hashlib

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


def roof_details(d, box, x, y, w, h, roof, seed, colors):
    """Varied rooftop equipment inside the existing roof outline. Picks one or
    two features from the seed: water tank, HVAC units, skylight strip, solar
    grid or a stair bulkhead. Never draws outside (x+5..x+w-6, y+5..)."""
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
