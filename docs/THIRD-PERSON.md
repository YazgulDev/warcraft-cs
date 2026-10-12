# Arthas third-person prototype

Select your Arthas, press **F6** for direct control, then **T** to switch to third person.
Press **T** again to return to FPS; **F6** returns to RTS from either mode.
The first prototype accepts native primary melee actors and is verified with campaign Arthas (`Hart`).
Ranged heroes and their projectiles are outside this prototype.

- Mouse aims and turns the hero; WASD moves relative to that bearing. Movement uses the hero's
  effective native speed, including slows, haste and roots. Existing jump/crouch controls remain.
- LMB plays the hero's attack animation even with no target. Hold it to repeat after recovery.
  A delayed horizontal forward sweep damages one nearest physical contact. Enemies behind the hero,
  outside melee reach or behind terrain/solid objects cannot receive the contact.
- Damage uses current native unit attack, the third-person multiplier, native armor and the existing
  allied-damage percentage. Native attack effects are controlled by `[Combat] EnableAttackEffects`.
  No CS ammo, CS weapon damage multipliers or AWP finishing are used for hero swings.
- Z/X/C/V and Alt self-casting retain learned slots, mana, native cooldowns and channel ownership.
  Spell targeting starts from the visible camera reticle. Starting an accepted spell cancels a pending swing.
- The hero's native body is visible; first-person arms, recoil and scope are disabled in this mode.
  The centered camera boom shortens against terrain, destructables and buildings. Moving allies do not push it.
- RTS menus/cinematics retain ownership. View transitions cancel pending contacts and keep inventory and position.

`[Shortcuts] ToggleView=T` is validated with the existing conflict rules and reloads on F8.
An older custom shortcut occupying T needs a unique `ToggleView` binding.

All `[ThirdPerson]` values are editable in the existing private `WarcraftCS.ini`:

| Key | Default | Range / meaning |
| --- | --- | --- |
| Enabled | true | false disables entry; F8 returns an active third-person view to FPS |
| CameraDistance | 500 | 80–1000 Warcraft units, collision-shortened |
| CameraHeight | 35 | 0–200 units above the existing physical eye |
| CameraClearance | 16 | 0–64 units reserved at obstacles |
| InitialPitch | -12 | -60 to +30 degrees on entering third person |
| AttackRange | 150 | 30–400 units to the target model surface |
| AttackArc | 50 | 1–120 degrees, full horizontal sweep |
| AttackHeight | 64 | 0–200 units above the feet |
| AttackInterval | 2.2 | 0.2–5 seconds before repeating a swing |
| ContactDelay | 0.56 | 0–3 seconds, capped by AttackInterval |
| DamageMultiplier | 1 | 0–10 times current native attack |

Camera and contact rules reload on F8; initial pitch applies on the next view transition.
Swing timing reloads for the next swing. Delay and recovery scale with native attack-speed modifiers;
the configurable defaults are fitted to Arthas, not universal timings for every Warcraft model.
Animation playback uses the native sequence without globally replacing the unit's animation time scale.
Updates insert missing keys and preserve existing custom values.

Known limits: one horizontal melee contact, no cleave sweep implementation or native ranged projectile
simulation. Arbitrary unit animation sets, extreme camera settings and every custom map are not verified.
The prototype remains offline Warcraft III 1.26a only.

Validation: an isolated Arthas map verified WASD movement, melee contact with native armor,
empty swings, configured allied damage, interruption by incapacitation, Holy Light damage,
Divine Shield, camera shortening behind a building and repeated FPS/third-person transitions.
The configuration, camera geometry and attack timeline also have automated regression checks.
