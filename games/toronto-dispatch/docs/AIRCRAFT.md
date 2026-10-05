# Aircraft and police helicopter

The user requested random planes and helicopters over the city on 2026-10-02. These original ambient flights add movement above the streets while preserving the accepted north-up camera, ground physics and building occlusion. They represent fictional scenery, without real aviation schedules, flight corridors or operator branding.

## Ambient flights

One flight crosses the loaded district at a time. Its type, cardinal direction and lateral offset vary. It starts beyond the clamped camera viewport and then follows a fixed world path; scrolling the camera does not attach it to the courier. Planes move 24 signed-Q4 units per VBlank, helicopters 12: nominally 90 and 45 pixels per second on the game's 60-frame clock. Plane travel lasts 224 VBlanks; helicopters last 448. Rotor phase changes every eight flight VBlanks.

The final sandbox source adds occasional larger jet ground shadows. They use the same plane speed, lifetime and offscreen arrival/departure rules. Half of the former ambient plane selections become jets; helicopter frequency and all original helicopter/commuter pacing remain unchanged. A visible jet keeps its identity when police attention rises and completes its path before a police helicopter can enter.

The first flight waits 360–487 active-world VBlanks, about six to eight seconds. After a flight ends, the next waits 1,200–2,223 VBlanks, about 20–37 seconds. The small seeded generator varies the next type, direction and path. These are occasional flybys rather than continuous traffic. State uses signed positions so aircraft can safely enter and leave viewport edges.

Pause, maps, menus and pending scene loads freeze flight position and countdown. A district load resets local flights. Unloaded districts do not run aircraft actors. Flights have no physical collision footprint or fare. Flight and pursuit state remain outside the 58-byte version 10 save.

## Police pursuit and evasion

The 2026-10-04 sandbox update gives an existing helicopter an actual police pursuit. The caller supplies the courier's current Q4 world coordinates separately from camera or TTC focus. A requested helicopter starts completely beyond the clamped viewport; a visible ambient helicopter may acquire that target at its existing location. A visible plane keeps its identity and continuous path until it exits.

Police helicopter movement accelerates by at most one Q4 unit per active VBlank on each axis, to component limits 12/14/16 at wanted levels 1/2/3. It aims 24px beside and 32px above the last seen courier position, leaving the player readable below it. Pursuit consumes at most 16 steps per update; a delayed update cannot place a new helicopter halfway across the screen. Existing cardinal artwork and rotor phases supply its direction without another actor or sprite sheet.

A helicopter observes an exposed courier within an 80px square of its actual position. Exposure comes from the full authored background attribute map in ROM, rather than the camera's scrolling VRAM. Priority roof/canopy cells, native shop interiors, TTC travel and the occupied launch beneath an authored Don bridge conceal the courier. Concealed or out-of-range movement never updates the last seen target. After 300 pursuit steps without a sighting, or when wanted attention resolves, the helicopter chooses an exit direction and leaves continuously. It retires only after the whole craft and shadow are offscreen. An 1800-VBlank aerial cooldown permits escape before a new offscreen search.

`td_police_observed()` combines the actual nearby uncaptured road patrol with aerial sight. Ground sight retains its open 96px boundary; a captured police-car identity cannot observe from an obsolete route position. Actual observation keeps `wanted_left` at 30. Thirty unobserved active-world seconds reduce one of the existing three wanted levels; pause and menus freeze this countdown. Countdown alone changes no fare, cargo or job state. Existing physical road-police capture and fines remain separate. The HUD uses the same timer for its visible losing-sight stars.

The pursuit module adds at most 15 transient bytes. `td_aircraft_police_target()` supplies attention, coordinates and exposure; `td_aircraft_police_observed()` supplies actual air sight. Boat concealment uses `td_boats_under_cover()`. No pursuit fields are serialized.

## Boat controls

On foot, press A beside a launch at a Core or Port Lands dock to board. A
accelerates aboard, left/right steer, and B brakes or reverses. Stop within
reach of a safe dock, hold Down and freshly press A to disembark. The docking
press is consumed so it cannot also re-enter a vehicle or accelerate. A nearby
walking boarder pauses the autonomous launch only within its inclusive 56px
boarding range; larger distances keep its continuous water route moving.
Original road vehicles and courier progress remain intact while aboard.

## Original source art

