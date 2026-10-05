#!/usr/bin/env python3
"""Author eight Port Lands courier contracts from registered native terrain.

Writes only port_lands_jobs.json. --check and --diff never write files. Preserve
all 51 existing stop identities and 88 native contracts, including Queen
platforms, with only six explicitly pinned Island geometries relocated.
Route/timing models are planning data, not observed gameplay or duration proof.
"""
import argparse
from collections import deque
import difflib
import json
import math
from pathlib import Path
import re
import sys
from island_campaign import historical_stop, relocate_island_stops

from create_district_jobs import (BASE_QUEST_FIELDS, FOOT_SPEED, SPEEDS,
                                  RouteModel, canonical, decode_grid, point,
                                  read_json, sha)

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "content/districts/port_lands_jobs.json"
PREFIX_STOPS, PREFIX_QUESTS = 51, 88
PREFIX_STOP_FIELDS = ("id", "u", "v", "name", "transit", "district", "reserved")
PREFIX_STOPS_SHA256 = "68f6104038ab9724ca197de13ecef7528ab5e8a4713ee8d05cff8b8004a73325"
PREFIX_QUESTS_SHA256 = "ae5d1177fc297337b7efe449144d6ffbe188c333d3607d6710080c9d065e130a"
STOP_KEYS = ("channel_stage", "fire_hall_books", "crane_walk", "turning_basin",
             "unwin_works", "beach_mail", "polson_quay", "south_park")
STOP_NAMES = ("CHANNEL STAGE", "FIRE HALL BOOKS", "CRANE WALK", "TURNING BASIN",
              "UNWIN WORKS", "BEACH MAIL", "POLSON PACKET", "RIVERBANK PARCEL")

# Existing native handoff/condition/vehicle rules; no loading timer, signatures,
# new cargo UI or unregistered western Port Lands shortcut is implied.
CONTRACTS = (
    ("CHANNEL STOCK RUN", ("BULKY STAGE STOCK", "TRUCK BRAKE EARLY"), 3, 1, 8,
     [0, 51], ["Collect stage stock at Union", "Unload at the Channel Stage apron"]),
    ("FIRE HALL BOOKS", ("COMMUNITY BOOK BOX", "PARK THEN WALK"), 0, 255, 3,
     [37, 52], ["Collect community books at Riverside", "Walk to the Fire Hall Books entrance"]),
    ("CRANE WALK PACKET", ("POCKET RELAY KIT", "PARK OR USE QUEEN"), 4, 255, 8,
     [0, 53], ["Collect a pocket relay packet at Union", "Walk to the Crane Walk handoff"]),
    ("TURNING BASIN LOAD", ("STAGE EQUIPMENT", "UNWIN THEN YARD"), 3, 1, 8,
     [51, 55, 54], ["Load stage equipment at Channel Stage", "Handoff equipment at Unwin Works",
                    "Deliver the reusable cases to Turning Basin"]),
    ("UNWIN GLASS", ("FRAGILE GLASS KIT", "BRAKE BEFORE TURNS"), 1, 0, 6,
     [39, 55], ["Collect the fragile kit at Carlaw Works", "Deliver intact glass at Unwin Works"]),
    ("BEACH MAIL", ("RIVER AND BEACH", "TWO FOOT HANDOFFS"), 0, 255, 12,
     [52, 58, 56], ["Collect the mail pouch at Fire Hall Books", "Walk to the Riverbank Parcel handoff",
                    "Deliver the final mail at the Beach entrance"]),
    ("HARBOUR PAPER RUN", ("URGENT PORT FILE", "MOTORCYCLE ROUTE"), 2, 2, 6,
     [57, 1], ["Collect harbour files at Polson Packet", "Reach St Lawrence before the cutoff"]),
    ("STAGE RETURN KIT", ("YARD RETURN PAPERS", "ORIGINAL BACK YARD"), 6, 0, 12,
     [54, 51, 39, 54], ["Collect return papers at Turning Basin", "Handoff the stage copy at Channel Stage",
                        "Collect the dispatch copy at Carlaw Works", "Return the original kit to Turning Basin"]),
)


