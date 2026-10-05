"""Pin saved campaign identities while relocating six Island endpoints.

Only six u/v/district triples and the separate reviewed stop18 display name may
normalize for historical checks. The display exception never moves a stop.
Native tables must separately match current source; historical normalization is
solely for old-prefix hashes, never a substitute for current geometry checks.
"""
import hashlib
import json
from pathlib import Path
from stop_names import historical_name

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "content/districts/island_campaign_v8.json"
FIXTURE_SHA256 = "ed3354c8f0e726f369c86f8f36f4373427a56d2d920bbca078252bed8890053b"
STOPS_SHA256 = "f21c09b432f6dff2257af7c9a54d8351d3bc2e79dd68dc270744b8b80ea45a14"
QUESTS_SHA256 = "64c3f534bd12c03dd50c825691e555974cc9143384199ea8fb5f0f0edb6bde5e"
ISLAND_DISTRICT = 5
OLD_ISLAND_GEOMETRY = {20: (444, 928, 0), 21: (720, 920, 0), 22: (848, 896, 0),
                       24: (560, 928, 0), 25: (760, 944, 0), 26: (912, 912, 0)}
NEW_ISLAND_GEOMETRY = {20: (320, 280, 5), 21: (512, 448, 5), 22: (920, 280, 5),
                       24: (160, 600, 5), 25: (512, 744, 5), 26: (904, 440, 5)}
ISLAND_IDS = frozenset(OLD_ISLAND_GEOMETRY)
STOP_FIELDS = ("id", "u", "v", "name", "transit", "district", "reserved")
QUEST_FIELDS = ("id", "title", "brief", "kind_id", "required_vehicle",
                "min_completed", "route", "time_limit_seconds", "reward")


def canonical_sha(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def frozen_campaign():
    raw = FIXTURE.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == FIXTURE_SHA256, "Immutable pre-Islands campaign fixture changed"
    data = json.loads(raw)
    assert tuple(data["native_stop_fields"]) == STOP_FIELDS
    assert tuple(data["native_quest_fields"]) == QUEST_FIELDS
    assert len(data["stops"]) == 59 and len(data["quests"]) == 96
    assert canonical_sha(data["stops"]) == STOPS_SHA256
    assert canonical_sha(data["quests"]) == QUESTS_SHA256
    return data


def historical_stop(stop):
    """Return a copy with only an allowed Island geometry restored to v8."""
    result = historical_name(stop)
    index = stop["id"]
    if index in ISLAND_IDS:
        geometry = (stop["u"], stop["v"], stop.get("district", 0))
        assert geometry in (OLD_ISLAND_GEOMETRY[index], NEW_ISLAND_GEOMETRY[index]), f"Unapproved Island geometry: {index}"
        old = frozen_campaign()["stops"][index]
        assert all(stop.get(field, 0) == old[field] for field in ("name", "transit", "reserved")), f"Island identity changed: {index}"
        assert not stop.get("foot_only", False) and "parking_anchor" not in stop, "Island walking is district access, not a parking flag"
        result.update(zip(("u", "v", "district"), OLD_ISLAND_GEOMETRY[index]))
    return result


def validate_preserved_campaign(campaign):
    """Pin all 59/96 native fields; planning metadata may be refreshed."""
    # This historical oracle still pins exactly59/96; new appendages have
    # a separate North64/104 gate and cannot broaden Island exceptions.
    assert len(campaign["stops"]) >= 59 and len(campaign["quests"]) >= 96
    stops = [{field: historical_stop(stop).get(field, 0) for field in STOP_FIELDS}
             for stop in campaign["stops"][:59]]
    quests = [{field: quest[field] for field in QUEST_FIELDS} for quest in campaign["quests"][:96]]
    assert canonical_sha(stops) == STOPS_SHA256, "Existing 59 stop identities changed outside six geometry exceptions"
    assert canonical_sha(quests) == QUESTS_SHA256, "Existing 96 native contracts/completion ordinals changed"


def validate_relocated_stops(stops, metadata):
    candidates = metadata["stop_candidates"]
    assert {item["id"] for item in candidates} == ISLAND_IDS and len(candidates) == 6
    for candidate in candidates:
        index = candidate["id"]
        stop = stops[index]
        assert (stop["u"], stop["v"], stop.get("district", 0)) == NEW_ISLAND_GEOMETRY[index]
        assert (candidate["x"], candidate["y"], ISLAND_DISTRICT) == NEW_ISLAND_GEOMETRY[index]
        assert (stop["name"], stop["transit"], stop.get("reserved", 0)) == (candidate["name"], candidate["transit"], candidate["reserved"])
        historical_stop(stop)


def relocate_island_stops(stops, world):
    """Copy generator inputs into the explicitly registered sixth-scene layout."""
    result = [dict(stop) for stop in stops]
    if len(world['districts']) > ISLAND_DISTRICT:
        assert world['districts'][ISLAND_DISTRICT]['scene'] == 'toronto_islands'
        for stop in result:
            if stop['id'] in ISLAND_IDS:
                historical_stop(stop)
                stop.update(zip(('u', 'v', 'district'), NEW_ISLAND_GEOMETRY[stop['id']]))
    return result
