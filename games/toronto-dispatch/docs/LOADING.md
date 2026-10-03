# Load Toronto Dispatch onto your Chromatic

Use your Chromatic and the **writable ModRetro DevDay cartridge**. The cartridge can be empty. The game is a CGB-only homebrew ROM, so select the `.gbc` build. No game has been written to physical hardware yet; the first physical boot is an important check.

The retained local loading candidate is `toronto-dispatch-route-feedback.gbc`, **524,288 bytes**, SHA-256 `4343f2b858e62f9e8daa2a3576a1d06bb7ee7208d2cd4a55434c36e168fb3100`. It retains 96 contracts / 59 stops / seven parking anchors and the 58-byte v8 save, with itinerary browsing, exact payout feedback and equivalent tram ROM tables. Header/compiled sprite/table guards and 1,097 static bytes pass. [Scoped native UI and Line 1 records](NATIVE_DISPATCH_UI_SAMPLES.json) cover previews/locks, first delivery/payment, timeout, reset and paid train/map/reset/arrival; broader campaign/hardware remain open. Retained `c625…` [Fire Hall replay](NATIVE_PORT_CAMPAIGN_SAMPLES.json) and `a638…` [Queen/table replay](NATIVE_TRAM_PROGRESS_SAMPLES.json) keep their own identities. The downloadable [Prototype 6 bundle](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) remains the earlier four-district `toronto-queen-streetcar-safe.gbc`, SHA-256 `23b2a7a25c9c593a51967e16a275cfb162bbb3e59f709eecd37dd77e2bb408f0`. No newer release or physical installation is claimed.

Retained Port Lands baseline `a212dd9e…`, `toronto-port-lands-labels.gbc`, keeps its earlier five-scene travel/aircraft/map/checkpoint proofs. Older 75-byte epoch `35337509…` also keeps its own delivery/90-loop identity; none of these records is assigned to new source.

Retained fused `a2438348…`, `toronto-streetlife-fused.gbc`, keeps separate H1/map/reset/police/shore/boat/first-delivery evidence and a later [Port car/foot sample](NATIVE_PORT_DRIVING_SAMPLE.json). That sample crosses all six decks by car, restores a Port car/walker, walks Beach to actual water and returns through Leslie; its H2/H3 cash-zero observations do not verify higher fine amounts. Current96-job source uses only Leslie and existing rules, without 114 service. [TESTING.md](../TESTING.md) and [PERFORMANCE.md](PERFORMANCE.md) retain exact scopes. Static reserve and sampled sprite counts do not prove deepest stack or whole-city pacing.

Earlier walker-contact, booked-tram HOLD, legacy-service and roof checks remain attached to their actual older ROMs. All vehicles/bridges/seams, the full campaign and two-hour target, full Old Toronto/Islands, browser recovery and physical execution remain open. No cartridge write/read-back, streaming or physical cold boot is recorded for this candidate.

The retained `a212…` baseline uses save v7; current street source uses v8 with the same 58-byte state, reusing obsolete cursor words for police attention/countdown and clearing them on valid pre-v8 imports. Prototype 6 uses v6. Pre-v8 binaries cannot restore rewritten v8 records once both slots are replaced; old 88-job v8 binaries reject new saved jobs/completion bits. Preserve existing cartridge saves through the supported backup workflow before testing a newer build. Host migrations and emulator reset evidence do not establish physical power-off persistence.

Original music and vehicle/event/transit effects are implemented, with music + effects, effects-only and silent options. [TESTING.md](../TESTING.md) separates native gameplay scenarios and their build identities; [AUDIO.md](AUDIO.md) records the actual PCM evidence and remaining listening checks.

It remains a prototype: full Old Toronto coverage, the full campaign/two-hour gameplay target, human handling/listening and physical cartridge behaviour are unverified. No cartridge write/read-back or physical cold boot is recorded for this candidate. A later source build must use its own new hash and test record.

The newer `ff5d…` preparation ROM retains the same five registered scenes and passes an existing Hanlan ferry round-trip regression. The fuller Island background remains unregistered source art and versioned migration is pending. This loading guide intentionally retains the independently packaged `4343…` bundle; preparation evidence does not change its binary or hardware status. [Preparation scope](../TESTING.md).

## 1. Prepare the computer and console

