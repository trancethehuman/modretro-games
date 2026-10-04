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
- [ ] Natively verify the fully crowded `BLOCKED` contact policy; broaden rail parking, traffic yielding, forced held arrivals and legacy transit conditions beyond the scoped ride checks.
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
- [x] Correct GCC misleading-indentation warnings with a whitespace-only renderer/harness edit and `-Werror` retained; full local checks and official portability rebuild `20370fea…` pass. Preserve its distinct source fingerprint and fresh scoped delivery/driving/plane/helicopter/shadow/map replay separately from `8e7af3ec…` journals.
- [x] Verify real hosted GNU `make check` steps for source-fix commit `3a1869a` in both PR/push runs.
- [ ] Broaden exact-portability-ROM roof/transit/all-scene/reset samples; five checksum/save-signature bytes differ, and earlier native recordings keep their original identity.
- [x] Repeat paid-ride reset/map freezing and three Queen journeys on corrected `14005662…`, plus actual plane/helicopter poses and delivery/driving. Retain this ROM's three distinct closed scoped-pass journals.
- [ ] Broaden remaining legacy transit/seam/arrival conditions and exact corrected-ROM roof restoration; earlier builds retain their own evidence.
- [ ] Broaden every-direction/district, crowded native ground-through-gap occlusion, scanline pressure, deepest stack and whole-city frame-pacing coverage.
- [ ] Assess crowded-scene and handheld readability/performance. Native flyby samples do not prove full-city, two-hour or physical-cartridge acceptance.

The published Prototype 6 remains unchanged. [AIRCRAFT.md](docs/AIRCRAFT.md) records the source design; [TESTING.md](TESTING.md) owns any later exact native evidence.

## Port Lands fifth-scene milestone — scoped native acceptance passed

- [x] Research official Port Lands streets, separate river/channel bridges, parks and industry; record source dates/licences, actual former-Toronto boundary checks and original compressed generation in [PORT_LANDS_PLAN.md](docs/PORT_LANDS_PLAN.md). Representative contemporary corridors are a working default; no 2026 era adoption or copied reference art.
- [x] Generate original 1,024 × 976 art/collision and register genuine district 4 / scene `f9f8ee05-b372-5338-91a7-f1f7a0cda44e`. Port Lands has 26 buildings, 117 raw / 98 flip-canonical patterns, 79 full-footprint-safe pedestrian routes and six swept-clear traffic loops, with water blocked outside six supported decks.
- [x] Extend East Leslie and register its sole reciprocal Port Lands seam for foot/car travel, yielding 15 pairs. Preserve existing IDs/local coordinates; withhold Core Cherry and Carlaw gateways. East now has 43 buildings; the five-scene source totals 235 buildings, 565 routes and 24 non-core traffic loops.
- [x] Generate the five-scene 512 × 244 atlas: 589 patterns, 13,507 bytes in bounded banked units and no added persistent WRAM. Host atlas checks pass with at most 164 visible patterns under the 172-slot limit.
- [x] Complete adapted five-entry runtime fixtures/full host suite and official exact ROM `a212dd9e…`; header, compiled aircraft allocations/Queen frames and 1,176-byte reserve pass, including bank-1 background ≤32 before aircraft scratch tiles 32–46.
- [x] Record ordinary paid Queen 46→49, Leslie foot entry into actual Port Lands and return to East, Cherry Beach/Unwin walking and blocked shore. A genuine reset restores the saved Beach checkpoint and parked Core car. Keep earlier four-scene recordings attached to their original ROMs.
- [x] Sample second-row map pans with all 58 game and 13 flight bytes frozen; separately sample plane/helicopter/rotor/shadow OAM at peak ten per scanline without overflow.
- [x] Repeat fresh final-ROM contract 01 completion, held-turn/continued acceleration, reverse, steering recovery and coast-to-stop. Preserve its separate scoped journal.
- [x] Record retained `a243…` ordinary-control Leslie car entry/return, all six car deck crossings, Commissioners walking, genuine Port walker/car reset/re-entry and Beach walking/re-entry to the real water edge. Preserve its separate [portable sample](docs/NATIVE_PORT_DRIVING_SAMPLE.json).
- [ ] Verify truck/motorcycle/scooter bridge cases, roof/crane/boat occlusion, wider water/scene combinations and crowded traffic/aircraft pacing; scoped car travel does not complete them.
- [ ] Broaden second-row marker/focus/cancellation/VRAM combinations and native coverage beyond sampled pans; host fixtures cover all 900 viewports.
- [x] Append eight original Port clients/jobs for 96 contracts/59 stops and seven parking anchors. Pin all previous 88/51 native records/briefs; retain save v8/58 bytes, 16 completion bytes and 16 actors. Validate every client/parking route through the sole Leslie seam, meaningful truck/multi-stop/closed-return routes and provisional deadlines.
- [x] Validate real-engine ordered/wrong-district/foot 58/parking/truck-transit/final95 credit/save fixtures and old 88-job v6/v7/v8 records; the Port-campaign engine gate passes 699,955 checks. Keep the shorter VBlank clock conversion equivalent without added persistent state.
- [x] Build exact 96/59 `c625…`; full suite/header/compiled/memory gates pass with 1,098 static bytes. Stationary 110/360 versus retained 108 is a tiny scoped sample, not whole-city pace.
- [x] Complete original jobs 0,2,1 and new Fire Hall Books/index 89 through ordinary Riverside pickup/Leslie/Port parking/legal foot handoff. Delivery condition 36 / cash 71 / done 4 and real reset/restored bit 89/Port car re-entry pass in a closed 11,024-frame [record](docs/NATIVE_PORT_CAMPAIGN_SAMPLES.json).
- [ ] Play the other seven new jobs and all 96 native progression, test saved active new-job recovery, and tune deadlines/condition/rewards/handling. Exact higher fine amounts are not isolated by the sampled H3/cash-clamped capture. Broader pacing remains pending; retained `a243…` six-car-bridge scope stays separate.
- [ ] Research/register/play a later 114 connection independently; no new TTC service was added.

