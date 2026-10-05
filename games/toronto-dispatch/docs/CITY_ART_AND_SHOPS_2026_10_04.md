# City art and walk-in shops, 2026-10-04

The complete feedback batch calls for a busier, readable north-up city. This
iteration adds original crossing paint, crossing signs, service-stop poles,
curb parking bays, small parking courts, facade styles and visible water waves
across the seven registered districts. It improves the existing car, truck,
motorcycle, scooter and emergency fleet artwork while preserving the approved
courier and civilian pixels. Three marked fictional shops now have genuine
160×144 native scenes with shelves, a keeper and a return doorway.

## Geography and source boundaries

Reviewed the City's [TO360 downtown pedestrian map, October 2024](https://www.toronto.ca/wp-content/uploads/2024/11/9774-TO360QRDowntownDistrictAccessible.pdf),
the [Downtown Plan map](https://www.toronto.ca/wp-content/uploads/2018/08/966f-city-planning-tocore-opa406-attachment-1-schedule-5-downtown-plan.pdf),
and the [TTC subway/streetcar schematic](https://www.ttc.ca/-/media/Project/TTC/DevProto/Documents/Home/Accessibility/TTC_SubwayStreetcarMap_Printable.pdf)
on 2026-10-04. These support downtown street ordering, the waterfront's position,
major landmark regions and the distinction between subway and surface routes.
The sources are references for geography, rather than imported map artwork.
No map, photograph, logo, raw geometry or public artwork enters the game assets.
Furniture, shops, parking courts, facade modules and vehicles are original MIT
game art. Shop names and entrances are fictional game design.

The existing seven-district compression remains geographically recognisable:
High Park and the western neighbourhoods lie west of downtown; downtown's
east-west streets run towards the Don and the eastern neighbourhoods; the
Port Lands lie south of the eastern mainland; the harbour and ferry-accessed
Islands remain water-separated. North continues the existing mainland street
connections. The runtime atlas, source district documents and mission routes
remain the authority for compressed connections.

This change preserves every registered outdoor collision cell, priority byte
and all 344 building footprints. It is not a surveyed, complete reproduction
of Old Toronto. Stop18 previously said `OSSINGTON BUS` at144/64, between the
Core's Dufferin80 and Bathurst208 columns. The actual original road labels,
source columns and native street lookup were checked together. Stop8 remains
correctly `DUFFERIN` at80/528. Stop18 is corrected to `BLOORCOURT BUS` without
changing its coordinate, ordinal, service, flags or mission references. The
City's [Bloorcourt BIA description](https://www.toronto.ca/business-economy/business-operation-growth/business-improvement-areas/bia-list/bia-list-a-e/)
places that region on Bloor West between Dufferin and Montrose. The region
supports this name, but an exact TTC stop at the compressed coordinate is not
established. It remains an explicitly fictional regional bus handoff with a
2026-10-04 source notice. Strict historical normalization allows this one
display correction while preserving immutable old campaign hashes. Current
source and native checks require the new label. The Core CN Tower and Union skyline
footprints overlap at this scale. Existing district documents also identify
conditional street/bridge compression. These remain fidelity limitations;
new art must not be reported as resolving them.

## Readable city details

Crosswalk stripes follow the actual authored junctions, with zebra paint on
each existing approach and crossing signs on clear curb cells. Entering-lane
stop bars sit 17–18 pixels from each junction centre. With the native NPC centre
stop at 24 pixels and a 5–7 pixel vehicle nose, the front remains behind the
bar. Art alone does not establish red-light behavior; native traffic tests do.

Bus and streetcar poles belong to matching registered service points. Subway
stations and ferry docks do not receive bus furniture. Park signs, benches,
bins, flowers and industrial crates use repeatable cells outside protected
corridors. Shop and glass facade modules are painted within existing solid
buildings, leaving roof priority, doors and footprints intact. Distinct roof
modules reuse existing tile patterns.

A fictional fire garage is marked `FIRE` inside the existing unnamed24×24
building at240/464 in Core. Its16×8 original sign at248/472 leaves the building's
door, footprint and palette priority intact. Existing sidewalk route57 runs
240–303 at y500; its identity selects the approved helmet-worker variant1.
Ordinary route walking, offscreen admission, social interaction and impacts
provide ambient crew activity without adding actors or changing human pixels.
This represents a fictional service garage, not a surveyed real fire station.
The final native frame near272/500 must confirm readable context and a visible
worker. No source placement alone establishes simultaneous native visibility.

Painted curb bays expose candidate coordinates for interactive parked cars;
the native sandbox module owns actual vehicles and theft. Five small parking
courts use otherwise unused ground. Water waves are two static background
motifs placed only in actual blocked water cells. The boat module supplies the
moving wake. Static scenery uses no new outdoor OAM objects or runtime state.

## Native shops

| Door | Outdoor foot coordinate | Native scene |
| --- | --- | --- |
| Harbour Grocer | Core, 124/152 | `scene_toronto_shop_grocery` |
| West End Corner | West, 64/180 | `scene_toronto_shop_corner_store` |
| East End Repairs | East, 768/148 | `scene_toronto_shop_repair_shop` |

Fresh A on foot within 12 pixels of a door queues the existing locked,
faded GBVM scene-change sequence. `td_shops_pending()` freezes the outdoor
step immediately after an accepted queue. `TORONTO_SHOP` moves a separate
courier actor through the room's actual full-body collision grid; the outdoor
district, exact foot position, parked vehicle and saved mission fields remain
untouched. Keeper interaction shows a brief greeting. Fresh B leaves from
anywhere; A near the bottom door or walking onto its threshold also leaves.
Held entry A is consumed until release.

`td_shop_world_seconds(elapsed)` advances the shared clock, deadline and music.
If the deadline fails, the room returns to the original district and preserves
the mission result. Routine outdoor HUD drawing is omitted while inside so
the room's greeting remains visible. Shops add only transient module state;
`td_shops_reset()` on session bootstrap discards it. Saves continue to restore
outdoors at the exact safe entrance and require no new format.

## Source and logic verification

The official plugin registered all three actual backgrounds, scene resources,
collision grids and civilian keepers. Official `graphics_analyze` accepted all
12 revised/new native art assets with zero reported violations; a final
independent analysis of all seven updated outdoor PNGs also returned valid
with zero violations after the fence/pole layer. After the final fence/pole layer, outdoor
raw/flip-canonical tile counts are Core205/159, West200/172, HighPark115/97,
East124/98, Port145/120, Islands130/88 and North133/110. The three rooms use
35/35, 26/26 and 28/22 tiles. These are source metrics, not compiled bank usage.

The player sheet retains80 source8×16 patterns. The fleet now uses20 source
8×16 patterns: all sixteen original service poses and their pixel/metadata
hashes remain exact, followed by four original taxi poses. The gold sedan has
a short roof light and checker trim, reuses bus palette6 and still occupies
two8×16 objects. Its source empty frame moves16→20 in all seven fleet loaders;
the source asset identity and every loader's remaining fields stay intact.
Official native-metadata dry-run validation and PNG analysis accepted the
extended asset with zero violations. This is source validation; the compiled
fleet allocation target is at most24 raw tiles per OBJ bank, subject to the
final ROM gate. Approved courier
and beacon pixel hashes are enforced by `create_player_sprites.py --check`;
the civilian PNG remains byte-identical. Hardware sprite allocation remains
a separate compiled-build gate, especially for the expanded controllable boat.

`python3 scripts/test_shops.py` verifies source/native room pixels, collision
grids, palettes, registration and keepers, then sanitizes the actual production
C module. Its75,750 checks cover all three rooms, failed scene allocations,
full-body shelf/wall collisions, held-button entry, keeper text, fresh-B exit,
walk-out, elapsed clock callbacks and unchanged outdoor state. This establishes
host logic only. Exact ROM build, native rendering/play, deadline continuity,
save restoration and physical cartridge checks are maintained separately by
the final integration workflow.

`python3 scripts/check_stop_names.py` rejects changes to any stop18 native
coordinate/service/flag field, unknown labels, a reverted current label,
unreviewed notices or an unrelated ID. Current campaign and all district job
generators retain the original native contract prefixes and immutable stop
hashes. Updated art metadata provenance is regenerated separately from native
quest fields and coordinates.


## Bright world and breakable furniture

The official palette editor updated eight existing shared world palettes while
preserving their IDs and authored defaults. Park foliage is bright green,
water is turquoise, and brick, coral, gold civic, glass and warehouse families
remain visually distinct. The UI palette is managed by the separate native
HUD/menu module. Approved courier, beacon and civilian source pixels stay exact.

`td_scenery.c` registers only final visible reachable background furniture:
signs, benches, bins, flower pots, bollards, crates, bus poles, fences and
street light poles. It does not change a single raw collision or roof-priority
byte. At speed magnitude3 or more, a whole-body vehicle contact breaks the
item, recoils through the driving module, flashes and scatters fragments for
24 visible frames, then leaves rubble on its exact original ground pixels.
Slow contact blocks the vehicle. Previously damaged items can be driven through.
Pedestrian routes, all64 stop approaches, three shop doors,34 reciprocal seam
endpoints, nine mission parking anchors and shared curb bays remain accessible
without destruction. Buildings, solid tree bases, the shoreline and bridges
retain structural collision; buildings do not collapse in this build.

Destruction is transient within one play session. Its 142-byte packed bitset
stores all 1,135 props by their absolute source index and persists through
district/shop travel; a cold reset or save reload rebuilds intact furniture.
Packing district boundaries into shared bytes saves 82 bytes versus the earlier
224-byte allocation of seven padded district rows. Four short animations and
18 owned background cells bring the native static state to 233 bytes, enforced
by a compile-time assertion. Background bank1
patterns80..97 are disjoint from aircraft64..78, lights47/48 and UI192..252.
No new actor, OBJ or save byte is added. Exact160×144 geometry bounds all
visible damaged cells to18; every cell restores its prior tile, palette, bank
and flips before scrolling/repainting. A changed scene or atlas ownership wins.
Whole half7 vehicle queries search at most4×4 nearby candidate cells per1px
substep using sorted binary row bounds, rather than scanning a district pool.
The existing flash-ring cursor packs a session-damage flag into its high bit;
its low two bits still select the four animation entries. Before the first real
collision breaks furniture, the renderer skips district lookups, geometry and
VRAM work. Every break sets the flag, district/shop/map travel preserves it,
and cold reset clears it. Restoration stays independent of the flag. This
shortcut adds no state bytes and keeps the full damaged-scene renderer.

The larger functional traffic signals use two separate8px heads per junction:
EW green for seven of each twelve game seconds and NS green for the other five.
Original stop coordinates and autonomous traffic rules remain unchanged.
Coral palette3 shows red; park palette6 shows green, independently of UI colors.
All164 second head cells are adjacent reachable non-priority ground, avoiding
other heads and registered furniture. The two original patterns remain47/48.

`test_scenery.py` sanitizes actual production C: the packed-storage tests cover
all 1,135 individual destruction bits against every other prop, district/scene
resets and single-bit debug-painted rendering. Existing checks cover continuous
Q4 body contact, all furniture/directions, diagonal misses, forward/reverse
thresholds, persistence, exact rubble pixels, animation phases,18-cell capacity,
scroll repaint, scene/shop isolation and modal restoration. Its independent
source access gate also checks every service and parking point. Native traffic
helpers passed9,944,824 checks; the revised actual signal renderer passed789,777.
The scenery suite passed1,772,932 checks after the clean-view shortcut: every
one of the1,135 props independently receives a real API impact, enables its
exact rendering and survives district/shop/map resets. It also checks two
flash-ring wraps, independent restoration, and zero district/map queries in
pristine sessions. Deliberate debug painting explicitly sets the damage flag;
an unmarked raw bit cannot silently bypass the fast path.
These host checks establish logic and source bounds; final ROM bank allocation,
CPU timing, native visible play and hardware are separately verified by integration.

The immutable traffic protection fixture is retained. The explicit
`city_feedback_protected.json` companion pins only accepted visual artifacts,
stop18's Bloorcourt display correction and native actor/notice capacity. It
restores historical visual fields before checking the old geometry/client/job
hashes; every mission reward, deadline, route, coordinate, district, stop ID,
terrain byte, priority mask, civilian pixel and unchanged sprite pose remains
protected. Negative cases reject altered footprints, raw terrain, scenery,
provenance, money and corrupted assets. Maintainer capture requires an explicit
flag and authenticates the exact historical Git source; it never runs in CI.
