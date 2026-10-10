#!/usr/bin/env python3
"""Author eight original eastern package jobs from registered native terrain.

Writes only east_jobs.json. --check and --diff never mutate source or engine
files. Route estimates require the actual fourth scene and remain planning data,
not observed playtime, proof of enjoyment or a completed two-hour campaign.
"""
import argparse
import difflib
import json
import math
from pathlib import Path
import re
import sys
import world2x

from create_district_jobs import (BASE_QUEST_FIELDS, FOOT_SPEED, SPEEDS,
                                  RouteModel, canonical, decode_grid, point,
                                  read_json, sha)

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "content/districts/east_jobs.json"
PREFIX_STOPS, PREFIX_QUESTS = 35, 80
PREFIX_STOP_FIELDS = ("id", "u", "v", "name", "transit", "district", "reserved")
PREFIX_STOPS_SHA256 = "64a094c134e3d3dd077a8a3296ccd9605a3ab2f43d2f03c734c7fd4f993d2953"
# Updated 2026-10-10: the western time limits follow the slower, heavier cars.
PREFIX_QUESTS_SHA256 = "75f94296936f5e8b4749ab2f73c936cfded6e7878275cbf139d8080031c0016e"
STOP_KEYS = ("danforth_hall", "withrow_walk", "riverside_queen", "gerrard_pape",
             "carlaw_works", "leslie_queen", "greenwood_walk", "ashbridge_queen")
STOP_NAMES = ("DANFORTH HALL", "WITHROW POST", "RIVERSIDE QUEEN", "GERRARD / PAPE",
              "CARLAW WORKS", "LESLIE / QUEEN", "GREENWOOD POST", "ASHBRIDGE QUEEN")

# Concrete handoffs use existing native parcel/fragile/express/freight/relay/
# return rules. Fictional parcel descriptions imply no new cargo UI or delays.
CONTRACTS = (
    ("EAST FIRST ROUND", ("EAST SHOP PARCELS", "QUEEN TO DANFORTH"), 0, 255, 3,
     [0, 37, 35, 0],
     ["Collect east shop parcels", "Riverside shop handoff", "Danforth hall handoff", "Return reusable satchel"]),
    ("DANFORTH ART KIT", ("FRAMED PRINT KITS", "BRAKE BEFORE TURNS"), 1, 0, 6,
     [5, 35, 38, 37, 5],
     ["Collect gallery print kits", "Danforth event handoff", "Gerrard print handoff", "Riverside gallery handoff", "Return protective cases"]),
    ("EAST FILE CUTOFF", ("URGENT EAST FILES", "MOTORCYCLE ROUTE"), 2, 2, 6,
     [0, 40, 42, 38, 0],
     ["Collect timed print files", "Leslie shop handoff", "Ashbridge service handoff", "Gerrard copy-shop handoff", "Return dispatch copies"]),
    ("CARLAW STOCK RUN", ("BULKY SHOP STOCK", "TRUCK BRAKE EARLY"), 3, 1, 8,
     [11, 39, 42, 37, 11],
     ["Collect bayfront stock crates", "Carlaw workshop handoff", "Ashbridge shop handoff", "Riverside stock handoff", "Return reusable crates"]),
    ("EAST RAIL RELAY", ("POCKET PARCEL KIT", "PARK THEN WALK"), 4, 255, 8,
     [0, 17, 35, 36, 38, 12, 0],
     ["Collect small relay parcels", "Bloor dispatch handoff", "Danforth hall handoff", "Withrow on-foot park handoff",
      "Gerrard rail-side handoff", "King dispatch handoff", "Return relay pouch"]),
    ("PARK PATH PARCELS", ("TWO PARK HANDOFFS", "PARK THEN WALK"), 0, 255, 12,
     [0, 36, 41, 42, 39, 0],
     ["Collect park supply parcels", "Withrow on-foot handoff", "Greenwood on-foot handoff",
      "Ashbridge supply handoff", "Carlaw workshop handoff", "Return park delivery satchel"]),
    ("EAST RETURN PACK", ("SHOP RETURN PAPERS", "ORIGINAL TO UNION"), 6, 0, 12,
     [0, 39, 40, 42, 37, 0],
     ["Collect return envelopes", "Carlaw paperwork handoff", "Leslie returns handoff", "Ashbridge returns handoff",
      "Riverside paperwork handoff", "Return original packet to Union"]),
    ("CROSS CITY BUNDLES", ("EAST TO WEST MAIL", "FINAL PARK DEPOT"), 0, 3, 18,
     [0, 35, 38, 40, 42, 39, 37, 31, 33, 0],
     ["Collect cross-city bundles", "Danforth print handoff", "Gerrard print handoff", "Leslie shop handoff",
      "Ashbridge service handoff", "Carlaw workshop handoff", "Riverside shop handoff",
      "Sorauren west handoff", "Parkside depot handoff", "Return empty bundle carrier"]),
)



