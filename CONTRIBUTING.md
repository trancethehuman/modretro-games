# Contributing

Read `AGENTS.md` and the relevant game's accepted decisions before starting. Use a focused branch and pull request. Explain the player-visible change and include the checks actually performed.

Each game belongs in `games/<game-slug>/` and should have a README, design, roadmap, decisions, and testing record. Share tooling only when multiple games actually need it.

Preserve editable source assets. Include provenance and licence terms for external assets or data. Do not submit commercial ROMs, copied game artwork, credentials, activation codes, or device backups.

Run `make check`. For gameplay changes also build the ROM, run the relevant emulator scenarios, and report hardware testing separately. A scaffold validation pass is not a ROM test.
