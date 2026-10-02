# Browser preview lifecycle failure

Local maintainer report, 2026-10-02. No external report or recovery action has been performed. This concerns the installed ModRetro plugin `1.0.33+codex.distribution.66e8961d3ce30093ea3c9ee4`, separate from native ROM testing.

## Observed evidence

The user’s older isometric preview kept advancing frames while browser input reported `ready:false`, reason `initializing`. **Restart game** failed with dialog category `player-action`; that category alone does not identify the underlying error. Record was unchecked, and no original recording job ID was available. Two earlier `web_preview_close` attempts returned an uncertain outcome.

| User preview identity | Value |
| --- | --- |
| ROM SHA-256 | `a3249acdebf3e840592528e02a719f636f5d77273e287fc112a9deafeb9d81fd` |
| Runtime SHA-256 | `69aa17be45fec5fde585af4f39ed850b5d64107ec094a6482f316fef2fe7c945` |
| Preview revision | `d6f17afcdfeb6dbc1e840926cc4148f0589c116637dc91c852c2467462be175d` |

Latest public status read on 2026-10-02: listener `9dac3b44aa967a830d7f9fe33794fbe7`, started `2026-10-02T04:11:49.386Z`, `listening:true`, `closureState:"unresolved"`, `players:[]`, and `closeError:"Browser recording close acknowledgement is UNKNOWN."` No precise status-read time was supplied. The browser’s older observation and this later empty player list are separate observations; neither proves a ROM crash.

## Confirmed implementation trap

Read-only inspection of installed public source establishes:

- Close requires acknowledgement of a fresh close epoch from **every remembered recording-capable view**, including idle views with no recording. The broker removes a view only through acknowledged retirement, without ordinary stale-view expiry. A vanished older view is a plausible cause of the original timeout, **not a confirmed cause of this incident**.
- UNKNOWN retains the close epoch and original deadline. The outer listener also retains its expired `drainDeadline`; even a later successful acknowledgement encounters that expired deadline. The listener remains unresolved and blocks replacement previews. This is the confirmed recovery trap.
- `initializing` describes the browser control bridge, while emulation can continue. Restart refuses an uninitialized bridge. `player-action` is a generic error wrapper. The original dialog’s full **Problem**/Copy error report is needed for a more specific restart diagnosis.

Source root is the exact installed package under `$CODEX_HOME/plugins/cache/openai-curated-remote/modretro-chromatic/1.0.33+codex.distribution.66e8961d3ce30093ea3c9ee4/`. References beneath that root: `dist/web-preview-recording.js:177–206,367–408` (views/close epoch); `dist/web-preview.js:1243–1246,1294–1334` (replacement gate/retained deadline); `dist/web-annotations/remote.js:113–127,212–218,296–307` (initialization/restart); `dist/web-annotations/player.js:214–229` (error category). `docs/agent-guide.md:683–685` explicitly provides no reset or reconciliation shortcut; `skills/modretro-chromatic-rom-debugging/SKILL.md:36–49` limits supported recovery to the current owner or a verified export when no live preview exists.

## Bounded recovery recommendation

Preserve the original error/status evidence and any available downloaded captures or saves. Quit and reopen Codex to attempt a fresh plugin process, then inspect public `web_preview` status before opening anything. Only when no live conflicting owner or unresolved capture remains, select this project and use supported `recover` for an unchanged verified export, or normal `open` if a new export is needed. Stop on unresolved ownership; do not loop on close/reload, force rebuild, edit journals, or kill servers. This recommendation has **not been attempted or verified** and does not recover unsaved browser progress.

For plugin maintainers: reproduce idle-view disappearance followed by close timeout, then test late acknowledgement and explicit retry. Provide an observable, ownership-preserving reconciliation path for a timed-out close without silently clearing uncertain recordings or saves.