The hand-placed sheet is 576×32 pixels: the retained 13 source cells, a transparent loader cell and four new jet shadows. Source and editable native metadata are in [original-art](../project/original-art/), with the generator at [create_aircraft_sprite.py](../scripts/create_aircraft_sprite.py). [The art manifest](../project/original-art/ambient_aircraft_art.json) records source hashes, colours, crop bounds and OAM reconstruction. The original 416×32 crop retains decoded RGB SHA-256 `bc20efa4a2602f32d958b2b2294f12924fee27fee31aab1a6221da23ace3c589`; all first 14 imported frame/object identities and the empty startup index remain intact.

| Frames | Artwork | Objects per frame |
| --- | --- | --- |
| 0–3 | Plane east, south, west, north | 4 |
| 4–7 | Helicopter east, south, west, north, rotor phase 0 | 4 |
| 8–11 | Same helicopter directions, rotor phase 1 | 4 |
| 12 | Stippled 16×8 shadow | 2 |
| 13 | Empty resource-loader startup | 0 |
| 14–17 | Large stippled jet ground shadow east, south, west, north | 4 |

Horizontal aircraft occupy a 32×16 source body; vertical aircraft occupy 16×32 within the shared 32×32 canvas. Four 8×16 objects reconstruct each aircraft pose. Canvas-safe metadata uses the existing `(8,-8)` compiler origin. Source palette/OAM checks do not prove imported compiler frames or runtime rendering; final compiled tables need their own check.

The jets use a sparse 32×32 nose, tailplane and swept-wing silhouette. Staggering the two wing halves across opposite 16×16 quadrants permits exact cardinal rotations with only four 8×16 objects. They retain transparent gaps and use only the original darkest shade. Source flipping dedup identifies six additional 8×16 patterns in total; the compiled guard independently caps aircraft at 34 raw tiles per OBJ bank, no more than eight above its retained 26-tile allowance, and still checks every scene's combined 128-tile budget.

The stable native asset ID is `48d4a3f8-a6bc-56c9-8956-6e847f0b30f1`, symbol `sprite_ambient_aircraft`. Current PNG SHA-256 is `1d538e0d60a6dd7d4feeb3f44248f12cfd537fb081ea74ec6b40e448736b8bc6`; source metadata SHA-256 is `ec4aa5c29fde16e574273a730bf124fccfb8333a70d67b3b0f4ac1dd8d5f6c2e`. The retained pre-jet PNG and metadata identities were `b8bcf4358415f5d9ba43146571740a8a23d955d536e18d86f2384c1c4b4508fe` and `a624b9f49d9b5efa25e8928692edea79d0a7ffa173effbab21e46594c6de4822`. This is original MIT-licensed art, with no copied photograph, logo or external sprite.

## Rendering and resource ownership

Each of the seven outdoor district scenes includes an empty aircraft loader so the compiler links and loads its sprite resource. Core, West and East use stock actor slot 2 after their Queen loader; the other districts use slot 1. The renderer caches the compiled sprite and base tile, then removes the loader before Toronto's ordinary actor clones. The current native actor pool has 22 slots; aircraft are drawn directly into OAM and allocate no additional actor.

The renderer decodes the genuine compiled ROM metasprites and primary/CGB tilesets while a new flight is still offscreen. It caches relative object coordinates, tile references, flip flags and opaque masks instead of reading banked metadata and OBJ VRAM on every visible frame. Two 80-byte aircraft templates hold the helicopter rotor phases, an 8-byte template holds the shadow, and two cache tags bring this cache to 170 persistent bytes. A scene reset or bind invalidates the cache; a different aircraft kind/direction refreshes it. This allocation is additional to the existing patch records and flight state.

Aircraft must appear above priority roofs while transparent gaps preserve the roof and its ground-sprite occlusion. The renderer uses the cached compiled pixels and flip flags to build an opaque mask. Only priority background tiles with nonzero covered pixels receive temporary colour-zero holes. The original palette/priority remains; aircraft precede ground objects in OAM and cover those holes. Ground cars and walkers keep the same roof/canopy priority through transparent aircraft gaps.

For each affected background tile, screen-space anchors are computed once and reused for its four aircraft-object mask comparisons. This keeps per-object work to relative offsets and bit masks.

Gameplay reserves CGB background bank 1 tiles 64–78 for at most 15 scratch patches. Compiled backgrounds must end before 64; signals use 47/48 and destructible scenery uses 80–97, preserving distinct scratch ownership. Original tile/attribute references are restored before the next stock render only when the map cell still holds that renderer's patch, so new scrolling data is preserved. Restore/discard patches when leaving gameplay before atlas/UI reuse of VRAM. The optional two-object shadow follows at `(8,24)` pixels, appended behind ground objects; background roof priority hides it without hiding it on ordinary roads.

