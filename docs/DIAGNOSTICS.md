# Diagnostic logging

Startup/settings reload logs every resolved keyboard shortcut and spell slot override. Invalid or
conflicting layouts revert as a whole. Spell records identify unit/slot/rawcode/native order, target
form and point, engine acceptance and mana. Unlearned/passive/unsupported notices are logged too.
Completion/interruption records distinguish explicit spell ownership from normal FPS order cancellation;
an accepted order does not by itself prove a successful effect when the engine rejects costs/targets.
Automatic lookups also log the hero type, all four resolved rawcodes and the requested slot, making
custom list-order/replacement problems distinguishable from native cost or cooldown rejection.

Health bars log the validated allied/enemy switches and distance, the shared K toggle and whether
each INI key save succeeded. Sampled records report native candidate and submitted-bar counts. Submission is
not proof of visible pixels: world depth may correctly occlude a submitted bar.

Ground-clipping test builds log native far-limit signature verification, FPS channel ceiling changes
and restoration. Sampled `world projection` records show `requestedFar`, `cameraFar`, `projectionFar`
and `near` before/after correction, distinguishing saved settings from actual engine values.

The ledge-recovery test branch adds sampled `movement blocked` records with `solid-volume`,
`uphill-terrain`, `invalid-terrain`, `native-pathing`, `native-relocation` or `world-boundary` reasons.
They include the original position, rejected sample, reachable height, sampled terrain/object height
and original/sample native cliff levels. All rejection paths share one `IntervalMs` clock and obey
`Detailed`; standing still produces no rejection record.

Test-branch rendering settings log the validated MaxFPS/DrawDistance snapshot, verified frame-rate
adapter setup, applied native/custom mode and failures to change the guarded operand. `render frames`
reports presentations within the configured Logging.IntervalMs, so use that interval when estimating
FPS from the count; loading/focus transitions and capture overhead affect individual samples.

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

HUD state records include both `program` and the independent `pipeline` binding. A bound separate
shader pipeline can discard or blacken fixed-function HUD/weapon geometry while `program=0` and
`observedErrors=0`. The raw `host` snapshot precedes program isolation; prepared passes should report
zero for both. The renderer restores the caller's program/pipeline and supported legacy-program/discard
enables after drawing. This is automatic render-state isolation, without a player setting.

`GraphicsDebug=true` adds an observer around each visible mod overlay pass. Error checkpoints run
at every boundary rather than only on the summary timer: `host-before-overlay`, `context-resources`,
`overlay-state`, `clean-view-effects`, `weapon-resource-load`, `world-effects`, `held-model`,
`flat-hud` and `restore-host`. An OpenGL error record includes its symbolic/raw code, phase and context.
Already queued host errors are labelled separately; a later boundary identifies the interval in which
an error was observed, not necessarily its root cause. Each checkpoint consumes at most 16 queued errors.
Failures trigger bounded state snapshots, including shader/pipeline, framebuffer, raster/texture state,
matrix-stack depths and pixel-unpack layout. `Detailed=false` omits these TRACE snapshots while retaining
the new ERROR/WARN records when GraphicsDebug is enabled.

When the context exposes KHR_debug/core 4.3 or ARB_debug_output, the observer also records delivered
driver messages with source, type, ID, severity and phase. Delivery is synchronous only during the mod
scope to keep the owning callback and phase valid. Existing message filters are preserved, an originally
enabled foreign callback is forwarded, and its callback/user/output/synchronous state is restored afterward.
Some drivers/non-debug contexts emit no messages, and previously filtered messages remain filtered;
this feature cannot guarantee a record for every silent GPU problem. Synchronous delivery and additional
state queries may cost frame time; `GraphicsDebug=false` disables this observer/checkpoints live on F8.
Existing sampled reticle checks remain controlled by `Detailed`.

`graphics weapon` records accepted-shot serial/animation time, loaded/held/scoped/contained/hidden
decisions immediately when a round or visibility changes, with ordinary unchanged frames sampled.
Failed held-model and buy-preview loads include the role, slot and path. These records distinguish an
intentional hidden model or missing resource from a submitted draw; they do not prove pixel visibility.
D3D9 failures include the operation, HRESULT and relevant state/texture dimensions or draw parameters,
covering scene/state-block capture/restoration, draw calls, render/texture/sampler/transform state,
depth clears and texture creation/locking/upload completion. Successful API calls produce no error record.

Identical graphics messages are collapsed within `IntervalMs` using a bounded 64-entry table. The next
occurrence after that interval reports how many intervening duplicates were suppressed in `repeats`.
A different phase/code/message/parameter can be recorded immediately; old entries can be evicted.
There is no periodic flush for a suppressed error that never occurs again; its first occurrence remains.
The existing file rotation bounds storage even during a persistent failure.

