# Third-person unit control prototype

Select a living owned mobile unit and press **F7** to enter third person directly.
Press **F7** again in third person to return to RTS; F7 in FPS switches directly to third person.
Press **T** again to return to FPS; **F6** returns to RTS from either mode.
Locomotion and primary attacks accept mobile melee and ranged units, including map-defined heroes.
Buildings, hidden and loaded units cannot be selected.

Mouse wheel **up** moves the third-person camera closer; **down** moves it farther away.
The session's chosen distance survives FPS/third-person switches within the same map. Native terrain
and building collision still shorten the camera when necessary, then restore the requested distance
when the view is clear. Camera rays for manual attacks and spells use the same zoomed position.
Scrolling in third person does not switch CS weapons or interrupt a spell. Buy menus, paused/disabled
control and lost focus discard wheel input; partial notches cannot cross view transitions.
`[ThirdPerson] CameraZoomStep=50` sets Warcraft units per wheel notch (0..500; 0 disables zoom).
`CameraMinDistance=80` and `CameraMaxDistance=1000` each accept 80..1000 units; a maximum below the
minimum becomes the minimum. Missing/malformed values use these defaults. `CameraDistance=500` is
clamped to that interval. F8 applies the settings and resets the session distance to CameraDistance;
new maps do the same. Updates insert missing keys while retaining custom values.

An in-range contact receives colored brackets (red for an enemy, green for an ally, gold for a destructable)
and a ground circle: red for an actual enemy, yellow for allied/neutral units and destructables.
The circle follows terrain/bridge decks and is covered by opaque scenery; no text caption is displayed.
`[ThirdPerson] TargetCircleRadius=64` sets its world-space radius, bounded to 16..256 Warcraft units.
Missing/malformed values use 64. F8 reloads the radius; updates preserve a custom value.
The marker predicts the same nearest contact as LMB, including
camera parallax, configured/native reach and terrain/body obstruction. Melee uses the full configured
forward sweep, so its nearest contact can sit beside the central reticle. Prediction never attacks,
orders or selects the target. `[ThirdPerson] ShowTargetIndicator=true` enables it; false hides it.
Missing/malformed values retain true. F8 reloads this option, and updates preserve an explicit false.
Casting, stun, disarm, hidden/cargo states and buy menus suppress the indicator.

- Mouse aims and turns the hero; WASD moves relative to that bearing. Movement uses the hero's
  effective native speed, including slows, haste and roots. Existing jump/crouch controls remain.
- WASD runs and Shift+WASD walks. Each model supplies its actual Stand/Walk/Run indices; Swim/Fly and
  a speed-scaled Walk cover models without a distinct Run sequence. Warcraft UnitUI walk/run animation
  speeds drive playback; shared sequences keep the same stride between gaits. Model/type changes reset metadata.
  Space lifts the visible body with the physical jump. Authored Jump/Land sequences win; native hit/ready/stand
  poses cover missing leap animations. Stock Arthas has no authored Jump sequence.
  Casting, incapacitation and melee swings keep native animation ownership. F6/T restore playback rate.
- LMB plays the hero's attack animation even with no target. Hold it to repeat after recovery.
  A delayed horizontal forward sweep damages one nearest physical contact. Enemies behind the hero,
  outside melee reach or behind terrain/solid objects cannot receive the contact.
- Ranged attacks converge from the unit's body toward the reticle contact. A second trace prevents firing
  through a wall beside the body. Reach uses the unit's current native primary attack range by default.
  Confirmed unit contacts dispatch the map's native missile and attack effects; empty attacks still play
  their animation. Native homing, impact behavior and map triggers retain ownership after launch.
  Disabling native attack effects, or attacking a destructable, uses an instant damage fallback.
- Autoattack cancellation tracks retained targets per unit, so repeated Stop commands cannot continuously
  reset a manual attack. Automatic pursuit remains blocked; movement and aiming stay under player control.
  Native preparation/impact guards also reject attacks from lingering autonomous behavior; scoped manual
  attacks and follow-only squad policies retain their explicit ownership.
- Damage uses current native unit attack, the third-person multiplier, native armor and the existing
  allied-damage percentage. Native attack effects are controlled by `[Combat] EnableAttackEffects`.
  No CS ammo, CS weapon damage multipliers or AWP finishing are used for hero swings.
- Z/X/C/V and Alt self-casting retain learned slots, mana, native cooldowns and channel ownership.
  Spell targeting starts from the visible camera reticle. Starting an accepted spell cancels a pending swing.
- The hero's native body is visible; first-person arms, recoil and scope are disabled in this mode.
  The centered camera boom shortens against terrain, destructables and buildings. Moving allies do not push it.
- RTS menus/cinematics retain ownership. View transitions cancel pending contacts and keep inventory and position.