## Living street simulation — scoped native prototype, broader acceptance pending

- [x] Record visible civilians, vehicle/human impacts, police/fire/ambulance/trucks/buses, boats, bridges and NPC traffic lights as accepted direction. Record the user's selected chaotic sandbox tone, escalating police pursuits and tougher penalties. Preserve courier focus, original branding and Old Toronto scope. First prototype aftermath is non-graphic; combat, weapons, lethal rules and stealing NPC vehicles are not adopted.
- [x] Integrate distinct original civilian walk/stumble art and swept visible-person contact. Newly hit humans recover for six active-world seconds; a contact update halves player velocity, adds hit count to H capped at three and fines $20 × hit count × resulting H. Each person deducts ten carried-condition points only after pickup. A stunned human cannot generate another charge during recovery; pause/menu time freezes it.
- [x] Integrate HUD attention 0–3, one-level cooling per 30 active seconds away from the police's 96-pixel AABB, and capture within a 32-pixel AABB with stop/fine $25 × H² and cleared attention. Preserve the 58-byte save payload with v8 reusing obsolete cursor words; valid older v4–v7 imports clear attention. These source rules supersede the earlier proximity patrol's lower fine/range.
- [x] Integrate one bounded police chase through cardinal collision-backed roads, eleven transient cached-plan/origin/stuck bytes, attention-dependent speed capped at eight pixels per sweep, replan on arrival/64 blocked updates and driven return to authored patrol without teleporting. Keep normal signal/junction/tram/person/vehicle guards. All paid RIDE states, including legacy subway/bus/ferry, suppress pursuit; capture requires ROAM. Global shortest paths, cross-scene continuity and backup units are not implemented.
- [x] Integrate sixteen-active-VBlank ordinary traffic and four-VBlank chasing-police motion, retaining eight-pixel sweeps and per-render validated eight-Q4 overlap escape. Nominal ordinary speed is 30 pixels/second, but capped delayed updates discard excess time; verify responsiveness rather than claiming guaranteed speed. Courier/contact/person/attention checks remain each active city update. The resettable one-byte light cache writes only changed VRAM cells. Retained fused-milestone engine/planner/traffic/light gates pass 695,857 / 11,133 / 2,424,828 / 12,842 with its full suite; that Port/clock checkpoint had 699,955 engine checks; current `d58…` has 800,394.
- [x] Integrate the transient 87-byte bucket snapshot, validating all six bodies/extents, visible people and the parked car once per batch. Safe distant-bucket rejection retains exact nearby geometry; terrain/tram-gated commits update accepted endpoints sequentially. Preserve legacy retreat, fail-closed admission and differential host coverage without new persistent RAM.
- [x] Record separate scoped native street evidence: 123-byte hull `555f3d31…` impact/stumble, map freeze, attention reset, police capture and boat; cadence `bd09f1c3…` compiled limits/1,098-byte reserve, 107 loops/360 VBlanks and condition-100 first delivery (cash 139/done one). Keep hull 91, eight-VBlank bucket 92 and retained baseline 132 as separate measurements; these are not whole-city performance acceptance.
- [x] Fuse admission, unchanged full terrain/future-tram guards and commit into one BANKED operation; retain public old-position/cache equality and all candidate bounds/cardinality/limits. Remove only private-proven old-body revalidation. Split unchanged standalone signal-stop code/constants into a ROM-only translation unit to resolve the traffic-bank limit. Final full suite and official `a2438348…` compiled/header/pose/resource/1,098-byte reserve gates pass.
- [x] Record final-ROM H1 impact/stumble, exact game/flight/boat map freeze, genuine saved-attention reset, road police approach/capture, park/exit/walking/blocked Core shore and a visible boat, plus a separate fresh condition-100 first delivery (cash 139/done one). Correct paired counter records 108 loops/360 VBlanks; keep invalid counter reads and earlier ROMs separate.
- [ ] Verify higher-heat/escape/all-paid-ride suppression/return, wider pause/save/map/water/bridge/roof/crowd cases, deepest stack and human responsiveness. Final timing remains below the 132-loop baseline; no whole-city performance or hardware claim follows from sampled functionality.
- [x] Integrate six distinct road vehicle kinds and fictional 12-second signals, red-line stops, body/queue clearance and occupied-junction admission. Emergency yielding priority preserves red/blocked-junction checks. No real TTC timing, synchronized visible paid bus, emergency incident dispatch or new service job is implied.
- [x] Integrate one bounded cosmetic boat on validated Core/Port Lands water routes, with pixel clipping beneath the existing Lake Shore/Commissioners decks, a distinct native scratch pair, menu freeze and no SRAM fields. Source/host checks cover capacity/UI rejection and loader order; compiled isolation/allocation passes on `a2438348…`. Boat driving, boarding and dock dwells are unimplemented; road/foot crossings use only existing collision-backed decks.
- [ ] Broaden exact-ROM ordinary-control records for civilian recovery/consequences, broader quiet decay/escape, moving-car H2 capture, red/green drivers, emergency yield, remaining paid-mode suppression/save migration and water/bridge occlusion. Later `d58…` funded H1/H2 on-foot and H3 moving-car captures plus one sampled Core patrol rejoin are recorded below; historical courier/flyby samples keep their own identities.
- [x] Add eight source Port courier jobs with reachable loading/parking/foot approaches for 96 contracts/59 stops; native execution/tuning is tracked above.
- [ ] Add later useful service incidents/jobs after acceptance, match visible bus movement to tested paid stops before claiming that connection and research any additional/opening bridge.
- [ ] Verify crowded CPU/scanline/stack limits, human readability/handling/fun and physical cartridge boot/save/audio separately. No new street-simulation feature is certified by the historical courier/flyby samples or quest count.

