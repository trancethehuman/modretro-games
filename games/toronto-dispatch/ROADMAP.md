# Roadmap

## 0 — Repository and decisions

- [x] Separate local Git repository, MIT licence, collection README, agent instructions, and skills.
- [x] Record the user's chosen jobs, view, handling, and city requirements.
- [x] Seed mission/vehicle planning content and source references with a validator.
- [x] Attach ModRetro Chromatic plugin and inspect its setup/build/playtest skills.
- [ ] Publish the repository and verify public visibility and files.

## 1 — Driving feasibility

- [x] Create a valid editable GB Studio project through the plugin.
- [x] Compile and boot a labelled starter room before implementing gameplay.
- Build an original angled test intersection with a car, wall collision, and depth ordering.
- Implement acceleration, coasting, braking, reverse, and speed-dependent turning.
- Build a valid ROM and open its playable emulator preview.
- Acceptance: no instant velocity changes; equal intended diagonal/cardinal speeds; braking before reversal; no wall penetration or permanent collision trap; compass/input readable on handheld.

## 2 — First complete delivery

- Research and draw a compressed connected downtown map with one depot and three distinct destination landmarks.
- Add acceptance, pickup, carrying, delivery, timeout, cancellation, and retry states.
- Add a signal-controlled intersection, bounded traffic, and a streetcar obstacle on a researched corridor.
- Acceptance: all three jobs have reachable endpoints; stopped interaction is required; vehicle compatibility enforced; pause freezes simulation/time; deadline and reward tuned from actual route runs; roaming works without a job.

## 3 — Device milestone

- Establish the actual console, cartridge, and Developer Mode readiness.
- Stream the tested build if supported, then load the identified writable cartridge.
- Acceptance: verified write/read-back where available, manual cold boot, movement/braking, one complete job, pause/restart, and audio. Record results without private device details.

## 4 — Vehicles and shift progression

- Add truck, motorcycle, and scooter with distinct handling and vehicle-specific jobs.
- Add a simple dispatch selection screen, shift score, and passenger pickup/drop-off with comfort feedback.
- Evaluate save support and test persistence before adding saved progression.

## 5 — Larger Toronto

- Add researched neighbourhood districts, landmarks, street/transit labels, and industrial areas.
- Verify map connections, mission continuity, memory/performance budgets, and district transitions.
- Add environmental variations only after the core route/traffic loop is enjoyable.

## Release evidence

Each release needs editable sources, attribution, actual build/toolchain versions, emulator checks, and a digest of the released ROM. Publish only original homebrew outputs. Hardware results must state the tested device/cartridge class and distinguish manual observations from tool reports.
