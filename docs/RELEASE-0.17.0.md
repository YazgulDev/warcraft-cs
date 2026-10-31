# Download and upgrade

- [**WarcraftCSLauncher_DLL_Included.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.17.0/WarcraftCSLauncher_DLL_Included.exe) — Player launcher with bundled project modules and private graphics components; no local C++ compilation.
- [**WarcraftCSLauncher.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.17.0/WarcraftCSLauncher.exe) — Developer/source installation with consented dependency preparation and local compilation.
- [**WarcraftCS-0.17.0-dist.zip**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.17.0/WarcraftCS-0.17.0-dist.zip) — standard launcher, sources, update manifest, requirements and checksum. Public ZIPs contain no DLL/MIX modules.
- [Sources](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.17.0/WarcraftCS-sources.zip) · [Update manifest](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.17.0/WarcraftCS-update.json) · [Player SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.17.0/WarcraftCSLauncher_DLL_Included.exe.sha256) · [Developer SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.17.0/WarcraftCSLauncher.exe.sha256).

Save progress and close Warcraft before updating. Use **Update from GitHub**, or install the new launcher into your existing dedicated mod folder, then restart Warcraft. Setup prepares new private assets automatically from your owned CS/Half-Life installation and preserves saves, custom settings, prices, renderer choice and opt-outs. Original game files and converted assets are not distributed. Source/public ZIPs contain no prebuilt native modules; the Player EXE is a separate asset. Auto continues to select OpenGL for 1.26a and Direct3D for experimental 1.27a.

## Warcraft CS 0.17.0 — 31 October 2026

Includes all changes since immutable **v0.16.0**: five CS firearms, dense smoke, the F2 settings panel and mouse sensitivity, controlled-view chat, a code-only nuclear RPG with local Warcraft effects and layered audio, and Gluon sound cancellation. Movement remains classic by default; existing ordinary RPG damage, economy, headshot and camera defaults are retained.

## Expanded CS arsenal and smoke

**B → 3 SMGs** now includes **3 TMP** and **4 UMP45**. **B → 4 Rifles** adds **4 FAMAS**, **5 Galil** and **6 G3SG1**. Existing weapon/menu IDs remain intact. Dollar payment with gold fallback remains the default. Weapons have private owned-CS models, hand-free buy previews, draw/fire/reload animation and audio, physical shot cadence, muzzle attachments and shell casings. F11 refills owned guns and F12 grants the complete available arsenal. The carried PRIMARY list scrolls its visible rows around selection to fit the expanded arsenal.

| Weapon | Magazine / max reserve | Shot interval, s | Reload, s | Weapon damage / native attack multiplier | Price |
| --- | --- | --- | --- | --- | --- |
| TMP | 30 / 120 | 0.07 | 2.125 | 20 / 0.65 | 1250 |
| UMP45 | 25 / 100 | 0.1 | 3.5 | 30 / 0.9 | 1700 |
| FAMAS | 25 / 90 | 0.0825 | 3.3 | 30 / 1 | 2250 |
| Galil | 35 / 90 | 0.0875 | 2.45 | 30 / 1 | 2000 |
| G3SG1 | 20 / 90 | 0.25 | 4.7 | 80 / 2.2 | 5000 |

Hold LMB for automatic fire; G3SG1 is the fast self-loading sniper, with RMB cycling normal view and two scope levels. FAMAS uses continuous automatic fire in this release. Y may assign RMB to squad orders instead. AWP's optional one-shot finishing rule applies only to AWP. New CS weapons do not depend on optional Half-Life resources.

New `[TMP]`, `[UMP45]`, `[FAMAS]`, `[Galil]`, `[G3SG1]` sections use the same validated controls as existing firearms: Magazine 1–1000; FireInterval .025–10 s; ReloadSeconds .1–30 s; Range 100–50000 world units (Combat.MinimumFirearmRange can raise effective range); MoveSpeed 50–500 CS units/s; ModelScale .25–2; Kick 0–10 degrees; Price/AmmoPrice 0–1000000 in selected currency; AmmoPack 1–1000; MaxReserve 0–100000; Damage 0–1000000; HeroMultiplier 0–1000. F8 applies future fire/reload tuning and clamps capacities without granting ammunition. Active reload deadlines retain the existing snapshot. Install/update converts the new resources from the player's own complete cstrike directory; no retail assets are bundled.