## Dispatch planning and reward feedback — full checks and scoped native passes

- [x] Add full itinerary browsing: Left/Right retains contract selection; Up/Down visits ordered/repeated stops with place, district, role and walking requirement. Preserve A/B, eligibility, paused world and existing mission/save IDs.
- [x] Show base, condition percentage/adjusted pay, time bonus and exact credit at the balance cap; reuse existing offer storage plus one transient itinerary byte. Preserve failure, cancellation, replay, return and reset semantics.
- [x] Separately build/play tram-only `a638c374…`, before UI changes. Its exact ROM-only progress tables retain motion/sweeps/schedules and 1,098 static bytes; paired 18-second Core counts 381/1,080 versus `c625…` 348/1,080, about 9.5% scoped. First delivery/held turn, paid Queen fare/map freeze/button reset/East arrival pass.
- [x] Build combined `4343f2b8…` with compiled limits/1,097 static bytes; host UI passes 24,185,014 checks/all 796 pages. Its own stationary sample repeats 381/1,080. Ordinary controls cover wrap/return previews/Beach WALK cues/locked denial, acceptance from a later preview still picking up at Union, exact condition/base/time/credit and zero-pay replay timeout.
- [x] Separately record combined-ROM Line 1 Union→King fare 30→27, all 58 paid-map bytes frozen, genuine paid reset/arrival and the parked Union car. Keep earlier tram-only Queen evidence on `a638c374…`.
- [x] Pass full combined `make check`, including 704,884 engine and 24,185,014 actual-UI checks.
- [ ] Broaden native damaged/capped payouts, cancellation/other routes and human/handheld readability/deep-stack checks. Scoped results do not complete all 96 contracts, full city, two hours or hardware.
- [ ] Compare ordinary truck/fragile/passenger/park-walk/transit route choices, then tune handling/deadlines/rewards from normal-speed human play. Retain the full-city and measured two-hour gates below.

## Next playable polish

