# Download and upgrade

- [**WarcraftCSLauncher_DLL_Included.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.18.0/WarcraftCSLauncher_DLL_Included.exe) - Player launcher with bundled project modules and private graphics components; no local C++ compilation.
- [**WarcraftCSLauncher.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.18.0/WarcraftCSLauncher.exe) - Developer/source installation with consented dependency preparation and local compilation.
- [**WarcraftCS-0.18.0-dist.zip**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.18.0/WarcraftCS-0.18.0-dist.zip) - standard launcher, sources, update manifest, requirements and checksum. Public ZIPs contain no DLL/MIX modules.
- [Sources](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.18.0/WarcraftCS-sources.zip) | [Update manifest](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.18.0/WarcraftCS-update.json) | [Player SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.18.0/WarcraftCSLauncher_DLL_Included.exe.sha256) | [Developer SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.18.0/WarcraftCSLauncher.exe.sha256).

Save progress and close Warcraft before updating. Use **Update from GitHub**, or install the new launcher into your existing dedicated mod folder, then restart Warcraft. Setup preserves saves, custom settings, controls, prices, renderer choices and opt-outs. Missing swimming keys are added automatically. The former stock RPG radius upgrades once; custom radii remain unchanged. This release adds no retail-asset conversion requirement. Original game files and converted assets are not distributed.

## Warcraft CS 0.18.0 - 2 November 2026

All changes since immutable **v0.17.0**: automatic swimming in deep water, bounded exits from canals, the visible militia-alarm crash fix, carried C4 in Bombs, and a larger ordinary RPG blast with distance falloff. Movement remains **classic** by default. The previous arsenal, economy, settings panel and nuclear RPG remain available.

## Swimming and leaving the water

On verified **Warcraft III 1.26a**, direct FPS, unit-view and third-person control automatically switch to surface swimming in deep native water. WASD paddles at 65% of the current weapon/unit speed, including native slows and haste. The body floats below the actual water plane, with the physical camera/aiming eye 12 Warcraft units above it. Shallow water uses the terrain bed and returns to walking. There are no dry concrete footsteps while swimming or wading.

To leave a canal, **move toward a reachable bank and press Space once**. A nearby shore probe permits a bounded jump through the existing collision sweeps. Holding Space in open water does not create repeated swimming jumps or additive bunnyhop speed. Tall walls, buildings and solid obstacles remain blocked. Roots/stuns prevent paddling and new exits without disabling buoyancy.

Classic, hull and recovery share liquid classification while retaining their solid/cliff collision strategies. Walking on an actual bridge remains walking; an overhead deck is distinct from the water below it. Native flying actors retain their existing movement. Third-person water bodies use their actual native sprite base, including wading and exit arcs. Models use authored Swim/Walk Swim/Stand Swim clips when present, with their own walk/stand fallback otherwise.

Edit `[Swimming]` in `WarcraftCS.ini` and press **F8**; startup and live reload use the same finite, bounded values. Updates preserve explicit opt-outs and custom tuning.

| Key | Default | Range / units |
| --- | --- | --- |
| Enabled | true | false restores the previous water-support movement |
| MinimumDepth | 96 | 32-512 Warcraft units, with small exit hysteresis |
| SpeedPercent | 65 | 10-100% of current weapon/unit speed |
| Acceleration | 6 | 1-30 wish-direction acceleration multiplier |
| Drag | 4 | 0-20 momentum damping per second |
| FloatSeconds | 0.2 | 0.05-2 seconds; buoyancy response time |
| EyeAboveWater | 12 | 0-48 Warcraft units above the liquid plane |
| ExitHeight | 128 | 0-192 Warcraft units; exit apex above water, 0 disables shore exits |
| ExitReach | 128 | 32-192 Warcraft units; bank detection in the movement direction |

See [Swimming](SWIMMING.md) and the [annotated INI](../config/WarcraftCS.ini). This is surface swimming; diving and drowning are not added. The experimental 1.27a host has no verified liquid adapter and retains its previous water movement.

## Militia alarm crash

Fix the 1.26a crash when a visible Town Hall raises an alarm and workers become militia, including when an FPS shot synchronously triggers the alarm. Gameplay previously ran after native skinning but before drawing, allowing replacement models with uninitialized bone matrices into the current frame. Controller actions now run once after completed world/UI/HUD submission, before the next native pose update. Movement keeps one elapsed-time tick per frame; the existing 1.27a presentation fallback is retained.

## C4 in Bombs and a wider RPG blast

Carried **C4 now appears in Bombs (group 4)** with HE, Flashbang and Smoke. Its separate group 5 column is retired; **Half-Life stays on group 6**. Ownership IDs, buying and planting remain intact. Tab still starts in Warcraft item-number mode: turn that mode off for weapon category keys, or use the weapon wheel.