Smoke uses depth-tested volume slices instead of sparse translucent puffs. Long paths through smoke become almost opaque; very close surfaces remain readable. Overlapping clouds compound. The overlay hides health bars, item names and targeting rings behind dense smoke. Bullets and player blind shots retain their physical traces.

| `[SmokeGrenade]` key | Default | Meaning |
| --- | --- | --- |
| Opacity | 0.99 | 0–1 density multiplier, zero disables both visual extinction and AI obstruction. |
| VisibilityDistance | 96 | 16–600 world units of smoke leaving approximately 1% contrast at full density. AI's visibility threshold is 10% contrast, roughly 48 units at stock density. |
| BlocksAI | true | true/false or 1/0. On verified 1.26a, all teams' native basic-attack acquisition, retained windups/contacts and explicit map basic-attack orders obey the smoke line of sight. |

Radius remains 300 (32–1000), DurationSeconds 18 (1–60). Launch snapshots include density, short visibility and AI policy: startup/F8 changes subsequent grenades; active clouds finish with their original values. Smoke neither pauses creatures nor changes their saved sight/acquisition statistics, so exiting/expiry/reset leaves no stat leases. Units can still move. Warcraft minimap fog, area spells, already launched missiles and map-authored damage remain native; this is not a global fog-of-war rewrite. 1.27a retains dense visual smoke but its unverified AI attack internals do not receive the targeting guard.

Updates insert missing weapon and smoke keys while preserving custom settings, including an existing Opacity=0.85.

## In-game settings and sensitivity

Press **F2** during first-person control, third-person control or clean F9 unit view. The panel deliberately covers the requested main settings: mouse sensitivity, CS audio, movement physics, view distance and sky. Native Warcraft shops and RTS keep their own controls. `[Shortcuts] SettingsMenu=F2` can be remapped; old custom layouts using F2 receive a free F3/F4/F5 binding, or NONE if all are occupied. F6 retains the existing view-toggle behavior and F10 retains the Warcraft menu.

Use the mouse to click +/- or click/drag a numeric slider. Up/down selects a row; left/right or the wheel adjusts it. **Apply & Save / Enter** writes changed keys and applies the validated menu domains immediately. **Cancel / Escape / F2** discards the draft and returns to the current controlled view. **Reset These** drafts the listed defaults while retaining the chosen CS preset; it does not reset the rest of the INI. Reset does not take effect until Apply & Save.

| Menu row | INI key | Default and range |
| --- | --- | --- |
| Mouse sensitivity | `[Mouse] Sensitivity` | 1x; 0.1–5. Main multiplier in every controlled view, including scopes. |
| Scoped sensitivity | `[Mouse] ScopeSensitivity` | 1x; 0.1–5. Additional multiplier while scoped. |
| CS volume | `[Audio] CSVolumePercent` | 100%; 0–100. Active CS voices update immediately; Warcraft audio stays unchanged. |
| Movement physics | `[Movement] Mode` | classic; classic / recovery / hull. Existing velocity/position are not reset by applying the mode. |
| View distance | `[Rendering] DrawDistance` | 5000 Warcraft units; 3000–20000. Native map visibility and atmospheric fog can still hide distant objects. |
| Sky visible | `[Sky] Visible` | true. Controls both map-authored sky visibility and CS sky drawing in controlled views. |
| Sky type | `[Sky] Enabled`, `WarcraftEnabled` | Warcraft by default. A type change selects Warcraft or CS, with the corresponding two existing keys. |
| CS sky preset | `[Sky] <terrain letter>` or `Default` | Current saved terrain preset. Only complete installed private WCS1 sky caches appear as choices. Unknown terrains use Default. |

