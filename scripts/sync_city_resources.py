"""Write the sixteen native city scenes and their backgrounds.

Each compressed district (core, west, High Park, east) is drawn at double
scale and cut into four 1024 x 976 scenes (world2x.py). The art generators
write each scene's PNG, CGB attributes (original-art/<slug>_attributes.json)
and collision grid (content/scenes/<slug>.json); this script registers them
as GB Studio resources with stable ids, palettes and the TORONTO scene type,
and removes the four 1x scenes they replace. --check verifies without writing.
"""
from pathlib import Path
import json
import sys
import uuid
import world2x

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'project'
SCENES = PROJECT / 'project/scenes'
BACKGROUNDS = PROJECT / 'assets/backgrounds'
TW, TH = world2x.SCENE_W // 8, world2x.SCENE_H // 8
NAMESPACE = uuid.UUID('6f1d3c52-8d0e-5a7b-9c2f-1e4b7a9d0c35')
# Shared scene settings (the TORONTO scene type, background and sprite
# palettes, player sheet): taken from the scene template below.
TEMPLATE = {
    'type': 'TORONTO',
    'paletteIds': ['10c26dfc-656e-513f-8e7b-0b23cfe0fa5e', '895b5791-cab0-5e02-bbab-4486f6a2c7ca',
                   'a836a2bb-8434-583e-a9d0-7b7d68e058e5', '6973f5bd-54db-5433-8457-5ffa542510b9',
                   '31152bcc-b039-5ab5-a6a3-6e3666f9390d', 'cb66440b-a024-5c73-89c9-3eb74648aab2',
                   'f0854ef8-e994-56e6-9bf7-5b63d34d3a1b'],
    'playerSpriteSheetId': '95080aab-0201-545e-be5a-f5e79b9a693e',
}
OLD = ('toronto_city', 'toronto_west', 'toronto_high_park', 'toronto_east')


def compress(values):
    """GB Studio's run-length text for tile attributes and collisions."""
    output, last, count = [], None, 0
    for value in list(values) + [None]:
        if value != last:
            if count:
                output.append(f'{last:02x}' + ('!' if count == 1 else f'{count:x}+'))
            last, count = value, 0
        count += 1
    return ''.join(output)


def scene_id(slug):
    return str(uuid.uuid5(NAMESPACE, 'scene/' + slug))


def background_id(slug):
    return str(uuid.uuid5(NAMESPACE, 'background/' + slug))


def sprite_palettes():
    """Sprite palette ids are owned by create_sprites.py; keep the current ones."""
    for name in OLD + tuple(world2x.scene_slug(i) for i in range(16)):
        path = SCENES / name / 'scene.gbsres'
        if path.exists():
            return json.loads(path.read_text())['spritePaletteIds']
    raise SystemExit('no scene to take sprite palettes from')


def start_scene():
    """The scene of the new-game position (td_district_world.h)."""
    import create_district_world
    district, u, v = create_district_world.START
    return district, u, v


def resources():
    palettes = sprite_palettes()
    out = {}
    for new_id in range(16):
        slug = world2x.scene_slug(new_id)
        attrs = json.loads((PROJECT / f'original-art/{slug}_attributes.json').read_text())
        collisions = world2x.scene_grid(new_id)
        assert len(attrs) == len(collisions) == TW * TH, slug
        name = world2x.SCENE_NAMES[new_id].title()
        background = {
            '_resourceType': 'background', 'id': background_id(slug), 'name': name,
            'symbol': f'bg_{slug}', 'tileColors': compress(attrs), 'filename': f'{slug}.png',
            'width': TW, 'height': TH, 'imageWidth': world2x.SCENE_W, 'imageHeight': world2x.SCENE_H,
            'autoColor': False,
        }
        old, q = new_id >> 2, new_id & 3
        scene = {
            '_resourceType': 'scene', 'id': scene_id(slug), '_index': new_id + 1, 'type': TEMPLATE['type'],
            'name': name, 'symbol': f'scene_{slug}',
            # Editor canvas: districts west to east, quadrants in place.
            'x': 300 + [1, 0, 0, 2][old] * 700 + (q & 1) * 340, 'y': 40 + (q >> 1) * 330,
            'width': TW, 'height': TH, 'backgroundId': background_id(slug), 'tilesetId': '',
            'colorModeOverride': 'none', 'paletteIds': TEMPLATE['paletteIds'], 'spritePaletteIds': palettes,
            'autoFadeSpeed': 1, 'script': [], 'playerHit1Script': [], 'playerHit2Script': [], 'playerHit3Script': [],
            'collisions': compress(collisions), 'playerSpriteSheetId': TEMPLATE['playerSpriteSheetId'],
        }
        out[BACKGROUNDS / f'{slug}.png.gbsres'] = json.dumps(background, indent=2) + '\n'
        out[SCENES / slug / 'scene.gbsres'] = json.dumps(scene, indent=2) + '\n'
    settings_path = PROJECT / 'project/settings.gbsres'
    settings = json.loads(settings_path.read_text())
    district, u, v = start_scene()
    settings.update(startSceneId=scene_id(world2x.scene_slug(district)), startX=u // 8, startY=v // 8)
    out[settings_path] = json.dumps(settings, indent=2) + '\n'
    return out


def main(check=False):
    out = resources()
    stale = [OLD_PATH for name in OLD for OLD_PATH in (SCENES / name / 'scene.gbsres', BACKGROUNDS / f'{name}.png',
                                                         BACKGROUNDS / f'{name}.png.gbsres') if OLD_PATH.exists()]
    if check:
        for path, text in out.items():
            assert path.exists() and path.read_text() == text, f'stale resource: {path.relative_to(ROOT)}'
        assert not stale, f'1x scene resources remain: {stale}'
        print('Sixteen city scenes and backgrounds match their art.')
        return
    for path, text in out.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
    for path in stale:
        path.unlink()
    for name in OLD:
        if (SCENES / name).exists() and not any((SCENES / name).iterdir()):
            (SCENES / name).rmdir()
    print('Registered sixteen city scenes; roof/canopy tiles carry background priority.')


if __name__ == '__main__':
    main(check='--check' in sys.argv)
