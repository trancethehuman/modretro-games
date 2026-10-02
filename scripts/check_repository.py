"""Validate planning content; deliberately does not claim ROM verification."""

import json
from datetime import date
from pathlib import Path
from urllib.parse import urlparse

ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def read(path):
    return json.loads(path.read_text())


def index(items, label):
    require(isinstance(items, list) and items, f"{label}: expected nonempty list")
    result = {}
    for item in items:
        key = item.get("id")
        require(isinstance(key, str) and key, f"{label}: missing id")
        require(key not in result, f"{label}: duplicate id {key}")
        result[key] = item
    return result


def integer(value, minimum, label):
    require(type(value) is int and value >= minimum, f"{label}: expected integer >= {minimum}")


def main():
    for filename in ("AGENTS.md", "README.md", "LICENSE", "THIRD_PARTY_NOTICES.md"):
        require((ROOT / filename).is_file(), f"Missing {filename}")
    skill_root = ROOT / "skills"
    require((ROOT / ".agents/skills").resolve() == skill_root, "Skill discovery must point to root skills")
    for name in ("modretro-development", "toronto-worldbuilding", "chromatic-cartridge"):
        require((skill_root / name / "SKILL.md").is_file(), f"Missing skill {name}")
    games = sorted(p for p in (ROOT / "games").iterdir() if p.is_dir())
    require(games, "No game folders")
    for game in games:
        for filename in ("README.md", "DESIGN.md", "ROADMAP.md", "DECISIONS.md", "TESTING.md"):
            require((game / filename).is_file(), f"{game.name}: missing {filename}")
        if game.name != "toronto-dispatch":
            continue
        sources = index(read(game / "content/sources.json"), "sources")
        for source in sources.values():
            url = urlparse(source["url"])
            require(url.scheme == "https" and url.hostname, "Source needs HTTPS URL")
            date.fromisoformat(source["reviewed"])
            require(source.get("scope"), "Source needs reviewed scope")
        vehicles = index(read(game / "content/vehicles.json"), "vehicles")
        for vehicle in vehicles.values():
            integer(vehicle["cargo_capacity"], 0, "cargo capacity")
            integer(vehicle["passenger_capacity"], 0, "passenger capacity")
        content = read(game / "content/missions.json")
        require(content["status"] == "proposal-not-engine-integrated", "Update validation when content is integrated")
        locations = index(content["locations"], "locations")
        missions = index(content["missions"], "missions")
        for mission in missions.values():
            label = mission["id"]
            require(mission["pickup"] in locations and mission["dropoff"] in locations, f"{label}: unknown location")
            require(mission["pickup"] != mission["dropoff"], f"{label}: identical endpoints")
            require(mission["kind"] in ("package", "passenger"), f"{label}: unknown kind")
            allowed = mission["allowed_vehicles"]
            require(allowed and len(allowed) == len(set(allowed)), f"{label}: empty or duplicate vehicle choices")
            integer(mission["time_limit_seconds"], 1, f"{label}: time limit")
            integer(mission["reward"], 0, f"{label}: reward")
            capacity = "cargo_capacity" if mission["kind"] == "package" else "passenger_capacity"
            units = mission["cargo_units"] if mission["kind"] == "package" else mission["passengers"]
            integer(units, 1, f"{label}: load")
            for vehicle in allowed:
                require(vehicle in vehicles, f"{label}: unknown vehicle {vehicle}")
                require(vehicles[vehicle][capacity] >= units, f"{label}: load exceeds {vehicle} capacity")
        print(f"{game.name}: {len(missions)} proposed missions, {len(vehicles)} vehicles, {len(sources)} sources valid")
    print("Repository scaffold checks passed. ROM and gameplay verification remain separate.")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, TypeError, OSError) as error:
        raise SystemExit(f"Check failed: {error}") from error
