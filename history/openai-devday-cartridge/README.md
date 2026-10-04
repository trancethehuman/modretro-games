# Original DevDay cartridge

The user requested on 2026-10-04 that the original game supplied on their ModRetro DevDay cartridge be preserved in this repository as a historical artifact before Toronto Dispatch replaces it.

**The original ROM has not been exported or committed.** It remains on the cartridge. This directory currently records the preservation request and observed header; it is not a playable backup.

## Observed cartridge header

The official plugin's supported cartridge detection reported these fields on 2026-10-04:

| Field | Reported value |
| --- | --- |
| Title | `OPENAI` |
| Declared ROM size | 131,072 bytes (128 KiB) |
| CGB support | Enhanced (`0x80`) |
| Cartridge type | MBC5 + RUMBLE + RAM + BATTERY (`0x1e`) |
| Declared RAM size | 32,768 bytes (32 KiB) |
| Header checksum | Valid, 244 (`0xf4`) |
| Stored global checksum | 29,895 (`0x74c7`) |

These fields do not establish a complete ROM digest. The header title alone does not identify the game as Codex Land.

## Preservation procedure

1. Obtain the exact original cartridge ROM through a supported export, or an authoritative original file with sufficient identity evidence. The current plugin and vendor CLI expose no ROM export.
2. Inspect its header, size and SHA-256. Save the untouched ROM as `original-openai.gbc`, and add `SHA256SUMS` and provenance documenting how it was obtained and what was verified.
3. Retain the original artifact's licence and attribution. The collection's MIT licence does not relicense a third-party demo.
4. Commit the original ROM and provenance before replacing the game. Keep saves, activation state, raw device journals and identifiers outside the archive.

The official [Updater backup](https://support.modretro.com/en_us/chromatic-firmware-updater-ryhoYnzCx) exports `.sav` data, which does not contain the ROM. That available backup was saved locally and excluded from Git.

The downloadable [Codex Land](https://developers.openai.com/modretro/codex-land) reference ROM was inspected separately: it is 262,144 bytes, titled `CODEXLAND`, with a different header checksum and CGB flag. It is not an identity match for this cartridge and has not been added to this archive.
