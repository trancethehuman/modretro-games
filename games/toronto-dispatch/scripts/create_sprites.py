"""Build the registered actor sprite sheet, its CGB sprite palettes and the
engine frame map from the original designs in sprite_art.py.

Writes:
  project/assets/sprites/dispatch_topdown.png(.gbsres)   registered sprite
  project/original-art/dispatch_topdown.png               editable copy
  project/dispatch_topdown.metadata.json                  frame metadata
  project/project/palettes/td_sprite_*.gbsres             8 OBJ palettes
  project/project/scenes/toronto_*/scene.gbsres           spritePaletteIds
  engine/include/td_sprites.h                              frame constants
`--check` verifies that every output is current without writing.
"""
import hashlib
import io
import json
import sys
import uuid
from pathlib import Path
from PIL import Image
import sprite_art as A

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'project'
ENGINE = PROJECT / 'plugins/toronto-driving/engine'
SPRITE_ID = '95080aab-0201-545e-be5a-f5e79b9a693e'  # existing registered sprite
SHADES = [(0x65, 0xFF, 0x00), (0xE0, 0xF8, 0xCF), (0x86, 0xC0, 0x6C), (0x07, 0x18, 0x21)]
CANVAS = 64
# GB Studio masks every frame inside a canvas whose bottom row is tile y 0,
# so nothing can sit below a frame's baseline. All tiles are raised by LIFT
# pixels and the origin lowered to match: compiled offsets are unchanged and
# the night headlamps of a south-facing vehicle fit below it.
LIFT = 16
CANVAS_HEIGHT = 96

# CGB OBJ palettes. GB Studio sprite palettes compile resource colours
# [0, 1, 3] to sprite indices 1 (light), 2 (mid) and 3 (dark); colour [2] is
# unused, so it repeats the mid tone. Index 0 is transparent.
PALETTES = [
    ('courier_vehicle', 'Courier vehicle', ['F8F8F0', 'E87820', 'E87820', '182030']),
    ('courier_person', 'Courier uniform', ['F8C8A0', 'E87820', 'E87820', '182030']),
    ('traffic_red', 'Traffic red', ['F0D0B8', 'C83028', 'C83028', '101018']),
    ('traffic_blue', 'Traffic blue and ferry', ['D0E8F8', '3068C8', '3068C8', '101018']),
    ('signal_yellow', 'Taxi and beacon yellow', ['F8F8E0', 'F0C020', 'F0C020', '282010']),
    ('police', 'Police navy', ['F0C090', '2850B0', '2850B0', '101828']),
    ('walker_teal', 'Pedestrian teal', ['F0C090', '309878', '309878', '282030']),
    ('walker_violet', 'Pedestrian violet', ['C89070', '8858B8', '8858B8', '302018']),
]
P = {key: i for i, (key, _, _) in enumerate(PALETTES)}


def ident(name):
    return str(uuid.uuid5(uuid.NAMESPACE_URL, 'toronto-dispatch/sprites/' + name))


