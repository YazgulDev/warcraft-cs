# Warcraft CS 0.8.0

Released 2026-10-12. These notes cover all changes since the immutable **v0.7.0** tag:
FPS minimap and health bars, native hero spells and their HUD, configurable shortcuts,
rendering and movement fixes, squad avoidance, unit-scaled weapons, native attack effects
and revised ammunition/C4 defaults.

## Minimap, health bars and compact HUD

- **M** toggles the native minimap in the top-right corner of FPS. It retains Warcraft terrain,
  fog and unit markers. Rectangular maps retain black letterboxing; the original RTS map area
  is restored pixel for pixel, and leaving FPS preserves Warcraft's normal minimap layout.
- **K** toggles ally and enemy health bars together and saves both preferences to the INI.
  Allies are green and enemies red. Dead, hidden, transported and fog-hidden units are excluded;
  world depth hides bars behind obstacles. The controlled unit's HP remains in the bottom HUD.
- Top shortcut hints use compact groups, display configured keys and wrap before the minimap
  on narrow windows. Duplicate buy/ammunition hints are removed; gold appears beside HP/ammunition.

New `[Interface]` settings, all applied at startup and on **F8**:

| Key | Default | Supported value |
| --- | --- | --- |
| ShowMinimap | true | true/false; M changes the session only |
| MinimapSizePercent | 22 | 10–40 percent of window height |
| ShowAllyHealthBars | false | true/false; K saves both team switches |
| ShowEnemyHealthBars | false | true/false; independently editable in the INI |
| HealthBarDistance | 2000 | 0–15000 Warcraft units; 0 hides bars |
| ShowSpellIcons | true | true/false |
| ShowSpellCooldowns | true | true/false |
| SpellIconSizePercent | 7 | 4–14 percent of window height, fitted to narrow windows |

## First-person hero spells and configurable controls

- **Z / X / C / V** cast the four native hero spell slots. Aim at a unit or ground point with
  the reticle; hold **Alt** with a spell key to self-target. Native learning, mana, cooldown,
  targeting, casting/channeling and cast-range restrictions remain in effect.
- Preserve passive and unlearned slots instead of shifting remaining abilities to different keys.
  Resolve research columns even when a primary spell is fifth or later in a custom ability list;
  recognize learned replacements and relative passive icon paths, while excluding other research rows.
- Four bottom-right icons show configured keys, actor-owned command-button art, current mana/disabled
  states and a radial native cooldown sweep. Art is read locally from the current map/game archives.
- Reject unavailable requests before dispatch and preserve an active channel on a rejected request.
  DotA Shrapnel displays its map-owned charges, rejects empty-charge casts and reflects actual recharge.
- Explicit spells may approach targets within their native rules. Moving, firing, switching weapons
  or opening the buy menu interrupts casting/channeling. Unknown/passive slots show a notice.
- **P** now toggles speed; **V** is the fourth spell key. F6 remains a recovery alias; Escape/F10
  retain native menu behavior.

`[Shortcuts]` configures FPS toggle, movement, walking/crouching/jumping, reload/buy/ammo,
pickup, squad actions, speed/fog/minimap/health bars, free refill/all-weapons, settings reload,
four spells, self-cast and all seven weapon slots. Use physical English-layout keys: A–Z,
0–9, F1–F12, SPACE, CTRL, SHIFT, ALT, TAB, PERIOD or NONE. Enabled bindings must be unique;
Escape/F10 are reserved and ToggleFPS cannot be NONE. Invalid/conflicting layouts fall back
together to defaults and log the problem. F8 reloads bindings and matching HUD labels.

New `[Spells]` settings:

| Key | Default | Supported value |
| --- | --- | --- |
| Enabled | true | true/false |
| AimDistance | 3000 | 100–15000 Warcraft units; native cast range still applies |
| Slot1Ability … Slot4Ability | empty | empty for native research mapping, or a four-character rawcode |
| Slot1Order … Slot4Order | 0 | 0 for supported native order detection, or 852000–853999 |
| Slot1Target … Slot4Target | auto | auto, unit, point, immediate or self |

## Frame pacing, camera and movement

- Raise the default client frame cap to **144 FPS** and FPS world draw distance to **15000 Warcraft units**.
  Guard native render-demand changes and pace presentation using high-resolution elapsed time;
  movement follows elapsed time without changing Warcraft simulation timers.
