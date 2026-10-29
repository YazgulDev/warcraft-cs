## Download and upgrade

- [**WarcraftCSLauncher_DLL_Included.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.14.0/WarcraftCSLauncher_DLL_Included.exe) — recommended Player launcher, with bundled project modules and no local C++ compilation.
- [**WarcraftCSLauncher.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.14.0/WarcraftCSLauncher.exe) — Developer/source installation with consented dependency preparation and local compilation.
- [**WarcraftCS-0.14.0-dist.zip**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.14.0/WarcraftCS-0.14.0-dist.zip) — standard launcher, sources, update manifest, requirements and checksum. Public ZIPs contain no DLL/MIX modules.
- [Sources](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.14.0/WarcraftCS-sources.zip) · [Update manifest](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.14.0/WarcraftCS-update.json) · [Player SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.14.0/WarcraftCSLauncher_DLL_Included.exe.sha256) · [Developer SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.14.0/WarcraftCSLauncher.exe.sha256).

Save campaign progress and close Warcraft before updating, then restart. Use **Update from GitHub**, or install the new launcher into the same dedicated mod folder. Setup preserves saves, custom settings, prices, audio volume and opt-outs while adding missing INI keys. F8 reloads settings after installation; replacing a loaded DLL requires a restart. Your own Warcraft/CS/Half-Life files remain required; no games or converted assets are bundled.

## Warcraft CS 0.14.0 — 28 October 2026

Includes all changes since the immutable **v0.13.0** tag: rendered-model shot alignment, head-contact/blood corrections, saved friendly-fire preferences, campaign economy startup, HUD/weapon graphics-state isolation and expanded diagnostics. Classic movement, dollar/gold payments, $200 kill rewards, weapon balance and optional army escort retain their 0.13.0 defaults.

## Head contacts and shot alignment

Precise firearm and energy rays now use the unit sprite's complete rendered basis, position and separate visual scale. This accounts for interpolated rotation, tilted/nonuniform transforms and rendered XY/Z differing from gameplay coordinates. World-unit ray distance and nearest physical surface selection remain authoritative; empty broad bounds do not steal shots aimed at neighboring units. Melee, spell and third-person volume targeting retain their existing contracts.

Mixed head/neck triangles use interpolated named-head skin influence at the actual contact, replacing a majority vote over vertices. Separate named heads remain separate surfaces; there is no invented hit region across their empty gap. Invalid render transforms, unsupported models or unavailable pose clocks retain conservative body hits without inventing a head location. Gates, terrain, hidden/cargo/effect filtering and native defenses retain their rules.

Direct Tau/Gluon contacts now share conventional firearms' named-head bonuses by default. Confirmed HP loss on an eligible anatomical head contact emits blood from the exact contact independently of the damage-bonus and Headshot-caption switches. Blood remains controlled by `CombatVisuals.HeadshotBlood`; disabling headshot bonuses removes the bonus/caption, while physical head blood can remain. Contact-based blood/text anchoring avoids looking up an already removed target after a lethal map trigger. Misses, immunity, explosives, melee and visual-only continuous-beam tracking do not create fabricated head blood.

Fired rounds add candidate/pose/sequence/local-ray/head classification and miss records. Reticle prediction and visual-only Gluon tracking do not emit candidate diagnostics. These records help distinguish geometry gaps, unsupported poses, ally protection and weapon aim/recoil.

## Friendly-fire preference

Q now saves only `Combat.TargetAllies` to the existing INI, so F8 and restarting preserve the chosen direct allied-target policy. Feedback reports **FRIENDLY FIRE ON (50%)**, off, zero effective damage or an explicit save failure. The default remains ally protection (`TargetAllies=false`), with `Damage.FriendlyFirePercent=50` applied when targeting is enabled. Updates preserve both saved values. C4/native spells retain their separate policies, and immunity/defenses remain authoritative.

## Campaign economy and U feedback

When workers are available but the base is missing, AI construction centers on available builders instead of a distant controlled hero. Worker race takes precedence over a captured foreign hall. Bounded native placement retries try alternate sites within a decision instead of waiting after a single rejection.

