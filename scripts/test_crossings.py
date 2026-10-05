"""Independent source-only paint/layout/terrain checks for the crossing revision."""
import hashlib
import json
from pathlib import Path
import sys
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
sys.path.insert(0,str(GAME / "scripts"))
import crossing_protection as protection
from campus_art import patterns
from create_traffic_signals import decode


def main():
    proof = protection.check(); checks = protection.negatives(proof)
    art = json.loads((GAME / "content/crossing_art.json").read_text())
    assert art["source_colors"] == ["#071821","#306850","#86c06c","#e0f8cf"]; checks += 1
    for key in ("collision_bytes_changed","palette_priority_bytes_changed","building_footprints_changed",
                "road_geometry_changed","npc_changes","new_oam_objects","new_runtime_state_bytes",
                "net_canonical_pattern_change_per_district"):
        assert art[key] == 0; checks += 1
    counts = {"city":1113,"west":354,"high_park":320,"east":484,"port_lands":225,"north":337}
    before_counts = {"city":174,"west":172,"high_park":97,"east":98,"port_lands":120,"north":110}
    total = 0
    for slug,row in proof["backgrounds"].items():
        old = Image.open(GAME / row["previous"]).convert("RGB")
        current = Image.open(GAME / row["native"]).convert("RGB")
        native = json.loads((GAME / f"project/assets/backgrounds/toronto_{slug}.png.gbsres").read_text())
        scene = json.loads((GAME / f"project/project/scenes/toronto_{slug}/scene.gbsres").read_text())
        grid, attrs = decode(scene["collisions"]), decode(native["tileColors"])
        assert len(grid) == len(attrs) == 128*122; checks += 1
        metadata = json.loads((GAME / row["metadata"]).read_text())
        before, after = patterns(old), patterns(current)
        assert len(after) == len(before) == before_counts[slug]; checks += 1
        assert len(after-before) == len(before-after) == 2; checks += 1
        assert row["pattern_counts"] == {"before_canonical":len(before),"after_canonical":len(after),
            "introduced":2,"retired_old_crossing_patterns":2}; checks += 1
        assert len(row["stamp_cells"]) == counts[slug]; checks += 1
        cells = {tuple(p) for p in row["stamp_cells"]}
        for x,y in cells:
            i = y//8*128+x//8
            assert grid[i] == 0 and not attrs[i]&128, ("Paint moved off the clear road",slug,x,y); checks += 1
            previous = old.crop((x,y,x+8,y+8)); stripe = current.crop((x,y,x+8,y+8))
            assert set(stripe.get_flattened_data()) == {protection.RGB[1],protection.RGB[3]}; checks += 1
            # Count and bounds independently of the painter's two templates.
            white = [(px,py) for py in range(8) for px in range(8) if stripe.getpixel((px,py)) == protection.RGB[3]]
            assert len(white) == 12; checks += 1
            xs,ys = {p[0] for p in white},{p[1] for p in white}
            assert (xs == {3,4} and ys == set(range(1,7))) or (ys == {3,4} and xs == set(range(1,7))); checks += 1
            if previous.getpixel((1,0)) == protection.RGB[3]:
                assert xs == {3,4}, ("North/south zebra no longer follows road travel",slug,x,y)
            else:
                assert ys == {3,4}, ("East/west zebra no longer follows road travel",slug,x,y)
            checks += 1
        # Old crossings may be overwritten by later stopbars/props. Only
        # final visible stripes may change; all other authored placements,
        # including those later objects, remain identical at the same cells.
        for p in metadata["scenery"]["placements"]:
            if (p["x"],p["y"]) not in cells:
                x,y = p["x"],p["y"]
                assert old.crop((x,y,x+8,y+8)).tobytes() == current.crop((x,y,x+8,y+8)).tobytes(); checks += 1
        # Every signal clock/admission rule, old terrain, geometry and source
        # field is protected by raw resource pins and the older chained proofs.
        matching = [r for r in art["districts"] if r["district"] == slug]
        assert len(matching) == 1 and matching[0]["paint_cells"] == counts[slug]; checks += 1
        total += len(cells)
    assert total == 2833; checks += 1
    # The ferry-only Island has no replaced zebra pattern and remains untouched.
    traffic = json.loads((ROOT / "tests/fixtures/city_feedback_protected.json").read_text())
    island = "project/assets/backgrounds/toronto_islands.png"
    assert hashlib.sha256((GAME / island).read_bytes()).hexdigest() == traffic["raw_scopes"][island]["after_sha256"]; checks += 1
    print(f"PASS: {checks} crossing paint/layout/negative checks across 2,833 cells; no net pattern growth; native/hardware review separate")


if __name__ == "__main__":
    main()
