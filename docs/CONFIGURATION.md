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

## Renderer selection

In launcher 0.15.0 or newer, choose **Graphics → Auto / OpenGL / Direct3D** before Play.
Auto uses OpenGL on 1.26a and Direct3D 9 on experimental 1.27a. The validated choice
is saved as `Renderer` in `%LOCALAPPDATA%/WarcraftCSLauncher/last-install.json`,
independently of the gameplay INI and RoC/TFT edition. Missing old preferences use Auto.
Restart Warcraft to apply a renderer change; F8 reloads game settings only. Install/Update
prepares the private 1.26a graphics components automatically.

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

Release 0.13.0 defaults to `[Movement] Mode=classic`, retaining movement before the 24 October
jump/collision redesign (2babf58). `Mode=recovery` enables the optional round-hull, platform support
and Home rescue; `Mode=hull` retains the intermediate round-hull build.
Mode names are case-insensitive; unknown/empty/missing values use classic.
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
| `Damage / FriendlyFirePercent` | `50`; 0–100%, including allied buildings and C4's planter. Zero disables allied damage. Direct attacks also require Q / `TargetAllies=true`. |
| `Combat / TargetAllies` | `false` on a fresh install; Q changes and saves direct allied targeting across F8 and restarts. The message reports the effective friendly damage percentage or a save failure. Protected allies are skipped by direct rays. |
| `Combat / EnableAttackEffects` | `true`; confirmed firearm/melee hits use native attack callbacks, passives and item orbs. Native chances, restrictions and orb priority remain. False selects direct damage. |
| `Combat / WaterBlocksAttacks` | `false`; traces cross water but still stop at solid beds, cliffs, decks, gates and bodies. True restores water-surface blocking. |
| `Combat / IgnorePassableInvulnerableObjects` | `true`; ignore nonphysical Locust effects and invulnerable noncolliding helpers. Physical invulnerable units still intercept rays. |
| `Combat / ScaleFireRateFromAttackSpeed` | `false`; true divides firearm intervals by the native attack-speed multiplier (0.2–5). Reloads, melee and planting retain their timing. |
| `Loadout / Mode`, `BombCount` | `melee`, `0`; the starting kit is USP, knife and sword. `all` grants all available weapons. Starting counts apply on the next map. |
| `Loadout / MaxBombs`, `UnlimitedBombPurchases` | `100`, `true`; free/rune bomb capacity is 0–1000. Paid purchases may exceed it unless disabled. Refills retain surplus bought charges. |
| `Buy / Access`, `Radius`, `ShopTypes` | `anywhere`, `600`, empty; `friendly` requires an allied building/shop, `shops` requires a shop. Add custom shop rawcodes with `ShopTypes`. |
| `Runes / AmmoPercent`, `AmmoWeapons` | `40`, `all`; rewards apply to owned weapons. `current` limits recovery to the held weapon. Rejected pickups grant no ammo. |

Weapon sections define `Price`, `AmmoPrice`, `AmmoPack`, `Magazine`, `MaxReserve`,
`Damage` and `HeroMultiplier`. Purchases use the selected **dollars or Warcraft gold**. A gun purchase gives
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

## Shopping and army

[Release 0.13.0](RELEASE-0.13.0.md) keeps reloading active while B is open or ammunition is purchased. Purchasing/equipping a different weapon still cancels the old reload.

`[ArmyAI] FollowActor=false` disables automatic hero escort while retaining economy and defense. Set true to restore escort. Accepts true/false or 1/0; missing, empty and malformed values default to false. F8 applies on the next active AI decision (`IntervalMs`); updates add missing keys once and preserve saved choices. Without a hall, reserve fighters remain in place instead of rallying to the actor. Explicit squad commands are unchanged.

## Currency, targeting and item names

Release 0.13.0 adds these startup/F8 settings. Installation updates fill missing keys and preserve custom choices.

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

Unsupported model/pose adapters never invent a head location. Experimental 1.27a retains approximate body hits and lacks item labels. Dollar rewards remain unchanged; the current movement default is described below.

## Visual combat feedback

