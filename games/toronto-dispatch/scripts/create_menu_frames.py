"""Compile deliberately authored original menu pixels to native 2bpp patterns.

The three patterns occupy the atlas/story gap188..190. This source generator
does not establish compiled allocation or native gameplay appearance.
"""
from pathlib import Path
import json

GAME = Path(__file__).resolve().parents[1]
SOURCE = GAME / "content/menu_frames.json"
OUTPUT = GAME / "project/plugins/toronto-driving/engine/include/td_menu_frames.h"


def compile_source(source):
    if source["tile_bank"] != 1 or source["first_tile"] != 188 or source["last_tile"] != 190:
        raise ValueError("Original menu patterns must stay inside their three-tile bank1 ownership gap")
    if source["palette_slot"] != 7 or len(source["patterns"]) != 3:
        raise ValueError("Menu frames require exactly three original four-color UI patterns")
    pixels = []
    for pattern in source["patterns"]:
        rows = pattern["rows"]
        if len(rows) != 8 or any(len(row) != 8 or set(row) - set("0123") for row in rows):
            raise ValueError("Each native menu pattern needs eight deliberate eight-pixel rows")
        for row in rows:
            pixels.extend(sum(((int(value) >> bit) & 1) << (7-x) for x, value in enumerate(row)) for bit in (0, 1))
    lines = ["/* Generated original pixels from content/menu_frames.json; MIT. */",
             "#ifndef TD_MENU_FRAMES_H", "#define TD_MENU_FRAMES_H",
             "#define TD_MENU_FRAME_FIRST 188", "#define TD_MENU_FRAME_COUNT 3",
             "static const UBYTE td_menu_frame_pixels[]={"]
    for offset in range(0, len(pixels), 16):
        lines.append("    " + ",".join(f"0x{value:02x}" for value in pixels[offset:offset+16]) + ",")
    lines.extend(["};", "#endif", ""])
    return "\n".join(lines)


def main():
    OUTPUT.write_text(compile_source(json.loads(SOURCE.read_text())))
    print("Generated three original bank1 menu-frame patterns in188..190; gameplay/font/live badges unchanged.")


if __name__ == "__main__":
    main()
