# Bunnyhop and airborne movement

[Project home](../README.md) · [Documentation](README.md) · [Controls](CONTROLS.md)

These movement features ship in 0.7.0. Their original feature branches are retained for development history.

The requested reference is LamWarp's [How to Bunny Hop](https://www.youtube.com/watch?v=WschEm9uYao).
Run up with W, jump, and release W. Hold A while smoothly turning the mouse left, then D while turning
right. Repeat these air strafes while holding Space to jump immediately upon landing. AutoJump assists
jump timing; it never turns the camera or supplies A/D input. `AutoJump=false` requires a fresh Space
press for every takeoff. Keeping W pressed or turning against the strafe direction makes gaining speed harder.

Takeoffs skip ground friction and use air acceleration on their first frame. Air acceleration limits only
velocity along the input direction, retaining perpendicular momentum; changing that direction with the
mouse accumulates speed above the weapon's running speed. Straight jumps preserve existing speed without
creating an automatic bonus. Release Space or stop jumping to let ground friction brake.
Shift/Ctrl suppress the extra takeoff boost. Warcraft roots/stuns still stop horizontal movement and
block takeoff; release Space after recovery. Existing slows still cap movement speed.

Jump over low solids when the feet clear their transformed model bounds. Land on their top or continue
past them. Taller walls, buildings and trees retain collision. Walking or jumping off a ledge keeps the
current height initially, then gravity accelerates the fall until landing. No fall damage is added.
Landing uses the player's full collision footprint: a foot reaching the top catches the lip before
the center crosses, and support ends after the trailing foot leaves. Native cliff rims include the
lower-bank pathing cell; validated movement there avoids Warcraft's nearest-cell relocation.
Map bounds and unsupported flat pathing cells remain blocked. Unknown custom models cannot grant
permission to cross blocked pathing. Native cliff levels distinguish cliff faces from ordinary hills.
Shallow-water support retains Warcraft's native height rather than dropping onto the submerged mesh.

The development preview uses a round upright player hull and local surface contacts. Every swept
step checks feet, standing/crouching body height and head clearance against the current terrain,
transformed obstacle bounds and native bridge decks. Jumping directly below an underside stops upward
velocity; releasing crouch under a low ceiling retains the last fitting stance. A low step is accepted
only when the raised body also fits. Wall contacts preserve tangent motion, and existing solid overlap
still allows retreat rather than trapping a spawned/transformed unit.
A native 32-unit path cell which intersects a known model can overhang its real bounds. A clear
adjacent lane uses the checked 3D hull instead of inheriting that cell's empty corners; unrelated
unknown cells retain native blocking.

Terrain normals come from neighboring samples only eight world units away, using one-sided slopes so
flat ledge lips remain usable. Walkable ramps retain step tolerance and existing hop momentum.
Flat airborne ledge contacts obey actual jump reach, so a short jump cannot enter below an unreachable
top and become trapped beneath its floor. A blocked
native cliff face cannot act as ground or grant another jump; a fall onto its face slides down its local
plane. No height from a distant terrace blocks an otherwise clear passage. The default local walk angle
is 46 degrees. Flat lower banks and downhill departures remain usable.

`HullRadius=24` caps the player's native radius (world units); smaller native actors retain their radius.
The unknown-profile/legacy footprint uses this configured radius. Physical height follows the existing
stance, 108 world units standing and 66 fully crouched. Ordinary creature solids retain their native
footprints and authored vertical bounds. Static non-tree destructables use transformed triangles from a supported stationary Stand pose.
The cylinder tests the occupied height, so an arch opening stays open and a low plinth beside a tall
column has its own reachable top. Circle/triangle intersection supplies floor and ceiling heights
across the whole footprint, including partial landings. Opaque/cutout faces collide; additive glow
cards do not. Stand animation, unsupported materials/formats and unreadable models retain conservative
standing bounds/native pathing. Buildings and moving units retain their existing bounds/profiles;
trees retain decoded trunks rather than foliage. This is static surface detail, not animated
polygon-perfect collision. The player uses the configured physical hull, not weapon/cape visual extents.
A known actor radius remains valid when FPS temporarily disables its native pathing mask.


Edit `WarcraftCS/WarcraftCS.ini` in the prepared runtime, then press F8 in FPS. Updates add missing keys
without replacing custom values. F8 replaces tuning without resetting velocity or position; the next
simulation step applies the new speed cap. Jump speed applies to the next takeoff.

