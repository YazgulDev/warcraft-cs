# Nuclear bomb and FPS text input preview

This local preview includes the previous arsenal/dense-smoke and in-game settings features. Published v0.16.0 assets are unchanged.

During FPS, TPS or clean F9 unit view, **Enter** opens a text field. **Enter** submits; **Escape** cancels without leaving FPS. Cyrillic, Backspace and Ctrl+V from the Unicode clipboard are supported. Movement, aiming, firing and shortcuts are blocked during entry; gravity, reload and the map continue. Focus loss, native menus, view changes and map transitions discard unfinished text. RTS and native shops keep their normal controls. Submitted ordinary text goes through Warcraft's own chat/cheat handler and map chat triggers; the custom HUD displays the most recent sent line. This is offline single-player input, not multiplayer support. No typed text is written to diagnostic logs. Native 1.26a chat receives UTF-8 directly; keyboard WM_CHAR bytes use the Windows ANSI codepage. The experimental 1.27a adapter has no verified native chat submission entry, so ordinary submission reports unavailable there; local code entry and text rendering remain separate.

In **weapon FPS (F6)**, enter **PUTINMOLODEC** as a complete line (case insensitive). This grants/selects **one nuclear RPG round**. Enter never fires it. **LMB** fires from the existing RPG using its held model, firing animation, rocket model, motor, trail, inherited motion and swept collisions. The rocket detonates on impact with terrain/obstacles or its ordinary flight limit. It does not consume or grant ordinary RPG ammunition. **RMB or Escape** holsters without consuming the charge; enter the code again to select it. Switching view, opening other gameplay panels, losing focus or changing weapons holsters it. The bomb is code-only: buying, drops, refill, F11/F12 and runes cannot grant it.

The default blast radius is **8000 XY Warcraft units** around the actual impact. It kills hostile living creatures, including air units, and neutral creatures/buildings. Neutral shops are destroyed even with their stock invulnerability: attributed damage is attempted first, then native `KillUnit` if necessary. Trees, gates, barrels and other runtime destructables in range use native `KillDestructable`, preserving their death events and pathing changes. Decorative terrain/doodads without runtime destructable objects remain scenery. Allied/self nonneutral units and buildings are preserved. Hostile units and buildings in range are destroyed by default, including Avul/damage-immune enemies. Hidden/loaded units and invulnerable non-hostile creatures remain protected. Death/damage triggers, experience and eligible configured CS rewards use the existing native paths; if damage leaves an eligible enemy alive, the default `ForceEnemyDeath=true` policy finishes it through native `KillUnit`. Identity and current hostility are checked again before forced death, retaining native death events and eligible CS rewards. Victim handles are snapshotted and checked against object identity, and a removed/transferred launch actor cancels unsafe damage attribution.

Effects are local: a large native Warcraft **NeutralBuildingExplosion**, then **Flame Strike** after 0.8 seconds, then native **SmokeSmudge** after another 7 seconds, lasting 18 seconds. Smoke now uses 37 evenly distributed emitters with a central scale of 14, replacing 7 smaller sources. The explosion uses a scale of 12 and Flame Strike a scale of 7. Native models come from the player's own Warcraft MPQs. A short local flash/shake attenuates to zero by 10000 units; a small ground shock ring uses the shared OpenGL/Direct3D renderer. Effects have bounded counts and expire automatically. No full-map overlay or procedural mushroom is used. The impact plays the actual Warcraft goblin-sapper explosion from the owner's MPQ through the existing CS mixer, replacing the Half-Life debris sound. Missing/unsupported archive audio falls back to the existing private CS C4 explosion; no retail sample is bundled or written to disk.

The default is **one launch per map**. Code entry again while already carrying the charge selects the same charge. `[NuclearBomb] RepeatCodes=true` allows a new charge after the previous strike/effect has finished. A selected unlaunched charge survives holstering, but neither charges nor launched strikes cross map unload/save-load transitions.

Editable settings, startup/F8 (changes affect the next launch):

| Section/key | Default | Range / meaning |
| --- | --- | --- |
| Chat.Enabled | true | Enter text field in controlled views |
| Chat.MessageSeconds | 10 | 0..60 seconds of local last-message display |
| NuclearBomb.Enabled | true | Allow code grants and launches |
| NuclearBomb.Code | PUTINMOLODEC | 1..64 ASCII letters/digits/underscore/hyphen; invalid uses default |
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
| NuclearBomb.ExplosionSound | sapper | sapper / city / artillery / off, case insensitive |
| NuclearBomb.ExplosionVolumePercent | 100 | 0..100, multiplied by current CS master volume |
| NuclearBomb.ShakeDegrees | 0.6 | 0..3 degrees, zero disables |

Installer migration inserts missing keys while preserving custom values. The independent `DestructionVersion=2` policy group upgrades the former `DestroyBuildings=false` default once. Explicit v2 opt-outs (`DestroyBuildings=false` and/or `ForceEnemyDeath=false`) remain supported. Other balance, sound, smoke, allies and radius settings are preserved. Unversioned/v1 global stock `Radius=0` and pre-v3 stock `Radius=1600` upgrade once to 8000. Previous stock visual radius/scales, smoke duration and screen-effect radius upgrade with the same BehaviorVersion=3 group. Explicit v3 opt-backs, other custom radii, neutral/destructable opt-outs, density, sound choice and volume survive repeated upgrades. Explicit v2/v3 zero radius remains local and clamps to 100 at runtime. Retired `DelaySeconds`, `TargetDistance` and `EffectSeconds` entries can remain but are ignored. Flight uses existing `[RPG]` parameters. F8 reloads nuclear settings for the next launch; active flight/effects retain their launch snapshot. The general CS master volume still affects ongoing audio immediately.

Modules: `input/FpsTextEntry` owns bounded UTF-8 entry; `platform/NativeChat` adapts submitted text to the host; `combat/NuclearInventory` owns code-only consumption; `platform/NativeRockets` owns shared flight and dispatches the selected impact payload; `platform/NativeNuclearBomb` owns the phase clock; `platform/NativeNuclearDestruction` owns radial eligibility and native death; `platform/NativeEffectService` owns only its verified scaled native effects and expiry. `presentation/NuclearView` draws the local shock ring; the held weapon uses the existing RPG GunMesh. The controller coordinates these modules and existing modal movement.

The original local-impact 1.26a OpenGL verification covers the actual RPG model and short-click launch, real swept impact, native effect scaling/order/expiry, nearby ground/air/neutral kills, invulnerable neutral shop and barrel death, tree death, distant/allied/nonneutral-building/invulnerable-creature survival, ordinary ammo preservation and ordinary RPG regression. Portable tests cover snapshots, config boundaries, repeatable migration and map-unload callbacks. The same native gameplay, effect scaling/order/expiry and ordinary RPG checks passed in Direct3D 1.26a, with its held-model screenshot inspected. The enlarged 8000-radius/37-emitter smoke and archive-sound follow-up is checked separately in the private journal. Final packaging results are recorded in the private verification journal. The previous preview's UTF-8/native-chat checks remain applicable. Real 1.27a is unavailable on this PC; that experimental adapter does not yet verify effect scaling or native destructable death, and no real 1.27a test is claimed.

The hostile-death follow-up adds an allied tower and an invulnerable hostile tower/creature to the disposable scene. Portable tests cover forced enemy death after damage immunity, protected allies/distant targets, explicit policy opt-outs and migration. Native verification and final launcher validation are recorded in the private journal. Damage remains an instantaneous 8000-radius strike; targets created later or outside the radius are outside this policy.
