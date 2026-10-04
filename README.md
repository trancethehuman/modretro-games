# ModRetro Games

Original open-source homebrew games for ModRetro Chromatic / Game Boy Color. Each game keeps its editable project, artwork, design and verification record in `games/<game-slug>/`.

| Game | Features | Current status |
| --- | --- | --- |
| [Toronto Dispatch](games/toronto-dispatch/) | North-up courier sandbox, six Toronto districts, 96 contracts, walking/driving and scheduled transit | Native prototype; matching emulator campaign reaches 22 unique completions and all six loaded districts. Full campaign, two enjoyable human hours and hardware remain pending. |

Toronto Dispatch contains 241 buildings, 595 pedestrian routes, 59 service points, seven parking anchors and four player vehicles. Explore compressed mainland neighbourhoods, Port Lands industry/bridges and public walking Islands. Momentum/braking, pedestrians and police consequences, scheduled train/bus/Queen/ferry travel, a scrollable atlas, original audio and save v9 progression are implemented.

![Toronto Dispatch street impact](games/toronto-dispatch/docs/screenshots/courier-clearance-visible-impact.png) ![Delivery result](games/toronto-dispatch/docs/screenshots/courier-clearance-market-result.png)

Unmodified Core emulator frames from current `e797…`; [provenance](games/toronto-dispatch/docs/screenshots/courier-clearance-provenance.json) records their exact frames and hashes.

## Play or load

Read the [controls and features](games/toronto-dispatch/README.md) and [Chromatic loading guide](games/toronto-dispatch/docs/LOADING.md). Current local ROM is `toronto-dispatch-courier-clearance.gbc`, 524,288 bytes, SHA-256 `e797f5725574248915f89945dcb1c1b59c7906c8ebe8dc1f155f97b56000374f`. On the dispatch board, Select jumps to the next eight-job chapter. RESULT B closes the receipt without reversing while held; release and press B again to brake/reverse normally.

This exact ROM's growing emulator campaign reaches [22 distinct completed quests](games/toronto-dispatch/docs/NATIVE_CAMPAIGN_TWENTY_TWO_SAMPLES.json), including all eight kinds, scheduled train/bus relays, three public Island roundtrips and west/east/Port Lands jobs. The latest continuation verifies three foot-only client handoffs, recovery of the original parked car and three Port Lands road bridges. Imported progress, test-controller corrections and route-checking scope remain explicit. A failed west-end run and successful mixed driving/walking retry are retained. Chapter/control/walker-yield/impact/map checks and a separate same-ROM checkpoint/reset record also pass.

The remaining 74 contracts, fuller Old Toronto, two measured enjoyable human hours, wider pacing/balance, browser recovery and physical stream/flash/cold-boot/save persistence remain unverified. Earlier ROM records retain their own identities in [TESTING.md](games/toronto-dispatch/TESTING.md). No hardware installation is claimed; [published Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) is a separate older release.

## Develop

Editable source adds an update-local terrain cache, separately built as `toronto-dispatch-update-terrain-banked.gbc` (`825444a0…`). Repeated [native comparisons](games/toronto-dispatch/docs/NATIVE_TERRAIN_CACHE_COMPARISON.json) observe 6.6% more completed updates while parked in Core and 3.9% more during one fixed driving sequence. These are scoped workloads, with no added persistent RAM or save bytes. This source experiment has its own gameplay record; the e797 loading bundle remains selected while active-job prompts and a car-entry HUD lag receive further polish.

Install the ModRetro Chromatic plugin, read [AGENTS.md](AGENTS.md) and the game's design/decisions/roadmap, then select `games/toronto-dispatch/project/project.gbsproj`. Follow [native build instructions](games/toronto-dispatch/docs/BUILD.md). Shared workflows live in [skills/](skills/), also exposed at `.agents/skills`.

```sh
python3 -m pip install --requirement requirements-dev.txt
make check
```

Repository checks validate sources and host engine behavior; native builds, emulator play and physical cartridge checks are separate evidence. Add new games under `games/<game-slug>/`; see [CONTRIBUTING.md](CONTRIBUTING.md).

Original code/art are [MIT licensed](LICENSE); [third-party notices](THIRD_PARTY_NOTICES.md) preserve upstream terms. This independent project is unaffiliated with ModRetro, Nintendo, Uber, the City or TTC.
