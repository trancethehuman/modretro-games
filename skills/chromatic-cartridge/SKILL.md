---
name: chromatic-cartridge
description: Prepare and verify supported homebrew ROM streaming and loading onto the user's identified Chromatic development cartridge.
---

Read `docs/HARDWARE.md`, the selected game's `TESTING.md`, and the installed ModRetro plugin's actual hardware skill. The user has authorised loading this project's game when the supported device is connected; preserve that scope.

Identify the console and writable cartridge through supported tools, check Developer Mode readiness, and select a successfully built/tested ROM. Never ask for an activation code in chat or commit device state. User interaction may be needed for activation or physical controls; explain the actual requirement.

Use official plugin capabilities for streaming and cartridge writes. Streaming is an emulator demonstration and does not verify stored cartridge execution. Do not substitute arbitrary firmware flashing, cartridge profiles, or third-party tools when official loading is unavailable.

Record the exact ROM digest, write result, and read-back verification if provided. Keep backups/saves local and ignored except the optional original DevDay ROM archive explicitly authorised in `AGENTS.md`. On 2026-10-04 the user waived the original-ROM preservation prerequisite after learning supported export was unavailable and approved continuing with Toronto Dispatch. No backup-before-flash requirement remains; the original ROM had never been exported, and a `.sav` export does not contain it. Preserve the earlier explicit erase/save-loss/no-automatic-backup consent for the same intended device and action. Have the user perform cartridge boot, gameplay, pause/restart, and audio checks; mark observations as pending until reported. Update the testing record with separate build, emulator, streaming, write, and physical boot evidence.