`[Shortcuts] ToggleThirdPerson=F7` enters/exits TPS independently of F6.
Free ammo refill now defaults to `Refill=F11`; updates relocate the legacy F7 refill binding to an unused
F11/F12/F5/F4, preserving unrelated custom shortcuts. An explicitly configured TPS key is preserved.
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
| MeleeHeightTolerance | 32 | 0–100 units above/below AttackHeight; include short creep bodies |
| UseNativeAttackRange | true | read current native primary ranged reach, including map/item changes |
| RangedAttackRange | 600 | 30–5000 world units; manual reach or fallback if native range is unavailable |
| RangedRangeMultiplier | 1 | 0.1–5 times ranged reach; final reach bounded to 30–5000 world units |
| NativeRangedProjectiles | true | preserve native missile flight when attack effects are enabled; false selects instant contacts |
| AttackInterval | 2.2 | 0.2–5 seconds before repeating a swing |
| ContactDelay | 0.56 | 0–3 seconds, capped by AttackInterval |
| DamageMultiplier | 1 | 0–10 times current native attack |
| MovementAnimations | true | enable run/idle/leap pose playback; visual jump height remains physical |
| AutoAnimationSpeed | true | use the model's UnitUI gait speeds when available; false uses manual stride |
| AnimationSpeedMultiplier | 1 | 0.1–3 times gait playback speed; visual only |
| RunCycleDistance | 210 | 40–1000 world units travelled per cycle; used when automatic metadata is unavailable or disabled |
| TakeoffDuration | 0.10 | 0.02–0.3 seconds for the takeoff pose |
| LandingDuration | 0.16 | 0–0.5 seconds for the landing pose; zero skips it |

Camera and contact rules reload on F8; initial pitch applies on the next view transition.
Swing timing reloads for the next swing. Delay and recovery scale with native attack-speed modifiers;
the configurable defaults are fitted to Arthas, not universal timings for every Warcraft model.
Combat playback uses the native sequence without replacing its attack animation time scale.
Custom imported models without native gait metadata use RunCycleDistance. The table supplies stock/custom
UnitUI speeds; per-object map overrides not present in that table require manual stride configuration.
Locomotion temporarily owns playback rate and restores the map's previous rate when yielding.
Movement settings reload on F8; phase durations apply at the next physical contact.
Updates insert missing keys and preserve existing custom values. A legacy custom RunCycleDistance
keeps automatic animation speed disabled until explicitly enabled, so its chosen stride still applies.

Known limits: one melee contact per swing and no custom cleave sweep implementation. Native ranged missiles
retain the map's original tracking/impact rules after launch. Arbitrary unit animation sets, extreme camera
settings and every custom map are not verified.
The prototype remains offline Warcraft III 1.26a only.
Native attack retry/wind-up and attack-pose callbacks are suppressed during direct control, including
Sniper. Blocking only native damage still allowed a visible autonomous attack cycle.
Manual LMB attacks and ordinary RTS behavior retain their separate dispatch paths.
Locomotion also observes the actual native sprite sequence: if AI replaces a cached Stand/Walk/Run
pose, direct control restores it. Correct looping sequences are not restarted; casts/stuns/attacks
still own their animation and movement-animation opt-out still yields to Warcraft.

Validation: an isolated Arthas map verified WASD movement, melee contact with native armor,
empty swings, configured allied damage, interruption by incapacitation, Holy Light damage,
Divine Shield, camera shortening behind a building and repeated FPS/third-person transitions.
Native checks also verify speed-matched running, all leap phases, body elevation with unchanged unit
fly height, rate restoration on stun/T and live F8 stride changes. A captured jump was visually checked
through takeoff, apex and landing. Configuration migration, camera geometry, the attack timeline and
motion phase transitions also have automated regression checks.
The expanded native scene verified direct RTS entry/exit, FPS entry and T switching, plus run/Shift-walk
for Arthas, footman, knight, rifleman, tauren, Maiev, a wolf and a harpy. The captured movement cycles were inspected.
Live F8 checks confirmed both the visual multiplier and the manual stride change playback while moving.
Combat checks verified held melee against an attacking creep without repeated Stop, native ranged missiles
for Archmage, Blood Mage and Priestess of the Moon, range limits/manual reach reload, building occlusion,
live damage multiplier, zero allied damage, instant projectile opt-out, stun interruption and FPS contact regression.
A separate private DotA fixture verified Slardar's held attacks, no autonomous damage while idle or extra
impacts between manual contacts, and Sniper's native projectile. Captured attack cycles were visually inspected.
The follow-up Sniper fixture verified idle control without manual swings/damage, repeated LMB missile contacts
and native collision-profile decoding. Four preserved-map ramp trials crossed in both directions along two axes.
Portable regressions cover steep grounded sweeps at 20 FPS, narrow native footprints, legacy opt-out,
non-colliding/invalid profiles and retained bridge/tree/wall/cliff protections. Other custom-map routes remain unverified.
Target-prediction regressions exercise the actual attack picker with camera parallax, changing native/manual
range, terrain obstruction, hidden/loaded/invisible/passable/dead units, allied contacts and melee reach.
Idle prediction produces no animation or damage; manual contact selects the same predicted body.
An isolated DotA scene verified walk-to-idle, forced native sprite replacement, standing next to an
attacking creep, reachable/unreachable targets and manual contact for Sniper, Rifleman and the map's
Hamg/Treant melee definition. Sniper's marker opt-out/re-enable also passed live F8 reload.
The marker screenshots were visually inspected. Other models/maps remain unverified.
Ground-circle checks verified red/yellow pixels, world-depth occlusion and OpenGL state restoration.
An isolated DotA scene verified enemy and own-unit circles and live F8 radius changes from 64 to 96.
Both native circle screenshots were visually inspected.
Wheel-zoom regressions verify fractional/signed input, bounds, opt-out and real config migration.
An isolated DotA scene verified native camera displacement, unchanged TPS weapon, retained FPS weapon
scrolling, discarded fractions across view changes, F8 distance/step/limits reload and collision shortening
without losing the requested distance. The zoomed attack contact and captured native views were checked.
