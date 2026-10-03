# Load Toronto Dispatch onto your Chromatic

Use your Chromatic and the **writable ModRetro DevDay cartridge**. The cartridge can be empty. The game is a CGB-only homebrew ROM, so select the `.gbc` build. No game has been written to physical hardware yet; the first physical boot is an important check.

The verified local loading candidate is `toronto-dispatch-queen-street-life.gbc`, **524,288 bytes**, SHA-256 `5ae4a83b3cb13dfbd838e4fdf9b49df79e8db1949af360e91e69a97fe148dda2`. It contains six compressed Toronto districts, 96 contracts, 59 service points, seven mainland parking anchors and save v9 / 58 bytes. Its official build/header, compiled resource/table guards, 1,097-byte static reserve and complete source suite pass. [Its scoped native replay](NATIVE_QUEEN_STREET_LIFE_SAMPLES.json) covers a full-condition first delivery, paid Queen 47→43→47 with correct pedestrian/view behavior, paid reset, original parked-car recovery, audio labels, two exact map-freeze pairs and resumed driving. This fixes the later native actor-flags/view corruption and remote-car filtering; it does not inherit the older Island campaign results.

The earlier `8a96…` six-scene build has a [retained pause/audio-map failure](NATIVE_ISLAND_MAP_FAILURE.json) and must not be selected. The older five-scene `4343…` itinerary/payout, `ff5d…` preparation and `c625…` Port-job recordings keep their own scopes in [TESTING.md](../TESTING.md). The downloadable [Prototype 6 bundle](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) remains the separate four-district `23b2…` release; no newer GitHub release or physical installation is claimed here.

Current source preserves all contract IDs/fields and completion bits, relocating only six Island stop geometries. Version 9 reads valid older v4–v8 records, validates frozen old Island terrain and legitimate ferry bookings before relocation, and preserves cash and the mainland car. Normal v5–v8 restore keeps the older committed slot until the next alternating v9 write; v4 has its separate upgrade. Older ROMs cannot read both rewritten v9 slots. Preserve existing cartridge saves through the supported backup workflow before testing a newer build. Host migration checks and emulator resets do not prove native old-save imports or physical power-off persistence.

This is still a prototype. Full Old Toronto, all played contracts/balanced deadlines, two measured enjoyable hours, wider vehicle/traffic/pacing/stack checks, human handling/audio feedback, browser recovery and cartridge write/read-back/cold boot remain open. [BUILD.md](BUILD.md), [TESTING.md](../TESTING.md) and [AUDIO.md](AUDIO.md) distinguish the available evidence. A later source change needs its own ROM identity and replay.

A [separate campaign replay](NATIVE_ISLAND_CAMPAIGN_SAMPLES.json) completes 13 unique quests on retained `7b2af59c…` (`toronto-dispatch-islands-safe.gbc`), covering all eight job kinds. Centre Letters finishes its 195-second limit at full condition with 136 seconds left. Active-job map freezing, carried and paid-ferry resets, cancellation and a deliberate paid-crossing timeout pass. Island arrivals require **Select at each ordered dock/client** before moving to the next objective. The remaining eight Island jobs and the broader acceptance checks above remain pending.

## 1. Prepare the computer and console

