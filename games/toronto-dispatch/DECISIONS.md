# Decisions

## Accepted, 2026-10-01

- Original game for the user's ModRetro device and writable cartridge.
- A collection repository with one folder per game, GitHub publication, MIT licence, README, agent instructions, and reusable skills.
- Toronto courier driving with timed missions, objectives, obstacles, traffic, and open roaming.
- Initially angled/isometric; superseded by the user's explicit top-down revision on 2026-10-02.
- More realistic driving: momentum, braking, and tighter traffic rules.
- Mixed courier jobs, with packages first and passenger rides later.
- Cars, trucks, motorcycles, and scooters with mission-dependent availability.
- Recognisable Toronto landmarks, neighbourhoods, street names, trains, streetcars, and industrial areas.
- Use the ModRetro Chromatic plugin and supplied official guide.

## Accepted, 2026-10-02

- Limit the city to Old Toronto (the former City of Toronto), including its waterfront and Toronto Islands; exclude the surrounding GTA.
- Research actual maps, neighbourhoods and iconic buildings before producing the game city.
- Release must have buildings, roads, named streets, open-world roaming, a scrollable map and varied playable quests targeting at least two hours of gameplay. Duration is a playtest acceptance target, not inferred from quest counts.
- Train and TTC bus services run autonomously on game schedules. The courier pays a small in-game fare, boards at stops and hops off strategically closer to delivery destinations.
- Transit schedules and fares are fictional gameplay parameters; real route/station names and geographic relationships remain researched.
- Street architecture must vary in building type, footprint, height and design; include tall and wide buildings rather than repeating one block shape.

## Presentation and handling revision, 2026-10-02

- Switch to a perpendicular, direct-facing top-down city because the user found isometric driving difficult. Preserve momentum and braking.
- Widen roads, grow the usable city, vary architecture and increase building density.
- Buildings must block movement, roof edges/canopies must occlude sprites, and people must visibly walk, approach a car, enter it and drive.
- Add autonomous walking NPCs. Correct overly fast turns and abrupt speed loss during steering.

## Implemented handling and reliability tuning, 2026-10-02

- Slower speed-dependent yaw, with reverse using the magnitude of speed; opposing steering inputs cancel.
- Steering keeps acceleration and smoothed velocity. Glancing contact removes blocked-axis motion without draining scalar speed on every tick. Asphalt is now 48 pixels wide with 8-pixel sidewalks; all 80 building footprints remain unchanged, and building shadows/priority regions are clipped at the road edge.
- Collision checks every tile overlapped by the car footprint, including narrow rails. A small corner correction searches 1–6 pixels of clear lateral space only with acceleration held, no brake and forward speed at least 3. It validates bounds, candidate footprint and swept lateral clearance, with at most one correction per rendered update. Broad head-on walls remain solid and stop the car.
- Hidden pedestrians cannot slow the vehicle. Traffic stop lines use the authored junction coordinates. Autonomous traffic keeps moving while the courier waits/rides.
- Traffic motion and screen presentation are separate. Road loops and a bus proxy run continuously; six nearby pedestrian actors follow 102 fixed, collision-validated world routes and retain their routes while visible. Full TTC route geometry and moving streetcars remain future work.
- Car entry/exit checks the whole door approach against the native collision grid. Pressed actions occur once across motion substeps.
- Transit cannot interrupt an active car entry/exit. If successful A entry and B transit inputs coincide, entry takes priority and its animation finishes without opening transit.
- Starting a fresh trip with no active job clears the previous job's failure condition. An actual job deadline expiring during a paid ride still causes failure, shown after arrival at the booked destination.
- Mission/transit time counts every VBlank independently from bounded motion catch-up.
- Two alternating version-5 SRAM records, CRC16 and last-byte commit preserve a recovery snapshot. Boarding and cancellation are saved, and paid trips resume after resetting. Old version-4 records keep earnings/completions and retire changed active routes.
- Authored contracts have distinct routes/briefs and chapter progression. New Island delivery points require walking, and ferry transfers go through the mainland. Condition changes base rewards; fast passenger turns reduce comfort.
- Original City Shift music, vehicle/braking ambience and event cues use the native audio driver. The pause menu exposes music + effects, effects only and silent; the preference resets on boot. Native PCM confirms output and mode behaviour, while human listening and physical audio checks remain pending.

Native driving regression evidence covers the first job, the formerly stopping Distillery turn at speed 24, and parking/re-entry. Host regressions check collision, assistance and state rules; current build identities and results belong in [TESTING.md](TESTING.md). The observed approximately 29.5 rendered updates per second is a bounded sample; performance, handheld feel, human audio review and physical SRAM behaviour remain open. These results do not establish full Old Toronto coverage or the two-hour release target.

## Implemented first western expansion, 2026-10-02

