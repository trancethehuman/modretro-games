# Scooters and campus delivery

The Oct5 extension makes the existing delivery scooter available as a parked
vehicle and adds independent riders to the loaded mainland streets. It uses
original empty-scooter poses while preserving all 49 preceding courier,
vehicle, beacon and armed poses. City driving remains north-up.

At Union, look on the sidewalk just southeast of the starting car. On foot,
approach the empty scooter and press A or B to enter. A accelerates, left/right
steer while moving, and B brakes or reverses near rest. Park before getting out
through Start's car action. Other mainland areas use validated curb bays;
abandoned protected vehicles retain their places. Taking a found scooter adds
one police-attention star, capped at three, under the existing vehicle-theft
rule; re-entering your own parked vehicle follows its separate owned path.

Three existing contracts require a scooter: Parkdale Art (contract 74, unlocked
after six completed jobs), West Closing Round (contract 80, after eighteen),
and Cross City Bundles (contract 88, after eighteen). The dispatch board reports
WRONG VEHICLE until the courier occupies the required vehicle. Routes, rewards,
deadlines, unlock thresholds and all 104 quest identities are unchanged. Campus
Envelopes (contract 21, unlocked after twelve jobs) already serves the University
area and allows any vehicle; this extension does not add or rewrite contracts.

Two independent riders use authored road and sidewalk circuits without replacing
any of the eight traffic slots or eight pedestrians. Full-pose admission takes
place outside the camera guard. They move continuously, yield to occupied bodies
and obey the authored red-light system. Their ordinary movement and crash phases
use eight active VBlank quanta, with bounded delayed-update catch-up. Menus,
dialogue, interiors and queued scene mismatches freeze them.

Faster player-vehicle contact can push a light scooter after checked terrain,
body, tram and street-object admission. Occupied impacts use the existing
non-graphic airborne/prone civilian poses and incur attention, money and carried
condition consequences once. Mass and speed transfer momentum before those
consequences; the same contact does not apply a second pedestrian slowdown.
Vacant scooters have a bounded shove and create no human-impact fine. Rider
wrecks remain while visible and retire after leaving the camera guard. Vacant
parked wrecks retain their damage flag and can be reseeded on an eligible later
scene bind; protected owned/abandoned vehicles are not replaced. This transient
street state does not enlarge or alter the v11 / 58-byte career save.

The parked/rendering fixtures and dedicated actual-C suite validate existing
fleet preservation, full-body blockers, entry, signed reverse impacts, red
lights, equal elapsed-time partitions and modal/scene freezing. The immutable
art extension proof reconstructs R6's exact preceding assets and rejects changes
outside the two approved one-OBJ empty poses. Native build, play and cartridge
evidence are recorded separately in [TESTING.md](../TESTING.md).

University College, Convocation Hall and Robarts are original compressed campus
landmarks. Their orientation and architectural character use official UofT
references; their game footprints and forecourts are fictional. Road/sidewalk
scooter play is a sandbox choice, rather than a real-world transportation rule.
See [geography and sources](GEOGRAPHY.md).

## Matched R8 evidence

[Official R8 build](CAMPUS_SCOOTERS_BUILD.json) and full `make check` pass with
246 unchanged inputs, 532 static reserve bytes, HOME 207 bytes free, Core
backgrounds 46/47 and compiled OBJ peak 126/128. The [fresh native record](NATIVE_CAMPUS_SCOOTERS.json)
verifies A/B Union entry, one Market Start delivery on the scooter, campus
landmark views, map freezing and committed-save reset. Native occupied/parked
scooter ramming and the three exclusive contracts remain broader play gates;
their host checks are separate. [Supported discovery](CARTRIDGE_CAMPUS_SCOOTERS_DISCOVERY_2026_10_05.json)
found no connected console, so this ROM has no cartridge-write acceptance.