def frames():
    """(name, grid, palette, size) in engine frame order.

    Vehicles are drawn in the courier vehicle palette (0) and people in the
    courier uniform palette (1). The engine adds a per-actor palette offset
    when it draws an actor (actor.c.patch), so one frame set serves the
    courier, traffic in six colours, civilians in four and the police."""
    out = []

    def add(name, g, pal, size=None):
        out.append((name, g, P[pal], size or (len(g[0]), len(g))))

    for vehicle, design in (('car', A.car_frames()), ('van', A.van_frames()),
                            ('motorcycle', A.moto_frames()), ('scooter', A.scooter_frames())):
        for i, g in enumerate(design):
            add(f'player_{vehicle}_{i}', g, 'courier_vehicle')
    for design in A.PEOPLE_DESIGNS:
        for i, g in enumerate(A.person_frames(design)):
            add(f'person_{design}_{i}', g, 'courier_person')
    add('beacon', A.grid(A.BEACON), 'signal_yellow')
    add('beacon_pulse', A.grid(A.BEACON_PULSE), 'signal_yellow')
    add('car_door_open', A.door_frame(), 'courier_vehicle')
    # Traffic-only designs (cardinal views), also in the vehicle palette.
    # Taxis are the full-size sedan in yellow: they share its tiles.
    for name, design in (('traffic_taxi', cardinal(A.car_frames())), ('traffic_compact', A.compact_frames()),
                         ('traffic_pickup', A.pickup_frames()), ('traffic_sports', A.sports_frames()),
                         ('police', A.police_frames())):
        for i, g in enumerate(design):
            add(f'{name}_{i}', g, 'courier_vehicle')
    # Sidewalk pickups: cash, first aid, ammunition (they bob, no glint frame).
    for name, design, pal in (('pickup_cash', A.PICKUP_CASH, 'signal_yellow'),
                              ('pickup_first_aid', A.PICKUP_FIRST_AID, 'traffic_red'),
                              ('pickup_ammo', A.PICKUP_AMMO, 'police')):
        add(name, A.grid(design), pal)
    # Buses and streetcars only run east-west; ferries north-south. A ferry
    # frame 48 high keeps 16-pixel rows, so north is the flip of south.
    bus_e = A.big_frame(A.bus_zone(), 0, (40, 16))
    add('bus_e', bus_e, 'traffic_red', (40, 16)); add('bus_w', A.flip_h(bus_e), 'traffic_red', (40, 16))
    car_e = A.big_frame(A.streetcar_zone(), 0, (48, 16))
    add('streetcar_e', car_e, 'traffic_red', (48, 16)); add('streetcar_w', A.flip_h(car_e), 'traffic_red', (48, 16))
    ferry_s = A.big_frame(A.ferry_zone(), 90, (16, 48))
    add('ferry_s', ferry_s, 'traffic_blue', (16, 48)); add('ferry_n', A.flip_v(ferry_s), 'traffic_blue', (16, 48))
    # Street life: knock-downs in the people palette, effects and pointers.
    for i, g in enumerate(A.knockdown_frames()):
        add(f'knock_{i}', g, 'courier_person')
    add('spark', A.grid(A.SPARK), 'signal_yellow')
    for i, (g, size, _) in enumerate(A.shot_frames()):
        add(f'shot_{i}', g, 'signal_yellow', size)
    add('reticle', A.RETICLE, 'traffic_red')
    for i, g in enumerate(A.arrow_frames()):
        add(f'arrow_{i}', g, 'signal_yellow')
    # Animation: courier punch and pistol poses, smoke, collection pops and
    # night headlamps.
    for kind in ('punch', 'shoot'):
        for i, g in enumerate(A.action_frames(kind)):
            add(f'courier_{kind}_{i}', g, 'courier_person')
    for i, g in enumerate(A.SMOKE):
        add(f'smoke_{i}', g, 'courier_vehicle')
    add('parcel', A.PARCEL, 'courier_vehicle')
    for i, g in enumerate(A.SPARKLE):
        add(f'sparkle_{i}', g, 'signal_yellow')
    for i, g in enumerate(A.beam_frames()):
        add(f'beam_{i}', g, 'signal_yellow')
    return out


def cardinal(frames):
    """Road traffic only drives cardinally: diagonal slots repeat a cardinal
    view so they share its tiles."""
    return [frames[i & 6] for i in range(8)]


# The courier's vehicles in engine order, and the headlamp beam frame centre
# relative to the vehicle in the eight headings (E, SE, S, SW, W, NW, N, NE).
PLAYER_VEHICLES = ('car', 'van', 'motorcycle', 'scooter')
BEAM_OFFSETS = ((16, 0), (10, 10), (0, 16), (-10, 10), (-16, 0), (-10, -10), (0, -16), (10, -10))


def slice_count(fr, sheet, places, frame_defs):
    """Unique 8x16 OBJ tiles after GB Studio's flip-aware de-duplication."""
    seen = set()
    for (name, g, pal, size), fd in zip(fr, frame_defs):
        for t in fd['tiles']:
            sx, sy = t['sliceX'], t['sliceY']
            px = tuple(tuple(sheet.getpixel((sx + a, sy + b)) if 0 <= sy + b < sheet.height else SHADES[0]
                             for a in range(8)) for b in range(16))
            variants = {px, tuple(r[::-1] for r in px), px[::-1], tuple(r[::-1] for r in px[::-1])}
            if not variants & seen:
                seen.add(px)
    return len(seen)


def narrow(g):
    """TRUE when a 16x16 drawing only uses columns 4..11: one centred 8x16
    OBJ then shows it, halving its hardware sprites and tiles."""
    return all(not v for row in g for x, v in enumerate(row) if x < 4 or x > 11)


