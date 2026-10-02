# Native build and preview

Verified 2026-10-02. Select `games/toronto-dispatch/project/project.gbsproj` through the ModRetro Chromatic plugin before issuing project operations. The format is native GB Studio distributed resources (`.gbsproj` / `.gbsres`), not a hand-built interchange stub.

## Tested toolchain

| Component | Version / identity |
| --- | --- |
| ModRetro Chromatic plugin | 1.0.33 |
| GB Studio CLI | 4.3.2, commit `ccb891b2670134ba8237416772eea4ed09d34e1e` |
| GBVM engine | `bd6f41cc5e05cbe6601dcc7f8e2db89bed527fe3` |
| GBDK | 4.5.0, macOS arm64 |
| Python / PyBoy | 3.13.12 / 2.7.0 |
| Pillow | 12.3.0 |
| Node.js | 26.6.0 |

The plugin manages dependencies outside the repo. Start with its setup skill and `toolchain_doctor` for authoring, projectBuild and play. Prepare missing build/emulator components using `toolchain_prepare`; do not modify the installed plugin cache.

## Plugin operations

1. Select the existing native project, inspect its health, then edit native scenes/scripts with the plugin's revision-aware tools.
2. Build using `rom_build` with `outputPath: "build/toronto-dispatch.gbc"`. Paths are relative to the selected project's directory. The plugin invokes GB Studio CLI `make:rom`.
3. Inspect ROM headers and digest. Run `emulator_run` on that exact ROM, then use `emulator_step` / `emulator_observe` to test native frames. Retain recordings in ignored `build/`.
4. Use `web_preview` to create the official GB Studio / Binjgb export (`make:web`) and open its returned URL in Codex's built-in browser. Keep the user preview available; do not reload during human play. Browser export and native ROM may have different digests and must be tracked separately.
5. Run `make check` from the repository root for repository/content validation. It does not compile a ROM.

Builds, browser save states and cartridge backups are ignored. Do not publish local preview capability URLs, personal machine paths, activation codes or saves.

## Current starter scope

The single `Development boot` scene displays two pages identifying this as a setup build, then permits basic directional movement in the supplied workshop. Its base scene uses GB Studio's TOPDOWN type. This verifies the authoring/build/play pipeline only; the accepted angled/isometric presentation and vehicle physics still need an engine feasibility milestone. No music or missions are implemented.

ROM is CGB-only, 64 KiB, MBC5+RUMBLE+RAM+BATTERY with 32 KiB declared RAM. This is compiler output, not verification of the user's cartridge compatibility. Follow [hardware workflow](../../../docs/HARDWARE.md) before any write.
