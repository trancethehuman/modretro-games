# Toronto Dispatch — second physical feedback batch

The user played the installed hardware-feedback ROM (`9c155a70…`) and supplied
the following 23 points. They confirmed this is the complete list and requested
implementation followed by loading a tested replacement ROM. These are accepted
requirements, not implementation or verification claims. Keep the current ROM,
source-pinned ZIP and installation evidence unchanged as the baseline.

1. **Character design looks great.** Preserve the approved courier and pedestrian
   designs while improving their activity and surrounding vehicles/world.
2. **Natural off-screen arrivals.** Pedestrians, vehicles, helicopters and planes
   must enter from outside the camera view rather than appear in its middle.
   Keep visible identities/positions continuous. Check camera scrolling, actor
   recycling, district entry, delayed updates and sprite admission separately.
3. **Quick vehicle theft.** Approach parked or occupied street vehicles and use
   A or B to interact quickly. Show an occupant being dragged out before the
   courier enters an occupied vehicle. Preserve that vehicle's location and
   make it controllable. Resolve proximity priority with transit/shops; consume
   the interaction so it cannot also accelerate, reverse or charge a fare.
4. **Larger controllable boats.** Show hulls large enough to see NPC occupants in
   harbour/river water. Board, drive and safely disembark. Respect water/shore
   collision and bridge layering; retain the parked road vehicle and mission.
5. **Realistic crossings.** Add visible pedestrian crossing stripes, stop lines
   and crossing signs at actual intersections, with readable lane relationships.
6. **Toronto accuracy.** Audit streets, districts, landmarks, waterfront, Islands,
   rail corridors and transit against dated official City/TTC sources. Preserve
   accepted Old Toronto scope. Distinguish deliberate playable compression from
   geographic facts; do not silently adopt the proposed map era/full layout.
7. **Enterable shops.** Mark a few grocery/corner shops or other buildings outside.
   Walking in must open a separate full-screen playable interior level. Test
   walking inside and returning through the door, with mission and vehicle state
   intact; a text menu alone does not meet this request.
8. **Visible transit and emergency activity.** Include obvious bus stops/buses,
   police cars, fire vehicles/firefighters and other emergency traffic. Existing
   service sprites/schedules do not by themselves prove this is visible or fun.
9. **Cars, parked vehicles and water.** Improve car silhouettes; put varied usable
   parked cars on streets. Distinguish water from asphalt with colour and animated
   ripples/wakes. Parked vehicles must support actual entry/theft.
10. **More and better cars/buildings.** Increase apparent and actual street traffic
    where hardware budgets allow, improve NPC vehicle artwork, and improve the
    city's building designs. Verify density/readability in native moving views.
11. **Realistic NPC driving.** Drivers follow lanes, traffic signals, queues,
    junction clearance and other road users continuously, without teleports.
12. **NPC social behaviour.** People approach one another, stop, converse with a
    visible animation, then resume walking. Keep identities and obstacle safety.
13. **Drive on sidewalks.** Player cars can mount and travel along sidewalks.
    Preserve solid buildings/rails/water, roof occlusion and human collisions.
    Ordinary NPC road driving still follows its lanes and traffic rules.
14. **Human impacts slow cars.** A struck person flies away and falls while the
    vehicle visibly loses momentum, even if acceleration stays held. Apply one
    consequence per impact episode, with existing non-graphic presentation.
15. **8-bit music.** Ensure original chiptune music is audible and suitably mixed
    on the new build, with working music/effects/silent options. Existing generated
    audio or PCM alone does not establish physical listening quality.
16. **Vehicle collisions.** Cars collide with other vehicles and parked bodies,
    with speed loss/impact feedback and usable separating/reverse recovery.
    Subsequent feedback requires relative-speed and weight-sensitive momentum
    transfer: a faster car should push a slower car out of the way while losing
    less speed than in a head-on collision. Both bodies move through validated
    positions; walls, other vehicles and humans still participate in collisions.
17. **Better NPC cars and parking lots.** Improve fleet designs and add recognisable
    accessible parking lots/bays with vehicles the courier can approach and use.
18. **Functional lights.** Traffic signals visibly control crossing traffic and
    pedestrians where modelled. Test red stopping, green release, safe junction
    clearance and the player's consequences; decorative poles alone are not enough.