- [x] Correct route-consistent overlapping traffic retreat, explicit booked-tram HOLD occupancy and full-foot/body alighting endpoints. Preserve a real ordinary-button save-reset walker deadlock on `20370fea…`; corrected `14005662…` permits walking and all four directions without changing cash, condition or the parked car. Actual-C/UI regressions pass 476,667 / 8,036,664 checks; a pure banked route query resolves the native 16-KiB code limit. Save v7 remains 58 bytes.
- [ ] Natively reproduce rare rail retreat, persistent crowded BLOCKED contact and paid blocked-arrival/HOLD retries through ordinary controls. Host regressions cover their source behavior; independent NPC/terrain cooldowns, native CPU cost and hardware remain separate gates.
- [ ] Before changing transit landing geometry, strengthen or recheck fallback paths across the full foot footprint. Current actual-grid audit finds no mismatch at all 21 transit nodes/eight cardinal candidates/19 samples; centre-only path connectivity is a future-map limitation.
- [x] Apply condition/comfort damage only after pickup; recover valid older damaged approach saves and preserve occupied penalties/deadlines. Actual-C regressions and a matched native defect/replay, condition-scaled delivery and controlled held turn pass on candidate `64be19fa…`. Published Prototype 6 remains the earlier ROM; [TESTING.md](TESTING.md) scopes evidence.
- [ ] Resolve the plugin's unresolved browser recording-close acknowledgement, then show and test the current city preview.
- [ ] Tune steering, traffic spacing and frame pacing from handheld/human feedback. Historical samples and current a0e23 scoped 28.5/s have separate identities; crowded-scene/whole-city performance remains pending.
- [ ] Listen to the original score/effects and tune their mix, event distinction and physical speaker/headphone behaviour.
- [ ] Expand pedestrian variety, street furniture, local activity and visible transit boarding/riding.
- [ ] Finish the moving-Queen native acceptance gates above; improve the current partial bus route geometry and research later streetcar corridors.
- [ ] More distinctive Toronto landmark silhouettes, district character, industrial yards and additional navigable streets.

## Full game release

- [x] Stage fuller Islands research using visually inspected official City maps and established paths/bridges, preserving proposed-versus-existing and map-era distinctions. Freeze the old six stop/nine quest source identities and exact354-tile/60-byte walking mask before migration.
- [x] Register Islands as district 5 without mainland seams: six source scenes, 241 buildings, 595 pedestrian routes, 24 added mainland traffic loops and 15 seam pairs. Remove old Core strips while preserving mainland art/collision/attributes.
- [x] Relocate only six stable endpoint geometry triples; preserve 96 contracts/59 identities/seven parking anchors, every native job field and completion bit. Add typed ferry/full-body foot source checks and 510 independent preservation/negative regressions; retain all nine deadlines as provisional native tuning inputs.
- [x] Integrate version-9 migration with the same 58 bytes and immutable 60-byte legacy mask, v8 attention/paid-trip/car preservation, Island traffic suppression, WALK/ferry cues and shared idle insufficient-cash return assistance. Actual six-scene engine migration gate passes 787,948 checks.
- [x] Build corrected six-scene `7b2af59c…` (`toronto-dispatch-islands-safe.gbc`) and pass full checks, including 787,948 engine / 25,107,699 UI. Preserve the earlier `8a96…` map failure: the pause AUDIO mixed-varargs overflow corrupted cached focus; bounded label assembly now passes all three audio/cache samples.
- [x] Close a scoped ordinary-input replay covering all three docks/clients/bridges, inland/coastal routes, blocked shore/private yards/airport gap, tree-canopy occlusion and map YOU/CAR/DEPOT focus/panning with all 58 game bytes frozen.
- [x] Sample Island/paid/free v9 resets, all three scheduled roundtrips, seven $4 legs (cash 30→2), zero-fare assistance/cancellation/reset and mainland car recovery. Complete a condition-100 Core Market Start with credit 109/cash 111/done 1 and restore its completion. [Portable evidence](docs/NATIVE_ISLAND_DISTRICT_SAMPLES.json) retains the closed 36,438-frame scope.
- [x] Close a separate `7b2…` ordinary-input campaign with 13 unique completions across all eight kind IDs, including Centre Letters after its 12-job unlock. Its unchanged 195-second deadline finishes at condition 100 with 136 seconds left / credit 145; active-job map, client/paid-return resets and unpaid WAIT cancellation pass. An intentional missed-client replay expires during a paid return, restores after reset and yields zero credit/no extra completion/fare. [Portable campaign evidence](docs/NATIVE_ISLAND_CAMPAIGN_SAMPLES.json) retains the closed 65,845-frame scope.
- [x] Correct the native parked-actor flags store/invalid selector guard and loaded-district remote-car filtering. Retained `5ae4…` (`toronto-dispatch-queen-street-life.gbc`) passes official/compiled/full checks (797,467 engine / 2,400,936 pedestrian) and a separate 11,219-frame native first-delivery/Queen 47→43→47/pedestrian/paid-reset/car-recovery/audio/map pass. Both 240-frame map pairs freeze all 58 bytes; reset preserves the committed trip fields, not the latest fractional subsecond. [Portable evidence](docs/NATIVE_QUEEN_STREET_LIFE_SAMPLES.json) keeps this scope separate from `7b2…`'s 13-job campaign and the diagnostic failures.
- [ ] Natively import historical v8 Island checkpoints; play/tune the remaining eight Island missions, wider carrying/timeout/cancellation cases and deadlines. Sample remaining crowd/occlusion/whole-city performance and human route choice; the scoped 13-job campaign does not establish old-format migration, all 96 contracts, two measured enjoyable hours or hardware.
- [ ] Extend the six-scene source milestone across the full historical Old Toronto footprint, waterfront and fuller Islands, verifying each playable milestone. Port Lands broader acceptance remains above; the larger [17-district layout](docs/OLD_TORONTO_EXPANSION.md) and proposed 2026 era remain unadopted proposals.
- [ ] Extend researched western/eastern TTC coverage beyond the representative Queen game service, including King504 and Line2; preserve original fictional timing and distinguish the adopted map baseline from live detours.
- [ ] Playtest and refine the authored objectives across all eight job types, including the unlock sequence, vehicle rules and condition/comfort rewards.
- [ ] Record representative timings and a complete campaign; validate **at least two hours of varied, enjoyable gameplay**. Quest count does not verify duration.
- [ ] Test crowded-scene sprite/CPU limits, diagonal walking, collision corner recovery, cancellation and schedule edge cases.
- [x] Host tests interrupt every SRAM write in both save slots; native soft reset resumes a paid trip with its remaining time and saved fare.
- [ ] Test physical SRAM cold boot and interrupted power; host/native reset tests do not establish cartridge persistence.
- [x] Audit pinned native runtime/distribution notices; include them with every ROM bundle.