- `[Rendering] MaxFPS=144` accepts 30–360; **0** restores Warcraft's original limiter, approximately 64 FPS.
  `[Rendering] DrawDistance=15000` accepts 3000–20000 Warcraft units. F8 applies both live.
  Hardware, driver overrides, map load, native visibility and atmospheric fog still limit results.
- Fix nearby ground disappearing above the native 10000-unit camera ceiling. Match the live camera's
  near clip and temporarily lift only the FPS far-channel ceiling; restore it on RTS exit, menus,
  faults and camera changes using generation keys. Log actual world projection separately from INI requests.
- Decode standing tree trunks for movement collision instead of canopy-sized extents, sharing weapon
  targeting geometry and retaining native pathing when model data cannot be read.
- Recognize diagonal cliff rims so landing below a ledge does not trap the player. Permit retreat
  from an existing solid overlap/native footprint while rejecting deeper penetration, new solids
  and unknown flat blockers. Add sampled rejection reasons and terrain/cliff coordinates to diagnostics.
- Block unwanted FPS pursuit/retaliation before chase starts and stop the separate native walking
  motor from moving an idle actor. Preserve scripted placement, spell approach, speed modifiers,
  other units and ordinary RTS control. Keep native attack-target references valid during morphs;
  fix Avatar and other transformation crashes.

## Squads

Recruited followers stay behind the player and yield sideways when obstructing the forward corridor,
then rejoin their formation. New `[Squad]` settings reload on F8:

| Key | Default | Supported value |
| --- | --- | --- |
| RearFormation | true | true/false |
| YieldToPlayer | true | true/false |
| PlayerClearance | 100 | 60–300 Warcraft units |
| YieldAheadDistance | 350 | PlayerClearance–1000 Warcraft units |
| OrderIntervalMs | 150 | 50–1000 milliseconds |

## Weapons, native passives and ammunition

- Default `[Damage] Mode` changes to **unit**: current average native attack, including attack bonuses,
  multiplied by each weapon's HeroMultiplier. It works for heroes and ordinary units. **hero** remains
  a compatibility alias; **weapon** retains fixed INI damage. Native armor and friendly-fire scaling remain.
- `[Combat] EnableAttackEffects=true` lets confirmed firearm/melee contacts run native preparation,
  impact and cleanup, preserving learned passives, item orbs, native chances/restrictions, orb priority
  and map attack callbacks without pursuit orders. F8 switches it; false uses direct damage.
  C4 explosions remain separate from native weapon attacks.
- Fix passive-enabled firearm hits beyond the actor's native attack range by scoping the engine range
  check to the confirmed primary FPS contact. Keep weapon/terrain ray limits, evasion, ordinary RTS
  ranges and secondary-component behavior. Log contact distance and applied effects.
- `[Combat] IgnorePassableInvulnerableObjects=true` lets shots/melee pass through Locust effects and
  invulnerable noncolliding dummies, including Broodmother webs. Heroes, structures, vulnerable units,
  physical invulnerable bodies and unknown geometry still block contacts. F8 reloads it; log skipped hits.
- New firearm `MaxReserve` caps (0–100000 rounds): AK47 **90**, M4A1 **90**, USP **100**, AWP **30**.
  Purchases, free refills and runes share the cap; lowering it with F8 discards surplus reserve.
- `[Combat] ScaleFireRateFromAttackSpeed=false` optionally scales firearm cadence from native attack
  speed: 1.0 retains cadence, 2.0 doubles it; reload speed is unchanged.
- `[Movement] ScaleFromUnitMoveSpeed=false` optionally scales weapon movement speed by the unit's
  native movement speed divided by `UnitMoveSpeedReference=300` (1–1000 Warcraft units/s).
  The existing slow/haste behavior remains when disabled; roots, slows and haste remain native.
- Buy previews show current unit-scaled damage, firing cadence and magazine/reserve limits.
- Log active AWP finishing and native-effect settings per contact. A preserved `AWPOneShot=true`
  preference is independent of ordinary unit-scaled damage; the default remains false.

## Economy and C4 defaults

- Double the previous weapon prices: AK47 **1250**, M4A1 **1550**, USP **250**, AWP **2376**,
  knife **0**, sword **500** gold. Existing firearm ammo-pack prices remain.
