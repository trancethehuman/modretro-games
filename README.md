# ModRetro Games

Original open-source homebrew games for ModRetro Chromatic / Game Boy Color. Each game keeps its editable project, artwork, design and verification record in `games/<game-slug>/`.

| Game | Features | Status |
| --- | --- | --- |
| [Toronto Dispatch](games/toronto-dispatch/) | North-up courier sandbox, seven compressed Toronto districts, 104 contracts, walking/driving and scheduled transit | Playable local candidate. Native tests reach 14 distinct completions across separate same-ROM checkpoint continuations, representing all eight job types. Full campaign, human pacing and hardware testing remain open. |

## Toronto Dispatch

Deliver packages, fragile art, freight and passengers through Toronto-inspired streets. Drive a car, truck, motorcycle or scooter with momentum and braking; park to walk to clients, or pay for scheduled trains, buses, Queen streetcars and ferries. Explore mainland neighbourhoods, Port Lands bridges, public Islands and Uptown Hills using a scrollable city atlas.

The city contains 344 original buildings, 657 pedestrian routes, 64 service points and nine mainland parking anchors. Road traffic follows fictional signals, pedestrian impacts cause non-graphic recovery and escalating police consequences, and occasional planes, helicopters and boats add movement. Paid transit uses its own game timetable; its fares and travel times are fictional.

![Baldwin Steps on foot](games/toronto-dispatch/docs/playtest-north-current/baldwin-stairs-63970.png) ![Manitou walking bridge](games/toronto-dispatch/docs/playtest-relay-island-current/manitou-bridge-82748.png)

Original 160 × 144 frames from the current `7ab28…` ROM, at frames 63,970 and 82,748 in separate native continuations. Exact provenance is in the [North](games/toronto-dispatch/docs/NATIVE_NORTH_CURRENT_CONTINUATION.json) and [relay/Island](games/toronto-dispatch/docs/NATIVE_RELAY_ISLAND_CURRENT_CONTINUATION.json) records.

Read the [game guide and controls](games/toronto-dispatch/README.md) and [Chromatic loading instructions](games/toronto-dispatch/docs/LOADING.md). The reviewed local loading bundle is `games/toronto-dispatch/project/build/toronto-dispatch-vehicle-feedback-reviewed.zip`; generated binaries stay outside Git. It contains the tested 1 MiB `toronto-dispatch-vehicle-feedback.gbc`, SHA-256 `7ab28b84c242f7f2c8f9e02338e2d81ab6d789fb1d7997d0aa33e99baadd8dc0`.

Official compilation and repository checks pass. A [fresh native run](games/toronto-dispatch/docs/NATIVE_DISPATCH_VEHICLE_FEEDBACK.json) completes four jobs; later continuations add vehicle, passenger, North, relay and Island coverage. The [testing record](games/toronto-dispatch/TESTING.md) separates exact-ROM results, retained failures and controller corrections. All 104 played contracts, two measured enjoyable human hours, wider performance/stack, browser recovery and physical stream/flash/cold-boot/save persistence remain pending. [Published Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) is an older, separate build.

## Develop

Install the ModRetro Chromatic plugin, read [AGENTS.md](AGENTS.md) and the game's design, decisions and roadmap, then select `games/toronto-dispatch/project/project.gbsproj`. Follow the [native build instructions](games/toronto-dispatch/docs/BUILD.md). Shared workflows live in [skills/](skills/), also exposed at `.agents/skills`.

```sh
python3 -m pip install --requirement requirements-dev.txt
make check
```

Repository checks validate sources and host engine behavior; native builds, emulator play and physical cartridge checks are separate evidence. Add new games under `games/<game-slug>/`; see [CONTRIBUTING.md](CONTRIBUTING.md).

Original code/art are [MIT licensed](LICENSE); [third-party notices](THIRD_PARTY_NOTICES.md) preserve upstream terms. This independent project is unaffiliated with ModRetro, Nintendo, Uber, the City or TTC.