Release 0.13.0 defaults movement to `classic`; explicit `hull` and `recovery` remain available. `[CombatVisuals]` controls confirmed headshot blood, floating **Headshot** text and cosmetic casing pools. Current blood defaults/ranges are in [0.16.0 settings](RELEASE-0.16.0.md#blood-settings-and-upgrades) and the annotated INI; [0.13.0](RELEASE-0.13.0.md#visual-settings) records the original visual controls. F8 clears old visual pools before applying the new settings. There is no centered HEADSHOT word.

[Release 0.16.0](RELEASE-0.16.0.md) increases stock blood and adds `HeadshotDecapitation=true` and `FatalBloodMultiplier=3` (1–4). Removal affects subsequent fatal hostile direct head contacts on verified 1.26a named-head models; the body follows native death/decay and resurrection restores the model. Blood can be disabled independently. Startup/F8 reload the settings; updates preserve custom values and opt-outs. `BloodVersion=2` preserves an intentional return to old blood defaults.

## Flash area stun

`[Flashbang] StunAllUnits=true` and `StunDistanceFalloff=false` apply the full `UnitStunSeconds=2` to every exposed creature, including the actor and allies. False on the first restores enemy-only filtering; true on the second enables distance falloff. Both booleans accept true/false or 1/0. Physical cover blocks exposure, and existing map pauses are never released by the adapter. F8 affects subsequent throws; updates preserve saved choices.

## Weapon balance

[Release 0.13.0](RELEASE-0.13.0.md) defaults to `[RPG] Damage=1800`, `Radius=600`; `[M3] HeroMultiplier=6`; `[XM1014] HeroMultiplier=4.8`; `[HEGrenade] Damage=600`. Shotgun multipliers apply to the whole shell in unit/hero damage mode, with the result divided among pellets. Fixed weapon-mode shell damage remains 104/80. Flash and smoke remain non-damaging.

Startup/F8 loads the same validated values as fresh installation. Damage accepts 0–1000000, RPG radius 32–3000 world units, and multipliers 0–1000. F8 applies to new shots/launches/throws; active projectiles keep their original snapshot.

Setup and the loader upgrade former stock values once: RPG 100 damage and 525 radius (or unversioned 375), M3 x3, XM1014 x2.4, and HE 300 (or unversioned 100). Custom values, explicit zero and other preserved versioned settings remain authoritative. `[RPG] DamageVersion=2` / `BlastVersion=3`, each shotgun's `MultiplierVersion=2`, and `[HEGrenade] DamageVersion=3` record the current balance. After an update, editing values back to older stock numbers with those markers retains them. Prior version-2 HE 100 and RPG radius 375 are also preserved. Missing keys are inserted once; repeated upgrades do not duplicate/reset them.

### Small body-impact blood (0.16.0)

`[CombatVisuals] BodyBlood=true` emits a small spray on confirmed enemy body damage, excluding buildings. Defaults: `BodyBloodCount=6` (1–64), `BodyBloodScale=0.65` (.25–4), `BodyBloodSeconds=0.35` (.1–3 s), `BodyBloodSpeed=110` (0–500 world units/s). `BloodGravity` also applies. Startup/F8 apply changes and clear old particles; updates add missing keys and preserve custom values. `BodyBlood=false` and `HeadshotBlood=false` independently disable body and head sprays. Body impacts have no Headshot text or fatal multiplier.

## Headshot and campaign improvements (0.14.0)

[Release 0.14.0](RELEASE-0.14.0.md) adds these editable startup/F8 settings; older installations need the new native module first. Installation updates add missing keys and preserve saved choices.

| Section / key | Default | Bounds / effect |
| --- | --- | --- |
| Combat / HeadshotBoneWeight | .35 | .05–1 minimum interpolated named-head skin influence at the actual contact |
| Combat / EnergyHeadshots | true | true/false or 1/0; enable direct Tau/Gluon bonuses; false retains anatomical blood only |
| ArmyAI / ShowStatus | true | true/false or 1/0; report development/blockers when state changes; U on/off still confirms |
| ArmyAI / PlacementAttempts | 12 | 1–32 total native build-site attempts per AI decision |
| ArmyAI / LumberWorkers | 2 | 0–10 unassigned undead ghouls for native tree harvesting; zero stops new assignments |
| Logging / GraphicsDebug | true | true/false or 1/0; per-pass OpenGL errors/driver messages and failed D3D9 calls; F8 reloads |

Head/contact policy applies to subsequent shots; visual-only beam tracking cannot emit damage or blood. A confirmed damaging named-head contact emits blood independently of the optional Headshots bonus; the existing HeadshotBlood flag can still disable it. A disabled bonus produces no Headshot label. Army settings apply on the next active AI decision. F8 requests a current status report; already accepted native orders retain ownership. Lumber changes never cancel existing manual harvesting. No resources or workers are created to bypass a campaign restriction.

GraphicsDebug is independent of Logging.Detailed. Detailed=false suppresses optional TRACE summaries/snapshots, while enabled graphics ERROR/WARN messages and essential handled-fault data remain. Repeated graphics errors share bounded suppression with counts; existing file rotation remains 8 MiB plus three archives by default. Synchronous driver delivery and additional state queries can affect frame time; GraphicsDebug=false disables the added observer live on F8. See [diagnostic coverage and limits](DIAGNOSTICS.md).

## Ground-height camera response (0.16.0)

`[Interface] CameraHeightSmoothing=true` softens small support-height transitions while walking in first person. `CameraHeightSeconds=0.08` is the response time in seconds, clamped to .01–.3; smaller values follow ground more closely. Missing, malformed and non-finite values use .08. Startup/F8 apply the settings; F8 clears the old correction. Updates add missing keys and preserve custom values. Set `CameraHeightSmoothing=false` for the previous direct height response.

Only camera/aim height receives the bounded correction. Horizontal position, collision, physical feet, jump acceleration and third-person boom behavior retain their existing rules. Camera, firearm contacts, spell/item aim and hover queries share one eye height. The correction stays inside the standing/crouched collision body, resets on large relocations and view/map/focus transitions, and decays through airborne movement without filtering the jump trajectory. Movement logs distinguish `physicalEye`, requested `eye` and `eyeCorrection`.

The [arsenal/smoke preview](ARSENAL-SMOKE-PREVIEW.md) documents the new TMP, UMP45, FAMAS, Galil and G3SG1 sections and `[SmokeGrenade] VisibilityDistance=96` / `BlocksAI=true`. Existing opacity and weapon tuning are preserved during updates.

The [F2 in-game settings panel](INGAME-SETTINGS.md) edits mouse sensitivity, CS volume, movement physics, view distance and sky. `[Mouse] Sensitivity=1` and `ScopeSensitivity=1` preserve previous aiming; `[Sky] Visible=true` adds explicit controlled-view sky visibility. Apply saves only changed keys; F8 still reloads manual edits.

The [nuclear bomb and chat preview](NUCLEAR-CHAT-PREVIEW.md) documents `[Chat]` and `[NuclearBomb]` keys, ranges and next-launch F8 snapshots.

[Release 0.17.0](RELEASE-0.17.0.md) collects all new weapon, smoke, mouse, sky, chat and nuclear defaults/ranges. Nuclear gameplay settings apply to future launches on F8; `[NuclearAudio]` layer edits require Install/asset reconversion. Existing custom codes survive; the former stock nuclear code upgrades to `warneverchanges`.
