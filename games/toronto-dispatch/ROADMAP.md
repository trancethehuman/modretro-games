# Roadmap

## Playable prototype — implemented

- [x] Public MIT collection repository, one folder per game, README, AGENTS.md and workflow skills.
- [x] Genuine editable GB Studio project and original project-local scene engine.
- [x] User-revised north-up presentation, 1,024 × 976 scrollable city, 80 buildings and six architecture types.
- [x] Four road vehicles; momentum, traction, braking/reverse, speed-dependent yaw and glancing curb recovery.
- [x] Native building/water/rail collision, roof/canopy priority, walking animation and visible car entry/exit.
- [x] Autonomous pedestrians and bounded road traffic.
- [x] Native dispatch, collection/delivery, completion records, progression, cargo damage, vehicle constraints and timeout/retry in the playable prototype.
- [x] Fictional repeating schedules and paid travel on researched Line 1, 94 bus and Island ferry service points.
- [x] Pause/map freeze the clock; transit resumes correctly after pausing.
- [x] Native collision connectivity and contract/source consistency in repository CI.
- [x] Native emulator delivery/transit/map/entry/collision/save checks. See exact build scopes in TESTING.md.

## Authored campaign revision — content checks passed

- [x] 72 distinct original titles and routes across nine progressive chapters, with two short brief lines generated into native content.
- [x] 27 collision-verified walkable service points, including three fictional Island delivery entrances beyond the ferry docks.
- [x] Distinct transit relays, Island walking legs, mainland ferry transfers, signed-return routes and freight/passenger vehicle constraints.
- [x] Collision-path-based time allowances, text bounds, deterministic native content generation and unlock closure through distinct completions.
- [x] Build the revised ROM; native smoke verifies first-job condition reward, eligible dispatch, walking/entry and paid transit. Host checks cover passenger steering comfort. All eight job types still need full native playtests.

Campaign duration remains unverified. Contract counts, time limits and shortest-path estimates do not establish the two-hour release target.

## Next playable polish

- [ ] Resolve the plugin's unresolved browser recording-close acknowledgement, then show and test the current city preview.
- [ ] Tune steering, lane width, traffic spacing and frame pacing from handheld/human feedback.
- [ ] Add sound effects, original music and volume options.
- [ ] Expand pedestrian variety, street furniture, local activity and visible transit boarding/riding.
- [ ] Author streetcar rails and moving streetcars on researched corridors; improve the current partial bus route geometry.
- [ ] More distinctive Toronto landmark silhouettes, district character, industrial yards and additional navigable streets.

## Full game release

- [ ] Expand beyond the compressed central map to the historical Old Toronto footprint, preserving researched topology and Islands.
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