| `[Movement]` key | Default | Range and units |
| --- | --- | --- |
| Mode | classic | classic/recovery, case-insensitive. classic restores the collision behavior from commit 5578392 (25 October 2026, 06:28 Moscow), before the 17:08 cutoff; recovery enables the later platform/recovery paths. F8 switches without resetting velocity or position |
| UseNativeUnitCollision | true | true/false; live Warcraft actor/creature footprints. Ignore Locust/pathing-disabled helpers; buildings/trees retain solid model volumes. false restores visual creature boxes and the 24-unit actor footprint |
| BunnyHop | true | true/false; preserves excess takeoff momentum and enables the optional boost and air speed cap. false clips excess speed to the current weapon's running speed at each takeoff |
| AutoJump | true | true/false; false requires a new Space press per jump |
| JumpBoostPercent | 0 | 0–100 percent per moving takeoff; optional arcade bonus, separate from air strafe acceleration |
| MaxBunnySpeed | 1000 | 250–2000 GoldSrc units/s |
| AirAcceleration | 10 | 0–100 wish-direction acceleration multiplier |
| JumpSpeed | 268.328 | 1–800 GoldSrc units/s upwards |
| Gravity | 800 | 100–3000 GoldSrc units/s² downwards |
| StepHeight | 27 | 0–64 Warcraft units; automatic rise/drop limit, larger drops fall |
| MaxWalkSlope | 46 | 15–75 degrees; maximum local walkable slope. Blocked native cliff faces remain non-walkable |
| HullRadius | 24 | 8–64 Warcraft units; maximum player collision radius, preserving smaller native footprints |
| ObjectMeshCollision | true | true/false; use supported stationary prop surfaces for fit, support and head clearance. false restores conservative whole-model volumes |
| RecoverInvalidPosition | true | true/false; repair proven below-terrain states and lost deck support for an established rider; ordinary falls and underpasses stay unchanged |
| RecoveryDistance | 512 | 64–2048 horizontal Warcraft units; maximum search radius for a revalidated visited recovery point |

One GoldSrc unit corresponds to 1.5 Warcraft units. The default jump rises about 67.5 Warcraft units.
Finite out-of-range numbers are clamped; missing/empty/malformed/non-finite values use the defaults.
Boolean values accept true/false case-insensitively; invalid values use their default.

Older test configs with `JumpBoostPercent=8` keep that value during an update. Set it to **0** and press
**F8** for the strafe-only behavior described above. Set `AirAcceleration=0` to disable air strafe gain.
The default cap is 1500 Warcraft units/s, four times the knife's running speed of 375. The supported maximum
is 2000 GoldSrc units/s (3000 Warcraft units/s); swept collision still checks eight-unit segments.

This is a Warcraft adaptation, not an exact GoldSrc engine port. It omits Half-Life's punitive 1.7×
takeoff limiter in favor of the editable cap. Like discrete GoldSrc air acceleration, gain depends on
input timing and simulation step. Independent eight-second trials with alternating turns produce about
792, 1055 and 960 Warcraft units/s at 30, 60 and 144 FPS, respectively, starting from 375 with no takeoff bonus.
These are deterministic physics measurements, not a native-game performance or speed guarantee.
Ground support is resolved after each swept substep, so fast uphill movement does not reuse the floor
from the beginning of the frame. Unknown native collision profiles retain conservative bounds.
The collision preference, walk angle and hull-radius cap reload on F8; updates preserve custom values.
The native collision code requires installing this preview and restarting the game; F8 alone cannot replace it.
Unsupported animated/custom props and buildings retain conservative bounds and may still reject irregular openings.
Native walkable bridge decks remain the surface authority. `ObjectMeshCollision` reloads on F8 without
resetting current motion; setup adds a missing true default and preserves a saved false opt-out.
Almost-flat blocked cliff rims are walkable when their local rise across a terrain cell fits StepHeight.
A continuous steep face remains non-walkable even at its first shallow sliver, preventing repeat takeoffs.

The tree-collision test branch replaces canopy-sized standing extents with decoded trunk bounds,
reusing the shooting mesh parser. Native tree pathing and solid-trunk contact remain enforced;
unknown/unreadable geometry retains the conservative native pathing fallback.

The ledge-recovery test branch probes a full 128-unit native terrain cell in eight directions:
the blocked lower rim may extend beyond the former half-cell probe. Higher terrain is still rejected
before cliff-pathing exceptions. Existing solid overlap permits only retreat toward the nearest
expanded footprint face, including the trailing native pathing samples; entering another solid or
moving deeper into the current solid remains blocked.

