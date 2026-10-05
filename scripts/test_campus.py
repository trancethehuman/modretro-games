"""Fail-closed source-only campus geography/art/unchanged-geometry checks."""
import hashlib
import json
from pathlib import Path
import sys
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
sys.path.insert(0, str(GAME / "scripts"))
import campus_protection as protection
from city_layout import road
from create_traffic_signals import decode


def main():
    proof = protection.check(); checks = protection.negatives(proof)
    area = json.loads((GAME / "content/campus_area.json").read_text())
    scene = json.loads((GAME / "project/project/scenes/toronto_city/scene.gbsres").read_text())
    native = json.loads((GAME / "project/assets/backgrounds/toronto_city.png.gbsres").read_text())
    grid = decode(scene["collisions"]); attrs = decode(native["tileColors"])
    source_attrs = json.loads((GAME / "project/original-art/city_attributes.json").read_text())
    assert attrs == source_attrs and len(grid) == len(attrs) == 128*122; checks += 2
    for x, y in proof["stamp_cells"]:
        assert not road(x+4,y+4), ("Campus art is on asphalt", x,y)
        checks += 1
    for landmark in area["landmarks"]:
        x,y,w,h = landmark["footprint"]
        for ty in range(y//8,(y+h)//8):
            for tx in range(x//8,(x+w)//8):
                assert grid[ty*128+tx] == 15 and attrs[ty*128+tx] & 128
                checks += 2
    # The forecourt paving lives only on the old reachable sidewalks; it
    # doesn't pretend that decorative solid campus interior is traversable.
    for path in area["paths"]:
        x,y,w,h = path["rect"]
        for ty in range(y//8,(y+h)//8):
            for tx in range(x//8,(x+w)//8):
                assert grid[ty*128+tx] == 16 and not attrs[ty*128+tx] & 128
                checks += 2
    old = Image.open(GAME / protection.PREVIOUS).convert("RGB")
    current = Image.open(GAME / protection.BACKGROUND).convert("RGB")
    assert old.crop((400,88,448,144)).tobytes() == current.crop((400,88,448,144)).tobytes(); checks += 1
    assert current.getpixel((417,233)) != old.getpixel((417,233)); checks += 1
    sources = json.loads((GAME / "content/sources.json").read_text())
    for key in area["source_ids"]:
        rows = [s for s in sources if s["id"] == key]
        assert len(rows) == 1 and rows[0]["reviewed"] == "2026-10-05" and "utoronto.ca" in rows[0]["url"]
        checks += 1
    assert hashlib.sha256((GAME / protection.PREVIOUS).read_bytes()).hexdigest() == protection.OLD_SHA256; checks += 1
    print(f"PASS: {checks} original campus source/negative/footprint/path checks; compiled/native/hardware acceptance separate")


if __name__ == "__main__":
    main()
