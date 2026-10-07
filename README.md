# Warcraft CS — 0.5.0

Counter-Strike 1.6 inside Warcraft III: control your own hero or unit in first person,
shoot, and play regular maps, campaigns and compatible custom maps.
The world, enemies, quests and health remain governed by Warcraft.

An unofficial single-player fan project, unaffiliated with Blizzard Entertainment or Valve.
**This Git repository contains source code. The release launcher contains project-owned code
source and ready-to-use native mod modules; it does not include game files, models, sounds, maps or saves.
You need your own installed copies of both games.**
This is an early prototype with known limitations.

## Features

- Windows EXE launcher with game-folder selection, consent before dependency setup, live logs and Play.
- F6 switches between FPS and RTS; fullscreen launch, relative mouse look and hidden player-unit model.
- WASD, jumping, crouching, acceleration, friction and weapon movement speeds inspired by CS.
- Classic AK/M4 and USP/AWP punch calculations, recoil recovery and burst behavior.
- Primary and secondary melee attacks, locally imported CS sounds and a blood-free hit indicator.
- Damage to units, buildings and gates; allies receive 50% damage. C4 deals 2500 base area damage.
- Warcraft stuns, roots, slows and attack restrictions also limit FPS actions.
- E picks up items/runes through the real inventory; a successful rune restores 20% ammunition by default.
- H recruits your own units to follow and fight, O makes them follow without attacking, J releases them.
- INI settings for ammunition recovery, damage and squad behavior, reloaded with F8.
- Mouse-wheel weapon cycling through all seven slots, with high-resolution wheel support.

## Requirements

- Windows x64 and **Warcraft III 1.26a x86**, with `Game.dll` version `1.26.0.6401`.
- Installed Counter-Strike 1.6 with loose files in `cstrike/models` and `cstrike/sound`.
- Player installation needs no C++ Build Tools or Windows SDK. Python/NumPy may be prepared
  after agreement to convert your locally owned CS assets. Developer mode builds the DLLs from source.
- For manual source setup: Git, Python 3.10–3.14, Visual Studio 2022 or Build Tools with
  **Desktop development with C++**, MSVC x86 and Windows SDK.
- OpenGL support and enough disk space for a separate copy of Warcraft.

Reforged and other Warcraft patches are not supported.
The full game-file, system, build-dependency and updater requirements are in [REQUIREMENTS.md](REQUIREMENTS.md).

## Setup and play

### Windows client launcher

