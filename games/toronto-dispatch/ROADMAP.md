# Roadmap

## Initial central prototype — implemented

- [x] Public MIT collection repository, one folder per game, README, AGENTS.md and workflow skills.
- [x] Genuine editable GB Studio project and original project-local scene engine.
- [x] User-revised north-up presentation, 1,024 × 976 scrollable central-city prototype, 48-pixel asphalt with 8-pixel sidewalks, 80 unchanged building footprints and six architecture types.
- [x] Four road vehicles; momentum, traction, braking/reverse, speed-dependent yaw, glancing curb recovery and bounded 1–6 pixel corner correction with swept clearance.
- [x] Native building/water/rail collision, roof/canopy priority, walking animation and visible car entry/exit.
- [x] Separate continuous traffic motion/presentation and a bus proxy; 102 fixed world pedestrian routes with six active nearby actors.
- [x] Native dispatch, collection/delivery, completion records, progression, cargo damage, vehicle constraints and timeout/retry in the playable prototype.
- [x] Fictional repeating schedules and paid travel on researched Line 1, 94 bus and Island ferry service points.
- [x] Pause/map freeze the clock; transit resumes correctly after pausing.
- [x] Native collision connectivity and contract/source consistency in repository CI.
- [x] Native emulator delivery/transit/map/entry/collision/save checks. See exact build scopes in TESTING.md.
- [x] Original native music, vehicle/braking ambience, event cues and music + effects / effects only / silent modes; actual PCM output and mode checks.

Native driving regression evidence covers the first job, the previously stopping Distillery turn at speed 24, and parking/re-entry. Host collision/assistance regressions and native audio mode captures provide separate checks; [TESTING.md](TESTING.md) records exact build scopes. Audio capture verifies output, not human listening or physical speaker quality.

## Original campaign revision — content checks passed

- [x] 72 distinct original titles and routes across nine progressive chapters, with two short brief lines generated into native content.
- [x] 27 collision-verified walkable service points, including three fictional Island delivery entrances beyond the ferry docks.
- [x] Distinct transit relays, Island walking legs, mainland ferry transfers, signed-return routes and freight/passenger vehicle constraints.
- [x] Collision-path-based time allowances, text bounds, deterministic native content generation and unlock closure through distinct completions.
- [x] Build the revised ROM; native smoke verifies first-job condition reward, eligible dispatch, walking/entry and paid transit. Host checks cover passenger steering comfort. All eight job types still need full native playtests.

Campaign duration remains unverified. Contract counts, time limits and shortest-path estimates do not establish the two-hour release target.

## First western expansion — published Prototype 3, broader verification pending

- [x] Three genuine linked native scenes: central Toronto, Parkdale/Roncesvalles and High Park/Swansea/Junction; each 1,024 × 976 pixels in a logical 3,072 × 976 atlas.
- [x] 166 total building footprints, original western landmark styles, readable 48-pixel asphalt/8-pixel sidewalks, rail barriers and separate waterfront foot access.
- [x] Source attribution, original-compression notices and researched street endings/bends in [WEST_DISTRICT.md](docs/WEST_DISTRICT.md); proposed 2026 era remains unadopted.
- [x] Eleven reciprocal seam pairs, local player/parked-car district identities, shared job/clock/progression and version-6 save state.
- [x] 358 fixed pedestrian routes with six nearby actors in the loaded district; banked selection and a bounded coordinate cache. Six traffic loops in each western scene.
- [x] Eight appended progression-gated western package jobs and eight clients, producing 80 contracts/35 stops while preserving the original 72/27 prefix. Colborne service requires a short park-and-walk handoff.
- [x] Registered PNG/attribute/collision agreement, actual source tile budgets, accepted seam lanes, western client connectivity, swept traffic routes, native headers/text and old-content preservation in `make check`.
- [x] Build the expanded ROM, verify boot and drive from the core into the west scene. See exact evidence in [TESTING.md](TESTING.md).
- [x] Publish Prototype 3 after sampled native western travel, parked-car recovery, district-qualified active-job reset and core paid subway checks. This does not verify every western job or crossing.
- [ ] Verify all reciprocal crossings and foot-only waterfront access in the expanded ROM, then test parked-car recovery, job handoffs, failures and save/reset across districts.
- [ ] Play all eight western jobs with ordinary controls and refine driving routes, foot leg, deadlines, rewards and progression from observed play.
- [ ] Audit crowded western scenes, native occlusion and expanded-ROM performance.