## Core bus lanes and precise pedestrian bodies — scoped native pass

- [x] Compare pedestrian clearance against exact Q4 fleet centres and larger bus/fire bodies; preserve fractional fleet centres in native actor presentation without new persistent RAM or save fields.
- [x] Retain the `cf2f…` native bus/police standstill as [needs-review failure evidence](docs/NATIVE_CORE_BUS_LANE_FAILURE.json); correct the fictional six-vertex Core bus target/recovery/direction tables and cold spawn.
- [x] Build `d58…` (`toronto-dispatch-bus-lanes.gbc`); official header/compiled/memory gates and full source suite pass, including 800,394 engine / 2,421,337 pedestrian checks and 1,097 static reserve bytes.
- [x] Close a separate ordinary-input native record: full-condition first delivery, paid Line 1 fare once, exact 240-frame map freeze, genuine paid reset/arrival, two bus passes past police, all six authenticated legs and northbound human-clearance/OAM sample. [Portable evidence](docs/NATIVE_CORE_BUS_LANES_SAMPLES.json) keeps older Queen/Island campaign results separate.
- [x] Regress the seeded nine-pixel human wait/resume case in the host engine; this specific case is not native acceptance.
- [x] Close an additional unchanged-`d58…` [courier/heat record](docs/NATIVE_COURIER_HEAT_SAMPLES.json): unique jobs 0/2/1, condition-92 scaled pay 115, First Art entirely on foot with pay 138, parked-car recovery and done 3 / cash 392. Verify visible H1/H2/H3 impact charges, exact game/person/fleet H3 map freeze, committed-attention save/reset, live road police approach and funded $225 H3 capture (cash 272→47 / heat 0 / speed 0), then post-capture reset. In that record the patrol waits behind the courier; it supplies no return pass.
- [x] Add an unchanged-`d58…` [lower-heat record](docs/NATIVE_LOWER_HEAT_PATROL_SAMPLES.json): two genuine full-condition jobs fund $25 H1 (243→218) and $100 H2 (158→58) Core on-foot captures. After clearing King’s lane, observe sampled driven return endpoints and actual target leg 2→0 without reset/district change; recover the original car and save a neutral checkpoint without restoring it.
- [x] Officially restore the genuine done-2 checkpoint and close a [separate same-ROM continuation](docs/NATIVE_SIX_JOB_CONTINUATION_SAMPLES.json): four new unique jobs 1/6/3/4 reach done 6 / bitmap `5F…` / cash 626. Verify fragile/signature/truck/relay credits 138/132/140/257, two separate carried impacts, and two trains/four buses costing $14 with a walked transfer. Preserve earlier independent scopes rather than summing campaigns.
- [x] Sample the after-braking stationary distant-police H1 countdown, funded moving-car H1 capture inside a held-A interval (post-sample speed 2), and positive-H1 paid Line 1 westward police return. Save the final released-input six-job checkpoint without restoring it.
- [ ] Verify H2 moving-car capture, active/higher-heat escapes, other patrol-return routes/districts and every paid-mode suppression branch; keep new-job SRAM reset, wider human-impact/crowd/performance and full campaign gates open.
- [ ] Broaden native pedestrian wait/resume, crowded lane/junction and performance coverage; full campaign, eight remaining Island jobs, native older-save imports, two measured enjoyable hours, human handling, browser recovery and hardware remain pending.

