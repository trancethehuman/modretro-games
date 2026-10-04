# ModRetro Games

Original open-source homebrew games for ModRetro Chromatic / Game Boy Color. Each game keeps its editable project, artwork, design and verification record in `games/<game-slug>/`.

| Game | Features | Current status |
| --- | --- | --- |
| [Toronto Dispatch](games/toronto-dispatch/) | North-up courier sandbox, seven authored Toronto districts, 104 contracts, walking/driving and scheduled transit | Northern playtest candidate reaches six growing completions across three records, with local trains, walking, reset and gateway checks. Full campaign, two enjoyable human hours and hardware remain pending. |

Current editable source contains 344 buildings, 657 pedestrian routes, 64 service points, nine parking anchors and four player vehicles. Explore compressed mainland neighbourhoods, Port Lands industry/bridges, public walking Islands and Casa Loma/Summerhill/St Clair. Momentum/braking, pedestrians and police consequences, scheduled train/bus/Queen/ferry travel, a scrollable atlas, original audio and save v10 / 58-byte progression are implemented. The old 96 contracts and 59 stops remain unchanged.

![Baldwin Steps courier approach](games/toronto-dispatch/docs/evidence/north-continued/north-continued-6183.png) ![St Clair train arrival](games/toronto-dispatch/docs/evidence/north-continued/north-continued-9674.png)

Unmodified 160 × 144 emulator frames at Baldwin Steps (6,183) and St Clair (9,674), exact North ROM `22157…`. The [fresh record](games/toronto-dispatch/docs/NATIVE_NORTH_FRESH.json) and its [genuine-checkpoint continuation/provenance](games/toronto-dispatch/docs/NATIVE_NORTH_CONTINUED.json) retain source, input, PNG and pixel hashes; these are emulator captures.

## Play or load

Read the [controls and features](games/toronto-dispatch/README.md) and [Chromatic loading guide](games/toronto-dispatch/docs/LOADING.md). Latest local northern playtest candidate is `toronto-dispatch-north-initial.gbc`, 1,048,576 bytes, SHA-256 `22157d720c8746118da93bb7c3027ef24693a2e4697118e70e5a95fecddf0d95`. Its prepared local `toronto-dispatch-north-initial.zip` passes 1,235 independent package checks; the guide selects this North ROM for local playtesting. The older six-scene `03e09…` bundle retains its own pins. The active-job board shows the current stop and resumes with A; completed car entry immediately refreshes the driving HUD. Select jumps to the next eight-job chapter. RESULT B closes the receipt without reversing while held; release and press B again to brake/reverse normally.

Official northern build, 47,502 independent compiled checks and full `make check` pass. A [fresh replay](games/toronto-dispatch/docs/NATIVE_NORTH_FRESH.json) completes jobs 0/2/1 and loads North; its [closed continuation](games/toronto-dispatch/docs/NATIVE_NORTH_CONTINUED.json) adds Baldwin Book Box, reaching four unique completions / cash 523 after fines and two train fares. Scoped checks cover both northern train directions, paused-map freezing, genuine SRAM resets, public foot clients, original-car recovery and both Core/North gateways by car and foot. Continuation curation passes 26,017 checks; independent evidence comparison passes 55,785, with 359 prose/link/privacy checks. A [separate supplement](games/toronto-dispatch/docs/NATIVE_NORTH_SUPPLEMENT.json) imports those four completions and adds only truck freight 3 and signed return 6: six growing completions / cash 455, with condition pay and funded H3 fine recorded. Its curation/independent checks pass 17,189 / 31,888. Earlier-ROM campaign progress and timing stay in [TESTING.md](games/toronto-dispatch/TESTING.md).

Full Old Toronto, all 104 played contracts, two measured enjoyable human hours, wider pacing/balance, browser recovery and physical stream/flash/cold-boot/save persistence remain unverified. [TESTING.md](games/toronto-dispatch/TESTING.md) retains detailed evidence and failures. No hardware installation is claimed; [published Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) is a separate older release.

The [North milestone](games/toronto-dispatch/docs/NORTH_DISTRICT.md) distinguishes researched geography, original compression and exact-ROM playtest scope. Its six-job campaign grows through disclosed same-ROM imports; it does not inherit the older 22-completion campaign or older timing measurements.

## Develop

Install the ModRetro Chromatic plugin, read [AGENTS.md](AGENTS.md) and the game's design/decisions/roadmap, then select `games/toronto-dispatch/project/project.gbsproj`. Follow [native build instructions](games/toronto-dispatch/docs/BUILD.md). Shared workflows live in [skills/](skills/), also exposed at `.agents/skills`.

```sh
python3 -m pip install --requirement requirements-dev.txt
make check
```

Repository checks validate sources and host engine behavior; native builds, emulator play and physical cartridge checks are separate evidence. Add new games under `games/<game-slug>/`; see [CONTRIBUTING.md](CONTRIBUTING.md).

Original code/art are [MIT licensed](LICENSE); [third-party notices](THIRD_PARTY_NOTICES.md) preserve upstream terms. This independent project is unaffiliated with ModRetro, Nintendo, Uber, the City or TTC.