An interval-event sweep verifies at most 40 total OAM objects and 10 per scanline, counting X-hidden ground objects as the hardware does. Offscreen aircraft objects are hidden before this sweep and do not enlarge its bounded row range. The common path checks all six aircraft/shadow objects once; only window-limited or crowded frames try the four-object aircraft-only fallback. Cosmetic output is suppressed if that fallback cannot preserve ground sprites. Current, destination and hardware UI-window positions are also protected so flights cannot cover HUD text during window movement.

Jets reuse one existing cached four-object pose and append their whole shadow behind street actors. They allocate no additional actor or persistent byte, never fetch or patch background pixels, and remain hidden by ordinary roof/canopy priority. Their entire four-object submission passes the same window, 40-object and ten-per-scanline limits atomically.

The project-local `src/core/actor.c` override adds restoration at stock `actors_render()` entry and aircraft rendering at exit. It derives from pinned GBVM commit `bd6f41cc5e05cbe6601dcc7f8e2db89bed527fe3`, with the upstream MIT notice retained. [DISTRIBUTION.md](DISTRIBUTION.md) records source provenance and binary notices.

## Verification and historical acceptance

Current source tests separately exercise 6,054,445 actual flight/pursuit checks and 46,944,777 actual renderer checks, including every jet/source pixel under all flips and eight-pixel alignments, full-world cover, signed edges, window movement, exact OAM limits and unchanged ground ordering. Original ambient-flight and pixel/OAM oracles remain. The compiled gate adds independent source-PNG aircraft decoding and ten exact pixel/allocation rejection fixtures alongside the retained scene/actor/boat checks. These sanitizer fixtures adapt platform/register boundaries; they do not prove the current candidate's native frame pacing, cartridge visuals or linked memory reserve. The official final build, compiled jet/boat allocation, all-district graphics gate and hardware remain separate current gates tracked in [SANDBOX_FEEDBACK_2026_10_04.md](SANDBOX_FEEDBACK_2026_10_04.md).

The following records describe earlier candidates and remain historical:

Earlier aircraft candidate `8e7af3ec08349c4dbef473bae30ac940ca727c0524b04b9711c04817b8678258` passes the official build and compiled frame/resource gates. Its linked heap ends at `DA66`, below stack base `DF00`, leaving 1,178 bytes of static reserve; this does not measure deepest runtime stack use. Full source checks include 40,631,157 actual-renderer host checks. Host checks and compiled budgets remain separate from native visuals and frame pacing.

Scoped native recordings on this exact ROM pass sampled planes, helicopter rotor phases, shadows, world-fixed paths during camera movement, map freezing and priority-roof/scroll/pause restoration, plus first delivery, driving recovery and a delivery checkpoint restored through the game's reset buttons. Reviewed functional samples cover three paid Queen journeys, paid-map freezing and all four actual loaded scenes, including a High Park helicopter and shadow. That journey journal was automatically stopped by a mistaken tool clean-boot without an assessed `PASSED` label; it is retained separately from the genuine button-reset recording. Broader acceptance remains tracked in [ROADMAP.md](../ROADMAP.md). The build identity, linked allocation and exact evidence belong in [BUILD.md](BUILD.md) and [TESTING.md](../TESTING.md); do not transfer evidence from an earlier streetcar ROM to this feature.

Earlier contact-corrected candidate `14005662…` separately passes compiled aircraft/Queen gates and a 1,176-byte linked reserve. Its scoped native recordings show a plane/shadow, both visible helicopter rotor poses with shadow, exact paid-map state freezing, three Queen journeys with genuine paid-ride reset, walker save-load recovery and delivery/driving. Cosmetic helicopter output yields on a crowded frame and resumes when it can preserve the ten-object scanline limit. See the current identities in [BUILD.md](BUILD.md) and immutable journals in [TESTING.md](../TESTING.md).

Remaining native gates include every aircraft direction/district and crowded OAM/occlusion/frame pacing, plus forced blocked-contact/held-arrival/alighting cases. Ground-through-transparent-gap preservation has host coverage; the sampled native roof restoration does not establish all crowded overlaps. Handheld palette/readability, physical cartridge execution and full-city/two-hour acceptance remain separate gates. Published Prototype 6 does not contain these flybys.
