# Toronto Dispatch — design

## Player experience

You are a Toronto courier working a shift. Accept a job, collect the package, find a route, handle traffic, park, and deliver. The tension comes from route planning and controlled driving: braking early, choosing a usable lane, waiting for a streetcar, or taking a legal shortcut can outperform constant acceleration.

The city remains open between jobs. Landmarks help navigation; neighbourhoods change in building shapes, density, road width, trees, and industrial character. A compact readable city is the starting point for a larger world, not the full Toronto map on day one.

## Driving

Maintain ground-plane position, heading, signed speed, and collision footprint in a straight north-up top-down world. Acceleration changes speed over time. Releasing A allows rolling deceleration. B brakes; reverse becomes available only near rest. Turning has a speed-dependent limit. Trucks should require earlier braking; scooters should feel nimble but slower; motorcycles trade speed for vulnerability; cars provide the balanced baseline.

Prototype fixed-point values and a stable simulation tick after checking the engine. Diagonal motion must not be faster by accident. Screen and ground axes agree: east is right, north is up. Steering retains the existing left/right vehicle controls. Vehicle visuals and the collision footprint must agree, especially behind buildings.

Collision slows or stops the vehicle, applies a bounded time/cargo penalty, and offers recovery if stuck. Avoid rewarding repeated collisions or leaving the player trapped. Mission timeout must return the player to free roaming with a retry option.

## Traffic and environment

Traffic follows authored lanes and respects signals. Intersections have a readable signal phase. Streetcars are larger moving obstacles on researched corridors. A stopped streetcar boarding phase should teach the player to wait before the design adds stronger rule penalties. Obstacles include construction, parked vehicles, narrow passages, and crossings.

Legal route choice is the environmental advantage: connected lanes, alleys that permit the selected vehicle, avoiding queues, and timing intersections. Buildings, rails, and sidewalks are not free shortcuts. Add industrial-yard access and restricted missions after geography and collision are verified.

Cargo condition is part of the native contract rules. Traffic impacts, curb scrapes and head-on collisions reduce condition; fragile art takes a larger curb/wall penalty. Passenger jobs use condition as comfort, with an additional penalty for steering at high speed. Completion pays the base reward scaled by remaining condition, plus a remaining-time bonus. These current source rules still need tuning and verification in the new ROM. Weather traction, fuel, police pursuit and detailed traffic enforcement remain future options. Traffic rules need understandable feedback and tuning.

## Missions

Release target: at least two hours of varied gameplay. Build distinct job rules, route decisions and progression; do not pad the duration with mandatory idle waiting. Measure representative quest times and a campaign playthrough before claiming the target is met.

State flow: **available → accepted → pickup → carrying → delivered**. Failure and cancellation return to free roaming. Time starts at acceptance and pauses with the game. Pickup and delivery require the correct location, compatible vehicle, and a nearly stopped vehicle, plus an interaction press. Do not allow delivery through a wall or automatically complete at road speed.

The HUD shows the current objective, remaining time, and a destination cue. The map/dispatch screen identifies the relevant street and landmark. Keep text short enough for the handheld screen. Rewards reflect completion, remaining time and cargo/passenger condition, so controlled driving matters to earnings. No payments or real courier integration.

The engine-integrated campaign is in `content/campaign.json`, compiled by `scripts/create_campaign.py`. The original three design samples in `content/missions.json` are historical planning material. Vehicle definitions express qualitative intended handling, not tuned simulation numbers.

The authored revision has 72 distinct titles and routes across nine progressive chapters. Its eight rule types cover parcel rounds, fragile art, express files, truck freight, transit-friendly relays, passenger rides, signed returns and Island post. Two short native brief lines explain each contract. The first three routes teach the depot, market, gallery and Distillery circuit; later rounds connect related city destinations instead of repeating one random landmark sequence. Chapter and vehicle gates leave a path to every contract through distinct completions.

The campaign uses 27 service points. Three original fictional Island delivery entrances lie beyond the ferry docks on collision-verified walkable terrain. Island jobs require walking to those deliveries; journeys between Islands transfer through the mainland terminal. These entrances are compressed game design, not surveyed real-world access points.

