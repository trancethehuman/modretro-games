# Chromatic build and cartridge workflow

Source: [ModRetro DevDay Edition quickstart](https://support.modretro.com/en_us/chromatic-devday-edition-quickstart-guid-By1iOlcMg), reviewed 2026-10-01.

## What the official guide establishes

- The ModRetro Chromatic plugin works with GB Studio projects and builds Game Boy Color compatible games.
- It provides a playable browser emulator and automated playtesting.
- Live device demos run in an emulator and stream to the connected Chromatic.
- A built homebrew ROM can be written to the supported physical cartridge.
- The included DevDay Edition activation code enables Developer Mode on the computer through the Chromatic Firmware Updater. On macOS the guide specifies Cmd-I to open that dialog.

The guide starts with updating the Chromatic through the official updater. When a device is connected, inspect its state and determine what is required; do not assume that game-loading authorisation covers unrelated firmware modification. The updater/activation may require the user's interaction. Never collect or publish the activation code.

## Setup record

Initial inspection, 2026-10-01:

- `ChromaticFirmwareUpdater.app` is installed in `/Applications`.
- The ModRetro plugin has no callable tools or supplied skills in the initial chat. Connector searches for ModRetro/Chromatic returned no match; Codex's Plugins tab may offer a different catalog.
- Computer Use cannot control the Codex app, so plugin installation/attachment requires the user.
- No `/dev/cu.usb*` serial node was visible. This alone does not establish the absence of every USB device.
- Device edition, cartridge model, Developer Mode activation, and firmware version are unverified.

Plugin preparation, 2026-10-02:

- The user installed the ModRetro Chromatic plugin; version 1.0.33 is callable.
- Setup and authoring skills were read. Official build/emulator dependencies were prepared outside the repository, with GB Studio CLI 4.3.2, GBDK 4.5.0 and PyBoy 2.7.0 verified.
- Toronto Dispatch now has a native editable starter project, compiled ROM, retained browser preview and headless emulator evidence. See its [build instructions](../games/toronto-dispatch/docs/BUILD.md) and [test record](../games/toronto-dispatch/TESTING.md).
- No streaming, device writing, firmware update or Developer Mode activation was attempted. Device/cartridge readiness is still unverified.

## Execution workflow after the plugin is attached

1. Read its actual setup, GB Studio, playtest, and hardware skills. Inspect installed versions and prepare missing official dependencies.
2. Create/select the editable game project and build a homebrew ROM. Record build output and a SHA-256 digest locally.
3. Run emulator tests: boot, controls, collision, mission transitions, pause/restart, and audio. Use the same preview during edits.
4. When the user connects hardware, identify the console and development cartridge through supported tools. Verify Developer Mode readiness. Power off before cartridge changes, following the vendor's instructions.
5. Stream a short demo if supported to assess screen readability, controls, and audio. Record this as streaming evidence.
6. Write only the selected build to the identified writable development cartridge using the plugin's supported process. Follow any actual prerequisite or approval rules in its hardware skill. Keep backups, codes, and device identifiers local and ignored.
7. Use tool-provided read-back/checksum verification if available; report if it is unavailable. Have the user boot the cartridge and try movement, braking, one complete job, pause/restart, and sound. Physical button presses cannot be claimed from software results alone.
8. Record the build, flash result, verification method, and manual observations in the game's `TESTING.md`.

If the hardware is another edition or cartridge type, establish its supported loading path before writing. Do not improvise FPGA/MCU flashing, third-party firmware, or unsupported cartridge profiles.
