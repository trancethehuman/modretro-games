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

## Proposed starting choices

- Working title: **Toronto Dispatch**.
- First district: a compressed downtown area connecting Union Station, St. Lawrence Market, and the Distillery District.
- Fictional dispatch company, fictional pickup businesses, and original landmark artwork.
- One car and three package jobs for the first playable milestone, followed by the other vehicles and passenger jobs.
- Straight north-up pixel artwork and matching native collision grid, with CGB background priority for roofs and canopies.
- Controls: D-pad steers toward a compass direction; A accelerates; B brakes and reverses near rest; Select interacts at pickup/drop-off; Start pauses. This mapping needs a handheld playtest.
- A documented baseline transit map rather than changing live detours. The map era is not yet selected.

## Unresolved implementation questions

- The native TORONTO scene extension builds and runs. Continue tuning driving, occlusion, input responsiveness and crowded-scene performance on the handheld.
- First-district dimensions, ROM banking, active-traffic budget, save system, and audio driver.
- Car handling parameters, realistic traffic-rule penalties, and mission time budgets after playtesting.
- Exact hardware/cartridge edition and Developer Mode readiness.
- Island delivery transport: ferry/on-foot or specifically authorised service-vehicle jobs, consistent with researched access rules.

Material changes to accepted presentation, driving feel, or hardware target require a design discussion. Routine tuning and reversible implementation choices can proceed autonomously.
