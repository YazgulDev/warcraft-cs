# CS 1.6 weapon presentation

## Owned conversion and animated barrel

Extend `tools/export_models.py` and private setup from the player's installed `cstrike/models/v_*.mdl` and sound files. Preserve retail animation names, frame rates, bone transforms and sound events; inspect the actual model before choosing idle, draw, fire and reload clips. Single shots must restart the firing clip, while a burst must not restart it on every scheduled bullet. Pump and semi-auto shotguns use the shared per-shell reload timeline and interruption policy.

Export muzzle attachments as the optional `WCM1` footer of `WCG2`: uint32 count, then uint32 bone and three float32 local coordinates per anchor. The loader limits count to eight and checks the bone index and finite coordinates. Source attachment records have stride 88, bone at +36 and origin at +40; header attachment count/index are at 212/216. Choose the barrel matching the active model variant: stock M4A1 and USP use attachment 2 for their permanently unsilenced poses. Do not copy those indices blindly to other models. Export ordered anchors for alternating dual barrels. Buy-preview conversion must preserve the footer. Legacy caches derive an anchor from their forward-most idle vertex; regenerate private caches to obtain authored anchors.

`GunMesh` transforms the anchor with the same interpolated animation bone as the mesh, then maps GoldSrc (x,y,z) to GL (y,z,-x). Never place flashes at guessed screen coordinates. `MuzzleFlashView` renders original additive geometry, so no retail flash sprite is distributed. Its GL state must be restored and depth writes disabled.

## Common runtime path

- Append stable weapon IDs; include each firearm in `WeaponSlots::Firearm`. An accepted real round in `ShooterController::Fire` updates the shared shot serial after ammo consumption. Each scheduled burst round passes through that path. Empty clicks, rejected cadence, reload actions, melee and planting never create flashes.
- `MuzzleFlashPulse` starts on the first presented frame and expires once. Switching weapons, views, shop/focus/map transitions and disabled visuals must consume or invalidate stale events. Scoped AWP can show its flash without drawing hands. F7/F9 must not inherit first-person effects.
- `ViewmodelMotion` consumes actual movement speed and grounded/duck state. `Overlay` applies its pose to both mesh and flash. Keep camera, aim ray, recoil damage calculations and reticle independent of this decorative pose. Fade on stopping and airborne frames; suppress in scope and cursor/casting modes.
- Use `[Viewmodel]` controls for movement and flashes. Any additional player tuning needs validated INI defaults, F8 reload, migration preserving custom values and documentation. Do not duplicate per-gun bob logic.

## Verification

Run `tools/test-viewmodel.ps1` for frame-rate-independent movement, pulse expiry, synthetic MDL attachment export and actual hidden-WGL pixels/state/animated anchors. Extend meaningful cases for new behavior. Run gameplay/config migration, buy catalog, affected recoil/fire/reload tests and build the native DLL/launcher. Use only synthetic test assets in source.

In an authorized private offline map, buy/select the new weapon; walk, stop, duck, jump, shoot, reload and interrupt when supported. Capture enough frames to observe brief flashes; inspect the muzzle in idle/fire/moving poses, all fire modes and scoped mode if applicable. Check each accepted bullet consumes ammunition and emits one event. Verify disabled/F8 settings, inventory/shop transitions and F7/F9. Record actual results privately and keep game assets, recordings, saves and binaries out of Git. Follow game-automation authorization and PID/focus guards; this skill grants no restart or publication permission.

## Consumable grenades

Append grenade IDs through WeaponSlots::Grenade and exclude them from Firearm. Integrate the equipment catalog, one-charge repeat purchases/caps, empty-stack switching, owned refills/runes, private viewmodel and hand-free preview conversion. Keep pullpin/throw terminal poses until GrenadeThrow advances; deploy maps from ordinary draw. A held pin must never consume inventory or cook its fuse. Native launch success spends exactly one charge; holstering/menus/casts/focus loss cancel only an unlaunched throw. NativeGrenades retains launched effects independently of the current weapon/view and uses Warcraft time for pause-aware fuses/clouds. Do not dispatch hitscan, reload or muzzle flashes for grenades. Run `tools/test-grenades.ps1`, catalog/config/migration tests and native checks for each throw/effect plus normal firearm regressions. Document any Warcraft adaptation (flash unit disorientation), tuning and smoke's native-AI visibility limits.

Owned pinpull.wav has odd-length PCM and unpadded trailing LIST metadata. Keep the GameAudio regression that accepts complete bounded PCM without requiring optional metadata; verify pin-pull audio in a real voice as well as detonation sounds.

Thrown grenades must use owned w_hegrenade/w_flashbang/w_smokegrenade models in private assets/world caches, separate from held v_ models and buy previews. Extend setup preflight and synthetic install fixtures together. Use GrenadeWorldView/GunMesh raw world geometry with native perspective/depth and centered authored origins; never replace the body with a screen disc. Preserve context ownership and test actual WGL scale/spin/occlusion plus native flight/landing in F6/F7/F9. World appearance knobs are launch snapshots reloaded through F8.
