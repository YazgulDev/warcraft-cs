# Nuclear bomb and FPS text input preview

This local preview includes the previous arsenal/dense-smoke and in-game settings features. Published v0.16.0 assets are unchanged.

During FPS, TPS or clean F9 unit view, **Enter** opens a text field. **Enter** submits; **Escape** cancels without leaving FPS. Cyrillic, Backspace and Ctrl+V from the Unicode clipboard are supported. Movement, aiming, firing and shortcuts are blocked during entry; gravity, reload and the map continue. Focus loss, native menus, view changes and map transitions discard unfinished text. RTS and native shops keep their normal controls. Submitted ordinary text goes through Warcraft's own chat/cheat handler and map chat triggers; the custom HUD displays the most recent sent line. This is offline single-player input, not multiplayer support. No typed text is written to diagnostic logs. Native 1.26a chat receives UTF-8 directly; keyboard WM_CHAR bytes use the Windows ANSI codepage. The experimental 1.27a adapter has no verified native chat submission entry, so ordinary submission reports unavailable there; local code entry and text rendering remain separate.

In **weapon FPS (F6)**, enter **PUTINMOLODEC** as a complete line (case insensitive). This grants/selects **one nuclear RPG round**. Enter never fires it. **LMB** fires from the existing RPG using its held model, firing animation, rocket model, motor, trail, inherited motion and swept collisions. The rocket detonates on impact with terrain/obstacles or its ordinary flight limit. It does not consume or grant ordinary RPG ammunition. **RMB or Escape** holsters without consuming the charge; enter the code again to select it. Switching view, opening other gameplay panels, losing focus or changing weapons holsters it. The bomb is code-only: buying, drops, refill, F11/F12 and runes cannot grant it.

The default blast radius is **1600 XY Warcraft units** around the actual impact. It kills hostile living creatures, including air units, and neutral creatures/buildings. Neutral shops are destroyed even with their stock invulnerability: attributed damage is attempted first, then native `KillUnit` if necessary. Trees, gates, barrels and other runtime destructables in range use native `KillDestructable`, preserving their death events and pathing changes. Decorative terrain/doodads without runtime destructable objects remain scenery. Allied/self nonneutral units and hostile nonneutral buildings are preserved by default. Hidden/loaded units and invulnerable ordinary creatures remain protected. Death/damage triggers, experience and eligible configured CS rewards use the existing native paths; scripted damage protection can still protect bosses. Victim handles are snapshotted and checked against object identity, and a removed/transferred launch actor cancels unsafe damage attribution.

Effects are local: a large native Warcraft **NeutralBuildingExplosion**, then **Flame Strike** after 0.8 seconds, then native **SmokeSmudge** after another 7 seconds, lasting 8 seconds. Native models come from the player's own Warcraft MPQs. A short local flash/shake attenuates to zero by 2400 units; a small ground shock ring uses the shared OpenGL/Direct3D renderer. Effects have bounded counts and expire automatically. No full-map overlay or procedural mushroom is used.

The default is **one launch per map**. Code entry again while already carrying the charge selects the same charge. `[NuclearBomb] RepeatCodes=true` allows a new charge after the previous strike/effect has finished. A selected unlaunched charge survives holstering, but neither charges nor launched strikes cross map unload/save-load transitions.

Editable settings, startup/F8 (changes affect the next launch):

| Section/key | Default | Range / meaning |
| --- | --- | --- |
| Chat.Enabled | true | Enter text field in controlled views |
| Chat.MessageSeconds | 10 | 0..60 seconds of local last-message display |
| NuclearBomb.Enabled | true | Allow code grants and launches |
| NuclearBomb.Code | PUTINMOLODEC | 1..64 ASCII letters/digits/underscore/hyphen; invalid uses default |
| NuclearBomb.RepeatCodes | false | Allow another charge after use |
| NuclearBomb.DestroyBuildings | false | Include hostile buildings |
| NuclearBomb.KillRewards | true | Use existing CS kill rewards/drop policy |
| NuclearBomb.BehaviorVersion | 2 | One-time migration from former global stock radius |
| NuclearBomb.Radius | 1600 | 100..5000 XY Warcraft units; never whole-map |
| NuclearBomb.VisualRadius | 1200 | 64..4000, ground-effect spread capped by blast radius |
| NuclearBomb.DestroyNeutralBuildings | true | Include neutral buildings, overriding their stock Avul |
| NuclearBomb.DestroyNeutralUnits | true | Include neutral creatures, respecting creature invulnerability |
| NuclearBomb.DestroyDestructables | true | Kill trees/gates/barrels through native death |
| NuclearBomb.ExplosionScale | 8 | 0.25..20 native model scale |
| NuclearBomb.FlameScale | 5 | 0.25..15 central native Flame Strike scale |
| NuclearBomb.SmokeScale | 6 | 0.25..15 central native smoke scale |
| NuclearBomb.FlameDelaySeconds | 0.8 | 0..5 seconds after impact |
| NuclearBomb.FlameSeconds | 7 | 0.5..15 seconds before smoke |
| NuclearBomb.SmokeSeconds | 8 | 1..30 seconds of smoke |
| NuclearBomb.ScreenEffectRadius | 2400 | 0..8000, zero disables local flash/shake |
| NuclearBomb.FlashSeconds | 0.65 | 0..3 seconds, zero disables |
| NuclearBomb.ShakeDegrees | 0.6 | 0..3 degrees, zero disables |

Installer migration inserts missing keys while preserving custom values. Only former unversioned/v1 stock `Radius=0` migrates once to 1600. Explicit v2 zero clamps to 100 at runtime. Positive customized radii and neutral/destructable opt-outs survive repeated upgrades. Retired `DelaySeconds`, `TargetDistance` and `EffectSeconds` entries can remain in old files but are ignored. Flight uses the existing `[RPG]` parameters. F8 reloads settings for the next launch; a flying round and its subsequent effects retain the launch snapshot.

Modules: `input/FpsTextEntry` owns bounded UTF-8 entry; `platform/NativeChat` adapts submitted text to the host; `combat/NuclearInventory` owns code-only consumption; `platform/NativeRockets` owns shared flight and dispatches the selected impact payload; `platform/NativeNuclearBomb` owns the phase clock; `platform/NativeNuclearDestruction` owns radial eligibility and native death; `platform/NativeEffectService` owns only its verified scaled native effects and expiry. `presentation/NuclearView` draws the local shock ring; the held weapon uses the existing RPG GunMesh. The controller coordinates these modules and existing modal movement.

Native 1.26a OpenGL verification covers the actual RPG model and short-click launch, real swept impact, native effect scaling/order/expiry, nearby ground/air/neutral kills, invulnerable neutral shop and barrel death, tree death, distant/allied/nonneutral-building/invulnerable-creature survival, ordinary ammo preservation and ordinary RPG regression. Portable tests cover snapshots, config boundaries, repeatable migration and map-unload callbacks. The same native gameplay, effect scaling/order/expiry and ordinary RPG checks passed in Direct3D 1.26a, with its held-model screenshot inspected. Final packaging results are recorded in the private verification journal. The previous preview's UTF-8/native-chat checks remain applicable. Real 1.27a is unavailable on this PC; that experimental adapter does not yet verify effect scaling or native destructable death, and no real 1.27a test is claimed.
