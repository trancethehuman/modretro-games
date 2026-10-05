# Original St. George campus source

`core_before_campus.png` is the project's own original Core background from
commit `fb589599f6411d517fe8e2af9d8c43dbf6d89144`, retained as the exact source
before the University of Toronto campus overlay. It is original MIT game art,
not downloaded map imagery, a cartridge export or a ROM.

SHA-256: `2e425c04a43227dea7b126b5dab9291617f24ac0509f1d2b5142eddb68076c7e`.

`scripts/campus_art.py` authors the campus's native cells after ordinary city
decoration. `content/campus_area.json` records compressed locations and official
factual sources. The independent `campus_protection.py` and
`tests/fixtures/campus_area_protected.json` authenticate the retained source,
current pixels, exact permitted cells, outside-city pixel identity and unchanged
native geometry/attributes. They reconstruct the old PNG for the historical
feedback guard instead of recapturing or relaxing its existing fixture.

All campus architecture, clock, UT lettering, dome and paving are original.
Official university photographs, maps, crests and logos are not game assets.
Source projections do not establish native or physical cartridge presentation.
