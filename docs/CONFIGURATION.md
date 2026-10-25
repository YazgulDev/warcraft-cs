# Settings

[Project home](../README.md) · [Documentation](README.md) · [Controls](CONTROLS.md)

## Edit and reload

Edit the installed `WarcraftCS.ini`:

| Installation | Location |
| --- | --- |
| EXE launcher | `<installation folder>/Game/WarcraftCS/WarcraftCS.ini` |
| Default source setup | `.local/warcraft-cs/WarcraftCS/WarcraftCS.ini` |

Save and press **F8** in a controlled view. Most settings apply immediately;
starting kits apply on the next map, and live explosives retain their launch/plant
settings. Installation updates add missing keys and preserve custom values.

The [annotated INI](../config/WarcraftCS.ini) is the complete list of keys, defaults,
units, bounds and reload rules. Values below describe the published release;
an older installation may retain different saved choices.

## Guides by topic

| Topic | Guide |
| --- | --- |
| Air strafing, jumping and collision | [Movement](MOVEMENT.md) |
| Owned weapon groups and wheel preview | [Weapon selection](WEAPON-SELECTION.md) |
| Firearms, prices, ammo, grenades and army AI | [Arsenal and shops](ARSENAL-SHOPS-AI.md) |
| Rockets and laser guidance | [Half-Life RPG](HALF-LIFE-RPG.md) |
| Native attacks, camera distance and indicators | [Third-person control](THIRD-PERSON.md) |
| Log detail, rotation and renderer traces | [Diagnostics](DIAGNOSTICS.md) |

## Keyboard and interface

`[Shortcuts]` remaps movement, weapon groups, spells, buying, squads and display
switches. Use English physical keys: `A-Z`, `0-9`, `F1-F12`, `SPACE`, `CTRL`,
`SHIFT`, `ALT`, `TAB`, `HOME`, `PERIOD`, `SEMICOLON`, or `NONE` to disable an optional action.
Keys must be unique; change conflicting bindings together. Invalid layouts fall
back to defaults and log the reason. Escape/F10 are reserved, and F6 remains the
FPS recovery key. Mouse actions and buy-menu row numbers remain fixed.

| Section / keys | Defaults and behavior |
| --- | --- |
| `Audio / CSVolumePercent` | `100`; 0–100%, including fractional values. F8 affects playing and future CS sounds; Warcraft audio keeps its own volume. |
| `Interface / ShowMinimap`, `MinimapSizePercent` | `true`, `22`; side length 10–40% of window height. M changes session visibility; F8 restores the saved preference. |
| `Interface / ShowSpeed` | `false`; P changes session visibility. Speed is horizontal CS units/s. |
| `Interface / ShowSpellIcons`, `ShowSpellCooldowns`, `SpellIconSizePercent` | `true`, `true`, `7`; size 4–14% of window height. Icons follow the controlled unit's abilities. |
| `Interface / ShowUnitHud`, `UnitHudScalePercent` | `true`, `100`; portrait, health, mana and hero XP; scale 60–160%. |
| `Interface / ShowInventory`, `InventoryControls` | `true`, `true`; six native slots, item-number keys and mouse inventory in F6/F7. |
| `Inventory / NumbersByDefault`, `DropDistance` | `true`, `80`; item numbers initially on, drop offset 16–128 world units. Tab changes the current mode. |
| `Interface / ShowAllyHealthBars`, `ShowEnemyHealthBars`, `HealthBarDistance` | `false`, `false`, `2000`; distance 0–15000 world units, zero hides bars. K toggles and saves both teams. |
| `Interface / FloatingTextDistance` | `1200` world units; zero hides native world labels in controlled views. |
| `Interface / DisableFogOfWar` | `false`; N temporarily removes/restores fog and the unexplored mask in offline FPS. RTS and cinematics restore map visibility. |
| `UnitView / HideHoverCircles`, `PreventEnemyAttacks` | `true`, `true`; F9 hover presentation and protection from enemy basic-attack targeting. |

HUD spell markers are `LV0` for unlearned, `PASS` for passive, `EMPTY` for unmapped,
and `?` for an unsupported native timer. Warcraft still decides whether a cast or
item use succeeds. F9 hides the HUD and disables player attacks, casts and inventory
interaction regardless of these display preferences.

## Movement collision