19. **More walkers and architectural variety.** Make the streets busier through
    additional visible people, varied buildings and activities. Measure sprite,
    tile, memory and frame budgets; route counts are not simultaneous population.
20. **Improve driving simulation.** Tune momentum, traction, acceleration, braking,
    reverse and cornering together, preserving the accepted slower readable pace.
    Verify repeated turns, diagonal/cardinal travel, collisions and recovery.
21. **On-screen objective arrows.** After choosing a job, normal gameplay needs
    a clear arrow toward the active pickup/handoff/delivery or the relevant next
    district/transport approach. Update it after each stage and vehicle change;
    do not substitute the paused map or a distant hidden marker.
22. **Improve the whole game.** Keep the positive playable foundation and make
    these systems coherent: readable controls, lively city, usable routes and
    enjoyable courier/sandbox play. This is an integration/human review criterion.
23. **No turning at rest.** Holding steering must not rotate a stationary car,
    including held acceleration/brake interactions. Steering resumes with genuine
    movement; preserve braking/reverse and slow-speed manoeuvring.

## Accepted changes to earlier rules

The user subsequently expanded this same build: use GTA on Game Boy Color as
visual navigation inspiration; reduce the permanent gameplay HUD; improve menu
layout and colours; brighten parks, trees and the city; add a visible rebound
when ramming cars; add collidable, animated breakable fences, poles and street
objects; make signals easier to see; add a pursuing police helicopter; and show
police attention as stars with an actual evasion/escape mechanic. Whole-building
collapse versus structural impact damage is an open clarification. These additions
are accepted requirements and still need their own implementation/native evidence.

