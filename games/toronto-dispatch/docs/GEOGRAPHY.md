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
