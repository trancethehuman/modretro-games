# Toronto Dispatch

A north-up, top-down pixel-art courier sandbox for ModRetro Chromatic / Game Boy Color. Drive with momentum and braking, park and walk, or pay for scheduled transit while delivering timed jobs through seven compressed Toronto districts. The latest menu-input candidate has a fresh scoped control replay; older campaign records retain their original ROMs.

## Play and install

Latest local playtest candidate is `project/build/toronto-dispatch-menu-input-release.gbc`, **1,048,576 bytes**, SHA-256 **`7ccfa3b0532b2fa2f65f5198644c8f5ee3dc59df2b2e40034a3f1af954bdbeb9`**. Official build, full source checks, public curation and independent native/prose review pass; its new package remains pending. Follow the [Chromatic loading guide](docs/LOADING.md) and [build record](docs/BUILD.md), then use the ModRetro Chromatic plugin's native emulator for the exact inspected ROM. Generated ROMs stay out of Git. Published [Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) and the prepared older North bundle remain separate artifacts.

| Action | Controls |
| --- | --- |
| Drive | A accelerates; left/right steer; B brakes and then reverses near rest |
| Leave menus / delivery result | Menu-used A/B stay consumed until released. Release and press again to accelerate or brake/reverse; steering and walking work normally. RESULT A opens dispatch |
| Plan a quest | Without active work, Select opens dispatch; during work use Pause → Dispatch jobs. Select jumps chapters, left/right selects offers, up/down browses stops; A accepts a new job or resumes active work, B returns |
| Collect / hand off | Select at each ordered marker; finish returning jobs at their final stop |
| Walk / enter car | Pause → Park / recover car; D-pad walks, A enters near the parked car |
| Vehicle | Pause → Change vehicle, while stopped and permitted by the job |
| Transit | On foot, B at a station/terminal/platform; left/right selects, A waits/boards. At Wellesley, up switches train/bus |
| City map | Pause → Scroll City Map; D-pad pans, A centres job/booked stop/depot, Select changes focus, B returns |
| Save / audio | Pause menu; Audio cycles music + effects, effects only and silent |

When accepting a quest, review every stop and walking/return cue. An active preview shows CURRENT STOP separately from its browsed itinerary; A resumes the carried job without replacing it. Arrival alone does not advance a handoff: use Select. Mainland cars stay parked during transit and Island walking; an idle Island courier with insufficient cash has scheduled $0 return assistance.

## City and jobs

Current source contains **344 buildings, 657 fixed pedestrian routes, 104 contracts, 64 service points and nine mainland parking anchors**. Four player vehicles, wide roads, varied original buildings, walking/car entry, roof/canopy occlusion, a scrollable atlas, original audio and save v10 / 58-byte progression are implemented in current source. The seven areas cover compressed Core, western neighbourhoods, High Park/Junction, eastern neighbourhoods, Port Lands, public Islands and Uptown Hills. North adds researched railway underpasses, foot-only Baldwin Steps, public Casa Loma/Rosehill handoffs and two fictional Line1 landings; it preserves the previous 96 contracts and 59 stops.

Jobs include packages, fragile art, freight, signatures/returns and transit relays. Port Lands uses legal Leslie access and authored collision-backed bridges; Islands use ferry/public walking paths. Civilian impacts cause non-graphic recovery, condition loss on carried jobs, escalating fines and local police pursuit. Six road vehicle kinds obey fictional signals; planes/helicopters and under-deck boats are cosmetic. Visible road traffic and paid transit schedules are separate game abstractions.

![Baldwin Steps courier approach](docs/evidence/north-continued/north-continued-6183.png) ![St Clair train arrival](docs/evidence/north-continued/north-continued-9674.png)

Unmodified 160 × 144 emulator frames at Baldwin Steps (6,183) and St Clair (9,674), exact North ROM `22157…`. The [fresh record](docs/NATIVE_NORTH_FRESH.json) and its [genuine-checkpoint continuation/provenance](docs/NATIVE_NORTH_CONTINUED.json) retain source, input, PNG and pixel hashes; these are emulator captures.

## Verified scope

Current `7ccfa…` passes official build, 231 changed-code and 219,647 independent source/resource checks. Full `make check` passes 881,419 real-engine cases; terrain-cache comparison matches 6,508 snapshots / 3,475,272 fields. Its fresh closed ordinary-input replay completes only Market Start at condition 100 / credit 104 / cash 134 / done 1. Held Help/dispatch/pause/result/save inputs, independent A/B release, fresh reverse/acceleration, truck coast, Core→North Yonge travel, walking/car recovery and exact paused-map state are sampled. A real in-worker reset retains the later periodic save; it does not establish physical persistence or latest-live full-state equality. The [public native record](docs/NATIVE_MENU_INPUT_RELEASE.json) passes 13,669 curation checks; independent native/prose review passes 34,355 checks with zero findings. New packaging remains pending.

Two stationary timing samples count 345 Core and 686 uncrowded North engine loops over 1,080 VBlanks each. They are scoped workload observations, without a whole-city performance or human-responsiveness claim. A separate source audit identifies incorrect or centreline road traffic across 36 role loops; a lane fix and its native verification remain pending.

The screenshots above and [older North records](docs/NORTH_DISTRICT.md) belong to unchanged `22157…`: three records reach six growing completions; a fourth reaches eight but remains needs-review after its Pause-close reverse leak. The new candidate separately corrects menu-input release and does not inherit those completions, train rides or underpass checks. Earlier `03e09…` two-job and `e797…` 22-job campaign records remain in [TESTING.md](TESTING.md); their exact ROM/source/package pins remain in [BUILD.md](docs/BUILD.md).

All 104 contracts, remaining region/Island routes, full Old Toronto, two measured enjoyable human hours, wider handling/balance/pacing/stack, native older-save imports, browser recovery, human handling/audio and physical cartridge execution remain pending.

## Develop

Select `project/project.gbsproj` with the ModRetro Chromatic plugin. Read [DESIGN.md](DESIGN.md), [DECISIONS.md](DECISIONS.md) and [ROADMAP.md](ROADMAP.md) before changing gameplay; follow [BUILD.md](docs/BUILD.md) and run `make check` from the repository root. Original code/art are MIT licensed; [distribution notices](docs/DISTRIBUTION.md) retain upstream terms.
