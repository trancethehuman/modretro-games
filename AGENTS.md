# ModRetro Games

This repository contains original homebrew games for ModRetro Chromatic / Game Boy Color. Each game lives in `games/<game-slug>/`. Shared tooling belongs in `scripts/`; reusable project workflows belong in `skills/` (also exposed at `.agents/skills`).

## Working agreements

- Read the current game's `DESIGN.md`, `ROADMAP.md`, and `DECISIONS.md` before changing gameplay or architecture. Update decisions when the user changes direction; distinguish accepted decisions from proposals.
- Use the ModRetro Chromatic plugin for GB Studio project creation, ROM builds, playable previews, automated playtests, device streaming, and cartridge loading. Check available skills and tools first. If unavailable, report that limitation and continue independent repository/content work. Do not substitute a browser game or claim a JSON content file is a playable game.
- Keep editable GB Studio projects, original assets, and any engine extensions inside the game's folder. Keep generated ROMs, toolchains, local activation state, and device backups out of Git.
- Target actual cartridge execution. A browser preview or streamed emulator session does not prove the cartridge boots. Keep build, emulator, streaming, flash verification, and manual hardware checks as separate evidence.
- Build small playable milestones and verify them before expanding the city. Check hardware palette, tile, sprite, memory, and performance limits using the installed plugin and authoritative documentation.
- For Toronto Dispatch, preserve the accepted straight, north-up top-down view (user revision 2026-10-02), mixed courier jobs (packages first), momentum, braking, and traffic rules. Roads must be wide enough for readable driving. Match building collision footprints to artwork, use native roof/canopy occlusion, and preserve visible walking and car-entry behaviour. Record material changes.
- Use official Toronto/TTC sources for geography and transit. Record sources and dates; separate source-derived facts from compressed map design. Real neighbourhoods, landmarks, rail corridors, and street names must remain recognisable and geographically coherent.
- Use original art, music, characters, and fictional courier branding. Track third-party licences in `THIRD_PARTY_NOTICES.md`; the root MIT licence does not override their terms.
- Never commit activation codes, credentials, machine configuration, cartridge backups, saves, or private information. Device writing must target the user's identified writable development cartridge using the official supported path. Never infer permission to alter unrelated console firmware or another cartridge.
- Inspect Git status before committing. This folder has its own Git repository; do not stage its parent directory. Use `codex/` for new feature branches. The user authorised a public GitHub repository and MIT licence.
- Run `make check` for repository/content edits and the plugin's build/playtests for game edits. Keep a concise account of what is actually verified and what remains pending.
- Use plain, concise communication without exclamation marks. Ask gameplay questions as useful choices while continuing work that does not depend on the answers.

## Workflow skills

- `skills/modretro-development/SKILL.md`: GB Studio creation, iteration, and evidence.
- `skills/toronto-worldbuilding/SKILL.md`: geography, transit, and playable map compression.
- `skills/chromatic-cartridge/SKILL.md`: supported device testing and cartridge loading.

Project-local rules and decisions are the durable memory for this collection. Update them as part of development when the user adopts a rule; do not store private hardware activation details.
