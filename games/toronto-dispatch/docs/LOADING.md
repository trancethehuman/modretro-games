# Load Toronto Dispatch onto your Chromatic

Select the latest local playtest candidate, **`toronto-dispatch-north-initial.gbc`**, **1,048,576 bytes**, SHA-256 **`22157d720c8746118da93bb7c3027ef24693a2e4697118e70e5a95fecddf0d95`**. It contains seven compressed districts, 104 contracts, 64 service points and save v10 / 58 bytes. The supported target is your **writable ModRetro DevDay cartridge**; it can be empty. This is a CGB-only homebrew ROM.

Official build/resource and full source checks pass. The exact ROM has three fresh Core deliveries and a continuation completing Baldwin Book Box, plus walking/car recovery, both northern entrances, Summerhill–St Clair scheduled travel, paused map and emulator SRAM resets. These scoped records do not prove every contract or two enjoyable human hours. No physical installation has been attempted. Detailed [build](BUILD.md) and [test evidence](../TESTING.md) remain separate from physical verification.

## 1. Prepare the computer and console

1. Install the ModRetro Chromatic plugin in Codex on the Mac connected to the console. It is already installed in this project owner's current setup. Open this repository as the project.
2. Use the [official ModRetro Updater](https://support.modretro.com/en_us/articles/chromatic-firmware-updater-ryhoYnzCx) for the firmware/setup steps described in the [DevDay quickstart](https://support.modretro.com/en_us/chromatic-devday-edition-quickstart-guid-By1iOlcMg). If this computer needs Developer Mode, press **Cmd-I** in the updater and enter the included activation code privately. Windows uses Ctrl-I. Keep the code out of chat and the repo. Plugin discovery cannot tell you whether activation is complete.
3. Power off the Chromatic, insert the writable ModRetro development cartridge, connect it using a USB **data** cable, then turn it on. Leave it connected during detection or installation. If more than one Chromatic is connected, identify the intended one instead of assuming Player 1.
4. Return to Codex after the updater has finished its work. macOS requires no extra vendor driver. If the plugin reports a concrete access or setup issue, follow that result and the [shared hardware workflow](../../../docs/HARDWARE.md).

The updater activates the computer and handles console firmware. The game itself is loaded through the plugin's cartridge write action. Do not use Cart Clinic's update button as a substitute for selecting this original ROM.

## 2. Select the actual game build

The editable project is `games/toronto-dispatch/project/project.gbsproj`. The selected loading ROM is:

```text
games/toronto-dispatch/project/build/toronto-dispatch-north-initial.gbc
```

Generated ROMs are excluded from Git. Any later source change needs a distinct output with its own matching debug artifacts, inspection and native record; do not reuse this measured candidate hash. Official downloadable bundles include `SHA256SUMS`, instructions and notices. Do not rename a browser export or `.gbsproj` file to `.gbc`.

The North loading bundle is packaged after committing the matching frozen project sources. Its `SHA256SUMS` verifies the ROM above, and `BUILDINFO.json` identifies that committed source and instructions. Preserve those files and the notices alongside the ROM. The older `03e09…` UI bundle and published Prototype 6 retain their separate identities below.

For a source build, ask Codex:

> Use the ModRetro Chromatic plugin. Select Toronto Dispatch and build current source to a new distinct filename under `build/` with matching debug artifacts, preserving measured `toronto-dispatch-north-initial.gbc` and all older files. Run the memory and compiled resource/frame/isolation/progress-table guards, inspect its exact path/size/SHA-256 and test that same ROM's quest previews/payment, street/courier/transit/save behavior and pacing before preparing installation. Retain older measured binaries separately.

Use `rom_inspect` on the final file. Match its digest to the tested build in [TESTING.md](../TESTING.md) or the downloaded release's checksum. A new build can have a different hash: compare it to its own new inspection/playtest rather than silently adopting an old checksum. The current engine uses MBC5 and battery SRAM; ROM header validity alone does not prove that a cartridge supports it.

Optional read-only checks for the expanded candidate from the repository root on macOS (substitute the exact new filename for a new build):

```sh
shasum -a 256 games/toronto-dispatch/project/build/toronto-dispatch-north-initial.gbc
wc -c < games/toronto-dispatch/project/build/toronto-dispatch-north-initial.gbc
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
| Delivery result controls | B closes the result without reversing while held; release and press again for normal braking/reverse. A opens dispatch |
| Quest planning and payment | Dispatch Select jumps chapters; left/right selects offers and up/down browses stops. During a job open Pause → Dispatch jobs, check CURRENT STOP and A RESUME without replacing work. Check condition/base/time/credited pay after delivery, and zero payment after timeout |
| Walking and car entry | Stop, Start → Park / recover car; walk with D-pad, approach the parked car and press A to enter. With no active job, the completed-entry HUD shows the vehicle and driving controls |
| Map and pause | Start → Scroll City Map; D-pad pans across areas, A centres the job/booked stop/depot, Select changes focus, B returns; mission time freezes |
| Transit | On foot at a station, Queen curb sign or ferry terminal, B opens routes; choose with left/right and board with A. At Wellesley, up changes train/bus. Check direction, wait and ride time before boarding; fare is charged once and mission time continues |
| Islands | Park on the mainland, walk to the ferry terminal and travel to Hanlan's, Centre or Ward's; follow public paths/bridges, with no car teleport. A no-job courier with cash below $4 can return from an Island dock for $0 on the normal schedule |
| Northern district | Follow Spadina or Yonge north of Bloor. Complete three starter jobs to unlock chapter 13's Baldwin Book Box, collect at Union and park below Baldwin Steps; walk the public stair route to deliver |
| Northern train stops | Walk to Summerhill or St Clair, press B, choose the other stop and board with A. Each trip costs $3; the parked car stays at its original location |
| Saving | Use the pause menu's Save action, record cash and completed count, power off/on and confirm both persist |
| Audio | Start → Audio, then A cycles music + effects, effects only and silent; B returns. Check the city score, engine and braking sounds, short delivery/transit cues, and silence in silent mode. Menu/world pause stops music and engine; short interface/result cues may finish. The mode defaults on each boot |
| Readability/performance | Check text, building occlusion, traffic and pedestrians for flicker, slowdown or delayed input |

Keep a note of the ROM SHA-256 and any problem's location/action. Save v10 preserves the old 96 contract IDs, appends eight North contracts and explicitly imports valid v9 and older supported records through documented migration. Host and compiled checks cover older saves; native old-save imports and physical migration are unverified. See BUILD/TESTING. Emulator reset persistence does not prove power-off persistence on a physical cartridge.

## If installation does not finish

- **In progress, lost reply or unknown result:** keep the device connected and use the same operation's **Check status** or have Codex query its original operation ID. Do not press Install again or create a new flash request. A closed panel does not cancel a write.
- **Developer Mode required:** preserve the original error/outcome, complete activation privately in ModRetro Updater, then return for fresh discovery and a newly requested write after the old operation is resolved.
- **Connection or cartridge error:** use the original diagnostic and its recovery instructions. Power off before reseating the cartridge. A generic `device.program_failed` does not identify activation, driver or contact failure by itself. Only writable supported ModRetro cartridges are eligible for this path.
- **Finished failure:** use **Copy error** for the bounded report and original operation ID. Keep raw journals private. If the plugin confirms closure, acknowledge/dismiss that failed operation before an explicitly requested new installation; dismissing does not prove the cartridge was unchanged.
- **Successful write but bad boot:** preserve the exact ROM/hash and observation. Test the same ROM in the native emulator and report the physical failure before rewriting. Do not reset console firmware or use an unrelated flash profile as an automatic fix.

A host-streamed `play` demo is optional and never writes the cartridge. It can help assess the screen/buttons, but it cannot replace the cold-boot and save checks above.

Device procedure reviewed against installed plugin 1.0.33 deployment documentation; the North ROM selection was inspected on 2026-10-03. No activation code, device token or private preview URL is required in these instructions.

## Evidence and older builds

The selected North build's [compiled audit](NORTH_BUILD_AUDIT.json) verifies 47,502 assertions and 203 frozen project inputs. Its [fresh record](NATIVE_NORTH_FRESH.json) completes three Core jobs and loads North while carrying job 96. Its [separately recorded continuation](NATIVE_NORTH_CONTINUED.json) completes the Baldwin stairs delivery, verifies two train trips and map/reset/car retention, and exercises reciprocal northern seams. Progress imported from the fresh same-ROM checkpoint is disclosed. Independent fresh and continued reviews pass 37,871 and 55,785 evidence checks, respectively; the continuation also passes 359 documentation/link/privacy checks. Wider regional jobs and performance remain pending. Main code bank 2 is full, UI bank 1 has three bytes free and save bank 29 has fifteen; further code changes require a fresh capacity review. The linked 1,096-byte static reserve does not measure deepest stack use.

The retained [03e09 native record](NATIVE_UI_POLISH_SAMPLES.json) keeps its own six-district, v9 identity: `toronto-dispatch-ui-polish-direct.gbc`, 524,288 bytes, SHA-256 `03e09fa85351c91f56d7370a3f0e0f856cd147a3c8d5ebfd328a2786a38f25d6`. Its older ZIP is 130,616 bytes, SHA-256 `67b7412321d04d9385f9201451fcb8f2755084262042fcfcecc6c26aeb238c21`, source `9816ac43b9c6163882098189c5ac9c93943a70d6`. Its scope and 4,052 packaging checks are not assigned to North.

The [retained e797 22-job campaign](NATIVE_CAMPAIGN_TWENTY_TWO_SAMPLES.json) and [825 workload comparison](NATIVE_TERRAIN_CACHE_COMPARISON.json) keep their original ROM identities. Neither its 22 completions nor the measured 6.648%/3.896% gains are assigned to this North ROM.

The retained `964f…` [five-job continuation](NATIVE_COURIER_CHAIN_SAMPLES.json), earlier matched-route, bus/heat/patrol, Queen and Island records retain their own identities in [TESTING.md](../TESTING.md). Their wider acceptance is not inherited by this ROM. Failed `c4fa…`, `8a96…` and `cf2f…` candidates must not be selected. Downloadable [Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) remains an older separate release.

Full Old Toronto, remaining Island jobs, all contracts/balance, two enjoyable human hours, wider vehicle/crowd/pacing/stack checks, native older-save imports, human handling/audio, browser recovery and hardware remain open. Host migration checks and emulator resets do not establish physical power-off persistence.
