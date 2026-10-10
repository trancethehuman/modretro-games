# Queen streetcar research and game design

Reviewed 2026-10-02. [streetcar.json](../content/streetcar.json) specifies eight representative 501 Queen platforms across the registered West, Central and East scenes. The banked timetable, fare/destination UI, paid cross-scene travel and original curb signs are now integrated into the game source. Sampled native acceptance of exact final ROM `23b2a7a2…` passed, including safe Union alighting; this is the [Prototype 6 milestone](https://github.com/trancethehuman/toronto-dispatch/releases/tag/v0.2.0-prototype.6). This document records no moving streetcar artwork, physical cartridge result or two-hour playthrough.

The game service follows Queen's normal geographic corridor with fictional operation. The game's era remains unadopted. It deliberately omits construction diversions, most intermediate stops, directional curb arrangements and the unexpanded ends of the real route. Its Parkdale and Alton endpoints are game subset endpoints, not real TTC termini. The High Park/Queensway corridor, Humber and Neville Park do not gain a scheduled service here. Full former-Toronto coverage remains a separate target.

## Source-derived facts and current diversions

The official [501 eastbound](https://www.ttc.ca/routes-and-schedules/501/0/5157) and [501 westbound](https://www.ttc.ca/routes-and-schedules/501/1) lists identify the Queen corridor, directional stop numbers and street names. They also show today's downtown eastbound York/Adelaide/Church and westbound Church/Richmond/York pattern. The [TTC Family of Services list](https://www.ttc.ca/wheel-trans/family-of-services/Family-of-Services-Stops-and-Routes) retains Queen/Yonge identities 6861 eastbound and 3079 westbound; this establishes a reference pair, not current 501 boarding on Queen at Yonge.

Two construction facts prevent describing the game as an October 2026 transit simulation. The [Ontario Line announcement](https://www.ttc.ca/news/2023/April/TTC-streetcar-and-bus-service-changes-begin-May-1-to-accommodate-Ontario-Line-construction), dated 2023-04-27, explains the Queen closure and the Richmond/Adelaide plan; the current route descriptions confirm that pattern. The [Toronto Hydro advisory](https://www.ttc.ca/service-advisories/Service-Changes/501-301-Temporary-route-change-due-to-Toronto-Hydro-work) applies from 2026-09-21 at 21:00 until 2026-10-15 at 04:00 and diverts 501/301 via King and Shaw, with Queen replacement buses. These are recorded as excluded operations, not imported into the game schedule.

Selected facts and review dates are in [sources.json](../content/sources.json). Stop reference labels below abbreviate intersections; they are not reproduced full route tables or real boarding instructions.

## Eight original shared platforms

Native coordinates are whole local world pixels, north-up. Every platform is a sidewalk cell with collision value `16`; the same south-curb point represents both directions. This is deliberate compression. It does not reproduce the TTC's actual eastbound/westbound boarding side or surveyed platform position. Transit boarding requires the courier on foot; `reserved: 0` leaves the delivery foot-only flag unchanged.

| ID | Handheld name | District | `(u,v)` | Eastbound reference | Westbound reference |
| --- | --- | --- | --- | --- | --- |
| 43 | QUEEN PARKDALE | 1 West | `(836,556)` | Queen/Jameson 6840 | Queen/Lansdowne 6842 |
| 44 | QUEEN DUFFERIN | 0 Central | `(116,556)` | Queen/Dufferin 6834 | Queen/Dufferin 16849 |
| 45 | QUEEN SPADINA | 0 Central | `(372,556)` | Queen/Spadina 6853 | Queen/Spadina 3088 |
| 46 | QUEEN YONGE | 0 Central | `(676,556)` | Queen/Yonge 6861 | Queen/Yonge 3079 |
| 47 | QUEEN BROADVIEW | 0 Central | `(980,556)` | Queen/Broadview 3032 | Queen/Broadview 3033 |
| 48 | QUEEN SAULTER | 3 East | `(128,556)` | Queen/Saulter east side 3072 | Queen/Boulton 3031 |
| 49 | QUEEN LESLIE | 3 East | `(780,524)` | Queen/Leslie 3056 | Queen/Leslie 3057 |
| 50 | QUEEN ALTON | 3 East | `(880,524)` | Queen/Alton 15699 | Queen/Alton 3029 |

The eastbound and westbound route lists support each pair except Queen/Yonge, whose normal-corridor identities use the Family of Services reference. Parkdale consolidates different nearby directional stops: there is no asserted eastbound Queen/Lansdowne stop. Saulter/Boulton likewise becomes one Riverside game platform; neither side street is added to the road drawing. De Grassi is not presented as a TTC stop. Alton fits the Ashbridge point west of Greenwood; real Woodfield stops 15336/15337 lie east of Greenwood and are not moved into the existing Ashbridge block. Smaller real streets remain omitted at this compression.

The 43 existing delivery/station records retain their native fields. IDs 43–50 are supplemental platforms with `transit: 4`; they do not turn the Parkdale, Riverside, Leslie or Ashbridge clients into stations or change a quest route. The metadata pins the canonical seven-field original prefix and its count. The duplicate client names are avoided, and each new handheld name is ASCII and at most 18 characters.

## Ground access and route topology

The platforms sit 28 pixels south of Queen's centreline. Intersection platforms move 36 pixels away from the perpendicular road so they land on actual sidewalks. Leslie uses the west side, keeping 100 pixels between Leslie and Alton; placing it east of Leslie would leave only 28 pixels and overlap the strict `<15`-pixel stop interaction regions. All same-district platform pairs are separated by at least 30 pixels along an axis.

Short walking approaches preserve the existing roads. Parkdale connects to client 29 at `(864,528)`; Dufferin to client 8 at `(80,528)`; Spadina to client 4 at `(336,528)`; Yonge to station 13 at `(640,528)`; Riverside to client 37 at `(128,528)`; Leslie to client 40 at `(816,496)`; and Alton to client 42 at `(880,496)`. Broadview approaches its existing intersection `(944,528)`. A tile-centre search on the registered grids gives 24–64 pixels for these approaches, with road crossings still exposed to traffic. These are planning distances, not measured player times or venue access surveys. Queen/Yonge also provides a walkable transfer to the retained subway interaction point.

The local source centrelines are:

```text
West:    (836,528) → (1000,528)
Central: (24,528) → (1000,528)
East:    (24,528) → (672,528) → (672,496) → (880,496)
```

Use the registered reciprocal Queen seams `West (1000,528) ↔ Central (24,528)` and `Central (1000,528) ↔ East (24,528)`. Broadview and the Don stay in Central; the existing Queen bridge crosses the river. The East step is original orthogonal compression of Queen, not imported streetcar track geometry. No King, Front, Gerrard or southern bridge is created.

The research task read all three registered collision resources and original background PNGs. Centre and `x±4`, `y±3` samples at every platform were sidewalk `16`. An illustrative axis-aligned 31×11-pixel rectangle swept at one-pixel intervals over the three listed centrelines produced 2,033 clear samples and zero blocked samples. That checks source terrain only. It chooses no actual streetcar sprite, orientation, traffic lane, turning footprint, collision behaviour or animation. Future moving vehicles must validate their full native footprints and both direction lanes separately, including turns and scene boundaries. Scene and collision byte hashes in the metadata make this static review reproducible and require revalidation after terrain changes.

## Fictional scheduled operation

Service ID `4` uses a fare of three game dollars, charged once on boarding. The timetable is derived autonomously from the existing world clock even when the route is off-screen; game pause and the city map freeze that clock. It is not a copied TTC timetable or real fare.

For west-to-east stop index `i` from 0 to 7, the period is 64 seconds. Eastbound departures start at `4*i`; westbound departures start at `32+4*(7-i)`. Each window is two seconds. Both window seconds allow immediate boarding; a closed window waits for the next departure. Left/right selects a destination; the menu derives and labels eastbound or westbound from that choice and shows departure, duration and fare. Selecting the origin cannot board. Ride time is `4*abs(destination_index-origin_index)` seconds, at most 28. Pixel gaps deliberately vary while adjacent scheduled trips remain four seconds. The boarding window adds no extra ride second.

Source integration retains one fare across arrival, pause, scene changes and paid-trip reset; keeps the parked car's actual district; uses the booked destination; preserves active deadlines/failure behaviour; and validates the saved service/origin/destination through the same route module. Failed destination scene allocation leaves the paid trip recoverable for retry. Direction is derived from origin and target, with no added save field. The exact final ROM separately samples these rules through three Queen journeys and legacy transit; unplayed platform/condition coverage remains pending.

## Implementation and verification scope

The original seven-pixel signs are generated into West, Central and East artwork without a TTC logo or changed collision permissions. The campaign now contains 51 points: the unchanged 43 client/station records followed by the eight platforms. All 88 contracts and the 58-byte version-6 save layout remain unchanged. The banked [td_transit.c](../project/plugins/toronto-driving/engine/src/td_transit.c) supplies service membership, menu stops, labels, fares, departure countdowns and journey duration to the runtime, UI and save validation. Unknown or malformed origin encodings and invalid/self journeys cannot board; preserved Wellesley bus selection and mainland ferry transfers retain their semantics.

[Production-source transit tests](../../../scripts/test_transit.py) pass 609,452 ASan/UBSan checks against an independent oracle. They cover all origin/selection/target encodings, all valid route phases, both Queen directions, label buffers and failures, clock edges and complete 16-bit clock sweeps for representative subway/bus/ferry/Queen pairs. These host checks validate C logic, not native bank ABI, scene execution or hardware. Current engine fixtures pass 2,857 checks, including safe-alighting source rules; broader historical host/native results remain in [TESTING.md](../TESTING.md).

The earlier unpublished `f56ff75e…` ROM completed Yonge→Alton, Alton→Parkdale and Parkdale→Yonge with ordinary controls, charging three fares once each. Its sampled paid map/reset restored the trip and parked car, and its driving regression retained speed 24 at the held turn. A separate legacy journey arrived at Union on top of the player's parked car and blocked walking. That predecessor establishes scoped Queen behaviour while preserving a concrete failure; it does not establish final acceptance.

The current source keeps the stop centre when clear, otherwise searches connected cardinal 12/18-pixel points clear of the parked car and loaded traffic. If every candidate is blocked, the already-paid ride stays recoverable for retry. Remote arrivals query the destination collision resource rather than the origin's traffic cache. Host fixtures cover bounds, collision/swept access, car/traffic exclusion, blocked retry and deadline behaviour. Exact final ROM `23b2a7a2…` separately repeats three Queen journeys, paid pause/map/reset/car recovery and legacy train/bus/ferry travel. Its Union arrival at `(572,720)` clears the car at `(560,720)`; walking away and re-entry pass. This resolves the predecessor trap in a native sample. Remaining platform/condition acceptance needs further native play. Exact final build and release identities belong in [BUILD.md](BUILD.md) and [TESTING.md](../TESTING.md).

## King 504 audit and deferred coverage

The official [504 eastbound](https://www.ttc.ca/routes-and-schedules/504/0/6052) and [504 westbound](https://www.ttc.ca/routes-and-schedules/504/1/6842) descriptions distinguish 504A, linking Dundas West and Distillery through Roncesvalles, King, Sumach and Cherry, from 504B, linking Dufferin Gate and Broadview through King, Queen and Broadview. Neither branch continues along Queen through Leslieville to Alton. A future King service must preserve those branch identities rather than extend 504 across this eight-stop Queen service.

The authored West geometry already bends King into Roncesvalles at the shared Queen/Queensway junction and supports the Howard Park approach. Central King is continuous west of the Don, but its horizontal road ends before the river and has no collision-faithful King-to-Queen diagonal connection. Sumach/Cherry/Distillery Loop and the station/terminal loops need their own explicit compression. Broadview remains in Central. Routing 504 through an invented King river bridge, or quietly diverting it up Parliament, would assert unsupported normal topology.

The following are sidewalk-clear future **candidates only**, not additional platform IDs or registered 504 stops: West Roncesvalles/Howard Park `(444,388)`, West King/Queen junction `(668,636)`, Central King/Dufferin `(116,668)`, King/Bathurst `(244,668)`, King/Spadina `(300,668)`, King/Bay `(596,668)`, King/Yonge `(676,668)` and King/Parliament `(780,668)`. They passed the same centre/nearby sidewalk sample check. Exact branch service, directional pairs, complete road access and native rendering still need design and validation. In particular, the east-side Spadina proposal `(372,668)` touches a solid building tile at `x+4`, so `(300,668)` uses the clear west curb. No candidate is a surveyed TTC platform.

## Rights and evidence limits

All platform pixels, road compression, timetable values and later vehicle artwork are original game work. TTC pages supply selected factual route/stop identities and citations. The [TTC website terms](https://www.ttc.ca/transparency-and-accountability/policies/web-site-terms-and-conditions-of-use) reserve website material and branding and restrict substantial republication. No TTC map image, logo, route geometry, live feed or timetable dataset is bundled; source attribution does not grant their reuse. The City's Open Government Licence applies to the previously recorded City geographic information, not TTC artwork or branding. Retain the existing City attribution and runtime notices described in [THIRD_PARTY_NOTICES.md](../../../THIRD_PARTY_NOTICES.md).

Static metadata/access checks, source host tests, the earlier native sample and corrected final-ROM acceptance remain separate evidence from performance, human enjoyment and physical loading. The unchanged campaign still requires representative gameplay and a complete duration playthrough. Remaining platforms, schedule/arrival conditions and physical behaviour still need further build-specific evidence in [TESTING.md](../TESTING.md).