1. Install the ModRetro Chromatic plugin in Codex on the Mac connected to the console. It is already installed in this project owner's current setup. Open this repository as the project.
2. Use the [official ModRetro Updater](https://support.modretro.com/en_us/articles/chromatic-firmware-updater-ryhoYnzCx) for the firmware/setup steps described in the [DevDay quickstart](https://support.modretro.com/en_us/chromatic-devday-edition-quickstart-guid-By1iOlcMg). If this computer needs Developer Mode, press **Cmd-I** in the updater and enter the included activation code privately. Windows uses Ctrl-I. Keep the code out of chat and the repo. Plugin discovery cannot tell you whether activation is complete.
3. Power off the Chromatic, insert the writable ModRetro development cartridge, connect it using a USB **data** cable, then turn it on. Leave it connected during detection or installation. If more than one Chromatic is connected, identify the intended one instead of assuming Player 1.
4. Return to Codex after the updater has finished its work. macOS requires no extra vendor driver. If the plugin reports a concrete access or setup issue, follow that result and the [shared hardware workflow](../../../docs/HARDWARE.md).

The updater activates the computer and handles console firmware. The game itself is loaded through the plugin's cartridge write action. Do not use Cart Clinic's update button as a substitute for selecting this original ROM.

## 2. Select the actual game build

The editable project is `games/toronto-dispatch/project/project.gbsproj`. The current expanded source output is:

```text
games/toronto-dispatch/project/build/toronto-dispatch-queen-street-life.gbc
```

Generated ROMs are excluded from Git. Any later source change needs a distinct output with its own matching debug artifacts, inspection and native record; do not reuse this measured candidate hash. Official downloadable bundles include `SHA256SUMS`, instructions and notices. Do not rename a browser export or `.gbsproj` file to `.gbc`.

For a source build, ask Codex:

> Use the ModRetro Chromatic plugin. Select Toronto Dispatch and build current source to a new distinct filename under `build/` with matching debug artifacts, preserving measured `toronto-dispatch-queen-street-life.gbc` and all older files. Run the memory and compiled resource/frame/isolation/progress-table guards, inspect its exact path/size/SHA-256 and test that same ROM's quest previews/payment, street/courier/transit/save behavior and pacing before preparing installation. Retain older measured binaries separately.

Use `rom_inspect` on the final file. Match its digest to the tested build in [TESTING.md](../TESTING.md) or the downloaded release's checksum. A new build can have a different hash: compare it to its own new inspection/playtest rather than silently adopting an old checksum. The current engine uses MBC5 and battery SRAM; ROM header validity alone does not prove that a cartridge supports it.

Optional read-only checks for the expanded candidate from the repository root on macOS (substitute the exact new filename for a new build):

```sh
shasum -a 256 games/toronto-dispatch/project/build/toronto-dispatch-queen-street-life.gbc
wc -c < games/toronto-dispatch/project/build/toronto-dispatch-queen-street-life.gbc
```

For a downloaded bundle, from its extracted directory:

```sh
shasum -a 256 -c SHA256SUMS
```

Only proceed when the named ROM's checksum passes. Keep the bundle's build/source identity and notices alongside the ROM.

## 3. Install on the writable cartridge

The direct Codex/plugin route works independently of a browser preview. After the build is inspected, send this prompt and identify the selected device if there is a choice:

> Use the ModRetro Chromatic plugin to find my connected Chromatic and the writable ModRetro DevDay cartridge I inserted. Detect that cartridge if needed, then rediscover the intended device. I acknowledge that writing will erase this cartridge's existing game data, may lose saves, and makes no automatic backup. I approve writing the exact Toronto Dispatch ROM just inspected and tested to this selected development cartridge. Show me the file path, byte count, SHA-256 and selected device, then flash it through the supported plugin workflow and follow the original operation to completion. Do not alter unrelated firmware or another cartridge. Guide me through a real cartridge cold boot afterward.

The plugin must use the inspected ROM path, size and SHA-256 in `flash`, along with a fresh selection token and a unique request ID. If cartridge detection consumed a token, it must rediscover before writing. Keep the computer, cable and console connected until the original write operation has finished.

A working official browser preview also offers **Install on Chromatic**: choose the intended device, review the erasure warning and choose **Confirm Install**. This installs that preview's built ROM, which can differ from the native ROM. Use its exact inspection/test evidence. The project's previously paused preview has an unresolved close acknowledgement; use the direct route above while that state is unresolved.

**A successful vendor write is installation evidence. It is not evidence that the cartridge cold-boots, runs smoothly or preserves saves.** If the tool supplies read-back verification, retain that result separately; if it does not, record read-back as unavailable.

## 4. Verify the cartridge, then play

After a successful write, power off and disconnect USB, then power on with the same cartridge inserted. This tests stored cartridge execution independently of host streaming. Use the physical buttons; emulator inputs cannot control the console.

| Check | Expected result |
| --- | --- |
| Cold boot | Toronto Dispatch's title/help/start flow appears and enters the city; no blank screen, corrupt tiles or reset loop |
| Driving | A accelerates, left/right steer, B brakes and reverses near rest; corners retain momentum |
| First delivery | Select opens dispatch; accept the first Union-to-St. Lawrence job with A, Select collects at Union, drive east on Front Street, brake and Select delivers at the marker |
| Quest planning and payment | Dispatch left/right selects jobs; up/down browses stops, districts and walking/return cues. Check condition/base/time/credited pay after delivery, and zero payment after timeout |
| Walking and car entry | Stop, Start → Park / recover car; walk with D-pad, approach the parked car and press A to enter |
| Map and pause | Start → Scroll City Map; D-pad pans across areas, A centres the job/booked stop/depot, Select changes focus, B returns; mission time freezes |
| Transit | On foot at a station, Queen curb sign or ferry terminal, B opens routes; choose with left/right and board with A. At Wellesley, up changes train/bus. Check direction, wait and ride time before boarding; fare is charged once and mission time continues |
| Islands | Park on the mainland, walk to the ferry terminal and travel to Hanlan's, Centre or Ward's; follow public paths/bridges, with no car teleport. A no-job courier with cash below $4 can return from an Island dock for $0 on the normal schedule |
| Saving | Use the pause menu's Save action, record cash and completed count, power off/on and confirm both persist |
| Audio | Start → Audio, then A cycles music + effects, effects only and silent; B returns. Check the city score, engine and braking sounds, short delivery/transit cues, and silence in silent mode. Menu/world pause stops music and engine; short interface/result cues may finish. The mode defaults on each boot |
| Readability/performance | Check text, building occlusion, traffic and pedestrians for flicker, slowdown or delayed input |

Keep a note of the ROM SHA-256 and any problem's location/action. Current save v9 preserves the 96 contract IDs and imports valid v4–v8 records through the documented migration. Host checks cover older saves; native old-save imports and physical migration are unverified. See BUILD/TESTING. Emulator reset persistence does not prove power-off persistence on a physical cartridge.

## If installation does not finish

- **In progress, lost reply or unknown result:** keep the device connected and use the same operation's **Check status** or have Codex query its original operation ID. Do not press Install again or create a new flash request. A closed panel does not cancel a write.
- **Developer Mode required:** preserve the original error/outcome, complete activation privately in ModRetro Updater, then return for fresh discovery and a newly requested write after the old operation is resolved.
- **Connection or cartridge error:** use the original diagnostic and its recovery instructions. Power off before reseating the cartridge. A generic `device.program_failed` does not identify activation, driver or contact failure by itself. Only writable supported ModRetro cartridges are eligible for this path.
- **Finished failure:** use **Copy error** for the bounded report and original operation ID. Keep raw journals private. If the plugin confirms closure, acknowledge/dismiss that failed operation before an explicitly requested new installation; dismissing does not prove the cartridge was unchanged.
- **Successful write but bad boot:** preserve the exact ROM/hash and observation. Test the same ROM in the native emulator and report the physical failure before rewriting. Do not reset console firmware or use an unrelated flash profile as an automatic fix.

A host-streamed `play` demo is optional and never writes the cartridge. It can help assess the screen/buttons, but it cannot replace the cold-boot and save checks above.

Reviewed 2026-10-02 against official ModRetro support and installed plugin 1.0.33 deployment documentation. No activation code, device token or private preview URL is required in these instructions.
