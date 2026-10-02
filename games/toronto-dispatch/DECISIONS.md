# Decisions

## Accepted, 2026-10-01

- Original game for the user's ModRetro device and writable cartridge.
- A collection repository with one folder per game, GitHub publication, MIT licence, README, agent instructions, and reusable skills.
- Toronto courier driving with timed missions, objectives, obstacles, traffic, and open roaming.
- Angled/isometric city view: the user's clarification of “asymmetric.”
- More realistic driving: momentum, braking, and tighter traffic rules.
- Mixed courier jobs, with packages first and passenger rides later.
- Cars, trucks, motorcycles, and scooters with mission-dependent availability.
- Recognisable Toronto landmarks, neighbourhoods, street names, trains, streetcars, and industrial areas.
- Use the ModRetro Chromatic plugin and supplied official guide.

## Accepted, 2026-10-02

- Limit the city to Old Toronto (the former City of Toronto), including its waterfront and Toronto Islands; exclude the surrounding GTA.
- Research actual maps, neighbourhoods and iconic buildings before producing the game city.

## Proposed starting choices

- Working title: **Toronto Dispatch**.
- First district: a compressed downtown area connecting Union Station, St. Lawrence Market, and the Distillery District.
- Fictional dispatch company, fictional pickup businesses, and original landmark artwork.
- One car and three package jobs for the first playable milestone, followed by the other vehicles and passenger jobs.
- Isometric artwork over a ground-plane collision model, with depth ordering and an explicit compass. Avoid full 3D.
- Controls: D-pad steers toward a compass direction; A accelerates; B brakes and reverses near rest; Select interacts at pickup/drop-off; Start pauses. This mapping needs a handheld playtest.
- A documented baseline transit map rather than changing live detours. The map era is not yet selected.

## Unresolved implementation questions

- The installed plugin's available GB Studio engine extensions and whether they can support vehicle physics, angled movement, and depth sorting within cartridge limits.
- First-district dimensions, ROM banking, active-traffic budget, save system, and audio driver.
- Car handling parameters, realistic traffic-rule penalties, and mission time budgets after playtesting.
- Exact hardware/cartridge edition and Developer Mode readiness.
- Island delivery transport: ferry/on-foot or specifically authorised service-vehicle jobs, consistent with researched access rules.

Material changes to accepted presentation, driving feel, or hardware target require a design discussion. Routine tuning and reversible implementation choices can proceed autonomously.
