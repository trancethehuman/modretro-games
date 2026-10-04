# ModRetro Games

Original open-source homebrew games for ModRetro Chromatic / Game Boy Color. Each game keeps its editable project, artwork, design and verification record in `games/<game-slug>/`.

| Game | Features | Current status |
| --- | --- | --- |
| [Toronto Dispatch](games/toronto-dispatch/) | North-up courier sandbox, seven authored Toronto districts, 104 contracts, walking/driving and scheduled transit | Right-hand traffic candidate passes a fresh three-job/pursuit/transit/reset replay. Older campaign progress remains separate; full campaign, two enjoyable human hours and hardware remain pending. |

Current editable source contains 344 buildings, 657 pedestrian routes, 64 service points, nine parking anchors and four player vehicles. Explore compressed mainland neighbourhoods, Port Lands industry/bridges, public walking Islands and Casa Loma/Summerhill/St Clair. Momentum/braking, pedestrians and police consequences, scheduled train/bus/Queen/ferry travel, a scrollable atlas, original audio and save v10 / 58-byte progression are implemented. The old 96 contracts and 59 stops remain unchanged.

![Core street traffic and courier](games/toronto-dispatch/docs/native-right-hand-traffic/occupied-police-lane.png) ![St Clair on foot](games/toronto-dispatch/docs/native-right-hand-traffic/st-clair-on-foot.png)

Original 160 × 144 emulator frames from the current `e4a9…` traffic ROM: Core at frame 7,322 and St Clair at 13,648. The [native record](games/toronto-dispatch/docs/NATIVE_RIGHT_HAND_TRAFFIC.json) retains their exact source, input, PNG and pixel hashes. These are emulator captures; historical Baldwin Steps and North campaign evidence remains separately linked in TESTING.md.

## Play or load

Read the [controls and features](games/toronto-dispatch/README.md) and [Chromatic loading guide](games/toronto-dispatch/docs/LOADING.md). Latest local playtest candidate is `toronto-dispatch-right-hand-traffic-filter.gbc`, 1,048,576 bytes, SHA-256 `e4a9fb301fd4ba6a00c58e2f8e756924d8398d6f373f7c6864cc1cdff28ac2d6`. Official build and full `make check` pass. Its [fresh native record](games/toronto-dispatch/docs/NATIVE_RIGHT_HAND_TRAFFIC.json) completes three unique deliveries, observes H1/H2/H3 consequences, police capture/return, paid Union–St Clair travel, walking/car entry, paused map controls and in-worker save restoration. [Source/build audit](games/toronto-dispatch/docs/RIGHT_HAND_TRAFFIC_BUILD_AUDIT.json) keeps lane, ABI and capacity checks separate from gameplay. Generated ROMs stay local; package details are in BUILD.md.

Menu-used A/B must be released before they accelerate or brake/reverse in the city. Steering and walking remain active normally. Dispatch keeps the full route preview, current-stop cue and A resume; Select jumps to the next eight-job chapter.

The older `22157…` [North records](games/toronto-dispatch/docs/NORTH_DISTRICT.md) and source-pinned package retain their own campaign progress. [TESTING.md](games/toronto-dispatch/TESTING.md) and [BUILD.md](games/toronto-dispatch/docs/BUILD.md) separate all exact-ROM results and failures. Full Old Toronto, all 104 played contracts, two measured enjoyable human hours, wider pacing/balance, browser recovery and physical stream/flash/cold-boot/save persistence remain unverified. The 36 traffic circuits now use right-hand lanes with separately checked police routing. Broader queue behavior remains open. No hardware installation is claimed; [published Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) is a separate older release.

## Develop

Install the ModRetro Chromatic plugin, read [AGENTS.md](AGENTS.md) and the game's design/decisions/roadmap, then select `games/toronto-dispatch/project/project.gbsproj`. Follow [native build instructions](games/toronto-dispatch/docs/BUILD.md). Shared workflows live in [skills/](skills/), also exposed at `.agents/skills`.

```sh
python3 -m pip install --requirement requirements-dev.txt
make check
```

Repository checks validate sources and host engine behavior; native builds, emulator play and physical cartridge checks are separate evidence. Add new games under `games/<game-slug>/`; see [CONTRIBUTING.md](CONTRIBUTING.md).

Original code/art are [MIT licensed](LICENSE); [third-party notices](THIRD_PARTY_NOTICES.md) preserve upstream terms. This independent project is unaffiliated with ModRetro, Nintendo, Uber, the City or TTC.
