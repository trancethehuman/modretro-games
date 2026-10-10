# Toronto Dispatch ROM distribution

The game's original code, art and content use the repository MIT licence. A compiled ROM also links GBVM and compiler libraries and uses starter-derived font cells. Publish the corresponding notices with the binary. The source repository does not vendor the compiler, emulator or plugin executable.

## Binary bundle

Publish a downloadable archive containing these files together:

| File | Purpose |
| --- | --- |
| `toronto-dispatch.gbc` | Exact native ROM that was built and tested |
| `SHA256SUMS` | SHA-256 of that exact named ROM; verify after packaging/download |
| `LOADING.md` | [Cartridge instructions](LOADING.md) and hardware verification boundaries |
| `ROM_NOTICES.txt` | [Full game, GB Studio, GBVM and starter MIT notices](ROM_NOTICES.txt), plus runtime credits |
| `BUILDINFO.json` or release notes | Source commit/tag, build tool versions/pins, ROM byte count/hash, emulator test evidence and hardware status |

Include the root MIT licence or its full text in `ROM_NOTICES.txt` (already included). Keep the notice and loading text alongside a loose ROM too. Publish the exact source tag/commit with the release so later rebuilds can identify the original assets, engine extension and content. Do not package private saves, backup files, local journals, device tokens, activation codes or preview URLs. ROMs and release archives stay in ignored build/output directories rather than the source Git history.

This archive is a homebrew game distribution. It does not distribute GB Studio, GBDK, PyBoy, Binjgb or the ModRetro plugin themselves. A browser export includes an emulator and requires its separate distribution notices; the native-ROM audit below does not cover redistributing the web player or a development toolchain.

After committing the matching source/docs and inspecting/testing the exact native ROM, use the repository's `scripts/package_rom.py`. Pass `--rom`, `--expected-sha256`, the full `--source-commit` and an ignored `build/<name>.zip` through `--output`. It reads notices, loading text and tool versions from that commit, rewrites loading links to public source permalinks, checks the supplied ROM hash/header, and verifies the resulting six-file archive. Packaging does not rebuild the game or establish source-to-ROM correspondence; retain the official build and native test evidence separately.

## Pinned native runtime audit

Reviewed 2026-10-02 using the installed official plugin toolchain, the project's `musicDriver: "huge"` setting and the current build's linker symbols. Re-audit when changing compiler/engine pins or adding another runtime. [BUILD.md](BUILD.md) records build operation and versions.

| Component | Actual build identity and terms | Binary distribution action |
| --- | --- | --- |
| Original Toronto Dispatch code/art/content | Root MIT, copyright 2026 Hai Nghiem | Include the root notice; keep upstream portions under their own terms |
| GB Studio generated project/build code | CLI 4.3.2, commit `ccb891b2670134ba8237416772eea4ed09d34e1e`; MIT copyright 2019–2026 Chris Maltby | Full notice retained in `project/LICENSE` and `ROM_NOTICES.txt` |
| GBVM engine | Commit `bd6f41cc5e05cbe6601dcc7f8e2db89bed527fe3`, compatibility `4.3.0-e1`; MIT copyright 2020 Toxa | Full [GBVM notice](licenses/GBVM-MIT.txt) included in `ROM_NOTICES.txt` |
| ModRetro starter art and the project's Bench Mono font asset (the in-game HUD/menu font is original since 2026-10-06) | Plugin 1.0.33; MIT copyright 2026 Eric Provencher | Full notice retained in `project/ASSET_LICENSE` and `ROM_NOTICES.txt` |
| hUGEDriver | GBVM-pinned `third-party/HUGE_TRACKER/hUGEDriver.asm` / `lib/hUGEDriver.lib`; linked `_hUGE_init`, `_hUGE_dosound`, `_hUGE_mute_channel` symbols | Upstream [public-domain dedication](https://github.com/SuperDisk/hUGEDriver#license); credit retained in `ROM_NOTICES.txt` |
| GBDK / SDCC runtime libraries | GBDK 4.5.0; GPLv2 with linking exception | The [upstream ROM-distribution guidance](https://github.com/gbdk-2020/gbdk-2020/blob/4.5.0/LICENSE) permits game ROM distribution without GBDK attribution; credit is included. Linking these libraries does not by itself put the game under GPL. The unmodified [linking exception and GPL text](licenses/GBDK-GPLV2-LE.txt) are retained for reference |

The GBDK linking exception concerns a linked game executable. Redistributing the compiler/toolchain or copying/modifying its own source has separate licence/source obligations; no such distribution is made by this native bundle. The project-local engine extension adds original files and does not modify or vendor GBDK library source.

GBT Player is a different GBVM music option and is not selected or present as linked `gbt_*` symbols in this build. If that option is enabled, audit and include its source notice. GBDK's optional ZX0 library decompressor uses BSD 3-Clause with a binary notice requirement; no ZX0 references were found in the current build's linker symbols or project-local engine. A retained [ZX0 BSD notice](licenses/ZX0-BSD3.txt) is available for a future build that links it. The optional crash handler's zlib terms are described in the pinned GBDK licence overview. Verify added features against the actual final link, rather than assuming every file in a development toolchain is in the ROM.

## Audit provenance

- [GB Studio licence at the CLI pin](https://github.com/chrismaltby/gb-studio/blob/ccb891b2670134ba8237416772eea4ed09d34e1e/LICENSE), matched to the installed source licence and retained `project/LICENSE`.
- [GBVM licence at the engine pin](https://github.com/chrismaltby/gbvm/blob/bd6f41cc5e05cbe6601dcc7f8e2db89bed527fe3/LICENSE), copied without modification. Its SHA-256 is `86299a4390a678ce8dd073c24f9e820834601b5ec696b85334fcdd468401f5b1`.
- The installed GBVM hUGEDriver assembly SHA-256 is `a9b3d9a5e3584f029dfc09a33d4b481a38b05955210c5f5956dca5189c116543`; prebuilt library SHA-256 is `5ec32b4e0ca4ea48d795b76e7b86875a60d2b1ee0ed52c63b30e514977682fc6`. These identify the selected toolchain driver, not the game's ROM.
- [GBDK 4.5.0 licence overview](https://github.com/gbdk-2020/gbdk-2020/blob/4.5.0/LICENSE) and installed `licenses/LICENSE_GPLV2_LE`, `LICENSE_SDCC`, `LICENSE_ZX0`, `LICENSE_crashhandler`. Copied text is preserved in the reference files above; the toolchain remains external.
- [ModRetro starter provenance](../project/STARTER_PROVENANCE.json) and [asset licence](../project/ASSET_LICENSE); the original city art and vehicle/pedestrian generators are project-owned.

No official map image, geographic dataset, TTC logo, commercial courier branding or third-party music is included. Factual geography references and any future dataset imports are covered by the root [third-party notices](../../../THIRD_PARTY_NOTICES.md), independently of this native runtime audit.
