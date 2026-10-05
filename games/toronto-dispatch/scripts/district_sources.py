"""Resolve one registered district's original art without changing its identity.

An explicit world-row art_source is a game-root-relative content path. Existing
rows retain their scene-derived filenames; source naming does not rename scenes.
"""
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def district_art_path(district, root=ROOT):
    root = Path(root).resolve()
    scene = district.get("scene")
    if not isinstance(scene, str) or not re.fullmatch(r"toronto_[a-z0-9_]+", scene):
        raise ValueError("Invalid district scene for original art")
    fallback = ("content/city_art.json" if district.get("id") == 0 else
                "content/districts/" + scene.removeprefix("toronto_") + "_art.json")
    relative = district.get("art_source", fallback)
    if (not isinstance(relative, str) or not relative or "\\" in relative or
            Path(relative).is_absolute() or ".." in Path(relative).parts or
            not relative.startswith("content/") or not relative.endswith(".json")):
        raise ValueError("District art_source must be a safe relative content JSON path")
    path = (root / relative).resolve()
    if not path.is_relative_to(root / "content"):
        raise ValueError("District art_source leaves the public content folder")
    if not path.is_file() or path.suffix != ".json":
        raise ValueError("District art_source must identify a regular content JSON file")
    return path


def read_district_art(district, root=ROOT):
    """Read the explicit/default source JSON; never fall back after a bad path."""
    return json.loads(district_art_path(district, root).read_text())
