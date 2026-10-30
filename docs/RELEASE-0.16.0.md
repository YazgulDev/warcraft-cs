# Download and upgrade

- [**WarcraftCSLauncher_DLL_Included.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.16.0/WarcraftCSLauncher_DLL_Included.exe) — Player launcher with bundled project modules and private graphics components; no local C++ compilation.
- [**WarcraftCSLauncher.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.16.0/WarcraftCSLauncher.exe) — Developer/source installation with consented dependency preparation and local compilation.
- [**WarcraftCS-0.16.0-dist.zip**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.16.0/WarcraftCS-0.16.0-dist.zip) — standard launcher, sources, update manifest, requirements and checksum. Public ZIPs contain no DLL/MIX modules.
- [Sources](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.16.0/WarcraftCS-sources.zip) · [Update manifest](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.16.0/WarcraftCS-update.json) · [Player SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.16.0/WarcraftCSLauncher_DLL_Included.exe.sha256) · [Developer SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.16.0/WarcraftCSLauncher.exe.sha256).

Save progress and close Warcraft before updating. Use **Update from GitHub**, or install the new launcher into the same dedicated mod folder, then restart Warcraft. Setup adds missing keys and preserves custom settings, saved graphics choice, prices, audio, opt-outs and saves. Your own Warcraft/Counter-Strike/Half-Life files remain required for the selected modes; game files and converted assets are not bundled. Graphics selection stays **Auto / OpenGL / Direct3D**, applied on the next launch; Auto uses OpenGL on 1.26a and Direct3D on experimental 1.27a.

## Warcraft CS 0.16.0 — 30 October 2026

Includes all changes since immutable **v0.15.0**: Direct3D portrait animation repair, cosmetic head removal on fatal head contacts, denser headshot blood, small body wounds and smoother first-person ground-height transitions. Movement mode, collision, weapon damage and economy retain their existing defaults.

## Direct3D portrait animation

The previous native world scissor could exclude the private portrait target during its clear, leaving old face depth behind and rejecting later animation frames. Capture now disables scissor before color/depth clears and restores the preceding viewport even when Warcraft resumes native UI by changing only its projection. This repairs the frozen portrait animation without stopping Warcraft's own animation clock. The existing square framing, minimap, item panels and renderer selection remain available.

## Headshots and blood

A fatal hostile direct head contact can remove the struck named head while preserving the native body, death animation/decay, kill events, rewards and resurrection. Two-headed creatures retain their other head. The render adapter validates the live unit/model identity and current visible skin palettes, and declines unsupported layouts. Resurrection restores the complete native model; no extra kill or explosive-death flag is issued.

The struck skull identity now travels from the accepted posed triangle through hit selection to corpse rendering. The anchor comes from a bone actually used by visible skin, avoiding uncomputed authoring-parent matrices on naga and Crypt Fiends. Blended neck/jaw groups replace only the struck head's contribution; body and other-skull contributions remain native. Gameplay's headshot-classification weight is independent of this visual blend. This fixes the stretched-head artifacts seen on those rigs.

Headshot spray defaults increase to **48 droplets**, **1.75 scale**, **0.8 seconds** and **220 world units/s**. A fatal contact uses a **3x** burst. A lethal shotgun pellet or beam contact still produces its burst immediately after a nonfatal head contact, while duplicate Headshot captions remain coalesced.

Confirmed non-head damage to an enemy creature emits a smaller spray at the accepted impact: **6 droplets**, **0.65 scale**, **0.35 seconds**, **110 world units/s**. Body wounds create no Headshot label or fatal multiplier. Misses, zero/immune damage, friendly contacts, destructables and buildings do not create body blood. Body pellet/beam coalescing is independent of head contacts; all blood shares the bounded 256-droplet cosmetic pool. Damage and target selection remain authoritative.

### Blood settings and upgrades

