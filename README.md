# ModRetro Games

An open-source collection of original games for ModRetro Chromatic and Game Boy Color. Every game has its own folder, editable source, design notes, and build/testing record.

## Games

| Game | Idea | Status |
| --- | --- | --- |
| [Toronto Dispatch](games/toronto-dispatch/) | A top-down Toronto courier game with timed jobs, momentum, braking, pedestrians and scheduled transit | Native prototype: three linked districts, 80 contracts, 35 service points, 166 buildings and four vehicles |

## Start developing

1. Install the **ModRetro Chromatic** plugin from the Codex Plugins tab and attach it to the development chat. Follow the [official DevDay quickstart](https://support.modretro.com/en_us/chromatic-devday-edition-quickstart-guid-By1iOlcMg).
2. Ask it to inspect existing dependencies and prepare missing dependencies for GB Studio creation, ROM builds, and automated playtests.
3. Read [AGENTS.md](AGENTS.md) and the game's design, decisions, and roadmap. Local workflow skills are in [skills/](skills/), with Codex discovery through `.agents/skills`.
4. Select `games/toronto-dispatch/project/project.gbsproj` using the plugin. It is a genuine editable project created by the plugin; each additional game gets its own project folder.
5. Build and open the same playable emulator preview during iteration. Audit assets and run gameplay checks before preparing a cartridge build.

Toronto Dispatch builds with GB Studio CLI 4.3.2 / GBDK 4.5.0 and runs in PyBoy 2.7.0. The working source links compressed central Toronto, Parkdale/Roncesvalles and High Park/Swansea/Junction. Each scene is 1,024 × 976 pixels, arranged in a logical 3,072 × 976 atlas. Walking/car entry, momentum and braking, wide roads, collision/roof occlusion, existing core paid transit, original audio and saved progression remain part of the prototype. Its 358 fixed pedestrian routes support six nearby walkers in the loaded district. Eight added western package jobs unlock through progression, including a park-and-walk delivery near Colborne Lodge.

The current native build passes first-delivery and held-acceleration turning checks, both directions of central/west and west/HighPark travel, parked-car recovery and saved restarts into the correct district. All western contracts, remaining seam lanes, crowded-scene performance, full former-Toronto coverage, a measured two-hour campaign and physical cartridge checks remain pending. Western TTC corridors are researched but do not yet add scheduled native services. Preview/browser state also needs confirmation. See [build instructions](games/toronto-dispatch/docs/BUILD.md), [build-specific evidence](games/toronto-dispatch/TESTING.md), [western geography and original layout](games/toronto-dispatch/docs/WEST_DISTRICT.md), and the broader [Old Toronto expansion proposal](games/toronto-dispatch/docs/OLD_TORONTO_EXPANSION.md). The proposed 2026 map era has not been adopted.

![Native Roncesvalles street](games/toronto-dispatch/docs/screenshots/roncesvalles.png) ![Native High Park local map](games/toronto-dispatch/docs/screenshots/high-park-map.png)

Unmodified emulator frames from the linked-district ROM; [provenance](games/toronto-dispatch/docs/screenshots/provenance.json).

## Repository checks

Use Python 3.10+, Make and a C compiler supporting AddressSanitizer/UBSan (Clang or GCC):

```sh
python3 -m pip install --requirement requirements-dev.txt
make check
```

This validates source references, native campaign/text integration, registered PNG/palette/collision consistency, pixel tile budgets, map-bank sizes, reciprocal seam lanes, traffic/pedestrian paths and client connectivity. Generator checks detect stale native headers and route-planning snapshots. Behavioral fixtures use the real C engine with host hardware stubs, including turning, curb contact, scene changes, clock gaps and interrupted saves. These checks do **not** compile or playtest a ROM. GitHub Actions is configured to install pinned Pillow and run the same checks.

## Playing on a cartridge

See [Toronto Dispatch loading instructions](games/toronto-dispatch/docs/LOADING.md) and [docs/HARDWARE.md](docs/HARDWARE.md). The official workflow supports emulator preview, streaming an emulator to a connected Chromatic, and writing a built homebrew ROM to the writable development cartridge. These are separate verification steps. Developer Mode activation is required for the included DevDay cartridge; never commit or share the activation code.

A playable city ROM has been built and tested in the native emulator. No device stream or cartridge write has been attempted. Hardware testing will be recorded per game after the device and supported cartridge are connected.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Add new games under `games/<game-slug>/`; keep unrelated games independent. Original code and assets are MIT licensed. Starter artwork and upstream metadata retain their MIT notices. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for external tools and data terms.

This is an independent homebrew project, with no affiliation with ModRetro, Nintendo, Uber, the City of Toronto, or the TTC.
