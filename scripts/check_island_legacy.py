"""Check the frozen public v8 Island terrain/ordinal fixture before migration.

This is source data, not a captured cartridge save. Current five-scene terrain
is checked exhaustively against its 60-byte mask; no migration runs yet.
"""
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
FIXTURE_SHA256 = "98189fd41246488c93d1723513b166de97a812fc959c959ccede9381dd8e42ae"


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
    scene_raw = (ROOT / data["scene_path"]).read_bytes()
    assert hashlib.sha256(scene_raw).hexdigest() == data["scene_sha256"], "Current pre-migration Core scene differs from frozen source"
    grid = validator.decode(json.loads(scene_raw)["collisions"])
    assert len(grid) == 128 * 122
    assert hashlib.sha256(bytes(grid)).hexdigest() == data["collision_bytes_sha256"]
    membership, represented, packed_bytes = set(), set(), 0
    for region in data["regions"]:
        x, y, width, height = (region[key] for key in ("tile_x", "tile_y", "tile_width", "tile_height"))
        assert all(type(value) is int and value > 0 for value in (x, y, width, height))
        assert x + width <= 128 and y + height <= 122
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
            collision = grid[tile[1] * 128 + tile[0]]
            assert collision in (15, 16) and valid == (collision == 16)
            if valid:
                membership.add(tile)
                counted += 1
        assert counted == region["walkable_tiles"]
        packed_bytes += len(mask)
    # Reject missing walkable old Island ground outside the chosen rectangles.
    for y in range(108, 122):
        for x in range(128):
            assert (not grid[y * 128 + x] & 15) == ((x, y) in membership)
    assert data["validation"] == {"region_tiles": len(represented), "bitmap_bytes": packed_bytes,
                                   "walkable_tiles": len(membership), "public_car_tiles": 0}
    assert (len(represented), packed_bytes, len(membership)) == (475, 60, 354)
    campaign = json.loads((GAME / "content/campaign.json").read_text())
    assert [stop["id"] for stop in data["stops"]] == [20, 21, 22, 24, 25, 26]
    for stop in data["stops"]:
        assert stop == {key: campaign["stops"][stop["id"]].get(key, 0) for key in stop}
        assert (stop["u"] // 8, stop["v"] // 8) in membership
        assert stop["district"] == stop["reserved"] == 0
    assert len(data["quests"]) == 9
    for quest in data["quests"]:
        current = campaign["quests"][int(quest["id"].split("-")[1]) - 1]
        assert quest == {key: current[key] for key in quest}
        assert quest["kind_id"] == 7 and quest["required_vehicle"] == 255
    print("Frozen public v8 fixture: 475 terrain tiles/60 mask bytes/354 walkable tiles, six stable stop ordinals and nine Island contracts match current source; migration remains pending")


if __name__ == "__main__":
    check()
