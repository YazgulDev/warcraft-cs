# Direct3D for Warcraft III 1.26a — local preview

[Project home](../README.md) · [Installation](../INSTALL.md) · [Compatibility](COMPATIBILITY.md)

This feature branch adds a Direct3D launch path for **1.26.0.6401** and a graphics
selector to both launcher variants. It is not part of the published 0.14.0 assets.

Choose **Graphics → Direct3D**, then Play. Auto retains OpenGL on 1.26a and D3D9
on 1.27a; explicit OpenGL remains available. The choice is saved with the existing
launcher preferences as `Renderer=Auto|OpenGL|Direct3D`. Missing/empty legacy
preferences use Auto, invalid values are rejected. Changes take effect on the
next game launch and do not change the selected RoC/TFT edition or gameplay INI.
The preference file is `%LOCALAPPDATA%/WarcraftCSLauncher/last-install.json`.

Run Install with the preview launcher once for an older installation. Player
setup installs the bundled components; Developer/source setup obtains pinned
upstream dependencies automatically. After setup either renderer works offline.
There is no separate DirectX/SDK installer or manual DLL configuration. A source
checkout uses `tools/launch.ps1 -Renderer Direct3D` after setup; the existing
`-Renderer D3D9` spelling remains an alias. Auto is the default.

## Implementation and distribution

1.26a renders through native D3D8. The app-local **d3d8to9 v1.16.0** bridge converts
its calls to D3D9, where the mod's existing adapter draws weapons, effects, text,
panels, reticle and menus. This retains the verified 1.26a gameplay/native bindings.
1.27a uses its native D3D9 path and keeps its existing experimental limitations.
Recovery also avoids probing an inactive/minimized game, leaves a grace period
after focus returns, and recognizes when search/system slots name the same
already anchored runtime.

The UI correction filters Warcraft's native UI draw submissions rather than just
scissoring its top and bottom bands. Native cursor and long selected-unit HP bars
therefore do not remain over the FPS world. The mod's health bars continue to use
`Interface.ShowAllyHealthBars`, `ShowEnemyHealthBars`, `HealthBarDistance` and K;
their defaults have not changed. Hardware cursor visibility follows mouse-look
ownership and returns to the game's requested state in free-cursor menus/RTS.
The native animated portrait renders to its own D3D color/depth target and is
composited into the HUD. Capture ends on the next orthographic projection even
when the engine omits another viewport call. Reset releases its default-pool
surfaces; this does not introduce an external portrait asset or readback per frame.
The portrait's D3D camera uses a square HUD aspect while preserving its native
vertical framing, so the rectangular capture does not squeeze the displayed face.

Direct3D now also captures the native minimap. A private full-resolution UI
color/depth canvas keeps hidden Warcraft chrome away from the world; a GPU-only
`StretchRect` copies the evaluated native minimap rectangle for the common HUD.
The native terrain, fog and markers keep their usual engine updates. Portrait
capture nests inside the private UI target. World color/depth and viewport are
restored before weapons/HUD, including an extra world submission before Present;
Reset releases the default-pool capture surfaces.

Both APIs use the existing `Interface.ShowMinimap=true`, `MinimapSizePercent=22`
(10–40 percent of client height), M toggle, `ShowUnitHud`, `UnitHudScalePercent`,
`ShowInventory` and `InventoryControls` settings. F8 reload and update preservation
remain unchanged. Items use the shared six-slot inventory, imported/native icons,
charges, numeric use, I mouse panel and drag/drop behavior. A unit without native
inventory slots has no item row, in either API; no inventory abilities are invented.

Only the marked private game directory receives `d3d8.dll`, `d3dx9_43.dll` and
`d3dcompiler_43.dll`. The bridge is compiled with a static C++ runtime; its import
table is checked to avoid a hidden dependency on a developer machine's Visual C++
installation. Its build checksum and upstream revision are recorded in
`direct3d-runtime.json`. Setup validates pinned SDK hashes and the bridge's build
checksum, backs up a different pre-existing private
wrapper, and preserves saves/settings. The source-only EXE and public ZIPs contain
no DLLs. The separate Player EXE embeds the bridge, Microsoft's D3DX redistributable
and their license notices with the normal native package. Original game folders
and Windows system directories remain untouched.

Upstream references and hashes:

- [d3d8to9 v1.16.0](https://github.com/crosire/d3d8to9/tree/v1.16.0), revision
  `255338f698c8270b537f0a91a13f795f4f988250`, tag archive SHA-256
  `3b4e8263f8792cc839cc3bd2576d5f74082a664997a2f8a4766e08cfd881b13f`.
- [Microsoft.DXSDK.D3DX 9.29.952.8](https://www.nuget.org/packages/Microsoft.DXSDK.D3DX/9.29.952.8),
  archive: `ead0906ae8a26c18a7525da7490127a2110f7c58f18293738283e30e97c6ea4b`;
  x86 D3DX DLL: `2fea401698db2764ec5943de6634bf0eb5dbe191a963b1ef3620a027730b83e9`;
  x86 compiler: `aaca0bac2e1e94966f4546b6234333f9cdc1a4599024e30cd20aef4c1fd1b4f1`.

## Native verification

The isolated owned 1.26a Health-Bars map ran through the actual D3D9 presentation
callback at 1422×693. Screenshots were inspected after the first USP shot, during
M4A1 reload, after switching to P90 and after FPS re-entry. Weapons/hands, green
reticle, gold status/hints, ammo/dollars, health/mana and spell panels were visible.
The log recorded 81 `d3d9-present` summaries, accepted round serial 128, M4A1
reload start/completion, ten weapon slots loaded/drawn and zero ERROR entries.
This is actual game verification, separate from hidden-device rendering tests.

The UI correction was additionally checked in the owned map with its actual
native portrait, no leaking selected-building HP line, a buy menu, an ogre target,
RTS exit and FPS re-entry. Real hidden HAL-device tests verify all four native draw
forms, preservation of world/canvas pixels, actual portrait texture pixels,
hardware cursor suppression despite engine show requests, free-cursor restoration,
projection-only capture termination, Reset and native RTS restoration. The WGL
portrait animation/world color/depth regression also passes.

The corrected build also ran at native fullscreen 1920×1080 with `focus=1`:
screenshots show the portrait and reticle, no native cursor/long HP line, working
buy menu and RTS/FPS transitions. With mod health bars enabled the ogre has a
normal-sized bar at distance 300; changing the lab-only `HealthBarDistance` from
2000 to 200 produces zero candidates/submissions and removes that bar. The log
contains no ERROR entries. All lab module/INI/caches/Save files were restored.

UI parity was then checked on the same owned Arthas hero in OpenGL and Direct3D
at 1422×693: native minimap, portrait, HP/mana/XP, spell icons and four item icons;
healing-potion consumption, slot 3→5 item move, I panel, M off/on, movement and
RTS/FPS re-entry. The final D3D portrait camera was checked again at 1920×1080,
with buy menu, third person, live minimap size 22→32 and unit-HUD scale 100→140.
Logs confirm native item-use/move acceptance and no ERROR entries in those runs.
Actual screenshots were inspected. Hidden HAL tests additionally verify nested
portrait/minimap ownership, GPU crop pixels, world color/depth preservation,
Reset and an extra RenderWorld inside the same BeginScene. WGL minimap/portrait
and interactive-inventory regression tests pass. Original lab files were restored.

Full campaign and Alt-Tab recovery parity remain unverified.
The change supplies an alternate graphics path, not a confirmed diagnosis or fix
for the AMD OpenGL failure in issue 10.
