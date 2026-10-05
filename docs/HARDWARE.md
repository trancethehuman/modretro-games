# Chromatic and writable cartridge

Toronto Dispatch is an original Game Boy Color homebrew game. The supported target is a ModRetro Chromatic with the writable cartridge supplied with the DevDay Edition. Read the game-specific [loading instructions](../games/toronto-dispatch/docs/LOADING.md) for the build to install, first route and hardware checks.

The [official DevDay quickstart](https://support.modretro.com/en_us/chromatic-devday-edition-quickstart-guid-By1iOlcMg) describes computer activation, emulator play, streamed demos and cartridge writing. Activation happens in the [official ModRetro Updater](https://support.modretro.com/en_us/articles/chromatic-firmware-updater-ryhoYnzCx): Cmd-I on macOS, Ctrl-I on Windows. Enter the supplied code there privately. Do not put it in Codex, a terminal, logs or this repository. Firmware setup follows the updater's own prompts. Updating console firmware and writing this game are separate actions.

## Before writing

Use the ModRetro Chromatic plugin on the computer physically connected to the console. Select this repository's native project, build a ROM, inspect that exact file and test it in the native emulator. No additional vendor driver is required on macOS. Windows and Linux have different runtime/access prerequisites; follow the installed plugin's deployment skill rather than installing an unrelated driver.

Power off before inserting or changing the cartridge. Connect a USB data cable, turn on the console and identify the intended device using the plugin. Discovery is not proof of activation, cartridge support or game installation. Only the intended writable ModRetro cartridge is in scope; a blank cartridge alone does not establish compatibility.

The first write erases the selected cartridge's existing game data, saves may be lost, and the plugin makes no automatic backup. The user must acknowledge this for the selected cartridge before writing. An already empty cartridge still needs to be identified. Later requested writes on that same identified device can reuse the acknowledgement; a different or ambiguous device needs a fresh choice.

On 2026-10-04 the user requested that the original DevDay cartridge ROM be backed up and checked into this repository as a historical artifact before replacement. After learning that supported ROM export was unavailable, the user waived that prerequisite and approved proceeding with Toronto Dispatch. The original ROM was never exported. The optional archive remains an exception to the usual backup exclusion, with provenance and a SHA-256 in [history/openai-devday-cartridge](../history/openai-devday-cartridge/README.md); ordinary saves, device identifiers and activation state stay local.

The official [ModRetro Updater](https://support.modretro.com/en_us/chromatic-firmware-updater-ryhoYnzCx) provides **Cart Clinic → Backup** for save data (`.sav`). This does not export the game ROM. The installed plugin's public tools and bundled vendor CLI 1.2.1 expose no ROM-export command. Do not replace an original-ROM backup with a save file or a similarly named downloaded game.

## Supported deployment and recovery

The direct plugin workflow uses `rom_inspect`, `device` discovery, optional `setup` cartridge detection, and `flash`. It does not require a working browser preview. Match the inspected path, byte count and SHA-256 when writing; retain the original operation ID and follow its status to a terminal result. Device tokens expire and are consumed by detection, streaming or flashing: rediscover after an action before the next one.

An optional `play` demo emulates the ROM on the computer and streams to the console. It writes no cartridge. Physical USB capture shows the console's video, but does not itself identify the installed ROM or measure hardware frame rate. Keep emulator, streaming, vendor write result, read-back result and cartridge cold-boot observations as separate evidence.

For a failed or missing reply, query the original operation's status. Do not automatically retry an uncertain write, delete its journal, reset firmware or substitute another cartridge profile. Follow reported recovery: Developer Mode errors go to ModRetro Updater; connection or contact errors get their indicated checks. A generic programming error does not prove which of those caused it. Keep codes, saves, ordinary backups, device identifiers and private preview URLs out of Git; the original-ROM archive above is the explicit exception.

## Campus and scooter R8 installation — 2026-10-05

The official Chromatic Firmware Updater 1.7.4+2f7c620 reported success. The user
confirmed completion of its battery/USB reset and the same writable DevDay cartridge
before fresh USB enumeration selected the sole intended Chromatic, with zero
conflicts, unmatched functions or diagnostics. No fresh cartridge-capacity detection
was performed; the earlier supported detection established 4 MiB.

The [supported installation record](../games/toronto-dispatch/docs/CARTRIDGE_CAMPUS_SCOOTERS_INSTALL_2026_10_05.json)
binds `toronto-dispatch-campus-scooters-r8.gbc`, 1,048,576 bytes, SHA-256
`bdcebd6ff4463cb745f2fe47255119b381d5a098e57a8ba27d5652bfc2a97b0d`.
Vendor CLI 1.2.1 reported success after 60,380 ms and exited/closed with code 0;
the writer closed at 15:50:54.889 UTC and the operation completed at
15:50:54.892 UTC. Exactly one plugin write request was made, with no automatic
retry. No separate complete cartridge read-back digest was supplied.

The user confirmed R8 boots after power-off, USB disconnection and power-on with
the cartridge inserted, and responds to physical buttons. Extended physical
gameplay/performance, power-off save persistence, audio and flicker remain pending.
The prior 9c155 baseline, earlier zero-device discoveries and frozen build/package/
native evidence remain historical with their own identities. Raw device details,
activation state, saves and operation journals remain local and excluded from Git.

## Cartridge preservation evidence as of 2026-10-04

The connected Chromatic was discovered without diagnostics. Supported cartridge detection succeeded: 4 MiB ISSI flash, current title `OPENAI`, declared 128 KiB ROM and 32 KiB save RAM. Detection did not write a game. Toronto Dispatch's inspected 1 MiB ROM fits; this does not establish Developer Mode activation or cartridge boot.

Updater 1.7.4's Cart Clinic exported a 32,768-byte save file into the ignored local `backups/` directory. Its SHA-256 is `c35020473aed1b4642cd726cad727b63fff2824ad68cedd7ffb73c7cbd890479`; all exported bytes are zero. Export completion and file contents were checked; restore has not been tested. This file is not the original ROM. The user subsequently waived original-ROM preservation, so its export and archival commit are no longer installation requirements.

## Physical cartridge installation — 2026-10-04

The supported plugin installed the exact 1,048,576-byte `toronto-dispatch-driving-only.gbc`, SHA-256 `8be1eff05236edfd05e475fe143f5b48e20db8d4a09c30f46e038e7d9643bdfb`, on the freshly reselected intended cartridge. Vendor CLI 1.2.1 reported success after 49,679 ms and closed with exit code 0; no write retry was made. No separate complete cartridge read-back digest was reported. See the [sanitised installation record](../games/toronto-dispatch/docs/CARTRIDGE_INSTALL_2026_10_04.json); raw operation journals remain local and ignored.

After the completed write, the user confirmed that Toronto Dispatch appears and responds to physical buttons after powering off, disconnecting USB and powering on with the cartridge inserted. This establishes a user-reported cartridge cold boot and button response. Gameplay, power-off save persistence, audio and hardware performance remain unverified.

## Hardware feedback retry installation — 2026-10-04

After reconnecting the intended console, the user explicitly requested a retry of the same inspected/native-tested 1,048,576-byte 9c155 ROM. Fresh enumeration matched the intended device; no fresh cartridge-capacity detection was performed. Vendor CLI 1.2.1 reported success after 52,404 ms, and the writer exited/closed with code 0. The operation finished at 23:02:07.282 UTC. See the [sanitized successful retry](../games/toronto-dispatch/docs/CARTRIDGE_UPDATE_HARDWARE_FEEDBACK_RETRY_2026_10_04.json).

The first failure below remains historical. These were two separately authorized plugin write requests, with no automatic plugin retry. The vendor supplied no complete cartridge read-back digest and did not verify manual boot/play. The user confirmed the new ROM boots and responds to physical buttons after powering off, disconnecting USB and powering on with the cartridge inserted. Extended physical gameplay, save persistence, audio and flicker remain pending. Source, ROM files, frozen reviewed ZIPs and prior records remain unchanged; private journals and device details remain local and ignored.

## First hardware feedback update attempt — 2026-10-04

The supported plugin attempted to write the inspected/native-tested 1,048,576-byte `toronto-dispatch-hardware-feedback.gbc`, SHA-256 `9c155a70c0cc3d6ccec986fc0ab7ddc3a204fd04e80879ad0233926156d4b10e`, to the freshly selected intended device. Vendor CLI 1.2.1 returned a terminal `flash.write_failed` error and closed with exit/process-close code 1. Its diagnostic reports a USB bulk IN transfer timeout and absent end-of-attempt fence during phase `drawing`. Three internal vendor communication attempts belonged to one plugin write request; no automatic additional flash request was dispatched.

See the [sanitized failed-attempt record](../games/toronto-dispatch/docs/CARTRIDGE_UPDATE_HARDWARE_FEEDBACK_2026_10_04.json). Cartridge contents were unverified after this first attempt; the earlier 8be1 was the last user-confirmed physical game. No complete read-back or new-ROM cold boot was claimed. Recovery required the user's power-off/reconnection and explicit retry request, followed by fresh device discovery; that separate successful retry is recorded above. Raw device identities, tokens and journals remain private and ignored.

## Evidence as of 2026-10-02

The user installed plugin 1.0.33. Official authoring dependencies were prepared outside this repo: GB Studio CLI 4.3.2, GBDK 4.5.0 and PyBoy 2.7.0. Native builds and emulator tests are recorded in [BUILD.md](../games/toronto-dispatch/docs/BUILD.md) and [TESTING.md](../games/toronto-dispatch/TESTING.md).

Read-only USB discovery on 2026-10-02 succeeded and returned no connected Chromatic or unmatched USB functions. No live-device demo, cartridge write, activation or firmware update has been performed for this game. Console edition, writable cartridge identity, capacity, activation and physical boot/save/audio remain unverified. A previous browser preview has an unresolved recording-close acknowledgement; preserve that state. The direct supported `flash` workflow remains separate from that preview.

Workflow reference: installed ModRetro Chromatic plugin 1.0.33, `skills/chromatic-deployment/SKILL.md` and `docs/chromatic-device-testing.md`, reviewed 2026-10-02.