1. Install the ModRetro Chromatic plugin in Codex on the Mac connected to the console. It is already installed in this project owner's current setup. Open this repository as the project.
2. Use the [official ModRetro Updater](https://support.modretro.com/en_us/articles/chromatic-firmware-updater-ryhoYnzCx) for the firmware/setup steps described in the [DevDay quickstart](https://support.modretro.com/en_us/chromatic-devday-edition-quickstart-guid-By1iOlcMg). If this computer needs Developer Mode, press **Cmd-I** in the updater and enter the included activation code privately. Windows uses Ctrl-I. Keep the code out of chat and the repo. Plugin discovery cannot tell you whether activation is complete.
3. Power off the Chromatic, insert the writable ModRetro development cartridge, connect it using a USB **data** cable, then turn it on. Leave it connected during detection or installation. If more than one Chromatic is connected, identify the intended one instead of assuming Player 1.
4. Return to Codex after the updater has finished its work. macOS requires no extra vendor driver. If the plugin reports a concrete access or setup issue, follow that result and the [shared hardware workflow](../../../docs/HARDWARE.md).

The updater activates the computer and handles console firmware. The game itself is loaded through the plugin's cartridge write action. Do not use Cart Clinic's update button as a substitute for selecting this original ROM.

## 2. Select the actual game build

The editable project is `games/toronto-dispatch/project/project.gbsproj`. The current expanded source output is:

```text
games/toronto-dispatch/project/build/toronto-dispatch-route-feedback.gbc
```

Generated ROMs are excluded from Git. Any later source change needs a distinct output with its own matching debug artifacts, inspection and native record; do not reuse this measured candidate hash. Official downloadable bundles include `SHA256SUMS`, instructions and notices. Do not rename a browser export or `.gbsproj` file to `.gbc`.

For a source build, ask Codex:

> Use the ModRetro Chromatic plugin. Select Toronto Dispatch and build current source to a new distinct filename under `build/` with matching debug artifacts, preserving measured `toronto-dispatch-route-feedback.gbc` and all older files. Run the memory and compiled resource/frame/isolation/progress-table guards, inspect its exact path/size/SHA-256 and test that same ROM's quest previews/payment, street/courier/transit/save behavior and pacing before preparing installation. Retain older measured binaries separately.

Use `rom_inspect` on the final file. Match its digest to the tested build in [TESTING.md](../TESTING.md) or the downloaded release's checksum. A new build can have a different hash: compare it to its own new inspection/playtest rather than silently adopting an old checksum. The current engine uses MBC5 and battery SRAM; ROM header validity alone does not prove that a cartridge supports it.

Optional read-only checks for the expanded candidate from the repository root on macOS (substitute the exact new filename for a new build):

```sh
shasum -a 256 games/toronto-dispatch/project/build/toronto-dispatch-route-feedback.gbc
wc -c < games/toronto-dispatch/project/build/toronto-dispatch-route-feedback.gbc
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
| Saving | Use the pause menu's Save action, record cash and completed count, power off/on and confirm both persist |
| Audio | Start → Audio, then A cycles music + effects, effects only and silent; B returns. Check the city score, engine and braking sounds, short delivery/transit cues, and silence in silent mode. Menu/world pause stops music and engine; short interface/result cues may finish. The mode defaults on each boot |
| Readability/performance | Check text, building occlusion, traffic and pedestrians for flicker, slowdown or delayed input |

Keep a note of the ROM SHA-256 and any problem's location/action. Version6 includes migration for valid v5 prototype saves, retaining the original72 contract IDs. Host checks cover migration; physical cartridge migration is unverified. Other save-format changes may retire active jobs or reject earlier records. See BUILD/TESTING. Emulator reset persistence does not prove power-off persistence on a physical cartridge.

## If installation does not finish

- **In progress, lost reply or unknown result:** keep the device connected and use the same operation's **Check status** or have Codex query its original operation ID. Do not press Install again or create a new flash request. A closed panel does not cancel a write.
- **Developer Mode required:** preserve the original error/outcome, complete activation privately in ModRetro Updater, then return for fresh discovery and a newly requested write after the old operation is resolved.
- **Connection or cartridge error:** use the original diagnostic and its recovery instructions. Power off before reseating the cartridge. A generic `device.program_failed` does not identify activation, driver or contact failure by itself. Only writable supported ModRetro cartridges are eligible for this path.
- **Finished failure:** use **Copy error** for the bounded report and original operation ID. Keep raw journals private. If the plugin confirms closure, acknowledge/dismiss that failed operation before an explicitly requested new installation; dismissing does not prove the cartridge was unchanged.
- **Successful write but bad boot:** preserve the exact ROM/hash and observation. Test the same ROM in the native emulator and report the physical failure before rewriting. Do not reset console firmware or use an unrelated flash profile as an automatic fix.

A host-streamed `play` demo is optional and never writes the cartridge. It can help assess the screen/buttons, but it cannot replace the cold-boot and save checks above.

Reviewed 2026-10-02 against official ModRetro support and installed plugin 1.0.33 deployment documentation. No activation code, device token or private preview URL is required in these instructions.
