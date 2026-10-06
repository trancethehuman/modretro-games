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

# CGB OBJ palettes. GB Studio sprite palettes compile resource colours
# [0, 1, 3] to sprite indices 1 (light), 2 (mid) and 3 (dark); colour [2] is
# unused, so it repeats the mid tone. Index 0 is transparent.
PALETTES = [
    ('courier_vehicle', 'Courier vehicle', ['F8F8F0', 'E87820', 'E87820', '182030']),
    ('courier_person', 'Courier uniform', ['F8C8A0', 'E87820', 'E87820', '182030']),
    ('traffic_red', 'Traffic red', ['D0E8F8', 'C83028', 'C83028', '101018']),
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
    """(name, grid, palette, size) in engine frame order."""
    out = []

    def add(name, g, pal, size=(16, 16)):
        out.append((name, g, P[pal], size))

    for vehicle, design in (('car', A.car_frames()), ('van', A.van_frames()),
                            ('motorcycle', A.moto_frames()), ('scooter', A.scooter_frames())):
        for i, g in enumerate(design):
            add(f'player_{vehicle}_{i}', g, 'courier_vehicle')
    for i, g in enumerate(A.person_frames(0)):
        add(f'courier_walk_{i}', g, 'courier_person')
    add('beacon', A.grid(A.BEACON), 'signal_yellow')
    add('beacon_pulse', A.grid(A.BEACON_PULSE), 'signal_yellow')
    add('stop_marker', A.grid(A.BEACON), 'traffic_red')
    add('depot_marker', A.grid(A.BEACON_PULSE), 'courier_vehicle')
    add('car_door_open', A.grid(A.DOOR_E), 'courier_vehicle')
    for name, design, pal in (('traffic_red', A.car_frames(), 'traffic_red'),
                              ('traffic_blue', A.car_frames(), 'traffic_blue'),
                              ('traffic_taxi', cardinal(A.car_frames(True)), 'signal_yellow'),
                              ('traffic_van', A.van_frames(), 'traffic_red'),
                              ('traffic_motorcycle', A.moto_frames(), 'traffic_blue')):
        for i, g in enumerate(design):
            add(f'{name}_{i}', g, pal)
    for name, variant, pal in (('walker_a', 0, 'walker_teal'), ('walker_b', 1, 'walker_violet'),
                               ('walker_c', 1, 'walker_teal'), ('walker_d', 0, 'walker_violet')):
        for i, g in enumerate(A.person_frames(variant)):
            add(f'{name}_{i}', g, pal)
    for name, design, pal in (('cone', A.CONE, 'courier_vehicle'), ('cone_down', A.CONE_KNOCKED, 'courier_vehicle'),
                              ('bin', A.BIN, 'traffic_blue'), ('bin_down', A.BIN_KNOCKED, 'traffic_blue'),
                              ('newsbox', A.NEWSBOX, 'traffic_red'), ('newsbox_down', A.NEWSBOX_KNOCKED, 'traffic_red'),
                              ('barrel', A.BARREL, 'courier_vehicle'), ('barrel_down', A.BARREL_KNOCKED, 'courier_vehicle'),
                              ('debris', A.DEBRIS, 'courier_vehicle')):
        add(name, A.grid(design), pal)
    bus_e = A.big_frame(A.bus_zone(), 0, (40, 16))
    bus_s = A.big_frame(A.bus_zone(), 90, (16, 40))
    add('bus_e', bus_e, 'traffic_red', (40, 16)); add('bus_w', A.flip_h(bus_e), 'traffic_red', (40, 16))
    add('bus_s', bus_s, 'traffic_red', (16, 40)); add('bus_n', A.flip_v(bus_s), 'traffic_red', (16, 40))
    car_e = A.big_frame(A.streetcar_zone(), 0, (64, 16))
    car_s = A.big_frame(A.streetcar_zone(), 90, (16, 64))
    add('streetcar_e', car_e, 'traffic_red', (64, 16)); add('streetcar_w', A.flip_h(car_e), 'traffic_red', (64, 16))
    add('streetcar_s', car_s, 'traffic_red', (16, 64)); add('streetcar_n', A.flip_v(car_s), 'traffic_red', (16, 64))
    ferry_s = A.big_frame(A.ferry_zone(), 90, (16, 40))
    add('ferry_s', ferry_s, 'traffic_blue', (16, 40)); add('ferry_n', A.flip_v(ferry_s), 'traffic_blue', (16, 40))
    for name, design in (('gull_e_0', A.grid(A.GULL_UP)), ('gull_e_1', A.grid(A.GULL_LEVEL)),
                         ('gull_w_0', A.flip_h(A.grid(A.GULL_UP))), ('gull_w_1', A.flip_h(A.grid(A.GULL_LEVEL)))):
        add(name, design, 'courier_vehicle')
    # Street life added after the original 140 frames so their indices stay put.
    for i, g in enumerate(A.police_frames()):
        add(f'police_{i}', g, 'traffic_blue')
    for i, g in enumerate(A.officer_frames()):
        add(f'officer_{i}', g, 'police')
    for look, pal in KNOCK_LOOKS:
        for i, g in enumerate(A.knockdown_frames()):
            add(f'knock_{look}_{i}', g, pal)
    add('spark', A.grid(A.SPARK), 'signal_yellow')
    add('bullet', A.grid(A.BULLET), 'signal_yellow')
    for i, g in enumerate(A.arrow_frames()):
        add(f'arrow_{i}', g, 'signal_yellow')
    return out


# Knock-down looks in engine order: four civilian walkers, officer, courier.
KNOCK_LOOKS = (('walker_a', 'walker_teal'), ('walker_b', 'walker_violet'), ('walker_c', 'walker_teal'),
               ('walker_d', 'walker_violet'), ('officer', 'police'), ('courier', 'courier_person'))


def cardinal(frames):
    """Road traffic only drives cardinally: diagonal slots repeat a cardinal
    view so they share its tiles."""
    return [frames[i & 6] for i in range(8)]


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


def tile_boxes(size):
    """8x16 tile cells and their canvas coordinates for a frame size.

    Small frames keep the original 16x16 coordinates (x 0/8, y 0) so compiled
    offsets are unchanged. Wide frames are centred on the same point; tall
    frames grow upward (GB Studio y is up) and report an anchor offset."""
    w, h = size
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
        cells, anchor = tile_boxes(size)
        if size != (16, 16):
            anchors[name] = anchor
        tiles = []
        for n, (px, py, cx, cy) in enumerate(cells):
            sx, sy = ox + px, oy + py
            # Skip fully transparent 8x16 slices: fewer OAM entries and tiles.
            if all(sheet.getpixel((sx + a, sy + b)) == SHADES[0] for a in range(8) for b in range(16)
                   if 0 <= sy + b < height):
                continue
            tiles.append({'id': ident(f'{name}-tile-{n}'), 'x': cx, 'y': cy, 'sliceX': sx, 'sliceY': sy,
                          'flipX': False, 'flipY': False, 'palette': 0, 'paletteIndex': pal,
                          'objPalette': 'OBP0', 'priority': False})
        assert tiles, name
        frame_defs.append({'id': ident(f'{name}-frame'), 'tiles': tiles})
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
                'canvasOriginX': 8, 'canvasOriginY': 8, 'canvasWidth': CANVAS, 'canvasHeight': CANVAS,
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
    for name in ('player_car_0', 'player_van_0', 'player_motorcycle_0', 'player_scooter_0', 'courier_walk_0',
                 'beacon', 'beacon_pulse', 'stop_marker', 'depot_marker', 'car_door_open',
                 'traffic_red_0', 'traffic_blue_0', 'traffic_taxi_0', 'traffic_van_0', 'traffic_motorcycle_0',
                 'walker_a_0', 'walker_b_0', 'walker_c_0', 'walker_d_0',
                 'cone', 'cone_down', 'bin', 'bin_down', 'newsbox', 'newsbox_down', 'barrel', 'barrel_down', 'debris',
                 'bus_e', 'bus_w', 'bus_s', 'bus_n', 'streetcar_e', 'streetcar_w', 'streetcar_s', 'streetcar_n',
                 'ferry_s', 'ferry_n', 'gull_e_0', 'gull_w_0',
                 'police_0', 'officer_0', 'knock_walker_a_0', 'spark', 'bullet', 'arrow_0'):
        macro = 'TD_FRAME_' + name.upper().removesuffix('_0')
        lines.append(f'#define {macro} {index[name]}')
    lines.append(f'#define TD_KNOCK_FRAMES {len(A.knockdown_frames())}')
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
