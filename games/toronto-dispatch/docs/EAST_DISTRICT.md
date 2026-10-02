# First eastern district: authored source and integration plan

Reviewed **2026-10-02**. The source PNG and collision metadata are original north-up game artwork. This document does not establish a playable fourth scene, a new transit service, full former-city coverage or measured campaign duration. Scene registration, build, native playtests and hardware checks are separate evidence. The review date does not select a 2026 game era.

The viewport continues **east of Broadview already retained in the central scene**, through Riverside, Riverdale, Leslieville and the western Danforth to Greenwood. It does not duplicate Broadview or place a second Don River east of it. The retained core owns the Don crossings; the new western approaches continue those streets after the east bank. Riverdale Park East and Don Jail remain core refinement work. The southern cutline is an unexpanded game viewport, not a real municipal border, and this milestone adds no Port Lands or Don-mouth crossing.

## Source facts and reuse

The City [Toronto Centreline](https://open.toronto.ca/dataset/toronto-centreline-tcl/) and [Former Municipality Boundaries](https://open.toronto.ca/dataset/former-municipality-boundaries/) portals declare the [Open Government Licence – Toronto v1.0](https://www.toronto.ca/city-government/data-research-maps/open-data/open-data-licence/). The metadata includes its attribution: **Contains information licensed under the Open Government Licence – Toronto.** Commercial adaptation and redistribution require attribution; official marks, third-party rights and endorsement are excluded. Preserve the notices with distributed derived facts. The parent publication step must include these datasets in `THIRD_PARTY_NOTICES.md`.

Live City API geometry was fetched in memory, rather than inferred from a portal screenshot. The road query selected 786 features in the WGS84 envelope `[-79.385,43.645,-79.310,43.711]`. Named junctions were joined by their City FROM/TO intersection IDs and ray-cast against the actual `AREA_NAME='TORONTO'`, `OBJECTID=14035041` polygon. Source hashes, parameters, selected rounded coordinates and source IDs are in `content/districts/east_art.json` under `research`. No raw official geometry, map imagery, photos, murals or logos are bundled or traced. Every pixel road polyline and facade is authored design.

All recorded Danforth/Logan/Carlaw/Pape/Jones/Greenwood, Gerrard/Logan/Carlaw/Pape, Dundas/Logan/Carlaw/Pape and Queen/Logan/Carlaw/Pape/Leslie/Greenwood junctions tested inside the former TORONTO polygon. This verifies selected geographic facts, not every invented pixel. The former municipality extends north of Danforth in some places; Danforth is not treated as its universal boundary. East York extensions and the rest of the eastern former-city footprint need subsequent districts.

The City [Prince Edward Viaduct history](https://www.toronto.ca/explore-enjoy/history-art-culture/online-exhibits/web-exhibits/web-exhibits-architecture-infrastructure/bridging-the-don-the-prince-edward-viaduct/) supports Bloor→Danforth and the older Gerrard/Queen crossings. The [Lower Don Trail project](https://www.toronto.ca/city-government/planning-development/construction-new-facilities/improvements-expansion-redevelopment/lower-don-trail/) identifies Queen, Dundas and Riverdale bridge access. Current closures are not imported. City Centreline shows Carlton ending west of the Don, while Gerrard crosses it; the route name **506 Carlton** does not make Carlton the river-crossing street. The [TTC Gerrard/Broadview stop](https://www.ttc.ca/routes-and-schedules/506/1/1082) supports that distinction.

The City [rail-bridge record](https://secure.toronto.ca/council/agenda-item.do?item=2020.EX16.5) names Eastern, Queen, Dundas, Logan, Carlaw and Gerrard rail crossings. Major-rail Centreline features support the northeast arc through the neighbourhood. Roads reopen the original pixel barrier at these represented corridors. The [historical Jones corridor record](https://www.toronto.ca/legdocs/1998/minutes/council/appa/cc980603/to6rpt.htm) supports its Danforth→Queen continuity; its historical bicycle/traffic proposal is not adopted as current turn rules.

**Pape has no invented car bridge.** Its queried road edges end on opposite sides of the railway at intersection IDs `13463403` and `13463593`, without a Pape road edge joining them. The City [2018 SmartTrack report, page 26](https://www.toronto.ca/legdocs/mmis/2018/ex/bgrd/backgroundfile-113811.pdf) describes an existing pedestrian overpass and a proposed replacement underpass. The compressed game connection remains foot-only. This is source-derived connectivity, rather than a fictional restriction or a temporary construction closure. The future station/underpass is not depicted as operating.

## Original coordinate plan

Each background is **1,024 × 976 pixels / 128 × 122 tiles / 15,616 collision cells**. Roads retain 48-pixel asphalt and an 8-pixel sidewalk on each side. Collision values are `0` road, `16` foot-only ground and `15` solid. Positions below are local game pixels, with north up; distances, widths and bends are compressed independently of geographic scale.

| Core right approach | Eastern left approach | Integration condition |
| --- | --- | --- |
| `(1000,64)` Bloor | `(24,64)` Danforth | Parent extends/open core right edge and registers reciprocal seam |
| `(1000,400)` Dundas | `(24,400)` Dundas | Same |
| `(1000,528)` Queen | `(24,528)` Queen | Same |
| `(1000,288)` currently College/Carlton | `(24,288)` Gerrard | **Conditional only**; correct core east-bank label/topology before reopening the closed border or registering a transition |

There are no King, Front or Lake Shore portals. King/Front end west of the retained Don; another bridge there would be inaccurate. Native seam source checks sweep `x=20..28` using a five-pixel car half-footprint at every integer lane offset from `−18..+18` and every foot offset from `−28..+28`. The closed conditional Gerrard connection checks only its safe inset arrival at `x=24`, and is separate from the three usable proposed seam approaches.

| Corridor | Original pixel interpretation |
| --- | --- |
| Danforth | `(24,64)→(976,64)` |
| Gerrard | `(24,288)→(672,288)→(672,256)→(976,256)` |
| Dundas | `(24,400)→(672,400)→(672,368)→(976,368)` |
| Queen | `(24,528)→(672,528)→(672,496)→(976,496)` |
| Logan / Carlaw | Columns `224 / 384`, Danforth→Eastern |
| Pape | Column `544`, northern road `64..208`, southern road `288..688`; foot connection `208..288` |
| Jones / Leslie / Greenwood | Columns `704 / 816 / 944`; Jones and Greenwood end at Queen, Leslie does not extend through to Danforth |
| Bain | `(224,208)→(544,208)`; represented Logan→Carlaw→Pape portion |
| Eastern | `(80,720)→(384,720)→(384,688)→(704,688)→(704,656)→(976,656)`; viewport endpoints, no Don crossing |

Queen, Dundas and Gerrard rise a 32-pixel cardinal step in the eastern half to express their geographic northeast trend while keeping perpendicular driving. Smaller Riverdale/Withrow residential streets are omitted. The railway uses original eight-pixel steps from the southwest cutline past Queen/Dundas west of Logan, through Gerrard/Carlaw, then northeast. It is a navigational barrier with represented road crossings and the Pape walking alternative; the pixels do not reproduce engineering grades or stairs.

## Character, landmarks and courier endpoints

Withrow Park lies between Logan and Carlaw, north of Bain: the [City park address](https://www.toronto.ca/legdocs/bills/2024/bill0369.pdf) and [former-city street context](https://www.toronto.ca/legdocs/pre1998bylaws/toronto%20-%20former%20city%20of/1993-0425.pdf) support this relationship. Its original compressed garden is `[256,104,96,72]`. [Greenwood Park](https://www.toronto.ca/city-government/planning-development/construction-new-facilities/park-facility-projects/greenwood-park-playground-improvements/) is at 150 Greenwood; its compressed garden is `[848,288,64,48]`. Park paths are invented readable service approaches, not surveyed entrances.

The [Riverside](https://www.toronto.ca/business-economy/business-operation-growth/business-improvement-areas/bia-list/bia-list-q-y/) and [Leslieville](https://www.toronto.ca/business-economy/business-operation-growth/business-improvement-areas/bia-list/bia-list-f-p/) commercial corridors help place the Queen storefront character. BIA limits are not claimed as exact residential neighbourhood boundaries. The source contains six architecture styles, varied small homes, taller blocks and broad factories, with 45 collision footprints. Original landmarks are the [Danforth Music Hall, 147 Danforth](https://www.thedanforth.com/visit), [Opera House, 735 Queen East](https://www.theoperahousetoronto.com/about), and [Ashbridge Estate](https://www.heritagetrust.on.ca/pages/sites/archaeology/see-what-weve-discovered/ashbridge-estate) at [1444 Queen East](https://www.heritagetrust.on.ca/user_assets/documents/Inventory-of-OHT-owned-properties-ENG-Sep-30-2019-FINAL.pdf). Carlaw Works is a fictional courier depot and factory. Facades and service points are original; their game entrances do not establish real access rights or business partnerships.

| Candidate key | Local point | Access |
| --- | --- | --- |
| `danforth_hall` | `(144,64)` | Road |
| `withrow_walk` | `(320,144)` | Foot-only; park at `(224,144)`, walk 96 pixels |
| `riverside_queen` | `(128,528)` | Road |
| `gerrard_pape` | `(544,288)` | Road; reachable locally even while the western Gerrard connector is closed |
| `carlaw_works` | `(384,608)` | Road |
| `leslie_queen` | `(816,496)` | Road |
| `greenwood_walk` | `(880,312)` | Foot-only; park at `(816,312)`, walk 64 pixels |
| `ashbridge_queen` | `(880,496)` | Road |

These eight fictional service candidates are source metadata, not proof of eight native jobs. Separate campaign tooling can append them while preserving existing stops. The parks provide short last-mile choices; the Pape shortcut gives walking/transit-friendly jobs a different route from vehicle jobs. There are no mandatory idle waits or duration claims.

## Source checks and remaining gates

`create_east_art.py --check` deterministically compares both source/native-destination PNG copies, attributes and metadata. It verifies 85 exact raw tile patterns / 63 flip-canonical patterns against the ≤320 target, four exact manual source shades, palette slots 0..6, blocked building footprints, and no asphalt roof priority. Six cardinal traffic loops have swept clearance for a conservative eight-pixel car half-footprint, including the last→first segment. All road clients, park parking anchors and three proposed road seams share a conservative car graph. Foot clients/ports and 128 fixed 63-pixel pedestrian walks pass actual generated collision checks. Pape's connecting tiles remain car-blocked/foot-passable.

Static source inspection is complete; it does not verify actors, occlusion, frame pacing, timing or physical cartridge execution. The parent must register matching scene/background/attributes/collision data, extend only the three truthful core seams, generate banked world routes, validate the actual four-scene resource set, build and test crossings, jobs and district/save recovery. The [TTC Line 2 station order](https://www.ttc.ca/routes-and-schedules/2/1/14908) and [Chester entrance](https://www.ttc.ca/subway-stations/chester-station) are researched future transit inputs, not added autonomous train/bus/streetcar schedules. Full Old Toronto, waterfront/Islands, a selected map era, measured two-hour play, human audio/handling review and hardware acceptance remain pending.