- Three native scenes now link compressed central Toronto, Parkdale/Roncesvalles and High Park/Swansea/Junction. Each local scene remains 1,024 × 976 pixels, with a logical 3,072 × 976 atlas. Camera scrolling is local; genuine GBVM scene changes handle the authored seams.
- Preserve the original 80 core footprints and add 37 western and 49 High Park/Junction footprints, for 166 total. Original architectural styles, carhouse bays, Regency veranda/chimneys, Sunnyside colonnade and Junction brick detail keep the new districts distinct. Generic building/shadow priority is clipped at asphalt; core tree canopies and CN raised lips retain intentional depth occlusion on passable ground.
- Use official City/TTC topology and original pixel compression. College ends at Dundas, Howard Park reaches Parkside without a through car road across High Park, King bends into Roncesvalles, and rail/Gardiner barriers have explicit road or foot crossings. The [western research and seam table](docs/WEST_DISTRICT.md) record source facts, City data attribution and design choices separately.
- Eleven reciprocal seam pairs connect the three scenes, including a foot-only waterfront pair. Ordinary crossings retain local player/vehicle state, job/cargo/progression, world clock and runtime audio state. A parked car carries its own district identity; foot travel into another scene leaves it where parked. Save validation reads each district's own collision resource without changing the loaded scene.
- The completion bitmap now has capacity for 80 contracts. Eight progression-gated western package routes and eight clients are appended after the unchanged 72-contract/27-stop prefix. The Colborne service flag requires an on-foot handoff, even when a parked car is nearby; its Queensway parking approach keeps the intended last-mile walk short. A signed-return description continues to mean an ordered return route, with no new signature system.
- The route pool contains 358 collision-validated paths, with six nearby pedestrian actors in the loaded scene. A banked selector stores six identities and a 24-byte coordinate cache in RAM; each western scene also has six authored closed traffic loops. Rendered actors remain bounded as the world grows.
- Version-6 alternating SRAM records store district-qualified player/parked-car positions and the larger completion bitmap. Valid version-5 state migrates into the expanded core layout; the older version-4 migration still keeps earnings/completions while retiring obsolete active work. CRC, last-byte commit and interrupted-write checks remain part of the host suite; physical cold boot/power-loss proof is pending.
- Registered PNGs, native palette/priority bytes and collision arrays must agree. Repository checks validate actual source pixel budgets, full overlapped vehicle tiles, accepted seam lanes, western clients, traffic/NPC paths and deterministic native content/headers. These checks remain separate from ROM execution evidence.

The expanded ROM boots and central-to-west driving has been verified. Remaining reciprocal crossings, western handoffs, parked-car recovery, save/transit cases and crowded-scene performance still need native tests; [TESTING.md](TESTING.md) owns exact build evidence. Full former-Toronto coverage, measured two-hour gameplay, western scheduled TTC services, a confirmed current browser preview and physical cartridge checks remain open. The 2026 era is still a proposal, and this source milestone does not establish publication of an expanded release.

## Working defaults and pending proposals

- Working title: **Toronto Dispatch**.
- First district: a compressed downtown area connecting Union Station, St. Lawrence Market, and the Distillery District.
- Fictional dispatch company, fictional pickup businesses, and original landmark artwork.
- One car and three package jobs for the first playable milestone, followed by the other vehicles and passenger jobs.
- Straight north-up pixel artwork and matching native collision grid, with CGB background priority for roofs and canopies.
- Implemented controls: left/right steer the vehicle; A accelerates; B brakes and reverses near rest; Select interacts at pickup/drop-off; Start pauses. On foot, the D-pad walks, A enters the nearby parked car and B opens transit. Handheld comfort still needs human playtesting.
- A documented baseline transit map rather than changing live detours. The map era is not yet selected.
- The [researched expansion plan](docs/OLD_TORONTO_EXPANSION.md) proposes 17 linked native districts and a 2026 map baseline. That full layout remains a proposal; the implemented three-scene western stage is its own compressed layout. The user has not adopted the proposed era; full Old Toronto, its waterfront and Islands are the accepted scope.

## Unresolved implementation questions

- The native TORONTO scene extension builds and runs. Continue tuning driving, occlusion, input responsiveness and crowded-scene performance on the handheld.
- Remaining expanded-district native crossings, ROM/resource and crowded-actor performance, parked-car/save/transit recovery and audio mix after listening. Per-scene dimensions and the version-6 save layout are implemented choices; host checks and one native crossing do not complete the other acceptance gates.
- Car handling parameters, realistic traffic-rule penalties, and mission time budgets after playtesting.
- Exact hardware/cartridge edition and Developer Mode readiness.
- Island delivery transport: ferry/on-foot or specifically authorised service-vehicle jobs, consistent with researched access rules.

Material changes to accepted presentation, driving feel, or hardware target require a design discussion. Routine tuning and reversible implementation choices can proceed autonomously.
