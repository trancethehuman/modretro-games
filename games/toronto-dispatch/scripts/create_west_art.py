"""Author the western districts (West, High Park) at double scale: four
native scenes each, with backgrounds, attributes and collision grids.

The authored plan is west_layout.py; outer_art.py maps and paints it. No map
imagery, photos, official logos or downloaded geometry are used as pixels.
"""
import sys
import outer_art
from west_layout import WEST, HIGH_PARK, WEST_AREAS, HIGH_PARK_AREAS

# Stable candidate keys consumed by the campaign tooling, in plan order.
KEYS = {1: ["dufferin_college", "lansdowne_bloor", "parkdale_queen", "roncy_howard_park", "sorauren"],
        2: ["bloor_park_gate", "parkside_south", "colborne_service"]}


def main(check=False):
    for spec, areas in ((WEST, WEST_AREAS), (HIGH_PARK, HIGH_PARK_AREAS)):
        plan = {**spec, "stop_candidates": [{"key": k, **s} for k, s in zip(KEYS[spec["id"]], spec["stop_candidates"])]}
        outer_art.write(plan, areas, check, {"source_research": "content/districts/west-research.json"})


if __name__ == "__main__":
    main(check="--check" in sys.argv)
