# Toronto Dispatch

A north-up, top-down pixel-art courier sandbox for ModRetro Chromatic / Game Boy Color. Drive with momentum and braking, park and walk, or pay for scheduled transit while delivering timed jobs through six compressed Toronto districts.

## Play and install

The current local candidate is `project/build/toronto-dispatch-ui-polish-direct.gbc`, **524,288 bytes**, SHA-256 **`03e09fa85351c91f56d7370a3f0e0f856cd147a3c8d5ebfd328a2786a38f25d6`**. Follow the [Chromatic loading guide](docs/LOADING.md); generated ROMs stay out of Git. Use the ModRetro Chromatic plugin's native emulator to play the exact inspected build. Published [Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) remains an older separate release.

| Action | Controls |
| --- | --- |
| Drive | A accelerates; left/right steer; B brakes and then reverses near rest |
| Close delivery result | B returns to the city without reversing while held. Release B, then press it again for normal braking/reverse. A opens dispatch |
| Plan a quest | Without active work, Select opens dispatch; during work use Pause → Dispatch jobs. Select jumps chapters, left/right selects offers, up/down browses stops; A accepts a new job or resumes active work, B returns |
| Collect / hand off | Select at each ordered marker; finish returning jobs at their final stop |
| Walk / enter car | Pause → Park / recover car; D-pad walks, A enters near the parked car |
| Vehicle | Pause → Change vehicle, while stopped and permitted by the job |
| Transit | On foot, B at a station/terminal/platform; left/right selects, A waits/boards. At Wellesley, up switches train/bus |
| City map | Pause → Scroll City Map; D-pad pans, A centres job/booked stop/depot, Select changes focus, B returns |
| Save / audio | Pause menu; Audio cycles music + effects, effects only and silent |

When accepting a quest, review every stop and walking/return cue. An active preview shows CURRENT STOP separately from its browsed itinerary; A resumes the carried job without replacing it. Arrival alone does not advance a handoff: use Select. Mainland cars stay parked during transit and Island walking; an idle Island courier with insufficient cash has scheduled $0 return assistance.

## City and jobs

Source contains **241 buildings, 595 fixed pedestrian routes, 96 contracts, 59 service points and seven mainland parking anchors**. Four player vehicles, wide roads, varied original buildings, walking/car entry, roof/canopy occlusion, a scrollable atlas, original audio and save v9 / 58-byte progression are implemented. The six areas cover compressed Core, western neighbourhoods, High Park/Junction, eastern neighbourhoods, Port Lands and public Islands.

Jobs include packages, fragile art, freight, signatures/returns and transit relays. Port Lands uses legal Leslie access and authored collision-backed bridges; Islands use ferry/public walking paths. Civilian impacts cause non-graphic recovery, condition loss on carried jobs, escalating fines and local police pursuit. Six road vehicle kinds obey fictional signals; planes/helicopters and under-deck boats are cosmetic. Visible road traffic and paid transit schedules are separate game abstractions.

![Visible street impact](docs/screenshots/courier-clearance-visible-impact.png) ![Delivery result](docs/screenshots/courier-clearance-market-result.png)

Unmodified sampled Core frames on retained `e797…`; [provenance](docs/screenshots/courier-clearance-provenance.json) records exact frames and hashes.

## Verified scope

Selected `03e09…` passes the official build/resource guards and full source suite: 804,864 engine and 25,110,098 UI checks, plus 6,463 matched terrain-cache snapshots / 3,431,853 field payloads. Save v9 remains 58 bytes with a 1,096-byte linked reserve; the direct UI edits fit without a fallback. A fresh [native record](docs/NATIVE_UI_POLISH_SAMPLES.json) verifies current/other/locked/completed previews, A resume and B/Start/Select priorities, entry HUD repaint before the next world second, interrupted-entry freeze/resume, map panning and one Market Start delivery (credit 108 / cash 138 / done 1). First Art is accepted but not picked up or completed. A genuine in-worker reset retains the committed active job and earnings; physical persistence remains unverified. Independent compiled/native reviews pass 597 / 5,253 checks; published evidence passes 481 checks. The prepared loading package passes 4,052 independent checks against source `9816ac43…`; its checksum and instructions are in [LOADING.md](docs/LOADING.md).

A [same-ROM continuation](docs/NATIVE_UI_POLISH_ART.json) imports that genuine one-job checkpoint and adds First Art: cargo condition 88%, credit 123, cash 261 and done 2. Ordinary walking, active-foot board freeze/resume, original-car recovery and durable SRAM reset pass. The fresh record above still completes only Market; these two current-ROM completions form one growing save. [Original frames](docs/screenshots/ui-polish-art-provenance.json) retain byte and pixel provenance.

The [retained e797 campaign](docs/NATIVE_CAMPAIGN_TWENTY_TWO_SAMPLES.json) reaches 22 unique completions and all six loaded districts, including foot clients, train/bus relays, three Island roundtrips and three Port road decks. Imported progress, failures/retries and test-controller corrections remain explicit. Its progress is not assigned to `03e09…`. The retained `825…` [cache comparisons](docs/NATIVE_TERRAIN_CACHE_COMPARISON.json) measure modest Core workload gains; this UI ROM has its own single Core timing trial: 385 loops / 1,080 VBlanks and an 80-loop / 240-VBlank driving tail. The numbers match 825, without inheriting its repeated trials or establishing whole-city pacing. Other exact-ROM evidence and failed candidates remain in [TESTING.md](TESTING.md) and [BUILD.md](docs/BUILD.md).

All 96 contracts, the remaining region/Island routes, full Old Toronto, two measured enjoyable human hours, wider handling/balance/pacing/stack, native older-save imports, browser recovery, human handling/audio and physical cartridge execution remain pending.

## Develop

Select `project/project.gbsproj` with the ModRetro Chromatic plugin. Read [DESIGN.md](DESIGN.md), [DECISIONS.md](DECISIONS.md) and [ROADMAP.md](ROADMAP.md) before changing gameplay; follow [BUILD.md](docs/BUILD.md) and run `make check` from the repository root. Original code/art are MIT licensed; [distribution notices](docs/DISTRIBUTION.md) retain upstream terms.