## First eastern expansion — Prototype 4, broader verification pending

- [x] Four registered local scenes, each 1,024 × 976 pixels, in a logical 4,096 × 976 atlas; 211 building footprints and original eastern landmarks/architecture.
- [x] Three researched core/east seam pairs through Bloor/Danforth, Dundas and Queen, for 14 reciprocal pairs total. Conditional Gerrard stays closed; no invented King/Front/Lake Shore bridge or duplicate Broadview/Don.
- [x] 486 fixed pedestrian routes with six nearby actors and 18 traffic loops across the three non-core scenes. Source/host checks cover registered collision, palette/priority, tile budgets, traffic and accepted seam lanes.
- [x] Eight eastern jobs and eight fictional clients bring authored content to 88 contracts/43 stops, preserving the previous 80/35 prefix. Withrow/Greenwood deliveries require short park-and-walk legs; Pape preserves its researched pedestrian rail crossing.
- [x] Banked world data and travel-mode graph routing, next-district beacon/HUD, marker refresh on car entry/exit and explicit missing-road-route feedback. The 37-byte traffic/next-district cache retains eight VM contexts and version-6 save format.
- [x] Banked parking-anchor cues for Colborne, Withrow and Greenwood; driving targets the legal approach and exiting restores the client marker. Native Withrow handoff and car re-entry pass; other park handoffs still need their own samples.
- [x] Shared two-second transit departure window, countdown in the choice screen and immediate boarding on confirmation during an open window; no additional TTC service.
- [x] Build candidate `1da71ba5…`; load all four actual native scenes, complete first delivery, retain speed 24 through the held-turn regression, sample all three core/east approaches, drive core-to-west and walk west-to-High Park.
- [x] Complete nine distinct contracts with ordinary buttons: 01, 02, 03, 04, 07, 81, 82, 83 and 85. This samples car/truck/motorcycle and a Withrow park-walk relay; about eight minutes of purposeful game-clock progression does not meet the two-hour target.
- [x] Restore nine completions, cash/clock, foot player in High Park and car parked in the west after native reset; pan the loaded local map while the player/clock remain frozen.
- [x] Full host/generated-resource checks and actual-ROM memory guard pass with 1,484-byte reserve. Host engine, bridge, navigation and memory-fixture results remain separate from native samples.
- [x] Final-ROM immediate subway boarding, one fare deduction and arrival on foot with parked car retained. A bounded 120-frame sample advances 59 rendered updates (~29.5/s); OAM inspection sees no over-limit scanlines.
- [ ] Verify remaining reciprocal seam lanes, Colborne/Greenwood handoffs and final-candidate paid-ride reset recovery. Exact sample scope belongs in [TESTING.md](TESTING.md).
- [ ] Play all eight eastern jobs and remaining western routes with ordinary controls; refine deadlines, condition rewards, walking legs and unlock progression from observed play.
- [ ] Verify crowded eastern art/actors, occlusion, deepest stack use and frame pacing.
- [x] Prepare the Prototype 4 ROM, loading instructions, distribution notices and exact sampled verification record. Release identifier: `v0.2.0-prototype.4`; GitHub review/CI and publication are tracked with that release.

## Browsable city map — Prototype 5

