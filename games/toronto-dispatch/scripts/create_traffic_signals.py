"""Original fictional signal junctions derived from registered road geometry.

Coordinates/phases are game design, not surveyed lights or a TTC timetable.
Only junctions with three straight, 24px approaches receive a signal. Bends
alone do not create phantom lights. Registered collision must support the
centre and each emitted approach's 5px body at an 8px lane offset.
"""
import json
import sys
from pathlib import Path

from check_campaign import decode
from city_layout import COLS, ROWS
from district_sources import read_district_art

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "project/plugins/toronto-driving/engine/include/td_traffic_signals.h"


def intervals(segments, horizontal):
    groups = {}
    for a, b in segments:
        if (a[1] == b[1]) != horizontal:
            continue
        lateral = a[1] if horizontal else a[0]
        ends = sorted((a[0], b[0]) if horizontal else (a[1], b[1]))
        groups.setdefault(lateral, []).append(ends)
    merged = []
    for lateral, values in sorted(groups.items()):
        low, high = sorted(values)[0]
        for start, end in sorted(values)[1:]:
            if start <= high:
                high = max(high, end)
            else:
                merged.append((lateral, low, high))
                low, high = start, end
        merged.append((lateral, low, high))
    return merged


def model():
    world = json.loads((ROOT / "content/districts/world.json").read_text())
    entries = world["districts"]
    assert [d["id"] for d in entries] == list(range(len(entries)))
    output = []
    for district in entries:
        scene = json.loads((ROOT / "project/project/scenes" / district["scene"] / "scene.gbsres").read_text())
        assert (scene["width"], scene["height"]) == (128, 122)
        grid = decode(scene["collisions"])
        background = json.loads((ROOT / "project/assets/backgrounds" / (district["scene"] + ".png.gbsres")).read_text())
        assert background["id"] == scene["backgroundId"]
        attrs = decode(background["tileColors"])
        assert len(attrs) == len(grid)
        if district.get("traffic_enabled", True) is False:
            art = read_district_art(district)
            assert art["roads"] == [] and art["traffic_loops"] == [] and 0 not in grid, 'Disabled traffic must not hide drivable roads'
            output.append([])
            continue

        def drivable(u, v):
            return 8 <= u <= 1016 and 8 <= v <= 968 and all(
                grid[y * 128 + x] == 0
                for y in range((v - 5) // 8, (v + 5) // 8 + 1)
                for x in range((u - 5) // 8, (u + 5) // 8 + 1))

        if district["id"] == 0:
            roads = [[(0 if y in (64, 288, 400, 528, 640) else 24, y),
                      (848 if y in (640, 720) else 1024 if y in (64, 400, 528) else 992, y)]
                     for y in ROWS]
            roads += [[(x, 24), (x, 816)] for x in COLS]
        else:
            slug = district["scene"].removeprefix("toronto_")
            art = read_district_art(district)
            roads = [road["points"] for road in art["roads"]]
        segments = [(a, b) for road in roads for a, b in zip(road, road[1:])]
        assert all((a[0] == b[0]) != (a[1] == b[1]) for a, b in segments)
        hs, vs = intervals(segments, True), intervals(segments, False)
        candidates = {(x, y) for y, left, right in hs for x, top, bottom in vs
                      if left <= x <= right and top <= y <= bottom}
        signals = []
        for u, v in sorted(candidates, key=lambda p: (p[1], p[0])):
            arms = 0
            for y, left, right in hs:
                if y == v and left <= u <= right:
                    if u - left >= 24 and all(drivable(u - 24, v + lane) for lane in (-8, 8)):
                        arms |= 8
                    if right - u >= 24 and all(drivable(u + 24, v + lane) for lane in (-8, 8)):
                        arms |= 2
            for x, top, bottom in vs:
                if x == u and top <= v <= bottom:
                    if v - top >= 24 and all(drivable(u + lane, v - 24) for lane in (-8, 8)):
                        arms |= 1
                    if bottom - v >= 24 and all(drivable(u + lane, v + 24) for lane in (-8, 8)):
                        arms |= 4
            if arms.bit_count() >= 3 and drivable(u, v):
                corners = [(u + 24, v - 32), (u - 32, v - 32), (u + 24, v + 24), (u - 32, v + 24)]
                clear = [(x // 8, y // 8) for x, y in corners if 0 <= x < 1024 and 0 <= y < 976
                         and not grid[y // 8 * 128 + x // 8] & 15 and not attrs[y // 8 * 128 + x // 8] & 128]
                assert clear, f"{district['scene']}: no non-priority pavement corner at {(u, v)}"
                signals.append((u, v, arms, *clear[0]))
        assert signals, f"{district['scene']}: no supported junctions"
        output.append(signals)
    return output


def source(data=None):
    data = model() if data is None else data
    offsets = [0]
    for signals in data:
        offsets.append(offsets[-1] + len(signals))
    assert offsets[-1] <= 65535
    text = ("/* Generated by create_traffic_signals.py from actual authored roads/collision.\n"
            " * Original fictional 12-second lights; not surveyed Toronto signals. */\n"
            "#ifndef TD_TRAFFIC_SIGNALS_H\n#define TD_TRAFFIC_SIGNALS_H\n"
            f"#define TD_TRAFFIC_SIGNAL_DISTRICTS {len(data)}\n"
            "static const UWORD td_signal_offsets[]={" + ",".join(map(str, offsets)) + "};\n")
    for name, order in (("h", lambda p: (p[1], p[0])), ("v", lambda p: (p[0], p[1]))):
        if name == "v":
            text += "#ifndef TD_TRAFFIC_SIGNALS_HORIZONTAL_ONLY\n"
        text += f"static const td_signal_t td_signals_{name}[]={{\n"
        for signals in data:
            if signals:
                text += "  " + ",".join("{%d,%d,%d,%d,%d}" % p for p in sorted(signals, key=order)) + ",\n"
        text += "};\n"
        if name == "v":
            text += "#endif\n"
    return text + "#endif\n"


if __name__ == "__main__":
    generated = source()
    if "--check" in sys.argv:
        assert HEADER.read_text() == generated, "Native traffic signal geometry is stale"
        print("Traffic signals match registered roads and collision")
    else:
        HEADER.write_text(generated)
        print("Generated supported fictional junctions: " + ", ".join(map(str, map(len, model()))))
