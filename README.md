# ModRetro Games

Original open-source homebrew games for ModRetro Chromatic / Game Boy Color. Each game keeps its editable project, artwork, design and verification record in `games/<game-slug>/`.

| Game | Features | Status |
| --- | --- | --- |
| [Toronto Dispatch](games/toronto-dispatch/) | North-up courier sandbox, seven compressed Toronto districts, 104 contracts, walking/driving and scheduled transit | Selected sandbox-stable ROM passes official build, repository checks and scoped native play. Its cartridge write and cold boot are pending; 9c155 remains the last physically confirmed build. |

## Toronto Dispatch

Deliver packages, fragile art, freight and passengers through Toronto-inspired streets. Drive a car, truck, motorcycle or scooter with momentum and braking; park to walk to clients, or pay for scheduled trains, buses, Queen streetcars and ferries. Explore mainland neighbourhoods, Port Lands bridges, public Islands and Uptown Hills using a scrollable city atlas.

The city contains 344 original buildings, 657 pedestrian routes, 64 service points and nine mainland parking anchors. Commuters, workers, backpackers and seniors walk nearby. Road traffic follows fictional signals; pedestrian impacts show non-graphic airborne/prone poses and escalate police consequences. The walking courier can be struck and recover. Original roof details, signs, benches and planters enrich the streets; planes, helicopters and boats add movement. Paid transit uses fictional fares and timetables.

The selected build adds playable shops, controllable boats, vehicle theft, taxis, larger jet shadows and a clearer police-star display. Start opens a main menu with Save Game and Settings; Settings offers sound choices and a control guide. The existing original 8-bit song, engine/brake sounds and event cues can play together, as effects only, or with all sound off. Sound choices reset at boot.

![Baldwin Steps on foot](games/toronto-dispatch/docs/playtest-north-current/baldwin-stairs-63970.png) ![Manitou walking bridge](games/toronto-dispatch/docs/playtest-relay-island-current/manitou-bridge-82748.png)

Original 160 × 144 frames from the retained `7ab28…` ROM, at frames 63,970 and 82,748 in separate native continuations. Exact provenance is in the [North](games/toronto-dispatch/docs/NATIVE_NORTH_CURRENT_CONTINUATION.json) and [relay/Island](games/toronto-dispatch/docs/NATIVE_RELAY_ISLAND_CURRENT_CONTINUATION.json) records.

Read the [game guide and controls](games/toronto-dispatch/README.md) and [Chromatic loading instructions](games/toronto-dispatch/docs/LOADING.md). The selected 1 MiB update `toronto-dispatch-sandbox-stable.gbc` has SHA-256 `096862abfd1e1fa7d5ceb6dc6d808b08a580ac9dc5b33d428e4f97d297eee45b`. Generated binaries stay outside Git; earlier ROMs and bundles remain preserved separately.

Official compilation, four compiled guards and full `make check` pass. The [fresh native record](games/toronto-dispatch/docs/NATIVE_SANDBOX_STABLE.json) includes a full tram-cycle idle check, Settings/map freeze, one delivery, interrupted-entry save/reset, boat driving/docking, grocery entry/return, vehicle theft and sampled police pursuit. [Build](games/toronto-dispatch/docs/SANDBOX_STABLE_BUILD.json), [timing](games/toronto-dispatch/docs/SANDBOX_STABLE_TIMING.json) and [memory optimization](games/toronto-dispatch/docs/SANDBOX_STABLE_OPTIMIZATION.json) retain the exact scope. The failed `668727…` candidate remains ineligible. These samples do not establish universal smoothness, every sandbox interaction, all 104 contracts or two enjoyable human hours.

The last physically confirmed ROM is `toronto-dispatch-hardware-feedback.gbc`, SHA-256 `9c155a70c0cc3d6ccec986fc0ab7ddc3a204fd04e80879ad0233926156d4b10e`. Its [user-requested retry](games/toronto-dispatch/docs/CARTRIDGE_UPDATE_HARDWARE_FEEDBACK_RETRY_2026_10_04.json) completed with vendor-reported success and a closed writer; the user confirmed cold boot with USB disconnected and physical buttons. Complete read-back was unavailable. The selected sandbox-stable ROM still needs its own cartridge write, cold boot and physical save/audio/flicker checks.

## Develop

Install the ModRetro Chromatic plugin, read [AGENTS.md](AGENTS.md) and the game's design, decisions and roadmap, then select `games/toronto-dispatch/project/project.gbsproj`. Follow the [native build instructions](games/toronto-dispatch/docs/BUILD.md). Shared workflows live in [skills/](skills/), also exposed at `.agents/skills`.

```sh
python3 -m pip install --requirement requirements-dev.txt
make check
```

Repository checks validate sources and host engine behavior; native builds, emulator play and physical cartridge checks are separate evidence. Add new games under `games/<game-slug>/`; see [CONTRIBUTING.md](CONTRIBUTING.md).

Original code/art are [MIT licensed](LICENSE); [third-party notices](THIRD_PARTY_NOTICES.md) preserve upstream terms. This independent project is unaffiliated with ModRetro, Nintendo, Uber, the City or TTC.