Main sensitivity preserves the former 0.14 degrees/count at 1x, and preserves the former scoped 0.035/0.009 degrees/count when both multipliers are 1x. Raw mouse counts and cursor fallback share the same validated gain. These preferences load on startup; **F8** still reloads manual INI edits outside the panel. Blank/malformed/non-finite numeric values use documented defaults; finite values are clamped. Installer updates add missing keys without resetting custom sensitivity, sky/audio preferences or existing shortcuts.

Sky presets apply to the current terrain's existing key, rather than changing a fallback that the map's override would ignore. Changing a CS preset takes visible effect with CS sky type selected. Missing/custom unavailable names remain identifiable until a valid installed preset is selected. Native-only installs retain native sky controls without needing CS resources. Native Warcraft sky fallback behavior still uses WarcraftEnabled; disabling Visible additionally hides an authored map sky. Leaving FPS/TPS/F9 restores the latest authored map sky, including map-script replacements. The native sky lease is verified on 1.26a; 1.27a engine internals remain experimental and unverified.

The world continues running. The panel owns look, movement and combat inputs while open; gravity and map simulation continue. Queued clicks/held fire cannot leak into a shot on close, and the mouse is reacquired without replaying old cursor movement. Reload clocks continue. Applying a menu change preserves unrelated session M/N/Y preferences, camera zoom, weapon inventory and ammunition. Map/view/focus/cinematic changes cancel an unsaved draft.

Changed keys are written to a temporary sibling file, flushed and atomically replaced. Save failure leaves the original file and live settings intact and keeps the error visible in the panel. Unrelated keys/comments stay in the same INI. The menu never writes weapon damage, prices or other unrequested configuration.

## Text input and nuclear RPG

During FPS, TPS or clean F9 unit view, **Enter** opens a text field. **Enter** submits; **Escape** cancels without leaving FPS. Cyrillic, Backspace and Ctrl+V from the Unicode clipboard are supported. Movement, aiming, firing and shortcuts are blocked during entry; gravity, reload and the map continue. Focus loss, native menus, view changes and map transitions discard unfinished text. RTS and native shops keep their normal controls. Submitted ordinary text goes through Warcraft's own chat/cheat handler and map chat triggers; the custom HUD displays the most recent sent line. This is offline single-player input, not multiplayer support. No typed text is written to diagnostic logs. Native 1.26a chat receives UTF-8 directly; keyboard WM_CHAR bytes use the Windows ANSI codepage. The experimental 1.27a adapter has no verified native chat submission entry, so ordinary submission reports unavailable there; local code entry and text rendering remain separate.

In **weapon FPS (F6)**, enter **warneverchanges** as a complete line (case insensitive). This grants/selects **one nuclear RPG round**. Enter never fires it. **LMB** fires from the existing RPG using its held model, firing animation, rocket model, motor, trail, inherited motion and swept collisions. The rocket detonates on impact with terrain/obstacles or its ordinary flight limit. It does not consume or grant ordinary RPG ammunition. **RMB or Escape** holsters without consuming the charge; enter the code again to select it. Switching view, opening other gameplay panels, losing focus or changing weapons holsters it. The bomb is code-only: buying, drops, refill, F11/F12 and runes cannot grant it.

The default blast radius is **8000 XY Warcraft units** around the actual impact. It kills hostile living creatures, including air units, and neutral creatures/buildings. Neutral shops are destroyed even with their stock invulnerability: attributed damage is attempted first, then native `KillUnit` if necessary. Trees, gates, barrels and other runtime destructables in range use native `KillDestructable`, preserving their death events and pathing changes. Decorative terrain/doodads without runtime destructable objects remain scenery. Allied/self nonneutral units and buildings are preserved. Hostile units and buildings in range are destroyed by default, including Avul/damage-immune enemies. Hidden/loaded units and invulnerable non-hostile creatures remain protected. Death/damage triggers, experience and eligible configured CS rewards use the existing native paths; if damage leaves an eligible enemy alive, the default `ForceEnemyDeath=true` policy finishes it through native `KillUnit`. Identity and current hostility are checked again before forced death, retaining native death events and eligible CS rewards. Victim handles are snapshotted and checked against object identity, and a removed/transferred launch actor cancels unsafe damage attribution.

