# Experimental movement

This feature is developed on `feature/add-bunnyhop-and-airborne-traversal`, based on release 0.6.1.
It has not been merged or published as a release.

Move with WASD and hold Space to chain jumps. Moving takeoffs skip friction and add horizontal speed;
air strafing retains momentum. Release Space or stop jumping to let ground friction brake.
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
| BunnyHop | true | true/false; enables takeoff boost and air speed cap |
| AutoJump | true | true/false; false requires a new Space press per jump |
| JumpBoostPercent | 8 | 0–100 percent per moving takeoff |
| MaxBunnySpeed | 1000 | 250–2000 GoldSrc units/s |
| AirAcceleration | 10 | 0–100 wish-direction acceleration multiplier |
| JumpSpeed | 268.328 | 1–800 GoldSrc units/s upwards |
| Gravity | 800 | 100–3000 GoldSrc units/s² downwards |
| StepHeight | 27 | 0–64 Warcraft units; larger downward steps fall |

One GoldSrc unit corresponds to 1.5 Warcraft units. The default jump rises about 67.5 Warcraft units.
Finite out-of-range numbers are clamped; missing/empty/malformed/non-finite values use the defaults.
Boolean values accept true/false case-insensitively; invalid values use their default.

The boost is a configurable gameplay adaptation inspired by Half-Life, not an exact GoldSrc engine port.
Conservative model boxes can reject narrow passages or irregular props. Native walkable bridge decks
remain the surface authority rather than the box surrounding the complete bridge mesh.

Independent checks: `tools/test-movement.ps1`, `tools/test-warcraft-collision.ps1`,
`tools/test-gameplay-settings.ps1` and `tools/test-gameplay-config.ps1`.
The opt-in `tools/build.ps1 -TestMovement` includes `tests/MovementScene.cpp`, which accepts explicit
requests only for development in a disposable custom map copy. It must never run in campaign saves.
Ordinary builds exclude its scene, requests and destructive test fixtures.

For that explicit test build only, write `hop x y yaw frames`, `prop x y yaw frames`,
`walkprop x y yaw frames`, `wall x y yaw frames` or `cliff x y yaw frames` to
`WarcraftCS/test-movement.request` inside the runtime. `flat` and `scan` print candidate corridors and
cliffs to the runtime log. The scene clears destructables around its trial corridor and creates paused
test actors; it does not save the map. `prop` and `walkprop` compare a scaled native barricade with and
without jumping, while `wall` attempts the same jump against a native town hall.
