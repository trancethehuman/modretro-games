"""Authenticate only the original campus overlay; preserve historical city pins.

The existing feedback fixture is never recaptured. Its accepted pre-campus PNG
is retained as source artwork, authenticated and reconstructed exactly for that
older fixture. Outside-cell RGB, unchanged native metadata/geometry and every
current new pixel remain pinned separately; no terrain field is omitted.
"""
import copy
import hashlib
import io
import json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]
FIXTURE = REPO / "tests/fixtures/campus_area_protected.json"
BACKGROUND = "project/assets/backgrounds/toronto_city.png"
PREVIOUS = "project/original-art/campus/core_before_campus.png"
OLD_SHA256 = "2e425c04a43227dea7b126b5dab9291617f24ac0509f1d2b5142eddb68076c7e"


def permitted_cells():
    # Independent of the painter and editable proof: exact original three
    # footprints, clock north lip, one monogram and two old sidewalk strips.
    cells = {(x,y) for x in range(368,400,8) for y in range(96,136,8)}
    cells |= {(x,y) for x in range(368,400,8) for y in range(208,248,8)}
    cells |= {(x,y) for x in (416,424) for y in (200,208)}
    cells |= {(x,240) for x in range(408,440,8)} | {(416,232)}
    cells |= {(x,y) for x in range(368,440,8) for y in (144,256)}
    return [list(c) for c in sorted(cells, key=lambda c: (c[1],c[0]))]


def sha(payload):
    return hashlib.sha256(payload).hexdigest()


def fixture():
    value = json.loads(FIXTURE.read_text())
    assert value["schema"] == 1 and value["before_commit"] == "fb589599f6411d517fe8e2af9d8c43dbf6d89144"
    assert value["prior_feedback_png_sha256"] == value["files"][PREVIOUS] == OLD_SHA256
    return value


def png_history(payload, proof=None):
    """Return authenticated prior PNG bytes only after complete current checks."""
    from crossing_protection import png_history as crossing_history
    payload = crossing_history(BACKGROUND,payload)
    proof = fixture() if proof is None else proof
    assert sha(payload) == proof["files"][BACKGROUND], "Unapproved current campus PNG"
    old_payload = (ROOT / PREVIOUS).read_bytes()
    assert sha(old_payload) == OLD_SHA256, "Retained accepted pre-campus pixels changed"
    with Image.open(io.BytesIO(payload)) as current, Image.open(io.BytesIO(old_payload)) as previous:
        assert current.format == previous.format == "PNG" and current.mode == previous.mode == "RGB"
        assert current.size == previous.size == (1024, 976), "Campus cannot change city dimensions or encoding"
        assert sha(current.tobytes()) == proof["after_rgb_sha256"]
        assert sha(previous.tobytes()) == proof["before_rgb_sha256"]
        cells = proof["stamp_cells"]
        assert cells == permitted_cells(), "Campus stamp permission changed"
        assert len(cells) == len({tuple(c) for c in cells})
        assert all(x % 8 == y % 8 == 0 and 368 <= x <= 432 and 96 <= y <= 256 for x, y in cells)
        restored = current.copy()
        for x, y in cells:
            restored.paste(previous.crop((x, y, x + 8, y + 8)), (x, y))
        assert restored.tobytes() == previous.tobytes(), "Campus changed pixels outside its approved native cells"
    return old_payload


def check():
    proof = fixture()
    assert set(proof["files"]) == {PREVIOUS, BACKGROUND, "content/campus_area.json",
        "project/assets/backgrounds/toronto_city.png.gbsres", "project/original-art/city_attributes.json",
        "project/project/scenes/toronto_city/scene.gbsres", "content/city_art.json"}
    for relative, expected in proof["files"].items():
        payload = (ROOT / relative).read_bytes()
        if relative == BACKGROUND:
            from crossing_protection import png_history as crossing_history
            payload = crossing_history(BACKGROUND,payload)
        assert sha(payload) == expected, ("Campus protected source changed", relative)
    assert sha(png_history((ROOT / BACKGROUND).read_bytes(), proof)) == OLD_SHA256
    # Old fixtures continue to protect all geometry, paths, signals and jobs.
    older = json.loads((REPO / "tests/fixtures/city_feedback_protected.json").read_text())
    assert older["raw_scopes"][BACKGROUND]["after_sha256"] == OLD_SHA256, "Historical city fixture was recaptured"
    from campus_art import LANDMARKS, paint, patterns
    area = json.loads((ROOT / "content/campus_area.json").read_text())
    assert area["landmarks"] == list(LANDMARKS) and area["district"] == 0 and area["scene"] == "toronto_city"
    assert area["visual_anchors"] == {"robarts": [384,108], "university-college": [424,208],
                                      "convocation": [384,224], "original_UT_sign": [416,232]}
    assert area["before_source_commit"] == proof["before_commit"]
    previous = Image.open(ROOT / PREVIOUS).convert("RGB")
    authored = previous.copy()
    value = paint(authored, json.loads((ROOT / "content/city_art.json").read_text())["blocks"])
    from crossing_protection import png_history as crossing_history
    actual = Image.open(io.BytesIO(crossing_history(BACKGROUND,(ROOT / BACKGROUND).read_bytes()))).convert("RGB")
    assert authored.tobytes() == actual.tobytes(), "Campus generator is stale"
    assert value == area["source_checks"] and value["stamp_cells"] == proof["stamp_cells"]
    assert {k: v for k, v in value.items() if k != "stamp_cells"} == proof["pattern_counts"]
    assert value["introduced_canonical_patterns"] == 15 and len(patterns(actual)) == 174
    assert value["after_canonical_patterns"] - value["before_canonical_patterns"] <= 15
    return proof


def negatives(proof=None):
    proof = fixture() if proof is None else proof
    payload = (ROOT / BACKGROUND).read_bytes()
    checks = 0

    def reject(data, model):
        nonlocal checks
        try:
            png_history(data, model)
        except (AssertionError, OSError):
            checks += 1
        else:
            raise AssertionError("Campus overlay accepted an old/new/outside scope mutation")

    reject(payload + b"corruption", proof)
    for x, y in ((0,0), (400,96), (336,176), (560,720), (368,96), (416,232)):
        changed = Image.open(io.BytesIO(payload)).convert("RGB")
        old = changed.getpixel((x,y)); changed.putpixel((x,y), ((old[0] + 1) % 256, old[1], old[2]))
        output = io.BytesIO(); changed.save(output, format="PNG")
        reject(output.getvalue(), proof)
        # Even forged current hashes cannot authorize a road/ROM/outside pixel.
        if (x,y) in ((0,0), (400,96), (336,176), (560,720)):
            forged = copy.deepcopy(proof)
            forged["files"][BACKGROUND] = sha(output.getvalue()); forged["after_rgb_sha256"] = sha(changed.tobytes())
            reject(output.getvalue(), forged)
    for key, value in (("stamp_cells", proof["stamp_cells"] + [[336,176]]),
                       ("before_rgb_sha256", "changed"), ("after_rgb_sha256", "changed")):
        changed = copy.deepcopy(proof); changed[key] = value; reject(payload, changed)
    assert checks == 14
    return checks
