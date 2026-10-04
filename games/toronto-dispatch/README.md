# Toronto Dispatch

A north-up, top-down pixel-art courier sandbox for ModRetro Chromatic / Game Boy Color. Deliver timed jobs through seven compressed Toronto districts: drive with momentum and braking, park and walk to clients, or pay for scheduled transit to take a shortcut.

![Baldwin Steps on foot](docs/playtest-north-current/baldwin-stairs-63970.png) ![Manitou walking bridge](docs/playtest-relay-island-current/manitou-bridge-82748.png)

Original 160 × 144 frames from the current `7ab28…` ROM. The [North](docs/NATIVE_NORTH_CURRENT_CONTINUATION.json) and [relay/Island](docs/NATIVE_RELAY_ISLAND_CURRENT_CONTINUATION.json) records retain their exact native provenance.

## Play and install

Use the [Chromatic loading guide](docs/LOADING.md). The reviewed local bundle is **`project/build/toronto-dispatch-vehicle-feedback-reviewed.zip`**, 146,249 bytes, SHA-256 `f897d7bf93eeb2c9baaed7304f10aea9c24e8af0c73173f1d42584f15f7b9b38`. It contains the tested **`toronto-dispatch-vehicle-feedback.gbc`**, 1,048,576 bytes, SHA-256 `7ab28b84c242f7f2c8f9e02338e2d81ab6d789fb1d7997d0aa33e99baadd8dc0`, with checksums, BUILDINFO, instructions and licences. Its guide is frozen at matching source commit `6e9cd46cc2a8af40a87dfc696283c08632261e28`; later evidence leaves the source inputs, ROM and ZIP unchanged. The [package audit](docs/DISPATCH_VEHICLE_PACKAGE_AUDIT.json) passes. Generated binaries remain local; [published Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) is an older separate build.

| Action | Controls |
| --- | --- |
| Drive | A accelerates; left/right steer; B brakes and then reverses near rest |
| Park and walk | Start → Park / recover car exits; D-pad walks, A enters near the original parked vehicle |
| Choose a vehicle | Start → Change vehicle, while stopped and permitted by the job |
| Plan a quest | Without active work, Select opens dispatch; during work use Start → Dispatch jobs. Select jumps chapters, left/right selects offers, up/down browses stops; A accepts or resumes, B returns |
| Collect / hand off | Select at each ordered marker; returning jobs finish at their final stop |
| Take transit | On foot, B at a station/terminal/platform; left/right selects, A waits/boards. At Wellesley, up switches train/bus |
| Browse the map | Start → Scroll City Map; D-pad pans, A centres job/booked stop/depot, Select changes focus, B returns |
| Save / audio | Start menu; Audio cycles music + effects, effects only and silent |

Release menu-used A/B before pressing them again to accelerate or brake. A delivery result's A opens dispatch. Review every stop before accepting; arrival alone does not advance the job, so press Select at the marker. Active previews show CURRENT STOP and A resumes the existing job. An eligible incomplete vehicle-specific offer shows WRONG VEHICLE until you occupy the required vehicle.

Mainland vehicles remain parked during transit and Island walking. Trains cost $3, buses $2, Queen streetcars $3 and ferries $4 in game money. WAIT can be cancelled with B without abandoning the contract or paying a fare. Menus pause the clock; waiting and riding use delivery time. Island offers show $8/$16/$24 required ferry budgets, with extra cash needed for optional travel and fines. If an active Island job cannot fund the return, use Start → Cancel Active Job, then B at the dock for scheduled $0 assistance. Abandoning pays nothing and earns no completion.

## City and jobs

Explore compressed Core, western neighbourhoods, High Park/Junction, eastern neighbourhoods, Port Lands, public Islands and Uptown Hills. The current source contains **344 buildings, 657 pedestrian routes, 104 contracts, 64 service points and nine mainland parking anchors**. Four player vehicles, wide roads, varied original buildings, roof/canopy occlusion, walking/car entry, a scrollable atlas, original audio and saved progression are implemented.

Eight job types cover parcel rounds, fragile art, express files, truck freight, transit relays, passenger rides, ordered returns and Island post. Cargo starts after pickup; damage reduces the base payment, while remaining time adds a bonus. Fast steering also reduces passenger comfort. Port Lands routes use Leslie access and authored bridges; Island deliveries use ferries and public walking paths. North includes railway underpasses, foot-only Baldwin Steps and public Casa Loma/Rosehill handoffs.

Six road vehicle kinds obey fictional signals. Pedestrian impacts cause non-graphic recovery, condition loss on carried jobs, escalating fines and local police pursuit. Aircraft and under-deck boats are scenery. Visible road traffic and paid transit schedules use separate game abstractions; geography is compressed and game service times/fares are fictional.

## Verification and remaining work

The current ROM passes official compilation, full `make check` and the [compiled review](docs/DISPATCH_VEHICLE_BUILD_AUDIT.json). A [fresh native run](docs/NATIVE_DISPATCH_VEHICLE_FEEDBACK.json) completes four unique jobs, checks truck eligibility and a funded police fine, and restores committed progress after an in-worker reset.

Separate genuine same-ROM checkpoint continuations reach **16 distinct completions**, representing all eight job types. [Passenger checks](docs/NATIVE_PASSENGER_CONTINUATION.json) cover vehicle/transit restrictions and slow/fast steering comfort. [North checks](docs/NATIVE_NORTH_CURRENT_CONTINUATION.json) add three jobs, underpass driving, foot delivery and original-car recovery. [Relay/Island checks](docs/NATIVE_RELAY_ISLAND_CURRENT_CONTINUATION.json) add two jobs, nine paid rides, unpaid-WAIT cancellation, public bridge walking, map panning/freezing, original-motorcycle recovery and committed in-worker reset. [East/Port checks](docs/NATIVE_EAST_PORT_CURRENT_CONTINUATION.json) add a full-condition motorcycle round through Riverside and Danforth, then required-truck freight through Leslie to the Channel apron. Cumulative recordings include source-loaded scenes from all seven districts. Imported completions are retained as ancestry, rather than counted as newly played in each continuation.

The [testing record](TESTING.md) preserves exact build identities, older evidence, failed attempts and controller corrections. Motorcycle job 74's successful automated route has a one-second deadline margin; passenger transit refusal still shares awkward heavy-cargo wording. Full 104-contract play, two measured enjoyable human hours, broader handling/reward/deadline tuning, older-save imports, performance/stack, browser recovery and physical cartridge cold-boot/save/audio checks remain pending. Emulator reset verifies committed in-worker progress, with periodic saves, rather than physical battery persistence or every latest live field.

## Develop

Select `project/project.gbsproj` with the ModRetro Chromatic plugin. Read [DESIGN.md](DESIGN.md), [DECISIONS.md](DECISIONS.md) and [ROADMAP.md](ROADMAP.md) before changing gameplay; follow [BUILD.md](docs/BUILD.md) and run `make check` from the repository root. Original code/art are MIT licensed; [distribution notices](docs/DISTRIBUTION.md) retain upstream terms.
