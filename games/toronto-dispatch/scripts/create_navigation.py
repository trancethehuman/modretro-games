"""Compile collision-derived cardinal quest directions into bounded ROM units.

There is no runtime search, world cache, spawn, save or movement mutation. Car
routes retain the full half7 terrain body and prefer asphalt to walkable ground;
foot routes retain a half3 body. Named client/parking, seam and ferry coordinates
are the only goals. Each direction strictly decreases its offline potential.
"""
import heapq
import json
from pathlib import Path
import sys

from check_campaign import decode

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "project/plugins/toronto-driving/engine"
ROW_BYTES_PER_UNIT = 6500
PATTERN_BYTES_PER_UNIT = 6500
# Leave room for the mask decoder within its native 16KiB ROM bank. The
# official linked build still owns the final code+constant placement check.
MASK_BYTES_PER_UNIT = 15600
WIDTH, HEIGHT = 128, 122
DIRECTIONS = ((0, -1), (1, 0), (0, 1), (-1, 0))


def inputs():
    world = json.loads((ROOT / "content/districts/world.json").read_text())
    campaign = json.loads((ROOT / "content/campaign.json").read_text())
    goals = set()
    for stop in campaign["stops"]:
        goals.add((stop["district"], stop["u"], stop["v"]))
        if "parking_anchor" in stop:
            parking = stop["parking_anchor"]
            goals.add((stop["district"], parking["u"], parking["v"]))
    for portal in world["portals"]:
        for point in (portal["from"], portal["to"]):
            goals.add((point["district"], point["u"], point["v"]))
    grids = []
    for district in world["districts"]:
        scene = json.loads((ROOT / "project/project/scenes" / district["scene"] / "scene.gbsres").read_text())
        assert (scene["width"], scene["height"]) == (WIDTH, HEIGHT)
        grids.append(decode(scene["collisions"]))
    assert len(grids) == 7 and [d["id"] for d in world["districts"]] == list(range(7))
    goals = sorted(goals, key=lambda p: (p[0], p[2], p[1]))
    assert 0 < len(goals) < 255
    return goals, grids


