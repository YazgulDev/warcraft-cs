# Controls

[Project home](../README.md) · [Documentation](README.md) · [Settings](CONFIGURATION.md)

Start a single-player map or campaign and select a living unit you own. The tables
show default bindings; `[Shortcuts]` in the INI can change keyboard actions.

## View modes and movement

| Key | Action |
| --- | --- |
| F6 | Switch between armed first person and RTS |
| F7 | Enter third-person unit control; press again to return to RTS |
| F9 | Enter unarmed first person; press again to return to RTS |
| T | Switch between first- and third-person control |
| F10 | Open the Warcraft pause menu and return control to RTS |
| WASD / mouse | Move / look and aim |
| Shift / Ctrl / Space | Walk / crouch / jump |
| Home (Mode=recovery) | Emergency return to nearby visited safe ground after a fall or invalid placement |
| Mouse wheel in third person | Change camera distance |

With **Only Warcraft modes** installed, T switches between F7 and F9 instead.
F9 keeps movement and ambience but hides the HUD and blocks player attacks, spells
and inventory actions. Enemy basic attacks are blocked by default in this view;
`[UnitView] PreventEnemyAttacks=false` allows them.

Hold Space to repeat jumps. For air strafing, build speed with W, jump, release W,
then alternate A with a left mouse turn and D with a right turn.
See [movement](MOVEMENT.md) and [third-person control](THIRD-PERSON.md).

Select `[Movement] Mode=recovery` and press F8 to enable the latest recovery movement.
The local preview defaults to `Mode=classic`, restoring movement before the 24 October collision redesign and leaving Home inactive. Optional `Mode=recovery` enables platform protection and Home recovery. Published 0.12.0 used recovery by default.
Recovery uses `[Shortcuts] RecoverPosition=HOME` (`NONE` disables it).
It rechecks the destination and cannot restore a destroyed platform, rewind across a native
script relocation or escape cargo, roots/stuns, casting or cursor menus. In valid open ground,
Home does not provide horizontal teleportation. A fresh controlled session starts a new history.

## Weapons and buying

| Key | Action |
| --- | --- |
| Left mouse | Fire or primary melee attack |
| Right mouse | Weapon action: stronger knife/sword attack, AWP zoom, Glock burst or RPG guidance |
| R | Reload |
| B | Open or close the buy menu; choose purchases with numbers or the mouse |
| . | Buy an ammo pack using the selected currency and its fallback policy |
| 1 / 2 / 3 / 4 / 5 | Browse carried primary weapons / pistols / melee / grenades / C4 |
| Mouse wheel in armed first person | Preview the previous or next carried weapon |
| Left mouse in weapon selection | Equip the highlighted weapon without firing |
| Right mouse / 0 / Esc in weapon selection | Cancel selection |
| 0 / Esc in the buy menu | Go back or close |
| F11 / F12 | Free refill of carried ammunition / grant every available weapon and ammunition |

**Tab toggles item-number mode.** While it is on, 1–6 use inventory slots; switch it
off to select weapon groups. The default starting kit is USP, knife and sword.
Published 0.12.0 defaults to dollars. In the [local feature preview](DUAL-CURRENCY-HEADSHOTS-PREVIEW.md), click DOLLARS or GOLD at the top of B to select payment. Dollars are the default; insufficient dollars automatically fall back to sufficient gold. Prices and buying zones remain configurable.

With a grenade selected, hold left mouse to pull its pin, then release to throw.
With C4 selected, hold left mouse for three seconds while grounded to plant it.
Each charge keeps its own fuse; switching weapons or leaving FPS does not disarm it.
The optional RPG uses left mouse to launch, right mouse for guidance and R to reload;
turn guidance off to reload while a rocket is in flight.

See [weapon selection](WEAPON-SELECTION.md), [arsenal and grenades](ARSENAL-SHOPS-AI.md)
and [RPG controls](HALF-LIFE-RPG.md) for details and legacy direct-slot bindings.

## Spells, inventory and units

| Key | Action |
| --- | --- |
| Z / X / C / V | Cast learned hero spell slots 1–4 at the reticle target |
| Alt + spell or item key | Target yourself when allowed |
| E | Open a nearby shop under the reticle; otherwise pick up an item or rune |
| Tab | Toggle inventory number keys |
| I | Open the mouse inventory; click to use, drag to move/drop, right-click to cancel |
| H / O / J | Recruit owned units to follow and fight / follow without this module's attacks / release |
| Right mouse in F7/F9 | Order available owned mobile units to attack the reticle target |
| Y in armed first person | Switch right mouse between weapon actions and unit target orders |
| Q | Enable or disable direct allied targeting |
| U | Toggle the army assistant for available owned workers, buildings and troops |

Native mana, cooldowns, immunity and inventory rules apply. F9 remains an unarmed
view: its unit orders do not give the controlled actor an attack. In the mouse
inventory, I or Esc closes the panel; aiming and movement resume after it closes.
See [spell and squad settings](CONFIGURATION.md#spells-and-squads).

## Display and settings

| Key | Action |
| --- | --- |
| F8 | Reload the INI |
| P / M | Toggle speed display / minimap |
| N | Toggle offline FPS fog of war |
| K | Toggle allied and enemy health bars together and save the choice |

F6 restores normal Warcraft control. Enable your chosen view again after changing maps.

## Aiming labels in the local preview

Direct hits on verified named head geometry deal x2 damage by default and show blood and floating **Headshot** above the target after confirmed damage in the current preview. Look at a ground item or rune in F6/F9 to see its localized name below the reticle. Settings and host limitations are in the [preview guide](DUAL-CURRENCY-HEADSHOTS-PREVIEW.md).
