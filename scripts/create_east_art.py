"""Author the eastern district (Riverside, Riverdale, Leslieville) at double
scale: four native scenes with backgrounds, attributes and collision grids.

The authored plan is east_layout.py; outer_art.py maps and paints it. Pillow
paints exact manual-palette pixels; official maps/images are not inputs.
"""
import sys
import outer_art
from east_layout import EAST, RESEARCH, EAST_AREAS


def main(check=False):
    outer_art.write(EAST, EAST_AREAS, check,
                    {"source_research": "docs/EAST_DISTRICT.md", "research": RESEARCH},
                    # Pape's rail crossing stays foot-only (no invented car bridge).
                    {"foot_only_points": [(544, 248)]})


if __name__ == "__main__":
    main(check="--check" in sys.argv)
