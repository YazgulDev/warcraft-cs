# Warcraft CS 0.9.1

Counter-Strike 1.6 inside Warcraft III: control your own hero or unit in first person,
shoot, and play regular maps, campaigns and compatible custom maps.
The world, enemies, quests and health remain governed by Warcraft.

An unofficial single-player fan project, unaffiliated with Blizzard Entertainment or Valve.
**This Git repository contains source code. The standard launcher and release ZIPs contain
project source without prebuilt DLLs. A separate DLL-included EXE supplies ready-to-use project modules.
Neither variant includes game files, models, sounds, maps or saves.
You need your own Warcraft III installation. CS 1.6 is required only for the full weapon mode.**
This is an early prototype with known limitations.

**[Download 0.9.1](https://github.com/YazgulDev/warcraft-cs/releases/tag/v0.9.1)** В·
[Release Notes](docs/RELEASE-0.9.1.md) В· [Installation](INSTALL.md) В· [Requirements](REQUIREMENTS.md)
В· [Troubleshooting](TROUBLESHOOTING.md) В· [Discussions](https://github.com/YazgulDev/warcraft-cs/discussions)

0.9.1 fixes HUD text appearing as solid bars after F6 or F7. Shortcut hints, health/ammunition,
buy-menu text and spell labels now use alpha glyph textures instead of WGL bitmap display lists.
No gameplay settings or defaults change. See the [0.9.1 release notes](docs/RELEASE-0.9.1.md)
for the fix, upgrade steps and validation limits; the [0.9.0 notes](docs/RELEASE-0.9.0.md)
describe the preceding gameplay and installation features.

## Features

- Both launchers offer **Only Warcraft modes**: F7 third person with native attacks/spells,
  F9 first person without weapons/UI. No CS folder or conversion; Player also skips Python/NumPy.
  T switches between these two views and F6 returns to RTS. Uncheck and Install with CS to restore weapons.
- Windows EXE launcher with game-folder selection, consent before dependency setup, live logs and Play.
- Remembered Reign of Chaos / The Frozen Throne selection; `play-roc.bat` starts RoC from a source installation.
- F6 switches between FPS and RTS; fullscreen launch, relative mouse look and hidden player-unit model.
- Consistent fullscreen DPI scaling and texture/context recovery after Alt-Tab; native hero portraits keep their layout.
- WASD, jumping, crouching, acceleration, friction and weapon movement speeds inspired by CS.
- Half-Life-style air strafing and held-Space bunnyhops; jump over low solids and fall from cliffs.
- P toggles a horizontal speed counter; N toggles offline FPS fog and the unexplored mask.
- Z/X/C/V cast the four learned hero spell slots from first person; keyboard shortcuts are configurable.
- K toggles ally (green) and enemy (red) health bars together and saves the choice in the INI.
- **M** toggles the native FPS minimap.
- The bottom-right spell panel shows the four mapped ability icons, configured keys and native cooldowns.
- Filled crosshair geometry and automatic control recovery after native campaign movies.
- Classic AK/M4 and USP/AWP punch calculations, recoil recovery and burst behavior.
- Primary and secondary melee attacks, locally imported CS sounds and a blood-free hit indicator.
- CS-only volume in INI, including shots, reloads, footsteps and C4; F8 applies it to ongoing and future sounds.
- Damage to units, buildings and gates; configurable allied damage (50% by default). C4 deals 2500 base area damage.
- Weapons scale damage from the controlled unit attack by default; fixed weapon damage remains configurable.
- Native weapon passives and item orbs apply to confirmed contacts, including distant firearm hits.
- AWP uses ordinary configured damage by default; instant enemy kills remain an explicit INI option.
- Warcraft stuns, roots, slows and attack restrictions also limit FPS actions.
- E picks up items/runes through the real inventory; a successful rune restores 40% ammunition by default.
- H recruits your own units to follow and fight, O makes them follow without attacking, J releases them.
- INI settings for ammunition recovery, damage and squad behavior, reloaded with F8.
- Mouse-wheel weapon cycling through all seven slots, with high-resolution wheel support.
- B opens a CS-style buy menu using Warcraft gold; configurable prices, starting kits and shop/building access.
- No starting C4 by default; unlimited paid C4 purchases and configurable reserve ammunition caps.
- Multiple planted C4 charges with independent fuses; F11 refills carried equipment and F12 grants every weapon for free.
- Warcraft sky by default in FPS, optional privately converted CS skyboxes, and visible native walkable bridge decks.
- Configurable 144 FPS pacing and 5000-unit default draw distance; corrected near-ground clipping.
- Trunk-based tree collision, cliff retreat, stable idle movement and safe transformations.
- Rear squad formations and sideways yielding around the player.
- Camera/distance filtering for floating world labels.

## Requirements

- Windows x64 and **Warcraft III 1.26a x86**, with `Game.dll` version `1.26.0.6401`.
- For CS weapons: installed Counter-Strike 1.6 with loose files in `cstrike/models` and `cstrike/sound`.
  The Warcraft-only profile does not require CS or asset conversion.
- Standard `WarcraftCSLauncher.exe` builds locally and needs C++ Build Tools / Windows SDK.
  The separate `WarcraftCSLauncher_DLL_Included.exe` offers Player setup without those tools.
  Both may prepare Python/NumPy after agreement to convert your locally owned CS assets.
- For manual source setup: Git, Python 3.10вЂ“3.14, Visual Studio 2022 or Build Tools with
  **Desktop development with C++**, MSVC x86 and Windows SDK.
- OpenGL support and enough disk space for a separate copy of Warcraft.

Reforged and other Warcraft patches are not supported.
The full game-file, system, build-dependency and updater requirements are in [REQUIREMENTS.md](REQUIREMENTS.md).

## Setup and play

### Windows client launcher

Choose a launcher from **[Releases](https://github.com/YazgulDev/warcraft-cs/releases/latest)**:

| Download | Installation |
| --- | --- |
| `WarcraftCSLauncher.exe` or the distribution ZIP | Source-only EXE; **Install** compiles the mod locally and may prepare missing Build Tools / SDK (several GB). |
| `WarcraftCSLauncher_DLL_Included.exe` | Separate EXE with ready mod DLLs; **Install вЂ” Player** avoids Build Tools / SDK. **Install вЂ” Developer** builds locally. |

Clients do not need Git or a separate source checkout. All public ZIPs exclude prebuilt mod DLLs.
**Antivirus:** Defender detected the bundled `WarcraftCS.mix` in 0.5.0. The unsigned mod uses
native function hooks and a DLL loader, which can resemble suspicious behavior to heuristic scanners.
This is a plausible explanation; the exact detection cause has not been confirmed by the vendor.
Source builds can also be flagged. See [troubleshooting](TROUBLESHOOTING.md#antivirus).

Choose these folders using **Browse...** (the examples are illustrative):

| Launcher field | Which folder to choose | Example |
| --- | --- | --- |
| Warcraft III 1.26a folder | The game root containing `war3.exe`, `Game.dll`, `Mss32.dll` and the Warcraft MPQ archives. Do not choose `Maps` or `save`. | `E:\Warcraft III` |
| Counter-Strike 1.6 folder | `cstrike`, containing `models/v_knife.mdl` and `sound`; the launcher also accepts its `Half-Life` parent. Do not choose the Steam library root or `models` alone. | `C:\SteamGames\steamapps\common\Half-Life\cstrike` |
| Install Warcraft CS here | A dedicated writable folder outside both game installations. This receives a separate Warcraft copy. | Default: `%LOCALAPPDATA%\WarcraftCS`, or `D:\WarcraftCS` |

1. Select your installed Warcraft III 1.26a folder and CS 1.6 folder (`cstrike` or its Half-Life parent).
2. Select a separate installation folder for Warcraft CS.
3. Read the download details and agree to downloading/installing the dependencies and their terms.
4. Click **Install** in the standard launcher, or choose **Install вЂ” Player / Developer** in the DLL-included launcher. Install uses the version embedded in that EXE.
5. Choose **The Frozen Throne** or **Reign of Chaos** under **Game to launch**, then click **Play**, select your own living unit and press F6. The choice is remembered after a successful launch.

Both editions use the same private Warcraft III 1.26a installation. RoC selects the original campaign menus;
it does not turn two-player custom maps into solo missions. Launch through the launcher or `tools/launch.ps1`
to keep fullscreen DPI scaling consistent after Alt-Tab. For source users, double-click `play-roc.bat`
or run `play.cmd -Edition ReignOfChaos`; `play.cmd` alone starts Frozen Throne.
`-Windowed` and `-Map` remain available for either edition.

The launcher checks new stable GitHub releases automatically at every startup; **Check for updates**
retries the check. The separate **Update** button fetches the latest stable project release from GitHub.
After confirmation it reinstalls that release even when the installed version and revision already match.
A window shows its version and notes: **Update** confirms installation; **Not now** postpones it.
Both launchers show one change per line. Reviewed lists in `docs/launcher-changes/<version>.txt`
are included in the update manifest and embedded for known legacy versions, so GitHub API limits
do not replace release changes with a generic announcement. See the release page for full notes.
The source-only launcher keeps source-only updates and builds locally, including when updating a previous
Player installation. The DLL-included launcher retains its variant and saved Player/Developer mode.
The dialog explains download requirements and consent.
Warcraft must be closed. Player installs verified bundled DLLs; Developer rebuilds them locally. Both
modes convert owned assets and update the launcher if needed. Saves, INI settings and private sword selections are
retained. Each session requires the download agreement. Offline Play remains available.
Install and Update both recopy owned Warcraft files, regenerate private CS assets and reinstall/build
the selected mod DLLs. Missing/corrupt game resources are repaired in the marked private `Game` folder.
Private-only maps remain; edited content that shares an original map/campaign filename is backed up
under `backups/runtime-content/<id>/` before replacement. Original game installs remain unchanged.

No download or installation starts before agreement. Microsoft tools may need administrator approval,
several GB of space and a Windows restart for local compilation. Only the separately named EXE includes prebuilt modules.
The launcher itself is unsigned; dependency installers have verified vendor signatures.
Detailed folder examples, first launch, updates and removal: [INSTALL.md](INSTALL.md).
Developer build instructions: [Client launcher](docs/CLIENT-LAUNCHER.md).

Both launchers include **Read logs**. It opens game/session archives, installation,
launcher and replacement journals directly, with refresh, copy and full-file export. See
[Diagnostics](docs/DIAGNOSTICS.md) for logging controls and retention.

If the old 0.5.0 Player updater reports that this release requires a source build, download the new
DLL-included EXE directly and select your existing installation folder. No compiler download is silently enabled.

### Source setup

Run in PowerShell, replacing the game paths with your own:

```powershell
git clone https://github.com/YazgulDev/warcraft-cs.git
cd warcraft-cs
git switch release/0.9.1
.\setup.cmd -WarcraftDirectory "E:\Warcraft III" -CounterStrikeDirectory "C:\SteamGames\steamapps\common\Half-Life\cstrike"
.\play.cmd
```

Setup downloads a pinned MinHook dependency separately, installs NumPy in a local environment,
reads your CS models/sounds/sky textures, uses an optional privately supplied sword or generates the original sword,
and builds the mod in a private Warcraft copy.
Your original installations remain unchanged. Additional installation and removal details are in
[INSTALL.md](INSTALL.md).

Choose **Single Player в†’ Custom Game** or **Campaign**, select your own living unit and press F6.
Individual maps do not need editing. Enable FPS again after changing maps.
Native campaign movies can replace Warcraft's window; mod controls reconnect automatically after
playback so F6 enables FPS in the next mission.

Optional launches:

```powershell
.\play.cmd -Windowed
.\play-roc.bat
.\play-roc.bat -Windowed
.\play.cmd -Map "E:\Warcraft III\Maps\(4)LostTemple.w3m"
```

Close Warcraft before updating its private runtime. Preserve your progress first.

## Controls

| Key | Action |
| --- | --- |
| F6 / F10 | Toggle FPS в†” RTS / pause menu and return to RTS |
| WASD / mouse | Move / look and aim |
| Shift / Ctrl / Space | Walk / crouch / jump (hold Space for repeated hops by default) |
| Left mouse button | Shoot or primary melee attack |
| Right mouse button | Knife/sword: strong attack; AWP: cycle two zoom levels and normal view |
| F7 | Enter third person directly; press again in third person to return to RTS |
| R / F11 | Reload / refill all carried ammunition for free |
| F9 | Enter unarmed first-person unit view; press again to return to RTS |
| F12 | Get all weapons and full ammunition for free |
| B | Open/close the buy menu; number keys or mouse select categories and purchases |
| . | Buy one ammo pack for the weapon currently held |
| Esc / 0 in buy menu | Close / back to previous menu (0 closes the main menu) |
| 1 / 2 / 3 / 4 / 5 / 6 / 7 | AK47 / M4A1 / USP / AWP / knife / C4 / original greatsword |
| Mouse wheel up / down | Previous / next owned weapon |
| Hold left mouse button with C4 | Plant for 3 seconds while standing on the ground; multiple charges are allowed |
| E | Pick up the nearest item/rune if the unit has an available inventory |
| H / O / J | Your units: follow and fight / follow without attacking / release squad |
| F8 | Reload settings |
| P | Show/hide horizontal speed in CS units/s |
| M | Show/hide minimap |
| N | Disable/restore fog of war while in FPS |
| K | Show/hide allied and enemy health bars together |
| Z / X / C / V | Cast hero spell slots 1вЂ“4; target the visible unit or world surface under the reticle |
| Alt + spell key | Target yourself when the spell allows it |

## Configuration

The local third-person prototype adds **F7** to enter directly with a selected owned mobile unit.
WASD runs and Shift+WASD walks using that unit's authored model animations. F7/F6 return to RTS;
T switches FPS/third person. The reticle, native melee/ranged primary attacks and Z/X/C/V spells remain available.
See [third-person controls, settings and limits](docs/THIRD-PERSON.md).
Mouse wheel up/down moves the third-person camera closer/farther. `[ThirdPerson] CameraZoomStep=50`
sets the step; `CameraMinDistance=80` / `CameraMaxDistance=1000` bound the requested distance.
F8 reloads these settings and resets zoom to `CameraDistance`; wheel choices otherwise last for the map.
Reachable targets have brackets and a ground circle: red for enemies, yellow for other contacts.
`[ThirdPerson] ShowTargetIndicator` hides/shows both; `TargetCircleRadius=64` sets the circle radius
in Warcraft units (16..256). Change them in `Game/WarcraftCS/WarcraftCS.ini` and press F8 to reload.

EXE installation: `<installation folder>/Game/WarcraftCS/WarcraftCS.ini`.
Default source installation: `.local/warcraft-cs/WarcraftCS/WarcraftCS.ini`.
Edit it and press F8 in FPS.

- `[Shortcuts]` configures all keyboard actions listed in the shipped INI, including movement,
  weapon slots, spell slots, pickup, buying, squad commands, display switches and settings reload.
  Defaults are `Spell1=Z`, `Spell2=X`, `Spell3=C`, `Spell4=V`, `Speed=P`, `Minimap=M`, `SelfCast=ALT`.
  Values are English physical `A-Z`, `0-9`, `F1-F12`, `SPACE`, `CTRL`, `SHIFT`, `ALT`, `TAB`,
  `PERIOD` (or `.`), and `NONE` to disable an action. Keys are case-insensitive and must be unique;
  swap all conflicting bindings together. An invalid layout falls back to the complete default layout
  and logs why. Escape/F10 stay reserved; F6 always remains an FPS recovery alias and may only bind
  ToggleFPS. ToggleFPS cannot be disabled. F8 (or the configured ReloadSettings key) applies edits.
  HUD labels follow the same loaded bindings. Mouse actions and buy-menu row numbers remain fixed.
  Install/Update adds missing keys while preserving existing shortcuts.

- `[Spells] Enabled=true`, `AimDistance=3000` (Warcraft units, 100..15000) enable native spell orders.
  The owner's `UnitAbilities.slk` and race ability UI tables supply the four hero research columns,
  preserving passive/unlearned gaps. The entire custom hero list is read, including fifth/later skills;
  declared button columns take precedence over list order, and learned replacements win column collisions.
  Unknown entries fit around declared buttons; extra/hidden research columns are excluded.
  Passive, empty or unsupported slots show a notice. Spell effects,
  mana, cooldowns, legal targets and native cast range remain Warcraft's responsibility.
  Native basic attacks and retaliation cannot start a chase while FPS owns the hero; other units and
  the hero's spell components retain native control, and F6 immediately restores ordinary RTS attacks.
  AimDistance only limits reticle selection; point spells require an actual ground/object surface under the reticle.
  Movement/fire/reload/weapon switching and opening the buy menu interrupt windup or channeling;
  held movement/fire does not repeatedly interrupt a newly started cast. Mouse look remains available.
  For custom map heroes, edited ability lists or unsupported dynamic order getters, set
  `Slot1Ability=AHtb` (four-character rawcode), `Slot1Order=852095` (native spell order, 852000..853999),
  and `Slot1Target=unit`; repeat for slots 2вЂ“4. Empty ability/zero order uses automatic lookup.
  Target modes are `auto`, `unit`, `point`, `immediate`, `self`; SelfCast targets the actor for allowed
  unit spells. These settings reload live and survive updates. Automatic discovery of arbitrary
  map object modifications and automatic learning of hero skills are outside this first version.

- `[Rendering] MaxFPS=144` raises the client frame cap above Warcraft's original ~64 FPS.
  Positive targets are clamped to 30вЂ“360; `0` restores the native limiter. Actual FPS depends on
  hardware, scene load and graphics-driver limits. `DrawDistance=5000` sets the controlled-view camera's
  far clip in Warcraft units; valid distances are 3000вЂ“20000. Both apply at startup/on F8.
  Updates preserve existing values. Draw distance does not reveal unexplored units or remove the
  map's atmospheric fog; N controls fog of war separately. Greater distances increase render work.

- `[Logging] Detailed=true` enables control, validated-setting, movement and renderer details.
  `IntervalMs=1000` samples movement/render summaries every second (100вЂ“60000 ms).
  `MaxFileMB=8` bounds each runtime log to 1вЂ“64 MiB; `ArchiveCount=3` keeps 0вЂ“8 previous
  sessions/segments. Startup and F8 apply all four values. Essential events/errors remain when details
  are disabled. Updates preserve custom values. See [Diagnostics](docs/DIAGNOSTICS.md).

- `[Movement]` enables Half-Life-inspired bunnyhopping and air strafing. Run with W, jump, release W,
  then alternate A + mouse left and D + mouse right. Hold Space to jump on landing and retain momentum.
  Turning in the air builds speed; straight jumps retain the run-up. `JumpBoostPercent=0` is the default,
  with an optional 0вЂ“100% bonus per moving takeoff. Low objects can be cleared above their bounds; tall solids remain blocked.
  Walking/jumping off a cliff starts a gravity-driven fall. Tune these [movement settings](docs/MOVEMENT.md);
  all apply on F8 without resetting the current jump. Default speed cap: 1000 CS units/s; configurable up to 2000.

- `[Audio] CSVolumePercent=100` controls all CS sounds independently of Warcraft audio. Range: **0вЂ“100**;
  **0** mutes CS, **100** retains the previous levels, and fractional values are accepted.
  F8 applies the new volume immediately, including sounds already playing; restarting also loads it.
  Missing/empty/malformed/non-finite values use 100; finite values outside the range are clamped.
- `[Runes] AmmoPercent=40`; `AmmoWeapons=all` or `current`.
- `[Damage] Mode=unit` is the default: current average Warcraft attack of **any** controlled
  unit, including creeps and attack bonuses, multiplied by each weapon's `HeroMultiplier`.
  `Mode=hero` remains a compatible alias; `Mode=weapon` selects fixed damage. C4 retains fixed explosion damage.
- `[Damage] AWPOneShot=false` is the test-build default: AWP uses unit attack times `HeroMultiplier=3` by default
  and native armor; `Mode=weapon` instead uses `[AWP] Damage` (115). Set `true` to enable instant enemy kills in `weapon` mode; F8 applies changes.
  Missing/empty/invalid values use `false`; `true`/`false` and `1`/`0` are accepted.
  Updates preserve existing values, so older configs with `AWPOneShot=true` need an explicit edit to disable it.
- **Q** toggles direct allied targeting in FPS and third person. The HUD shows `ALLIES ON/OFF`.
  `[Combat] TargetAllies=false` protects allied units/buildings from direct attacks and suppresses
  their target markers by default. `true`/`1` enable contacts; `false`/`0` disable them; malformed
  values use false. Startup/F8 restores this saved preference. `[Shortcuts] ToggleAllyTargeting=Q`
  is remappable; `NONE` disables the shortcut. Updates preserve custom choices, and an older custom
  action bound to Q keeps Q while this new action is added as NONE. With allied targeting disabled,
  allied units/buildings are skipped by FPS/TPS attacks so enemies behind them remain reachable.
  Enabling it makes allies direct contacts again. Terrain, gates and other hostile bodies still block;
  C4 splash and native spells retain their policies.
- `[Damage] FriendlyFirePercent=50` sets damage to owned/allied units and buildings, including
  firearms, both melee attacks and C4 (including its planter). Range: 0вЂ“100; 0 disables allied damage,
  100 applies full damage. Press F8 to reload; C4 retains the settings from when it was planted.
- `[Squad] RecruitRadius`, `MaxUnits`, `FollowDistance` and `CombatLeash` control squad behavior.
  `RearFormation=true` keeps destinations behind the player. `YieldToPlayer=true` sends followers
  left/right before they rejoin when they enter your travel corridor. `PlayerClearance=100` (60..300)
  and `YieldAheadDistance=350` (clearance..1000) are Warcraft units. `OrderIntervalMs=150` (50..1000)
  controls response frequency. All reload with F8 and survive launcher updates; native pathfinding
  still governs travel around obstacles.
- `[Loadout] Mode=melee` starts each map with a loaded USP, knife and sword; `Mode=all` grants all weapons
  with their initial ammunition. `BombCount=0` excludes C4 from the default kit; a positive count opts
  into starting bombs. Buying C4 grants ownership; F11 grants/refills C4 and F12 grants all weapons.
  Updates preserve an existing count; set it to 0 explicitly to adopt the new default on the next map.
  `MaxBombs=100` sets the starting/free-refill/rune amount (0..1000).
  `UnlimitedBombPurchases=true` allows repeated paid purchases beyond that amount; false restores
  it as the purchase cap. F8 applies this switch; refills never discard surplus purchased bombs.
  F6/F8 and changing the controlled unit never grant another starting kit.
- `[AK47]`, `[M4A1]`, `[USP]`, `[AWP]`, `[Knife]`, `[Sword]`, `[C4]` use `Price` in native Warcraft
  **gold**. Buying a firearm gives one loaded magazine, with zero reserve rounds. `AmmoPrice` buys
  `AmmoPack` reserve rounds (R loads them). A C4 weapon/ammo purchase grants one charge. Full ammo,
  duplicate non-consumable weapons and rejected purchases do not cost gold.
- `[Buy] Access=anywhere` allows buying anywhere (default); `friendly` requires a nearby allied
  building **or shop**; `shops` allows shops only. `Radius=600` is the center distance in Warcraft units.
  Stock neutral merchants, mercenary camps and shops with purchase/sell/neutral-interaction abilities
  qualify when allied or neutral-passive and visible/alive.
  Add custom shop unit rawcodes with `ShopTypes=nmer,ngme`. Both B and every purchase, including `.`,
  check the zone. The menu does not pause the world; leaving a valid zone closes it.
- F11 refills carried magazines/reserves and grants C4 up to at least `MaxBombs` for free;
  surplus purchased bombs are retained. F12 also unlocks all weapons.
  These shortcuts do not spend gold or bypass Warcraft incapacitation. Legacy `AllowFreeRefill` is ignored.
  Rune rewards still follow `[Runes]` settings, apply only to owned weapons and use `MaxBombs` for C4.
- `[Sky] Enabled=false` keeps Warcraft's native sky by default; `true` enables a private CS sky in FPS. `Default=Des` is the unknown-tileset fallback;
  map tileset keys such as `W=snow`, `A=forest`, `D=DrkG` choose a filename stem from your
  `cstrike/gfx/env`. Set `Enabled=false` to retain Warcraft's original sky. Missing caches also keep
  the native sky; run setup again to convert your installed CS sky textures. F8 applies changes live.
- `[Sky] WarcraftEnabled=true` (default) shows Warcraft's stock summer/winter sky in FPS on maps without
  a sky. Keep `Enabled=false` for this mode. Existing map skies are preserved; F6 and cinematics
  restore the map's original sky. Set `WarcraftEnabled=false` and press F8 to disable the preview.
- `[Interface] ShowSpeed=false` hides the horizontal speed counter by default. P toggles it during
  the session, including across F6 switches; startup/F8 restores the INI value (`true`/`false`, also `1`/`0`).
  The counter uses CS/GoldSrc units/s, matching `MaxBunnySpeed`; vertical jump/fall velocity is excluded.
  Updates add missing keys and preserve custom values. The HUD omits basic WASD/reload/jump/crouch hints.
- `[Interface] ShowMinimap=true` shows Warcraft's native minimap in the FPS top-right corner.
- `[Interface] ShowSpellIcons=true` shows four spell icons at the bottom right; `ShowSpellCooldowns=true`
  adds the live remaining cooldown. `SpellIconSizePercent=7` controls icon size (4..14% of window height).
  Art comes from your current map/game. `LV0` marks an unlearned spell, `PASS` a passive,
  `EMPTY` an unmapped slot, and `?` an unsupported native timer. Rejected casts do not start a fake timer.
    Icons follow the current native command button (including toggle art), skill level and replacement
    abilities. A native cooldown sweep/countdown updates after casts/resets; insufficient mana tints
    active skills blue and shows `NO MANA`, while disabled skills dim. Other selected units and the
    research submenu cannot overwrite the controlled hero's icons.
  F8 reloads these settings and shortcuts; updates preserve custom values.
  **M** toggles it for the session, including across F6 switches; startup/F8 restores the saved preference
  (`true`/`false`, also `1`/`0`). Terrain, fog of war and native markers follow the current map's visibility.
  `MinimapSizePercent=22` sets the square's side to 22% of window height (10..40, fractional values allowed).
  F8 applies size changes live; missing/invalid values use the defaults and updates preserve custom values.
  The FPS minimap is a display; use RTS for map clicks/orders. Native RTS layout and campaign bounds stay intact.
  Top hints use three compact groups and wrap before the minimap; gold appears beside health/ammunition.
- `[Interface] DisableFogOfWar=false` keeps map visibility by default (`true`/`false`, also `1`/`0`).
  Set `true` or press N in offline FPS to remove both explored fog and the unexplored black mask.
  N changes only the session; startup/F8 loads the saved preference. F6, menus and cinematics restore
  the map's captured fog/mask settings; returning to FPS reapplies the session choice. Updates preserve
  custom values. Sword and secondary-melee shortcuts are omitted from the HUD; their controls remain.
- Health bars use `[Interface] ShowAllyHealthBars=false`, `ShowEnemyHealthBars=false` and
  `HealthBarDistance=2000` (Warcraft units, 0..15000; zero hides bars). K toggles and saves both teams;
  a mixed saved state first enables both. F8 reloads file edits. Bars exclude your controlled actor,
  dead/hidden/transported units, fog-hidden targets and passive neutrals. Opaque world depth occludes
  bars; RTS keeps its native health display.
- `[Interface] FloatingTextDistance=1200` limits native world labels to the FPS camera view and distance
  in Warcraft units. Set `0` to hide world labels and press F8; RTS, harvesting and screen-space map text keep native behavior.

The buy panel follows the original CS layout: translucent black background, amber outlined rows,
red mouse selection, a hand-free weapon preview and live prices/capacities in the detail panel.
A successful purchase closes the menu; rejected purchases leave it open.
Only supported categories appear, preserving CS keys: 1 pistols, 4 rifles, 6 primary ammo,
7 secondary ammo, 8 equipment (knife, sword, C4), 0 back/cancel. The `.` key buys current-weapon ammo.
Primary/secondary ammo rows buy one pack per carried weapon in that category; every transaction checks
funds independently. Weapon prices are doubled: AK47 1250, M4A1 1550,
USP 250, AWP 2376, knife 0, sword 500 and C4 3000. Other ammo-pack prices are unchanged;
both C4 purchase paths charge 3000 gold for one bomb by default.
No round, buy-time or team-spawn restriction is imposed.

When adding a weapon, apply [warcraft-cs-weapon-menu](skills/warcraft-cs-weapon-menu/SKILL.md).
`src/economy/BuyCatalog.hpp` supplies both navigation and presentation so new weapons cannot be forgotten in
one of those lists. Setup converts private buy previews alongside the player's own weapon models.
Every planted C4 keeps its own 35-second simulation-time fuse and damage snapshot. The HUD shows
the active charge count and the nearest explosion. Switching weapons or leaving FPS keeps charges active;
Warcraft pause suspends their clocks, and changing maps discards old native handles.

Setup adds missing configuration keys during updates and preserves existing custom values.
Source checks: `tools/test-all.ps1` covers gameplay and purchase rules; `tools/test-sky-view.ps1`
additionally checks synthetic sky colors/depth and context replacement in a hidden desktop OpenGL
window, including the pixel-upload state left by Warcraft after Alt-Tab.

Native armor still applies after allied scaling. Ordinary items do not grant ammunition.
A researched Warcraft Backpack accepts equipment on creeps but cannot activate runes/tomes.
Rejected pickups and full inventories grant no ammunition. A unit without a normal attack deals zero
weapon damage in `hero` mode. C4 uses its fixed `[C4] Damage` value.

## Compatibility and limitations

**Offline single player only.** Multiplayer is not supported for now. The mod hooks the game globally,
but the complete campaign and every custom map have not been verified.
Map scripts can conflict with camera or unit control. Maps without living owned units cannot provide
an FPS character. Hitboxes approximate model bounds rather than individual bones.

The recoil calculations are adapted, but **complete CS bullet spread/accuracy and the GoldSrc client
are not implemented**. Purchases use Warcraft gold; CS rounds and team economy are not implemented.

## TODO

Planned work:

- [ ] Improve movement.
- [ ] Fix gameplay and camera bugs.
- [ ] Add drivable vehicles.
- [x] Add an installer that is easy to use.
- [ ] Test the full campaign.
- [x] Improve hitboxes and hit registration.
- [ ] Make further improvements based on playtesting and feedback.
- [ ] Add multiplayer support(possibly)
- [ ] Add Warcraft Reforge support(possibly)

## Help

Warcraft CS is a hobby project. Help is provided when time allows; replies and fixes are not guaranteed.

1. Read [Troubleshooting](TROUBLESHOOTING.md) and check the launcher's error message.
2. If the problem remains, ask in [Discussions вЂ” Q&A](https://github.com/YazgulDev/warcraft-cs/discussions/categories/q-a).
   Include the release version, Player/Developer mode, Windows version, GPU, steps to reproduce
   and the relevant log: `install.log` in your client folder for setup, or
   `Game/WarcraftCS/WarcraftCS.log` for in-game problems.
3. Remove personal information from logs before uploading them. Do not attach game files or saves.

Working installations are useful feedback too: share your Windows/GPU details and whether you tested
Reign of Chaos, The Frozen Throne, a campaign or a custom map. Use **Ideas** for feature requests.

## Source layout and validation

`src` contains the runtime and independent calculations; `tests` contains numerical checks and
optional scenes for disposable test maps; `tools` handles builds, conversion and audits;
`setup` prepares your local game files. Camera, audio, recoil, inventory and squads have separate responsibilities.
Module ownership and dependency boundaries: [docs/MODULES.md](docs/MODULES.md).
The [modular-code skill](skills/modular-code/SKILL.md) guides future source changes.

```powershell
.\tools\test-all.ps1
.\tools\test-cs-audio.ps1
python .\tools\audit_sources.py --revision HEAD
```

Verification notes: [docs/VALIDATION.md](docs/VALIDATION.md).
The CS audio check uses real XAudio2 and a silent synthetic WAV; it requires a working Windows audio device.
Branch/release rules: [CONTRIBUTING.md](CONTRIBUTING.md).
New game tuning parameters follow [configurable-game-settings](skills/configurable-game-settings/SKILL.md).
Never commit or redistribute `.local`, game content, converted caches or proprietary DLLs.

## Credits and licenses

Original source code is available under **MIT OR Apache-2.0**.
Adapted calculations and external components retain their terms: ReGameDLL_CS/ReHLDS use MIT;
MinHook/HDE is obtained separately under BSD-2-Clause.

Native Warcraft integration facts were researched with JassSpyEngine, RenderEdge and UjAPI;
their implementations are not bundled. Valve/Blizzard game materials are excluded and are not covered by the code licenses.

For the full list of references, adaptations and license terms, see [NOTICE](NOTICE),
[LICENSE-MIT](LICENSE-MIT), [LICENSE-APACHE](LICENSE-APACHE) and `licenses/`.

### Unit scaling and ammunition limits

- `[AK47]`, `[M4A1]`, `[USP]` and `[AWP] MaxReserve` set reserve rounds, excluding the magazine.
  Defaults are 90, 90, 100 and 30; range 0..100000. Purchases, F11/F12 and rune rewards all use these caps.
  F8 discards surplus reserve after a reduction, without granting ammo or changing loaded rounds.
- `[Combat] ScaleFireRateFromAttackSpeed=false` is optional (`true`/`false`, also `1`/`0`).
  When enabled, firearm interval is divided by native attack-speed multiplier (0.2..5).
  Agility, items and buffs participate; 2.0 doubles the rate. Reloads, melee and planting are unchanged.
- `[Movement] ScaleFromUnitMoveSpeed=false` is optional (`true`/`false`). When enabled, CS weapon
  speed scales by current Warcraft movement speed divided by `UnitMoveSpeedReference=300`
  (Warcraft units/s, range 1..1000). This replaces the legacy relative slow/haste ratio; it never doubles bonuses.
  With it disabled, existing slow/haste behavior remains. Root/stun and bunnyhop limits remain enforced.
- All settings reload with F8. Updates preserve existing prices, damage mode and rune percentage;
  edit old configs explicitly to adopt the new defaults (`Mode=unit`, `AmmoPercent=40`, prices above).

Spell requests now check native cooldown, mana and disabled state before dispatch and preserve active
channels when a request is unavailable. DotA 6.88f AI Sniper's Shrapnel uses three map-owned charges
with 40-second regeneration, while its native cooldown is zero. Its FPS icon displays the actual charge
count, and an empty marker blocks requests. The mod does not invent a cooldown or alter the map's timer;
unknown charged spells retain their map's native validation.

### Weapon attack effects

`[Combat] WaterBlocksAttacks=false` lets firearm and melee traces in FPS and third person cross
the water surface to reach submerged unit hitboxes. The solid bed, cliffs, walkable bridge decks,
gates and other bodies still obstruct attacks. This does not change movement or water rendering.
Set `true` and press **F8** to restore the old liquid-surface blocker; `false` disables it.
`true`/`false` and `1`/`0` are accepted. Missing/empty/malformed values default to `false`, and
updates preserve existing choices. The same setting applies to third-person target prediction and contact.

`[Combat] EnableAttackEffects=true` routes firearm and melee contacts through the controlled unit's
native attack preparation and impact. Learned passives and carried items can apply Mana Burn,
lifesteal, Bash, critical strikes and attack callbacks with Warcraft's own chances, restrictions
and orb priority. Incompatible orb effects do not become stackable; map-specific scripted effects
still depend on the map's implementation. No basic attack/chase order or extra weapon projectile
is issued. C4 explosions and destructable hits retain direct damage.

Firearms use their configured FPS range even on a melee hero. After the ray verifies the target
and terrain, only that contact's native impact range check is extended to its target distance.
The hero's ordinary RTS range and native passive restrictions remain unchanged.

`[Combat] IgnorePassableInvulnerableObjects=true` lets weapon rays continue through Locust effect
units (including DotA Broodmother webs) and invulnerable noncolliding dummy units. Physical
invulnerable units, heroes, buildings, terrain and destructable gates still intercept rays.
Set it to `false` (or `0`) and press F8 to restore legacy selection; `true`/`1` enable the filter.
Missing, empty or malformed values use `true`. Updates add the key while preserving custom values.

Set `EnableAttackEffects=false` and press F8 to disable this attack-effect path. Direct damage keeps
the unit's attack type and armor multiplier; map scripts listening to all damage can still run.
Missing/empty/invalid values enable the feature; `true`/`false` and `1`/`0` are accepted. Updates add
the key to old configurations and preserve explicit custom values, including C4 prices and
`AWPOneShot`. To adopt these defaults in an existing installation, set `[C4] Price=3000`,
`AmmoPrice=3000` and `[Damage] AWPOneShot=false` explicitly.

### Unarmed first-person unit view

Select your unit and press **F9** to see the world from its first-person camera. WASD and mouse
look remain available, with the unit's native movement speed and disabling effects. Weapons,
hands, crosshair, HUD, minimap, native panels and floating labels are hidden. Your unit cannot
attack or cast player-requested spells in this mode. By default enemies cannot select it for basic attacks.
Inventory purchases and free weapon/ammo requests are also blocked while the mode is active.
Warcraft ambience remains audible; CS footsteps are omitted.

`[UnitView] HideHoverCircles=true` also hides native hover circles on units, buildings and
destructables in F9. Set `false` and press F8 to retain them; missing/malformed values use `true`.
Both boolean words and 1/0 work. Leaving F9 restores the map's latest preselection preferences;
RTS/FPS/TPS selection and target markers keep their own behavior. Updates preserve your choice.

Press **F9 again** or **F6** to return to RTS. **F7** selects third person; **T** switches the
controlled camera to third person. The free all-weapons shortcut is now **F12**. These keys are
editable under `[Shortcuts]`, including `ToggleUnitView`. Updates migrate the old `AllWeapons=F9`
to an unused F12/F5/F4 and preserve custom bindings. If another custom action already uses F9,
`ToggleUnitView` remains `NONE`: assign an unused key yourself.

Set `[UnitView] Enabled=false` and press **F8** to disable this mode. If already active, F8 returns
you to RTS. Missing or invalid values use `true`; `true`/`false` and `1`/`0` are accepted.

`[UnitView] PreventEnemyAttacks=true` rejects hostile basic-attack acquisition, wind-up and
map-issued attack orders only during live F9. Set it to `false` and press **F8** to allow enemies
to attack again while staying in unit view. Leaving F9 also restores normal targeting. Missing,
empty or malformed values use `true`; `true`/`false` and `1`/`0` are accepted, and updates preserve
your choice. This changes attack targeting rather than invulnerability: already airborne missiles,
spells, area damage and direct map-script damage can still affect the unit.

### Automatic squad spells

**H** recruits only your nearby units to follow and fight. They can now cast supported native
combat spells, heal injured allies and use buffs/summons when enemies are nearby. Warcraft
validates each order and owns its mana cost, cooldown and target restrictions; active casts
are preserved until completion. **O** selects following without this module's combat casts,
and **J** releases the squad. These commands also work in unarmed unit view.

Edit `[Squad]` in `Game/WarcraftCS/WarcraftCS.ini`, then press **F8**:

| Key | Default | Meaning and accepted range |
| --- | --- | --- |
| `AutomaticSpells` | `true` | Enable this module's squad spell AI; `true`/`false`, `1`/`0`. |
| `SpellSearchRadius` | `700` | Look for enemies and injured allies within 50..2000 world units. |
| `AreaSpellRadius` | `250` | Require an enemy within 16..1000 units for untargeted area spells. |
| `HealBelowPercent` | `85` | Heal allies below this health percentage, 1..100. |
| `BuffRefreshSeconds` | `20` | Minimum retry interval for buffs/summons and cooldown-free area spells, 1..600 seconds. |
| `SpellIntervalMs` | `1000` | Minimum interval between spell decisions per follower, 100..10000 milliseconds. |

Numeric values are clamped to their ranges; missing/nonfinite/malformed values use defaults.
Updates preserve existing settings. Set `AutomaticSpells=false` to cancel this module's current
casts and stop new ones while retaining ordinary squad attacks. Existing Warcraft/map autocast
switches are preserved and may still cast independently.

The initial catalog covers recognized stock orders and map abilities based on those orders,
including Storm Bolt, Chain Lightning, Slow, Priest Heal, Healing Wave, Bloodlust and summons.
Arbitrary custom spell orders, item abilities, passives and toggle modes are not automated.
Native target validation still applies; not every customized map ability is supported.