Release 0.12.0 defaults to `[Movement] Mode=recovery`, enabling the latest round-hull,
platform support and Home rescue. Set `Mode=classic` for movement before the 24 October
jump/collision redesign (2babf58). `Mode=hull` retains the intermediate round-hull build.
Mode names are case-insensitive; unknown/empty/missing values use recovery.
Save and press F8 to switch without resetting motion. Updates preserve an explicit mode.

Optional hull/recovery modes use `[Movement] MaxWalkSlope=46` (15–75 degrees) and
`HullRadius=24` (8–64 world units, the maximum player radius). Smaller native actor
radii are retained. Both apply on F8 without resetting current position/velocity;
updates preserve explicit values. `ObjectMeshCollision=true` (true/false) uses supported stationary
prop surfaces for actual-height fit, floor and head clearance; false restores coarse object volumes.
It also reloads on F8 and survives updates. Unknown/animated geometry retains conservative fallback.
In `Mode=recovery`, `RecoverInvalidPosition=true` (true/false) repairs proven buried states and lost support for
an established platform rider. It never promotes someone walking below a bridge onto its deck.
`RecoveryDistance=512` (64–2048 horizontal Warcraft units) bounds the search through visited
checkpoints. Both reload on F8; updates preserve explicit values. In recovery mode, Home remains available
when automatic repair is disabled, unless `Shortcuts.RecoverPosition=NONE`.
Install and restart once for the native hull/terrain code. See [movement contacts and geometry limits](MOVEMENT.md).

## Damage, buying and ammunition

| Section / keys | Defaults and behavior |
| --- | --- |
| `Damage / Mode` | `unit`: live Warcraft attack damage times each weapon's `HeroMultiplier`. `hero` is a compatible alias; `weapon` selects fixed per-weapon damage. C4 stays fixed. |
| `Damage / AWPOneShot` | `false`; opt into enemy finishing in weapon mode. Ordinary AWP uses its configured damage and native armor. |
| `Damage / FriendlyFirePercent` | `50`; 0–100%, including allied buildings and C4's planter. Zero disables allied damage. |
| `Combat / TargetAllies` | `false`; Q changes direct allied targeting for the session. Protected allies are skipped by direct rays. |
| `Combat / EnableAttackEffects` | `true`; confirmed firearm/melee hits use native attack callbacks, passives and item orbs. Native chances, restrictions and orb priority remain. False selects direct damage. |
| `Combat / WaterBlocksAttacks` | `false`; traces cross water but still stop at solid beds, cliffs, decks, gates and bodies. True restores water-surface blocking. |
| `Combat / IgnorePassableInvulnerableObjects` | `true`; ignore nonphysical Locust effects and invulnerable noncolliding helpers. Physical invulnerable units still intercept rays. |
| `Combat / ScaleFireRateFromAttackSpeed` | `false`; true divides firearm intervals by the native attack-speed multiplier (0.2–5). Reloads, melee and planting retain their timing. |
| `Loadout / Mode`, `BombCount` | `melee`, `0`; the starting kit is USP, knife and sword. `all` grants all available weapons. Starting counts apply on the next map. |
| `Loadout / MaxBombs`, `UnlimitedBombPurchases` | `100`, `true`; free/rune bomb capacity is 0–1000. Paid purchases may exceed it unless disabled. Refills retain surplus bought charges. |
| `Buy / Access`, `Radius`, `ShopTypes` | `anywhere`, `600`, empty; `friendly` requires an allied building/shop, `shops` requires a shop. Add custom shop rawcodes with `ShopTypes`. |
| `Runes / AmmoPercent`, `AmmoWeapons` | `40`, `all`; rewards apply to owned weapons. `current` limits recovery to the held weapon. Rejected pickups grant no ammo. |

Weapon sections define `Price`, `AmmoPrice`, `AmmoPack`, `Magazine`, `MaxReserve`,
`Damage` and `HeroMultiplier`. Purchases use **Warcraft gold**. A gun purchase gives
a loaded magazine; reserve ammo costs extra. Duplicate weapons, full packs and
rejected purchases do not charge gold. F11/F12 are free assistance shortcuts.
Exact prices and capacities are in the [INI](../config/WarcraftCS.ini).

Native immunity, armor and map damage scripts still apply. Disabling attack effects
does not disable the map's general damage triggers. A unit with no normal attack
deals zero weapon damage in unit mode. F8 can clamp reduced ammo capacities but
does not grant a fresh starting kit.

## Spells and squads

