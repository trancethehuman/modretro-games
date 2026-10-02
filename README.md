# ModRetro Games

An open-source collection of original games for ModRetro Chromatic and Game Boy Color. Every game has its own folder, editable source, design notes, and build/testing record.

## Games

| Game | Idea | Status |
| --- | --- | --- |
| [Toronto Dispatch](games/toronto-dispatch/) | An isometric Toronto courier game with timed jobs, momentum, braking, traffic, and an explorable city | Editable GB Studio starter builds and boots; city and driving are next |

## Start developing

1. Install the **ModRetro Chromatic** plugin from the Codex Plugins tab and attach it to the development chat. Follow the [official DevDay quickstart](https://support.modretro.com/en_us/chromatic-devday-edition-quickstart-guid-By1iOlcMg).
2. Ask it to inspect existing dependencies and prepare missing dependencies for GB Studio creation, ROM builds, and automated playtests.
3. Read [AGENTS.md](AGENTS.md) and the game's design, decisions, and roadmap. Local workflow skills are in [skills/](skills/), with Codex discovery through `.agents/skills`.
4. Select `games/toronto-dispatch/project/project.gbsproj` using the plugin. It is a genuine editable project created by the plugin; each additional game gets its own project folder.
5. Build and open the same playable emulator preview during iteration. Audit assets and run gameplay checks before preparing a cartridge build.

The ModRetro plugin is installed. The starter has been compiled with GB Studio CLI 4.3.2 / GBDK 4.5.0 and tested with PyBoy 2.7.0 and the plugin's browser emulator. This is a labelled setup room: Toronto streets, isometric rendering, driving and missions are still planned. See the [build instructions](games/toronto-dispatch/docs/BUILD.md), [test evidence](games/toronto-dispatch/TESTING.md), and [Old Toronto research](games/toronto-dispatch/docs/TORONTO_RESEARCH.md).

## Repository checks

Python 3.10+ and Make are sufficient for scaffold validation:

```sh
make check
```

This validates game content, source references, vehicle compatibility, and repository structure. It does **not** compile or playtest a ROM. GitHub Actions runs the same check.

## Playing on a cartridge

See [docs/HARDWARE.md](docs/HARDWARE.md). The official workflow supports emulator preview, streaming an emulator to a connected Chromatic, and writing a built homebrew ROM to the writable development cartridge. These are separate verification steps. Developer Mode activation is required for the included DevDay cartridge; never commit or share the activation code.

A starter ROM has been built and tested in emulators. No device stream or cartridge write has been attempted. Hardware testing will be recorded per game after the device and supported cartridge are connected.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Add new games under `games/<game-slug>/`; keep unrelated games independent. Original code and assets are MIT licensed. Starter artwork and upstream metadata retain their MIT notices. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for external tools and data terms.

This is an independent homebrew project, with no affiliation with ModRetro, Nintendo, Uber, the City of Toronto, or the TTC.