Effects are local: a large native Warcraft **NeutralBuildingExplosion**, then **Flame Strike** after 0.8 seconds, then native **SmokeSmudge** after another 7 seconds, lasting 18 seconds. Smoke uses 37 evenly distributed emitters with a central scale of 14. The explosion uses a scale of 12 and Flame Strike a scale of 7. Native models come from the player's own Warcraft MPQs. A short local flash/shake attenuates to zero by 10000 units; a small ground shock ring uses the shared OpenGL/Direct3D renderer. Effects have bounded counts and expire automatically. No full-map overlay or procedural mushroom is used. The impact plays a private layered CS/Half-Life composite through the existing CS mixer, with an owned Warcraft goblin-sapper fallback. See the sound settings below; no retail sample is bundled.

The default is **one launch per map**. Code entry again while already carrying the charge selects the same charge. `[NuclearBomb] RepeatCodes=true` allows a new charge after the previous strike/effect has finished. A selected unlaunched charge survives holstering, but neither charges nor launched strikes cross map unload/save-load transitions.

Editable settings, startup/F8 (changes affect the next launch):

| Section/key | Default | Range / meaning |
| --- | --- | --- |
| Chat.Enabled | true | Enter text field in controlled views |
| Chat.MessageSeconds | 10 | 0..60 seconds of local last-message display |
| NuclearBomb.Enabled | true | Allow code grants and launches |
| NuclearBomb.Code | warneverchanges | 1..64 ASCII letters/digits/underscore/hyphen; invalid uses default |
| NuclearBomb.RepeatCodes | false | Allow another charge after use |
| NuclearBomb.DestroyBuildings | true | Include hostile buildings |
| NuclearBomb.ForceEnemyDeath | true | Finish surviving hostile targets with native death, including invulnerability |
| NuclearBomb.DestructionVersion | 2 | One-time upgrade of the former false hostile-building default |
| NuclearBomb.KillRewards | true | Use existing CS kill rewards/drop policy |
| NuclearBomb.BehaviorVersion | 3 | One-time migration from former global stock radius |
| NuclearBomb.Radius | 8000 | 100..20000 XY Warcraft units; never whole-map |
| NuclearBomb.VisualRadius | 3200 | 64..8000, ground-effect spread capped by blast radius |
| NuclearBomb.DestroyNeutralBuildings | true | Include neutral buildings, overriding their stock Avul |
| NuclearBomb.DestroyNeutralUnits | true | Include neutral creatures, respecting creature invulnerability |
| NuclearBomb.DestroyDestructables | true | Kill trees/gates/barrels through native death |
| NuclearBomb.ExplosionScale | 12 | 0.25..20 native model scale |
| NuclearBomb.FlameScale | 7 | 0.25..15 central native Flame Strike scale |
| NuclearBomb.SmokeScale | 14 | 0.25..30 central native smoke scale |
| NuclearBomb.FlameDelaySeconds | 0.8 | 0..5 seconds after impact |
| NuclearBomb.FlameSeconds | 7 | 0.5..15 seconds before smoke |
| NuclearBomb.SmokeSeconds | 18 | 1..45 seconds of smoke |
| NuclearBomb.ScreenEffectRadius | 10000 | 0..20000, zero disables local flash/shake |
| NuclearBomb.FlashSeconds | 0.65 | 0..3 seconds, zero disables |
| NuclearBomb.SmokeEmitters | 37 | 1..49 native emitters; bounded to fit the shared effect pool |
| NuclearBomb.ExplosionSound | layered | layered / sapper / city / artillery / off, case insensitive |
| NuclearBomb.SoundVersion | 2 | One-time upgrade of the former stock sapper cue |
| NuclearBomb.ExplosionVolumePercent | 100 | 0..100, multiplied by current CS master volume |
| NuclearBomb.ShakeDegrees | 0.6 | 0..3 degrees, zero disables |

