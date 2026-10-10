"""Generate the overlay sprite tiles (td_overlay_data.h): the police
helicopter, sharks, boats, rain, clouds and their shadows, sun rays and
gulls.

These are drawn straight into OAM by td_overlay.c rather than as GBVM
actors, from the ten 8x16 tile pairs at bank-0 tiles 236..255 that the UI
art leaves free and the four at bank-1 tiles 248..255 above the font. A
design is the left half (8 x 16) of a figure whose right half is its mirror
image, one half of a wider figure, or a single 8 x 16 sprite. Colours:
'.' transparent, 1 light, 2 mid, 3 dark (the OBJ palette's order); the
engine picks the palette per sprite. Original pixel art.

`--check` verifies the header without writing.
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'project/plugins/toronto-driving/engine/include/td_overlay_data.h'
FIRST, FIRST1 = 236, 248

# (name, rows): sixteen rows of eight. DESIGNS take bank-0 tiles 236..255;
# DESIGNS1 the bank-1 tiles 248..255 above the font (td_font.h).
DESIGNS = [
    # Police helicopter from above, nose north: the left halves of a 16x32
    # figure (cabin and glass bubble, skids, tail boom and tail rotor) in two
    # rotor positions, blades across the diagonals and fore and aft.
    ('heli_top_a', [
        "........", "........", "........", "3.......", ".3....33", "..3..311",
        "...33111", ".3..3111", ".3.33111", ".3332321", ".3.32232", ".3.32221",
        ".3.32221", ".3.32232", ".3..1311", ".3..3222"]),
    ('heli_bot_a', [
        ".3..3322", ".333.322", ".33...33", ".3....32", "3.....32", "......32",
        "......32", "......32", "......32", "......32", ".....222", "....3333",
        "......32", ".......3", ".......3", "........"]),
    ('heli_top_b', [
        "........", "........", "........", "........", "......33", ".....311",
        "....3111", ".3..3111", ".3.32111", ".3332221", "33.32222", "33333331",
        "33.32221", ".3.32222", ".3..1111", ".3..3222"]),
    ('heli_bot_b', [
        ".3...322", ".333.322", ".3....33", ".3....32", "......32", "......32",
        "......32", "......32", "......32", "......32", ".....222", "....3333",
        "......32", ".......3", ".......3", "........"]),
    # Shark fin cutting the water, with its wake (moving west; flipped east).
    ('shark_a', [
        "........", "........", "........", "....3...", "...33...", "..333...",
        ".3332...", ".3332...", "33322...", "33322111", "11111...", "..11....",
        "........", "........", "........", "........"]),
    ('shark_b', [
        "........", "........", "........", "....3...", "...33...", "..333...",
        ".3332...", "33322...", "33322...", "33322.11", ".1111111", "...111..",
        "........", "........", "........", "........"]),
    # Rain: two falling streaks.
    ('rain', [
        ".....1..", "....12..", "....2...", "...2....", "........", "........",
        "........", "........", "..1.....", ".12.....", ".2......", "2.......",
        "........", "........", "........", "........"]),
    # Cloud puff (left half) and its dithered shadow on the ground.
    ('cloud', [
        "........", "........", ".....111", "...11111", "..111111", ".1111111",
        ".1111111", "11111111", "11111111", "11111111", ".1111111", "..111111",
        "...11111", "........", "........", "........"]),
    ('cloud_shadow', [
        "........", "........", ".....3.3", "...3.3.3", "..3.3.3.", ".3.3.3.3",
        "..3.3.3.", ".3.3.3.3", "3.3.3.3.", ".3.3.3.3", "..3.3.3.", "...3.3.3",
        "....3.3.", "........", "........", "........"]),
    # A shaft of sunlight (falls from the top-left).
    ('sun_ray', [
        "1.......", ".1......", "1.1.....", ".1.1....", "..1.1...", "...1.1..",
        "....1.1.", ".....1.1", "......1.", ".......1", "........", "1.......",
        ".1......", "..1.....", "...1....", "....1..."]),
]
DESIGNS1 = [
    # A gull gliding; drawn upside down it flaps (wings down).
    ('gull', [
        "........", "........", "........", "........", "3......3", "11....11",
        ".111111.", "...11...", "...33...", "........", "........", "........",
        "........", "........", "........", "........"]),
    # A motorboat heading east (left and right halves; flipped heading west):
    # white hull, blue seats, windscreen and outboard motor.
    ('boat_l', [
        "........", "........", "........", "........", "........", "..333333",
        ".3111113", "33122213", "33122213", ".3111113", "..333333", "........",
        "........", "........", "........", "........"]),
    ('boat_r', [
        "........", "........", "........", "........", "........", "33333...",
        "111113..", "11111113", "11111113", "111113..", "33333...", "........",
        "........", "........", "........", "........"]),
    # A small sailboat under sail, heading east.
    ('sail', [
        "...3....", "...31...", "...311..", "...3111.", "...3111.", "..13111.",
        ".113111.", ".1131111", "11131111", "...3....", "33333333", "31111113",
        ".322223.", "..3333..", "........", "........"]),
]


def tile_bytes(rows):
    out = []
    for row in rows:
        lo = hi = 0
        for x, c in enumerate(row):
            v = 0 if c == '.' else int(c)
            lo |= (v & 1) << (7 - x)
            hi |= (v >> 1) << (7 - x)
        out += [lo, hi]
    return out


def build():
    assert len(DESIGNS) * 2 <= 256 - FIRST and len(DESIGNS1) * 2 <= 256 - FIRST1
    lines = ['/* Generated by scripts/create_overlay.py: overlay sprite tiles (helicopter,',
             ' * sharks, boats, rain, clouds, sun rays, gulls). Original art. */',
             '#ifndef TD_OVERLAY_DATA_H', '#define TD_OVERLAY_DATA_H',
             f'#define TD_OVERLAY_FIRST {FIRST}', f'#define TD_OVERLAY_TILES {len(DESIGNS) * 2}',
             f'#define TD_OVERLAY1_FIRST {FIRST1}', f'#define TD_OVERLAY1_TILES {len(DESIGNS1) * 2}',
             '/* Tile and OAM bank bit (0x08: VRAM bank 1) of each design. */']
    data = [[], []]
    for bank, first, designs in ((0, FIRST, DESIGNS), (1, FIRST1, DESIGNS1)):
        for k, (name, rows) in enumerate(designs):
            assert len(rows) == 16 and all(len(r) == 8 and set(r) <= set('.123') for r in rows), name
            lines.append(f'#define TD_OV_{name.upper()} {first + 2 * k}')
            lines.append(f'#define TD_OV_{name.upper()}_BANK {8 * bank}')
            data[bank] += tile_bytes(rows)
    lines.append('#ifdef TD_OVERLAY_DATA')
    for suffix, values in (('', data[0]), ('1', data[1])):
        lines.append(f'static const UBYTE td_overlay_tiles{suffix}[{len(values)}]={{')
        lines += ['    ' + ','.join(f'0x{b:02X}' for b in values[i:i + 16]) + ',' for i in range(0, len(values), 16)]
        lines.append('};')
    lines += ['#endif', '#endif', '']
    return '\n'.join(lines)


def main():
    text = build()
    if '--check' in sys.argv:
        assert OUT.read_text() == text, 'overlay tiles are stale; run scripts/create_overlay.py'
        print(f'Overlay sprite tiles match their art: {len(DESIGNS) + len(DESIGNS1)} designs.')
        return
    OUT.write_text(text)
    print(f'Wrote {OUT.relative_to(ROOT)}: {len(DESIGNS) + len(DESIGNS1)} designs.')


if __name__ == '__main__':
    main()