def native_table(code, name):
    found = re.search(r"\b" + re.escape(name) + r"\s*\[[^;=]+\]\s*=\s*\{(.*?)\n\};", code, re.S)
    assert found, f"Missing native prefix table: {name}"
    return found.group(1)


def preserved_prefix(campaign):
    stops, jobs = campaign["stops"][:PREFIX_STOPS], campaign["quests"][:PREFIX_QUESTS]
    assert len(stops) == PREFIX_STOPS and [s["id"] for s in stops] == list(range(PREFIX_STOPS))
    assert len(jobs) == PREFIX_QUESTS and [q["id"] for q in jobs] == [f"contract-{i:02d}" for i in range(1, 89)]
    normalized_stops = [{field: historical_stop(stop).get(field, 0) for field in PREFIX_STOP_FIELDS} for stop in stops]
    normalized_jobs = [{field: job[field] for field in BASE_QUEST_FIELDS} for job in jobs]
    assert sha(canonical(normalized_stops)) == PREFIX_STOPS_SHA256, "Existing 51 native stop fields changed"
    assert sha(canonical(normalized_jobs)) == PREFIX_QUESTS_SHA256, "Existing 88 native contract fields changed"
    code = (ROOT / "project/plugins/toronto-driving/engine/src/td_content.c").read_text()
    rows = re.findall(r'\{(\d+),(\d+),"([^"]+)",(\d+),(\d+),(\d+)\}', native_table(code, "td_stops"))
    assert len(rows) >= PREFIX_STOPS, "Native stop prefix is unavailable"
    for stop, row in zip(stops, rows[:PREFIX_STOPS]):
        u, v, name, transit, district, flags = row
        native = historical_stop(dict(id=stop['id'], u=int(u), v=int(v), name=name,
                                      transit=int(transit), district=int(district), reserved=int(flags)))
        historical = historical_stop(stop)
        assert all(native.get(field, 0) == historical.get(field, 0) for field in PREFIX_STOP_FIELDS), f"Native stop prefix differs: {stop['id']}"
    rows = re.findall(r'\{"([^"]+)",(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),\{([\d,]+)\}\}', native_table(code, "td_jobs"))
    assert len(rows) >= PREFIX_QUESTS, "Native contract prefix is unavailable"
    for job, row in zip(jobs, rows[:PREFIX_QUESTS]):
        title, *numbers, route = row
        expected = [job["kind_id"], len(job["route"]), job["required_vehicle"], job["min_completed"], job["time_limit_seconds"], job["reward"]]
        assert title == job["title"] and list(map(int, numbers)) == expected, f"Native contract prefix differs: {job['id']}"
        assert list(map(int, route.split(","))) == job["route"] + [255] * (12 - len(job["route"])), f"Native route prefix differs: {job['id']}"
    briefs = re.findall(r'"([^"]*)"', native_table(code, "td_briefs"))
    assert briefs[:PREFIX_QUESTS] == ["".join(line.ljust(18) for line in job["brief"]) for job in jobs], "Existing native brief prefix differs"
    return stops, jobs, normalized_stops, normalized_jobs


def registered_port(world, metadata):
    districts = world["districts"]
    assert len(districts) >= 5 and [d["id"] for d in districts] == list(range(len(districts)))
    assert [d["scene"] for d in districts[:5]] == ["toronto_city", "toronto_west", "toronto_high_park", "toronto_east", "toronto_port_lands"]
    header = (ROOT / "project/plugins/toronto-driving/engine/include/td_district.h").read_text()
    count = re.search(r"^\s*#define\s+TD_DISTRICT_COUNT\s+(\d+)\s*$", header, re.M)
    assert count and int(count.group(1)) == len(districts), "Native district count differs from actual registry"
    entry = districts[4]
    scene = read_json(ROOT / "project/project/scenes" / entry["scene"] / "scene.gbsres")
    assert scene["_resourceType"] == "scene" and scene["type"] == "TORONTO" and scene["symbol"] == entry["symbol"]
    assert (scene["width"], scene["height"]) == (128, 122)
    assert metadata["id"] == 4 and metadata["dimensions"] == [1024, 976]
    assert decode_grid(scene["collisions"]) == metadata["collisions"], "Registered Port Lands collisions differ from authored artwork"
    ports = [p for p in world["portals"] if 4 in (p["from"]["district"], p["to"]["district"])]
    assert len(ports) == 1, "Use only the registered Leslie / Port Lands seam"
    portal = ports[0]
    assert portal["name"] == "Leslie / Port Lands"
    assert portal["from"] == {"district": 3, "u": 816, "v": 952}
    assert portal["to"] == {"district": 4, "u": 912, "v": 24}
    assert set(portal["access"]) == {"foot", "vehicle"} and portal["modeled_crossing_pixels"] == 48
    return scene


