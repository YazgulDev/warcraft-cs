# Diagnostic logging (test branch)

Both source-only and DLL-included launchers have **Read logs**. It works before installation,
without download consent and while setup or Warcraft is running. The selected installation folder
determines which game's logs appear. The current game journal opens first when available.

The reader offers file selection, Refresh, optional automatic refresh every two seconds, Copy text
and Save full log. It displays a bounded 512 KiB tail for large files; Save full log exports the entire
current file snapshot. Copy text copies the displayed tail. Refresh also discovers newly created
journals. Reading does not edit logs or stop the game. An empty list explains when logs will appear.

| Journal | Location |
| --- | --- |
| Current game | `Game/WarcraftCS/WarcraftCS.log` in a launcher installation |
| Previous game sessions/segments | `Game/WarcraftCS/WarcraftCS.1.log`, `.2.log`, etc. |
| Installer output | `install.log` in the selected installation |
| Launcher, including failures before installation | `%LOCALAPPDATA%/WarcraftCSLauncher/launcher.log` and numbered archives |
| Launcher replacement | `updates/<revision>/launcher-replacement.log` in the installation |

Source installs use the corresponding `WarcraftCS/` folder inside their prepared runtime. Select
that runtime folder in the launcher's folder field to read its source-game logs; this read-only action
does not install anything. Prior runtime sessions are now archived rather than overwritten at startup.

Runtime records include a UTC timestamp, monotonic tick, severity, process and thread identifiers.
Startup records the version, compiled source/config fingerprint, build time and actual Windows kernel
build. Logs cover native binding/hooks, map/FPS/window/pause lifecycle, validated settings, control
edges, existing weapon/reload/hit/item/buy/squad/audio events and faults. Sampled summaries add physics,
camera, player status, render frequency, world-frame age, focus, context, viewport, color/stipple state
and reticle submission decisions. OpenGL vendor/renderer/version identify the driver used by the game.
An observed GL error may originate in the host frame; a submission record does not prove visible pixels.

Dedicated `reticle draw begin`, `reticle state` (`inherited` / `prepared`) and `reticle draw end`
records surround the actual hip-fire/scope primitives. They include center, recoil gap or scope radius,
thickness, primitive/vertex count, context, viewport/scissor, color masks/color, polygon/stipple,
logic operation, depth/stencil/alpha/blend/cull and texture state. Sampled GL errors already pending
are labeled `before-draw-host`; errors observed after geometry and state restoration are labeled
`draw-and-restore`. These diagnostics consume the sampled GL error queue and do not read GPU pixels.
`reticle skipped` explains inactive FPS, missing context/client area, contained/hidden units,
stale world frames or controller faults. Mode/skip/context transitions are immediate; repeated
records follow `IntervalMs`. `Detailed=false` disables these optional checks and records.

Only mod control keys/buttons are traced; raw character text and mouse-motion packets are not written.
Launcher records include startup/exit, version/variant, install/update/Play results, backend output and
managed exception stacks. Launcher journals are limited to 4 MiB each with three archives. Oversized
individual native/launcher messages are marked as truncated. Logging is local; exporting is a user action.

Edit the existing private `WarcraftCS.ini`; F8 in FPS or restart applies the validated snapshot:

| `[Logging]` key | Default | Range/meaning |
| --- | --- | --- |
| `Detailed` | `true` | `true`/`false` (also `1`/`0`). Disable optional TRACE records; essential events and reported faults remain. |
| `IntervalMs` | `1000` | 100–60000 milliseconds between movement/render summaries. Control/action edges remain event driven. |
| `MaxFileMB` | `8` | 1–64 MiB per runtime journal; size rollover starts a new segment. |
| `ArchiveCount` | `3` | 0–8 older sessions/segments. Reducing it removes surplus numbered runtime archives; zero retains only the current file. |

Missing/malformed keys use defaults, numeric values are clamped, and updates add missing keys without
replacing custom values or duplicating sections. Default runtime retention uses at most 32 MiB across
the current journal and three archives. Installer transcripts and older existing replacement logs
retain their existing storage policy. Logs can fail to persist when the disk/path is unavailable.

For a missing-crosshair report, reproduce with the affected weapon and attach the current game log,
an FPS screenshot, launcher version/mode, GPU/driver and map/edition. The source fingerprint distinguishes
different test DLLs carrying the same version number. These diagnostic changes are on a test branch;
the published 0.6.1 launcher/DLLs must be replaced by the test build to record these new fields.
