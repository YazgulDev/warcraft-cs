# Warcraft CS 0.9.1

Released 2026-10-16. These notes cover the complete change set since the immutable **v0.9.0** tag.
This patch addresses HUD text rendered as solid yellow bars after entering FPS with **F6**
or third person with **F7**. Gameplay features from 0.9.0 remain available.

## Download and upgrade

- [WarcraftCSLauncher_DLL_Included.exe](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.9.1/WarcraftCSLauncher_DLL_Included.exe): ready project modules; choose **Install — Player** to avoid C++ Build Tools/SDK downloads.
- [WarcraftCSLauncher.exe](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.9.1/WarcraftCSLauncher.exe): source installation, with a local native build and dependency preparation after consent.
- `WarcraftCS-0.9.1-dist.zip` contains the standard launcher, sources, update manifest, requirements and checksum. All public ZIPs remain free of DLL/MIX modules; the DLL-included EXE is separate.

Existing clients can use **Update from GitHub**, then close and restart Warcraft to load the new
module. A running process continues using its already loaded module. A fresh installation can
use **Install** in either launcher. Existing gameplay settings and the Warcraft-only profile are
preserved; this patch adds no configuration keys, changes no defaults and requires no INI edits.

The supported host remains offline **Warcraft III 1.26a x86**, with `Game.dll` **1.26.0.6401**.
The full weapon profile also requires owned Counter-Strike 1.6 assets. The Warcraft-only profile
needs no CS assets. No original game libraries, fonts, maps, saves, models or sounds are bundled.
See [installation](https://github.com/YazgulDev/warcraft-cs/blob/v0.9.1/INSTALL.md) and
[requirements](https://github.com/YazgulDev/warcraft-cs/blob/v0.9.1/REQUIREMENTS.md).

## HUD text fix

- Replace driver-generated `wglUseFontBitmapsA` display lists with system-font glyph coverage
  rasterized in a separate GDI memory DC and uploaded as alpha textures. Text is drawn with
  ordinary textured quads, avoiding the old bitmap-list path implicated by the report.
- Apply the same renderer to shortcut hints, health/ammunition/gold, transient status messages,
  buy-menu headings/options and spell keys/readiness/cooldown labels.
- Keep transparent spaces, glyph bearings, measured character advances and the existing
  resolution-scaled status/spell sizes. Partially offscreen text can retain its visible characters.
- Isolate texture uploads from inherited unpack offsets, row stride, alpha transfer and color
  mapping. Restore upload and draw state, including texture matrices, color masks and polygon
  state, so text does not inherit those host settings or change Warcraft's subsequent rendering.
- Retain Overlay's context/focus ownership: rebuild size-dependent atlases and forget old GPU
  names when the OpenGL context changes, without deleting a replacement context's resources.

## Source, packaging and documentation

- Add the focused `presentation/HudFont` module and include it in the normal x86 build.
- Add `test-hud-font.ps1` to the native-independent regression wrapper. It reads actual WGL pixels
  to check glyph coverage and transparent spaces at five sizes, resize, inherited render/upload
  state, partially offscreen strings and three context replacements.
- Update the module guide, changelog, current-version download/install references, validation
  record and version-specific launcher summary. Historical release notes remain unchanged.
- Both launcher variants embed the same audited source snapshot. The Player payload contains
  ordinary project modules with opt-in scene fixtures excluded; retail game files stay private.

## Validation and limits

The complete native-independent regression wrapper passed for the merged runtime sources.
The font pixel/state oracle passed on the local Intel UHD OpenGL renderer. A separate offline
Warcraft 1.26a test map exercised actual F6/F7 input, FPS/TPS and RTS return/reentry, readable
shortcut/status/spell text and the buy menu using an ordinary module without test handlers.
Screenshots were inspected; no controller faults or OpenGL errors were recorded. The test game
exited normally, private module/config backups were restored byte-exact, and save snapshot
comparison found no changes. The 0.9.1 rebuild changes version/source metadata, not that runtime logic.

Release checks cover source-only auditing, both actual launcher EXEs, complete release-body and
manifest summary resolution, source identity and checksums, native-free public ZIPs, and Player
payload source/version binding, x86 layout and corrupt-package rejection. Launcher/install/update
regressions are also checked for this package.

The reporter's exact GPU/driver failure could not be reproduced from a photograph. This patch
removes the affected rendering path and has been verified locally; confirmation on the reporter's
computer remains outstanding. This is not a complete campaign, custom-map or cross-driver test.
Existing prototype limitations from the [0.9.0 notes](https://github.com/YazgulDev/warcraft-cs/blob/v0.9.1/docs/RELEASE-0.9.0.md)
still apply.

<!-- launcher-summary
- Fixed HUD text appearing as solid bars after pressing F6 or F7.
- Improved shortcut hints, status, buy-menu text and spell labels across font sizes and OpenGL context changes.
-->
