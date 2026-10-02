# Chromatic and writable cartridge

Toronto Dispatch is an original Game Boy Color homebrew game. The supported target is a ModRetro Chromatic with the writable cartridge supplied with the DevDay Edition. Read the game-specific [loading instructions](../games/toronto-dispatch/docs/LOADING.md) for the build to install, first route and hardware checks.

The [official DevDay quickstart](https://support.modretro.com/en_us/chromatic-devday-edition-quickstart-guid-By1iOlcMg) describes computer activation, emulator play, streamed demos and cartridge writing. Activation happens in the [official ModRetro Updater](https://support.modretro.com/en_us/articles/chromatic-firmware-updater-ryhoYnzCx): Cmd-I on macOS, Ctrl-I on Windows. Enter the supplied code there privately. Do not put it in Codex, a terminal, logs or this repository. Firmware setup follows the updater's own prompts. Updating console firmware and writing this game are separate actions.

## Before writing

Use the ModRetro Chromatic plugin on the computer physically connected to the console. Select this repository's native project, build a ROM, inspect that exact file and test it in the native emulator. No additional vendor driver is required on macOS. Windows and Linux have different runtime/access prerequisites; follow the installed plugin's deployment skill rather than installing an unrelated driver.

Power off before inserting or changing the cartridge. Connect a USB data cable, turn on the console and identify the intended device using the plugin. Discovery is not proof of activation, cartridge support or game installation. Only the intended writable ModRetro cartridge is in scope; a blank cartridge alone does not establish compatibility.

The first write erases the selected cartridge's existing game data, saves may be lost, and the plugin makes no automatic backup. The user must acknowledge this for the selected cartridge before writing. An already empty cartridge still needs to be identified. Later requested writes on that same identified device can reuse the acknowledgement; a different or ambiguous device needs a fresh choice.

## Supported deployment and recovery

The direct plugin workflow uses `rom_inspect`, `device` discovery, optional `setup` cartridge detection, and `flash`. It does not require a working browser preview. Match the inspected path, byte count and SHA-256 when writing; retain the original operation ID and follow its status to a terminal result. Device tokens expire and are consumed by detection, streaming or flashing: rediscover after an action before the next one.

An optional `play` demo emulates the ROM on the computer and streams to the console. It writes no cartridge. Physical USB capture shows the console's video, but does not itself identify the installed ROM or measure hardware frame rate. Keep emulator, streaming, vendor write result, read-back result and cartridge cold-boot observations as separate evidence.

For a failed or missing reply, query the original operation's status. Do not automatically retry an uncertain write, delete its journal, reset firmware or substitute another cartridge profile. Follow reported recovery: Developer Mode errors go to ModRetro Updater; connection or contact errors get their indicated checks. A generic programming error does not prove which of those caused it. Keep codes, saves, backups, device identifiers and private preview URLs out of Git.

## Evidence as of 2026-10-02

The user installed plugin 1.0.33. Official authoring dependencies were prepared outside this repo: GB Studio CLI 4.3.2, GBDK 4.5.0 and PyBoy 2.7.0. Native builds and emulator tests are recorded in [BUILD.md](../games/toronto-dispatch/docs/BUILD.md) and [TESTING.md](../games/toronto-dispatch/TESTING.md).

Read-only USB discovery on 2026-10-02 succeeded and returned no connected Chromatic or unmatched USB functions. No live-device demo, cartridge write, activation or firmware update has been performed for this game. Console edition, writable cartridge identity, capacity, activation and physical boot/save/audio remain unverified. A previous browser preview has an unresolved recording-close acknowledgement; preserve that state. The direct supported `flash` workflow remains separate from that preview.

Workflow reference: installed ModRetro Chromatic plugin 1.0.33, `skills/chromatic-deployment/SKILL.md` and `docs/chromatic-device-testing.md`, reviewed 2026-10-02.