Already handled controller/Army AI faults and failures during their recovery now add `native fault`
ERROR records with the original exception code, address, module/base/offset, exception arguments and
x86 registers. A safe, bounded frame-pointer walk reports up to 24 return addresses when available.
Optimized code may omit frame pointers, so this is explicitly a best-effort chain, not a complete symbolic
stack. Recovery/disable behavior is unchanged; no process-wide exception interception is installed.
These essential fault records remain enabled independently of Detailed/GraphicsDebug. Unhandled game
crashes, silent driver faults and unavailable storage still cannot be guaranteed to reach the log.

Movement settings are recorded after validation at startup and F8. `movement contact` traces each
takeoff/landing with horizontal speed in Warcraft and GoldSrc units, XYZ velocity, movement inputs
and view yaw. These transitions complement sampled position/ground/speed summaries when diagnosing
lost hop momentum or strafe gain. `Detailed=false` disables the optional movement records.

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
| `GraphicsDebug` | `true` | `true`/`false` (also `1`/`0`). Enable per-pass GL checkpoints/driver messages and failed D3D9 call diagnostics. F8 reloads it; independent of Detailed. |
| `IntervalMs` | `1000` | 100–60000 milliseconds between movement/render summaries. Control/action edges remain event driven. |
| `MaxFileMB` | `8` | 1–64 MiB per runtime journal; size rollover starts a new segment. |
| `ArchiveCount` | `3` | 0–8 older sessions/segments. Reducing it removes surplus numbered runtime archives; zero retains only the current file. |

Missing/malformed keys use defaults, numeric values are clamped, and updates add missing keys without
replacing custom values or duplicating sections. Default runtime retention uses at most 32 MiB across
the current journal and three archives. Installer transcripts and older existing replacement logs
retain their existing storage policy. Logs can fail to persist when the disk/path is unavailable.

For a missing-crosshair report, reproduce with the affected weapon and attach the current game log,
an FPS screenshot, launcher version/mode, GPU/driver and map/edition. The source fingerprint distinguishes
different DLLs carrying the same version number. These diagnostic fields ship in 0.7.0;
update older launchers and modules to record them.

FPS attack guards log validated hook installation for native validation/dispatch and numeric/string
map orders. Bounded `FPS blocked native attack` traces identify rejection phases; existing cancellation
records still report cleanup of a lingering native target. Spell components keep their own diagnostics.

`FPS native attack target lookup installed` reports the forwarding native target hook. Sampled
`spell HUD slot` records include rawcode, learned level, passive status, timer support, live remaining
seconds and Art path. `Spell icon loaded` reports archive decode/upload success, dimensions and path
once per map/context cache entry. A missing/unsupported icon remains an empty framed slot.

The native walking guard separately reports verified/installed motor interception and bounded
`FPS blocked native walking` records with the caller offset. An idle actor may previously drift
with order=0 and mod speed=0; compare successive XY samples rather than relying on orders alone.
The stats getter and explicitly owned spell approach retain native speed; no persistent speed/root
field is modified. Squad order records include `yielding`, accepted native result and destination.

Spell HUD samples additionally report total cooldown, matched native command button, mana shortage
and disabled state. Actor-level definitions supply cost/duration when an unavailable button drops
its native object reference. A remaining timer is always read from Warcraft's simulation clock.
Live command art and the current actor's ability chain account for toggles and map replacements.

The target lookup preserves its native return value: Warcraft's morph/combat update dereferences
retained targets without a null check. Suppression belongs to order dispatch and the walking motor;
fabricating a null target can crash a transformation such as Avatar.

Unit status includes `attackSpeed`; scaling configuration records fire/move switches and the movement
reference. Weapon settings include `maxReserve`. Spell HUD records `charges=-1` for unknown charge
sources, or the actual marker count; unavailable requests log their reason, remaining native timer
and charges. An accepted IssueOrder is an order acknowledgement, not proof of a spell effect.
Shrapnel's three charges/40-second map timer must not be mistaken for an infinite-cast cooldown bug.

`Native weapon attack effects verified/installed` reports adapter availability. Settings report
`Attack effects enabled` and `AWP finishing active`; a stored AWPOneShot preference is active only in
weapon damage mode. Each shot records `finishing` and `effects`. The native contact record reports
the substituted base and original weapon kind. A native dodge/immune hit is handled without fallback
damage even when no base roll is consumed. Compare target HP/mana/buffs and attacker HP to establish
the actual passive effect; a successful dispatch alone does not prove that a chance-based effect proc'd.

Native contacts also record center-to-center `distance`. Confirmed FPS contacts extend only their
synchronous native impact range check to that distance; the production ray enforces weapon range
and terrain first. Ordinary RTS attack range is unchanged. `substituted=0` can still represent a
native cancellation or miss; it must not trigger a second direct-damage event.

Settings record `Pass-through effect objects enabled`. Each intersected unit skipped by the
nonphysical-object policy records `shot pass-through unit/type/entry`; the final `shot` still names
the actual nearest physical target. The filter does not make immune bodies damageable.
