"""Exercise the source checker on synthetic native seam collision resources.

The fixtures do not register new scenes or establish Toronto geography, ROM
banking, gameplay duration, emulator performance or hardware behaviour.
"""
import copy
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'games/toronto-dispatch/scripts'))
from check_district_world import check_seams


def fixture(horizontal, vehicle=True, lateral_centres=(384, 448)):
    normal, lateral = ('u', 'v') if horizontal else ('v', 'u')
    extent = 1024 if horizontal else 976
    districts = [
        {'id': 0, 'width_pixels': 1024, 'height_pixels': 976, 'atlas_x': 0, 'atlas_y': 0},
        {'id': 1, 'width_pixels': 1024, 'height_pixels': 976,
         'atlas_x': 1024 if horizontal else 0, 'atlas_y': 0 if horizontal else 976},
    ]
    a = {'district': 0, normal: extent - 24, lateral: lateral_centres[0]}
    b = {'district': 1, normal: 24, lateral: lateral_centres[1]}
    resources = {}
    for endpoint in (a, b):
        grid = [15] * (128 * 122)
        for y in range(122):
            for x in range(128):
                coordinate = y if horizontal else x
                if (endpoint[lateral] - 28) // 8 <= coordinate <= (endpoint[lateral] + 28) // 8:
                    grid[y * 128 + x] = 0 if vehicle else 16
        resources[endpoint['district']] = (128, 122, grid, [0] * len(grid))
    return {'districts': districts, 'portals': [
        {'name': 'synthetic compressed seam', 'from': a, 'to': b,
         'access': ['foot', 'vehicle'] if vehicle else ['foot'], 'modeled_crossing_pixels': 48},
    ]}, resources


def set_tile(resources, district, u, v, value):
    resources[district][2][(v // 8) * 128 + u // 8] = value


def main():
    checks = 0

    def accepted(world, resources):
        nonlocal checks
        before = copy.deepcopy((world, resources))
        result = check_seams(world, resources)
        assert len(result) == 2, 'Each accepted seam needs two directed native entries'
        assert (world, resources) == before, 'The read-only checker mutated source fixtures'
        checks += 1

    def rejected(world, resources, reason):
        nonlocal checks
        try:
            check_seams(world, resources)
        except AssertionError as error:
            assert reason in str(error), f'Expected {reason!r}, got {str(error)!r}'
        else:
            raise AssertionError(f'Invalid seam was accepted: {reason}')
        checks += 1

    for horizontal in (True, False):
        normal, lateral = ('u', 'v') if horizontal else ('v', 'u')
        extent = 1024 if horizontal else 976
        atlas_normal, atlas_lateral = ('atlas_x', 'atlas_y') if horizontal else ('atlas_y', 'atlas_x')
        # East/west and south/north, both road and foot-only, keep independent
        # lateral centres. Reversing authored endpoints must preserve validity.
        for vehicle in (True, False):
            for reverse in (False, True):
                world, resources = fixture(horizontal, vehicle)
                if reverse:
                    portal = world['portals'][0]
                    portal['from'], portal['to'] = portal['to'], portal['from']
                accepted(world, resources)

        world, resources = fixture(horizontal)
        world['districts'][1][atlas_lateral] = 32
        accepted(world, resources)

        # Both inclusive native lateral bounds remain valid with their full
        # approach lanes, including the outermost accepted walking offsets.
        world, resources = fixture(horizontal, lateral_centres=(32, 943 if horizontal else 991))
        accepted(world, resources)

        world, resources = fixture(horizontal)
        world['portals'][0]['to'][normal] = extent - 24
        rejected(world, resources, 'insets must be opposite')

        world, resources = fixture(horizontal)
        endpoint = world['portals'][0]['to']
        endpoint[normal], endpoint[lateral] = 384, 24
        rejected(world, resources, 'Mixed seam axes')

        for value in (31, 944 if horizontal else 992):
            world, resources = fixture(horizontal)
            world['portals'][0]['from'][lateral] = value
            rejected(world, resources, 'outside native')

        world, resources = fixture(horizontal)
        world['districts'][1][atlas_normal] += 8
        rejected(world, resources, 'edges are not adjacent')

        world, resources = fixture(horizontal)
        world['districts'][1][atlas_lateral] = 976 if horizontal else 1024
        rejected(world, resources, 'edges do not overlap')

        # A blocked outermost walking offset must fail on either end.
        for side, offset in (('from', -28), ('to', 28)):
            world, resources = fixture(horizontal)
            endpoint = world['portals'][0][side]
            position = {normal: endpoint[normal], lateral: endpoint[lateral] + offset}
            set_tile(resources, endpoint['district'], position['u'], position['v'], 15)
            rejected(world, resources, 'Blocked foot seam lane')

        # Foot-only terrain at the outer car approach is still walkable; the
        # full half5 road footprint must reject it independently.
        world, resources = fixture(horizontal)
        endpoint = world['portals'][0]['from']
        position = {normal: extent - 1, lateral: endpoint[lateral] + 18}
        set_tile(resources, endpoint['district'], position['u'], position['v'], 16)
        rejected(world, resources, 'Blocked native car seam lane')

        # The lower-inset half8 check reaches an interior tile not covered by
        # the half5 outer approach or the foot centre samples.
        world, resources = fixture(horizontal)
        endpoint = world['portals'][0]['to']
        position = {normal: 32, lateral: endpoint[lateral]}
        set_tile(resources, endpoint['district'], position['u'], position['v'], 16)
        rejected(world, resources, 'Blocked centred car seam footprint')

        world, resources = fixture(horizontal, False)
        endpoint = world['portals'][0]['to']
        resources[endpoint['district']] = fixture(horizontal, True)[1][endpoint['district']]
        rejected(world, resources, 'Foot-only seam admits a car')

        world, resources = fixture(horizontal)
        world['portals'].append(copy.deepcopy(world['portals'][0]))
        rejected(world, resources, 'Duplicate seam pair')

    world, resources = fixture(False)
    world['portals'][0]['to']['district'] = 2
    rejected(world, resources, 'Unregistered seam district')
    world, resources = fixture(False)
    world['portals'][0]['from']['u'] = True
    rejected(world, resources, 'Non-integer seam endpoint')
    print(f'Native seam source checker: {checks} synthetic geometry/footprint cases passed; no new playable district or ROM evidence.')


if __name__ == '__main__':
    main()
