# Download and upgrade

- [**WarcraftCSLauncher_DLL_Included.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.20.0/WarcraftCSLauncher_DLL_Included.exe) — Player launcher with bundled modules and compatibility components. Runs without local C++ compilation.
- [**WarcraftCSLauncher.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.20.0/WarcraftCSLauncher.exe) — Developer launcher for a source installation with local compilation.
- [**WarcraftCS-0.20.0-dist.zip**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.20.0/WarcraftCS-0.20.0-dist.zip) — Developer launcher, sources, update manifest, requirements and checksum.
- [**WarcraftCS-Surf-Flow.zip**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.20.0/WarcraftCS-Surf-Flow.zip) — Connected surf course with installation and play instructions.
- [Requirements](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.20.0/REQUIREMENTS.md) · [Sources](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.20.0/WarcraftCS-sources.zip) · [Update manifest](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.20.0/WarcraftCS-update.json) · [Player SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.20.0/WarcraftCSLauncher_DLL_Included.exe.sha256) · [Developer SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.20.0/WarcraftCSLauncher.exe.sha256) · [Direct Surf Flow map](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.20.0/WarcraftCS-Surf-Flow.w3m).

Save and close Warcraft, then use **Update from GitHub** or **Install as Player** in the new launcher. Select your existing dedicated mod installation and restart Warcraft. Setup preserves saves, custom settings and renderer preferences. Reinstall or update to convert the new models and sounds from your own CS 1.6 and Half-Life installations. The source and distribution ZIPs contain source code. The Player EXE bundles the compiled project modules and compatibility components. Game assets are supplied by your own installations.