## Retained RESULT B release handling — a0bd scoped native pass

- [x] Consume RESULT-closing B until release without freezing the active world or altering ordinary fresh braking/reverse; add one transient byte, preserving v9 / 58-byte saves and all campaign fields.
- [x] Build current `a0bd…` (`toronto-dispatch-result-controls.gbc`), with full checks / 800,554 engine cases, independent 514-check compiled audit and 1,096-byte static reserve.
- [x] Close a fresh 55-interval native first-delivery/held-B stop/truck/fresh-reverse/genuine-reset record; distinguish later automatic saved pose from live SAVE A inspection and blank early boot from visible HELP. [Portable evidence](docs/NATIVE_RESULT_CONTROLS_SAMPLES.json) preserves the save-only final checkpoint.
- [x] Close a separate current-ROM continuation from a disclosed genuine done-1 import: new fragile/art and Distillery completions reach done 3; condition-70 pay 102, full-condition pay 124, cash-zero continuation, ordinary car recovery and funded H1 foot/H2 stationary-car fines pass.
- [x] Compare First Connection from the exact same done-3 neutral checkpoint in independent transit/walking and conservative cardinal-driving branches; each reaches done 4. Retain actual four fares/$10, Castle wait-versus-walk, credits 254/252, nine whole seconds of deadline difference and $3 cash difference as a single-phase observation. [Selected native evidence](docs/NATIVE_MATCHED_TRANSIT_SAMPLES.json) keeps both branches and imported completions explicit.
- [ ] Broaden this exact ROM's campaign/transit/heat/district/pacing evidence. Keep four older `d58…` records separate; all 96 jobs, remaining Islands, two measured enjoyable human hours, native older-save imports, browser and hardware remain pending.

## Pedestrian admission correction — retained 964f scope

- [x] Reproduce the Core route-19/bus first-appearance overlap in a host fixture and defer newly selected/hidden people near the occupied courier's current body or preceding bounded sweep. Use strict ten-pixel admission clearance and loaded-view applicability; preserve continuously visible eight-pixel impacts, six-second recovery and all penalties.
- [x] Pass strict host compilation and ASan/UBSan pedestrian checks (2,423,750) plus the actual-engine gate (800,555). Establish prior visibility in the retained-impact engine fixture; add no persistent/history/save bytes, routes, art or cadence changes.
- [x] Build `964f…` and pass 580 matching compiled checks, including flags destination/context and admission/impact radii. Retain save v9/58 bytes and 1,096-byte linked reserve; local frame 27→28 does not establish deepest-stack or performance behavior.
- [x] Close a fresh native delivery/held-B live-world/continuously visible route-83 impact/map-freeze/pan/saved-progress reset replay; retain its one OAM frame and save-only checkpoint scope. [Curated evidence](docs/NATIVE_PEDESTRIAN_ADMISSION_SAMPLES.json) discloses two unretained diagnostic responses.
- [x] Continue the same `964f…` from a disclosed genuine done-1 checkpoint: four new unique jobs 1/2/3/6 reach done 5 / bitmap `4F…` / cash 472, with fragile condition-80 pay 116, full-condition express/truck pays 124/156 and ordered condition-78 return pay 116. Retain natural route-83 reset admission, clamped H3 car capture and moving H1 cooldown as their sampled scopes, distinct from host route 19 and general escape. [Curated chain evidence](docs/NATIVE_COURIER_CHAIN_SAMPLES.json) pins the closed journal and save-only final checkpoint.
- [ ] Natively reproduce the specific first-appearance case and broaden crowd/vehicle/transit/heat/pacing checks. The host/compiled case does not diagnose prior Bay impacts, and older `a0bd…` matched-route/campaign scopes are not assigned to `964f…`. All 96 jobs, remaining Islands, two enjoyable human hours, browser and hardware remain pending.

## Courier clearance and quest discovery

