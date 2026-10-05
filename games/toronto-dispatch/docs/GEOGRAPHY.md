# Toronto geography

The accepted scope is Old Toronto (former City of Toronto), its waterfront, and the Toronto Islands. See [the map and landmark research](TORONTO_RESEARCH.md) for visually inspected maps, neighbourhood production groups, the landmark shortlist, island access constraints, and outstanding boundary checks.

The goal is a recognisable compressed city, with accurate street names, intersections, neighbourhood relationships, landmark placement, and transit identity. This is game geography, not navigation guidance.

## Sources

`content/sources.json` records official source URLs and review dates. Maps have now been visually inspected as documented in the research report; no street geometry, boundary data, or map image has been imported into the game. Coarse landmark positions are researched; precise entrances and playable road links still require verification.

- [Toronto Centreline](https://open.toronto.ca/dataset/toronto-centreline-tcl/): planned source for street topology and names.
- [Neighbourhoods](https://open.toronto.ca/dataset/neighbourhoods/): planned administrative boundary reference. Informal neighbourhood labels may differ.
- [TTC route maps](https://www.ttc.ca/routes-and-schedules/504/0): downtown and rail/streetcar maps are linked here.
- [501 Queen](https://www.ttc.ca/routes-and-schedules/501/1): reviewed route identity; service arrangements can change.

Before art production, inspect the actual data/map, confirm each proposed district's road graph and landmark locations, and add official landmark references. Maintain north/south/east/west relationships in the north-up top-down camera. Choose and document a baseline map era; current construction detours should not silently reshape the game.

## First-district proposals

Union Station, St. Lawrence Market, and the Distillery District are proposed anchors. The depot is fictional. Precise entrances, street links, lane directions, streetcar intersections, industrial boundaries, and coordinates are intentionally absent until researched.

Keep verified geometry separate from gameplay edits. Record compressed distances, omitted blocks, widened lanes, or invented access points as design changes. Narrow passages need explicit vehicle access rules; do not infer scooter permission from a path's appearance.

## Data and artwork terms

Check the applicable licence and attribution before importing datasets. The City portal pages link Open Government Licence – Toronto. TTC maps are references, not bundled game artwork. Make original landmark and vehicle sprites; do not copy official logos or commercial art. Update root `THIRD_PARTY_NOTICES.md` when external material is included.

## Hospital recovery landmark — 2026-10-05

[UHN's Toronto General directions](https://www.uhn.ca/corporate/Directions/Pages/directions_TGH.aspx)
identify its mailing address as 200 Elizabeth Street and place the hospital east
of University Avenue just south of College Street, with Elizabeth entrances
between College and Gerrard. This supports the relative hospital placement; no
map, photo, floor plan, logo or building artwork is imported. Current construction
and actual entrance restrictions do not define this fictional game access.

The original existing Core building at `(512,320)` represents a compressed game
hospital. Its new original mint-plus badge at `(512,344)` and recovery forecourt
at `(504,344)` preserve the east-University/south-College relation. The forecourt
is a fictional access point, not a surveyed Toronto General entrance. University
is Core column 480, College is row 288 and Dundas is row 400; omitted blocks and
the small facade are deliberate cartridge-scale compression.

`content/hospital.json` retains all nine bounded recovery candidates and their
complete half-three-pixel terrain checks. Six candidates are raw-clear; the
three eastward candidates at column 520 overlap the building and must be skipped.
Recovery must additionally check live vehicles and use a clear full-body position.
The badge keeps existing roof priority. This addition does not change the base
city pixels, collision grid, 64 quest stops or generated navigation goals.
Source checks do not establish native recovery or physical visual acceptance.

## University of Toronto — St. George campus, 2026-10-05

The [official 2018–19 campus map](https://my.alumni.utoronto.ca/s/731/images/editor_documents/reunion/alumnireunion2019/utsg_campus_map.pdf)
names Bloor, College, St. George, University/Queen's Park and King's College
Circle, and places Robarts in grid B2, University College in D3 and Convocation
Hall in E3. This historical map's text/grid supports relative orientation,
not a current campus access survey. Current official pages identify
[University College's Romanesque Revival exterior](https://www.fs.utoronto.ca/news/five-iconic-buildings-on-the-st-george-campus/),
[Robarts at 130 St. George](https://library.utoronto.ca/library/robarts), its
[triangular shape](https://library.utoronto.ca/space/robarts/robarts-library-stacks),
and [Convocation Hall's dome and columns](https://www.utoronto.ca/news/convocation-hall-close-u-t-carpenters-share-iconic-building-s-quirks).
All were reviewed on 2026-10-05. Official imagery and geometry are not imported.

`content/campus_area.json` records the original compressed interpretation west
of University/Queen's Park, south of Bloor and north of College. Robarts reuses
the existing `(368,96,32,40)` body, University College `(408,208,32,40)`, and
Convocation Hall `(368,208,32,40)`. UC's tower anchor `(424,208)` is north/east
of the Con Hall dome `(384,224)`; Robarts `(384,108)` is northwest. This is
architectural compression, not their real footprint sizes or entrance locations.
The entire King's College Circle and St. George street graph are not newly
simulated. The existing Royal Ontario Museum remains unchanged.

Original stone paving follows the two already reachable sidewalk forecourts,
with a short original UT sign, clock/arcade, domed rotunda and angular library
roof. Existing native palette families give cream stonework, blue roof accents
and warm autumn details. All collision and palette/priority bytes remain exact;
decorative building interiors remain solid. The source checks admit 15 added
canonical tile patterns, with final compiled/native/hardware acceptance separate.

## Scooter geography and fictional street behavior — 2026-10-05

The [City's electric-scooter page](https://www.toronto.ca/services-payments/streets-parking-transportation/transportation-projects/micromobility/electric-bicycles-e-bikes-e-scooters/),
modified 2026-08-11 and reviewed 2026-10-05, states that electric kick scooters
cannot be ridden or parked on Toronto public streets, sidewalks, paths or parks.
The user's accepted sidewalk/road scooter activity is a fictional sandbox rule.
Existing player vehicle 3 is an original fictional seated delivery scooter,
with no rental branding. Campus placement, missions and rider routes must keep
their researched orientation without implying real-world transport permission.
