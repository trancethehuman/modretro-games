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

## Moving Queen streetcar — reviewable a0e23 candidate, broader acceptance pending

- [x] Draw original Queen rails and an editable eight-pose streetcar sheet; four 8×16 OAM objects per pose, 28×12 horizontal / 12×28 vertical body.
- [x] Implement one banked cyclic unit on the fictional 64-second schedule, two-second door dwells, both offset lanes, original East bend and reciprocal scene seams.
- [x] Validate actual-C full-body geometry and sweeps against registered grids: 3,771,783 checks pass at the pre-presentation-correction checkpoint.
- [x] Derive paid camera/view scene while preserving the 58 saved origin bytes, booked trip and parked car until safe alighting; freeze pending scene-load catch-up and pause/map motion.
- [x] Add rail-parking refusal, cold valid older-car recovery, swept stationary-car/foot separation, carrying-only impact damage and future-sweep traffic yielding in source.
- [x] Write version 7 using the same 58-byte state and two CRC16 records in SRAM bank 3; validate paid blocked-arrival hold, read valid versions 6/5/4 and test interrupted writes/fallback. Older version-6 ROM rollback cannot be assumed after both slots are rewritten.
- [x] Guard WAIT boarding at the actual origin and cancel a displaced wait without fare; bound boarding cue arithmetic. Full `make check` passed with 3,450 engine checks before the presentation corrections.
- [x] Retain native `deb78bbd…` failure at frame 3,444 and its stopped/closed/archived journal; diagnose empty horizontal compiler metasprite frames, without an actor-size ABI claim.
- [x] Preserve original PNG/native IDs with canvas-safe sprite metadata and correct runtime actor bounds to GBVM Q5; engine fixtures pass 3,454 checks, including every cycle pose.
- [x] Build corrected candidate `b1cf9a37…` and pass full source checks with 3,454 engine checks before performance edits; gate compiled cardinal/door frames separately from playback.
- [x] Sample ordinary-button horizontal approach/open doors, terminal vertical frames, Yonge 46→Parkdale 43→Alton 50 rides through West/Core/East, two single fares and East alighting; preserve the stopped/archived `needs-review` journal.
- [x] Sample paid-map freezing of clock/fare/ride, East-objective browsing and visible-tram restoration on that exact candidate.
- [x] Restore one-pass scalar traffic while retaining direct phase/additive range math; final full `make check` passes 110,986 engine and 3,771,783 motion checks with all other suites green.
- [x] Record same-ROM `a0e23f03…` three paid rides, exact 58-byte paid-map freeze, paid soft reset and ordinary Union car re-entry; scoped Core pacing is 57 updates / 120 VBlanks (~28.5/s), above b1cf 45 and failed 84e5's 35.
- [x] Repeat fresh same-ROM contract 01 delivery for cash 139/done 1/condition 100, held speeds 10→22→16 with pedestrian brake at 864, and blocked-terrain reversal/resumed 18. Preserve both closed/archived scoped-pass recordings.
- [x] Check matching compiled frames, 1,432-byte linked reserve and 66 OBJ tiles/bank; sample four-object tram OAM without over-limit scanlines.
- [ ] Broaden whole-city/crowded performance and actual hardware/human handling/palette assessment; this candidate does not establish universal 28.5/s or the full game's duration.
- [ ] Resolve fully crowded `BLOCKED` contact-recovery policy; broaden native rail parking, traffic yielding, held-arrival/reset and legacy transit conditions beyond the scoped ride checks.
- [ ] Broaden atlas cancellation and sprite/VRAM ownership coverage; test crowded scanline/frame pacing and collect human boarding/route-choice feedback.

Published Prototype 6 remains the separate timetable/safe-alighting release. Corrected candidate samples do not establish an accepted final moving-tram milestone; exact evidence is in [TESTING.md](TESTING.md).

## Ambient aircraft — scoped native samples, broader acceptance pending

