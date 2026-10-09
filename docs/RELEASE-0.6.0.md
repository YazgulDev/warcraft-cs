# Warcraft CS 0.6.0

This release brings all completed work since 0.5.1 into the release: a CS-style buy menu,
configurable loadouts and economy, multiple C4 charges, Reign of Chaos launching,
sky options, fullscreen recovery and native rendering/movement fixes.

## Downloads and requirements

| Download | Contents and installation |
| --- | --- |
| `WarcraftCSLauncher.exe` | Source-only launcher. **Install** builds locally with Microsoft C++ Build Tools / Windows SDK. |
| `WarcraftCSLauncher_DLL_Included.exe` | Separate launcher with project-built modules. **Install — Player** avoids Build Tools / SDK; **Install — Developer** builds locally. |
| `WarcraftCS-0.6.0-dist.zip` | Source-only launcher/checksum, exact sources, update manifest and requirements. |
| `WarcraftCS-sources.zip` | Audited project sources, including `play-roc.bat`. |

Requires Windows x64, owned Warcraft III **1.26a x86 / Game.dll 1.26.0.6401**, owned CS 1.6
models/sounds and OpenGL. Both launcher variants may prepare Python/NumPy for local conversion
after agreement. No game files, models, sounds, maps or saves are distributed.
Public ZIPs contain no prebuilt mod DLLs; only the separately named DLL-included EXE supplies them.

## Launching and fullscreen recovery

- Both EXEs offer **The Frozen Throne / Reign of Chaos** under **Game to launch** and remember a successful selection.
- New `play-roc.bat` launches RoC by double-click or command line. It forwards `-Windowed` and `-Map`;
  `play.cmd -Edition ReignOfChaos` also works. Plain `play.cmd` retains TFT.
- Fullscreen launches set DPI awareness before game startup, keeping HUD/frame dimensions consistent after Alt-Tab.
- Focus recovery refreshes viewmodel, buy-preview and sky resources when Warcraft replaces its OpenGL context.

## Buying, equipment and ammunition

- **B** opens a translucent CS-style buy panel with amber borders, red mouse selection, live prices/details
  and private weapon previews without hands. Number keys and mouse both work.
- Categories retain CS keys: **1** pistols, **4** rifles, **6** primary ammo, **7** secondary ammo,
  **8** equipment, **0** back/cancel. **.** buys reserve ammunition for the held weapon.
- Purchases spend native Warcraft gold. A firearm includes one loaded magazine and no reserve rounds;
  ammo packs supply reserve rounds for **R**. Successful purchases close the menu; rejected purchases keep it open.
  Duplicate non-consumables, full ammo, insufficient gold and invalid zones never debit gold.
- Configure weapon `Price`, `AmmoPrice` and `AmmoPack`. Default weapon prices are AK47 **625**,
  M4A1 **775**, USP **125**, AWP **1188**, knife **0**, sword **250**, C4 **50** gold;
  ammo-pack prices retain their previous defaults.
- `[Buy] Access=anywhere|friendly|shops`, `Radius` and optional `ShopTypes` control purchase zones.
  Friendly buildings and visible living allied/neutral shops are recognized, including stock merchants.
  Opening, every purchase and quick ammo recheck access; leaving the zone closes the menu. The world continues running.
- `[Loadout] Mode=melee` starts with USP **12/100**, knife, sword and **20** C4 charges;
  `Mode=all` grants every weapon. `BombCount` and `MaxBombs` configure charges/capacity.
  Starting equipment is granted once per map; F6, F8 and unit selection do not duplicate it.
- **F7** always refills carried equipment for free; **F9** grants all weapons and full ammunition for free.
  Gold is unchanged; Warcraft incapacitation restrictions still apply. Legacy `AllowFreeRefill` is ignored.
  Runes reward only owned weapons and respect configured ammunition/C4 capacity.

## C4 and sky

- Multiple C4 charges can remain planted together. Each has a **35-second simulation-time fuse**
  and its own damage/allied-damage snapshot. Weapon changes and leaving FPS retain charges;
  Warcraft pause suspends fuses and map changes clear old handles. HUD shows charge count and the nearest explosion.
- Warcraft's stock summer/winter sky is enabled by default in FPS on maps without a sky:
  `[Sky] WarcraftEnabled=true`. Existing map skies are preserved; RTS/cinematics restore the original state.
- Optional `[Sky] Enabled=true` selects privately converted CS skyboxes using tileset mappings and `Default`.
  Missing caches retain the native sky. CS sky is disabled by default and takes precedence when explicitly enabled.
- Correct GoldSrc sky face/roof/floor orientation and isolate pixel-upload state to prevent corrupted textures
  or driver faults after Alt-Tab. F8 applies sky settings live. Owned textures are converted locally.

## Native rendering, movement and source organization

- Preserve native hero portraits and RTS viewport layout while expanding the FPS world view.
- Correct near-ground clipping in FPS, including rendering outside the world-batch callback.
- Traverse native walkable bridge/destructable decks using their surface height rather than terrain underneath.
- Move AWP and USP viewmodels slightly closer.
- Filter floating world labels by FPS camera direction and `[Interface] FloatingTextDistance=1200`;
  `0` hides world labels. RTS, harvesting and screen-space text keep native behavior.
- Group native sources into runtime/platform/input/application/config/audio/movement/combat/economy/inventory/
  squad/geometry/presentation modules; launcher sources into app/game/installation/updates/packaging.
  Keep public entry points and updater paths compatible; share the buy catalog between input and presentation.
- Setup adds missing INI keys while preserving custom settings, with idempotent fresh/legacy/empty-file handling.
  Update README, installation instructions, source-layout guidance, current branch references and release documentation.

## Antivirus

Defender detected the embedded `WarcraftCS.mix` in 0.5.0. Unsigned DLL loading and native function hooks
can resemble suspicious behavior to heuristic scanners; this is a plausible explanation, not a vendor-confirmed
cause or a guarantee of a false positive. Local builds can also be flagged. Check official SHA256 files and
submit detections to the antivirus vendor; see [troubleshooting](../TROUBLESHOOTING.md#antivirus).

## Updating and validation limits

Save progress, close Warcraft and use **Update** with your existing installation folder. Launcher variant,
supported Player/Developer mode, saves, private sword choices and custom INI values are preserved.
Installation still requires explicit dependency agreement. Very old 0.5.0 Player clients should download
the current DLL-included EXE directly and select the existing folder.

Release checks cover independent gameplay/rendering rules, synthetic asset conversion, config migration,
launcher/update integrity, native builds, both EXE packages and source-only audits. Previous disposable
Warcraft sessions verified RoC/TFT menus, fullscreen focus recovery, native purchases, multiple C4,
sky rendering, bridges, portraits and floating labels. The new batch entry point was checked with a
substitute launch target, including spaces/Unicode paths, forwarded options and exit codes, without
interrupting the owner's active game. No new complete campaign or clean-machine dependency-installation
verification is claimed. Offline single player only; Reforged, multiplayer and arbitrary custom-map
compatibility remain unsupported or unverified.