def after_index(quests, quest):
    """The native index of the contract this one waits for (255 for none)."""
    after = quest.get('after')
    return 255 if after is None else [q['id'] for q in quests].index(after)

def native_table(code, name):
    found = re.search(r"\b" + re.escape(name) + r"\s*\[[^;=]+\]\s*=\s*\{(.*?)\n\};", code, re.S)
    assert found, f"Missing native prefix table: {name}"
    return found.group(1)


def preserved_prefix(campaign):
    """Pin existing native fields, including the eight published western jobs."""
    stops, jobs = campaign["stops"][:PREFIX_STOPS], campaign["quests"][:PREFIX_QUESTS]
    assert len(stops) == PREFIX_STOPS and [stop["id"] for stop in stops] == list(range(PREFIX_STOPS))
    assert len(jobs) == PREFIX_QUESTS and [job["id"] for job in jobs] == [f"contract-{index:02d}" for index in range(1, 81)]
    normalized_stops = [{field: stop.get(field, 0) for field in PREFIX_STOP_FIELDS} for stop in stops]
    normalized_jobs = [{field: job[field] for field in BASE_QUEST_FIELDS} for job in jobs]
    assert sha(canonical(normalized_stops)) == PREFIX_STOPS_SHA256, "Existing 35 native stop fields changed"
    assert sha(canonical(normalized_jobs)) == PREFIX_QUESTS_SHA256, "Existing 80 contract fields changed"
    code = (ROOT / "project/plugins/toronto-driving/engine/src/td_content.c").read_text()
    rows = re.findall(r'\{(\d+),(\d+),"([^"]+)",(\d+),(\d+),(\d+)\}', native_table(code, "td_stops"))
    assert len(rows) >= PREFIX_STOPS, "Native stop prefix is unavailable"
    for stop, row in zip(stops, rows[:PREFIX_STOPS]):
        u, v, name, transit, district, flags = row
        assert (int(u), int(v), name, int(transit), int(district), int(flags)) == (
            stop["u"], stop["v"], stop["name"], stop["transit"], stop.get("district", 0), stop.get("reserved", 0)), f"Native stop prefix differs: {stop['id']}"
    rows = re.findall(r'\{"([^"]+)",(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),\{([\d,]+)\}\}', native_table(code, "td_jobs"))
    assert len(rows) >= PREFIX_QUESTS, "Native contract prefix is unavailable"
    for job, row in zip(jobs, rows[:PREFIX_QUESTS]):
        title, *numbers, route = row
        expected = [job["kind_id"], len(job["route"]), job["required_vehicle"], job["min_completed"], after_index(jobs, job), job["time_limit_seconds"], job["reward"]]
        assert title == job["title"] and list(map(int, numbers)) == expected, f"Native contract prefix differs: {job['id']}"
        assert list(map(int, route.split(","))) == job["route"] + [255] * (12 - len(job["route"])), f"Native contract route differs: {job['id']}"
    briefs = re.findall(r'"([^"]*)"', native_table(code, "td_briefs"))
    assert briefs[:PREFIX_QUESTS] == ["".join(line.ljust(18) for line in job["brief"]) for job in jobs], "Native brief prefix differs"
    return stops, jobs, normalized_stops, normalized_jobs


def registered_east(world, metadata):
    districts = world["districts"]
    assert len(districts) == 16 and [district["id"] for district in districts] == list(range(16)), "Register the sixteen native scenes before estimating eastern jobs"
    assert [district["scene"] for district in districts[12:]] == [world2x.scene_slug(i) for i in range(12, 16)]
    header = (ROOT / "project/plugins/toronto-driving/engine/include/td_district.h").read_text()
    count = re.search(r"^\s*#define\s+TD_DISTRICT_COUNT\s+(\d+)\s*$", header, re.M)
    assert count and int(count.group(1)) == len(districts), "Native district count disagrees with actual registry"
    for entry in districts[12:]:
        scene = read_json(ROOT / "project/project/scenes" / entry["scene"] / "scene.gbsres")
        assert scene["_resourceType"] == "scene" and scene["type"] == "TORONTO" and scene["symbol"] == entry["symbol"]
        assert (scene["width"], scene["height"]) == (128, 122)
    assert metadata["id"] == 3 and metadata["dimensions"] == [world2x.WORLD_W, world2x.WORLD_H]
    east_edges = 0
    for portal in world["portals"]:
        if {portal["from"]["district"] >> 2, portal["to"]["district"] >> 2} != {0, 3}:
            continue
        core, east = (portal["from"], portal["to"]) if portal["from"]["district"] >> 2 == 0 else (portal["to"], portal["from"])
        assert core["u"] == 1000 and east["u"] == 24
        assert set(portal["access"]) == {"foot", "vehicle"} and portal["modeled_crossing_pixels"] == 48
        east_edges += 1
    assert east_edges == 3, "Eastern jobs require the three registered reciprocal road seams; Gerrard must remain closed"


