#!/usr/bin/env python3
"""Author eight western package contracts; never generate engine files.

Read the registered native collision resources and reciprocal portals before
writing planning estimates. --check and --diff do not change any files.
"""
import argparse
from collections import defaultdict, deque
import difflib
import hashlib
import heapq
import json
import math
from pathlib import Path
import sys
from island_campaign import historical_stop, ISLAND_DISTRICT, relocate_island_stops

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "content/districts/west_jobs.json"
SPEEDS = {0: 86.25, 1: 71.25, 2: 101.25, 3: 63.75}
FOOT_SPEED = 30.0
BASE_STOPS_SHA256 = "981629d78d412ae0cc1a12fdd6158331c092976e9c1bdd173c83fdbedf55d08f"
BASE_QUESTS_SHA256 = "be356c3b6e9736ce43a998ba4b0856b438dfc60b90dbf288b0a317b88e1efd5b"
BASE_QUEST_FIELDS = ("id", "title", "brief", "kind_id", "required_vehicle",
                     "min_completed", "route", "time_limit_seconds", "reward")
STOP_NAMES = [
    "DUFFERIN COLLEGE", "LANSDOWNE BLOOR", "PARKDALE QUEEN",
    "RONCY HOWARD PARK", "SORAUREN SERVICE", "BLOOR PARK GATE",
    "PARKSIDE SOUTH", "COLBORNE SERVICE",
]
STOP_KEYS = [
    "dufferin_college", "lansdowne_bloor", "parkdale_queen", "roncy_howard_park",
    "sorauren", "bloor_park_gate", "parkside_south", "colborne_service",
]
# Original package work, using existing native kind rules. A return is an
# ordered depot-return route; the engine does not implement a signature record.
CONTRACTS = [
    ("WEST WINDOW RUN", ("WEST SHOP PARCELS", "RETURN TO UNION"), 0, 255, 3,
     [0, 27, 29, 31, 0],
     ["Collect shop parcels", "Dufferin handoff", "Parkdale shop handoff",
      "Sorauren handoff", "Return empty satchel"]),
    ("PARKDALE ART", ("SMALL FRAMED ART", "SCOOTER CARE RUN"), 1, 3, 6,
     [5, 29, 31, 5],
     ["Collect framed art", "Parkdale exhibit handoff", "Sorauren frame exchange",
      "Return borrowed case"]),
    ("WEST RUSH FILES", ("MOTORCYCLE FILES", "CHOOSE A CORRIDOR"), 2, 2, 6,
     [0, 28, 32, 30, 0],
     ["Collect urgent files", "Lansdowne office handoff", "Bloor gate handoff",
      "Howard Park office handoff", "Return dispatch copy"]),
    ("WEST LOADING RUN", ("TRUCK SUPPLY LOAD", "WIDE ROAD APPROACH"), 3, 1, 8,
     [11, 27, 31, 33, 29, 0],
     ["Collect boxed supplies", "Dufferin supply drop", "Sorauren loading drop",
      "Parkside roadside drop", "Parkdale stock drop", "Return empty crates"]),
    ("WEST TRANSFER KIT", ("SMALL PACKAGE KIT", "TRAIN BUS OR ROAD"), 4, 255, 8,
     [0, 17, 18, 28, 30, 0],
     ["Collect small repair kit", "Bloor Yonge handoff", "Bloorcourt relay handoff",
      "Lansdowne handoff", "Howard Park handoff", "Return reusable pouch"]),
    ("LODGE LAST MILE", ("PARK THEN WALK", "LODGE LAST MILE"), 0, 255, 12,
     [0, 29, 34, 33, 0],
     ["Collect museum parcel", "Parkdale package exchange", "Walk to Lodge service",
      "Parkside return handoff", "Return collection box"]),
    ("WEST RETURN PAPERS", ("CAR DOCUMENT ROUND", "RETURN ORIGINAL"), 6, 0, 12,
     [0, 31, 30, 27, 0],
     ["Collect return documents", "Sorauren document exchange", "Howard Park exchange",
      "Dufferin return handoff", "Return original to Union"]),
    ("WEST CLOSING ROUND", ("SCOOTER SHOP ROUND", "WEST TO HIGH PARK"), 0, 3, 18,
     [0, 27, 28, 32, 33, 30, 29, 0],
     ["Collect closing parcels", "Dufferin handoff", "Lansdowne shop handoff",
      "Bloor gate handoff", "Parkside shop handoff", "Howard Park handoff",
      "Parkdale final handoff", "Return shift satchel"]),
]


