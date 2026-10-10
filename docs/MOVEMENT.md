# Bunnyhop and airborne movement

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
Map bounds and unsupported flat pathing cells remain blocked. Unknown custom models cannot grant
permission to cross blocked pathing. Native cliff levels distinguish cliff faces from ordinary hills.
Shallow-water support retains Warcraft's native height rather than dropping onto the submerged mesh.

Edit `WarcraftCS/WarcraftCS.ini` in the prepared runtime, then press F8 in FPS. Updates add missing keys
without replacing custom values. F8 replaces tuning without resetting velocity or position; the next
simulation step applies the new speed cap. Jump speed applies to the next takeoff.

| `[Movement]` key | Default | Range and units |
| --- | --- | --- |
| BunnyHop | true | true/false; preserves excess takeoff momentum and enables the optional boost and air speed cap. false clips excess speed to the current weapon's running speed at each takeoff |
| AutoJump | true | true/false; false requires a new Space press per jump |
| JumpBoostPercent | 0 | 0–100 percent per moving takeoff; optional arcade bonus, separate from air strafe acceleration |
| MaxBunnySpeed | 1000 | 250–2000 GoldSrc units/s |
| AirAcceleration | 10 | 0–100 wish-direction acceleration multiplier |
| JumpSpeed | 268.328 | 1–800 GoldSrc units/s upwards |
| Gravity | 800 | 100–3000 GoldSrc units/s² downwards |
| StepHeight | 27 | 0–64 Warcraft units; larger downward steps fall |

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
Conservative model boxes can reject narrow passages or irregular props. Native walkable bridge decks
remain the surface authority rather than the box surrounding the complete bridge mesh.

Independent checks: `tools/test-movement.ps1`, `tools/test-warcraft-collision.ps1`,
`tools/test-gameplay-settings.ps1` and `tools/test-gameplay-config.ps1`.
The opt-in `tools/build.ps1 -TestMovement` includes `tests/MovementScene.cpp`, which accepts explicit
requests only for development in a disposable custom map copy. It must never run in campaign saves.
Ordinary builds exclude its scene, requests and destructive test fixtures.

For that explicit test build only, write `strafe x y yaw frames`, `hop x y yaw frames`, `prop x y yaw frames`,
`walkprop x y yaw frames`, `wall x y yaw frames` or `cliff x y yaw frames` to
`WarcraftCS/test-movement.request` inside the runtime. `flat` and `scan` print candidate corridors and
cliffs to the runtime log. The scene clears destructables around its trial corridor and creates paused
test actors; it does not save the map. `prop` and `walkprop` compare a scaled native barricade with and
without jumping, while `wall` attempts the same jump against a native town hall.
`strafe` runs up for one second and supplies alternating turns without W; it requires three takeoffs
and a peak above 450 Warcraft units/s. `hop` verifies retained straight-line speed without a bonus;
run both with `JumpBoostPercent=0`. Contact logs include speed in both coordinate systems and velocity.
