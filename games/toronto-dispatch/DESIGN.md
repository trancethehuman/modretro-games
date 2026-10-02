# Toronto Dispatch — design

## Player experience

You are a Toronto courier working a shift. Accept a job, collect the package, find a route, handle traffic, park, and deliver. The tension comes from route planning and controlled driving: braking early, choosing a usable lane, waiting for a streetcar, or taking a legal shortcut can outperform constant acceleration.

The city remains open between jobs. Landmarks help navigation; neighbourhoods change in building shapes, density, road width, trees, and industrial character. A compact readable city is the starting point for a larger world, not the full Toronto map on day one.

## Driving

Maintain ground-plane position, heading, signed speed, and collision footprint separately from isometric screen position. Acceleration changes speed over time. Releasing A allows rolling deceleration. B brakes; reverse becomes available only near rest. Turning has a speed-dependent limit. Trucks should require earlier braking; scooters should feel nimble but slower; motorcycles trade speed for vulnerability; cars provide the balanced baseline.

Prototype fixed-point values and a stable simulation tick after checking the engine. Diagonal motion must not be faster by accident. Rendering projects the ground plane into an angled view; input includes a clear compass so steering does not become ambiguous. Vehicle visuals and the collision footprint must agree, especially behind buildings.

Collision slows or stops the vehicle, applies a bounded time/cargo penalty, and offers recovery if stuck. Avoid rewarding repeated collisions or leaving the player trapped. Mission timeout must return the player to free roaming with a retry option.

## Traffic and environment

Traffic follows authored lanes and respects signals. Intersections have a readable signal phase. Streetcars are larger moving obstacles on researched corridors. A stopped streetcar boarding phase should teach the player to wait before the design adds stronger rule penalties. Obstacles include construction, parked vehicles, narrow passages, and crossings.

Legal route choice is the environmental advantage: connected lanes, alleys that permit the selected vehicle, avoiding queues, and timing intersections. Buildings, rails, and sidewalks are not free shortcuts. Add industrial-yard access and restricted missions after geography and collision are verified.

Passenger comfort, cargo damage, weather traction, fuel, police pursuit, and detailed traffic enforcement are future options, not requirements for the first milestone. Traffic rules need understandable feedback and tuning before they affect rewards.

## Missions

State flow: **available → accepted → pickup → carrying → delivered**. Failure and cancellation return to free roaming. Time starts at acceptance and pauses with the game. Pickup and delivery require the correct location, compatible vehicle, and a nearly stopped vehicle, plus an interaction press. Do not allow delivery through a wall or automatically complete at road speed.

The HUD shows the current objective, remaining time, and a destination cue. The map/dispatch screen identifies the relevant street and landmark. Keep text short enough for the handheld screen. Rewards reflect completion, remaining time, cargo condition, and eventually safe driving. No payments or real courier integration.

Initial mission content is in `content/missions.json`; endpoints, rewards, and seconds are proposals until map routes are measured. Vehicle definitions express qualitative intended handling, not tuned simulation numbers.

## First district

The larger world is restricted to Old Toronto (the former City of Toronto), its waterfront and Toronto Islands. Use [the researched world reference](docs/TORONTO_RESEARCH.md) before drawing geography or landmarks. Island jobs must account for ferry access and restrictions on public vehicles.

Start with a proposed downtown circuit from Union Station through St. Lawrence Market to the Distillery District, with an original depot near Union. Verify exact street links and landmark placement using the geography sources before drawing the playable collision map.

Use a compact connected map, streamed or linked in districts if the engine requires it. District transitions should preserve vehicle, speed safely, and mission state. Keep real spatial relationships and labels while compressing distance. The broader city should eventually include the waterfront, Chinatown/Kensington, west-end neighbourhoods, and the Port Lands industrial area; those areas need their own researched map plan.

Streetcars, subway stations, and railway corridors are part of the city identity. Full passenger transit simulation is not required for the courier prototype. Transit route identity must be researched separately from current service detours.

## Art and sound

Use original low-resolution pixel art: readable vehicles, warm brick blocks, glass towers, storefronts, streetcar rails, tree canopies, and distinct landmark silhouettes. Keep camera orientation consistent and make the player's vehicle visible behind tall buildings using techniques supported by the engine. Road markings, signals, mission markers, and collision boundaries must remain readable at native resolution.

The target Game Boy Color display is 160 × 144 pixels. Palette, tile, sprite, VRAM, scene, and ROM limits must be checked against the installed toolchain; do not design a desktop-resolution asset pack and scale it down later. Reserve screen space for a small HUD without hiding approaching traffic.

Add original engine, braking, collision, pickup, completion, and streetcar audio once the gameplay loop is stable. Music must leave sound effects intelligible and permit mute/volume decisions appropriate to hardware.

## Engine feasibility before production

Use the ModRetro plugin's GB Studio workflow. A tiny proof must establish angled backgrounds, ground-plane vehicle control, collision, and depth ordering. Standard exploration scenes may need an engine extension for this combination. Inspect supported extension hooks and build/resource limits before committing to a whole city. If the chosen workflow cannot support the accepted design, present the tested constraint and viable options to the user.

Do not claim the concept or content scaffold is playable. ROM compilation, emulator gameplay, live streaming, and physical cartridge boot each need separate evidence.
