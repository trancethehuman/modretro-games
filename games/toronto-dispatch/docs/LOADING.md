# Load Toronto Dispatch onto your Chromatic

Use your Chromatic and the **writable ModRetro DevDay cartridge**. The cartridge can be empty. The game is a CGB-only homebrew ROM, so select the `.gbc` build. No game has been written to physical hardware yet; the first physical boot is an important check.

The current 2026-10-02 handling/city/audio ROM is 262,144 bytes, SHA-256 `a2f00db4ef834112a3491e50cec832653023a0456f0d9cbca6d2386be7322a59`. Its milestone identifier is `v0.2.0-prototype.2`; download available bundles from the [releases page](https://github.com/trancethehuman/modretro-games/releases). The official native build/header checks, 471 host engine checks, representative native button scenarios and a separate eight-interval PCM run for this exact ROM passed. A different build needs its own inspection and test identity.

Original music and vehicle/event/transit effects are implemented, with music + effects, effects-only and silent options. [TESTING.md](../TESTING.md) separates native gameplay scenarios and their build identities; [AUDIO.md](AUDIO.md) records the actual PCM evidence and remaining listening checks.

It remains a prototype: full Old Toronto coverage, the full campaign/two-hour gameplay target, human listening and physical cartridge behaviour are unverified. No console was connected during the latest discovery, and no cartridge write/read-back or cold boot was performed. A later source build must use its own new hash and test record.

## 1. Prepare the computer and console

1. Install the ModRetro Chromatic plugin in Codex on the Mac connected to the console. It is already installed in this project owner's current setup. Open this repository as the project.
2. Use the [official ModRetro Updater](https://support.modretro.com/en_us/articles/chromatic-firmware-updater-ryhoYnzCx) for the firmware/setup steps described in the [DevDay quickstart](https://support.modretro.com/en_us/chromatic-devday-edition-quickstart-guid-By1iOlcMg). If this computer needs Developer Mode, press **Cmd-I** in the updater and enter the included activation code privately. Windows uses Ctrl-I. Keep the code out of chat and the repo. Plugin discovery cannot tell you whether activation is complete.
3. Power off the Chromatic, insert the writable ModRetro development cartridge, connect it using a USB **data** cable, then turn it on. Leave it connected during detection or installation. If more than one Chromatic is connected, identify the intended one instead of assuming Player 1.
4. Return to Codex after the updater has finished its work. macOS requires no extra vendor driver. If the plugin reports a concrete access or setup issue, follow that result and the [shared hardware workflow](../../../docs/HARDWARE.md).

The updater activates the computer and handles console firmware. The game itself is loaded through the plugin's cartridge write action. Do not use Cart Clinic's update button as a substitute for selecting this original ROM.

## 2. Select the actual game build

The editable project is `games/toronto-dispatch/project/project.gbsproj`. The normal native output is:

```text
games/toronto-dispatch/project/build/toronto-dispatch.gbc
```

Generated ROMs are excluded from Git. A source checkout needs the official plugin build described in [BUILD.md](BUILD.md); an official downloadable ROM bundle should include `SHA256SUMS`, loading instructions and licence notices. Do not rename a browser export or a `.gbsproj` file to `.gbc`.

For a source build, ask Codex:

> Use the ModRetro Chromatic plugin. Select this repository's Toronto Dispatch project, build `build/toronto-dispatch.gbc`, inspect the resulting ROM, and show its exact path, size and SHA-256. Boot and smoke-test that exact native ROM before preparing installation.

Use `rom_inspect` on the final file. Match its digest to the tested build in [TESTING.md](../TESTING.md) or the downloaded release's checksum. A new build can have a different hash: compare it to its own new inspection/playtest rather than silently adopting an old checksum. The current engine uses MBC5 and battery SRAM; ROM header validity alone does not prove that a cartridge supports it.

Optional read-only checks from the repository root on macOS:

```sh
shasum -a 256 games/toronto-dispatch/project/build/toronto-dispatch.gbc
wc -c < games/toronto-dispatch/project/build/toronto-dispatch.gbc
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
| Walking and car entry | Stop, Start → Park / recover car; walk with D-pad, approach the parked car and press A to enter |
| Map and pause | Start → map; pan with D-pad, A centres the objective, B returns; mission time freezes while paused |
| Transit | On foot at a station/terminal, B opens routes; choose with left/right and board with A; fare and mission time update once |
| Saving | Use the pause menu's Save action, record cash and completed count, power off/on and confirm both persist |
| Audio | Start → Audio, then A cycles music + effects, effects only and silent; B returns. Check the city score, engine and braking sounds, short delivery/transit cues, and silence in silent mode. Menu/world pause stops music and engine; short interface/result cues may finish. The mode defaults on each boot |
| Readability/performance | Check text, building occlusion, traffic and pedestrians for flicker, slowdown or delayed input |

Keep a note of the ROM SHA-256 and any problem's location/action. Do not expect old prototype saves to work across save-format changes; the current build's save version and verified behaviour are documented in BUILD/TESTING. Emulator reset persistence does not prove power-off persistence on a physical cartridge.

## If installation does not finish

- **In progress, lost reply or unknown result:** keep the device connected and use the same operation's **Check status** or have Codex query its original operation ID. Do not press Install again or create a new flash request. A closed panel does not cancel a write.
- **Developer Mode required:** preserve the original error/outcome, complete activation privately in ModRetro Updater, then return for fresh discovery and a newly requested write after the old operation is resolved.
- **Connection or cartridge error:** use the original diagnostic and its recovery instructions. Power off before reseating the cartridge. A generic `device.program_failed` does not identify activation, driver or contact failure by itself. Only writable supported ModRetro cartridges are eligible for this path.
- **Finished failure:** use **Copy error** for the bounded report and original operation ID. Keep raw journals private. If the plugin confirms closure, acknowledge/dismiss that failed operation before an explicitly requested new installation; dismissing does not prove the cartridge was unchanged.
- **Successful write but bad boot:** preserve the exact ROM/hash and observation. Test the same ROM in the native emulator and report the physical failure before rewriting. Do not reset console firmware or use an unrelated flash profile as an automatic fix.

A host-streamed `play` demo is optional and never writes the cartridge. It can help assess the screen/buttons, but it cannot replace the cold-boot and save checks above.

Reviewed 2026-10-02 against official ModRetro support and installed plugin 1.0.33 deployment documentation. No activation code, device token or private preview URL is required in these instructions.
