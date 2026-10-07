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

## Core street plan from the Centreline, 2026-10-07

The core scene's streets come from the [Toronto Centreline](https://open.toronto.ca/dataset/toronto-centreline-tcl/) (v2, EPSG:2952, downloaded 2026-10-07; Open Government Licence – Toronto). Source facts and method are in [core-research.json](../content/districts/core-research.json); the compressed layout is `scripts/city_layout.py`.

- Source-derived: street order, relative spacing and extents. Segments were rotated 16.6 degrees to the downtown grid; positions are metres east of Yonge and north of Queen (Bathurst −2,075 m, Spadina −1,438, University −592, Parliament 1,247, Broadview 2,478; Bloor 2,079 m, College 1,040, Dundas 460, King −382, Front −688, rail about −820, Queens Quay about −1,200).
- Compressed design: piecewise-linear mapping between anchor streets; seam rows shared with the West and East scenes unchanged; north-south streets snapped to 64 px where possible (atlas tile budget). Minor streets are omitted (Bay, Church, Jarvis, Sherbourne, Shaw, Strachan, St George, McCaul, Richmond, Adelaide, Wellington, the Esplanade); Lake Shore folds into Queens Quay; Harbord (Hoskin) and Wellesley share one row but end at Queen's Park Crescent, and College, Carlton and Gerrard form one row; Front continues east of Parliament as Mill St.
- Streets end where they do: Ossington and Broadview at Queen, Front at Bathurst, King and Mill at the Don. The Don is crossed at Bloor, Gerrard, Dundas, Queen and the waterfront; the rail corridor passes over Dufferin, Bathurst, Spadina, Yonge and Parliament.
- Neighbourhoods are character areas looked up from real positions (not official boundaries). Landmark art is original.
- Queen's Park (parks revision, 2026-10-07): source-derived from the Centreline, University Ave ends at College and Queen's Park Crescent runs round the Legislative Building and the park, with Hoskin meeting the west arm and Wellesley the east arm, rejoining as Queen's Park to Bloor. Compressed design: the crescent is drawn about three times wider than true scale (arms 104 px apart instead of about 36) with 45-degree corners, and both side streets meet it on the shared Harbord/Wellesley row. Parks elsewhere follow their real layout and amenities, recorded with sources in the research files.

## First-district proposals

Union Station, St. Lawrence Market, and the Distillery District are proposed anchors. The depot is fictional. Precise entrances, street links, lane directions, streetcar intersections, industrial boundaries, and coordinates are intentionally absent until researched.

Keep verified geometry separate from gameplay edits. Record compressed distances, omitted blocks, widened lanes, or invented access points as design changes. Narrow passages need explicit vehicle access rules; do not infer scooter permission from a path's appearance.

## Data and artwork terms

Check the applicable licence and attribution before importing datasets. The City portal pages link Open Government Licence – Toronto. TTC maps are references, not bundled game artwork. Make original landmark and vehicle sprites; do not copy official logos or commercial art. Update root `THIRD_PARTY_NOTICES.md` when external material is included.