- Increase `[Runes] AmmoPercent` from 20 to **40** for fresh configurations.
- Both `[C4] Price` and `AmmoPrice` are **3000 gold per charge**.
- `[Loadout] BombCount=0` removes C4 from the default starting kit. A positive count (0–1000)
  or explicit all-weapons mode opts in. Starting kit changes take effect on the next map, not F8.
- F7 still grants/refills C4 for free; F9 still grants every weapon/ammunition.
- `[Loadout] UnlimitedBombPurchases=true` permits paid repeat purchases beyond MaxBombs.
  When false, MaxBombs caps purchases. MaxBombs still controls free-refill/rune amounts;
  refills and F8 preserve surplus purchased bombs. The switch applies on F8.

## Upgrade and downloads

Close Warcraft after saving your progress, then use **Update** in an updater-enabled launcher or
install the new EXE into the existing client folder. Existing saves, custom INI values and private
content are preserved; missing keys receive defaults. **Existing damage modes, prices, rune rewards,
starting bomb counts, AWP finishing and display preferences are not overwritten.** Edit the private
`Game/WarcraftCS/WarcraftCS.ini` and press F8 in FPS to adopt different values.

- `WarcraftCSLauncher_DLL_Included.exe`: ready x86 project modules; **Install — Player** needs no C++ Build Tools/SDK.
- `WarcraftCSLauncher.exe`: source installation, with a local C++ build after dependency consent.
- `WarcraftCS-0.8.0-dist.zip`: standard launcher, exact source archive, manifest, requirements and launcher checksum.
- Separate sources, SHA256 files and updater manifest accompany the release. No game assets or retail DLLs are included.

## Validation, documentation and limitations

The release reruns native-independent regression suites, real INI/migration checks, launcher/update
regressions and both packaged-EXE integrity checks. The source snapshot is audited for proprietary
content, binaries, credentials and private paths. Ordinary Player modules exclude opt-in test scenes.

Feature development also includes approved disposable Warcraft/DotA tests of minimap/health bars,
extended clipping, spell mappings/cooldowns/charges, idle movement, transformations, squad behavior,
stock and DotA Mana Burn/lifesteal/Bash, distant firearm contacts and Broodmother webs.
These are bounded checks, not a claim of full map/campaign compatibility. Details and limitations:
[Validation](https://github.com/YazgulDev/warcraft-cs/blob/v0.8.0/docs/VALIDATION.md).

Update README, installation/requirements, movement, diagnostics, module ownership, tests and source
notices for the shipped features. Preserve old branches and immutable tags; rebuild both launcher
variants and their version-specific summaries. Full notes stay visible on GitHub; the launcher
shows a separate compact English list. Historical 0.7.0 summaries are restored to their published list.

Supported host remains **offline Warcraft III 1.26a x86 (1.26.0.6401)** with locally owned CS 1.6 assets.
Multiplayer, Reforged, other Warcraft patches and arbitrary custom ability implementations are unsupported
or unverified. Custom spells may need explicit slot mappings/orders; map scripting and native restrictions
still control their behavior. F6 returns to RTS when a map requires normal controls.

**Antivirus:** the unsigned loader hooks game functions, behavior that heuristic scanners can flag.
A Defender detection was recorded for the 0.5.0 module; its exact vendor cause is unconfirmed.
The project remains unsigned. See [troubleshooting](https://github.com/YazgulDev/warcraft-cs/blob/v0.8.0/TROUBLESHOOTING.md#antivirus).

<!-- launcher-summary
- Added an FPS minimap and ally/enemy health bars.
- Added first-person spells, live spell icons and cooldowns.
- Added configurable keyboard shortcuts.
- Improved FPS pacing and extended the default draw distance.
- Fixed ground clipping, tree collision and getting stuck near cliffs.
- Fixed unwanted pursuit, idle walking and transformation crashes.
- Improved squad formations and avoidance around the player.
- Added unit-scaled damage, optional firing speed and reserve ammo limits.
- Added native weapon passives and fixed distant passive hits.
- Fixed webs and nonphysical effects blocking weapon hits.
- Changed weapon prices, rune rewards and C4 starting inventory.
- Added unlimited C4 purchases and compact HUD hints.
-->