def clear_body(scene, u, v, half, vehicle=False):
    grid, width, height = scene["grid"], scene["width"], scene["height"]
    left, right, top, bottom = (u - half) // 8, (u + half) // 8, (v - half) // 8, (v + half) // 8
    return (0 <= left <= right < width and 0 <= top <= bottom < height and
            all(not (grid[y * width + x] if vehicle else grid[y * width + x] & 15)
                for y in range(top, bottom + 1) for x in range(left, right + 1)))


def foot_component(scene):
    origin = (912 // 8, 24 // 8)
    assert clear_body(scene, origin[0] * 8 + 4, origin[1] * 8 + 4, 2)
    queue, seen = deque([origin]), {origin}
    while queue:
        x, y = queue.popleft()
        for nxt in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
            if nxt not in seen and clear_body(scene, nxt[0] * 8 + 4, nxt[1] * 8 + 4, 2):
                seen.add(nxt); queue.append(nxt)
    return seen


def author():
    campaign = read_json(ROOT / "content/campaign.json")
    base_stops, base_jobs, normalized_stops, normalized_jobs = preserved_prefix(campaign)
    world_path, metadata_path = ROOT / "content/districts/world.json", ROOT / "content/districts/port_lands_art.json"
    world, metadata = read_json(world_path), read_json(metadata_path)
    native_scene = registered_port(world, metadata)
    scene = {"grid": decode_grid(native_scene["collisions"]), "width": native_scene["width"], "height": native_scene["height"]}
    connected = foot_component(scene)
    candidates = metadata["stop_candidates"]
    assert [candidate["key"] for candidate in candidates] == list(STOP_KEYS), "Preserve Port Lands client order"
    authored_stops = []
    for index, candidate in enumerate(candidates, PREFIX_STOPS):
        foot = candidate["foot_only"]
        assert type(foot) is bool and foot == (index in (52, 53, 56, 58))
        assert candidate["fictional_service_point"] is True
        stop = {"id": index, "u": candidate["x"], "v": candidate["y"], "name": STOP_NAMES[index - PREFIX_STOPS],
                "transit": 0, "district": 4, "reserved": int(foot), "foot_only": foot,
                "reference_label": candidate["name"], "geography_reference": metadata["source_research"],
                "location_notice": "Original fictional courier service entrance on compressed terrain; not a surveyed loading door"}
        assert stop["name"].isascii() and len(stop["name"]) <= 18, f"Native stop label too long: {index}"
        assert clear_body(scene, stop["u"], stop["v"], 2), f"Blocked full-foot client: {index}"
        assert (stop["u"] // 8, stop["v"] // 8) in connected, f"Disconnected full-foot client: {index}"
        if foot:
            u, v = candidate["parking_anchor"]
            stop["parking_anchor"] = {"u": u, "v": v}
            stop["access_notice"] = "Native delivery requires an on-foot handoff; auxiliary parking guidance does not require returning to the vehicle between foot clients"
            assert clear_body(scene, u, v, 8, True), f"Blocked full-car parking anchor: {index}"
            assert (u // 8, v // 8) in connected
            assert not clear_body(scene, stop["u"], stop["v"], 5, True), f"Foot-only client admits car body: {index}"
        else:
            assert clear_body(scene, stop["u"], stop["v"], 8, True), f"Blocked full-car loading bay: {index}"
        authored_stops.append(stop)
    stops = relocate_island_stops(base_stops, world) + authored_stops
    model = RouteModel(world, stops)
    last_mile = []
    for stop in authored_stops:
        node = point(4, stop["u"], stop["v"])
        assert model.usable(node, False)
        if stop["foot_only"]:
            anchor = stop["parking_anchor"]
            parked = point(4, anchor["u"], anchor["v"])
            assert model.usable(parked, True) and not model.usable(node, True)
            walking, _ = model.shortest(node, parked, False)
            assert 0 < walking <= 256, "Port Lands last-mile approach must remain a short purposeful walk"
            last_mile.append({"stop": stop["id"], "walking_pixels": walking})
        else:
            assert model.usable(node, True)
    jobs = []
    for index, (title, brief, kind, vehicle, gate, route, objectives) in enumerate(CONTRACTS, 89):
        assert title.isascii() and len(title) <= 18 and len(brief) == 2 and all(line.isascii() and len(line) <= 18 for line in brief), f"Native contract text exceeds 18 ASCII characters: {index}"
        assert 2 <= len(route) <= 12 and len(route) == len(objectives)
        assert all(0 <= stop < 59 for stop in route) and all(a != b for a, b in zip(route, route[1:]))
        assert 3 <= gate <= 12 and kind in (0, 1, 2, 3, 4, 6)
        assert any(stops[stop]["district"] == 4 for stop in route)
        assert vehicle == 255 or all(not stops[stop].get("foot_only", False) for stop in route)
        if kind == 3:
            assert vehicle == 1
        if kind == 6:
            assert route[0] == route[-1], "Port return contract must actually return to its starting yard"
        legs, road, walking = [], 0, 0
        for first, last in zip(route, route[1:]):
            distance, walk, seams = model.stop_leg(stops[first], stops[last])
            road += distance; walking += walk
            legs.append({"from_stop": first, "to_stop": last, "from_district": stops[first]["district"],
                         "to_district": stops[last]["district"], "vehicle_pixels": distance,
                         "required_walking_pixels": walk, "shortest_modeled_seams": seams})
        fastest, slowest = (SPEEDS[vehicle], SPEEDS[vehicle]) if vehicle != 255 else (max(SPEEDS.values()), min(SPEEDS.values()))
        fast_time, slow_time = road / fastest + walking / FOOT_SPEED, road / slowest + walking / FOOT_SPEED
        handling = len(route) * 5
        deadline = math.ceil(((60 if kind == 2 else 90) + (slow_time + handling) * (1.4 if kind == 2 else 1.8)) / 5) * 5
        jobs.append({"id": f"contract-{index:02d}", "title": title, "brief": list(brief),
                     "chapter": "Port Lands loading and park rounds", "kind": campaign["quest_types"][kind], "kind_id": kind,
                     "required_vehicle": vehicle, "min_completed": gate, "route": route,
                     "time_limit_seconds": deadline, "reward": 85 + math.ceil(road / 40) + (len(route) - 1) * 12 + (25 if kind in (1, 3) else 0),
                     "stages": [{"stop_id": stop, "district": stops[stop]["district"], "objective": objective} for stop, objective in zip(route, objectives)],
                     "objective_notice": "Ordered native parcel handoffs, condition/timer/vehicle rules and returns; no signature UI, loading delay, new cargo system or new transit service is implied",
                     "timing_design": {"planning_only": True, "measured_duration_seconds": None, "deadline_status": "Initial estimate; ordinary-input tuning pending",
                                       "vehicle_route_pixels": road, "required_walking_pixels": walking,
                                       "normalized_moving_seconds_fast": round(fast_time, 1), "normalized_moving_seconds_slow": round(slow_time, 1),
                                       "assumed_interaction_allowance_seconds": handling, "legs": legs,
                                       "basis": "Actual registered collision-grid tile-centre paths and the sole Leslie Port Lands seam. Driving uses integer-settled cardinal speed estimates; foot-client modeling returns to parking between legs, although the native game permits continuous walking. Excludes traffic, acceleration, transit, real handling and repositioning. Not observed duration."}})
    all_jobs = base_jobs + jobs
    assert len(jobs) == 8 and len({tuple(job["route"]) for job in all_jobs}) == 96
    assert len({job["title"] for job in all_jobs}) == 96
    assert {stop for job in jobs for stop in job["route"] if stop >= 51} == set(range(51, 59))
    completed = 0
    while completed < len(all_jobs):
        unlocked = sum(job["min_completed"] <= completed for job in all_jobs)
        assert unlocked > completed, f"Progression deadlock at {completed} completions"
        completed = unlocked
    return {"schema_version": 1, "status": "authored; native compilation and ordinary-input Port Lands contract playtests pending",
            "scope": "Eight purposeful Port Lands courier contracts and eight fictional clients appended after the preserved native 88/51 prefix; no new transit service or full Old Toronto coverage is implied",
            "duration_target_minutes": 120, "duration_verified": False,
            "duration_notice": "Counts, route models, deadlines and assumed handling do not establish two hours or enjoyment. Measure purposeful ordinary gameplay.",
            "base_stop_count": 51, "base_quest_count": 88, "result_stop_count": 59, "result_quest_count": 96,
            "preserved_base_stops_sha256": sha(canonical(normalized_stops)), "preserved_base_quests_sha256": sha(canonical(normalized_jobs)),
            "preserved_base_stop_fields": list(PREFIX_STOP_FIELDS), "preserved_base_quest_fields": list(BASE_QUEST_FIELDS),
            "world_sha256": sha(world_path.read_bytes()), "collision_resources": model.resources,
            "geography_metadata": [{"path": "content/districts/port_lands_art.json", "sha256": sha(canonical(metadata)),
                                    "notice": "Source-derived geography and original compressed artwork; retain source attribution and distribution notices"}],
            "normalized_cardinal_speeds_pixels_per_second": {"car": SPEEDS[0], "truck": SPEEDS[1], "motorcycle": SPEEDS[2], "scooter": SPEEDS[3], "walking": FOOT_SPEED},
            "transit_notice": "All eight clients have transit=0. Existing Queen platforms remain IDs 43..50 and can shorten light-cargo journeys before walking through Leslie; 114 boarding is not implemented by this content.",
            "geography_notice": "All interdistrict Port Lands travel uses East Leslie (816,952) to Port Lands (912,24). Core Cherry/Carlaw gateways remain closed; use only the registered road/water/bridge graph.",
            "save_notice": "Append-only native IDs fit the existing 16-byte completion bitmap and 58-byte v8 state. Older 88-contract ROMs reject saved new job IDs/completion bits; backward loading is not promised.",
            "native_stop_flags": {"reserved_bit_0": "Foot-only delivery; reject car-door proximity and require on-foot handoff"},
            "validation": {"full_foot_component_tiles": len(connected), "full_foot_half_pixels": 2, "full_vehicle_half_pixels": 8,
                           "foot_only_stop_ids": [52, 53, 56, 58], "parking_walks": last_mile},
            "stops": authored_stops, "quests": jobs}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="Verify Port Lands JSON without writing")
    mode.add_argument("--diff", action="store_true", help="Print a JSON diff without writing; exit1 when stale")
    args = parser.parse_args()
    try:
        expected = json.dumps(author(), indent=2) + "\n"
        current = OUTPUT.read_text() if OUTPUT.exists() else ""
        if args.diff:
            sys.stdout.writelines(difflib.unified_diff(current.splitlines(True), expected.splitlines(True),
                                                     fromfile="port_lands_jobs.json", tofile="authored port_lands_jobs.json"))
        if args.check or args.diff:
            if current != expected:
                if args.check:
                    print("Port Lands JSON is stale; regenerate after native scene registration.", file=sys.stderr)
                return 1
        else:
            OUTPUT.parent.mkdir(parents=True, exist_ok=True); OUTPUT.write_text(expected)
        print("Port Lands source: eight jobs/eight clients; body routes, 88 native job fields/briefs and 51 stop identities with six declared Island geometry exceptions, plus progression verified. Native play and measured duration require separate evidence.", file=sys.stderr)
        return 0
    except (AssertionError, KeyError, ValueError, OSError) as error:
        print(f"Cannot author Port Lands jobs: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