def tile_boxes(size, g=None, centred=False):
    """8x16 tile cells and their canvas coordinates for a frame size.

    Small frames keep the original 16x16 coordinates (x 0/8, y 0) so compiled
    offsets are unchanged; a narrow drawing uses one tile at x 4, centred on
    the same point. Wide frames are centred on the same point; tall frames
    grow upward (GB Studio y is up) and report an anchor offset."""
    w, h = size
    if size == (16, 16) and g is not None and narrow(g):
        return [(4, 0, 4, 0)], (0, 0)
    if centred and size == (16, 32):
        # Full-size vehicles facing north or south: centred on the 16x16 point,
        # rows -8..24, so the north view is the vertical flip of the south.
        return [(px, 16 * r, px, 8 - 16 * r) for r in range(2) for px in (0, 8)], (0, 0)
    if size == (32, 32):
        # Diagonal tracers: centred like wide frames, growing upward like tall ones.
        return [(px, 16 - 16 * r, px - 8, 16 * r) for r in range(2) for px in range(0, 32, 8)], (0, 8)
    assert w % 8 == 0 and (h == 16 or (w == 16 and h % 8 == 0)), size
    if h == 16:
        x0 = 8 - w // 2
        return [(px, 0, x0 + px, 0) for px in range(0, w, 8)], (0, 0)
    rows = (h + 15) // 16
    cells = []
    for r in range(rows):
        top = h - 16 * (r + 1)  # bottom row first; last row may overhang the top
        for px in (0, 8):
            cells.append((px, top, px, 16 * r))
    # Frame pixel rows run from 16-h (top) to 16 (bottom): centre at 16-h/2.
    return cells, (0, 8 - (16 - h // 2))


def build():
    fr = frames()
    names = [f[0] for f in fr]
    assert len(set(names)) == len(names)
    # Pack: 16x16 frames in rows of 16 cells, then each large frame on its own band.
    small = [i for i, f in enumerate(fr) if f[3] == (16, 16)]
    large = [i for i, f in enumerate(fr) if f[3] != (16, 16)]
    places = {}
    for k, i in enumerate(small):
        places[i] = ((k % 16) * 16, (k // 16) * 16)
    y = ((len(small) + 15) // 16) * 16
    x = 0
    band = 0
    for i in large:
        w, h = fr[i][3]
        hh = (h + 15) // 16 * 16
        if x + w > 256:
            x = 0; y += band; band = 0
        places[i] = (x, y + (hh - h))
        x += w; band = max(band, hh)
    height = y + band
    sheet = Image.new('RGB', (256, height), SHADES[0])
    for i, (name, g, pal, size) in enumerate(fr):
        ox, oy = places[i]
        for yy, row in enumerate(g):
            for xx, v in enumerate(row):
                sheet.putpixel((ox + xx, oy + yy), SHADES[v])
    frame_defs, anchors = [], {}
    for i, (name, g, pal, size) in enumerate(fr):
        ox, oy = places[i]
        cells, anchor = tile_boxes(size, g, centred=True)
        if size != (16, 16):
            anchors[name] = anchor
        tiles = []
        for n, (px, py, cx, cy) in enumerate(cells):
            sx, sy = ox + px, oy + py
            # Skip fully transparent 8x16 slices: fewer OAM entries and tiles.
            if all(sheet.getpixel((sx + a, sy + b)) == SHADES[0] for a in range(8) for b in range(16)
                   if 0 <= sy + b < height):
                continue
            tiles.append({'id': ident(f'{name}-tile-{n}'), 'x': cx, 'y': cy + LIFT, 'sliceX': sx, 'sliceY': sy,
                          'flipX': False, 'flipY': False, 'palette': 0, 'paletteIndex': pal,
                          'objPalette': 'OBP0', 'priority': False})
        assert tiles, name
        frame_defs.append({'id': ident(f'{name}-frame'), 'tiles': tiles})
    # Night variants of the courier's vehicles: the vehicle's own tiles plus
    # the headlamp beam's, placed ahead in the same metasprite. They reuse
    # existing tiles, so they cost no VRAM and no extra actor.
    index = {f[0]: i for i, f in enumerate(fr)}
    for vehicle in PLAYER_VEHICLES:
        for d, (dx, dy) in enumerate(BEAM_OFFSETS):
            body, beam = frame_defs[index[f'player_{vehicle}_{d}']], frame_defs[index[f'beam_{d}']]
            name = f'player_{vehicle}_lit_{d}'
            # GB Studio's compiler hides pixels that overlap in one frame,
            # which would create new tiles: the light never touches the body.
            car, light = fr[index[f'player_{vehicle}_{d}']][1], fr[index[f'beam_{d}']][1]
            def pixels(g, ox=0, oy=0):
                h, w = len(g), len(g[0])
                return {(x - w // 2 + ox, y - h // 2 + oy) for y in range(h) for x in range(w) if g[y][x]}
            assert not pixels(car) & pixels(light, dx, dy), name
            # GB Studio tile y grows upward; screen dy grows downward.
            tiles = [dict(t, id=ident(f'{name}-body-{n}')) for n, t in enumerate(body['tiles'])]
            tiles += [dict(t, id=ident(f'{name}-beam-{n}'), x=t['x'] + dx, y=t['y'] - dy)
                      for n, t in enumerate(beam['tiles'])]
            frame_defs.append({'id': ident(f'{name}-frame'), 'tiles': tiles})
            fr.append((name, None, P['courier_vehicle'], fr[index[f'player_{vehicle}_{d}']][3]))
    assert len(fr) <= 256, 'actor frame indices are one byte'
    # Colour-only scenes split OBJ tiles evenly over both VRAM banks below the
    # UI art at tile 128: at most 64 8x16 tiles (128 8x8 tiles) per bank.
    unique = slice_count(fr, sheet, places, frame_defs)
    assert unique <= 128, f'Actor sprites need {unique} 8x16 tiles; the VRAM budget is 128'
    return fr, sheet, frame_defs, anchors


def png_bytes(img):
    buf = io.BytesIO()
    img.save(buf, format='PNG', optimize=True)
    return buf.getvalue()


def outputs():
    fr, sheet, frame_defs, anchors = build()
    png = png_bytes(sheet)
    empty = [{'id': ident(f'empty-{n}'), 'frames': [{'id': ident(f'empty-frame-{n}'), 'tiles': []}]} for n in range(7)]
    state = {'id': ident('state'), 'name': '', 'animationType': 'fixed', 'flipLeft': False,
             'animations': [{'id': ident('all-frames'), 'frames': frame_defs}] + empty}
    resource = {'_resourceType': 'sprite', 'id': SPRITE_ID, 'name': 'Top-down vehicles and courier',
                'symbol': 'sprite_top_down_vehicles_and_courier', 'states': [state],
                'filename': 'dispatch_topdown.png', 'width': sheet.width, 'height': sheet.height,
                'checksum': hashlib.sha1(png).hexdigest(), 'numTiles': 0,
                'canvasOriginX': 8, 'canvasOriginY': 8 - LIFT, 'canvasWidth': CANVAS, 'canvasHeight': CANVAS_HEIGHT,
                'boundsX': 2, 'boundsY': 2, 'boundsWidth': 12, 'boundsHeight': 12, 'animSpeed': 255}
    files = {
        PROJECT / 'assets/sprites/dispatch_topdown.png': png,
        PROJECT / 'original-art/dispatch_topdown.png': png,
        PROJECT / 'assets/sprites/dispatch_topdown.png.gbsres': json.dumps(resource, indent=2) + '\n',
    }
    meta = {'frames': [{'index': i, 'name': f[0], 'palette': PALETTES[f[2]][0], 'size': list(f[3])}
                       for i, f in enumerate(fr)],
            'palettes': [{'key': k, 'name': n, 'colors': c} for k, n, c in PALETTES],
            'anchors': anchors, 'sheet': [sheet.width, sheet.height],
            'note': 'Original pixel art from scripts/sprite_art.py; colour 0 transparent.'}
    files[PROJECT / 'dispatch_topdown.metadata.json'] = json.dumps(meta, indent=2) + '\n'
    palette_ids = []
    for key, name, colors in PALETTES:
        pid = ident('palette-' + key)
        palette_ids.append(pid)
        files[PROJECT / f'project/palettes/td_sprite_{key}.gbsres'] = json.dumps(
            {'_resourceType': 'palette', 'id': pid, 'name': 'Sprite ' + name, 'colors': colors,
             'defaultName': 'Sprite ' + name, 'defaultColors': colors}, indent=2) + '\n'
    for scene in ('toronto_city', 'toronto_west', 'toronto_high_park', 'toronto_east'):
        path = PROJECT / f'project/scenes/{scene}/scene.gbsres'
        data = json.loads(path.read_text())
        data['spritePaletteIds'] = palette_ids
        files[path] = json.dumps(data, indent=2) + '\n'
    # Engine frame map.
    index = {f[0]: i for i, f in enumerate(fr)}
    lines = ['/* Generated by scripts/create_sprites.py from original sprite designs. */',
             '#ifndef TD_SPRITES_H', '#define TD_SPRITES_H',
             f'#define TD_SPRITE_FRAMES {len(fr)}']
    for name in ('player_car_0', 'player_van_0', 'player_motorcycle_0', 'player_scooter_0',
                 *(f'person_{d}_0' for d in A.PEOPLE_DESIGNS),
                 'beacon', 'beacon_pulse', 'car_door_open',
                 'traffic_taxi_0', 'traffic_compact_0', 'traffic_pickup_0', 'traffic_sports_0', 'police_0',
                 'pickup_cash', 'pickup_first_aid', 'pickup_ammo',
                 'bus_e', 'bus_w', 'streetcar_e', 'streetcar_w', 'ferry_s', 'ferry_n',
                 'knock_0', 'spark', 'shot_0', 'reticle', 'arrow_0',
                 'courier_punch_0', 'courier_shoot_0', 'smoke_0', 'parcel', 'sparkle_0', 'beam_0',
                 'player_car_lit_0'):
        macro = 'TD_FRAME_' + name.upper().removesuffix('_0')
        lines.append(f'#define {macro} {index[name]}')
    lines.append('#define TD_FRAME_COURIER_WALK TD_FRAME_PERSON_SHORT')
    lines.append(f'#define TD_KNOCK_FRAMES {len(A.knockdown_frames())}')
    lines.append(f'#define TD_PEOPLE_DESIGNS {len(A.PEOPLE_DESIGNS)}')
    lines.append(f'#define TD_SMOKE_FRAMES {len(A.SMOKE)}')
    # Round heads relative to the actor point (whole pixels, screen axes):
    # the engine draws a round with its head on the bullet's position.
    heads = []
    for g, (w, h), (hx, hy) in A.shot_frames():
        heads.append((hx - w // 2, hy - h))
    # Headlamp beam offsets per heading, for a beam drawn as its own actor.
    lines.append('#define TD_BEAM_DX {' + ','.join(str(dx) for dx, _ in BEAM_OFFSETS) + '}')
    lines.append('#define TD_BEAM_DY {' + ','.join(str(dy) for _, dy in BEAM_OFFSETS) + '}')
    lines.append('#define TD_SHOT_HEAD_DX {' + ','.join(str(x) for x, _ in heads) + '}')
    lines.append('#define TD_SHOT_HEAD_DY {' + ','.join(str(y) for _, y in heads) + '}')
    for name, (dx, dy) in sorted(anchors.items()):
        if dy:
            lines.append(f'#define TD_ANCHOR_{name.upper()}_DY ({dy})')
    lines += ['#endif', '']
    files[ENGINE / 'include/td_sprites.h'] = '\n'.join(lines)
    return files, fr


def main():
    files, fr = outputs()
    stale = []
    for path, content in files.items():
        data = content if isinstance(content, bytes) else content.encode()
        if path.suffix == '.png':
            # PNG encoders differ: compare decoded pixels, not bytes.
            if not path.exists() or Image.open(path).convert('RGB').tobytes() != Image.open(io.BytesIO(data)).convert('RGB').tobytes():
                stale.append(path)
            continue
        if path == PROJECT / 'assets/sprites/dispatch_topdown.png.gbsres' and path.exists():
            # The checksum belongs to the on-disk PNG, whose encoder can differ.
            expected = json.loads(data)
            expected['checksum'] = hashlib.sha1(path.with_suffix('').read_bytes()).hexdigest()
            if json.loads(path.read_text()) != expected:
                stale.append(path)
            continue
        if not path.exists() or path.read_bytes() != data:
            stale.append(path)
    if '--check' in sys.argv:
        assert not stale, 'Sprite outputs are stale: ' + ', '.join(str(p.relative_to(ROOT)) for p in stale)
        print(f'Actor sprites match their original designs: {len(fr)} frames, {len(PALETTES)} CGB palettes.')
        return
    for path in stale:
        content = files[path]
        path.write_bytes(content if isinstance(content, bytes) else content.encode())
    print(f'Wrote {len(stale)} sprite outputs: {len(fr)} frames, {len(PALETTES)} CGB palettes.')


if __name__ == '__main__':
    main()
