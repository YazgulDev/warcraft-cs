# Portals and heavy weapons

[Project home](../README.md) | [Controls](CONTROLS.md) | [Settings](CONFIGURATION.md)

The local development build supports active native waygates during direct first-person,
third-person and unit-view control. Approach the portal to travel to its map-authored
destination. Inactive, hidden or dead waygates do not activate. Native placement keeps the
destination pathing behavior; the camera, physical floor, velocity and recovery history
are rebased to the actual destination. Leave the arrival portal before travelling again.

Scripted region teleports keep their own map triggers and destinations. The motor detects
native position changes and rebases its floor/camera, including destinations at another
height. The manual movement budget prevents fast ordinary walking/jumping from being
classified as a teleport. No campaign exits or destinations are invented.

## Configuration

All `[Portals]` values reload with F8; setup preserves custom values.

| Key | Default | Range / meaning |
| --- | --- | --- |
| Enabled | true | Enable portal interaction and native-relocation correction |
| Radius | 160 | 32-512 Warcraft units from an active gate's center |
| HeightTolerance | 160 | 16-512 Warcraft units; gates at another elevation cannot pull the actor through floors |
| CooldownSeconds | 1 | 0.25-10 simulation seconds; paused with Warcraft, plus the leave-gate latch |

`[Loadout] Crowbar=true` adds an enabled Crowbar to the initial kit on startup/map load.
Its complete owned Half-Life resource seal is still required. Missing files and the Only
Warcraft profile never unlock it. F8 or camera switches do not grant another kit. Set false
to keep it purchasable without a free starting grant. The normal kit still selects the USP.

## Increased stock damage

| Weapon | Fixed damage | Native attack multiplier |
| --- | --- | --- |
| AWP | 230, previously 115 | 6, previously 3 |
| G3SG1 | 160, previously 80 | 4.4, previously 2.2 |
| HE grenade | 1200, previously 600 | Fixed blast; existing radius/falloff retained |
| Tau Cannon | 200 primary / up to 2000 charged, previously 100 / 1000 | Fixed energy damage |

Native armor, immunity, attack effects and headshot rules still apply. These values do not
turn the AWP into an unconditional one-shot weapon. Ordinary RPG stays at 1800 peak damage
and radius 1000; its existing distance falloff and friendly/cover rules remain in place.

Damage and charged damage accept 0-1000000; native multipliers accept 0-1000. F8 affects new
contacts/throws. Flying grenades/rockets retain their launch snapshot. Setup and the loader
upgrade only former stock values, preserving custom tuning, zero damage and explicit current
version opt-backs: AWP/G3SG1 DamageVersion=2 and MultiplierVersion=2, HE DamageVersion=4,
Tau DamageVersion=3 and ChargedDamageVersion=2. Older stock chains also upgrade correctly.

## Native exploded deaths

`[Combat] ExplosiveWeaponDeaths=true` makes eligible hostile creatures killed by Gluon,
Tau or ordinary RPG use Warcraft's native exploded/artillery death path. Native death
events, map callbacks, kill ownership, armor and immunity remain authoritative. Structures
and allies keep their ordinary death policy. Native model/hero restrictions are respected.

The flag is scoped to the damage transaction: surviving creatures regain their previous
state, and a map-owned exploded flag is preserved. Removed/replaced victims and map changes
cannot receive stale restoration writes. Full-body explosion takes precedence over energy
head-removal presentation. Disabling the setting restores normal deaths; F8 applies to
subsequent contacts, while RPG rounds retain their captured policy in flight.

Public functions are resolved from exact native names/signatures, excluding null-registration
stubs and ambiguous bindings. The exploded flag layout is derived from the verified public
setter rather than assuming another patch's unit layout. An unavailable adapter retains
normal death. Exploded-flag leases are enabled only on the verified 1.26a host; experimental
1.27a retains normal deaths. No real 1.27a portal verification is claimed.
