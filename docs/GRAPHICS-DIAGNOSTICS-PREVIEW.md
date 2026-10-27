# Graphics diagnostics preview

Local follow-up to 0.13.0; not a published release. Includes the prior
[HUD repair](HUD-ISSUE-10-PREVIEW.md), [shot-alignment/friendly-fire](HIT-ALIGNMENT-FRIENDLY-FIRE-PREVIEW.md)
and [headshot/campaign](HEADSHOTS-CAMPAIGN-PREVIEW.md) fixes. Existing gameplay defaults are retained.

## Included diagnostics

- OpenGL error checkpoints on every visible overlay pass, separating queued host errors, resource
  uploads, world effects, held weapons, flat HUD and restoration. Records identify code/name/phase/context.
- Scoped KHR_debug/core 4.3 or ARB_debug_output observer when supported: driver source/type/ID/severity,
  phase and bounded message text. Synchronous delivery is limited to the scope; prior callback/user and
  output/synchronous settings are restored, existing filters preserved, and enabled observers forwarded.
- Failure-triggered state snapshots plus sampled matrix-stack and pixel-unpack layout, complementing
  existing pipeline/framebuffer/raster/texture diagnostics. No framebuffer readback in the game loop.
- Immediate accepted-round/model-visibility traces with shot serial, animation/time and skip reasons;
  held-model and buy-preview load errors identify paths and slots.
- Failed D3D9 API operations record HRESULT and state/dimensions/draw parameters: scene/state-block
  ownership, capture/restoration, shaders, render/texture/sampler/transform state, viewport, depth clear,
  draw submission and texture create/lock/unlock operations.
- Existing controller and Army AI exception filters record original exception address/module offsets,
  code/arguments, x86 registers and up to 24 best-effort frame-pointer return addresses. Recovery remains
  owned by the existing runtime. This is neither a process-wide crash handler nor a complete symbolized stack.

## Settings and upgrading

The launcher update inserts `[Logging] GraphicsDebug=true` without replacing existing choices.
Valid values are true/false or 1/0; missing/empty/malformed values use true. F8 in FPS applies it live.
`Detailed=true` retains TRACE summaries, state snapshots and shot/model decisions. With Detailed=false,
enabled graphics ERROR/WARN records and essential handled-fault data remain available.

Duplicate graphics messages share a bounded 64-entry table. The first occurrence is immediate, then
the next occurrence after Logging.IntervalMs (default 1000 ms, range 100–60000) includes suppressed
repeat counts. A new phase/code/message/parameter can log immediately. Suppressed trailing duplicates
are not periodically flushed. Existing rotation defaults to 8 MiB/current file plus three archives.

Save and close Warcraft before updating through the new launcher, then restart. Config-only edits use F8.
Use **Read logs** to export `Game/WarcraftCS/WarcraftCS.log` and relevant numbered archives after reproducing
the problem. Full parameter details and existing diagnostic coverage: [Diagnostic logging](DIAGNOSTICS.md).

## Verification and limits

Hidden real WGL tests inject queued errors between named passes and high/medium driver messages,
verify callback chaining/state restoration across three contexts, disabled-observer error preservation,
Detailed=false ERROR retention, repeat counts and immediate shot/visibility transitions. An actual
Windows SEH exception verifies original context/arguments and successful existing recovery.
The actual D3D9 pixel regression exercises a rejected nested BeginScene call and verifies the HRESULT
log before recovering to successful rendering and native-state restoration. Parser and repeated-install
tests cover defaults, invalid values, F8-style snapshots and preservation of a custom opt-out.

The driver may emit no debug messages in a non-debug context; existing filters can suppress messages.
Checkpoints consume the error queue and identify observed intervals rather than proving root cause.
Synchronous debug delivery and state queries may affect frame time; turn GraphicsDebug off with F8 if
needed. No claim is made that issue #10 is confirmed fixed on the reporter's Radeon or that every game/OS
fault is captured. No new live campaign or Warcraft 1.27a gameplay verification is claimed.

<!-- launcher-summary
- Added per-pass OpenGL errors and supported driver messages for HUD, weapon and effect failures.
- Added failed D3D9 operation/HRESULT diagnostics and immediate shot/model visibility traces.
- Added original exception context, module offsets, registers and best-effort native stack chains for handled faults.
- Enabled GraphicsDebug by default, with F8 reload, preserved custom settings and bounded repeated messages/log files.
- Preserved the previous HUD isolation, headshot alignment, saved friendly-fire and campaign-economy fixes.
-->