- [x] Record the user-requested random plane/helicopter flybys and keep them cosmetic, with original unbranded art.
- [x] Draw the editable 416×32 sheet: four plane directions, eight two-phase helicopter poses and two-object shadow, with an empty loader frame in metadata. Check source pixels, palette, OAM coverage and compiler-safe canvas metadata.
- [x] Implement signed-Q4 flight state, varied type/direction/lateral paths, occasional waits and rotor phase selection separately from the cartridge save.
- [x] Integrate four-scene loaders/direct rendering; actual-source fixtures cover menu/scene-load suppression and unchanged save/collision rules, with native pause/map state freezing sampled.
- [x] Check compiled-mask roof holes, ground-priority preservation, conditional restoration, UI/VRAM ownership and bounded OAM with 40,631,157 renderer host checks. Native samples verify roof/scroll/pause restoration; crowded ground-through-gap coverage remains below.
- [x] Officially build exact `8e7af3ec…`; pass compiled aircraft/Queen frame/header/tile gates, full source checks and 1,178-byte linked reserve. Retain both earlier aircraft intermediates separately.
- [x] Preserve scoped passed plane/helicopter/rotor/shadow/map/roof and first-delivery/driving recordings on this exact ROM, including solid-contact reverse recovery and bounded frame-pacing observations.
- [x] Review three paid Queen journeys through Core/West/East, paid-map state freezing and walking into an actually loaded High Park scene with parked car retained. Preserve the auto-stopped journey journal without relabelling its unassessed outcome.
- [x] Record a fresh delivery and genuine game-button SRAM reset restoring its saved checkpoint, then a new cosmetic flight. This is emulated checkpoint recovery, not physical persistence or paid-ride reset.
- [ ] Repeat exact-ROM paid-ride reset and remaining legacy transit/seam/arrival conditions; resolve the three contact/held-arrival/alighting branches in the next-polish list.
- [ ] Broaden every-direction/district, crowded native ground-through-gap occlusion, scanline pressure, deepest stack and whole-city frame-pacing coverage.
- [ ] Assess crowded-scene and handheld readability/performance. Native flyby samples do not prove full-city, two-hour or physical-cartridge acceptance.

The published Prototype 6 remains unchanged. [AIRCRAFT.md](docs/AIRCRAFT.md) records the source design; [TESTING.md](TESTING.md) owns any later exact native evidence.

## Next playable polish

- [ ] Fix confirmed crowded-contact/held-arrival branches: an overlapping yielding NPC can trap a walker, held destination doors must share occupancy with traffic queries, and alighting fallback must reject the booked tram body. Temporary actual-C probes establish branch defects; native reachability and the final policy/replays remain pending.
- [x] Apply condition/comfort damage only after pickup; recover valid older damaged approach saves and preserve occupied penalties/deadlines. Actual-C regressions and a matched native defect/replay, condition-scaled delivery and controlled held turn pass on candidate `64be19fa…`. Published Prototype 6 remains the earlier ROM; [TESTING.md](TESTING.md) scopes evidence.
- [ ] Resolve the plugin's unresolved browser recording-close acknowledgement, then show and test the current city preview.
- [ ] Tune steering, traffic spacing and frame pacing from handheld/human feedback. Historical samples and current a0e23 scoped 28.5/s have separate identities; crowded-scene/whole-city performance remains pending.
- [ ] Listen to the original score/effects and tune their mix, event distinction and physical speaker/headphone behaviour.
- [ ] Expand pedestrian variety, street furniture, local activity and visible transit boarding/riding.
- [ ] Finish the moving-Queen native acceptance gates above; improve the current partial bus route geometry and research later streetcar corridors.
- [ ] More distinctive Toronto landmark silhouettes, district character, industrial yards and additional navigable streets.

## Full game release

- [ ] Extend the working four-scene stage across the full historical Old Toronto footprint, waterfront and fuller Islands. The larger [17-district layout](docs/OLD_TORONTO_EXPANSION.md) remains a proposal; its proposed 2026 era still needs adoption.
- [ ] Extend researched western/eastern TTC coverage beyond the representative Queen game service, including King504 and Line2; preserve original fictional timing and distinguish the adopted map baseline from live detours.
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