def author():
    campaign = read_json(ROOT / "content/campaign.json")
    base_stops, base_jobs, normalized_stops, normalized_jobs = preserved_prefix(campaign)
    world_path = ROOT / "content/districts/world.json"
    world = read_json(world_path)
    metadata_path = ROOT / "content/districts/east_art.json"
    metadata = read_json(metadata_path)
    registered_east(world, metadata)
    # Plan positions (1x), mapped onto the district's double-scale scenes.
    candidates = metadata["plan"]["stop_candidates"]
    assert [candidate["key"] for candidate in candidates] == list(STOP_KEYS), "Preserve the eight eastern service point IDs"
    authored_stops = []
    for index, candidate in enumerate(candidates, PREFIX_STOPS):
        foot = candidate.get("foot_only", False)
        assert type(foot) is bool and foot == (index in (36, 41)), "Preserve Withrow/Greenwood park-and-walk delivery flags"
        assert candidate["fictional_service_point"] is True
        scene, u, v, anchor = world2x.map_stop(3, candidate["x"], candidate["y"], candidate.get("parking_anchor"))
        stop = {"id": index, "u": u, "v": v, "name": STOP_NAMES[index - PREFIX_STOPS],
                "transit": 0, "district": scene, "reserved": int(foot), "foot_only": foot,
                "reference_label": candidate["name"], "geography_reference": metadata["source_research"],
                "location_notice": "Original fictional courier service entrance on compressed terrain; not a surveyed loading door"}
        if foot:
            stop["parking_anchor"] = {"u": anchor[0], "v": anchor[1]}
            stop["access_notice"] = "Native delivery requires parking and walking; no real-world park vehicle-access policy is asserted"
        authored_stops.append(stop)
    stops = [{**stop, "district": stop.get("district", 0)} for stop in base_stops] + authored_stops
    model = RouteModel(world, stops)
    for stop in authored_stops:
        node = point(stop["district"], stop["u"], stop["v"])
        assert model.usable(node, False)
        if stop["foot_only"]:
            assert not model.usable(node, True), "Foot-only park artwork must block vehicle tile-centre access"
            anchor = stop["parking_anchor"]
            parked = point(stop["district"], anchor["u"], anchor["v"])
            assert model.usable(parked, True), "Park-and-walk anchor must remain road-accessible"
            walked, _ = model.shortest(node, parked, False)
            assert 0 < walked <= 384, "Eastern last-mile legs must remain short purposeful walks"
        else:
            assert model.usable(node, True), f"Eastern roadside service point is blocked: {stop['name']}"
    jobs = []
    for index, (title, brief, kind, vehicle, gate, route, objectives) in enumerate(CONTRACTS, 81):
        assert title.isascii() and len(title) <= 18 and len(brief) == 2 and all(line.isascii() and len(line) <= 18 for line in brief)
        assert 2 <= len(route) <= 12 and len(route) == len(objectives)
        assert all(0 <= stop < 43 for stop in route) and all(first != last for first, last in zip(route, route[1:]))
        assert 3 <= gate <= 18 and kind in (0, 1, 2, 3, 4, 6)
        assert any(stops[stop]["district"] >> 2 == 3 for stop in route)
        assert vehicle == 255 or all(not stops[stop].get("foot_only", False) for stop in route)
        legs, road, walking = [], 0, 0
        for first, last in zip(route, route[1:]):
            distance, walk, seams = model.stop_leg(stops[first], stops[last])
            road += distance; walking += walk
            legs.append({"from_stop": first, "to_stop": last,
                         "from_district": stops[first]["district"], "to_district": stops[last]["district"],
                         "vehicle_pixels": distance, "required_walking_pixels": walk,
                         "shortest_modeled_seams": seams})
        fastest = SPEEDS[vehicle] if vehicle != 255 else max(SPEEDS.values())
        slowest = SPEEDS[vehicle] if vehicle != 255 else min(SPEEDS.values())
        fast_time, slow_time = road / fastest + walking / FOOT_SPEED, road / slowest + walking / FOOT_SPEED
        handling = len(route) * 5
        deadline = math.ceil(((60 if kind == 2 else 90) + (slow_time + handling) * (1.4 if kind == 2 else 1.8)) / 5) * 5
        jobs.append({
            "id": f"contract-{index:02d}", "title": title, "brief": list(brief),
            "chapter": "Eastern package connections", "kind": campaign["quest_types"][kind], "kind_id": kind,
            "required_vehicle": vehicle, "min_completed": gate, "route": route,
            "time_limit_seconds": deadline,
            "reward": 85 + math.ceil(road / 80) + (len(route) - 1) * 12 + (25 if kind in (1, 3) else 0),
            "stages": [{"stop_id": stop, "district": stops[stop]["district"], "objective": objective}
                       for stop, objective in zip(route, objectives)],
            "objective_notice": "Ordered native parcel handoffs, condition/timer rules and returns; no signature UI, new cargo animation, artificial delay or new transit service is implied",
            "timing_design": {
                "planning_only": True, "measured_duration_seconds": None,
                "vehicle_route_pixels": road, "required_walking_pixels": walking,
                "normalized_moving_seconds_fast": round(fast_time, 1), "normalized_moving_seconds_slow": round(slow_time, 1),
                "assumed_interaction_allowance_seconds": handling, "legs": legs,
                "basis": "Actual registered collision-grid tile-centre paths and authored reciprocal portals, using integer-settled cardinal speed estimates. Excludes transit, traffic, acceleration, entry animations, real handling and repositioning. Not observed duration.",
            },
        })
    all_jobs = base_jobs + jobs
    assert len(jobs) == 8 and len({tuple(job["route"]) for job in all_jobs}) == 88
    assert len({job["title"] for job in all_jobs}) == 88
    assert {stop for job in jobs for stop in job["route"] if stop >= 35} == set(range(35, 43))
    assert {stops[stop]["district"] >> 2 for stop in jobs[-1]["route"]} == {0, 1, 2, 3}, "Final package round must connect all four districts"
    done = 0
    while done < len(all_jobs):
        unlocked = sum(job["min_completed"] <= done for job in all_jobs)
        assert unlocked > done, f"Progression deadlock at {done} completions"
        done = unlocked
    return {
        "schema_version": 1, "status": "authored; native compilation and ordinary-input eastern job playtests pending",
        "scope": "Eight eastern package contracts and eight fictional service points added to the preserved native80/35 prefix; full Old Toronto remains unfinished",
        "duration_target_minutes": 120, "duration_verified": False,
        "duration_notice": "Counts, collision-route estimates, deadlines and assumed interaction allowances do not establish two hours or enjoyment. Measure purposeful ordinary gameplay.",
        "base_stop_count": 35, "base_quest_count": 80, "result_stop_count": 43, "result_quest_count": 88,
        "preserved_base_stops_sha256": sha(canonical(normalized_stops)), "preserved_base_quests_sha256": sha(canonical(normalized_jobs)),
        "preserved_base_stop_fields": list(PREFIX_STOP_FIELDS), "preserved_base_quest_fields": list(BASE_QUEST_FIELDS),
        "world_sha256": sha(world_path.read_bytes()), "collision_resources": model.resources,
        "geography_metadata": [{"path": "content/districts/east_art.json", "sha256": sha(canonical(metadata)),
                                "notice": "Source-derived geography and original compressed artwork; retain City attribution in metadata and distribution notices"}],
        "normalized_cardinal_speeds_pixels_per_second": {"car": SPEEDS[0], "truck": SPEEDS[1], "motorcycle": SPEEDS[2], "scooter": SPEEDS[3], "walking": FOOT_SPEED},
        "transit_notice": "All eight eastern service points have transit=0. Existing core Line1/94 services are optional relay choices; Line2/streetcar boarding is not implemented by this content.",
        "geography_notice": "Only Danforth, Dundas and Queen core/east road portals are registered. Conditional Gerrard portal stays closed. Pape's railway crossing is pedestrian-only; vehicle routes use the actual collision graph.",
        "native_stop_flags": {"reserved_bit_0": "Foot-only delivery; reject car-door proximity and require on-foot handoff"},
        "stops": authored_stops, "quests": jobs,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="Verify eastern JSON without writing")
    mode.add_argument("--diff", action="store_true", help="Print a unified JSON diff without writing; exit1 when stale")
    args = parser.parse_args()
    try:
        expected = json.dumps(author(), indent=2) + "\n"
        current = OUTPUT.read_text() if OUTPUT.exists() else ""
        if args.diff:
            sys.stdout.writelines(difflib.unified_diff(current.splitlines(True), expected.splitlines(True),
                                                     fromfile="east_jobs.json", tofile="authored east_jobs.json"))
        if args.check or args.diff:
            if current != expected:
                if args.check:
                    print("Eastern jobs JSON is stale; run scripts/create_east_jobs.py after native scene registration.", file=sys.stderr)
                return 1
        else:
            OUTPUT.parent.mkdir(parents=True, exist_ok=True)
            OUTPUT.write_text(expected)
        print("Eastern package content: eight jobs/eight appended stops; actual collision routes, prefix and progression verified. Duration remains unmeasured.", file=sys.stderr)
        return 0
    except (AssertionError, KeyError, ValueError, OSError) as error:
        print(f"Cannot author eastern jobs: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