For Surf Flow, copy the `.w3m` file to `<mod installation>/Game/Maps/Download`. In Warcraft, open **Single Player → Custom Game → Warcraft CS - Surf Flow**. Press **F6** for FPS and **P** for the speed display. [Surf Skyline and Surf Practice](https://github.com/YazgulDev/warcraft-cs/releases/tag/v0.19.0) are available with the previous release.

## Warcraft CS 0.20.0

This release covers the changes since [v0.19.0](https://github.com/YazgulDev/warcraft-cs/compare/v0.19.0...v0.20.0).

### Surf Flow and movement

- Surf Flow connects 40 banked sections and four broad decks through three legs. The course has two ordered checkpoints, a timer, fall recovery, practice stages and a session best. Turn on the connecting decks to complete the route without mandatory gaps or checkpoint teleports. The builders share the existing timer and checkpoint script with Skyline. The map is generated from original Python, JASS, MDX and texture definitions.
- `[Surfing] AccelerationMultiplier=2` doubles the air-input acceleration and wish-direction speed allowance after confirmed surf contact. The range is 0.1–4, and 1 restores the 0.19.0 response. Startup and F8 apply the setting. Free-air steering, the existing Bunnyhop cap and no-input gravity retain their own tuning.
- Nearby native pathing corrections preserve airborne height and velocity when the destination ground clears the player's feet. The allowance is four 32-unit pathing cells plus the budgeted movement step. Confirmed waygates and distant or buried relocations still rebase to the destination floor. Diagnostics distinguish preserved flight from a reset.
- High flight can cross a ground-only pathing cell when the native flight mask allows it and the body clears the terrain and solid geometry. Warcraft's path-mask refresh defers automatic Stop until landing. Basic-attack and pursuit guards remain active.
- Pathing-enabled rock doodads supply collision from their transformed models. A rock blocks walking through its body and allows a jump that clears its geometry. Ordinary stepping, narrow passages and Bunnyhop use the existing movement strategies.
- The original weapon-jump test arena now generates shadow and pathing grids matching its terrain dimensions, fixing the invalid shadow map dialog in that fixture.

### Blast and Tau jumps

RPG, HE, planted C4, Half-Life hand grenades and Satchel charges impart outward velocity through the existing movement collision. Aim a rocket at nearby ground to jump. Aim Tau down and fire, or charge and release secondary fire for a stronger jump. Only accepted shots impart recoil. Nuclear blasts are excluded. Terrain and solid cover block blast impulses, and unit status and collision still constrain movement.

| Setting | Default | Range and units |
| --- | --- | --- |
| BlastJump.Enabled | true | Enables mod-weapon blast propulsion |
| BlastJump.Speed | 650 | 0–3000 GoldSrc units/s at the blast center, fading to zero at its edge |
| BlastJump.MaximumUpSpeed | 1500 | 100–3000 GoldSrc units/s upward cap for applied impulses |
| RPG.SelfDamagePercent | 5 | 0–100% of the distance-attenuated damage to the shooter, independent of friendly fire |
| TauCannon.JumpRecoil | true | Enables recoil impulses independently of BlastJump.Enabled |
| TauCannon.PrimaryJumpSpeed | 120 | 0–3000 GoldSrc units/s |
| TauCannon.ChargedJumpSpeed | 800 | 0–3000 GoldSrc units/s at full charge |

One GoldSrc speed unit equals 1.5 Warcraft world units/s. RPG self damage uses the player's physical height and native defenses. Allied damage keeps its friendly-fire policy. F8 applies tuning to future launches, plants and accepted shots. Live explosives retain their snapshots. C4 damage iteration handles synchronous map-reset callbacks.

### Weapons, grenades and XP HUD

- P228 and Five-SeveN are available under **Buy → Pistols**, rows 4 and 5. Setup converts private viewmodels, previews and audio. Both use the regular pistol purchase, reload, animation, flash and casing paths. Defaults are 600 and 750 shop currency, 13/52 and 20/100 loaded/reserve rounds, 40 and 30 fixed damage, and native attack multipliers of 1.15 and 0.95. Ammo packs cost 40 and 50 for 13 and 50 rounds respectively.
- The Half-Life hand grenade and Satchel are under **Buy → Half-Life**, rows 5 and 6. Each requires its own complete private model and sound resources and can be enabled independently. The hand grenade holds a visible pin-pull pose with LMB held. After release and the pin pull, it spawns at the start of the throw and plays the full clip. Satchel throws on LMB press, settles on the first landing without bouncing and detonates on RMB. Both can propel their owner while leaving their health intact.
- Each purchase adds one grenade. CS HE, Flashbang and Smoke default to three carried charges each. Full stacks reject further purchases before debiting either wallet. The Half-Life hand grenade costs 450 and Satchel costs 750, with a capacity of three apiece.

| Explosive | Peak damage | Radius in Warcraft units | Launch speed in Warcraft units/s | Fuse |
| --- | ---: | ---: | ---: | --- |
| CS HE | 1800 | 600 | Shared CS trajectory | Shared CS fuse |
| Half-Life hand grenade | 1250 | 700 | 950 | 2.5 seconds from release |
| Satchel Charge | 1200 | 650 | 420 | Remote |

Damage falls linearly with three-dimensional distance to zero at the edge. Solid cover blocks the blast, and Warcraft still applies native defenses. Capacities accept 1–100, damage 0–1000000, blast radii 32–2000, guest throw speeds 50–2000 and the timed guest fuse 0.2–10 seconds. F8 affects future throws and purchases. Upgrades migrate former stock values while preserving custom values and current version markers. The cap fix reads the map's actual hero level limit. The HUD shows XP progress at level 9 on a standard map and shows `MAX` at level 10.

Pistol prices accept 0–1000000 shop currency, packs and magazines 1–1000 rounds, reserves 0–100000 rounds and native damage multipliers 0–1000. `FireInterval` accepts 0.025–10 seconds, `ReloadSeconds` 0.1–30 seconds and `Range` 100–50000 Warcraft units. `MoveSpeed` accepts 50–500 GoldSrc units/s, `ModelScale` 0.25–2 and `Kick` 0–10 degrees. The [annotated INI](https://github.com/YazgulDev/warcraft-cs/blob/v0.20.0/config/WarcraftCS.ini) lists defaults and version markers. Startup and F8 apply these settings.

### Launcher and configuration

- **Edit INI** opens the installed `Game/WarcraftCS/WarcraftCS.ini`. Saving retains the file encoding, keeps the previous file as `WarcraftCS.ini.bak` and detects outside edits before replacing the file. F8 reloads supported settings in FPS.
- The INI has 16 numbered English categories, including controls, movement, display, economy, weapon groups and diagnostics. Installation updates group existing sections and preserve saved comments, custom keys and custom sections. Windows PowerShell 5.1 handles older local ANSI files.
- Player setup reads the downloaded launcher's embedded data through .NET Framework reflection-only loading, fixing the install failure caused by Windows' Internet-zone marker. Source revision, host and payload hash checks still apply.

### Validation and compatibility

Owned offline Warcraft III 1.26a runs verified Surf Flow through both checkpoints with zero falls on OpenGL and Direct3D, plus blast and Tau propulsion and the movement corrections in disposable scenarios. OpenGL tests verified stacked CS grenade purchases, native HP falloff for HE and the Half-Life grenade, visible pin hold and throw recovery, immediate Satchel throws and remote detonation, and the level 9-to-10 HUD transition. The native lab files and saves were restored after testing. The release source includes the portable regression fixtures and map builders. The bundled Player runtime excludes fixture commands.

Supported host is **Warcraft III 1.26a x86 (Game.dll 1.26.0.6401)** for offline single player. **1.27a (1.27.0.52240)** remains experimental and received no new native validation for this release. Direct3D remains available on 1.26a for the OpenGL HUD symptoms discussed in [issue #10](https://github.com/YazgulDev/warcraft-cs/issues/10). The project licenses and third-party and AI credits are recorded in [NOTICE](https://github.com/YazgulDev/warcraft-cs/blob/v0.20.0/NOTICE).

<!-- launcher-summary
- Added Surf Flow, a connected time trial with 40 banked ramp sections and four turning decks.
- Added a configurable twofold surf acceleration multiplier while retaining ordinary Bunnyhop steering.
- Added RPG, HE, C4 and Half-Life blast jumps and primary or charged Tau recoil jumps.
- Changed ordinary RPG self damage to five percent of its distance-attenuated blast damage.
- Fixed airborne FPS actors snapping down during nearby native pathing corrections.
- Fixed invisible airborne walls from ground-only pathing cells and added solid rock doodad collision.
- Fixed the mismatched shadow and pathing grids in the original weapon-jump test arena.
- Fixed Player installation when Windows marks the launcher as downloaded.
- Added the launcher INI editor and organized the English configuration into 16 categories.
- Added the P228 and Five-SeveN with owned CS models, audio and shop entries.
- Added the Half-Life hand grenade and remote Satchel Charge with owner-safe blast jumps.
- Fixed the Half-Life grenade throw animation and made Satchel throws immediate and non-bouncing.
- Changed HE and Half-Life hand grenade damage and radius with linear three-dimensional falloff.
- Changed the default carry limit to three of each CS grenade type.
- Fixed the hero XP HUD showing MAX one level early.
-->
