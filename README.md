# Warcraft CS by Yazgul — 0.2.0

Counter-Strike 1.6 inside Warcraft III: control your own hero or unit in first person,
shoot, and play regular maps, campaigns and compatible custom maps.
The world, enemies, quests and health remain governed by Warcraft.

An unofficial single-player fan project, unaffiliated with Blizzard Entertainment or Valve.
**This repository contains source code. Game files, models, sounds, maps, saves and compiled
binaries are not distributed. You need your own installed copies of both games.**
Created by Yazgul with assistance from Codex (GPT-6). This is an early prototype with known limitations.

## Features

- F6 switches between FPS and RTS; fullscreen launch, relative mouse look and hidden player-unit model.
- WASD, jumping, crouching, acceleration, friction and weapon movement speeds inspired by CS.
- Unsilenced AK47, M4A1 and USP, AWP with two zoom levels, knife, C4 and a greatsword.
- Classic AK/M4 and USP/AWP punch calculations, recoil recovery and burst behavior.
- Primary and secondary melee attacks, locally imported CS sounds and a blood-free hit indicator.
- Damage to units, buildings and gates; allies receive 50% damage. C4 deals 2500 base area damage.
- Warcraft stuns, roots, slows and attack restrictions also limit FPS actions.
- E picks up items/runes through the real inventory; a successful rune restores 20% ammunition by default.
- H recruits your own units to follow and fight, O makes them follow without attacking, J releases them.
- INI settings for ammunition recovery, damage and squad behavior, reloaded with F8.

## Requirements

- Windows x64 and **Warcraft III 1.26a x86**, with `Game.dll` version `1.26.0.6401`.
- Installed Counter-Strike 1.6 with loose files in `cstrike/models` and `cstrike/sound`.
- Git, Python 3.10+, Visual Studio 2022 or Build Tools with **Desktop development with C++**,
  MSVC x86 and Windows SDK.
- OpenGL support and enough disk space for a separate copy of Warcraft.

Reforged and other Warcraft patches are not supported.

## Setup and play

Run in PowerShell, replacing the game paths with your own:

```powershell
git clone https://github.com/YazgulDev/warcraft-cs.git
cd warcraft-cs
git switch release/0.2.0
.\setup.cmd -WarcraftDirectory "E:\Warcraft III" -CounterStrikeDirectory "C:\SteamGames\steamapps\common\Half-Life\cstrike"
.\play.cmd
```

Setup downloads a pinned MinHook dependency separately, installs NumPy in a local environment,
reads your CS models/sounds, generates the original sword and builds the mod in a private Warcraft copy.
Your original installations remain unchanged. Additional installation and removal details are in
[INSTALL.md](INSTALL.md), currently in Russian.

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
| Hold left mouse button with C4 | Plant for 3 seconds while standing on the ground |
| E | Pick up the nearest item/rune if the unit has an available inventory |
| H / O / J | Your units: follow and fight / follow without attacking / release squad |
| F8 | Reload settings |

## Configuration

Default file: `.local/warcraft-cs/WarcraftCS/WarcraftCS.ini`.
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

**Offline single player only.** Multiplayer is not supported. The mod hooks the game globally,
but the complete campaign and every custom map have not been verified.
Map scripts can conflict with camera or unit control. Maps without living owned units cannot provide
an FPS character. Hitboxes approximate model bounds rather than individual bones.

The recoil calculations are adapted, but **complete CS bullet spread/accuracy and the GoldSrc client
are not implemented**. CS economy, rounds and weapon purchasing are also absent.
See [TROUBLESHOOTING.md](TROUBLESHOOTING.md), currently in Russian.

## TODO

Planned work:

- [ ] Improve movement.
- [ ] Fix gameplay and camera bugs.
- [ ] Add drivable vehicles.
- [ ] Add pilotable airplanes.
- [ ] Add an installer that is easy to use.
- [ ] Test the full campaign.
- [ ] Expose more gameplay values in configuration.
- [ ] Improve hitboxes and hit registration.
- [ ] Make further improvements based on playtesting and feedback.

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

Original Yazgul source code is available under **MIT OR Apache-2.0**.
Adapted calculations and external components retain their terms: ReGameDLL_CS/ReHLDS use MIT;
MinHook/HDE is obtained separately under BSD-2-Clause.

Native Warcraft integration facts were researched with JassSpyEngine, RenderEdge and UjAPI;
their implementations are not bundled. Valve/Blizzard game materials and downloaded sword models
are excluded and are not covered by the code licenses.

For the full list of references, adaptations and license terms, see [NOTICE](NOTICE),
[LICENSE-MIT](LICENSE-MIT), [LICENSE-APACHE](LICENSE-APACHE) and `licenses/`.
The source-only release layout and local preparation of owned game files were inspired by
[World of Skatecraft](https://github.com/Kimmo3223/world-of-skatecraft); its code and text were not copied.
