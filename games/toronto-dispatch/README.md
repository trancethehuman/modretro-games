# Toronto Dispatch

A north-up, top-down pixel-art courier sandbox for ModRetro Chromatic / Game Boy Color. Drive with momentum and braking, park and walk, or pay for scheduled transit while delivering timed jobs through seven compressed Toronto districts. The latest traffic candidate has a fresh scoped street-system replay; older campaign records retain their original ROMs.

## Play and install

Latest local playtest candidate is `project/build/toronto-dispatch-right-hand-traffic-filter.gbc`, **1,048,576 bytes**, SHA-256 **`e4a9fb301fd4ba6a00c58e2f8e756924d8398d6f373f7c6864cc1cdff28ac2d6`**. Official build, full source checks and compiled lane/ABI review pass. Follow the [Chromatic loading guide](docs/LOADING.md), [build record](docs/BUILD.md) and [fresh native record](docs/NATIVE_RIGHT_HAND_TRAFFIC.json). This exact ROM completes three unique deliveries and samples escalating pursuit, cash-zero recovery, paid North train trips, walking/car entry, map controls and in-worker save restoration. Generated ROMs stay out of Git. Published [Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) and older local packages retain their own identities.

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

![Core street traffic and courier](docs/native-right-hand-traffic/occupied-police-lane.png) ![St Clair on foot](docs/native-right-hand-traffic/st-clair-on-foot.png)

Original 160 × 144 emulator frames from the current `e4a9…` traffic ROM: Core at frame 7,322 and St Clair at 13,648. The [native record](docs/NATIVE_RIGHT_HAND_TRAFFIC.json) retains their exact source, input, PNG and pixel hashes. These are emulator captures; historical Baldwin Steps and North campaign evidence remains separately linked in TESTING.md.

## Verified scope

Current `e4a9…` passes the official native build and full `make check`, including 881,411 real-engine and 528,282 police checks. The [build audit](docs/RIGHT_HAND_TRAFFIC_BUILD_AUDIT.json) authenticates all 206 inputs, all 36 right-hand circuits and compiled body/ABI/table guards. Police return follows a finite directed-lane policy; pursuit remains a bounded local search. The helper bank has one free byte, gameplay ten, with 1,096 static reserve bytes and no peak-stack guarantee.

The [fresh closed replay](docs/NATIVE_RIGHT_HAND_TRAFFIC.json) delivers jobs 0/1/2, shows $20/$40/$60 human-impact fines and a carried H3 delivery, then underfunded H3 capture takes $95 to zero while preserving accepted work. A full-condition delivery recovers $107; two $3 Union–St Clair train trips leave $101. Waiting/map controls preserve the booking, walking recovers the original parked car, and a real in-worker reset restores three completions/$101. A stopped car blocks police's legal lane until it moves; the return then rejoins the ordinary circuit. A wrong street chosen by the test controller hit a building, and short frame intervals sometimes caught pending updates; those limitations remain in the evidence.

An initial `fc2ef…` traffic run remains [needs-review](docs/NATIVE_TRAFFIC_INITIAL_REVIEW.json). The coordinate filter preserves 3,306,549 actual-C query results and reduces membership-exit work 94.8% in that synthetic workload. No matched native speedup, whole-city frame rate, hardware or human-duration claim is inferred.

The [older North records](docs/NORTH_DISTRICT.md) belong to unchanged `22157…`: three records reach six growing completions; a fourth reaches eight but remains needs-review after its Pause-close reverse leak. The current candidate retains menu-input release and adds traffic corrections; its own three completions and two train trips are recorded separately. Old underpass/campaign results are not inherited. Earlier `03e09…` two-job and `e797…` 22-job campaign records remain in [TESTING.md](TESTING.md); their exact ROM/source/package pins remain in [BUILD.md](docs/BUILD.md).

All 104 contracts, remaining region/Island routes, full Old Toronto, two measured enjoyable human hours, wider handling/balance/pacing/stack, native older-save imports, browser recovery, human handling/audio and physical cartridge execution remain pending.

## Develop

Select `project/project.gbsproj` with the ModRetro Chromatic plugin. Read [DESIGN.md](DESIGN.md), [DECISIONS.md](DECISIONS.md) and [ROADMAP.md](ROADMAP.md) before changing gameplay; follow [BUILD.md](docs/BUILD.md) and run `make check` from the repository root. Original code/art are MIT licensed; [distribution notices](docs/DISTRIBUTION.md) retain upstream terms.
