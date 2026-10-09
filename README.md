# Warcraft CS — 0.6.1

Counter-Strike 1.6 inside Warcraft III: control your own hero or unit in first person,
shoot, and play regular maps, campaigns and compatible custom maps.
The world, enemies, quests and health remain governed by Warcraft.

An unofficial single-player fan project, unaffiliated with Blizzard Entertainment or Valve.
**This Git repository contains source code. The standard launcher and release ZIPs contain
project source without prebuilt DLLs. A separate DLL-included EXE supplies ready-to-use project modules.
Neither variant includes game files, models, sounds, maps or saves.
You need your own installed copies of both games.**
This is an early prototype with known limitations.

**[Download 0.6.1](https://github.com/YazgulDev/warcraft-cs/releases/tag/v0.6.1)** ·
[Release Notes](docs/RELEASE-0.6.1.md) · [Installation](INSTALL.md) · [Requirements](REQUIREMENTS.md)
· [Troubleshooting](TROUBLESHOOTING.md) · [Discussions](https://github.com/YazgulDev/warcraft-cs/discussions)

0.6.1 adds configurable CS sound volume with live F8 reload and reusable rules for game settings
and complete Release Notes. The project also includes the CS-style gold buy menu, configurable
starting equipment, multiple C4, RoC/TFT selection and `play-roc.bat`, fullscreen/Alt-Tab recovery,
native/optional CS skies, bridge movement and corrected FPS rendering. Earlier changes are in the [changelog](CHANGELOG.md).

## Features

- Windows EXE launcher with game-folder selection, consent before dependency setup, live logs and Play.
- Remembered Reign of Chaos / The Frozen Throne selection; `play-roc.bat` starts RoC from a source installation.
- F6 switches between FPS and RTS; fullscreen launch, relative mouse look and hidden player-unit model.
- Consistent fullscreen DPI scaling and texture/context recovery after Alt-Tab; native hero portraits keep their layout.
- WASD, jumping, crouching, acceleration, friction and weapon movement speeds inspired by CS.
- Classic AK/M4 and USP/AWP punch calculations, recoil recovery and burst behavior.
- Primary and secondary melee attacks, locally imported CS sounds and a blood-free hit indicator.
- CS-only volume in INI, including shots, reloads, footsteps and C4; F8 applies it to ongoing and future sounds.
- Damage to units, buildings and gates; configurable allied damage (50% by default). C4 deals 2500 base area damage.
- Warcraft stuns, roots, slows and attack restrictions also limit FPS actions.
- E picks up items/runes through the real inventory; a successful rune restores 20% ammunition by default.
- H recruits your own units to follow and fight, O makes them follow without attacking, J releases them.
- INI settings for ammunition recovery, damage and squad behavior, reloaded with F8.
- Mouse-wheel weapon cycling through all seven slots, with high-resolution wheel support.
- B opens a CS-style buy menu using Warcraft gold; configurable prices, starting kits and shop/building access.
- Multiple planted C4 charges with independent fuses; F7 refills carried equipment and F9 grants every weapon for free.
- Warcraft sky by default in FPS, optional privately converted CS skyboxes, and visible native walkable bridge decks.
- Corrected near-ground clipping and camera/distance filtering for floating world labels.

## Requirements

- Windows x64 and **Warcraft III 1.26a x86**, with `Game.dll` version `1.26.0.6401`.
- Installed Counter-Strike 1.6 with loose files in `cstrike/models` and `cstrike/sound`.
- Standard `WarcraftCSLauncher.exe` builds locally and needs C++ Build Tools / Windows SDK.
  The separate `WarcraftCSLauncher_DLL_Included.exe` offers Player setup without those tools.
  Both may prepare Python/NumPy after agreement to convert your locally owned CS assets.
- For manual source setup: Git, Python 3.10–3.14, Visual Studio 2022 or Build Tools with
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
| `WarcraftCSLauncher_DLL_Included.exe` | Separate EXE with ready mod DLLs; **Install — Player** avoids Build Tools / SDK. **Install — Developer** builds locally. |

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
4. Click **Install** in the standard launcher, or choose **Install — Player / Developer** in the DLL-included launcher. Install uses the version embedded in that EXE.
5. Choose **The Frozen Throne** or **Reign of Chaos** under **Game to launch**, then click **Play**, select your own living unit and press F6. The choice is remembered after a successful launch.

Both editions use the same private Warcraft III 1.26a installation. RoC selects the original campaign menus;
it does not turn two-player custom maps into solo missions. Launch through the launcher or `tools/launch.ps1`
to keep fullscreen DPI scaling consistent after Alt-Tab. For source users, double-click `play-roc.bat`
or run `play.cmd -Edition ReignOfChaos`; `play.cmd` alone starts Frozen Throne.
`-Windowed` and `-Map` remain available for either edition.

The launcher checks new stable GitHub releases automatically at every startup; **Check for updates**
retries the check. The separate **Update** button fetches the latest stable project release from GitHub.
A window shows its version and notes: **Update** confirms installation; **Not now** postpones it.
The source-only launcher keeps source-only updates and builds locally, including when updating a previous
Player installation. The DLL-included launcher retains its variant and saved Player/Developer mode.
The dialog explains download requirements and consent.
Warcraft must be closed. Player installs verified bundled DLLs; Developer rebuilds them locally. Both
modes convert owned assets and update the launcher if needed. Saves, INI settings and private sword selections are
retained. Each session requires the download agreement. Offline Play remains available.

No download or installation starts before agreement. Microsoft tools may need administrator approval,
several GB of space and a Windows restart for local compilation. Only the separately named EXE includes prebuilt modules.
The launcher itself is unsigned; dependency installers have verified vendor signatures.
Detailed folder examples, first launch, updates and removal: [INSTALL.md](INSTALL.md).
Developer build instructions: [Client launcher](docs/CLIENT-LAUNCHER.md).

This test branch adds **Read logs** to both launchers. It opens game/session archives, installation,
launcher and replacement journals directly, with refresh, copy and full-file export. See
[Diagnostics](docs/DIAGNOSTICS.md) for the new logging controls; these changes are not yet published.

If the old 0.5.0 Player updater reports that this release requires a source build, download the new
DLL-included EXE directly and select your existing installation folder. No compiler download is silently enabled.

### Source setup

Run in PowerShell, replacing the game paths with your own:

```powershell
git clone https://github.com/YazgulDev/warcraft-cs.git
cd warcraft-cs
git switch release/0.6.1
.\setup.cmd -WarcraftDirectory "E:\Warcraft III" -CounterStrikeDirectory "C:\SteamGames\steamapps\common\Half-Life\cstrike"
.\play.cmd
```

Setup downloads a pinned MinHook dependency separately, installs NumPy in a local environment,
reads your CS models/sounds/sky textures, uses an optional privately supplied sword or generates the original sword,
and builds the mod in a private Warcraft copy.
Your original installations remain unchanged. Additional installation and removal details are in
[INSTALL.md](INSTALL.md).

Choose **Single Player → Custom Game** or **Campaign**, select your own living unit and press F6.
Individual maps do not need editing. Enable FPS again after changing maps.

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
| F6 / F10 | Toggle FPS ↔ RTS / pause menu and return to RTS |
| WASD / mouse | Move / look and aim |
| Shift / Ctrl / Space | Walk / crouch / jump |
| Left mouse button | Shoot or primary melee attack |
| Right mouse button | Knife/sword: strong attack; AWP: cycle two zoom levels and normal view |
| R / F7 | Reload / refill all carried ammunition for free |
| F9 | Get all weapons and full ammunition for free |
| B | Open/close the buy menu; number keys or mouse select categories and purchases |
| . | Buy one ammo pack for the weapon currently held |
| Esc / 0 in buy menu | Close / back to previous menu (0 closes the main menu) |
| 1 / 2 / 3 / 4 / 5 / 6 / 7 | AK47 / M4A1 / USP / AWP / knife / C4 / original greatsword |
| Mouse wheel up / down | Previous / next owned weapon |
| Hold left mouse button with C4 | Plant for 3 seconds while standing on the ground; multiple charges are allowed |
| E | Pick up the nearest item/rune if the unit has an available inventory |
| H / O / J | Your units: follow and fight / follow without attacking / release squad |
| F8 | Reload settings |

## Configuration

EXE installation: `<installation folder>/Game/WarcraftCS/WarcraftCS.ini`.
Default source installation: `.local/warcraft-cs/WarcraftCS/WarcraftCS.ini`.
Edit it and press F8 in FPS.

- `[Logging] Detailed=true` enables control, validated-setting, movement and renderer details.
  `IntervalMs=1000` samples movement/render summaries every second (100–60000 ms).
  `MaxFileMB=8` bounds each runtime log to 1–64 MiB; `ArchiveCount=3` keeps 0–8 previous
  sessions/segments. Startup and F8 apply all four values. Essential events/errors remain when details
  are disabled. Updates preserve custom values. See [Diagnostics](docs/DIAGNOSTICS.md).

- `[Audio] CSVolumePercent=100` controls all CS sounds independently of Warcraft audio. Range: **0–100**;
  **0** mutes CS, **100** retains the previous levels, and fractional values are accepted.
  F8 applies the new volume immediately, including sounds already playing; restarting also loads it.
  Missing/empty/malformed/non-finite values use 100; finite values outside the range are clamped.
- `[Runes] AmmoPercent=20`; `AmmoWeapons=all` or `current`.
- `[Damage] Mode=weapon` uses fixed weapon damage. `Mode=hero` uses the current average Warcraft
  attack of **any** controlled unit, including creeps, multiplied by each weapon's `HeroMultiplier`.
- `[Damage] AWPOneShot=true` finishes enemies with one AWP hit only in `weapon` mode.
- `[Damage] FriendlyFirePercent=50` sets damage to owned/allied units and buildings, including
  firearms, both melee attacks and C4 (including its planter). Range: 0–100; 0 disables allied damage,
  100 applies full damage. Press F8 to reload; C4 retains the settings from when it was planted.
- `[Squad] RecruitRadius`, `MaxUnits`, `FollowDistance` and `CombatLeash` control squad behavior.
- `[Loadout] Mode=melee` starts each map with a loaded USP, knife, sword and C4; `Mode=all` grants all weapons
  with their initial ammunition. `BombCount=20` sets starting charges; `MaxBombs=100` caps carried
  charges (0–1000). F6/F8 and changing the controlled unit never grant another starting kit.
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
- F7 always refills carried magazines/reserves and C4 to `MaxBombs` for free; F9 also unlocks all weapons.
  These shortcuts do not spend gold or bypass Warcraft incapacitation. Legacy `AllowFreeRefill` is ignored.
  Rune rewards still follow `[Runes]` settings, apply only to owned weapons and use `MaxBombs` for C4.
- `[Sky] Enabled=false` keeps Warcraft's native sky by default; `true` enables a private CS sky in FPS. `Default=Des` is the unknown-tileset fallback;
  map tileset keys such as `W=snow`, `A=forest`, `D=DrkG` choose a filename stem from your
  `cstrike/gfx/env`. Set `Enabled=false` to retain Warcraft's original sky. Missing caches also keep
  the native sky; run setup again to convert your installed CS sky textures. F8 applies changes live.
- `[Sky] WarcraftEnabled=true` (default) shows Warcraft's stock summer/winter sky in FPS on maps without
  a sky. Keep `Enabled=false` for this mode. Existing map skies are preserved; F6 and cinematics
  restore the map's original sky. Set `WarcraftEnabled=false` and press F8 to disable the preview.
- `[Interface] FloatingTextDistance=1200` limits native world labels to the FPS camera view and distance
  in Warcraft units. Set `0` to hide world labels and press F8; RTS, harvesting and screen-space map text keep native behavior.

The buy panel follows the original CS layout: translucent black background, amber outlined rows,
red mouse selection, a hand-free weapon preview and live prices/capacities in the detail panel.
A successful purchase closes the menu; rejected purchases leave it open.
Only supported categories appear, preserving CS keys: 1 pistols, 4 rifles, 6 primary ammo,
7 secondary ammo, 8 equipment (knife, sword, C4), 0 back/cancel. The `.` key buys current-weapon ammo.
Primary/secondary ammo rows buy one pack per carried weapon in that category; every transaction checks
funds independently. Defaults are one quarter of the previous weapon prices: AK47 625, M4A1 775,
USP 125, AWP 1188 (rounded from 1187.5 gold), knife 0, sword 250 and C4 50. Ammo-pack prices are unchanged.
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
2. If the problem remains, ask in [Discussions — Q&A](https://github.com/YazgulDev/warcraft-cs/discussions/categories/q-a).
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
