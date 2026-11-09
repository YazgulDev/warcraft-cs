# Explosion and Tau jumps

Ordinary RPG, HE, planted C4 and Half-Life explosive blasts push the controlled player away from the
blast. Aim an RPG at nearby ground to rocket jump. Tau Cannon pushes opposite its
accepted shot: aim down and fire, or hold/release secondary fire for a stronger jump.
Charge-only, cancelled and empty shots add no force. Nuclear blasts are excluded.

RPG self damage defaults to **5% of its ordinary blast damage after distance falloff**,
independently of friendly fire. With Damage=1800 this is at most 90 raw damage, then
native armor/immunity applies. This is not 5% of the player's maximum health.
Other allied targets retain friendly-fire rules; HE/C4 damage rules are unchanged.
Invulnerability prevents damage but does not suppress blast propulsion.

| Section / key | Default | Range / meaning |
| --- | --- | --- |
| BlastJump / Enabled | true | true/false; ordinary RPG, HE, C4 and Half-Life explosive propulsion |
| BlastJump / Speed | 650 | 0–3000 GoldSrc units/s at blast center; linear 3D falloff |
| BlastJump / MaximumUpSpeed | 1500 | 100–3000 GoldSrc units/s; shared upward impulse cap |
| RPG / SelfDamagePercent | 5 | 0–100 percent; 0 disables RPG self damage |
| TauCannon / JumpRecoil | true | true/false; independent of BlastJump.Enabled |
| TauCannon / PrimaryJumpSpeed | 120 | 0–3000 GoldSrc units/s |
| TauCannon / ChargedJumpSpeed | 800 | 0–3000 GoldSrc units/s at full charge; interpolated from primary |

One GoldSrc speed unit equals 1.5 Warcraft world units/s. Edit
`WarcraftCS/WarcraftCS.ini`, then press F8 in FPS. Missing keys are inserted during
installation updates without replacing saved values. Flying rockets, thrown grenades and
planted C4 retain their launch/plant settings; Tau uses settings at shot acceptance.

Force uses physical body height and the existing blast cover checks. Existing
movement collision handles walls, ceilings, ground and swimming after the velocity
change. The impulse does not teleport the unit, grant a second Space bonus or
bypass stuns, roots, cargo or hidden actors. The current Bunnyhop speed cap still
bounds horizontal speed. These impulses cover the mod's weapons, not arbitrary
Warcraft map spells or visual explosion effects.

Small external native XY corrections while airborne retain physical height and
momentum instead of being treated as ground spawns. The correction allowance is four
32-unit pathing cells beyond the budgeted motor step; destination ground must remain
below the feet. Confirmed waygates and distant scripted relocations still discard the
old flight. Logs distinguish `preserve-flight` from `reset`, including distance,
allowance, floor, physical feet and velocity.
