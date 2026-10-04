"""Check immutable v8 Island terrain/ordinals independently after relocation.

This is source data, not a captured cartridge save. Current five-scene terrain
is retained as raw captured tiles, not regenerated from the new scene or mask.
Current Core water and declared six-stop relocation are separate source gates.
"""
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
FIXTURE_SHA256 = "98189fd41246488c93d1723513b166de97a812fc959c959ccede9381dd8e42ae"
RAW_GEOMETRY_SHA256 = "88e164ff172a4c18de30616cf06898c190a0eaf546852e485159dfe80e07585d"


def check():
    path = GAME / "content/districts/island_legacy_v8.json"
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == FIXTURE_SHA256, "Immutable legacy Island fixture changed"
    data = json.loads(raw)
    assert data["captured_source_commit"] == "d6ee4521921edd685b4ff89e81f7392ec995e6a6"
    assert (data["native_save_version"], data["native_state_bytes"], data["native_district"]) == (8, 58, 0)
    spec = importlib.util.spec_from_file_location("legacy_grid", GAME / "scripts/check_campaign.py")
    validator = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(validator)
    sys.path.insert(0, str(GAME / 'scripts'))
    from island_campaign import historical_stop, validate_preserved_campaign, validate_relocated_stops
    from district_sources import read_district_art
    geometry_raw = (ROOT / 'tests/engine/legacy_island_geometry.json').read_bytes()
    assert hashlib.sha256(geometry_raw).hexdigest() == RAW_GEOMETRY_SHA256, 'Immutable captured v8 tile subset changed'
    geometry = json.loads(geometry_raw)
    assert geometry['decodedCollisionSha256'] == data['collision_bytes_sha256']
    assert len(geometry['regions']) == 3
    scene_raw = (ROOT / data["scene_path"]).read_bytes()
    grid = validator.decode(json.loads(scene_raw)["collisions"])
    assert len(grid) == 128 * 122
    membership, represented, packed_bytes = set(), set(), 0
    for region, captured in zip(data["regions"], geometry['regions']):
        x, y, width, height = (region[key] for key in ("tile_x", "tile_y", "tile_width", "tile_height"))
        assert all(type(value) is int and value > 0 for value in (x, y, width, height))
        assert x + width <= 128 and y + height <= 122
        assert (x, y, width, height) == tuple(captured[key] for key in ('left', 'top', 'width', 'height'))
        tiles = bytes.fromhex(captured['tilesHex'])
        assert len(tiles) == width * height and set(tiles) == {15, 16}
        mask = bytes.fromhex(region["foot_bitmap_hex"])
        size = width * height
        assert region["bit_order"] == "LSB-first row-major" and len(mask) == (size + 7) // 8
        if size % 8:
            assert not mask[-1] >> (size % 8), "Unused legacy mask bits must be zero"
        counted = 0
        for index in range(size):
            tile = (x + index % width, y + index // width)
            assert tile not in represented, "Overlapping legacy regions"
            represented.add(tile)
            valid = bool(mask[index // 8] & (1 << (index % 8)))
            collision = tiles[index]
            assert collision in (15, 16) and valid == (collision == 16)
            if valid:
                membership.add(tile)
                counted += 1
        assert counted == region["walkable_tiles"]
        packed_bytes += len(mask)
    # New Core no longer presents the historical strips as navigable islands.
    # The historical gate uses independent raw captured tiles above; it does not
    # compare changed current scene bytes to the old whole-scene fingerprint.
    for y in range(108, 122):
        for x in range(128):
            assert grid[y * 128 + x] == 15, 'Former Core Island strip remains passable'
    assert data["validation"] == {"region_tiles": len(represented), "bitmap_bytes": packed_bytes,
                                   "walkable_tiles": len(membership), "public_car_tiles": 0}
    assert (len(represented), packed_bytes, len(membership)) == (475, 60, 354)
    campaign = json.loads((GAME / "content/campaign.json").read_text())
    validate_preserved_campaign(campaign)
    world = json.loads((GAME / 'content/districts/world.json').read_text())
    assert len(world['districts']) == 7 and world['districts'][5]['scene']=='toronto_islands' and world['districts'][6]['scene']=='toronto_north'
    validate_relocated_stops(campaign['stops'], read_district_art(world['districts'][5]))
    assert [stop["id"] for stop in data["stops"]] == [20, 21, 22, 24, 25, 26]
    for stop in data["stops"]:
        historical = historical_stop(campaign['stops'][stop['id']])
        assert stop == {key: historical.get(key, 0) for key in stop}
        assert (stop["u"] // 8, stop["v"] // 8) in membership
        assert stop["district"] == stop["reserved"] == 0
    assert len(data["quests"]) == 9
    for quest in data["quests"]:
        current = campaign["quests"][int(quest["id"].split("-")[1]) - 1]
        assert quest == {key: current[key] for key in quest}
        assert quest["kind_id"] == 7 and quest["required_vehicle"] == 255
    print("Frozen public v8 source: 475 independently captured terrain tiles/60 mask bytes/354 walking tiles unchanged; six declared geometry relocations, all 96 native contracts and removed Core strips verified. Save migration/native/hardware evidence is separate")


if __name__ == "__main__":
    check()
