# Chromatic and writable cartridge

Toronto Dispatch is an original Game Boy Color homebrew game. The supported target is a ModRetro Chromatic with the writable cartridge supplied with the DevDay Edition. Read the game-specific [loading instructions](../games/toronto-dispatch/docs/LOADING.md) for the build to install, first route and hardware checks.

The [official DevDay quickstart](https://support.modretro.com/en_us/chromatic-devday-edition-quickstart-guid-By1iOlcMg) describes computer activation, emulator play, streamed demos and cartridge writing. Activation happens in the [official ModRetro Updater](https://support.modretro.com/en_us/articles/chromatic-firmware-updater-ryhoYnzCx): Cmd-I on macOS, Ctrl-I on Windows. Enter the supplied code there privately. Do not put it in Codex, a terminal, logs or this repository. Firmware setup follows the updater's own prompts. Updating console firmware and writing this game are separate actions.

## Before writing

Use the ModRetro Chromatic plugin on the computer physically connected to the console. Select this repository's native project, build a ROM, inspect that exact file and test it in the native emulator. No additional vendor driver is required on macOS. Windows and Linux have different runtime/access prerequisites; follow the installed plugin's deployment skill rather than installing an unrelated driver.

Power off before inserting or changing the cartridge. Connect a USB data cable, turn on the console and identify the intended device using the plugin. Discovery is not proof of activation, cartridge support or game installation. Only the intended writable ModRetro cartridge is in scope; a blank cartridge alone does not establish compatibility.

The first write erases the selected cartridge's existing game data, saves may be lost, and the plugin makes no automatic backup. The user must acknowledge this for the selected cartridge before writing. An already empty cartridge still needs to be identified. Later requested writes on that same identified device can reuse the acknowledgement; a different or ambiguous device needs a fresh choice.

On 2026-10-04 the user requested that the original DevDay cartridge ROM be backed up and checked into this repository as a historical artifact before replacement. This specific archive is an exception to the usual backup exclusion. Preserve it with provenance and a SHA-256 in [history/openai-devday-cartridge](../history/openai-devday-cartridge/README.md); ordinary saves, device identifiers and activation state stay local. Installation is paused until the original ROM is preserved or the user changes that instruction.

The official [ModRetro Updater](https://support.modretro.com/en_us/chromatic-firmware-updater-ryhoYnzCx) provides **Cart Clinic → Backup** for save data (`.sav`). This does not export the game ROM. The installed plugin's public tools and bundled vendor CLI 1.2.1 expose no ROM-export command. Do not replace an original-ROM backup with a save file or a similarly named downloaded game.

## Supported deployment and recovery

The direct plugin workflow uses `rom_inspect`, `device` discovery, optional `setup` cartridge detection, and `flash`. It does not require a working browser preview. Match the inspected path, byte count and SHA-256 when writing; retain the original operation ID and follow its status to a terminal result. Device tokens expire and are consumed by detection, streaming or flashing: rediscover after an action before the next one.

An optional `play` demo emulates the ROM on the computer and streams to the console. It writes no cartridge. Physical USB capture shows the console's video, but does not itself identify the installed ROM or measure hardware frame rate. Keep emulator, streaming, vendor write result, read-back result and cartridge cold-boot observations as separate evidence.

For a failed or missing reply, query the original operation's status. Do not automatically retry an uncertain write, delete its journal, reset firmware or substitute another cartridge profile. Follow reported recovery: Developer Mode errors go to ModRetro Updater; connection or contact errors get their indicated checks. A generic programming error does not prove which of those caused it. Keep codes, saves, ordinary backups, device identifiers and private preview URLs out of Git; the original-ROM archive above is the explicit exception.

## Cartridge preservation evidence as of 2026-10-04

The connected Chromatic was discovered without diagnostics. Supported cartridge detection succeeded: 4 MiB ISSI flash, current title `OPENAI`, declared 128 KiB ROM and 32 KiB save RAM. Detection did not write a game. Toronto Dispatch's inspected 1 MiB ROM fits; this does not establish Developer Mode activation or cartridge boot.

Updater 1.7.4's Cart Clinic exported a 32,768-byte save file into the ignored local `backups/` directory. Its SHA-256 is `c35020473aed1b4642cd726cad727b63fff2824ad68cedd7ffb73c7cbd890479`; all exported bytes are zero. Export completion and file contents were checked; restore has not been tested. This file is not the original ROM. No cartridge flash has been dispatched, and the original game remains on the cartridge. Original-ROM export, archival commit and Toronto Dispatch's physical boot/save/audio checks remain pending.

## Evidence as of 2026-10-02

The user installed plugin 1.0.33. Official authoring dependencies were prepared outside this repo: GB Studio CLI 4.3.2, GBDK 4.5.0 and PyBoy 2.7.0. Native builds and emulator tests are recorded in [BUILD.md](../games/toronto-dispatch/docs/BUILD.md) and [TESTING.md](../games/toronto-dispatch/TESTING.md).

Read-only USB discovery on 2026-10-02 succeeded and returned no connected Chromatic or unmatched USB functions. No live-device demo, cartridge write, activation or firmware update has been performed for this game. Console edition, writable cartridge identity, capacity, activation and physical boot/save/audio remain unverified. A previous browser preview has an unresolved recording-close acknowledgement; preserve that state. The direct supported `flash` workflow remains separate from that preview.

Workflow reference: installed ModRetro Chromatic plugin 1.0.33, `skills/chromatic-deployment/SKILL.md` and `docs/chromatic-device-testing.md`, reviewed 2026-10-02.
