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
- The completion bitmap reserves capacity for 128 contracts, with 80 authored at this stage. Eight progression-gated western package routes and eight clients are appended after the unchanged 72-contract/27-stop prefix. The Colborne service flag requires an on-foot handoff, even when a parked car is nearby; its Queensway parking approach keeps the intended last-mile walk short. A signed-return description continues to mean an ordered return route, with no new signature system.
- The route pool contains 358 collision-validated paths, with six nearby pedestrian actors in the loaded scene. A banked selector stores six identities and a 24-byte coordinate cache in RAM; each western scene also has six authored closed traffic loops. Rendered actors remain bounded as the world grows.
- Version-6 alternating SRAM records store district-qualified player/parked-car positions and the larger completion bitmap. Valid version-5 state migrates into the expanded core layout; the older version-4 migration still keeps earnings/completions while retiring obsolete active work. CRC, last-byte commit and interrupted-write checks remain part of the host suite; physical cold boot/power-loss proof is pending.
- Registered PNGs, native palette/priority bytes and collision arrays must agree. Repository checks validate actual source pixel budgets, full overlapped vehicle tiles, accepted seam lanes, western clients, traffic/NPC paths and deterministic native content/headers. These checks remain separate from ROM execution evidence.

Prototype 3 is published with verified native boot, first delivery, held-acceleration turning, core/west driving, west/High Park walking in both directions, remote parked-car recovery, return driving, western active-job reset and a paid core subway trip. These are sampled routes, not every seam or western contract; [TESTING.md](TESTING.md) owns exact build evidence. Full former-Toronto coverage, measured two-hour gameplay, western scheduled TTC services, crowded-scene performance, a confirmed current browser preview and physical cartridge checks remain open. The 2026 era is still a proposal.

## Implemented eastern expansion and banked world navigation, 2026-10-02

- Four compressed native scenes now compile and load: core, west, High Park and Riverside/Riverdale/Leslieville/western Danforth. Each remains 1,024 × 976 pixels, in a logical 4,096 × 976 atlas. The total is 211 building footprints (`80+37+49+45`), 486 fixed pedestrian routes (`102+128+128+128`) with six nearby actors, and 18 closed traffic loops across the three added scenes. These counts describe bounded local resources, not full Toronto coverage or simultaneous off-screen simulation.
- Three new reciprocal pairs connect core/east through Bloor/Danforth, Dundas and Queen, for 14 pairs total. Broadview and the Don remain in the retained core. Gerrard is conditional on a core east-bank label/topology correction and stays closed; King/Front/Lake Shore add no new bridge. Pape's documented pedestrian rail connection remains car-blocked, with vehicle detours through researched road crossings. Original compressed pixels and selected City facts are distinguished in [EAST_DISTRICT.md](docs/EAST_DISTRICT.md) and `east_art.json`.
- Eight eastern package routes and eight fictional clients are appended after the preserved 80-contract/35-stop prefix, producing 88/43. Existing contract kinds support fragile venue cargo, motorcycle express, truck stock, relay/park mail, returns and a cross-city scooter round. Withrow/Greenwood handoffs require walking from short parking approaches. No new cargo animation, signature UI, operating future station or artificial loading delay is implied. Final-candidate ordinary-button play completes eastern contracts 81, 82, 83 and 85 alongside five original jobs; remaining eastern/western objectives and duration measurement stay pending.
- Names, portals, district graph traversal and non-core traffic tables move into a banked world module. Routing selects the fewest feasible district hops for the current travel mode, then approach distance; it is not street-level route optimisation. The beacon/HUD show the next district, refresh when entering/leaving the car, and show `NO ROAD ROUTE` when driving has no feasible district connection. Local foot-only handoffs retain `PARK THEN WALK`; the mode refresh is not a new global transit menu.
- The module adds a 36-byte six-vehicle target sample cache and one next-district byte, 37 runtime bytes total. The existing 24-byte pedestrian coordinate cache, eight VM contexts and 58-byte version-6 save format remain unchanged. The final candidate's linked memory guard passes with heap `D934`, stack `DF00` and 1,484 bytes of reserve. A native High Park reset restores nine completions, cash, clock, the courier on foot and the car parked in the west. This sample and host interrupted-write tests do not establish physical persistence or every deepest call path.
- A banked auxiliary getter supplies road parking anchors for Colborne 34, Withrow 36 and Greenwood 41. Drivers target the approach; exiting restores the actual client coordinates immediately, with no changed client record or save field. Native Withrow play verifies `(224,144)` parking cue → `(320,144)` foot handoff, completion and re-entry. Colborne/Greenwood still require native handoff samples.
- Transit selection and waiting use one two-second departure-window function. The choice screen displays its countdown. Confirming while the current window is open boards immediately; a closed window enters waiting. Fare deduction, paid-trip save state, cancellation priority and finishing a timed-out paid ride at its destination remain part of the state rules. This refactor changes no route names, fares or service geography and adds no scheduled eastern/western TTC service.
- The final-ROM subway sample confirms immediate open-window boarding, one fare charge (`30→27`) and arrival on foot at King while the car stays parked at Union. Its following 120-video-frame sample advances 59 rendered updates (about 29.5 per second); an OAM snapshot has 12 visible sprites, peak four per scanline and no over-limit scanlines. This is bounded core evidence, not crowded-world performance or paid-ride reset proof.
- Source/host checks cover the four registered resources, exact PNG/attributes/collision agreement, tile budgets, reciprocal lanes, actor routes, client connectivity, appended content and banked world behaviour. Eastern art uses 85 raw / 63 flip-canonical tiles under the 320 target. These are separate from compilation, native execution, public release and hardware evidence.

