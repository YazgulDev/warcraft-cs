## Download and upgrade

- [**WarcraftCSLauncher_DLL_Included.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.15.0/WarcraftCSLauncher_DLL_Included.exe) — recommended Player launcher, with bundled project modules and graphics components; no local C++ compilation.
- [**WarcraftCSLauncher.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.15.0/WarcraftCSLauncher.exe) — Developer/source installation with consented dependency preparation and local compilation.
- [**WarcraftCS-0.15.0-dist.zip**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.15.0/WarcraftCS-0.15.0-dist.zip) — standard launcher, sources, update manifest, requirements and checksum. Public ZIPs contain no DLL/MIX modules.
- [Sources](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.15.0/WarcraftCS-sources.zip) · [Update manifest](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.15.0/WarcraftCS-update.json) · [Player SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.15.0/WarcraftCSLauncher_DLL_Included.exe.sha256) · [Developer SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.15.0/WarcraftCSLauncher.exe.sha256).

Save campaign progress and close Warcraft before updating. Use **Update from GitHub**, or install the new launcher into the same dedicated mod folder, then restart Warcraft. Setup prepares graphics components automatically and preserves saves, custom INI settings, prices, audio and opt-outs. Select **Graphics → Auto / OpenGL / Direct3D** before Play; the saved selection takes effect on the next launch. No manual DLL copying or system DirectX installation is required. Your own complete Warcraft/Counter-Strike/Half-Life files remain required; game files and converted assets are not bundled.

## Warcraft CS 0.15.0 — 29 October 2026

Includes all changes since immutable **v0.14.0**: optional Direct3D for Warcraft III 1.26a, saved launcher graphics selection, native minimap/animated portrait and shared inventory in Direct3D, native cursor/health-bar filtering, safer graphics recovery and slope-camera correction. Movement mode, collision, weapon damage and economy retain their existing defaults.

## Graphics selection and automatic setup

Both launcher variants offer **Graphics → Auto / OpenGL / Direct3D**, independently of Reign of Chaos/The Frozen Throne and the Warcraft-only profile. Auto remains the default: **OpenGL on 1.26a**, **Direct3D 9 on experimental 1.27a**. Explicit renderer choices are validated and saved with the launcher preferences; old preferences without a renderer use Auto. F8 reloads game settings but does not switch the running graphics API: restart Warcraft to change it.

Warcraft III 1.26a uses native Direct3D 8. The new Direct3D option prepares an app-local **d3d8to9 1.16.0** bridge and pinned D3DX9/D3DCompiler components, then uses the existing D3D9 presentation adapter for weapons, effects and HUD. Player bundles these components. Developer setup obtains the pinned upstream dependencies and builds the bridge with a static C++ runtime, avoiding an additional Visual C++ redistributable requirement. Setup validates component hashes and bridge metadata, backs up a different private wrapper before replacement and preserves private saves/settings. Nothing is installed into the original game or system DirectX folders.

