# Toronto Dispatch

A north-up, top-down pixel-art courier sandbox for ModRetro Chromatic / Game Boy Color. Deliver timed jobs through seven compressed Toronto districts: drive with momentum and braking, park and walk to clients, or pay for scheduled transit to take a shortcut.

![Baldwin Steps on foot](docs/playtest-north-current/baldwin-stairs-63970.png) ![Manitou walking bridge](docs/playtest-relay-island-current/manitou-bridge-82748.png)

Original 160 × 144 frames from the retained `7ab28…` ROM. The [North](docs/NATIVE_NORTH_CURRENT_CONTINUATION.json) and [relay/Island](docs/NATIVE_RELAY_ISLAND_CURRENT_CONTINUATION.json) records retain their exact native provenance.

## Play and install

Use the [Chromatic loading guide](docs/LOADING.md). The selected update is **`toronto-dispatch-hardware-feedback.gbc`**, 1,048,576 bytes, SHA-256 `9c155a70c0cc3d6ccec986fc0ab7ddc3a204fd04e80879ad0233926156d4b10e`. It adds slower driving/trams, four smaller pedestrian types with impact poses, a clearer car, recoverable courier injuries, easier menus and richer street scenery. See the [update and verification record](docs/HARDWARE_FEEDBACK_2026_10_04.md). Generated binaries stay outside Git.

The physical cartridge still contains the prior `8be1…` driving-only build, which the user confirmed boots and plays. Its [installation](docs/CARTRIDGE_INSTALL_2026_10_04.json), reviewed ZIP and the earlier 7ab sixteen-job evidence remain preserved separately. This new update has not been flashed.

The local cartridge bundle is **`project/build/toronto-dispatch-hardware-feedback-reviewed.zip`**. Its [independent package check](docs/HARDWARE_FEEDBACK_PACKAGE.json) verifies the exact ROM, committed source, checksums, loading instructions and licences.

| Action | Controls |
| --- | --- |
| Drive | A accelerates; left/right steer; B brakes and then reverses near rest |
| Park and walk | Start → Get out of car exits; D-pad walks, A enters near the original parked vehicle |
| Choose a vehicle | Start → Change vehicle, while stopped and permitted by the job |
| Plan a quest | Without active work, Select opens dispatch; during work use Start → Jobs. Select jumps chapters, left/right selects offers, up/down browses stops; A accepts or resumes, B returns |
| Collect / hand off | Select at each ordered marker; returning jobs finish at their final stop |
| Take transit | On foot, B at a station/terminal/platform; left/right selects, A waits/boards. At Wellesley, up switches train/bus |
| Browse the map | Start → Select, or Start → City Map; D-pad pans, A centres job/booked stop/depot, Select changes focus, B returns |
| Save / audio | Start menu; Audio cycles music + effects, effects only and silent |

Hold directions to browse Pause, jobs and transit; A selects once and B returns. Release menu-used A/B before pressing them again to accelerate or brake. A delivery result's A opens dispatch. Review every stop before accepting; arrival alone does not advance the job, so press Select at the marker. Active previews show CURRENT STOP and A resumes the existing job. An eligible incomplete vehicle-specific offer shows WRONG VEHICLE until you occupy the required vehicle.

Mainland vehicles remain parked during transit and Island walking. Trains cost $3, buses $2, Queen streetcars $3 and ferries $4 in game money. Freight and passenger jobs require driving; attempts to use transit display DRIVE FOR THIS JOB. WAIT can be cancelled with B without abandoning the contract or paying a fare. Menus pause the clock; waiting and riding use delivery time. Island offers show $8/$16/$24 required ferry budgets, with extra cash needed for optional travel and fines. If an active Island job cannot fund the return, use Start → Cancel Job, then B at the dock for scheduled $0 assistance. Abandoning pays nothing and earns no completion.

## City and jobs

