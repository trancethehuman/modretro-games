# Original street scenery — 2026-10-04 hardware feedback

The background refresh adds small original benches, flower planters, bins,
wayfinding signs, harbour bollards, industrial crates and six reusable roof
modules. Parks and industrial districts use different prop families. Roof
modules stay inside existing solid generic buildings; landmark silhouettes,
roof lips, window rows and doorways retain their existing artwork.

The authored layer is `scripts/street_scenery.py`, shared by all seven existing
background generators. These are fictional decorative placements in the
compressed city, not surveyed Toronto street furniture. It adds no geography,
roads, buildings, service points, runtime state or OAM objects.

All 344 building footprints, canopy rectangles, pedestrian route metadata and
registered collision/palette/priority bytes match the pre-refresh source.
Full authored road/sidewalk/path corridors and service approaches are protected
from the added grass props. Only existing public grass cells (foot terrain16,
green palette6) admit these props, preventing furniture in Core water or closed
North private grounds. Core receives roof detail and clearer dynamic signals;
its Don/harbour water pixels retain their original artwork. A few benches reuse already solid tree-base cells.
Three job metadata files refresh only their exact source-art provenance hashes;
job values, distances, routes, deadlines, progression and rewards stay identical.
The traffic fixture refreshes only seven logical background PNGs (including four
matching original-art copies) and their seven art metadata hashes. All other
protected hashes and the historical lane baseline remain unchanged.

| District | Raw tiles | Flip-canonical tiles |
| --- | ---: | ---: |
| city | 189 | 144 |
| west | 177 | 154 |
| high_park | 99 | 85 |
| east | 106 | 85 |
| port_lands | 127 | 108 |
| islands | 125 | 83 |
| north | 114 | 94 |

Every source stays under its existing tile target. Official graphics analysis
reports four native source shades and no palette/tile violations. Registered
North receives the exact new original PNG. Native compiled tile allocation,
combined animation/OAM pressure, camera rendering and cartridge appearance
require the new ROM's separate build/play evidence; source checks do not prove
them.

The two existing dynamic signal tiles are now original two-head signals on a
visible post. Left controls east/west, right north/south. In each phase the
permitted head's bottom lamp is green; the opposing head's top lamp is red.
Unlit lamps remain dark. `scripts/create_signal_art.py --check` verifies the
32 bitmap bytes and retained source PNG. Tiles47/48, UI palette7, junction
positions, twelve-second cadence and admission rules are unchanged. The actual
C signal renderer passes21,770 host assertions, including lamp polarity, post
silhouette, reserved VRAM allocation, unchanged state and refresh lifecycle.