Download **[WarcraftCSLauncher.exe from Releases](https://github.com/YazgulDev/warcraft-cs/releases/latest)**.
Run the EXE; clients do not need Git or a separate source checkout.

Choose these folders using **Browse...** (the examples are illustrative):

| Launcher field | Which folder to choose | Example |
| --- | --- | --- |
| Warcraft III 1.26a folder | The game root containing `war3.exe`, `Game.dll`, `Mss32.dll` and the Warcraft MPQ archives. Do not choose `Maps` or `save`. | `E:\Warcraft III` |
| Counter-Strike 1.6 folder | `cstrike`, containing `models/v_knife.mdl` and `sound`; the launcher also accepts its `Half-Life` parent. Do not choose the Steam library root or `models` alone. | `C:\SteamGames\steamapps\common\Half-Life\cstrike` |
| Install Warcraft CS here | A dedicated writable folder outside both game installations. This receives a separate Warcraft copy. | Default: `%LOCALAPPDATA%\WarcraftCS`, or `D:\WarcraftCS` |

1. Select your installed Warcraft III 1.26a folder and CS 1.6 folder (`cstrike` or its Half-Life parent).
2. Select a separate installation folder for Warcraft CS.
3. Read the download details and agree to downloading/installing the dependencies and their terms.
4. Click **Install / Update — Player** for bundled DLLs without Build Tools/SDK. Developers can choose **Install / Update — Developer** to prepare missing C++ tools/SDK and compile locally.
5. Click **Play**, select your own living unit and press F6.

The launcher checks new stable GitHub releases automatically at every startup; **Check for updates**
retries the check. A window shows the new version and release notes: **Update** confirms installation
in your existing mode; **Not now** postpones it. The dialog explains download requirements and consent.
Warcraft must be closed. Player installs verified bundled DLLs; Developer rebuilds them locally. Both
modes convert owned assets and update the launcher if needed. Saves, INI settings and private sword selections are
retained. Each session requires the download agreement. Offline Play remains available.

No download or installation starts before agreement. Microsoft tools may need administrator approval,
several GB of space and a Windows restart in Developer mode. The launcher includes mod modules and source, never game files.
The launcher itself is unsigned; dependency installers have verified vendor signatures.
Detailed folder examples, first launch, updates and removal: [INSTALL.md](INSTALL.md).
Developer build instructions: [Client launcher](docs/CLIENT-LAUNCHER.md).

### Source setup

Run in PowerShell, replacing the game paths with your own:

```powershell
git clone https://github.com/YazgulDev/warcraft-cs.git
cd warcraft-cs
git switch release/0.5.0
.\setup.cmd -WarcraftDirectory "E:\Warcraft III" -CounterStrikeDirectory "C:\SteamGames\steamapps\common\Half-Life\cstrike"
.\play.cmd
```

Setup downloads a pinned MinHook dependency separately, installs NumPy in a local environment,
reads your CS models/sounds, uses an optional privately supplied sword or generates the original sword,
and builds the mod in a private Warcraft copy.
Your original installations remain unchanged. Additional installation and removal details are in
[INSTALL.md](INSTALL.md).

Choose **Single Player → Custom Game** or **Campaign**, select your own living unit and press F6.
Individual maps do not need editing. Enable FPS again after changing maps.

Optional launches:

```powershell
.\play.cmd -Windowed
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
| R / F7 | Reload / refill all ammunition, including C4 |
| 1 / 2 / 3 / 4 / 5 / 6 / 7 | AK47 / M4A1 / USP / AWP / knife / C4 / original greatsword |
| Mouse wheel up / down | Previous / next weapon; wraps through all seven slots |
| Hold left mouse button with C4 | Plant for 3 seconds while standing on the ground |
| E | Pick up the nearest item/rune if the unit has an available inventory |
| H / O / J | Your units: follow and fight / follow without attacking / release squad |
| F8 | Reload settings |

## Configuration

EXE installation: `<installation folder>/Game/WarcraftCS/WarcraftCS.ini`.
Default source installation: `.local/warcraft-cs/WarcraftCS/WarcraftCS.ini`.
Edit it and press F8 in FPS.

- `[Runes] AmmoPercent=20`; `AmmoWeapons=all` or `current`.
- `[Damage] Mode=weapon` uses fixed weapon damage. `Mode=hero` uses the current average Warcraft
  attack of **any** controlled unit, including creeps, multiplied by each weapon's `HeroMultiplier`.
- `[Damage] AWPOneShot=true` finishes enemies with one AWP hit only in `weapon` mode.
- `[Squad] RecruitRadius`, `MaxUnits`, `FollowDistance` and `CombatLeash` control squad behavior.

Native armor still applies; friendly damage uses a 0.5 coefficient. Ordinary items do not grant ammunition.
A researched Warcraft Backpack accepts equipment on creeps but cannot activate runes/tomes.
Rejected pickups and full inventories grant no ammunition. A unit without a normal attack deals zero
weapon damage in `hero` mode. C4 uses its fixed `[C4] Damage` value.

## Compatibility and limitations

**Offline single player only.** Multiplayer is not supported for now. The mod hooks the game globally,
but the complete campaign and every custom map have not been verified.
Map scripts can conflict with camera or unit control. Maps without living owned units cannot provide
an FPS character. Hitboxes approximate model bounds rather than individual bones.

The recoil calculations are adapted, but **complete CS bullet spread/accuracy and the GoldSrc client
are not implemented**. CS economy, rounds and weapon purchasing are also absent.

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

## Source layout and validation

`src` contains the runtime and independent calculations; `tests` contains numerical checks and
optional scenes for disposable test maps; `tools` handles builds, conversion and audits;
`setup` prepares your local game files. Camera, audio, recoil, inventory and squads have separate responsibilities.

```powershell
.\tools\test-all.ps1
python .\tools\audit_sources.py --revision HEAD
```

Verification notes: [docs/VALIDATION.md](docs/VALIDATION.md).
Branch/release rules: [CONTRIBUTING.md](CONTRIBUTING.md).
Never commit or redistribute `.local`, game content, converted caches or proprietary DLLs.

## Credits and licenses

Original source code is available under **MIT OR Apache-2.0**.
Adapted calculations and external components retain their terms: ReGameDLL_CS/ReHLDS use MIT;
MinHook/HDE is obtained separately under BSD-2-Clause.

Native Warcraft integration facts were researched with JassSpyEngine, RenderEdge and UjAPI;
their implementations are not bundled. Valve/Blizzard game materials are excluded and are not covered by the code licenses.

For the full list of references, adaptations and license terms, see [NOTICE](NOTICE),
[LICENSE-MIT](LICENSE-MIT), [LICENSE-APACHE](LICENSE-APACHE) and `licenses/`.