Final candidate `1da71ba5…` is built and sampled in the native emulator; `7a299125…` is an earlier four-scene build before the parking-anchor fix. Automated ordinary inputs complete nine distinct contracts (01, 02, 03, 04, 07, 81, 82, 83 and 85), load all four scenes, cover the three core/east approaches, drive core-to-west, walk west-to-High Park, pan the local map with player/clock frozen and repeat the held-turn regression at speed 24. The remote reset preserves that progression. This is about eight minutes of purposeful native gameplay, not a full campaign, two-hour acceptance or human review.

Prototype 4 identifies this four-scene milestone; Prototype 3 remains a separate historical release. [BUILD.md](docs/BUILD.md) records binary identity and linked memory; [TESTING.md](TESTING.md) owns exact scenarios. Remaining seam lanes/jobs and final paid-ride reset, full former-Toronto/waterfront/Islands, at least two hours of varied play, an adopted map era, new TTC services, crowded-scene performance, browser state and hardware checks remain pending.

## Browsable city atlas, 2026-10-02

The paused map now uses the four registered collision grids in geographic order High Park → west → core → east, with their authoritative offsets. At 1:8 scale the combined schematic is 512 × 122 pixels, padded to 64 × 16 native tiles. It shows actual road/walking ground and authored blocked water; roofs do not imply a drivable route. Its 160 × 96 viewport pans across districts without changing the loaded gameplay scene. This replaces camera panning over a single district.

Player, vehicle and objective markers retain district identities. The objective uses the existing road parking anchor while driving and actual client after exiting. Free-roam points to the Union depot; paused no-job waiting/riding points to the booked transit stop. Select cycles useful focus points; A centres the objective and B/Start can leave an incomplete repaint. No fare, progress, save schema, route, client or mission rule changes.

The renderer shares the existing 360-byte text cache, reserves CGB bank-1 ground tiles16–187, marker tiles8–14 and font192–240, and adds28 bytes of transient WRAM. Its172-slot double-hash dictionary avoids a linear search through every uploaded pattern; odd strides1–31 visit every slot, with bounded probing. The API/data remain autobanked. Linked budgets, host sanitizer results and native framebuffer/timing/state checks belong in [CITY_MAP.md](docs/CITY_MAP.md), [BUILD.md](docs/BUILD.md) and [TESTING.md](TESTING.md), each scoped to its exact build.

This adds navigation to the existing four areas. Full former Toronto, waterfront and fuller Islands, at least two measured hours of varied play, human handling/audio review and physical cartridge checks remain required.

## Queen 501 scheduled source milestone, 2026-10-02

The user already accepted autonomous paid transit and strategic alighting. The current implementation adds eight researched representative Queen platforms across west/core/east while retaining the four existing areas, all 88 contracts,43 original client/station records and 58-byte version-6 save format. Supplemental platforms 43–50 use original curb signs and shared directional boarding points; this is compressed game design, not surveyed TTC infrastructure or an added map era.

A banked transit module owns the service queries used by menus, runtime and saved-trip validation. Queen service4 costs three game dollars, repeats each direction every 64 world-clock seconds and has two-second boarding windows. Eastbound index`i` departs at `4*i`; westbound at `32+4*(7-i)`. Destination selection determines the direction, and a ride takes `4*abs(destination_index-origin_index)` seconds. Confirming in either open-window second boards immediately. Current construction diversions, full 501/504 coverage, TTC branding and real fares/timetables are outside this milestone. Research and source/compression distinctions are in [STREETCAR.md](docs/STREETCAR.md).

Remote paid arrivals commit an alighted state before queuing a scene; a failed queue retains the paid ride for retry. An earlier native Queen candidate completed three cross-district rides with pause/reset/map and parked-car recovery, but separate legacy-transit play exposed Union arrival onto the player's parked car. A bounded source correction retains the exact stop centre when clear, otherwise tries connected cardinal 12/18-pixel foot points clear of that car and loaded traffic; fully blocked arrivals retry without another fare. Host regressions pass. Exact final ROM `23b2a7a2…` repeats three Queen journeys with map/reset/car recovery and separately verifies subway, 94 bus, Island ferry and safe Union alighting with subsequent walking/car entry. This is the Prototype 6 milestone; these samples do not cover every platform or blocked-arrival condition. Historical results remain tied to their own ROM in [TESTING.md](TESTING.md).

