# Western Old Toronto — researched first expansion

Reviewed 2026-10-02. The first western stage preserves the existing 1,024 × 976 core and adds two backgrounds of that size: Parkdale/Roncesvalles, then High Park/Swansea/the within-boundary Junction. These are original compressed native layouts. They are not GIS projections, navigation maps, full Old Toronto coverage or a measured two-hour campaign. The proposed 2026 era remains unaccepted; stable corridors guide this stage, with live TTC diversions and changing park policies kept separate.

The machine-readable [research record](../content/districts/west-research.json) separates source facts and City intersection coordinates from authored pixel paths, clients, parks and seams. The [layout source](../scripts/west_layout.py) and [art generator](../scripts/create_west_art.py) retain editable original work. Generated `west_art.json` and `high_park_art.json` include explicit collision arrays, road/foot polylines, footprints, clients and six traffic loops per scene. The parent native workflow owns registration, scene transitions, engine integration, build and playtesting.

## Geographic scope and reuse

Use the City [Former Municipality Boundaries](https://open.toronto.ca/dataset/former-municipality-boundaries/) feature `AREA_NAME=TORONTO`, rather than today's amalgamated Toronto or an informal GTA rectangle. The live ring has 3,656 vertices. Selected City Centreline intersection points were tested against that ring in memory. Dundas/Runnymede is inside; Dundas/Jane is outside. Annette/Jane and Bloor/Jane are inside, so the Junction road stops before Jane while the lower western streets remain represented. Closed scene edges are original compression, not a surveyed trace of the municipality boundary.

Both the [Toronto Centreline portal](https://open.toronto.ca/dataset/toronto-centreline-tcl/) and former-boundary portal declare the [Open Government Licence – Toronto](https://www.toronto.ca/city-government/data-research-maps/open-data/open-data-licence/), version 1.0. It permits commercial reuse and adaptation with attribution, subject to its exclusions for third-party rights, personal information, official marks and logos. Reuse does not imply City endorsement. Blank CKAN licence fields or ArcGIS copyright fields alone are not licence evidence; the dataset portal declarations and linked licence are.

Contains information licensed under the Open Government Licence – Toronto.

This record includes selected named intersections, truncated coordinates and topology/classification facts. No raw GIS geometry or map imagery is bundled, and no official photos or branding are copied into artwork. Publication must retain the attribution and add these datasets to the collection's third-party notices. Reference documents and TTC pages inform original design; their images, text, logos and live schedules are not imported. Query parameters, retrieval dates and raw-response hashes are in the research JSON for audit; a later live response may differ.

## Road relationships that must survive compression

- College meets Dundas west of Lansdowne and ends there, apart from a small local stub. It must not become a car road through High Park. The [City intersection project](https://www.toronto.ca/services-payments/streets-parking-transportation/cycling-in-toronto/torontos-cycling-infrastructure/college-dundas-intersection-improvements/) and shared City intersection `13466813` support this topology.
- Dundas bends northwest toward Roncesvalles and Bloor, then north into the Junction. Howard Park leaves Dundas, crosses Roncesvalles and reaches Parkside. A same-named continuation is classified Trail in the queried Centreline; a street name alone does not establish car access.
- Queen, King, Roncesvalles and The Queensway meet at City node `13468436`. King bends north into Roncesvalles; Queen continues toward The Queensway. The [City Queen West study](https://www.toronto.ca/legdocs/mmis/2020/te/bgrd/backgroundfile-146519.pdf) supplies the persistent relationship, not current turn restrictions.
- Lansdowne reaches Queen and crosses below the railway south of Dundas. The [2010 City bylaw](https://www.toronto.ca/legdocs/bylaws/2010/law0828.pdf) names that underpass; this historical reference does not establish current parking rules.
- The railway/Gardiner separate Parkdale from the beach. The Beaty pedestrian/cycle bridge is a foot-only waterfront connection. Lake Shore connects between the new scenes; the southern Colborne approach supplies the researched road connection across the rail corridor. The source layouts do not add a car bridge from Parkside simply because it would shorten a delivery.
- Runnymede's southern endpoint is Morningside, not The Queensway. Swansea connections use Morningside/Ellis and the angled South Kingsway corridor instead of inventing a straight Runnymede extension.

Dufferin is retained in the core. The west scene represents streets west of that seam, with a fictional College-side client; it does not relocate Dufferin Grove or duplicate Bathurst/Ossington. Informal neighbourhood labels describe character rather than legal boundaries.

## Exact native seams

Every scene is 128 × 122 tiles, or 15,616 map cells. This stays below a single 16 KiB tilemap bank; the actual compiled tile/palette/resource placement still requires native build evidence. Roads have 48 pixels of asphalt and 8 pixels of sidewalk on each side. Portal strips extend to the image border; arrival centres are safely inset 24 pixels, with conservative car lane offsets of ±12 and sidewalk centres at ±28.

| Connection | Source pixel | Target pixel | Access |
| --- | --- | --- | --- |
| Core Bloor → west Bloor | `(24,64)` | `(1000,64)` | Vehicle and foot |
| Core College → west College | `(24,288)` | `(1000,288)` | Vehicle and foot |
| Core Dundas → west Dundas | `(24,400)` | `(1000,400)` | Vehicle and foot |
| Core Queen → west Queen | `(24,528)` | `(1000,528)` | Vehicle and foot |
| Core King → west King | `(24,640)` | `(1000,640)` | Vehicle and foot |
| West Dundas → High Park Dundas | `(24,96)` | `(1000,96)` | Vehicle and foot |
| West Bloor → High Park Bloor | `(24,240)` | `(1000,240)` | Vehicle and foot |
| West Howard Park → Parkside approach | `(24,352)` | `(1000,352)` | Vehicle and foot |
| West Queensway → High Park Queensway | `(24,640)` | `(1000,640)` | Vehicle and foot |
| West Lake Shore → High Park Lake Shore | `(24,832)` | `(1000,832)` | Vehicle and foot |
| West waterfront path → High Park path | `(24,896)` | `(1000,896)` | Foot only |

All have reciprocal transitions. The Howard Park seam lies east of Parkside. In the High Park scene its ordinary road stops at Parkside; the loop/platform inside the park is a foot/tram design area, with no ordinary through car road. Distances and north/south steps are deliberately compressed, so matching seam rows are not shared geographic latitude. The exact road paths are in the layout source and generated metadata.

## Landmark and transit references

The west scene uses an original long brick carhouse interpretation near The Queensway, with separate garage-bay details. The City identifies [Roncesvalles Carhouse at 20 The Queensway](https://www.toronto.ca/legdocs/mmis/2026/fwc/bgrd/backgroundfile-285594.pdf); the cited event document is used for the address, not an adopted event-era diversion.

High Park is a large wooded park with a pond, formal paths and original canopy art. [Colborne Lodge](https://www.toronto.ca/explore-enjoy/history-art-culture/museums/colborne-lodge/) is a Regency cottage in south High Park, north of The Queensway; its original game interpretation uses a veranda and chimneys. The [City High Park page](https://www.toronto.ca/explore-enjoy/parks-recreation/places-spaces/beaches-gardens-attractions/high-park/) describes varying vehicle restrictions and entrance direction. The game Bloor gate is a roadside service point, and the fictional Colborne client is foot-only. This design avoids implying that Bloor is a visitor car entrance or that today's park restrictions are an accepted permanent game-era policy.

The [Sunnyside Bathing Pavilion heritage record](https://secure.toronto.ca/HeritagePreservation/details.do?folderRsn=2438232&propertyRsn=140400) places it at 1755 Lake Shore Boulevard West. Its original native silhouette uses a wide colonnade with a central roof detail beside the waterfront walk. Junction commercial buildings use original brick/trim patterns inspired by the [City Junction study](https://www.toronto.ca/city-government/planning-development/planning-studies-initiatives/junction-phase-i-hcd-study/) and [2946 Dundas heritage research](https://www.toronto.ca/legdocs/mmis/2021/te/bgrd/backgroundfile-163406.pdf). No designation outcome, storefront business or copied façade is asserted.

The normal [506](https://www.ttc.ca/routes-and-schedules/506/0/3030) corridor uses College → Dundas → Howard Park → High Park Loop. The normal [504A](https://www.ttc.ca/routes-and-schedules/504/1/13874) corridor links King, Roncesvalles and Dundas West station. [505](https://www.ttc.ca/routes-and-schedules/505/0) ends at Dundas West station rather than continuing into the Junction. [Dundas West](https://www.ttc.ca/subway-stations/dundas-west-station?tab=0) and [Lansdowne](https://www.ttc.ca/subway-stations/lansdowne-station) provide station-location references. These identities/corridors do not establish new native Line 2, 501, 504, 505 or 506 schedules. Their boarding geometry and fictional service timing need a separate native milestone.

## Playable expansion and verification

Eight collision-checked fictional clients cover the College boundary, Lansdowne/Bloor, Parkdale/Queen, Roncesvalles/Howard Park, Sorauren, a roadside Bloor park gate, a south Parkside depot and Colborne service entrance. The Colborne client `(784,608)` uses parking anchor `(736,640)`, followed by a short walk. It is deliberately a park/walk contract, with no mandatory waiting to inflate duration. Bloor/Dundas versus College/Queen/King and the south park/waterfront approach create alternative routes and recovery options.

Source validation checks all overlapped tiles of a conservative 17 × 17 traffic footprint at every pixel along six loops per scene, including last-to-first segments. Every loop has at most 16 cardinal waypoints. Vehicle client/portal centres are asphalt-clear; all foot clients and portals connect. The arrays use `0` asphalt, `16` foot-only and `15` solid. Footprints, roof priority and visible road art derive from the same masks; roof/canopy priority is removed from asphalt. Source four-shade and flip-canonical tile counts remain below the 384-tile color-only budget. These are source checks, not proof of registered scene or compiled runtime behavior.

Native transition work must preserve the global player/mission/cargo/vehicle state, scene-qualified parked car, world clock, paid transit state, money, completed jobs, audio mode and save state. Reload scene resources without stale collision pointers or duplicate actors; restore camera and presentation after switching, bound speed/arrival safely, and prevent an immediate portal bounce. Test every reciprocal seam on foot and in every vehicle, during jobs and after saving/soft reset, against actual registered grids.

Remaining gates are native asset registration/build, compiled tile/palette and bank limits, every scene/seam/save/transit case, landmark colors and occlusion, crowded-frame performance and physical cartridge evidence. Northern/eastern Toronto, Port Lands and fuller Islands remain outside this western stage. Expanded coverage, eight added contracts and route estimates do not prove two hours of fun: time representative missions and a complete campaign before claiming the release duration.