Explore compressed Core, western neighbourhoods, High Park/Junction, eastern neighbourhoods, Port Lands, public Islands and Uptown Hills. The current source contains **344 buildings, 657 pedestrian routes, 104 contracts, 64 service points and nine mainland parking anchors**. Four player vehicles, wide roads, varied original buildings, roof/canopy occlusion, walking/car entry, a scrollable atlas, original audio and saved progression are implemented.

Eight job types cover parcel rounds, fragile art, express files, truck freight, transit relays, passenger rides, ordered returns and Island post. Cargo starts after pickup; damage reduces the base payment, while remaining time adds a bonus. Fast steering also reduces passenger comfort. Port Lands routes use Leslie access and authored bridges; Island deliveries use ferries and public walking paths. North includes railway underpasses, foot-only Baldwin Steps and public Casa Loma/Rosehill handoffs.

Six road vehicle kinds obey fictional signals. Small commuters, workers, backpackers and seniors walk nearby. Driving into a pedestrian shows non-graphic flight and a prone body, with carried-cargo damage, escalating fines and local police pursuit; local deaths reset when the district reloads. Vehicles can also knock down the walking courier, who recovers. Original roof details, signs, park furniture and clearer signals enrich the existing city. Aircraft and under-deck boats are scenery. Visible road traffic and paid transit schedules use separate game abstractions; geography is compressed and game service times/fares are fictional.

## Verification and remaining work

The current 9c155 ROM passes official compilation, full `make check` and [compiled resource/memory checks](docs/HARDWARE_FEEDBACK_BUILD.json). Its [fresh native run](docs/NATIVE_HARDWARE_FEEDBACK.json) completes Market Start at full condition, verifies stable cardinal driving, walking/car entry, airborne/prone humans, menu repeat/map freeze, one paid Queen trip, tram knockback/recovery and committed in-worker reset. Crowded OAM samples stay within hardware limits; physical flicker remains unverified on this update. The retained 8be1 four-job record and its physical installation retain their own identities.

Separate genuine checkpoint continuations on retained 7ab reach **16 distinct completions**, representing all eight job types. [Passenger checks](docs/NATIVE_PASSENGER_CONTINUATION.json) cover vehicle/transit restrictions and slow/fast steering comfort. [North checks](docs/NATIVE_NORTH_CURRENT_CONTINUATION.json) add three jobs, underpass driving, foot delivery and original-car recovery. [Relay/Island checks](docs/NATIVE_RELAY_ISLAND_CURRENT_CONTINUATION.json) add two jobs, nine paid rides, unpaid-WAIT cancellation, public bridge walking, map panning/freezing, original-motorcycle recovery and committed in-worker reset. [East/Port checks](docs/NATIVE_EAST_PORT_CURRENT_CONTINUATION.json) add a full-condition motorcycle round through Riverside and Danforth, then required-truck freight through Leslie to the Channel apron. Cumulative recordings include source-loaded scenes from all seven districts. Imported completions are retained as ancestry, rather than counted as newly played in each continuation.

A focused host test reproduces blocked walking after saving/resetting during car entry; near the parked car, press A to finish entering and recover. A repair and native regression remain pending.

The [testing record](TESTING.md) preserves exact build identities, older evidence, failed attempts and controller corrections. Motorcycle job 74 previously finished with one second left and needs replay at the slower speed; all 104 deadlines are unchanged. Full 104-contract play, two measured enjoyable human hours, broader handling/reward/deadline tuning, older-save imports, performance/stack, browser recovery and this update's physical installation/save/audio/flicker checks remain pending. Emulator reset verifies committed in-worker progress, rather than physical battery persistence or every latest live field.

## Develop

Select `project/project.gbsproj` with the ModRetro Chromatic plugin. Read [DESIGN.md](DESIGN.md), [DECISIONS.md](DECISIONS.md) and [ROADMAP.md](ROADMAP.md) before changing gameplay; follow [BUILD.md](docs/BUILD.md) and run `make check` from the repository root. Original code/art are MIT licensed; [distribution notices](docs/DISTRIBUTION.md) retain upstream terms.
