# Nuclear bomb and FPS text input preview

This local preview includes the previous arsenal/dense-smoke and in-game settings features. Published v0.16.0 assets are unchanged.

During FPS, TPS or clean F9 unit view, **Enter** opens a text field. **Enter** submits; **Escape** cancels without leaving FPS. Cyrillic, Backspace and Ctrl+V from the Unicode clipboard are supported. Movement, aiming, firing and shortcuts are blocked during entry; gravity, reload and the map continue. Focus loss, native menus, view changes and map transitions discard unfinished text. RTS and native shops keep their normal controls. Submitted ordinary text goes through Warcraft's own chat/cheat handler and map chat triggers; the custom HUD displays the most recent sent line. This is offline single-player input, not multiplayer support. No typed text is written to diagnostic logs. Native 1.26a chat receives UTF-8 directly; keyboard WM_CHAR bytes use the Windows ANSI codepage. The experimental 1.27a adapter has no verified native chat submission entry, so ordinary submission reports unavailable there; local code entry and text rendering remain separate.

In **weapon FPS (F6)**, enter **PUTINMOLODEC** as a complete line (case insensitive). This grants/selects **one nuclear bomb**. Enter never launches it. **LMB** launches a strike **800 Warcraft units ahead**, with a **3-second countdown**. **RMB or Escape** holsters without consuming it; enter the code again to select the retained charge. Switching view, opening other gameplay panels, losing focus or changing weapons holsters it. The bomb is intentionally code-only: no buy catalog, random drops, ammo purchases, F11/F12 or runes grant it.

By default the blast strikes hostile living creatures across the map, including neutral hostiles and flying units. Allied/self units, buildings, hidden/loaded actors and quest invulnerability are preserved. Native attributed chaos/universal damage retains death/damage triggers, experience and configured CS rewards; map damage triggers can still protect scripted bosses. There is no forced RemoveUnit/KillUnit bypass. A removed/transferred launch actor cancels unsafe damage attribution. Procedural mushroom, shockwave, a short flash, mild camera shake and existing private C4 audio need no extra asset download.

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
| NuclearBomb.DelaySeconds | 3 | 0.5..30 simulation seconds |
| NuclearBomb.Radius | 0 | 0 = whole map; 0..50000 XY Warcraft units |
| NuclearBomb.TargetDistance | 800 | 100..5000 Warcraft units ahead for the impact marker |
| NuclearBomb.FlashSeconds | 0.65 | 0..3 seconds, 0 disables |
| NuclearBomb.ShakeDegrees | 0.6 | 0..3 degrees, 0 disables |
| NuclearBomb.EffectSeconds | 8 | 3..20 seconds |

Installer migration inserts missing keys while preserving existing custom values. Impact marker distance is independent of blast radius.

Modules: `input/FpsTextEntry` owns bounded UTF-8 entry and the submission mailbox; `platform/NativeChat` adapts completed UTF-8 text to the verified host edit box and chat submission handler; `combat/NuclearInventory` separates grant/selection/consumption; `platform/NativeNuclearBomb` owns native timers, immutable victim identity and attributed damage; `presentation/NuclearView` owns original world/held geometry; `presentation/FpsTextView` owns UTF-8 wrapping and modal text drawing. The controller coordinates these modules and the existing modal movement path.

Portable input/configuration and actual Cyrillic pixel regressions passed. In the private 1.26a OpenGL scene, text did not move/fire the actor, Escape stayed in FPS, native chat processed a real command, Enter never detonated, and the blast killed hostile ground/air units while preserving an ally, buildings and Avul invulnerability. Direct3D 1.26a also passed native chat, text isolation and hostile/air/distant-target damage with preserved allies, buildings and invulnerability. Actual Cyrillic font pixels, settings/selection regressions, INI boundary cases and repeated-update preservation passed. Final packaging checks are recorded in the local journal. Real 1.27a verification is unavailable on this PC; no such claim is made.
