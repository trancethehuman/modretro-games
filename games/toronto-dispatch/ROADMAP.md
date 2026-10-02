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
- [x] Build `e812f7ef…`; repeat first-delivery/held-turn regression, render allfour atlas area headers and preserve paused position/clock/cash/deadline.
- [x] Native foot/car focus, car re-entry, partial redraw cancellation and camera/text/sprite restoration; map freezes WAIT/RIDE and resumes fare/arrival correctly.
- [x] Reset from a mapped paid ride restores the trip and reaches King without another fare. This is in-worker SRAM evidence, not a physical cold boot.
- [ ] Native coverage of every viewport, remote parked-car and allparking/client marker transitions; host fixtures cover these cases.
- [ ] Full Old Toronto expansion, measured two-hour varied gameplay and hardware acceptance remain part of the full-release scope below.

## Next playable polish

- [ ] Resolve the plugin's unresolved browser recording-close acknowledgement, then show and test the current city preview.
- [ ] Tune steering, traffic spacing and frame pacing from handheld/human feedback. A bounded sample observed about 29.5 rendered updates per second; crowded-scene performance remains pending.
- [ ] Listen to the original score/effects and tune their mix, event distinction and physical speaker/headphone behaviour.
- [ ] Expand pedestrian variety, street furniture, local activity and visible transit boarding/riding.
- [ ] Author streetcar rails and moving streetcars on researched corridors; improve the current partial bus route geometry.
- [ ] More distinctive Toronto landmark silhouettes, district character, industrial yards and additional navigable streets.

## Full game release

- [ ] Extend the working four-scene stage across the full historical Old Toronto footprint, waterfront and fuller Islands. The larger [17-district layout](docs/OLD_TORONTO_EXPANSION.md) remains a proposal; its proposed 2026 era still needs adoption.
- [ ] Implement boarding, autonomous schedules and strategic alighting for researched western/eastern TTC corridors; added district clients supply no new scheduled native service.
- [ ] Playtest and refine the authored objectives across all eight job types, including the unlock sequence, vehicle rules and condition/comfort rewards.
- [ ] Record representative timings and a complete campaign; validate **at least two hours of varied, enjoyable gameplay**. Quest count does not verify duration.
- [ ] Test crowded-scene sprite/CPU limits, diagonal walking, collision corner recovery, cancellation and schedule edge cases.
- [x] Host tests interrupt every SRAM write in both save slots; native soft reset resumes a paid trip with its remaining time and saved fare.
- [ ] Test physical SRAM cold boot and interrupted power; host/native reset tests do not establish cartridge persistence.
- [x] Audit pinned native runtime/distribution notices; include them with every ROM bundle.

## Device milestone

- [ ] Identify the connected Chromatic and supported writable cartridge; establish Developer Mode readiness.
- [ ] Stream if supported, then load the user's identified writable development cartridge through the official workflow.
- [ ] Verify write/read-back where available, manual cold boot, driving/braking, a full delivery, transit, save recovery and audio.

No physical streaming or cartridge write has been attempted. Record device observations separately from emulator results without private activation or device details.
