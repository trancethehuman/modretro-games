# ModRetro Games

Original open-source homebrew games for ModRetro Chromatic / Game Boy Color. Each game keeps its editable project, artwork, design and verification record in `games/<game-slug>/`.

| Game | Features | Status |
| --- | --- | --- |
| [Toronto Dispatch](games/toronto-dispatch/) | North-up courier sandbox, seven compressed Toronto districts, 104 contracts, walking/driving and scheduled transit | Playable local 8be1 candidate: four fresh native completions and clearer driving-only feedback. Retained 7ab tests reach 16 distinct completions/all eight job types; full campaign, human pacing and hardware remain open. |

## Toronto Dispatch

Deliver packages, fragile art, freight and passengers through Toronto-inspired streets. Drive a car, truck, motorcycle or scooter with momentum and braking; park to walk to clients, or pay for scheduled trains, buses, Queen streetcars and ferries. Explore mainland neighbourhoods, Port Lands bridges, public Islands and Uptown Hills using a scrollable city atlas.

The city contains 344 original buildings, 657 pedestrian routes, 64 service points and nine mainland parking anchors. Road traffic follows fictional signals, pedestrian impacts cause non-graphic recovery and escalating police consequences, and occasional planes, helicopters and boats add movement. Paid transit uses its own game timetable; its fares and travel times are fictional.

![Baldwin Steps on foot](games/toronto-dispatch/docs/playtest-north-current/baldwin-stairs-63970.png) ![Manitou walking bridge](games/toronto-dispatch/docs/playtest-relay-island-current/manitou-bridge-82748.png)

Original 160 × 144 frames from the retained `7ab28…` ROM, at frames 63,970 and 82,748 in separate native continuations. Exact provenance is in the [North](games/toronto-dispatch/docs/NATIVE_NORTH_CURRENT_CONTINUATION.json) and [relay/Island](games/toronto-dispatch/docs/NATIVE_RELAY_ISLAND_CURRENT_CONTINUATION.json) records.

Read the [game guide and controls](games/toronto-dispatch/README.md) and [Chromatic loading instructions](games/toronto-dispatch/docs/LOADING.md). The selected 1 MiB `toronto-dispatch-driving-only.gbc` has SHA-256 `8be1eff05236edfd05e475fe143f5b48e20db8d4a09c30f46e038e7d9643bdfb`. Its distinct local loading bundle is `games/toronto-dispatch/project/build/toronto-dispatch-driving-only-reviewed.zip`; generated binaries stay outside Git and earlier bundles remain preserved.

Official compilation and repository checks pass. The [fresh current-ROM record](games/toronto-dispatch/docs/NATIVE_DRIVING_ONLY_WARNING.json) completes four jobs, verifies both freight-stage warnings and committed reset. Retained 7ab continuations reach 16 distinct completions across all eight kinds and load all seven districts. The [testing record](games/toronto-dispatch/TESTING.md) keeps exact-ROM scopes and retained failures separate. Full 104-contract play, two measured enjoyable human hours, wider performance/stack, browser recovery and physical stream/flash/cold-boot/save/audio remain pending.

## Develop

Install the ModRetro Chromatic plugin, read [AGENTS.md](AGENTS.md) and the game's design, decisions and roadmap, then select `games/toronto-dispatch/project/project.gbsproj`. Follow the [native build instructions](games/toronto-dispatch/docs/BUILD.md). Shared workflows live in [skills/](skills/), also exposed at `.agents/skills`.

```sh
python3 -m pip install --requirement requirements-dev.txt
make check
```

Repository checks validate sources and host engine behavior; native builds, emulator play and physical cartridge checks are separate evidence. Add new games under `games/<game-slug>/`; see [CONTRIBUTING.md](CONTRIBUTING.md).

Original code/art are [MIT licensed](LICENSE); [third-party notices](THIRD_PARTY_NOTICES.md) preserve upstream terms. This independent project is unaffiliated with ModRetro, Nintendo, Uber, the City or TTC.