- [x] Generate a512×122 native collision-ground schematic of all four registered areas in geographic order, with road/walking/water/padding distinctions.
- [x] Scroll a160×96 viewport across areas; mark courier, car and active job/booked transit stop/free-roam depot. Preserve road-anchor versus true-foot-client targets.
- [x] A objective focus, useful Select focus cycle, B/Start cancellation during partial painting and opposed-key neutrality.
- [x] Banked API/data, bounded172-slot double hash, shared360-byte UI cache and28 transient runtime bytes; fixed/VRAM/actual-ROM memory allocations checked.
- [x] Actual-source sanitizer checks across all225 viewports, all marker overlaps, remote parked-car/client semantics, sparse collision chains, every vacant-slot position and full-cache exhaustion.
- [x] Build optimized `e812f7ef…` and final portable `2d1f6e4e…`; repeat first-delivery/held-turn regression, render allfour atlas area headers and preserve paused position/clock/cash/deadline.
- [x] Native foot/car focus, car re-entry, partial redraw cancellation and camera/text/sprite restoration; map freezes WAIT/RIDE and resumes fare/arrival correctly.
- [x] Reset from a mapped paid ride restores the trip and reaches King without another fare. This is in-worker SRAM evidence, not a physical cold boot.
- [ ] Native coverage of every viewport, remote parked-car and allparking/client marker transitions; host fixtures cover these cases.
- [ ] Full Old Toronto expansion, measured two-hour varied gameplay and hardware acceptance remain part of the full-release scope below.

## Queen scheduled streetcar — Prototype 6 milestone, sampled native acceptance passed

- [x] Research representative 501 Queen stop identities and the normal corridor; distinguish current construction diversions and preserve the unadopted era in [STREETCAR.md](docs/STREETCAR.md).
- [x] Append eight sidewalk platforms 43–50 across west/core/east, for 51 service points; preserve the original 43 records, all 88 contracts and 58-byte version-6 save format.
- [x] Add original seven-pixel curb signs without a TTC logo; check native sidewalk clearance, non-overlapping interaction areas, source references and walking connectivity.
- [x] Integrate banked route/fare/timetable queries, destination-derived east/west labels, three-dollar fare, 64-second period, two-second windows and four-second rides per stop interval.
- [x] Validate production transit C against an independent oracle: 609,452 sanitizer checks, including both Queen directions, invalid encodings, preserved subway/bus/ferry semantics and clock bounds.
- [x] Sample the earlier `f56ff75e…` native candidate with three paid Queen journeys across core/east/west, paid pause/map/reset and parked-car recovery; retain its exact evidence in [TESTING.md](TESTING.md).
- [x] Correct the legacy Union arrival onto the parked car with bounded 12/18-pixel connected alighting candidates, loaded-traffic clearance and paid retry when blocked. Current engine host fixtures pass 2,857 checks; the predecessor defect remains recorded.
- [x] Build and replay exact final ROM `23b2a7a2…`: first delivery, held turn, three Queen journeys across core/east/west, paid arrival/reset, map pause and car recovery; separately verify legacy Line 1 / 94 bus/Island ferry and safe Union alighting.
- [x] Complete final `make check`, with 2,857 engine and 609,452 transit checks; official final build/native evidence is recorded in [TESTING.md](TESTING.md), with build/allocation identity in [BUILD.md](docs/BUILD.md).
- [x] Publish [Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) after real CI jobs pass; anonymously verify the exact ZIP, native ROM, member checksums, source identity and notices. [TESTING.md](TESTING.md) records its identities.
- [ ] Validate human route-choice value, remaining platforms/arrival conditions and future visible streetcar boarding/riding. Full501, King 504, fuller TTC coverage and current detours are separate work.

## Realistic downtown — implemented 2026-10-07, human review pending

- Centreline-derived core street plan with a street hierarchy, neighbourhood blocks and landmarks in place.
- Larger cars, a sticky and wider pistol lock, and transit prompts, fare checks and a more frequent 501.
- Next: station markers on the city map, laneways in the largest blocks, larger diagonal car frames if sprite tiles allow, and applying the same method to the West and East scenes.

## Story overhaul — implemented 2026-10-07, human review pending