Moving streetcar artwork, human feedback on strategic transit, remaining platforms/arrival conditions, full former-Toronto coverage, measured two-hour varied gameplay and physical cartridge acceptance remain open. This implementation does not adopt the proposed 2026 era.

## Implemented pickup condition lifecycle, 2026-10-02

- Cargo/comfort starts at 100 on acceptance and can decline only after the first successful pickup. Traffic, walls, curbs and fast passenger steering keep their existing carried penalties; multi-stop and return legs remain carrying until the contract ends. Deadline, collision motion, braking, fines and cooldowns still apply on the approach.
- Collision text describes braking while empty and cargo damage while carrying. Rider warnings require an occupied passenger job.
- Cold startup normalizes valid older active-stage-0 saves to 100 after CRC/semantic validation. It preserves carried damage, retired failure state, earnings, deadlines and district-qualified positions; invalid active condition remains rejected. This changes no save bytes or version.
- The native candidate and retained predecessor failure are scoped in [TESTING.md](TESTING.md). The published Prototype 6 bundle does not contain this later correction. Further quest/transit route-choice and hardware acceptance remain open.

## Performance and city presentation pass, 2026-10-06

User direction: make the game run smoothly without reducing fidelity, gameplay or richness, and improve crosswalk placement, building variety, actor/vehicle/boat design and transit presentation.

- Implemented: the native update loop now completes every frame (about 59-60 updates per second in all four districts, previously about 29). Hot paths were rewritten as table lookups and small hand-written SM83 routines with C references kept for host tests. Gameplay is unchanged: a differential host fuzz compares the new and original engines step for step, and an emulator checker compares every assembly routine call against a model of its C reference.
- Implemented: a new original actor sheet (136 frames) with eight CGB sprite palettes: orange courier car and uniform, red/blue traffic, yellow taxi and job pin, teal/violet pedestrians and red-and-white TTC vehicles. Transit frames are drawn at scale: bus 40 px, streetcar 64 px (limited by ten sprites per scanline) and ferry 16 x 40 px.
- Implemented: crosswalks are derived from road geometry. Every junction square is rebuilt from the drivable shape so sidewalks turn the corner instead of running across the crossing road, and zebra bars are painted only on arms that link two sidewalks. Lane dashes stop short of junctions.
- Implemented: buildings gain seeded rooftop equipment (water tanks, HVAC, skylights, solar, bulkheads, gravel, antennas); empty walkable lots get lawns, canopy trees, plazas or striped parking. Background palette slot 6 becomes parks and trees (`E7DECC/8FB56A/4D7A52/172B38`, previously a tan ground tone); the wide-warehouse style uses the terracotta slot instead. No collision value changed in any district.
- Implemented: closing the city map restores the scene's CGB bank-1 background tiles that the atlas borrows.

## Working defaults and pending proposals

- Working title: **Toronto Dispatch**.
- First district: a compressed downtown area connecting Union Station, St. Lawrence Market, and the Distillery District.
- Fictional dispatch company, fictional pickup businesses, and original landmark artwork.
- One car and three package jobs for the first playable milestone, followed by the other vehicles and passenger jobs.
- Straight north-up pixel artwork and matching native collision grid, with CGB background priority for roofs and canopies.
- Implemented controls: left/right steer the vehicle; A accelerates; B brakes and reverses near rest; Select interacts at pickup/drop-off; Start pauses. On foot, the D-pad walks, A enters the nearby parked car and B opens transit. Handheld comfort still needs human playtesting.
- A documented baseline transit map rather than changing live detours. The map era is not yet selected.
- The [researched expansion plan](docs/OLD_TORONTO_EXPANSION.md) proposes 17 linked native districts and a 2026 map baseline. That full layout remains a proposal; the historical three-scene prototype and four-scene milestone use their own compressed layouts. The user has not adopted the proposed era; full Old Toronto, its waterfront and Islands are the accepted scope.

## Unresolved implementation questions

- The native TORONTO scene extension builds and runs. Continue tuning driving, occlusion, input responsiveness and crowded-scene performance on the handheld.
- Remaining district seams and western/eastern handoffs, final-candidate paid-transit recovery, crowded-actor performance, physical save recovery and audio mix after listening. The four-scene build, memory guard, nine-job progression, Withrow parking cue and remote High Park reset have sampled native evidence; they do not complete the other acceptance gates.
- Car handling parameters, realistic traffic-rule penalties, and mission time budgets after playtesting.
- Exact hardware/cartridge edition and Developer Mode readiness.
- Island delivery transport: ferry/on-foot or specifically authorised service-vehicle jobs, consistent with researched access rules.

Material changes to accepted presentation, driving feel, or hardware target require a design discussion. Routine tuning and reversible implementation choices can proceed autonomously.
