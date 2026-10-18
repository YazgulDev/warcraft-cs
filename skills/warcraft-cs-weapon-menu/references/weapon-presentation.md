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