- [x] Add readable captions and Select navigation through all twelve existing eight-offer groups, retaining per-offer locks/vehicles and individual offer/itinerary controls. Actual-engine tests cover all 96 starts and input chords; the separate `c4fa…` native record samples all twelve starts, wrap and lock/exit/held controls.
- [x] Preserve the [failed c4fa driveaway record](docs/NATIVE_IDLE_DRIVEAWAY_FAILURE.json). The same-route walker waits safely at rest but walks into the initial reverse after the speed-limited yield expires. Reproduce its exact route/clock/lag in actual people and whole-engine fixtures.
- [x] Build corrected `e797…` and pass full source checks / 1,242 compiled checks with all-speed proposed-step clearance, retained genuine impact/recovery and unchanged save layout/resources. Close a fresh chapter/one-delivery/visible-wait/reverse/clearance/resumed-walk/genuine-forward-impact/map record and a separate disclosed checkpoint-import/in-worker SRAM-reset record. [Curated evidence](docs/NATIVE_COURIER_CLEARANCE_SAMPLES.json) keeps the adapted trajectory and committed-versus-live save pose explicit.
- [x] Continue unchanged `e797…` from a disclosed genuine Market Start checkpoint: six new unique jobs 1/2/6/3/4/11 reach done 7 / bitmap `5F08…` / cash 760. Verify ordered returns, two required-truck freights with condition-scaled pays 139/226, a full-condition seven-trip train/bus relay costing $17, and funded H2/H1 police fines. Retain guard halts, the short arrival press/retry and first-fare WAIT→arrival receipt distinction. [Curated campaign evidence](docs/NATIVE_CAMPAIGN_SEVEN_SAMPLES.json) pins the closed journal and save-only final checkpoint; this is not seven fresh jobs or measured human duration.
- [x] Import the genuine seven-completion `e797…` checkpoint and add four unique jobs 14/8/9/5: motorcycle receipts, scooter round, condition-100 car art and eight-stage required-car passenger work reach done 11 / bitmap `7F4B…` / cash 844. Credits 83/104/224/238 and the charge ledger reconcile. [Curated evidence](docs/NATIVE_CAMPAIGN_ELEVEN_SAMPLES.json) retains whole-pixel handoff rejection, distinct contact/displacement receipts and ordinary controller recovery without a fleet identity or H3 capture claim. Prior truck work is inherited; these frames do not establish human handling, balance or two-hour enjoyment.
- [x] Import the genuine eleven-completion `e797…` checkpoint and add jobs 12/7/15/23: a nine-stop, eight-trip train/bus relay and Centre/Hanlan/Ward roundtrip posts reach done 15 / bitmap `FFDB80…` / cash 1,528, cumulatively covering all eight kinds. Credits 267/143/154/167 and eight relay fares $20 / positioning train $3 / six ferry fares $24 reconcile. [Curated evidence](docs/NATIVE_CAMPAIGN_FIFTEEN_SAMPLES.json) retains ordinary public foot routes, ordered client/dock returns and all six settled source-authenticated ferry scene reads, with the car parked in Core. Eleven completions are inherited; only four are new, and the final neutral checkpoint is saved only. Remaining 81 contracts, six other Island contracts, wider native acceptance and measured human enjoyment remain pending.
- [x] Import the genuine fifteen-completion `e797…` checkpoint and add jobs 72/74/80/88, reaching done 19 / cash 122. Preserve a cargo-zero West failure with no payout/new bit, its ordinary car/optional-foot retry, required-motorcycle High Park work with 18 seconds left, East round and required-truck Port apron delivery through Leslie. [Curated evidence](docs/NATIVE_CAMPAIGN_NINETEEN_SAMPLES.json) authenticates five newly visited mainland scenes; Islands are inherited, so all six scenes are cumulative. Credits 170/103/139/122 and observed decreases reconcile; controller guard/tolerance changes and explicit native-five-pixel corridor recovery are not ROM fixes or guaranteed seven-pixel clearance. Final checkpoint is save-only; same-frame fullscreen RESULT OAM is not world/performance proof. Remaining 77 contracts, six other Island contracts, broader native coverage and measured human enjoyment remain pending.
- [x] Import the genuine nineteen-completion `e797…` checkpoint and add jobs 89/77/90, reaching done 22 / cash 48. Verify Fire Hall, Colborne Lodge and Crane Walk foot-only clients, all three original-car recoveries and ordered Lodge returns. Credits 75/181/48 and observed decreases reconcile. [Curated evidence](docs/NATIVE_CAMPAIGN_TWENTY_TWO_SAMPLES.json) identifies Commissioners / Lake Shore Don / Cherry North deck crossings from actual coordinates, correcting misleading driver labels. Preserve the missed West→High Park controller guard, ordinary recovery and explicit five-pixel driver policy without a ROM fix or seven-pixel guarantee. Matching-counter 121 loops / 360 VBlanks and two world OAM samples are narrow stopped-Port observations; final neutral ROAM checkpoint is saved only. Remaining 74 contracts, six other Island contracts, broader native/human/hardware acceptance remain pending.
- [ ] Broaden corrected-ROM campaign, vehicle/crowd/district/pacing and save-import checks; retain failed c4fa and earlier 964f scopes separately. All 96 contracts, remaining Island jobs/full Old Toronto, two enjoyable measured human hours, browser and physical cartridge acceptance remain pending.