Ordinary RPG stock radius increases **600 -> 1000 Warcraft units**, with **1800 peak fixed damage**. Existing linear three-dimensional distance falloff reaches zero at the edge: at distances 0 / 500 / 750 / 1000, a baseline unmodified blast contributes 1800 / 900 / 450 / 0 damage before host-specific protection or damage handling. Cover, friendly-fire rules and target filtering remain in effect. This changes the ordinary RPG; nuclear blast settings are independent.

`[RPG] Radius` accepts 32-3000 and `Damage` accepts 0-1000000. F8 applies edits to **newly launched rockets**; flying rockets retain their launch snapshot. `BlastVersion=4` migrates the former stock radius once, including older stock chains. Custom radii and explicit current-version choices remain saved. See [Half-Life RPG](HALF-LIFE-RPG.md) and [Weapon selection](WEAPON-SELECTION.md).

## Source, setup and verification

The source-only tree adds portable water settings/contact logic, a signature-checked native liquid query, shared water/shore adapters, authored swimming animation selection and water body presentation. Project-authored offline militia/swimming JASS fixtures and opt-in native scenes support regression checks. The source audit admits those explicit fixture scripts without admitting arbitrary extracted JASS. Normal Player modules exclude the native test scenes and input mailboxes.

`tools/test-all.ps1` passes the complete native-independent suite, including movement/camera, collision, posed hitboxes, native combat, settings/migrations, weapon selection, grenades/RPG launch snapshots and falloff, HUD/font pixels, graphics state, host profiles and Miles forwarders. Real hidden OpenGL/Direct3D devices exercise graphics API behavior.

Owned offline 1.26a scenes reproduce the original militia morph crash and verify the correction through real weapon-triggered alarms in both renderers. Water scenes verify native liquid depth/plane, buoyancy, speed, Space bank exits, shallow exits, all three movement modes, F8 opt-out/re-enable, gunfire and native flight. Actual surface 166.4, bed 0, physical feet 82.4 and camera eye 178.4 agree; bank exits land on dry terrain at 256. OpenGL water frames were inspected. Lab files and saves are restored by hash; the user's campaign is not used.

Release checks audit the entire source tree, compare the embedded source archive with the reviewed index/tag, verify both EXEs and the source/version/hash-bound Player payload, exclude test fixture controls from normal modules, and exercise setup, upgrades, reinstall and launcher replacement with progress/config preservation. Public archives stay native-free; compiled project/compatibility modules are distributed only in the separate Player EXE. This validation does not establish every campaign or GPU driver.

## Compatibility and known limits

Offline single player only. Supported native validation uses owned **1.26a x86, Game.dll 1.26.0.6401**, in OpenGL and Direct3D. **1.27a, Game.dll 1.27.0.52240**, remains experimental; its swimming adapter is unavailable and no new real 1.27a verification is claimed. Reforged and other patches remain unsupported. Auto continues to choose OpenGL on 1.26a and Direct3D on experimental 1.27a.

Every campaign, custom bank, water material and creature rig has not been tested. Shore exits require a reachable nearby bank and do not climb arbitrary walls. Authored swimming clips vary by model. Native water physics and canal exits are verified in the isolated lab in both renderers. OpenGL FPS/TPS frames were inspected; no Direct3D capture is claimed.

The AMD RX 580 OpenGL UI failure in [issue #10](https://github.com/YazgulDev/warcraft-cs/issues/10) remains unresolved. This release does not claim to fix its root cause; Direct3D remains an available alternative renderer. Existing nuclear-death triggers and native map scripting can still affect campaign progression.

<!-- launcher-summary
- Added automatic surface swimming in deep native water on verified Warcraft 1.26a.
- Added slower paddling, smooth buoyancy and shallow-water walking in classic, hull and recovery.
- Added fresh-Space exits toward reachable banks and canal edges while retaining solid collision.
- Added editable Swimming settings for depth, speed, buoyancy, eye clearance and shore exits with F8 reload.
- Preserved custom water settings, bridge riders and native flying movement.
- Added authored swimming animation selection with native gait fallback and water body positioning.
- Fixed the visible Town Hall militia-alarm crash triggered by combat and worker model changes.
- Changed carried C4 to Bombs group 4 and removed its separate column while retaining Half-Life on group 6.
- Changed ordinary RPG stock radius from 600 to 1000 with 1800 peak damage and linear distance falloff.
- Preserved custom RPG tuning and flying-rocket snapshots during F8 and versioned upgrades.
- Updated movement guides, configuration, native fixtures and regression coverage.
-->