Source launch accepts `tools/launch.ps1 -Renderer Auto|OpenGL|Direct3D`; the earlier D3D9 spelling remains an alias. [Renderer implementation and verification](https://github.com/YazgulDev/warcraft-cs/blob/v0.15.0/docs/DIRECT3D-126A-PREVIEW.md), [requirements](https://github.com/YazgulDev/warcraft-cs/blob/v0.15.0/REQUIREMENTS.md).

## Direct3D interface and portrait

Direct3D now displays the native minimap with its terrain, fog and unit markers, using the same HUD placement, M toggle, size setting and wrapped hints as OpenGL. A private GPU canvas captures native UI, crops the evaluated Warcraft minimap rectangle and restores world color/depth/viewport before rendering the mod. It performs no per-frame CPU pixel readback and writes no graphics capture files.

The animated unit portrait has separate color/depth surfaces. Capture ends at the next UI projection even when the engine omits another viewport call. Its camera uses the square HUD aspect while preserving native vertical framing, correcting stretching. Portrait capture can nest inside the minimap canvas; resources are released before Direct3D Reset. A second world submission before Present restores the correct world target without copying surfaces inside an active scene.

Shared health/mana/XP, spell and item panels work in Direct3D. Native hero inventory icons/charges, healing-potion use, the I mouse panel and slot dragging were checked in both renderers. Inventory still depends on the controlled unit's actual native inventory ability: ordinary units without it do not gain fictional item slots.

Existing controls/settings remain: `Interface.ShowMinimap=true`, `MinimapSizePercent=22` (10–40), M visibility toggle, `ShowUnitHud`, `UnitHudScalePercent`, `ShowInventory`, `InventoryControls`, I mouse inventory and Tab numeric modes. F8 reloads supported INI values; updates preserve explicit settings. No new gameplay balance or camera-tuning parameter is introduced.

Native UI draw filtering now covers all four Direct3D draw forms. It prevents the engine cursor and oversized selected-unit health bars from leaking into FPS while keeping the mod's configured health bars. The hardware cursor is hidden during mouse look and its requested visibility restored for free-cursor menus/RTS. Native RTS rendering remains available when leaving controlled views.

## Camera on slopes and graphics recovery

On the verified 1.26a host, Warcraft's camera helper added an averaged terrain height after the mod had already supplied the physical eye height. This extra term changed across slopes and produced unwanted view shifts. The guard now removes that native contribution only while the mod owns the camera, including deferred render clamps. Normal RTS/unrelated cameras retain their behavior; unverified hosts keep the previous fallback. Walking, jumping and collision calculations are unchanged. [Camera implementation and native checks](https://github.com/YazgulDev/warcraft-cs/blob/v0.15.0/docs/CAMERA-SLOPE-PREVIEW.md).

Direct3D recovery no longer probes hidden, minimized or inactive windows and allows native reset a grace period after focus returns. Search/system aliases already anchored to the same graphics module are coalesced instead of creating another probe device.

## Source and packaging

Minimap capture, native UI filtering, camera terrain normalization, renderer selection and runtime dependency preparation have separate ownership. Upstream d3d8to9 and Microsoft component notices are included with the relevant private components and documented in NOTICE. Public source/distribution ZIPs remain native-free. Bundled Player packages support the graphics component layout while retaining validation of older module-only packages; malformed or missing components are rejected. Private gameplay/movement verification scenes are excluded from ordinary Player modules.

## Verification and limits

Source audit and native compilation pass. Fresh combined native runs on the owned 1.26a map pass six camera cases in each renderer (flat, level eye on a slope, ground following, standing, walking and jumping), with zero submitted-eye error after accounting for native getter latency. The same runs confirm portrait/minimap/item panels, native potion use and slot 3-to-5 moves, M/I controls and RTS/FPS transitions, with no ERROR records. Screenshots were inspected.

Fresh regression checks pass for map-camera guards, FPS projection, movement invariants, the complete launcher/install/update/reinstall suite, pinned app-local graphics installation and preservation, actual hidden-device Direct3D UI pixels/draw filtering/Reset, and safe Direct3D probe aliasing. Both final EXEs and source/distribution ZIPs are checked for matching source snapshots, full release-body/manifest summaries, independent checksums and native-free public archives; bundled runtime checks cover version/source/hash/layout validation and malformed-package rejection.

Owned 1.26a native checks cover weapons/reloads, animated portrait and minimap, actual Arthas inventory/potion/slot moves, M/I controls, movement and RTS/FPS transitions in OpenGL/Direct3D. Earlier Direct3D fullscreen checks covered 1920×1080, buy menus, third person, native cursor/health-bar filtering and live minimap/HUD scaling. Test installations were restored after verification.

Full campaign compatibility and Alt-Tab recovery parity remain unverified. Real 1.27a gameplay is unavailable locally; that host and OpenGL-on-1.27a remain experimental. This release adds another graphics path, but does **not** establish the root cause of the AMD RX 580 OpenGL failure in issue #10 or claim its resolution. Offline single player only; Reforged and other patches remain unsupported.

<!-- launcher-summary
- Added optional Direct3D for Warcraft III 1.26a with automatically prepared private graphics components.
- Added saved Auto, OpenGL and Direct3D choices in both launchers, applied on the next game launch.
- Added the native Direct3D minimap with the shared placement, size setting and visibility toggle.
- Fixed animated Direct3D portrait capture and square HUD framing.
- Fixed native cursor and oversized health bars leaking into first-person Direct3D views.
- Improved Direct3D surface restoration and recovery probe safety.
- Fixed extra native terrain height shifting the controlled camera on slopes without changing movement or collision.
- Preserved shared native inventory controls, graphics preferences, custom settings and saves during updates.
-->
