---
name: modretro-development
description: Create and iterate on this collection's original Chromatic games with the ModRetro GB Studio plugin, ROM builds, and gameplay verification.
---

Read root `AGENTS.md` and the selected game's design, decisions, roadmap, and testing record. Keep each game's editable project and assets in its own folder.

Discover and read the installed ModRetro Chromatic plugin's actual skills before invoking it. Use its project creation/selection, asset audit, build, emulator preview, and automated playtest tools. Inspect existing dependencies before installation and record exact versions and commands once known.

If unavailable, continue independent content and repository work, state the missing capability, and request plugin attachment. Do not invent tool calls, GB Studio schemas, build commands, or a playable ROM. Do not replace the user's cartridge target with a web game.

For Toronto Dispatch, preserve the user's revised north-up top-down presentation, vehicle momentum/braking, wide roads, pedestrians and walking/car-entry animations. The native TORONTO scene extension implements these mechanics. Match its actual collision grid to original artwork; verify roof/canopy background priority, glancing contact, reverse recovery and reachable endpoints. Use the campaign validator and native playtests before expanding the map.

Build a small milestone, audit its hardware limits, run the corresponding emulator scenarios, and update `TESTING.md` with observed results. Keep scaffold checks, compilation, emulator playback, device streaming, and cartridge execution distinct. Consult `docs/HARDWARE.md` only when proceeding to the device workflow. When the task is done, send the human several screenshots of the built ROM (title, gameplay, the changed feature, before/after where useful) and name the build they show; list any emulator memory writes used to stage a frame in the screenshot provenance.

`make check` includes host-C behavioral regressions for the actual Toronto scene engine through bounded hardware stubs. Use these for driving, clock, input and interrupted-save logic; they do not establish native ABI, rendering, CPU performance or physical persistence. Rebuild with the official plugin and repeat the corresponding native button scenario after runtime edits.

Toronto Dispatch runs close to several fixed budgets: the actor sprite sheet uses all 128 8x16 VRAM tiles (count unique flip-aware tiles before adding art), `td_ui.c` is near its 16 KB ROM bank, and the stack reserve guard needs 1,024 bytes. When a GB Studio CLI build fails with only an exit code, rerun its link step (`lcc` with the build folder's `obj/linkfile.lk`) to see `BankPack` overflows. Per-actor colours rely on `engine/src/core/actor.c.patch`; confirm it still applies after an engine upgrade.