Undead acolytes can construct a real haunted gold mine on an available mine; idle, unassigned ghouls can gather real trees. Returning lumber ghouls count toward the assigned limit and keep harvesting priority over optional escort/defense. Night elf halls use their available native entangle ability before assigning wisps to gold. Undead/night elf harvesting recognizes compatible owned active mines.

U confirms on/off. Optional status feedback reports development, queued work, completed targets or blockers such as unavailable economy/workers/resources, busy/reserved workers and native construction/training rejection. State transitions also log current gold/lumber; F8 requests a current report. Status observations do not issue extra orders. FollowActor remains false, and manual, squad, combat and construction/training leases retain priority.

Native costs, blight, placement, food, technology and campaign restrictions still decide success. No resources/workers are created and no forbidden technology is enabled. This remains a basic economy assistant, not a complete advanced-research campaign AI; hero-only maps cannot develop without an actual economy.

## HUD, weapons and graphics diagnostics

An inherited GLSL program or separate program pipeline can blacken/discard fixed-function weapon/HUD geometry even while CURRENT_PROGRAM is zero and no GL error is observed. The renderer now scopes program/pipeline isolation, supported ARB vertex/fragment programs and rasterizer discard around mod effects, weapons and HUD, then explicitly restores caller bindings/enables. Standalone flat-HUD drawing uses the same state owner. D3D9 retains its independently managed backend.

The raw HUD state log includes the separate pipeline beside the program. New every-pass OpenGL error checkpoints distinguish queued host errors, resources, world effects, held models, flat HUD and restoration. Records include symbolic/raw code, phase and context; bounded failure snapshots add stack depths, pixel-unpack layout and existing framebuffer/raster/texture state. Observed intervals do not by themselves establish root cause, and checkpoints deliberately consume the GL error queue.

Supported KHR_debug/core 4.3 or ARB_debug_output driver messages include source, type, ID, severity and phase. Delivery is synchronous during the scoped mod pass; existing filters are preserved, an originally enabled foreign observer is forwarded, and callback/user/output/synchronous settings are restored. Non-debug contexts/drivers can emit no messages, and existing filters may suppress them. Synchronous delivery and extra queries may affect frame time; the observer can be disabled with GraphicsDebug and F8.

D3D9 failures now identify operation/HRESULT and relevant state/dimensions/draw parameters for scene/state-block capture/restoration, shaders, render/texture/sampler/transform state, viewport, depth clear, drawing and texture create/lock/unlock operations. Successful calls emit no error record. Accepted-shot/model-visibility traces record serial, animation/time and draw/skip decisions; failed held-model and buy-preview loads identify role, slot and path.

Already handled controller/Army AI/recovery exceptions add original code, address, module/base/offset, arguments and x86 registers, plus up to 24 best-effort frame-pointer return addresses. Optimized code may omit frame pointers; this is not a complete symbolized stack. Existing disable/recovery ownership remains unchanged, without a process-wide exception interceptor. Essential handled-fault records remain enabled independently of Detailed/GraphicsDebug.

## New settings and log limits

Missing/empty/malformed keys use documented defaults; numeric values are bounded. Updates insert keys once without replacing explicit choices. Boolean keys accept true/false or 1/0.

| Section / key | Default | Range and application |
| --- | --- | --- |
| Combat / HeadshotBoneWeight | .35 | .05–1 minimum interpolated named-head skin influence; startup/F8, subsequent contacts |
| Combat / EnergyHeadshots | true | Direct Tau/Gluon bonus opt-in/out; startup/F8, subsequent contacts; anatomical blood independent |
| ArmyAI / ShowStatus | true | Development/blocker messages; next active AI decision after F8; U confirmation remains |
| ArmyAI / PlacementAttempts | 12 | 1–32 total native build-site attempts per decision; next decision after F8 |
| ArmyAI / LumberWorkers | 2 | 0–10 idle unassigned undead ghouls; zero stops new assignments without canceling existing manual harvesting; next decision after F8 |
| Logging / GraphicsDebug | true | Per-pass GL observer/checkpoints and failed D3D9 calls; live on F8 |

