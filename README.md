# ModRetro Games

Original open-source homebrew games for ModRetro Chromatic / Game Boy Color. Each game keeps its editable project, artwork, design and verification record in `games/<game-slug>/`.

| Game | Features | Current status |
| --- | --- | --- |
| [Toronto Dispatch](games/toronto-dispatch/) | North-up courier sandbox, seven authored Toronto districts, 104 contracts, walking/driving and scheduled transit | Menu-input playtest candidate passes a fresh one-job control/reset replay. Older North campaign progress remains separate; full campaign, two enjoyable human hours and hardware remain pending. |

Current editable source contains 344 buildings, 657 pedestrian routes, 64 service points, nine parking anchors and four player vehicles. Explore compressed mainland neighbourhoods, Port Lands industry/bridges, public walking Islands and Casa Loma/Summerhill/St Clair. Momentum/braking, pedestrians and police consequences, scheduled train/bus/Queen/ferry travel, a scrollable atlas, original audio and save v10 / 58-byte progression are implemented. The old 96 contracts and 59 stops remain unchanged.

![Baldwin Steps courier approach](games/toronto-dispatch/docs/evidence/north-continued/north-continued-6183.png) ![St Clair train arrival](games/toronto-dispatch/docs/evidence/north-continued/north-continued-9674.png)

Unmodified 160 × 144 emulator frames at Baldwin Steps (6,183) and St Clair (9,674), exact North ROM `22157…`. The [fresh record](games/toronto-dispatch/docs/NATIVE_NORTH_FRESH.json) and its [genuine-checkpoint continuation/provenance](games/toronto-dispatch/docs/NATIVE_NORTH_CONTINUED.json) retain source, input, PNG and pixel hashes; these are emulator captures.

## Play or load

Read the [controls and features](games/toronto-dispatch/README.md) and [Chromatic loading guide](games/toronto-dispatch/docs/LOADING.md). Latest local playtest candidate is `toronto-dispatch-menu-input-release.gbc`, 1,048,576 bytes, SHA-256 `7ccfa3b0532b2fa2f65f5198644c8f5ee3dc59df2b2e40034a3f1af954bdbeb9`. Its official build and full `make check` pass; a fresh closed emulator record completes Market Start and checks menu-held A/B release, truck controls, map freezing, North arrival and durable SRAM reset. This exact ROM has one completed job; its [public native record](games/toronto-dispatch/docs/NATIVE_MENU_INPUT_RELEASE.json) passes 13,669 curation checks. Independent native/prose review passes 34,355 checks with zero findings; its new package remains pending.

Menu-used A/B must be released before they accelerate or brake/reverse in the city. Steering and walking remain active normally. Dispatch keeps the full route preview, current-stop cue and A resume; Select jumps to the next eight-job chapter.

The older `22157…` [North records](games/toronto-dispatch/docs/NORTH_DISTRICT.md), screenshots above and source-pinned package retain their own campaign progress. [TESTING.md](games/toronto-dispatch/TESTING.md) and [BUILD.md](games/toronto-dispatch/docs/BUILD.md) separate all exact-ROM results and failures. Full Old Toronto, all 104 played contracts, two measured enjoyable human hours, wider pacing/balance, browser recovery and physical stream/flash/cold-boot/save persistence remain unverified. The traffic-side source audit also needs correction and native verification. No hardware installation is claimed; [published Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) is a separate older release.

## Develop

Install the ModRetro Chromatic plugin, read [AGENTS.md](AGENTS.md) and the game's design/decisions/roadmap, then select `games/toronto-dispatch/project/project.gbsproj`. Follow [native build instructions](games/toronto-dispatch/docs/BUILD.md). Shared workflows live in [skills/](skills/), also exposed at `.agents/skills`.

```sh
python3 -m pip install --requirement requirements-dev.txt
make check
```

Repository checks validate sources and host engine behavior; native builds, emulator play and physical cartridge checks are separate evidence. Add new games under `games/<game-slug>/`; see [CONTRIBUTING.md](CONTRIBUTING.md).

Original code/art are [MIT licensed](LICENSE); [third-party notices](THIRD_PARTY_NOTICES.md) preserve upstream terms. This independent project is unaffiliated with ModRetro, Nintendo, Uber, the City or TTC.
