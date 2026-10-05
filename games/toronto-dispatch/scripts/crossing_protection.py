"""Fail-closed crossing repaint proof chained before the campus/city proofs.

Each retained R8 PNG has a fixed byte identity. Reconstructing it requires the
current art to equal an independently drawn, two-by-six-pixel stroke replacement
in exactly the historical visible crossing cells. No geometry, palette/priority,
furniture, building or other painted pixel is exempted from protection.
"""
import copy
import hashlib
import io
import json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]
FIXTURE = REPO / "tests/fixtures/crossing_art_protected.json"
SLUGS = ("city", "west", "high_park", "east", "port_lands", "north")
OLD_PINS = {
    "city": "d6633f9a5eaef6e927a75650296de806321000bdcf0ab74500e539bed3d85c83",
    "west": "2eca37c9bb4c7365f68fe96982ac7efe2d410dda246e4c78915ff2a377e70058",
    "high_park": "1e4eeb5f545e647f04010cf19e224ba1a81ce7be747306e4e686a11fbfbdc5d1",
    "east": "066fc89020671fc61f6fd822ee55981af83d5bd17757aea4634efa6f24a4a35e",
    "port_lands": "3fea592190f7928b329cfdf6ebeb72fba84eb69c2cf42e96d3da43ed962725ae",
    "north": "9318fd9059bf7c921c08cdb72bdeeda27948ee49fcf6fd0db64d0b1fc46ca2c9",
}
PNG_FILES = {f"project/assets/backgrounds/toronto_{slug}.png" for slug in SLUGS}
RGB = ((7,24,33), (48,104,80), (134,192,108), (224,248,207))


def sha(payload):
    return hashlib.sha256(payload).hexdigest()


def fixture():
    proof = json.loads(FIXTURE.read_text())
    assert proof["schema"] == 1 and tuple(proof["backgrounds"]) == SLUGS
    assert proof["before_commit"] == "1068285b968a12d1b4892d4da35e14116f144261"
    for slug, pin in OLD_PINS.items():
        assert proof["backgrounds"][slug]["before_sha256"] == pin
    return proof


def _tile(vertical, old):
    """Independent oracle: no imported painter, palette mutation or flip shortcut."""
    image = Image.new("RGB", (8,8), RGB[1])
    for y in range(8):
        for x in range(8):
            along, across = (y,x) if vertical else (x,y)
            white = 1 <= across <= 4 if old else 1 <= along <= 6 and 3 <= across <= 4
            if white:
                image.putpixel((x,y), RGB[3])
    return image


def allowed_cells(metadata, previous):
    placements = {(p["x"],p["y"]) for p in metadata["scenery"]["placements"]
                  if p["kind"] == "crosswalk"}
    geometric = set()
    for junction in metadata["scenery"]["junctions"]:
        x,y = junction["x"],junction["y"]
        for arm in junction["arms"]:
            assert arm in ("north", "east", "south", "west")
            for across in range(-24,24,8):
                offset = -16 if arm in ("north", "west") else 8
                geometric.add((x+across,y+offset) if arm in ("north", "south")
                              else (x+offset,y+across))
    assert placements <= geometric, "Crossing placement outside an original authored junction"
    stripes = {_tile(True,True).tobytes(), _tile(False,True).transpose(Image.Transpose.FLIP_TOP_BOTTOM).tobytes()}
    result = {(x,y) for y in range(0,previous.height,8) for x in range(0,previous.width,8)
              if previous.crop((x,y,x+8,y+8)).tobytes() in stripes}
    assert result <= placements, "A stripe outside the old crossing list cannot be admitted"
    return [list(c) for c in sorted(result, key=lambda c: (c[1],c[0]))]


