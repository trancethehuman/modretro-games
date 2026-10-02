# Ambient aircraft

The user requested random planes and helicopters over the city on 2026-10-02. These original ambient flights add movement above the streets while preserving the accepted north-up camera, ground physics and building occlusion. They represent fictional scenery, without real aviation schedules, flight corridors or operator branding.

## Flight behaviour

One flight crosses the loaded district at a time. Its type, cardinal direction and lateral offset vary. It starts beyond the clamped camera viewport and then follows a fixed world path; scrolling the camera does not attach it to the courier. Planes move 24 signed-Q4 units per VBlank, helicopters 12: nominally 90 and 45 pixels per second on the game's 60-frame clock. Plane travel lasts 224 VBlanks; helicopters last 448. Rotor phase changes every eight flight VBlanks.

The first flight waits 360–487 active-world VBlanks, about six to eight seconds. After a flight ends, the next waits 1,200–2,223 VBlanks, about 20–37 seconds. The small seeded generator varies the next type, direction and path. These are occasional flybys rather than continuous traffic. State uses signed positions so aircraft can safely enter and leave viewport edges.

Pause, maps, menus and pending scene loads freeze flight position and countdown. A district load starts a fresh cosmetic schedule. Unloaded districts do not run aircraft actors. Flights have no collision footprint, fare, mission effect or audio in this milestone; flight state is separate from the unchanged 58-byte version-7 save.

## Original source art

The hand-placed sheet is 416×32 pixels: 13 source cells, with a fourteenth empty frame added in native metadata. Source and editable native metadata are in [original-art](../project/original-art/), with the generator at [create_aircraft_sprite.py](../scripts/create_aircraft_sprite.py). [The art manifest](../project/original-art/ambient_aircraft_art.json) records source hashes, colours, crop bounds and OAM reconstruction.

| Frames | Artwork | Objects per frame |
| --- | --- | --- |
| 0–3 | Plane east, south, west, north | 4 |
| 4–7 | Helicopter east, south, west, north, rotor phase 0 | 4 |
| 8–11 | Same helicopter directions, rotor phase 1 | 4 |
| 12 | Stippled 16×8 shadow | 2 |
| 13 | Empty resource-loader startup | 0 |

Horizontal aircraft occupy a 32×16 source body; vertical aircraft occupy 16×32 within the shared 32×32 canvas. Four 8×16 objects reconstruct each aircraft pose. Canvas-safe metadata uses the existing `(8,-8)` compiler origin. Source palette/OAM checks do not prove imported compiler frames or runtime rendering; final compiled tables need their own check.

The stable native asset ID is `48d4a3f8-a6bc-56c9-8956-6e847f0b30f1`, symbol `sprite_ambient_aircraft`. The PNG SHA-256 is `b8bcf4358415f5d9ba43146571740a8a23d955d536e18d86f2384c1c4b4508fe`. The source metadata SHA-256 is `a624b9f49d9b5efa25e8928692edea79d0a7ffa173effbab21e46594c6de4822`; an importer may remap native sub-IDs without changing the pixel source. This is original MIT-licensed art, with no copied photograph, logo or external sprite.

## Rendering and resource ownership

Each of the four gameplay scenes includes an empty aircraft loader so the compiler links and loads its sprite resource. Core, west and east use stock actor slot 2 after their Queen loader; High Park uses slot 1. The renderer caches the compiled sprite and base tile, then removes the loader before Toronto's ordinary actor clones. The existing 16-slot actor pool remains the ground simulation pool; aircraft are drawn directly into OAM.

The renderer decodes the genuine compiled ROM metasprites and primary/CGB tilesets while a new flight is still offscreen. It caches relative object coordinates, tile references, flip flags and opaque masks instead of reading banked metadata and OBJ VRAM on every visible frame. Two 80-byte aircraft templates hold the helicopter rotor phases, an 8-byte template holds the shadow, and two cache tags bring this cache to 170 persistent bytes. A scene reset or bind invalidates the cache; a different aircraft kind/direction refreshes it. This allocation is additional to the existing patch records and flight state.

