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

The native minimap and portrait capture adapters still require OpenGL and have
no D3D9 feed. Their images are omitted in Direct3D; other HUD panels remain drawn.
Full campaign/fullscreen/Alt-Tab parity is not implied by this windowed test.
The change supplies an alternate graphics path, not a confirmed diagnosis or fix
for the AMD OpenGL failure in issue 10.