- [x] Original premise and nine-chapter arc (Margo's depot, Rushly, the Lakelight contract) with 27 speaker cards; clients speak on delivery.
- [x] Briefing, pickup and delivery calls for all 88 contracts; routes and stop lists in story order and shortest driving order.
- [x] Chapters open after the previous chapter's spine contract; plot beats follow their contracts; `check_story.py` proves every fact is heard before it is mentioned.
- [ ] Human read-through on the Chromatic: dialogue length, typing speed and the pacing of chapter calls.
- Next: a story recap in the pause menu and individual portraits if UI tiles can be freed.

## Distinct districts and navigation names — implemented 2026-10-07, human review pending

- Done: neighbourhood, landmark and junction names on the HUD in all four scenes; downtown district building kits (Financial District, MaRS and Hospital Row, the museums and U of T, the Entertainment District, Chinatown, Kensington, St James, CityPlace) and neighbourhood colours; outer-scene looks on unchanged footprints; sidewalk curbs, joints and furniture; the Queens Quay boardwalk; flower beds.
- Next: area names under the city map's cursor, junction signs in the art, park benches and lamps, University College and King's College Circle if a wider U of T block becomes possible, and a denser outer-scene grid with re-verified routes.
- Pending: human review on the Chromatic (district legibility, pop-up frequency, sidewalk detail at 1x).

## Parks and drivable ground — implemented 2026-10-07, human review pending

- [x] Flat ground drivable downtown (lawns, lots, plazas, yards), as in the other scenes.
- [x] Queen's Park from the Centreline: the crescent, the Legislative Building, the King Edward VII statue, paths and trees; the 94 bus round the north end.
- [x] Trinity Bellwoods, Allan Gardens and Grange Park fill their blocks with their landmarks; Sorauren, High Park, Withrow and Greenwood parks gain their real amenities.
- [ ] Handheld review: do the parks read as themselves, and is cutting across open ground too easy.
- Next: Toronto Islands park shapes, Riverdale Park, waterfront parks south of Queens Quay; these need tile budget (downtown is at 382 of 384).

## Gameplay, scenery and performance — implemented 2026-10-07, human review pending

- [x] Spray bays in all four scenes, 20 lost parcels with a $500 bonus and a count on the pause menu, and walkers who run from gunfire.
- [x] Animated ripple water with foam and sand shores in all four scenes; the Yonge-Dundas rooftop screen.
- [x] Story: staged mid-campaign and festival-week chatter, delivery reactions, the police call's spray-bay hint, and Ernie's lost-parcel thread.
- [x] Fixed UI glyphs on the core scene's water and roofs (UI art moved clear of scene tiles).
- [x] Faster frames: walkers laid out on alternate updates; animated tiles copied inside vertical blank by an interrupt handler.
- [ ] Handheld review: animation on the Chromatic's screen, parcel visibility, spray bay price, the three-second status line in a vehicle, and VRAM timing on hardware.
- Next: ferry-lane ripples and more rooftop signs.

## Overhaul — implemented 2026-10-10, human review pending

- [x] Double-scale world: sixteen 1,024 × 976 scenes (four districts at 2,000 × 1,904), 16 px sidewalks, bigger blocks, parks and buildings; open inner seams, generated next-hop routing, in-lane traffic loops with signals in every scene; content, map (1:16), routes, places and pickups remapped; save v8.
- [x] Heavier, slower driving with tail slides and clipping contacts; A+B gets out; full-size 45-degree cars; running stride.
- [x] Projectile combat with misses on both sides; police cruiser, SUV, unmarked car and helicopter.
- [x] Swimming and sharks; weather (rain, cloud and shadow, sun rays) with gulls; a minimal radio strip, title and menus; four songs.
- [x] Greenery: a second green palette, tree species by place, hedges, pickets, park railings, flower beds, bushes and lawn patches, within every scene's tile budget and at an unchanged frame rate.
- [ ] Handheld review: driving feel and contract deadlines at the new scale, seam crossings, overlay flicker, weather readability, helicopter and shark difficulty, and the greens on the Chromatic screen.
- [ ] More walkers on screen at once (eight slots are tied to the actor pool and bitmask code).
- [ ] Draw more vehicle designs and local streets in the outer districts (their plans carry only main roads, so some blocks are very large).

## Next playable polish

- [x] Apply condition/comfort damage only after pickup; recover valid older damaged approach saves and preserve occupied penalties/deadlines. Actual-C regressions and a matched native defect/replay, condition-scaled delivery and controlled held turn pass on candidate `64be19fa…`. Published Prototype 6 remains the earlier ROM; [TESTING.md](TESTING.md) scopes evidence.
- [ ] Resolve the plugin's unresolved browser recording-close acknowledgement, then show and test the current city preview.
- [ ] Tune steering, traffic spacing and frame pacing from handheld/human feedback. A bounded sample observed about 29.5 rendered updates per second; crowded-scene performance remains pending.
- [ ] Listen to the original score/effects and tune their mix, event distinction and physical speaker/headphone behaviour.
- [x] Street life revision (candidate, 2026-10-06): gentler police pursuit and fewer officers, sidewalk pickups replacing the curbside props and gulls, eight walkers over 620 routes, traffic that keeps returning near the courier, redrawn cars and a dusk title screen; CLI/emulator evidence in [TESTING.md](TESTING.md).
- [x] Day/night cycle and animation (candidate, 2026-10-06): a 17-minute day with golden hour, dusk, night and dawn palettes, headlamps, tyre smoke, exhaust and impact effects, a look-ahead driving camera, punch/pistol poses, collection pops and a flashing patrol light bar, at unchanged frame rate; CLI/emulator evidence in [TESTING.md](TESTING.md).
- [ ] Handheld review of the day length, night readability, smoke frequency and camera look-ahead.
- [x] City life, menus, story and shooting (candidate, 2026-10-06): Rosa's radio calls with a portrait card (welcome, briefings, chapters, police, night and chatter), bottom-sheet menus with four actions at a time, a distance readout, bigger cars, eight road-vehicle designs in seven paints and six walker designs in four colours, D-pad aim with lock-on and long tracer rounds, and car damage with smoke and repairs; CLI/emulator evidence in [TESTING.md](TESTING.md).
- [ ] Handheld review of radio pacing, menu legibility, lock-on reach, tracer visibility and damage rates.
- [ ] Decide whether more road vehicles on screen are worth their frame-rate cost (measured at 3-4 updates per second on busy routes).
- [ ] Playtest police difficulty, pickup economy and street density on the handheld; decide whether a few clearly readable knock-over props should return.
- [ ] Expand street furniture, local activity and visible transit boarding/riding (pedestrian and vehicle variety added in the city-life revision).
- [ ] Author streetcar rails and moving streetcars on researched corridors; improve the current partial bus route geometry.
- [ ] More distinctive Toronto landmark silhouettes, district character, industrial yards and additional navigable streets.

## Full game release

- [ ] Extend the working sixteen-scene stage across the full historical Old Toronto footprint, waterfront and fuller Islands. The larger [17-district layout](docs/OLD_TORONTO_EXPANSION.md) remains a proposal; its proposed 2026 era still needs adoption.
- [ ] Extend researched western/eastern TTC coverage beyond the representative Queen game service, including King504 and Line2; preserve original fictional timing and distinguish the adopted map baseline from live detours.
- [ ] Playtest and refine the authored objectives across all eight job types, including the unlock sequence, vehicle rules and condition/comfort rewards.
- [ ] Record representative timings and a complete campaign; validate **at least two hours of varied, enjoyable gameplay**. Quest count does not verify duration.
- [ ] Test crowded-scene sprite/CPU limits, diagonal walking, collision corner recovery, cancellation and schedule edge cases.
- [x] Host tests interrupt every SRAM write in both save slots; native soft reset resumes a paid trip with its remaining time and saved fare.
- [ ] Test physical SRAM cold boot and interrupted power; host/native reset tests do not establish cartridge persistence.
- [x] Audit pinned native runtime/distribution notices; include them with every ROM bundle.

## Device milestone

- [x] Identify the connected Chromatic and supported writable cartridge (Player 1, writable 4 MiB cartridge; see TESTING.md).
- [x] Load the user's identified writable development cartridge through the official workflow (main builds, 2026-10-10).
- [ ] Verify write/read-back where available, manual cold boot, driving/braking, a full delivery, transit, save recovery and audio.

Cartridge writes are recorded in [TESTING.md](TESTING.md). Record device observations separately from emulator results without private activation or device details.