Aircraft must appear above priority roofs while transparent gaps preserve the roof and its ground-sprite occlusion. The renderer uses the cached compiled pixels and flip flags to build an opaque mask. Only priority background tiles with nonzero covered pixels receive temporary colour-zero holes. The original palette/priority remains; aircraft precede ground objects in OAM and cover those holes. Ground cars and walkers keep the same roof/canopy priority through transparent aircraft gaps.

For each affected background tile, screen-space anchors are computed once and reused for its four aircraft-object mask comparisons. This keeps per-object work to relative offsets and bit masks.

Gameplay reserves CGB background bank-1 tiles 32–46 for at most 15 scratch patches. Original tile/attribute references are restored before the next stock render only when the map cell still holds that renderer's patch, so new scrolling data is preserved. Restore/discard patches when leaving gameplay before atlas/UI reuse of VRAM. The optional two-object shadow follows at `(8,24)` pixels, appended behind ground objects; background roof priority hides it without hiding it on ordinary roads.

An interval-event sweep verifies at most 40 total OAM objects and 10 per scanline, counting X-hidden ground objects as the hardware does. Offscreen aircraft objects are hidden before this sweep and do not enlarge its bounded row range. The common path checks all six aircraft/shadow objects once; only window-limited or crowded frames try the four-object aircraft-only fallback. Cosmetic output is suppressed if that fallback cannot preserve ground sprites. Current, destination and hardware UI-window positions are also protected so flights cannot cover HUD text during window movement.

The project-local `src/core/actor.c` override adds restoration at stock `actors_render()` entry and aircraft rendering at exit. It derives from pinned GBVM commit `bd6f41cc5e05cbe6601dcc7f8e2db89bed527fe3`, with the upstream MIT notice retained. [DISTRIBUTION.md](DISTRIBUTION.md) records source provenance and binary notices.

## Acceptance

Earlier aircraft candidate `8e7af3ec08349c4dbef473bae30ac940ca727c0524b04b9711c04817b8678258` passes the official build and compiled frame/resource gates. Its linked heap ends at `DA66`, below stack base `DF00`, leaving 1,178 bytes of static reserve; this does not measure deepest runtime stack use. Full source checks include 40,631,157 actual-renderer host checks. Host checks and compiled budgets remain separate from native visuals and frame pacing.

Scoped native recordings on this exact ROM pass sampled planes, helicopter rotor phases, shadows, world-fixed paths during camera movement, map freezing and priority-roof/scroll/pause restoration, plus first delivery, driving recovery and a delivery checkpoint restored through the game's reset buttons. Reviewed functional samples cover three paid Queen journeys, paid-map freezing and all four actual loaded scenes, including a High Park helicopter and shadow. That journey journal was automatically stopped by a mistaken tool clean-boot without an assessed `PASSED` label; it is retained separately from the genuine button-reset recording. Broader acceptance remains tracked in [ROADMAP.md](../ROADMAP.md). The build identity, linked allocation and exact evidence belong in [BUILD.md](BUILD.md) and [TESTING.md](../TESTING.md); do not transfer evidence from an earlier streetcar ROM to this feature.

Current contact-corrected candidate `14005662…` separately passes compiled aircraft/Queen gates and a 1,176-byte linked reserve. Its scoped native recordings show a plane/shadow, both visible helicopter rotor poses with shadow, exact paid-map state freezing, three Queen journeys with genuine paid-ride reset, walker save-load recovery and delivery/driving. Cosmetic helicopter output yields on a crowded frame and resumes when it can preserve the ten-object scanline limit. See the current identities in [BUILD.md](BUILD.md) and immutable journals in [TESTING.md](../TESTING.md).

Remaining native gates include every aircraft direction/district and crowded OAM/occlusion/frame pacing, plus forced blocked-contact/held-arrival/alighting cases. Ground-through-transparent-gap preservation has host coverage; the sampled native roof restoration does not establish all crowded overlaps. Handheld palette/readability, physical cartridge execution and full-city/two-hour acceptance remain separate gates. Published Prototype 6 does not contain these flybys.