Time allowances use shortest paths through the actual collision grid, native vehicle/walking speeds and allowances for pickup travel, braking and interaction. Express contracts have tighter budgets. The generator validates route/title uniqueness, handheld text limits, Island foot access and unlock closure. These checks establish content consistency and feasibility estimates; they do not measure playtime or enjoyment. The authored revision and new condition/comfort rules need a fresh native ROM build and representative playtests before their gameplay is verified. The full two-hour campaign gate remains open.

## First district

The larger world is restricted to Old Toronto (the former City of Toronto), its waterfront and Toronto Islands. Use [the researched world reference](docs/TORONTO_RESEARCH.md) before drawing geography or landmarks. Island jobs must account for ferry access and restrictions on public vehicles.

Start with a proposed downtown circuit from Union Station through St. Lawrence Market to the Distillery District, with an original depot near Union. Verify exact street links and landmark placement using the geography sources before drawing the playable collision map.

Use a compact connected map, streamed or linked in districts if the engine requires it. District transitions should preserve vehicle, speed safely, and mission state. Keep real spatial relationships and labels while compressing distance. The broader city should eventually include the waterfront, Chinatown/Kensington, west-end neighbourhoods, and the Port Lands industrial area; those areas need their own researched map plan.

Streetcars, subway stations, and railway corridors are part of the city identity. Scheduled subway, bus and ferry travel is part of the prototype. Transit route identity must be researched separately from current service detours.

## Scheduled transit

The courier can park, pay an in-game fare at a real named stop, and ride autonomous trains or TTC buses to a strategic drop-off point. Service timing advances on the game clock; the map/dispatch interface shows the next departure. Timers continue while waiting or riding, while deliberate pause freezes simulation. Vehicles remain parked and must be recovered. Heavy cargo and passenger jobs may require the assigned road vehicle; small packages can be carried on foot and transit. Bus and subway route names follow official references, but timetable frequency, travel time and fares are explicitly fictional values tuned for enjoyable route choices.

## Art and sound

Use original low-resolution pixel art: readable vehicles, warm brick blocks, glass towers, storefronts, streetcar rails, tree canopies, and distinct landmark silhouettes. Keep camera orientation consistent and make the player's vehicle visible behind tall buildings using techniques supported by the engine. Road markings, signals, mission markers, and collision boundaries must remain readable at native resolution.

The target Game Boy Color display is 160 × 144 pixels. Palette, tile, sprite, VRAM, scene, and ROM limits must be checked against the installed toolchain; do not design a desktop-resolution asset pack and scale it down later. Reserve screen space for a small HUD without hiding approaching traffic.

Add original engine, braking, collision, pickup, completion, and streetcar audio once the gameplay loop is stable. Music must leave sound effects intelligible and permit mute/volume decisions appropriate to hardware.

## Engine feasibility before production

Use the ModRetro plugin's GB Studio workflow. Native tests must establish top-down backgrounds, vehicle control, collision, walking, roof/canopy occlusion, and car entry. Standard exploration scenes may need an engine extension for this combination. Inspect supported extension hooks and build/resource limits before committing to a whole city. If the chosen workflow cannot support the accepted design, present the tested constraint and viable options to the user.

Do not claim the concept or content scaffold is playable. ROM compilation, emulator gameplay, live streaming, and physical cartridge boot each need separate evidence.

## Current implementation and tuning

The 1,024 × 976 north-up world uses 40-pixel roads plus sidewalks, six architectural styles, original landmark interpretations and Island service areas. The map is compressed central Toronto, with the wider former municipality still pending. Native tile collision blocks building footprints, water and the rail corridor; CGB tile priority hides sprites beneath raised roof lips and tree canopies. Walking shows directional steps, parking places the courier beside the door, and A animates re-entry near the parked vehicle.

Velocity eases toward steering direction, yaw advances on a dedicated speed-dependent counter, and glancing curb contact slides along the free axis. Head-on wall impacts stop the vehicle. Acceleration no longer overrides braking when both are held. Simulation compensates for skipped render frames with bounded substeps; the clock follows video frames, while menus freeze it. Handheld feel and crowded-scene performance remain playtest targets.
