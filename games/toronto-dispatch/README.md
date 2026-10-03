# Toronto Dispatch

A north-up, top-down pixel-art courier sandbox for ModRetro Chromatic / Game Boy Color. Drive with momentum and braking, park and walk, or pay for scheduled transit while delivering timed jobs through six compressed Toronto districts.

## Play and install

The current local candidate is `project/build/toronto-dispatch-courier-clearance.gbc`, **524,288 bytes**, SHA-256 **`e797f5725574248915f89945dcb1c1b59c7906c8ebe8dc1f155f97b56000374f`**. Follow the [Chromatic loading guide](docs/LOADING.md); generated ROMs stay out of Git. Use the ModRetro Chromatic plugin's native emulator to play the exact inspected build. Published [Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) remains an older separate release.

| Action | Controls |
| --- | --- |
| Drive | A accelerates; left/right steer; B brakes and then reverses near rest |
| Close delivery result | B returns to the city without reversing while held. Release B, then press it again for normal braking/reverse. A opens dispatch |
| Plan a quest | Select opens dispatch; Select on the board jumps to the next eight-job chapter, left/right selects jobs, up/down browses ordered stops; A accepts, B returns |
| Collect / hand off | Select at each ordered marker; finish returning jobs at their final stop |
| Walk / enter car | Pause → Park / recover car; D-pad walks, A enters near the parked car |
| Vehicle | Pause → Change vehicle, while stopped and permitted by the job |
| Transit | On foot, B at a station/terminal/platform; left/right selects, A waits/boards. At Wellesley, up switches train/bus |
| City map | Pause → Scroll City Map; D-pad pans, A centres job/booked stop/depot, Select changes focus, B returns |
| Save / audio | Pause menu; Audio cycles music + effects, effects only and silent |

When accepting a quest, review every stop and walking/return cue. Arrival alone does not advance a handoff: use Select. Mainland cars stay parked during transit and Island walking; an idle Island courier with insufficient cash has scheduled $0 return assistance.

## City and jobs

Source contains **241 buildings, 595 fixed pedestrian routes, 96 contracts, 59 service points and seven mainland parking anchors**. Four player vehicles, wide roads, varied original buildings, walking/car entry, roof/canopy occlusion, a scrollable atlas, original audio and save v9 / 58-byte progression are implemented. The six areas cover compressed Core, western neighbourhoods, High Park/Junction, eastern neighbourhoods, Port Lands and public Islands.

Jobs include packages, fragile art, freight, signatures/returns and transit relays. Port Lands uses legal Leslie access and authored collision-backed bridges; Islands use ferry/public walking paths. Civilian impacts cause non-graphic recovery, condition loss on carried jobs, escalating fines and local police pursuit. Six road vehicle kinds obey fictional signals; planes/helicopters and under-deck boats are cosmetic. Visible road traffic and paid transit schedules are separate game abstractions.

![Visible street impact](docs/screenshots/courier-clearance-visible-impact.png) ![Delivery result](docs/screenshots/courier-clearance-market-result.png)

Unmodified sampled Core frames on current `e797…`; [provenance](docs/screenshots/courier-clearance-provenance.json) records exact frames and hashes.

## Verified scope

Current `e797…` passes official/header/compiled guards and full repository checks, with a 1,096-byte static reserve. Its [native records](docs/NATIVE_COURIER_CLEARANCE_SAMPLES.json) cover all twelve chapter starts and held-input rules, Market Start, held RESULT B, a visible walker waiting then resuming after reverse clearance, a genuine forward pedestrian impact and map simulation freeze. A separate record imports its genuine same-ROM checkpoint and verifies saved cash/progression/attention through the game's reset. Fresh plugin workers deliberately start with empty save RAM; physical persistence remains unverified.

One genuine campaign now reaches [22 unique completions](docs/NATIVE_CAMPAIGN_TWENTY_TWO_SAMPLES.json), cumulatively covering all eight quest kinds and all six loaded districts. Matching continuations sample required truck/car work, motorcycle/scooter deliveries, condition-scaled rewards, paid train/bus relays and Centre/Hanlan/Ward roundtrip posts. The latest three jobs verify true foot-only clients, original-car recovery and Commissioners, Lake Shore Don and Cherry North road decks. The earlier west-end continuation retains a cargo-exhaustion failure and its successful mixed driving/walking retry. These are scoped emulator tests; imported completions, controller corrections and actual collision-body route checks are disclosed.

The retained `964f…` five-job chain, earlier matched transit/driving trials and older Queen/13-job Island records retain their own hashes in [TESTING.md](TESTING.md) and [BUILD.md](docs/BUILD.md). Their wider acceptance is not inherited by this ROM. The failed `c4fa…` speed-gated clearance candidate remains documented separately.

The remaining 74 contracts, including six Island jobs, full Old Toronto, two measured enjoyable human hours, wider handling/balance/pacing/stack, native older-save imports, browser recovery, human handling/audio and physical cartridge execution remain pending.

## Develop

Select `project/project.gbsproj` with the ModRetro Chromatic plugin. Read [DESIGN.md](DESIGN.md), [DECISIONS.md](DECISIONS.md) and [ROADMAP.md](ROADMAP.md) before changing gameplay; follow [BUILD.md](docs/BUILD.md) and run `make check` from the repository root. Original code/art are MIT licensed; [distribution notices](docs/DISTRIBUTION.md) retain upstream terms.