Installer migration inserts missing keys while preserving custom values. The independent `DestructionVersion=2` policy group upgrades the former `DestroyBuildings=false` default once. Explicit v2 opt-outs (`DestroyBuildings=false` and/or `ForceEnemyDeath=false`) remain supported. Other balance, sound, smoke, allies and radius settings are preserved. Unversioned/v1 global stock `Radius=0` and pre-v3 stock `Radius=1600` upgrade once to 8000. Previous stock visual radius/scales, smoke duration and screen-effect radius upgrade with the same BehaviorVersion=3 group. Explicit v3 opt-backs, other custom radii, neutral/destructable opt-outs, density, sound choice and volume survive repeated upgrades. Explicit v2/v3 zero radius remains local and clamps to 100 at runtime. Retired `DelaySeconds`, `TargetDistance` and `EffectSeconds` entries can remain but are ignored. Flight uses existing `[RPG]` parameters. F8 reloads nuclear settings for the next launch; active flight/effects retain their launch snapshot. The general CS master volume still affects ongoing audio immediately.

## Nuclear audio preparation

The default `layered` sound combines the owner's CS C4 impact, a lower-pitched Half-Life mortar explosion, fading rocket rumble and quiet debris. Default duration is 8.45 seconds. Install automatically prepares a private mono PCM cache from `cstrike/sound/weapons/c4_explode1.wav` and `valve/sound/weapons/mortarhit.wav`, `ambience/rocketrumble1.wav` and `weapons/debris3.wav` (loose files or owned PAKs). No retail audio is distributed. If any required sample is missing or invalid, installation continues and the preset falls back to the Warcraft sapper cue; unsupported Warcraft audio retains the private CS C4 fallback. Samples mix below a bounded peak, with a smooth rumble fade and no amplification of deliberately quiet settings.

`SoundVersion=2` upgrades only the previous unversioned/v1 stock `sapper` choice to `layered`. Custom city/artillery/off choices and explicitly versioned sapper choices survive. Cue selection and overall `ExplosionVolumePercent` reload with F8 for the next launch. The following `[NuclearAudio]` controls are conversion settings: **run Install again after changing them**. F8 does not regenerate the sound cache. Saved mix controls survive subsequent installs.

| Key | Default | Range / meaning |
| --- | --- | --- |
| C4Volume | 0.85 | 0..1, impact layer gain |
| C4Pitch | 0.85 | 0.5..1.5, playback-rate multiplier |
| MortarVolume | 0.7 | 0..1 |
| MortarPitch | 0.72 | 0.5..1.5 |
| MortarDelaySeconds | 0.08 | 0..2 seconds after impact |
| RumbleVolume | 0.45 | 0..1 |
| RumblePitch | 0.7 | 0.5..1.5 |
| RumbleDelaySeconds | 0.45 | 0..4 seconds after impact |
| RumbleSeconds | 8 | 0..15 seconds; zero disables the tail |
| RumbleFadeSeconds | 3 | 0.05..10 seconds, bounded by rumble duration |
| DebrisVolume | 0.18 | 0..1 |
| DebrisPitch | 0.85 | 0.5..1.5 |
| DebrisDelaySeconds | 1.4 | 0..5 seconds after impact |

The grant code is **warneverchanges**, case insensitive. Install and F8 replace the former stock phrase while retaining other custom codes. It grants/selects a charge; only LMB launches. Older local previews upgrade their stock radius, effect sizes and smoke under BehaviorVersion=3, hostile-building policy under DestructionVersion=2 and sound cue under SoundVersion=2. Explicit versioned opt-backs, muted/custom sound, custom code and other preferences are preserved. Reinstall after editing NuclearAudio mixing controls; F8 changes cue/overall gain and future gameplay settings but does not rerender an existing WAV.

## Gluon audio repair

