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

## City engine and reproducible sources

The start scene uses the original project-local `TORONTO` scene extension, compatible with GBVM `4.3.0-e1`. It adds engine files without ejecting or vendoring GB Studio. `Development boot` remains a separate workshop scene.

From the repository root, with Python and Pillow available:

```sh
python3 games/toronto-dispatch/scripts/create_city_art.py
python3 games/toronto-dispatch/scripts/create_campaign.py
python3 games/toronto-dispatch/scripts/sync_city_resources.py
make check
```

`create_city_art.py` draws original indexed-colour background and sprite source cells; the background is already registered as a native asset. The sprite generator produces an editable source/metadata pair in `original-art` / `dispatch_topdown.metadata.json`. Existing sprite PNG changes must be applied to the registered `assets/sprites/dispatch_topdown.png` as part of a deliberate sprite edit. New sprite registration uses the plugin's validated `native_metadata` import; keep the existing root and bindings when editing an established asset.

`sync_city_resources.py` uses GB Studio's native byte-array RLE to write palette/background-priority attributes and the scene collision map. Native CGB attribute bit 7 marks raised roof lips/canopies. The scene extension interprets collision values 0 as road, 16 as walk-only pavement/Island ground and 15 as solid; the normal engine ladder meaning of bit 4 does not apply to this custom scene. The map has 15,616 tiles, so each tile/attribute/collision array fits one 16 KiB bank. `check_campaign.py` reads these actual native resources and proves stop connectivity, compatible road routes, ferry links and generated contract consistency.

Car physics stores Q4 coordinates and smoothed velocity; GBVM actors/camera use Q5. A bounded video-frame delta advances simulation between render updates. Menus freeze the world clock. Save version 4 occupies a checksummed record at SRAM bank 3, offset 0x100; GB Studio uses banks 0–2. Older prototype saves are rejected. Emulator soft-reset persistence is verified; physical cold-boot persistence and power-loss recovery remain pending.

Current ROM: CGB-only, 256 KiB, MBC5+RUMBLE+RAM+BATTERY with 32 KiB declared RAM. Header validity is verified, cartridge compatibility is not. No audio is implemented yet. Follow [hardware workflow](../../../docs/HARDWARE.md) before any write.

## Browser refresh limitation

A previous official web export compiled, but refreshing the owned preview failed with `Browser recording close acknowledgement is UNKNOWN`. The listener retained unresolved closure state; its paused starter save and local recording evidence were preserved. Native gameplay testing proceeded independently. Do not arbitrarily reload or replay that unresolved close operation. Resolve the plugin's preview state before treating the current city ROM as browser-verified. The new top-down ROM has not been verified in that browser view.