def legal_cells(grid, onfoot):
    half = 3 if onfoot else 7
    legal = []
    for y in range(HEIGHT):
        for x in range(WIDTH):
            u, v = x * 8 + 4, y * 8 + 4
            legal.append(8 <= u <= 1016 and 8 <= v <= 968 and all(
                not grid[row * WIDTH + col] & 15
                for row in range((v - half) // 8, (v + half) // 8 + 1)
                for col in range((u - half) // 8, (u + half) // 8 + 1)))
    return legal


def field(grid, legal, target, onfoot):
    u, v = target
    # A surveyed-style handoff may lie on a tile boundary. Seed a nearby full
    # body centre within the same native <15px interaction radius, never across
    # a building/rail. Ordinary native target coordinates remain unchanged.
    candidates = []
    for y in range(max(1, v // 8 - 2), min(HEIGHT - 1, v // 8 + 3)):
        for x in range(max(1, u // 8 - 2), min(WIDTH - 1, u // 8 + 3)):
            pu, pv = x * 8 + 4, y * 8 + 4
            if legal[y * WIDTH + x] and abs(pu - u) < 15 and abs(pv - v) < 15:
                # This connection must stay on the target's connected raw
                # walkable tiles; a close opposite side of a rail is no goal.
                xs = range(min(pu, u) // 8, max(pu, u) // 8 + 1)
                ys = range(min(pv, v) // 8, max(pv, v) // 8 + 1)
                if all(not grid[ry * WIDTH + rx] & 15 for ry in ys for rx in xs):
                    candidates.append((abs(pu - u) + abs(pv - v), y * WIDTH + x))
    distances = [65535] * (WIDTH * HEIGHT)
    values = [0] * (WIDTH * HEIGHT)
    if not candidates:
        return values, distances
    _, seed = min(candidates)
    distances[seed] = 0
    queue = [(0, seed)]
    while queue:
        cost, position = heapq.heappop(queue)
        if cost != distances[position]:
            continue
        x, y = position % WIDTH, position // WIDTH
        # Entering a walkable sidewalk/path costs more for a driving route.
        # This affects the advice only, never controls/terrain admission.
        step = 1 if onfoot or grid[position] == 0 else 8
        for dx, dy in DIRECTIONS:
            nx, ny = x + dx, y + dy
            if not (0 <= nx < WIDTH and 0 <= ny < HEIGHT):
                continue
            other = ny * WIDTH + nx
            if legal[other] and cost + step < distances[other]:
                distances[other] = cost + step
                heapq.heappush(queue, (cost + step, other))
    for position, cost in enumerate(distances):
        if cost == 65535:
            continue
        if cost == 0:
            values[position] = 5
            continue
        x, y = position % WIDTH, position // WIDTH
        choices = []
        for direction, (dx, dy) in enumerate(DIRECTIONS, 1):
            nx, ny = x + dx, y + dy
            if not (0 <= nx < WIDTH and 0 <= ny < HEIGHT):
                continue
            other = ny * WIDTH + nx
            step = 1 if onfoot or grid[other] == 0 else 8
            if distances[other] + step == cost:
                choices.append((abs(nx * 8 + 4 - u) + abs(ny * 8 + 4 - v), direction))
        assert choices, "Every nonterminal policy cell has a decreasing neighbour"
        values[position] = min(choices)[1]
    return values, distances


def packed_row(row):
    if not row:
        return bytes((1, 0, 0))
    pairs = []
    value = row[0]
    for x in range(1, len(row)):
        if row[x] != value:
            pairs.append(((x - 1) << 3) | value)
            value = row[x]
    pairs.append(((len(row) - 1) << 3) | value)
    # A ten-bit pair retains exact inclusive end0..127 and direction0..5.
    bits = sum(pair << (i * 10) for i, pair in enumerate(pairs))
    return bytes((len(pairs),)) + bits.to_bytes((len(pairs) * 10 + 7) // 8, "little")


def model():
    goals, grids = inputs()
    masks = [[legal_cells(grid, mode) for mode in range(2)] for grid in grids]
    mask_dictionary, mask_patterns, mask_indices = {}, [], []
    for district in range(len(grids)):
        for onfoot in range(2):
            for y in range(HEIGHT):
                bits = sum(int(masks[district][onfoot][y * WIDTH + x]) << x for x in range(WIDTH))
                pattern = bits.to_bytes(16, "little")
                if pattern not in mask_dictionary:
                    mask_dictionary[pattern] = len(mask_patterns)
                    mask_patterns.append(pattern)
                mask_indices.append(mask_dictionary[pattern])
    dictionary, patterns, indices = {}, [], []
    for district, u, v in goals:
        for onfoot in range(2):
            values, _ = field(grids[district], masks[district][onfoot], (u, v), onfoot)
            for y in range(HEIGHT):
                pattern = packed_row([values[y * WIDTH + x] for x in range(WIDTH)
                                      if masks[district][onfoot][y * WIDTH + x]])
                if pattern not in dictionary:
                    dictionary[pattern] = len(patterns)
                    patterns.append(pattern)
                indices.append(dictionary[pattern])
    assert len(patterns) < 65535 and len(indices) < 65535
    units, unit, size = [], [], 0
    for pattern in patterns:
        if unit and size + len(pattern) + 2 * (len(unit) + 1) > PATTERN_BYTES_PER_UNIT:
            units.append(unit)
            unit, size = [], 0
        unit.append(pattern)
        size += len(pattern)
    if unit:
        units.append(unit)
    assert len(units) <= 64
    return goals, units, indices, mask_patterns, mask_indices


def files(data=None):
    goals, units, indices, mask_patterns, mask_indices = model() if data is None else data
    result = {}
    declarations = ["/* Generated navigation ROM API; output never exposes a ROM pointer. */",
                    "#ifndef TD_NAVIGATION_DATA_H", "#define TD_NAVIGATION_DATA_H",
                    '#include "td_navigation.h"']
    declarations.append("UBYTE td_navigation_ordinal(UBYTE district,UBYTE onfoot,UBYTE x,UBYTE y) BANKED;")
    assert len(mask_patterns) < 65535
    mask_type = "UBYTE" if len(mask_patterns) < 256 else "UWORD"
    lines = ["/* Generated shared full-body collision masks; no saved/runtime cache. */",
             "#pragma bank 255", '#include "td_navigation_data.h"',
             "static const UBYTE masks[][16]={"]
    lines += ["    {" + ",".join(map(str, pattern)) + "}," for pattern in mask_patterns]
    lines += ["};", f"static const {mask_type} rows[]={{"]
    lines += ["    " + ",".join(map(str, mask_indices[i:i + 24])) + "," for i in range(0, len(mask_indices), 24)]
    # Count the set bits before each four-byte block once at generation time.
    # Ordinal lookup then scans at most three complete bytes, retaining the
    # exact mask and direction data without adding any runtime route memory.
    prefixes = [[sum(value.bit_count() for value in pattern[:offset])
                 for offset in range(0, 16, 4)] for pattern in mask_patterns]
    lines += ["};", "static const UBYTE prefixes[][4]={"]
    lines += ["    {" + ",".join(map(str, prefix)) + "}," for prefix in prefixes]
    lines += ["};", "static const UBYTE counts[]={0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};",
              "UBYTE td_navigation_ordinal(UBYTE district,UBYTE onfoot,UBYTE x,UBYTE y) BANKED {",
              "    const UBYTE *mask;UWORD pattern;UBYTE i,value,count;",
              "    if(district>=7||onfoot>1||x>=128||y>=122)return 255;",
              "    pattern=rows[((UWORD)district*2+onfoot)*122+y];mask=masks[pattern];",
              "    if(!(mask[x>>3]&(1<<(x&7))))return 255;",
              "    count=prefixes[pattern][x>>5];",
              "    for(i=(x>>5)<<2;i<(x>>3);i++){value=mask[i];count+=counts[value&15]+counts[value>>4];}",
              "    value=mask[x>>3]&((1<<(x&7))-1);",
              "    return count+counts[value&15]+counts[value>>4];", "}", ""]
    assert len(mask_patterns) * 20 + len(mask_indices) * (1 if mask_type == "UBYTE" else 2) + 16 <= MASK_BYTES_PER_UNIT
    result["src/td_navigation_masks.c"] = "\n".join(lines)
    offsets = [0]
    for unit, patterns in enumerate(units):
        offsets.append(offsets[-1] + len(patterns))
        name = f"td_navigation_pattern_{unit}"
        declarations.append(f"UBYTE {name}(UWORD pattern,UBYTE column) BANKED;")
        starts, payload = [], bytearray()
        for pattern in patterns:
            starts.append(len(payload))
            payload.extend(pattern)
        assert len(payload) + 2 * len(starts) <= PATTERN_BYTES_PER_UNIT
        lines = ["/* Generated exact ten-bit row runs; no persistent RAM. */", "#pragma bank 255",
                 '#include "td_navigation_data.h"',
                 "static const UWORD offsets[]={" + ",".join(map(str, starts)) + "};",
                 "static const UBYTE data[]={"]
        lines += ["    " + ",".join(map(str, payload[i:i + 32])) + "," for i in range(0, len(payload), 32)]
        lines += ["};", f"UBYTE {name}(UWORD pattern,UBYTE column) BANKED {{",
                  "    UWORD start,bit,value;UBYTE low,high,middle,shift;",
                  f"    if(pattern>={len(patterns)}||column>=128)return 0;",
                  "    start=offsets[pattern];low=0;high=data[start++];",
                  "    while(low<high){middle=low+(high-low)/2;bit=(UWORD)middle*10;shift=bit&7;",
                  "        value=(UWORD)data[start+(bit>>3)]|((UWORD)data[start+(bit>>3)+1]<<8);",
                  "        value=(value>>shift)&1023;",
                  "        if((value>>3)<column)low=middle+1;else high=middle;}",
                  "    if(low==data[start-1])return 0;",
                  "    bit=(UWORD)low*10;shift=bit&7;",
                  "    value=(UWORD)data[start+(bit>>3)]|((UWORD)data[start+(bit>>3)+1]<<8);",
                  "    return (value>>shift)&7;", "}", ""]
        # Every ten-bit value occupies two bytes, including the last, so the
        # decoder's unshifted read never crosses a generated row boundary.
        assert all((pattern[0] - 1) * 10 // 8 + 2 <= len(pattern) - 1 for pattern in patterns)
        result[f"src/td_navigation_patterns_{unit}.c"] = "\n".join(lines)
    row_fields = []
    for field_index in range(len(goals) * 2):
        rows = indices[field_index * 122:(field_index + 1) * 122]
        pairs, pattern = [], rows[0]
        for y in range(1, 122):
            if rows[y] != pattern:
                pairs.append((y - 1, pattern))
                pattern = rows[y]
        pairs.append((121, pattern))
        payload = bytearray((len(pairs),))
        for y, pattern in pairs:
            payload.extend((y, pattern & 255, pattern >> 8))
        row_fields.append(bytes(payload))
    row_units, group, size, row_offsets = [], [], 0, [0]
    for row in row_fields:
        if group and size + len(row) + 2 * (len(group) + 1) > ROW_BYTES_PER_UNIT:
            row_units.append(group)
            row_offsets.append(row_offsets[-1] + len(group))
            group, size = [], 0
        group.append(row)
        size += len(row)
    if group:
        row_units.append(group)
        row_offsets.append(row_offsets[-1] + len(group))
    for unit, fields in enumerate(row_units):
        name = f"td_navigation_rows_{unit}"
        declarations.append(f"UWORD {name}(UBYTE field,UBYTE row) BANKED;")
        starts, payload = [], bytearray()
        for field in fields:
            starts.append(len(payload))
            payload.extend(field)
        lines = ["/* Generated cardinal direction row IDs; no persistent RAM. */", "#pragma bank 255",
                 '#include "td_navigation_data.h"',
                 "static const UWORD offsets[]={" + ",".join(map(str, starts)) + "};",
                 "static const UBYTE rows[]={"]
        lines += ["    " + ",".join(map(str, payload[i:i + 24])) + "," for i in range(0, len(payload), 24)]
        lines += ["};", f"UWORD {name}(UBYTE field,UBYTE row) BANKED {{",
                  "    UWORD start,position;UBYTE low=0,high,middle;",
                  f"    if(field>={len(fields)}||row>=122)return 65535;",
                  "    start=offsets[field];high=rows[start++];",
                  "    while(low<high){middle=low+(high-low)/2;",
                  "        if(rows[start+(UWORD)middle*3]<row)low=middle+1;else high=middle;}",
                  "    if(low==rows[start-1])return 65535;position=start+(UWORD)low*3;",
                  "    return (UWORD)rows[position+1]|((UWORD)rows[position+2]<<8);", "}", ""]
        result[f"src/td_navigation_rows_{unit}.c"] = "\n".join(lines)
    declarations += ["#endif", ""]
    result["include/td_navigation_data.h"] = "\n".join(declarations)
    lines = ["/* Generated registered quest/parking/seam/ferry coordinates. */",
             "#ifndef TD_NAVIGATION_GOALS_H", "#define TD_NAVIGATION_GOALS_H",
             f"#define TD_NAVIGATION_GOALS {len(goals)}", "static const td_navigation_goal_t td_navigation_goals[]={"]
    lines += ["    {%d,%d,%d}," % (u, v, district) for district, u, v in goals]
    lines += ["};", "#endif", ""]
    result["include/td_navigation_goals.h"] = "\n".join(lines)
    lines = ["/* Generated BANKED dispatch; all caller coordinates are values. */", "#pragma bank 255",
             '#include "td_navigation_data.h"', "UBYTE td_navigation_cell(UWORD goal,UBYTE onfoot,UBYTE x,UBYTE y) BANKED {",
             "    UWORD pattern;UBYTE field,column;",
             "    static const UBYTE districts[]={" + ",".join(str(p[0]) for p in goals) + "};",
             f"    if(goal>={len(goals)}||onfoot>1||x>=128||y>=122)return 0;",
             "    column=td_navigation_ordinal(districts[goal],onfoot,x,y);if(column==255)return 0;",
             "    field=goal*2+onfoot;"]
    for unit in range(len(row_units)):
        lines += [f"    {'if' if not unit else 'else if'}(field<{row_offsets[unit+1]})pattern=td_navigation_rows_{unit}(field-{row_offsets[unit]},y);"]
    lines += ["    else return 0;"]
    for unit in range(len(units)):
        lines += [f"    if(pattern<{offsets[unit+1]})return td_navigation_pattern_{unit}(pattern-{offsets[unit]},column);"]
    lines += ["    return 0;", "}", ""]
    result["src/td_navigation_data.c"] = "\n".join(lines)
    return result


if __name__ == "__main__":
    generated = files()
    if "--check" in sys.argv:
        for relative, text in generated.items():
            assert (ENGINE / relative).read_text() == text, f"Navigation ROM source is stale: {relative}"
        print(f"Navigation ROM fields match exact registered bodies/goals: {len(generated)} bounded units")
    else:
        for path in [*ENGINE.glob("src/td_navigation_patterns_*.c"), *ENGINE.glob("src/td_navigation_rows_*.c")]:
            if str(path.relative_to(ENGINE)) not in generated:
                path.unlink()
        for relative, text in generated.items():
            (ENGINE / relative).write_text(text)
        print(f"Generated {len(generated)} bounded navigation ROM units; no persistent route memory")
