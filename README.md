# ModRetro Games

Original open-source homebrew games for ModRetro Chromatic / Game Boy Color. Each game keeps its editable project, artwork, design and verification record in `games/<game-slug>/`.

| Game | Features | Current status |
| --- | --- | --- |
| [Toronto Dispatch](games/toronto-dispatch/) | North-up courier sandbox, seven authored Toronto districts, 104 contracts, walking/driving and scheduled transit | Vehicle feedback candidate passes a fresh four-job replay, clear truck eligibility, funded police fine and saved-progress reset. Full campaign, two enjoyable human hours and hardware remain pending. |

Current editable source contains 344 buildings, 657 pedestrian routes, 64 service points, nine parking anchors and four player vehicles. Explore compressed mainland neighbourhoods, Port Lands industry/bridges, public walking Islands and Casa Loma/Summerhill/St Clair. Momentum/braking, pedestrians and police consequences, scheduled train/bus/Queen/ferry travel, a scrollable atlas, original audio and save v10 / 58-byte progression are implemented. The old 96 contracts and 59 stops remain unchanged.

![Core street traffic and courier](games/toronto-dispatch/docs/native-right-hand-traffic/occupied-police-lane.png) ![St Clair on foot](games/toronto-dispatch/docs/native-right-hand-traffic/st-clair-on-foot.png)

Original 160 × 144 emulator frames from the retained `e4a9…` traffic ROM: Core at frame 7,322 and St Clair at 13,648. The [native record](games/toronto-dispatch/docs/NATIVE_RIGHT_HAND_TRAFFIC.json) retains their exact provenance. The retained a935 [ferry recovery record](games/toronto-dispatch/docs/NATIVE_FERRY_CLARITY.json) and current [four-job record](games/toronto-dispatch/docs/NATIVE_DISPATCH_VEHICLE_FEEDBACK.json) keep separate exact-ROM scopes.

## Play or load

Read the [controls and features](games/toronto-dispatch/README.md) and [Chromatic loading guide](games/toronto-dispatch/docs/LOADING.md). Latest local playtest candidate is `toronto-dispatch-vehicle-feedback.gbc`, 1,048,576 bytes, SHA-256 `7ab28b84c242f7f2c8f9e02338e2d81ab6d789fb1d7997d0aa33e99baadd8dc0`. Official build and full `make check` pass. Its [fresh native record](games/toronto-dispatch/docs/NATIVE_DISPATCH_VEHICLE_FEEDBACK.json) completes four unique jobs, verifies the truck-only warning and acceptance, a funded level-three police fine and genuine in-worker save reset. The [focused build audit](games/toronto-dispatch/docs/DISPATCH_VEHICLE_BUILD_AUDIT.json) records the UI-only correction; the [old a935 feedback finding](games/toronto-dispatch/docs/NATIVE_FERRY_CAMPAIGN_FEEDBACK_FINDING.json) stays needs-review. Generated ROMs stay local; package details are in BUILD.md.

Menu-used A/B must be released before they accelerate or brake/reverse in the city. Dispatch keeps the full route preview, current-stop cue and A resume; Select jumps chapters. An eligible incomplete vehicle-specific offer shows WRONG VEHICLE until you occupy the required vehicle. Island offers show $8/$16/$24 ferry budgets. When an active Island job cannot fund a return, the HUD explains cancellation; abandoning the job allows the existing scheduled $0 assistance, with no payout or completion.

The older `22157…` [North records](games/toronto-dispatch/docs/NORTH_DISTRICT.md) and source-pinned package retain their own campaign progress. [TESTING.md](games/toronto-dispatch/TESTING.md) and [BUILD.md](games/toronto-dispatch/docs/BUILD.md) separate all exact-ROM results and failures. Full Old Toronto, all 104 played contracts, two measured enjoyable human hours, wider pacing/balance, browser recovery and physical stream/flash/cold-boot/save persistence remain unverified. The 36 traffic circuits now use right-hand lanes with separately checked police routing. Broader queue behavior remains open. No hardware installation is claimed; [published Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) is a separate older release.

## Develop

Install the ModRetro Chromatic plugin, read [AGENTS.md](AGENTS.md) and the game's design/decisions/roadmap, then select `games/toronto-dispatch/project/project.gbsproj`. Follow [native build instructions](games/toronto-dispatch/docs/BUILD.md). Shared workflows live in [skills/](skills/), also exposed at `.agents/skills`.

```sh
python3 -m pip install --requirement requirements-dev.txt
make check
```

Repository checks validate sources and host engine behavior; native builds, emulator play and physical cartridge checks are separate evidence. Add new games under `games/<game-slug>/`; see [CONTRIBUTING.md](CONTRIBUTING.md).

Original code/art are [MIT licensed](LICENSE); [third-party notices](THIRD_PARTY_NOTICES.md) preserve upstream terms. This independent project is unaffiliated with ModRetro, Nintendo, Uber, the City or TTC.
