# Roadmap

## Playable prototype — implemented

- [x] Public MIT collection repository, one folder per game, README, AGENTS.md and workflow skills.
- [x] Genuine editable GB Studio project and original project-local scene engine.
- [x] User-revised north-up presentation, 1,024 × 976 scrollable city, 80 buildings and six architecture types.
- [x] Four road vehicles; momentum, traction, braking/reverse, speed-dependent yaw and glancing curb recovery.
- [x] Native building/water/rail collision, roof/canopy priority, walking animation and visible car entry/exit.
- [x] Autonomous pedestrians and bounded road traffic.
- [x] 72 compiled contracts, dispatch, collection/delivery, progression, cargo damage, vehicle constraints and timeout/retry.
- [x] Fictional repeating schedules and paid travel on researched Line 1, 94 bus and Island ferry service points.
- [x] Pause/map freeze the clock; transit resumes correctly after pausing.
- [x] Native collision connectivity and contract/source consistency in repository CI.
- [x] Native emulator delivery/transit/map/entry/collision/save checks. See exact build scopes in TESTING.md.

## Next playable polish

- [ ] Resolve the plugin's unresolved browser recording-close acknowledgement, then show and test the current city preview.
- [ ] Tune steering, lane width, traffic spacing and frame pacing from handheld/human feedback.
- [ ] Add sound effects, original music and volume options.
- [ ] Expand pedestrian variety, street furniture, local activity and visible transit boarding/riding.
- [ ] Author streetcar rails and moving streetcars on researched corridors; improve the current partial bus route geometry.
- [ ] More distinctive Toronto landmark silhouettes, district character, industrial yards and additional navigable streets.

## Full game release

- [ ] Expand beyond the compressed central map to the historical Old Toronto footprint, preserving researched topology and Islands.
- [ ] Refine contract writing/objectives and sample all eight job types, including the unlock sequence and vehicle rules.
- [ ] Record representative timings and a complete campaign; validate **at least two hours of varied, enjoyable gameplay**. Quest count does not verify duration.
- [ ] Test crowded-scene sprite/CPU limits, diagonal walking, collision corner recovery, cancellation and schedule edge cases.
- [ ] Test SRAM cold boot, interrupted saves and safe recovery from interrupted transit.
- [ ] Audit linked runtime/distribution notices before publishing ROM releases.

## Device milestone

- [ ] Identify the connected Chromatic and supported writable cartridge; establish Developer Mode readiness.
- [ ] Stream if supported, then load the user's identified writable development cartridge through the official workflow.
- [ ] Verify write/read-back where available, manual cold boot, driving/braking, a full delivery, transit, save recovery and audio.

No physical streaming or cartridge write has been attempted. Record device observations separately from emulator results without private activation or device details.