Reference research on 2026-10-04: the publisher's
[Nintendo GBC description](https://www.nintendo.com/en-gb/Games/Game-Boy-Color/Grand-Theft-Auto-265944.html)
confirms the handheld top-down city/traffic design. The actual GBC street screenshot
in [this handheld retrospective](https://www.criticalhit.net/sponsored-content/grand-theft-auto-a-retrospective-hand-theft-auto/)
was visually inspected in the built-in browser: clear curb edges, striped crossings,
contrasting vehicle silhouettes and bright park greenery. These are presentation
references only; no Rockstar artwork, ROM, music or text is included in the project.

Vehicle theft is now in scope. Boats are now intended to be controllable rather
than cosmetic only. Player sidewalk driving is now allowed. Stationary yaw is
now forbidden. Approved human art, north-up presentation, momentum/braking,
original branding, geographically coherent Old Toronto and the existing mission
loop remain accepted. Earlier source descriptions are historical, not new rules.

## Accepted dialogue camera direction — future work, 2026-10-04

The user accepted dialogue cutscenes with a Pokémon battle-camera composition
reference: an original over-the-shoulder courier in the foreground faces the
speaker farther into the scene. Use bright pixel art and a readable text box.
All portraits, dialogue, scene art and music remain original; Nintendo sprites
and music are not included. Dialogue composition does not change the accepted
north-up city camera or approved overworld characters.

Current shopkeepers only provide short timed greetings. A future minimal
prototype is one original keeper conversation with static background/window
portraits, A advance and B return. It must preserve button consumption, room and
courier state, mission/vehicle state and existing elapsed world-clock/audio
behavior, with separately verified tile ownership/restoration and native memory.
Further original dialogue content and the cutscene implementation remain
unimplemented. This record neither modifies the current native/assets freeze
nor establishes new-build, dialogue gameplay or hardware acceptance.

## Final street life, interface and optimization requirements

Later feedback reiterates functional lights, lane/queue/pedestrian rules, buses,
trucks and a lively city. It adds recognisable taxis and large jet flybys conveyed
mainly through large ground shadows. Preserve existing aircraft/helicopter art,
off-screen continuity and roof layering while adding these original variants.
Ordinary traffic should try to yield to the on-foot courier; safe recovery from
an existing overlap and deliberate pursuing-police consequences stay distinct.

The final optimization pass must preserve gameplay and graphical fidelity:
retain population, detail, colors and nearby physical checks. Prove skipped work
cannot affect the state and measure the exact new native ROM rather than infer
speed from host counts. Keep any slow diagnostics separate from cartridge
selection. Beginner menus need clear Start/Save/Settings/Back and sound choices,
with a control guide, consumed inputs and paused world clocks. Original music,
effects-only and all-sound-off options require separate native and physical audio
checks. These are accepted requirements, not completed-build claims.

## Implementation and verification checklist

Source integration now includes a bounded two-body collision model. Relative
speed and fictional vehicle weights control transferred impulse and retained
courier momentum. NPC cache positions really move, in at-most-one-pixel checked
steps, then visibly recover their authored route anchor before normal driving
resumes. Fast rear contacts preserve more forward speed; head-on, heavy or
blocked contacts cause stronger recoil. Corner assistance checks its entire
path against cars and scheduled trams without causing an extra ram.

Focused actual-C sanitizer tests pass: 298 ramming/corner checks, 13,275 driving
checks, 132,557 sandbox checks, 423,263 scenery checks, 75,750 shop checks and
528,284 police-planner checks. The terrain-cache comparison matches 6,526
snapshots and 3,889,496 complete field payloads. These are host/source results;
native capacity, rendering, timing, ordinary-button play and hardware remain
separate gates.

Intermediate builds remain distinct. v1 rejects an undersized stock actor pool;
v2 rejects the missing scene-count declaration. v3 is a diagnostic ROM whose
compiled boat-layout/scratch checks fail. v4 and v5 fail closed on fixed-bank
code capacity. None is selected for cartridge loading. The installed baseline
ROM and frozen loading ZIP still match their original hashes.

### Native performance diagnostic: replacement not ready

Diagnostic v7 built, but its measured gameplay-loop rate fails the performance
gate. The comparison used the same Union position `(560,720)` and ordinary-input
protocol: boot for 180 raw emulator frames, hold A for 32, release all buttons
for 32, then measure `_game_time` updates in three consecutive 120-frame windows.
The installed `9c155a70…` baseline produced **34 / 34 / 34** updates; v7 produced
**6 / 7 / 6**. Over the 360 raw frames, those results correspond to about **17.0**
versus **3.17 gameplay updates per second**. This is a native timing failure,
not evidence that the replacement is ready for cartridge installation.

The source audit found that the larger boat renderer composed hull/wake pixels
and checked water/deck clipping before rejecting off-screen cells. Intact scenery
does no rubble composition or tile upload, and the signal renderer normally only
reads its visible heads after the initial pattern upload. Boat preparation is
therefore the main candidate being addressed, rather than a verified complete
explanation of every performance cost.

The revised source rejects the complete boat bounds before reading sprite
metadata, rejects invisible cells before decoding their pixels, and calculates
water/deck masks over exact eight-pixel-aligned row intervals. It retains the
original hull/wake bounds, shoreline and bridge clipping, and sprite admission
rules. Root is building a distinct v8 candidate to measure the result. That
optimization is **source work awaiting native comparison**; no improved frame
rate, completed native acceptance or replacement cartridge boot is claimed here.
The v7 failure remains part of the diagnostic history, and the tested baseline
ROM remains the loading reference until the replacement passes its own gates.

- [ ] Natural actor arrivals, stable visibility and social pedestrian pairs.
- [ ] More visible people/traffic within compiled and runtime hardware budgets.
- [ ] Quick parked/occupied vehicle theft, extraction/entry and persistent identity.
- [ ] Tuned driving, no stationary yaw, sidewalks and physical collision response.
- [ ] Larger NPC-occupied boats with boarding/driving/safe disembarking.
- [ ] Crossings, functioning lights, bus stops and visible emergency activity.
- [ ] Better fleet/buildings, parked vehicles/parking lots and animated water.
- [ ] Official-source geographic audit and any warranted map corrections.
- [ ] Marked entrances and genuine separate playable shop interiors.
- [ ] Audible original music and gameplay objective arrows.
- [ ] Official distinct ROM build, header/memory/art checks and `make check`.
- [ ] Ordinary-button native gameplay of the new systems and existing courier loop.
- [ ] Supported exact-ROM cartridge write and separate physical boot/play review.

Record each completed item with actual evidence and remaining limits here or in
linked build/native/hardware records. Do not transfer old-ROM test acceptance to
the replacement ROM or claim all 104 contracts/two measured human hours from
this checklist alone.