Independent checks: `tools/test-movement.ps1`, `tools/test-warcraft-collision.ps1`,
`tools/test-gameplay-settings.ps1`, `tools/test-gameplay-config.ps1` and `tools/test-static-collision.ps1`.
Static geometry tests cover arch width/headroom, partial plinth support, rotated/scaled/mirrored models,
malformed MDX and stationary versus animated Stand poses. Native collision tests retain the previous
uphill-hop measurements (distance 1814.3, peak speed 710.7) and add pathing-disabled actor fit,
shallow blocked rims, mesh opt-out, crouching through an arch and reachable/unreachable plinth jumps.
An owned IceCrownWall900 model decodes to 24 triangles; an animated IceCrownGate retains conservative
fallback. These checks do not establish native campaign verification of the new DLL.
The opt-in `tools/build.ps1 -TestMovement` includes `tests/MovementScene.cpp`, which accepts explicit
requests only for development in a disposable custom map copy. It must never run in campaign saves.
Ordinary builds exclude its scene, requests and destructive test fixtures.

For that explicit test build only, write `strafe x y yaw frames`, `hop x y yaw frames`, `prop x y yaw frames`,
`walkprop x y yaw frames`, `wall x y yaw frames`, `cliff x y yaw frames` or `climb x y yaw frames` to
`WarcraftCS/test-movement.request` inside the runtime. `flat` and `scan` print candidate corridors and
cliffs to the runtime log. The scene clears destructables around its trial corridor and creates paused
test actors; it does not save the map. `prop` and `walkprop` compare a scaled native barricade with and
without jumping, while `wall` attempts the same jump against a native town hall.
`strafe` runs up for one second and supplies alternating turns without W; it requires three takeoffs
and a peak above 450 Warcraft units/s. `hop` verifies retained straight-line speed without a bonus;
run both with `JumpBoostPercent=0`. Contact logs include speed in both coordinate systems and velocity.

The independent contact model follows the surface-normal, ground-contact and velocity-clipping principles
illustrated by [Valve's Half-Life SDK movement reference](https://github.com/ValveSoftware/halflife/blob/master/pm_shared/pm_shared.c).
No SDK implementation is embedded; Warcraft heightfields, native pathing and model bounds are adapted locally.

## Selectable movement modes

`[Movement] Mode=classic` is now the default. It restores the movement behavior before the
17:08 cutoff on 25 October 2026: the 06:28 passage-collision build, commit 5578392.
This retains its round hull, static prop passage tests and existing movement tuning.
It restores center-only native deck support, the previous whole-triangle height test, native-only
bridge snapshots and no automatic/manual position recovery. The latest recovery implementation
remains available with `Mode=recovery`; it is not deleted or mixed into classic collision.

Save the installed INI and press F8 to switch. Unknown/empty/missing Mode values select classic.
Updates add the missing classic default and preserve an explicit recovery preference. Switching
back to classic discards pending Home requests and recovery history. Velocity, stance, position,
jump/gravity, acceleration and air-strafe settings remain unchanged. The native update itself
requires installation and restart once. The tests cover both modes and config reload/preservation.

## Platform support and emergency recovery preview

Everything in this section applies only with `Mode=recovery`.

Native decks now support the same center/eight rim foot samples as terrain and props. A landing
with only the leading foot over a platform catches its deck. Supported stationary platform meshes
remain in movement snapshots even when the native deck exists; a missed query no longer deletes
the physical surface. Combat retains the prior coarse bridge policy. A sloping triangle is clipped
to the reachable height before querying support, retaining its lower swept contact when its uphill
end is higher than the previous feet.

`RecoverInvalidPosition=true` repairs a proven below-terrain state only when the body fits above
the real floor. An established native-platform rider can recover onto its still-present deck if
its support rises beyond a step or was missed. Someone walking under a bridge has no rider provenance
and is never lifted onto it. Dead/missing platforms do not become cached invisible floors.

Home (`Shortcuts.RecoverPosition=HOME`, or NONE) requests recovery on the game thread. It checks
recently visited grounded points within RecoveryDistance, including an upper bank before a fall.
Every destination is checked again for its current support, native walkability and body/head clearance.
A destroyed bridge, occupied landing or changed height is rejected. Valid flat ground cannot use Home
as a horizontal teleport. Recovery clears fall/horizontal velocity while retaining stance and held
jump/tuning state. Cargo, roots/stuns, casting and cursor menus reject manual requests.

History is bounded to 32 sampled visited positions, reset at map/actor/view entry, native shop return,
cargo unloading and external native relocations. Automatic repair has an independent F8 opt-out;
manual Home remains available. Configuration migration adds missing keys and preserves custom values.
Install and restart to load this native correction.

Regression checks cover platform-edge landings, native-query dropout with a surviving mesh, destroyed
platforms, a rising deck versus an underpass, buried feet, low headroom, automatic opt-out, upper-bank
rescue, rejected stale destinations and preserved ordinary falls/hop metrics. Owned ElevatorPuzzle
and InvisiblePlatform decode to 202 and 2 triangles respectively; no retail model is distributed.
The reported Ancient One campaign traversal with the corrected DLL has not yet been driven in game.