`[Spells] Enabled=true`, `AimDistance=3000` enable native hero-spell requests.
Aim distance is 100–15000 world units and limits reticle selection, not native cast
range. Passive/unlearned gaps remain empty. Point spells require a terrain point;
Alt targets the actor when allowed. Movement, firing, reloading, weapon changes and
opening the buy menu can interrupt an active cast. Warcraft owns mana and cooldowns.

For a custom hero, set `Slot1Ability=AHtb`, `Slot1Order=852095` and
`Slot1Target=unit`, repeating for slots 2–4. Ability values are rawcodes, order IDs
are 852000–853999, and target modes are `auto`, `unit`, `point`, `immediate`, `self`.
Empty ability/zero order uses automatic lookup. Extra custom abilities and dynamic
order replacements are not universally discoverable.

H/O/J recruit or release followers. `[Squad]` controls recruitment radius, cap,
formation, combat leash and yielding; native pathfinding still applies.
`ReticleAttack=true` allows RMB target orders in F7/F9. In armed FPS,
`ReticleAttackInFPS=false` starts with weapon actions; Y switches the current role.
`ReticleAttackDistance=3000` is 100–15000 world units. These orders reach available
owned mobile units independently of the H/O recruitment radius. Empty aim clears
the common target. Direct allied targets require Q/`TargetAllies` opt-in.

`PreserveCombatOrders=true` keeps fighting followers in combat rather than pulling
them back into formation. `AutomaticSpells=true` allows recognized stock casts;
O disables this module's combat casts for followers, and J releases them.

| Squad key | Default | Bounds |
| --- | ---: | --- |
| `SpellSearchRadius` | 700 | 50–2000 world units |
| `AreaSpellRadius` | 250 | 16–1000 world units |
| `HealBelowPercent` | 85 | 1–100% health |
| `BuffRefreshSeconds` | 20 | 1–600 seconds |
| `SpellIntervalMs` | 1000 | 100–10000 ms |

Custom spell orders, arbitrary item abilities, passives and toggles are not
automated. Existing native autocast settings may still act independently.

## Rendering and sky

`[Rendering] MaxFPS=144` sets a target of 30–360 FPS; `0` restores Warcraft's
native limiter. `DrawDistance=5000` accepts 3000–20000 world units. Actual performance
depends on hardware and map load; farther rendering does not reveal fog-hidden units.

`[Sky] WarcraftEnabled=true` supplies the native sky on maps without one.
`Enabled=false` keeps Warcraft skies; true uses privately converted CS skyboxes.
`Default=Des` and tileset mappings such as `W=snow` choose cache stems. Missing
caches retain the native sky. F8 applies changes; map/RTS transitions restore the
map's original state.

## Local feature preview settings

The [dual-currency/headshot preview](DUAL-CURRENCY-HEADSHOTS-PREVIEW.md) adds these startup/F8 settings. Installation updates fill missing keys and preserve custom choices. Published 0.12.0 downloads do not yet contain these additions.

| Section / key | Default | Meaning / range |
| --- | --- | --- |
| Economy / Currency | dollars | Initial and F8 menu choice: dollars or gold; click buttons in B for a session change |
| Economy / FallbackToGold | true | Pay the whole price in gold when dollars are insufficient; no mixed charge |
| Combat / PreciseUnitHits | true | Trace verified posed MDX surfaces; false restores old unit volumes |
| Combat / Headshots | true | Enable direct named-head bonuses and confirmed HUD feedback |
| Combat / HeadshotMultiplier | 2 | Damage factor, 1-10, before native defenses and allied scaling |
| Interface / ItemNames | true | Localized reticle names for ground items/runes in F6/F9 |
| Interface / ItemNameDistance | 1200 | Maximum ray length, 160-5000 world units |
| Interface / ItemNameRadius | 32 | Spherical aim tolerance, 4-128 world units |

Unsupported model/pose adapters never invent a head location. Experimental 1.27a retains approximate body hits and lacks item labels. Dollar rewards remain unchanged; the current preview movement default is described below.

## Current visual feedback preview

The [classic/blood/casings preview](CLASSIC-BLOOD-CASINGS-PREVIEW.md) changes the fresh/default movement mode to `classic`; explicit `hull` and `recovery` remain available and saved choices survive installation. Its `[CombatVisuals]` keys control confirmed blood/critical text and cosmetic casing pools. All defaults, units, ranges and F8 behavior are listed in that guide and the annotated INI. No centered HEADSHOT word is drawn in this preview.