def png_history(relative, payload, proof=None):
    """Return exact R8 bytes after verifying the entire final native artwork."""
    assert relative in PNG_FILES
    proof = fixture() if proof is None else proof
    slug = relative.rsplit("/toronto_",1)[1][:-4]
    row = proof["backgrounds"][slug]
    assert sha(payload) == row["after_sha256"], "Unapproved current crossing PNG"
    previous_payload = (ROOT / row["previous"]).read_bytes()
    assert sha(previous_payload) == row["before_sha256"] == OLD_PINS[slug], "Crossing predecessor changed"
    metadata = json.loads((ROOT / row["metadata"]).read_text())
    assert sha((ROOT / row["metadata"]).read_bytes()) == row["metadata_sha256"], "Historical crossing metadata changed"
    with Image.open(io.BytesIO(previous_payload)) as previous, Image.open(io.BytesIO(payload)) as current:
        assert current.format == previous.format == "PNG" and current.mode == previous.mode == "RGB"
        assert current.size == previous.size == (1024,976) and current.info == previous.info
        assert sha(previous.tobytes()) == row["before_rgb_sha256"]
        assert sha(current.tobytes()) == row["after_rgb_sha256"]
        cells = allowed_cells(metadata, previous)
        assert cells == row["stamp_cells"], "Crossing paint permission changed"
        assert all(x % 8 == y % 8 == 0 for x,y in cells)
        expected = previous.copy()
        old_vertical = _tile(True,True).tobytes()
        # The legacy horizontal strip was ROTATE_90, so its white rows have
        # the reverse offset. New bars are centred symmetrically instead.
        old_horizontal = _tile(False,True).transpose(Image.Transpose.FLIP_TOP_BOTTOM).tobytes()
        vertical, horizontal = _tile(True,False), _tile(False,False)
        for x,y in cells:
            old = previous.crop((x,y,x+8,y+8)).tobytes()
            assert old in (old_vertical,old_horizontal)
            expected.paste(vertical if old == old_vertical else horizontal, (x,y))
        assert current.tobytes() == expected.tobytes(), "Crossings changed a stripe, old object, road or outside pixel"
    return previous_payload


def check():
    proof = fixture()
    for relative, pin in proof["protected_files"].items():
        assert sha((ROOT / relative).read_bytes()) == pin, ("Crossing protected source changed", relative)
    for slug, row in proof["backgrounds"].items():
        relative = f"project/assets/backgrounds/toronto_{slug}.png"
        assert sha(png_history(relative, (ROOT / relative).read_bytes(), proof)) == OLD_PINS[slug]
        from crossing_art import native_bytes
        generated = native_bytes((ROOT / row["previous"]).read_bytes(), json.loads((ROOT / row["metadata"]).read_text()))
        assert generated == (ROOT / relative).read_bytes(), "Stale current native crossing layer"
    return proof


def negatives(proof=None):
    proof = fixture() if proof is None else proof
    checks = 0
    for slug, row in proof["backgrounds"].items():
        relative = f"project/assets/backgrounds/toronto_{slug}.png"
        payload = (ROOT / relative).read_bytes()

        def reject(value, candidate):
            nonlocal checks
            try:
                png_history(relative, value, candidate)
            except (AssertionError,OSError,ValueError):
                checks += 1
            else:
                raise AssertionError("Crossing proof accepted a paint/outside/pin mutation")

        reject(payload+b"corruption", proof)
        for x,y in ((0,0), tuple(row["stamp_cells"][0])):
            changed = Image.open(io.BytesIO(payload)).convert("RGB")
            old = changed.getpixel((x,y))
            changed.putpixel((x,y), RGB[3] if old != RGB[3] else RGB[1])
            output = io.BytesIO(); changed.save(output, format="PNG")
            reject(output.getvalue(), proof)
            forged = copy.deepcopy(proof)
            forged["backgrounds"][slug]["after_sha256"] = sha(output.getvalue())
            forged["backgrounds"][slug]["after_rgb_sha256"] = sha(changed.tobytes())
            reject(output.getvalue(), forged)
        for key, value in (("stamp_cells",row["stamp_cells"]+[[0,0]]),
                           ("before_sha256","changed"), ("before_rgb_sha256","changed")):
            forged = copy.deepcopy(proof); forged["backgrounds"][slug][key] = value
            reject(payload, forged)
    assert checks == 48
    return checks
