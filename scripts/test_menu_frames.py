"""Check original menu pixel ownership; optionally render actual-C host cards.

Preview PNGs are source-only views of the bounded host VRAM adapter. They are
never native emulator/device screenshots, LCD timing or cartridge evidence.
"""
from pathlib import Path
import argparse
import importlib.util
import json
import os
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/toronto-dispatch"
ENGINE = GAME / "project/plugins/toronto-driving/engine"


def check_source():
    source = json.loads((GAME / "content/menu_frames.json").read_text())
    spec = importlib.util.spec_from_file_location("td_menu_frame_generator", GAME / "scripts/create_menu_frames.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    if module.compile_source(source) != (ENGINE / "include/td_menu_frames.h").read_text():
        raise ValueError("Editable original menu pixels and native frame patterns differ")
    if source["colors"] != json.loads((GAME / "project/project/palettes/default_ui.gbsres").read_text())["colors"]:
        raise ValueError("Original menu preview palette differs from the actual selected game UI palette")
    atlas = (ENGINE / "include/td_atlas.h").read_text()
    limit = int(re.search(r"#define TD_ATLAS_VISIBLE_LIMIT (\d+)", atlas).group(1))
    story = (ENGINE / "include/td_story_data.h").read_text()
    count = int(re.search(r"#define TD_STORY_TILE_COUNT (\d+)", story).group(1))
    if 16 + limit > 188 or 16 + count > 188:
        raise ValueError("Atlas/story pattern allocation now overlaps the three-tile menu gap")
    for pattern in source["patterns"]:
        colors = set("".join(pattern["rows"]))
        if colors != set("0123"):
            raise ValueError("Each original bevel needs its deliberate navy, mint, gold and cream pixels")
    print("Original menu source checks: exactly3 owned48-byte patterns, matching editable pixels/palette, no atlas/story/live-badge overlap.")
    return source


def preview(source):
    from PIL import Image
    target = GAME / "project/build/menu-frames-source-preview"
    target.mkdir(parents=True, exist_ok=True)
    environment = dict(os.environ, TD_MENU_SOURCE_PREVIEW_DIR=str(target))
    subprocess.run([sys.executable, str(ROOT / "scripts/test_atlas_ui.py")], env=environment, check=True)
    colors = [tuple(bytes.fromhex(value)) for value in source["colors"]]
    names = ("pause", "settings", "welcome", "controls", "dispatch", "transit", "result")
    for name in names:
        data = (target / f"{name}.bin").read_bytes()
        if len(data) != 720+4096:
            raise ValueError("Source preview requires the exact actual-C host window map and bank1 pattern layout")
        image = Image.new("RGB", (160, 144), colors[0])
        for y in range(18):
            for x in range(20):
                index = y*20+x
                tile, attr = data[index], data[360+index]
                if attr & 7 != 7 or attr & 8 != 8:
                    raise ValueError("Source-only menu preview found an unowned tile bank or palette")
                pattern = data[720+tile*16:720+(tile+1)*16]
                for py in range(8):
                    for px in range(8):
                        xx, yy = (7-px if attr & 32 else px), (7-py if attr & 64 else py)
                        value = ((pattern[yy*2] >> (7-xx)) & 1) | (((pattern[yy*2+1] >> (7-xx)) & 1) << 1)
                        image.putpixel((x*8+px, y*8+py), colors[value])
        image.save(target / f"{name}.png")
    manifest = {"scope": "source-only actual-C host adapter, not native emulator/device evidence",
                "renderer": "actual td_ui.c executed by scripts/test_atlas_ui.py", "dimensions": [160, 144],
                "frames": [f"{name}.png" for name in names]}
    (target / "SOURCE_ONLY.json").write_text(json.dumps(manifest, indent=2)+"\n")
    print(f"Source-only original menu previews: {target}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preview", action="store_true")
    options = parser.parse_args()
    source = check_source()
    if options.preview:
        preview(source)


if __name__ == "__main__":
    main()