Existing `Logging.Detailed=true` enables optional TRACE summaries/snapshots/shot-model decisions. With Detailed=false, enabled graphics ERROR/WARN records and essential handled faults remain. Duplicate graphics messages use a bounded 64-entry table: first occurrence immediately, then the next occurrence after `IntervalMs` (default 1000, range 100–60000 ms) reports suppressed repeats. Different phases/codes/messages/parameters may log immediately; old entries can be evicted. Suppressed trailing duplicates are not periodically flushed.

Existing rotation defaults to `MaxFileMB=8` (range 1–64) and `ArchiveCount=3` (range 0–8), at most 32 MiB across the current game journal and three archives by default. Use **Read logs** to export `Game/WarcraftCS/WarcraftCS.log` and relevant numbered archives after reproducing a problem. Source installations use their prepared runtime's corresponding folder. Logs remain local; unavailable storage and unhandled/silent game/OS/driver faults cannot be guaranteed to produce records.

## Source, packaging and verification

Head-contact policy, native rendered transforms, campaign status, fixed-function state ownership, graphics observation and handled-fault context remain separate modules. Test runners now reject signed native abort statuses in the affected army, posed-shot, combat-visual and HUD suites. Configuration/parser and repeated-install tests cover new defaults, ranges, malformed values, saved Q choices and diagnostic opt-out preservation. Player-package tests distinguish missing game identity, adaptive local exports and legacy exact-Miles rejection, including atomic rejection before installing malformed export data. Public source snapshots contain owned source/tests/docs only; Player assets contain the project modules and notices, with no retail content.

Checks cover authored mixed-head/two-head/animation/transform/occlusion scenes, real INI persistence, synthetic worker-only campaign economy and native order restrictions, actual hidden WGL pixels/callback delivery/restoration across contexts, recovered Windows SEH, actual D3D9 pixels and a rejected BeginScene call, native module compilation, both launchers, installation/update/reinstall fixtures, payload hashes, compact summaries and package lint. The user's active campaign is not driven or restarted.

## Compatibility and remaining verification

Supported host remains offline **Warcraft III 1.26a x86, Game.dll 1.26.0.6401 (OpenGL)**. Experimental **1.27a x86, 1.27.0.52240 (D3D9)** retains its partial adapter and synthetic checks; **no real 1.27a gameplay test** is claimed. Verified posed headshots/item names remain limited to supported 1.26a adapters; 1.27a retains approximate targeting and lacks those labels/bonuses. Native animation blending/look-at and texture cutout pixels are not fully reproduced, so tracing is not pixel-perfect.

The inherited-program/pipeline defect is reproduced and corrected in real WGL fixtures. This does not prove every symptom of issue #10 is fixed on the reporter's Radeon: updated machine-specific reproduction/logs are still needed. The reported final undead mission has synthetic worker-only economy coverage, not a verified full mission run. No universal campaign/custom-map or cross-driver compatibility is claimed. Reforged, other patches and multiplayer remain unsupported.

<!-- launcher-summary
- Fixed precise shot alignment using the full rendered unit transform and visual scale.
- Fixed headshot classification on mixed head and neck faces at the actual contact.
- Enabled direct Tau and Gluon headshot bonuses with a configurable opt-out.
- Fixed verified head blood depending on the optional damage bonus or caption.
- Fixed Q friendly-fire preferences being lost on F8 or restart and improved effective-damage feedback.
- Fixed campaign base startup near available workers and added native mine and ghoul-lumber support.
- Added configurable army progress, blocker feedback and bounded construction-site retries.
- Fixed inherited OpenGL programs and separate pipelines hiding HUD, weapons and effects.
- Added per-pass OpenGL errors, supported driver messages and failed D3D9 operation diagnostics.
- Added shot/model traces and original context with best-effort stacks for handled native faults.
- Enabled F8-reloadable GraphicsDebug by default with bounded logs and duplicate counts.
- Preserved saved settings, classic movement, weapon balance and optional army escort.
-->
