"""Original thin crosswalk paint, layered over the preserved city source art.

The old generators keep their geometry and historical artwork identities. This
small final native-background layer replaces only complete old crossing paint
cells: two-pixel white bars have asphalt margins, rather than the old four-pixel
bars that filled the cell from edge to edge. Stop lines, curb signs, streetcar
rails, furniture, centre marks and intersection centres are never repainted.
"""
import io
from PIL import Image

RGB = ((7,24,33), (48,104,80), (134,192,108), (224,248,207))
THIN_ROWS = ("11111111",) + ("11133111",) * 6 + ("11111111",)
BACKGROUND_METADATA = {
    "city": "city_art.json",
    "west": "districts/west_art.json",
    "high_park": "districts/high_park_art.json",
    "east": "districts/east_art.json",
    "port_lands": "districts/port_lands_art.json",
    "north": "districts/north_art.json",
}


def _tile(rows):
    image = Image.new("RGB", (8,8))
    image.putdata([RGB[int(c)] for row in rows for c in row])
    return image


def templates():
    before = _tile(("13333111",) * 8)
    after = _tile(THIN_ROWS)
    return ((before, after),
            (before.transpose(Image.Transpose.ROTATE_90),
             after.transpose(Image.Transpose.TRANSPOSE)))


def repaint(image, metadata):
    """Repaint only visible final crossing strokes; preserve later overlays."""
    assert image.mode == "RGB" and image.size == (1024,976)
    old_to_new = {before.tobytes(): after for before,after in templates()}
    cells = {(p["x"],p["y"]) for p in metadata["scenery"]["placements"]
             if p["kind"] == "crosswalk"}
    changed = []
    for x,y in sorted(cells, key=lambda c: (c[1],c[0])):
        assert x % 8 == y % 8 == 0 and 0 <= x <= image.width-8 and 0 <= y <= image.height-8
        before = image.crop((x,y,x+8,y+8)).tobytes()
        if before in old_to_new:
            image.paste(old_to_new[before], (x,y))
            changed.append([x,y])
    return changed


def native_bytes(payload, metadata):
    """Keep historical PNGs/metadata exact; emit a separately checked native layer."""
    with Image.open(io.BytesIO(payload)) as source:
        assert source.format == "PNG" and source.mode == "RGB"
        image = source.copy()
    if not repaint(image, metadata):
        return payload
    output = io.BytesIO(); image.save(output, format="PNG")
    return output.getvalue()
