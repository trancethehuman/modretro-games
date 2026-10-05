# Toronto Dispatch cloud continuation — 2026-10-05

Continue the unfinished crossing artwork, framed-menu and city-performance work
on `codex/visible-queen-streetcar`. The source checkpoint was based on pushed
commit `1068285b968a12d1b4892d4da35e14116f144261`; this handoff commit preserves
the 30 modified tracked files and 19 previously untracked source/art files.
The original local continuous goal is paused. Use GPT-6.1 Sol with Ultra
reasoning for the cloud continuation.

## Current implementation

- Six mainland backgrounds repaint 2,833 crossing cells with separated bars.
  Stop bars, signals, road/collision/priority geography and non-crossing pixels
  remain protected. Islands are unchanged; canonical-pattern count is unchanged.
- Original fullscreen menu frames/cards add three bank-1 patterns, using 48
  bytes and no additional static WRAM. Preserve the compact city HUD, controls,
  saves, settings, sound, map, missions and transit.
- Rider admission and scenery row prefixes reduce repeated work while retaining
  all eight fleet slots, eight pedestrians, two scooter riders and live guards.
  Original-R8 differential tests covered 44,800 update snapshots with identical
  state results. Population, save format and update cadence are unchanged.

## Verification and limits

Focused crossing, world, campus, traffic, UI, scenery and scooter checks passed
at the checkpoint. The first full `make check` failed because historical
metadata pinned the predecessor crossing PNG. A narrow predecessor-reconstruction
proof was added and focused checks then passed; the complete suite still needed
its post-fix rerun when the original task paused.

The official R9 build completed and four compiled resource guards passed.
All 247 native input hashes in `CROSSING_MENU_R9_SOURCE_PINS.json` matched the
completed build. R9 ROM SHA-256:
`52d88c0677a99227b6edb63b26e0b86c5df081d0d412e4453bea577b055c01d9`.

Matched native ordinary-button measurements showed R8 idle 100/840 and active
23/240 completed loops, versus R9 idle 107/840 and active 26/240. The 7% idle
gain is insufficient for the reported lag. These counts are not display FPS
or proof of physical smoothness. **R9 is not selected or eligible for flashing.**
R8 remains installed; its cold boot and physical buttons were confirmed.

Temporary component-profiling edits were preserved privately and restored
before this checkpoint. They are unbuilt and excluded from this commit.
Generated ROM/debug artifacts and private recordings remain outside Git.

## Resume

1. Read root `AGENTS.md`, game `DESIGN.md`, `ROADMAP.md`, `DECISIONS.md`,
   `TESTING.md`, and the local and installed ModRetro skills.
2. Install `requirements-dev.txt` as needed and run `make check`. Native
   build/play requires the ModRetro plugin: select
   `games/toronto-dispatch/project/project.gbsproj`, run
   `toolchain_doctor({tasks:["projectBuild","play"]})`, and prepare missing
   build/emulator components. Recorded toolchain versions are GB Studio CLI
   4.3.2, GBDK 4.5.0 and PyBoy 2.7.0. Recreate Linux tools rather than copying
   Mac binaries or environments.
3. Profile the remaining native city-loop cost. One proposed optimization
   authenticates the external sandbox rider move batch before omitting only
   duplicated fleet checks, while retaining live parked-scooter, other-rider
   and wreck/ram fallback checks. This prototype is not integrated.
4. Keep all people, vehicles, artwork and guards. Build a distinct R10 candidate
   with pre-build pins and matching debug files. Repeat matched idle/driving,
   crowded and scrolling workloads; inspect native crossings and
   Pause→Settings→Guide→Map→Resume, held inputs, Save and reset.
5. Select/package only a substantially faster verified candidate with matching
   committed source. Local USB discovery and separate cartridge consent/testing
   are required for any later hardware operation. Cloud has no direct access
   to the laptop's USB device. Physical performance, audio, save persistence
   and readability remain separate acceptance checks.

This checkpoint is unfinished work. It authorizes no merge, deployment or
cartridge operation by itself.