The startup cue could continue for almost four seconds after trigger release, overlapping the shutdown cue. Release, holster and canceled attacks now stop the windup as well as the continuous motor. Restart cuts the previous shutdown sample. Stopped private voices remain stopped until accepted playback explicitly starts them; a released motor voice stays reserved so ordinary playback cannot revive a retained infinite buffer. The authored shutdown cue decays normally. Other private sounds and Warcraft audio retain their own lifetimes.

## Source, setup and verification

New modules separate text entry/native chat, nuclear inventory/flight/destruction/effect phases, settings drafts/storage/presentation and private PCM mixing. Setup merges saved gameplay settings before conversion, validates owned PCM, produces the composite atomically and removes its own stale optional audio when source files are unavailable. Synthetic tests cover settings migration, UTF-8 input, smoke extinction and attack guards, buying, nuclear policy/callbacks, real XAudio2 consumption and private audio conversion. No retail content, private logs, saves, native test scenes or recordings enter the public source archive.

The complete native-independent regression suites, repeated gameplay-config migration and real XAudio2 audio checks pass. Separate owned 1.26a OpenGL and Direct3D scenes verify the expanded weapons, smoke attack policy, settings persistence/sky restoration, nuclear flight/destruction/effects and Gluon cancellation; these feature checks were recorded during development with lab modules, settings, caches and saves restored by hash. Final release packaging checks validate both EXEs, identical source snapshots, version/source/runtime bindings, independent hashes and native-free public ZIPs. No real 1.27a test is claimed.

## Known limits

Offline single player only. The native chat adapter, nuclear scaling/destructable death, smoke AI guard and native sky behavior are verified on **Warcraft III 1.26a** (1.26.0.6401); **1.27a** (1.27.0.52240) remains experimental and unavailable for local native testing. On 1.27a ordinary native chat submission reports unavailable; visual smoke does not imply verified AI targeting obstruction. Reforged and other patches remain unsupported.

Smoke changes basic attack line of sight, not native minimap fog, area spells, already launched missiles or scripted map damage. The nuclear blast affects eligible targets present inside its local radius at impact; decorative terrain-only doodads, hidden/loaded units and future spawns are not destroyed. Death triggers may affect campaign progression. Full campaign/map compatibility remains under testing. The AMD RX 580 OpenGL UI failure in issue #10 remains unresolved; this release does not claim its root cause is fixed. Direct3D remains available as an alternative renderer.

<!-- launcher-summary
- Added TMP and UMP45 SMGs, FAMAS and Galil rifles, and a scoped G3SG1 automatic sniper.
- Added owned-CS models, animations, reload audio, muzzle flashes, casings and buy previews for the new guns.
- Improved smoke density and added configurable short visibility with all-team basic-attack obstruction on verified Warcraft 1.26a.
- Fixed health bars, item names and target markers revealing targets through dense smoke.
- Fixed scrolling in the expanded carried PRIMARY weapon list.
- Added an F2 settings panel for mouse sensitivity, CS volume, movement physics, view distance and sky.
- Added main and scoped sensitivity multipliers, atomic Apply and Save, cancellation and fire suppression while editing.
- Added controlled-view sky visibility, authored-map sky restoration and installed CS sky preset selection.
- Added controlled-view text input with Cyrillic, paste, cancellation and native Warcraft chat/commands on verified 1.26a.
- Added a code-only nuclear RPG round granted by warneverchanges, using shared rocket flight, trail and collisions.
- Added an 8000-unit local nuclear blast with scaled Warcraft explosion, Flame Strike, 37 smoke emitters and local flash/shake.
- Added hostile unit/building destruction through immunity, neutral shop and destructable death, with allied protection and configurable policies.
- Added automatic private layered CS/Half-Life nuclear audio with configurable mixing and owned Warcraft fallback.
- Preserved custom settings and migrated prior nuclear stock radius, building policy, cue and grant code during upgrades.
- Fixed Gluon startup audio continuing after firing and previous shutdown audio overlapping a restarted channel.
- Fixed stopped voices restarting immediately and kept released loop voices separate from ordinary playback.
- Updated configuration guides, module documentation and regression coverage for the included features.
-->