## Update-local terrain cache — source experiment, wider acceptance pending

- [x] Reuse one exact whole-pixel full-body terrain result per update with six automatic bytes; retain live dynamic guards and unchanged routes, input, clock, save v9 / 58 bytes and 1,096-byte persistent reserve.
- [x] Retain the first 16,501-byte gameplay-unit failure (117 over); move only the new helper to a banked module and build distinct `825444a0…`. Verify the actual pointer/nested-call ABI in 611 compiled checks; update frame 10→16 and eight helper locals are not deepest-stack acceptance.
- [x] Match 6,463 actual-C snapshots / 3,425,390 fields, including cargo/cash/save, actors, people/fleet, tile-hit markers and modeled bank preservation. Engine 804,856 and roads 2,339,979 checks pass.
- [x] Repeat the [fresh Core comparison](docs/NATIVE_TERRAIN_CACHE_COMPARISON.json): idle 361→385 loops / 1,080 VBlanks (+6.648%); fixed moving tail 77→80 / 240 (+3.896%). Preserve these modest scopes without whole-city or human-FPS claims.
- [x] Close a separate [825 gameplay replay](docs/NATIVE_TERRAIN_CACHE_GAMEPLAY.json) for first delivery, fast approach/glancing/solid-rail/reverse controls, H1/H2 contacts, map freeze, walking/car entry and in-worker committed SRAM recovery. Earlier e797 campaign completions are not inherited by this fresh replay.
- [x] Correct stale WALK/car controls after completed entry and active-job dispatch resume/feedback in the distinct direct UI milestone below; retain the 825 experiment and historical e797 bundle at their own identities.
- [ ] Broaden scene/crowd/stack/performance and remaining-contract coverage; full Old Toronto, two measured enjoyable human hours, browser and physical cartridge acceptance remain open.

## Direct active-job and entry HUD polish — 03e09 scoped native pass

- [x] Preserve the full pickup-first itinerary while showing the actual CURRENT STOP and A RESUME during work; retain individual offer locks, chapter/exit priority, cancellation and no-job acceptance. Add one completion-event HUD repaint with no new production state.
- [x] Retain seven engine/eight UI old-source failures, then pass the corrected full source suite: 804,864 engine / 25,110,098 UI. Match 6,463 cached/uncached snapshots / 3,431,853 fields including the host-only redraw observation.
- [x] Build direct `03e09…` without fallback, with bank 2 at 16,380 / 16,384 bytes and 1,096 static reserve. Close a fresh [native replay](docs/NATIVE_UI_POLISH_SAMPLES.json) for active preview/resume/chords/exits, board/map freeze, immediate car HUD, interrupted-entry freeze/resume, one full-condition Market Start and in-worker SRAM recovery. First Art is accepted but its remote pickup is rejected and stays stage 0.
- [x] Pass independent compiled/raw native reviews with 597 / 5,253 checks.
- [x] Audit one separate fresh 03e09 Core timing trial: 385 completed loops / 1,080 VBlanks, then 80 / 240 under A60/A60/B60/neutral60. Its 1,085-check audit retains exact 24 intervals / 92 memory receipts/one OAM; older 825 repeats and gain comparisons remain separate.
- [x] Public projection review passes 481 checks; the prepared source-pinned loading ZIP passes 4,052 independent package checks. Keep the historical e797 22-completion campaign and 825 performance gains separate; broader contracts/regions/Island jobs, full Old Toronto, measured enjoyable human duration, deepest stack/pacing, browser and physical acceptance remain pending.

## Device milestone

- [ ] Identify the connected Chromatic and supported writable cartridge; establish Developer Mode readiness.
- [ ] Stream if supported, then load the user's identified writable development cartridge through the official workflow.
- [ ] Verify write/read-back where available, manual cold boot, driving/braking, a full delivery, transit, save recovery and audio.

No physical streaming or cartridge write has been attempted. Record device observations separately from emulator results without private activation or device details.