All keys below belong to `[CombatVisuals]`, apply at startup/**F8**, and are editable in `WarcraftCS/WarcraftCS.ini`. F8 clears existing particles; head-removal choices affect subsequent deaths. Updates preserve custom values and explicit opt-outs.

| Key | Default | Range / meaning |
| --- | --- | --- |
| HeadshotDecapitation | true | true/false or 1/0; cosmetic struck-head removal on eligible future deaths |
| HeadshotBlood | true | true/false or 1/0; head spray independently of removal/text |
| FatalBloodMultiplier | 3 | 1–4; fatal head spray multiplier |
| BloodCount | 48 | 1–64 droplets per nonfatal head burst |
| BloodScale | 1.75 | .25–4 size factor |
| BloodSeconds | 0.8 | .1–3 seconds |
| BloodSpeed | 220 | 0–500 world units/s |
| BloodGravity | 400 | 0–2000 world units/s²; shared with body blood |
| BloodVersion | 2 | One-time stock-upgrade marker; 2 preserves an intentional return to old head-spray values |
| BodyBlood | true | true/false or 1/0; small confirmed enemy body spray |
| BodyBloodCount | 6 | 1–64 droplets per body burst |
| BodyBloodScale | 0.65 | .25–4 size factor |
| BodyBloodSeconds | 0.35 | .1–3 seconds |
| BodyBloodSpeed | 110 | 0–500 world units/s |

The former stock head values (16 droplets, .5 seconds, scale 1, speed 180) migrate individually once at startup/F8 and during setup. Nonstock custom values, disabled features and explicit `BloodVersion=2` choices survive. `HeadshotBlood=false` and `BodyBlood=false` are independent; set both false to hide all mod blood. Existing `CriticalText` separately controls the Headshot caption.

## Walking camera

Small support-height changes previously changed the first-person eye height immediately. A bounded vertical response now softens these transitions after collision, and the same corrected eye supplies the camera, direct shots and spell/item/hover aiming. Physical feet, horizontal position, collision, jump velocity and third-person boom behavior retain their existing rules. The eye remains inside the current standing/crouched collision body; large relocations, view/map/focus transitions and F8 discard old correction. Airborne vertical movement itself is not filtered; existing ground correction decays.

`[Interface] CameraHeightSmoothing=true` enables this behavior. `CameraHeightSeconds=0.08` is the response time in seconds, clamped to **.01–.3**; smaller values follow terrain more closely. Missing/malformed/non-finite values use .08. Settings apply at startup/F8 and updates preserve custom choices. False restores the former direct height response. Sampled movement diagnostics now separate `physicalEye`, requested `eye` and `eyeCorrection`.

## Source, packaging and checks

Native head rendering, blood pools and ground-height presentation have separate modules and configurable snapshots. Added tests cover live palette ownership, resurrection, stale/invisible rigs, mixed head/body skin, posed head identity, blood bounds/coalescing, real graphics pixels, camera response across frame rates and saved configuration migration. The complete regression runner includes the new head-gore and camera-height suites. Test-only native observers and mailboxes are excluded from ordinary Player modules. No new game art/audio or retail binaries are distributed; public source/distribution ZIPs remain native-free, and the separately named Player EXE bundles the project modules and pinned graphics dependencies with their notices.

Release validation covers the full native-independent regression runner, fresh configuration merges, hidden-device Direct3D portrait tests, launcher installation/update/reinstall and final dual-EXE/archive integrity checks. Source audits exclude credentials, private paths, binaries and decompiler dumps from the public tree.

Relevant feature checks and native 1.26a verification passed before release preparation. The Direct3D HAL regression reproduces the stale-depth portrait failure before the fix and passes after it; native Arthas samples continue animating. Fatal head contacts were checked on a footman, two-headed ogre, Crypt Fiends, Naga Myrmidons and Naga Sirens in private OpenGL/Direct3D scenes. Body/head contact checks confirm six/48 droplets, independent F8 opt-out and no body spray from a damaging building hit.

In the native OpenGL low-barrier A/B test, the former camera jumped **8.4177 units** in one frame; the default response reduced the largest displayed change to **1.47843 units**. A further **1,220** native flat/ridge frames matched the preceding requested eye within **.00002 units**, accounting for native getter latency. Portable tests cover 30/60/144 FPS response, body bounds, airborne isolation, relocation and opt-out. Test installations and saves were restored after verification.

## Known limits

Head removal is limited to validated **1.26a named-head rigs**. Unsupported models and **1.27a** keep native death. Head texture cutouts, unusual custom rigs and all campaign transitions are not exhaustively verified. Real 1.27a gameplay remains unavailable locally and experimental. Offline single player only; Reforged and other patches remain unsupported.

The ground-step camera fix is reproduced and measured; one-second logs from the earlier session cannot prove the cause of every reported shake, and general frame stutter is not claimed solved. The AMD RX 580 OpenGL UI failure in issue #10 remains unresolved; this release does not establish its root cause.

<!-- launcher-summary
- Improved first-person camera stability over small ground-height steps with configurable smoothing and matching aim height.
- Added a small configurable blood spray at confirmed enemy body impacts, independent of headshot blood.
- Added configurable fatal headshots that remove the struck head while preserving the body and native death events on verified 1.26a models.
- Preserved the other head on two-headed creatures and restored complete models on native resurrection.
- Improved headshot blood density, size and lifetime with a configurable fatal spray multiplier.
- Fixed a lethal pellet losing its blood burst immediately after a nonfatal head pellet.
- Preserved custom blood settings, explicit visual opt-outs and existing gameplay defaults during updates.
- Fixed Direct3D portrait animation freezing when the previous frame depth was left uncleared by the native world scissor.
- Fixed stretched headshot meshes on naga and Crypt Fiends by using active skin bones and preserving blended neck and jaw contributions.
-->
