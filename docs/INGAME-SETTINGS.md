# In-game settings

[Documentation](README.md) · [Configuration](CONFIGURATION.md) · [Previous arsenal preview](ARSENAL-SMOKE-PREVIEW.md)

Available in [release 0.17.0](RELEASE-0.17.0.md), alongside the expanded CS arsenal and dense smoke.

Press **F2** during first-person control, third-person control or clean F9 unit view. The panel deliberately covers the requested main settings: mouse sensitivity, CS audio, movement physics, view distance and sky. Native Warcraft shops and RTS keep their own controls. `[Shortcuts] SettingsMenu=F2` can be remapped; old custom layouts using F2 receive a free F3/F4/F5 binding, or NONE if all are occupied. F6 remains the recovery shortcut and F10 retains the Warcraft menu.

Use the mouse to click +/- or click/drag a numeric slider. Up/down selects a row; left/right or the wheel adjusts it. **Apply & Save / Enter** writes changed keys and applies the validated menu domains immediately. **Cancel / Escape / F2** discards the draft and returns to the current controlled view. **Reset These** drafts the listed defaults while retaining the chosen CS preset; it does not reset the rest of the INI. Reset does not take effect until Apply & Save.

| Menu row | INI key | Default and range |
| --- | --- | --- |
| Mouse sensitivity | `[Mouse] Sensitivity` | 1x; 0.1–5. Main multiplier in every controlled view, including scopes. |
| Scoped sensitivity | `[Mouse] ScopeSensitivity` | 1x; 0.1–5. Additional multiplier while scoped. |
| CS volume | `[Audio] CSVolumePercent` | 100%; 0–100. Active CS voices update immediately; Warcraft audio stays unchanged. |
| Movement physics | `[Movement] Mode` | classic; classic / recovery / hull. Existing velocity/position are not reset by applying the mode. |
| View distance | `[Rendering] DrawDistance` | 5000 Warcraft units; 3000–20000. Native map visibility and atmospheric fog can still hide distant objects. |
| Sky visible | `[Sky] Visible` | true. Controls both map-authored sky visibility and CS sky drawing in controlled views. |
| Sky type | `[Sky] Enabled`, `WarcraftEnabled` | Warcraft by default. A type change selects Warcraft or CS, with the corresponding two existing keys. |
| CS sky preset | `[Sky] <terrain letter>` or `Default` | Current saved terrain preset. Only complete installed private WCS1 sky caches appear as choices. Unknown terrains use Default. |

Main sensitivity preserves the former 0.14 degrees/count at 1x, and preserves the former scoped 0.035/0.009 degrees/count when both multipliers are 1x. Raw mouse counts and cursor fallback share the same validated gain. These preferences load on startup; **F8** still reloads manual INI edits outside the panel. Blank/malformed/non-finite numeric values use documented defaults; finite values are clamped. Installer updates add missing keys without resetting custom sensitivity, sky/audio preferences or existing shortcuts.

Sky presets apply to the current terrain's existing key, rather than changing a fallback that the map's override would ignore. Changing a CS preset takes visible effect with CS sky type selected. Missing/custom unavailable names remain identifiable until a valid installed preset is selected. Native-only installs retain native sky controls without needing CS resources. Native Warcraft sky fallback behavior still uses WarcraftEnabled; disabling Visible additionally hides an authored map sky. Leaving FPS/TPS/F9 restores the latest authored map sky, including map-script replacements. The native sky lease is verified on 1.26a; 1.27a engine internals remain experimental and unverified.

The world continues running. The panel owns look, movement and combat inputs while open; gravity and map simulation continue. Queued clicks/held fire cannot leak into a shot on close, and the mouse is reacquired without replaying old cursor movement. Reload clocks continue. Applying a menu change preserves unrelated session M/N/Y preferences, camera zoom, weapon inventory and ammunition. Map/view/focus/cinematic changes cancel an unsaved draft.

Changed keys are written to a temporary sibling file, flushed and atomically replaced. Save failure leaves the original file and live settings intact and keeps the error visible in the panel. Unrelated keys/comments stay in the same INI. The menu never writes weapon damage, prices or other unrequested configuration.

Verification: portable state/geometry tests cover draft cancellation, reset, failed saves, bounds, no-CS choices and the same sensitivity gain consumed by the controller. Real INI tests cover repeated saves/reloads, read-only rejection, comments/custom keys and validated sky discovery. Hidden WGL tests render real panel/fonts at 800x600 and 1024x768 with inherited hostile raster state. Separate owned Warcraft III 1.26a OpenGL and Direct3D runs exercised the production F2/Enter/Escape dispatcher, held-fire suppression, settings persistence through F8, physics/distance values, authored sky hide/reveal, CS preset renderer reload, F7/F9 access and authored sky restoration on RTS exit. All lab modules/config/caches/graphics bridge/saves were restored by hash. Direct3D captured the initial panel; Windows Graphics Capture timed out on the later F9 frame while native assertions continued. No 1.27a native test is claimed.

<!-- launcher-summary
- Added TMP and UMP45 SMGs, FAMAS and Galil rifles, and a scoped G3SG1 automatic sniper.
- Added private weapon models, animations, reload audio, muzzle flashes, casings and buy previews for the new guns.
- Changed smoke to a dense depth-tested volume with configurable short visibility and all-team AI attack occlusion on Warcraft 1.26a.
- Fixed health bars, item names and target markers revealing targets through dense smoke.
- Fixed carried-weapon browsing to keep long primary lists within the screen.
- Added an F2 in-game settings panel for mouse sensitivity, CS volume, movement physics, view distance and sky controls.
- Added main and scoped mouse sensitivity multipliers with defaults preserving previous aiming speeds.
- Added atomic Apply and Save, draft cancellation, mouse sliders and keyboard navigation without firing through the menu.
- Added controlled-view sky visibility with authored-map sky restoration and installed CS preset selection.
-->