def sha(value):
    return hashlib.sha256(value).hexdigest()


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode()


def read_json(path):
    return json.loads(path.read_text())


def decode_grid(text):
    result, position = [], 0
    while position < len(text):
        value = int(text[position:position + 2], 16)
        position += 2
        if text[position] == "!":
            count, position = 1, position + 1
        else:
            end = text.index("+", position)
            count, position = int(text[position:end], 16), end + 1
        assert count > 0
        result.extend([value] * count)
    return result


def point(district, u, v):
    return district, u // 8, v // 8


class RouteModel:
    """Ordinary tile-centre foot/road paths; ferries are separately typed legs.

    Vehicle paths conservatively require a clear 3x3 tile footprint. This is
    stricter than some native sub-tile positions, so it is not a lower bound.
    """
    def __init__(self, world, stops):
        self.resources = []
        self.grids = {}
        self.portals = world["portals"]
        self.nodes = set()
        self.graphs = {}
        self.results = {}
        self.full_foot_results = {}
        self.full_foot_available = {}
        self.stops = {stop["id"]: stop for stop in stops}
        # Service graph is deliberately separate from both navigation graphs.
        self.ferry_edges = {(origin, target): {"mode": "ferry", "from_stop": origin,
                             "to_stop": target, "walking_pixels": 0,
                             "worst_wait_seconds": 28, "ride_seconds": 8, "fare": 4}
                            for dock in (20, 21, 22)
                            for origin, target in ((10, dock), (dock, 10))}
        for district in world["districts"]:
            path = ROOT / "project/project/scenes" / district["scene"] / "scene.gbsres"
            scene = read_json(path)
            width, height = scene["width"], scene["height"]
            assert (width * 8, height * 8) == (district["width_pixels"], district["height_pixels"])
            grid = decode_grid(scene["collisions"])
            assert len(grid) == width * height
            self.grids[district["id"]] = (width, height, grid)
            self.resources.append({"district": district["id"],
                                   "scene": str(path.relative_to(ROOT)),
                                   "collision_sha256": sha(scene["collisions"].encode())})
        for stop in stops:
            self.nodes.add(point(stop["district"], stop["u"], stop["v"]))
            if stop.get("foot_only"):
                anchor = stop["parking_anchor"]
                self.nodes.add(point(stop["district"], anchor["u"], anchor["v"]))
        for portal in self.portals:
            for endpoint in (portal["from"], portal["to"]):
                self.nodes.add(point(endpoint["district"], endpoint["u"], endpoint["v"]))
        for node in self.nodes:
            assert self.usable(node, False), f"Blocked walking endpoint: {node}"
        for vehicle in (False, True):
            self.graphs[vehicle] = self.build_graph(vehicle)

    def usable(self, node, vehicle):
        district, x, y = node
        width, height, grid = self.grids[district]
        if not (0 <= x < width and 0 <= y < height):
            return False
        if not vehicle:
            return not (grid[y * width + x] & 15)
        if not (1 <= x < width - 1 and 1 <= y < height - 1):
            return False
        return all(grid[(y + dy) * width + x + dx] == 0
                   for dy in (-1, 0, 1) for dx in (-1, 0, 1))

    def build_graph(self, vehicle):
        graph = defaultdict(list)
        grouped = defaultdict(list)
        for node in sorted(self.nodes):
            grouped[node[0]].append(node)
        for district, points in grouped.items():
            width, height, _ = self.grids[district]
            available = bytearray(self.usable((district, x, y), vehicle)
                                  for y in range(height) for x in range(width))
            for start in points:
                if not self.usable(start, vehicle):
                    continue
                origin = start[2] * width + start[1]
                distance = {origin: 0}
                queue = deque([origin])
                while queue:
                    index = queue.popleft()
                    x, y = index % width, index // width
                    for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                        if not (0 <= nx < width and 0 <= ny < height):
                            continue
                        neighbor = ny * width + nx
                        if available[neighbor] and neighbor not in distance:
                            distance[neighbor] = distance[index] + 8
                            queue.append(neighbor)
                for end in points:
                    index = end[2] * width + end[1]
                    if start != end and index in distance:
                        graph[start].append((end, distance[index], None))
        access = "vehicle" if vehicle else "foot"
        for portal in self.portals:
            if access not in portal["access"]:
                continue
            first, last = portal["from"], portal["to"]
            first = point(first["district"], first["u"], first["v"])
            last = point(last["district"], last["u"], last["v"])
            assert self.usable(first, vehicle) and self.usable(last, vehicle), portal["name"]
            crossing = portal["modeled_crossing_pixels"]
            assert isinstance(crossing, int) and crossing > 0
            graph[first].append((last, crossing, portal["name"]))
            graph[last].append((first, crossing, portal["name"]))
        return graph

    def shortest(self, first, last, vehicle):
        key = (first, last, vehicle)
        if key in self.results:
            return self.results[key]
        assert self.usable(first, vehicle) and self.usable(last, vehicle), key
        pending, distances, parents = [(0, first)], {first: 0}, {}
        while pending:
            distance, node = heapq.heappop(pending)
            if distance != distances[node]:
                continue
            if node == last:
                break
            for end, weight, seam in self.graphs[vehicle][node]:
                cost = distance + weight
                if cost < distances.get(end, math.inf):
                    distances[end] = cost
                    parents[end] = (node, seam)
                    heapq.heappush(pending, (cost, end))
        assert last in distances, f"No {'vehicle' if vehicle else 'foot'} route: {first} -> {last}"
        seams, node = [], last
        while node != first:
            node, seam = parents[node]
            if seam:
                seams.append(seam)
        result = distances[last], list(reversed(seams))
        self.results[key] = result
        return result

    def stop_leg(self, first, last):
        assert first["district"] != ISLAND_DISTRICT and last["district"] != ISLAND_DISTRICT, "Island work requires explicit foot/ferry legs, never vehicle access"
        walking = 0
        start = point(first["district"], first["u"], first["v"])
        finish = point(last["district"], last["u"], last["v"])
        for stop, is_first in ((first, True), (last, False)):
            if stop.get("foot_only"):
                anchor = stop["parking_anchor"]
                parked = point(stop["district"], anchor["u"], anchor["v"])
                at_stop = start if is_first else finish
                walked, _ = self.shortest(at_stop, parked, False)
                assert walked <= 900, "Last-mile approach must not become a long empty walk"
                walking += walked
                if is_first:
                    start = parked
                else:
                    finish = parked
        road, seams = self.shortest(start, finish, True)
        return road, walking, seams

    def full_foot_shortest(self, first, last, half=5):
        """Four-pixel cardinal graph checking every tile under the whole body.

        Five-pixel half-width is deliberately conservative versus the courier;
        exact source anchors stay on this grid. No ferry or scene seam is added.
        """
        assert first["district"] == last["district"]
        district = first["district"]
        assert 1 <= half <= 8
        width, height, grid = self.grids[district]
        columns, rows = width * 2, height * 2
        key = district, half
        if key not in self.full_foot_available:
            def clear(x, y):
                u, v = x * 4, y * 4
                return (half <= u < width * 8 - half and half <= v < height * 8 - half and
                        all(not grid[ty * width + tx] & 15
                            for ty in range((v - half) // 8, (v + half) // 8 + 1)
                            for tx in range((u - half) // 8, (u + half) // 8 + 1)))
            self.full_foot_available[key] = bytearray(clear(x, y) for y in range(rows) for x in range(columns))
        available = self.full_foot_available[key]
        assert all(stop["u"] % 4 == stop["v"] % 4 == 0 for stop in (first, last)), "Full-body route anchors must align to the four-pixel graph"
        start = first["v"] // 4 * columns + first["u"] // 4
        finish = last["v"] // 4 * columns + last["u"] // 4
        assert all(0 <= stop['u'] < width * 8 and 0 <= stop['v'] < height * 8 for stop in (first, last)), 'Full-foot endpoint outside scene bounds'
        assert available[start] and available[finish], "Blocked full-foot route endpoint"
        result_key = district, half, start
        if result_key not in self.full_foot_results:
            queue, distances = deque([start]), {start: 0}
            while queue:
                index = queue.popleft()
                x, y = index % columns, index // columns
                for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                    neighbor = ny * columns + nx
                    if (0 <= nx < columns and 0 <= ny < rows and available[neighbor] and neighbor not in distances):
                        distances[neighbor] = distances[index] + 4
                        queue.append(neighbor)
            self.full_foot_results[result_key] = distances
        assert finish in self.full_foot_results[result_key], "Disconnected full-foot route"
        return self.full_foot_results[result_key][finish]

    def service_leg(self, first, last):
        """Explicit Island-post graph: one ferry spoke or an ordinary foot leg.

        Timings/fares are fictional native rules, not current City schedules.
        Ferry edges never appear in shortest(..., vehicle/foot) or stop_leg.
        """
        origin, target = first["id"], last["id"]
        if (origin, target) in self.ferry_edges:
            terminal = first if origin == 10 else last
            island = last if origin == 10 else first
            assert terminal["district"] == 0 and island["district"] == ISLAND_DISTRICT
            assert terminal["transit"] == island["transit"] == 3
            return dict(self.ferry_edges[origin, target])
        assert first["district"] == last["district"], "Island client access must explicitly visit a ferry dock"
        if first["district"] == ISLAND_DISTRICT:
            distance = self.full_foot_shortest(first, last)
        else:
            distance, _ = self.shortest(point(first["district"], first["u"], first["v"]),
                                        point(last["district"], last["u"], last["v"]), False)
        return {"mode": "foot", "from_stop": origin, "to_stop": target,
                "walking_pixels": distance, "worst_wait_seconds": 0,
                "ride_seconds": 0, "fare": 0}


def author():
    campaign = read_json(ROOT / "content/campaign.json")
    world_path = ROOT / "content/districts/world.json"
    world = read_json(world_path)
    assert [district["id"] for district in world["districts"]] == list(range(len(world["districts"])))
    assert len(world["districts"]) >= 3
    base_stops, base_jobs = campaign["stops"][:27], campaign["quests"][:72]
    assert [stop["id"] for stop in base_stops] == list(range(27))
    assert [job["id"] for job in base_jobs] == [f"contract-{index:02d}" for index in range(1, 73)]
    assert all(all(index < 27 for index in job["route"]) for job in base_jobs)
    normalized_base_stops = [{field: historical_stop(stop)[field] for field in ("id", "u", "v", "name", "transit")}
                             for stop in base_stops]
    normalized_base_jobs = [{field: job[field] for field in BASE_QUEST_FIELDS} for job in base_jobs]
    assert sha(canonical(normalized_base_stops)) == BASE_STOPS_SHA256, "Existing core stops changed"
    assert sha(canonical(normalized_base_jobs)) == BASE_QUESTS_SHA256, "Existing 72 contracts changed"
    stops = relocate_island_stops(base_stops, world)
    authored_stops, metadata_sources = [], []
    for district, filename, expected in ((1, "west_art.json", 5), (2, "high_park_art.json", 3)):
        metadata = read_json(ROOT / "content/districts" / filename)
        candidates = metadata["stop_candidates"]
        assert len(candidates) == expected, filename
        metadata_sources.append({"path": f"content/districts/{filename}",
                                 "sha256": sha(canonical(metadata)),
                                 "notice": "Source-derived geography; original compressed art and fictional delivery entrances"})
        for candidate in candidates:
            index = len(authored_stops)
            assert candidate["key"] == STOP_KEYS[index], "Stop candidate order changed; preserve appended IDs"
            stop = {"id": 27 + index, "u": candidate["x"], "v": candidate["y"],
                    "name": STOP_NAMES[index], "transit": 0, "district": district,
                    "reserved": int(candidate.get("foot_only", False)), "foot_only": candidate.get("foot_only", False),
                    "reference_label": candidate["name"],
                    "geography_reference": metadata["source_research"],
                    "location_notice": "Original fictional client entrance on compressed terrain; not a surveyed venue loading door"}
            if "source_relation" in candidate:
                stop["source_relation"] = candidate["source_relation"]
            if stop["foot_only"]:
                anchor = candidate["parking_anchor"]
                stop["parking_anchor"] = {"u": anchor[0], "v": anchor[1]}
                stop["access_notice"] = "Game requires parking and walking; this does not reproduce real seasonal vehicle-access rules"
            assert len(stop["name"]) <= 18 and stop["name"].isascii()
            authored_stops.append(stop)
    assert len(authored_stops) == 8 and authored_stops[-1]["foot_only"]
    assert all(not stop["foot_only"] for stop in authored_stops[:-1])
    stops += authored_stops
    model = RouteModel(world, stops)
    for stop in authored_stops:
        at_stop = point(stop["district"], stop["u"], stop["v"])
        assert model.usable(at_stop, not stop["foot_only"])
        if stop["foot_only"]:
            assert not model.usable(at_stop, True), "Foot-only art must actually block vehicles"
    jobs = []
    for index, (title, brief, kind, vehicle, gate, route, objectives) in enumerate(CONTRACTS, 73):
        assert len(title) <= 18 and all(len(line) <= 18 for line in brief)
        assert 2 <= len(route) <= 12 and len(route) == len(objectives)
        assert all(first != last for first, last in zip(route, route[1:]))
        assert any(stops[stop]["district"] != 0 for stop in route)
        assert vehicle == 255 or all(not stops[stop].get("foot_only") for stop in route)
        legs, road, walking = [], 0, 0
        for first, last in zip(route, route[1:]):
            distance, walk, seams = model.stop_leg(stops[first], stops[last])
            road += distance
            walking += walk
            legs.append({"from_stop": first, "to_stop": last,
                         "from_district": stops[first]["district"], "to_district": stops[last]["district"],
                         "vehicle_pixels": distance, "required_walking_pixels": walk,
                         "shortest_modeled_seams": seams})
        fastest = SPEEDS[vehicle] if vehicle != 255 else max(SPEEDS.values())
        slowest = SPEEDS[vehicle] if vehicle != 255 else min(SPEEDS.values())
        fast_time = road / fastest + walking / FOOT_SPEED
        slow_time = road / slowest + walking / FOOT_SPEED
        handling = len(route) * 5
        deadline = math.ceil(((60 if kind == 2 else 90) + (slow_time + handling) * (1.4 if kind == 2 else 1.8)) / 5) * 5
        jobs.append({
            "id": f"contract-{index:02d}", "title": title, "brief": list(brief),
            "chapter": "Western package routes", "kind": campaign["quest_types"][kind], "kind_id": kind,
            "required_vehicle": vehicle, "min_completed": gate, "route": route,
            "time_limit_seconds": deadline, "reward": 85 + math.ceil(road / 40) + (len(route) - 1) * 12 + (25 if kind in (1, 3) else 0),
            "stages": [{"stop_id": stop, "district": stops[stop]["district"], "objective": objective}
                       for stop, objective in zip(route, objectives)],
            "objective_notice": "Ordered native handoffs and depot returns; descriptions do not imply new signatures, cargo animations or loading delays",
            "timing_design": {
                "planning_only": True, "measured_duration_seconds": None,
                "vehicle_route_pixels": road, "required_walking_pixels": walking,
                "normalized_moving_seconds_fast": round(fast_time, 1),
                "normalized_moving_seconds_slow": round(slow_time, 1),
                "assumed_interaction_allowance_seconds": handling, "legs": legs,
                "basis": "Actual collision-grid tile-centre paths, reciprocal authored portals and integer-settled cardinal speed estimates; excludes transit, traffic, acceleration, real handling and repositioning. Not observed duration.",
            },
        })
    assert len(jobs) == 8 and len({tuple(job["route"]) for job in jobs}) == 8
    assert len({job["title"] for job in base_jobs + jobs}) == 80
    assert set(stop for job in jobs for stop in job["route"] if stop >= 27) == set(range(27, 35))
    done = 0
    while done < 80:
        unlocked = sum(job["min_completed"] <= done for job in base_jobs + jobs)
        assert unlocked > done, f"Progression deadlock at {done} completions"
        done = unlocked
    return {
        "schema_version": 1, "status": "authored; native integration and ordinary-input playtests pending",
        "scope": "Eight package contracts in three connected compressed native districts; full Old Toronto and complete campaign remain pending",
        "duration_target_minutes": 120, "duration_verified": False,
        "duration_notice": "Counts, route estimates, deadlines and assumed handling do not establish two hours or enjoyment. Measure purposeful ordinary play.",
        "base_stop_count": 27, "base_quest_count": 72, "result_stop_count": 35, "result_quest_count": 80,
        "preserved_base_stops_sha256": sha(canonical(normalized_base_stops)),
        "preserved_base_quests_sha256": sha(canonical(normalized_base_jobs)),
        "preserved_base_quest_fields": list(BASE_QUEST_FIELDS),
        "world_sha256": sha(world_path.read_bytes()), "collision_resources": model.resources,
        "geography_metadata": metadata_sources,
        "normalized_cardinal_speeds_pixels_per_second": {"car": SPEEDS[0], "truck": SPEEDS[1], "motorcycle": SPEEDS[2], "scooter": SPEEDS[3], "walking": FOOT_SPEED},
        "transit_notice": "No western transit service is added. The relay optionally uses existing core Line 1 and fictional 94 schedules; all eight new stops have transit=0.",
        "native_stop_flags": {"reserved_bit_0": "Foot-only delivery: require on-foot interaction, including when a parked car lies within the proximity radius"},
        "stops": authored_stops, "quests": jobs,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="Verify the committed JSON is current without writing")
    mode.add_argument("--diff", action="store_true", help="Print a unified JSON diff without writing; exit 1 when stale")
    args = parser.parse_args()
    try:
        content = author()
        expected = json.dumps(content, indent=2) + "\n"
        current = OUTPUT.read_text() if OUTPUT.exists() else ""
        if args.diff:
            sys.stdout.writelines(difflib.unified_diff(current.splitlines(True), expected.splitlines(True),
                                                     fromfile="west_jobs.json", tofile="authored west_jobs.json"))
        if args.check or args.diff:
            if current != expected:
                if args.check:
                    print("Western jobs JSON is stale; run scripts/create_district_jobs.py after registering native scenes.", file=sys.stderr)
                return 1
        else:
            OUTPUT.parent.mkdir(parents=True, exist_ok=True)
            OUTPUT.write_text(expected)
        print("Western package content: eight quests, eight appended stops, collision/portal routes and progression verified. Duration is unmeasured.", file=sys.stderr)
        return 0
    except (AssertionError, KeyError, ValueError, OSError) as error:
        print(f"Cannot author district jobs: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
