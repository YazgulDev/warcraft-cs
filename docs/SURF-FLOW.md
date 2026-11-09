# Connected Surf Flow

This original map is a continuous east/north/east time trial. Forty closed
banked ramp pieces meet four broad 1536-unit decks. Each ramp starts/ends with a
flat cross-section matching its neighboring deck; adjacent sections share the same
ridge and side heights. There are no mandatory launch gaps or checkpoint teleports.
The turning decks are physical connections where you can brake and change direction.
The previously published Skyline and Practice maps remain available separately.

Use the 0.20.0 Player launcher, then copy
`WarcraftCS-Surf-Flow.w3m` into the modded game's `Maps/Download` directory.
Select **Single Player → Custom Game → Warcraft CS - Surf Flow** and press **F6**
for FPS, **P** for speed. Start east along blue ramps, turn north at the first green
deck onto orange ramps, then turn east at the second checkpoint. Land on the gold
deck to finish. Ordinary movement across the decks connects the whole route;
`-cp` is a recovery command, not a required stage transition.

Run with W, then use A/D toward the ridge and smooth mouse turns on a steep side.
Brake on a green deck before turning toward the next section. The transitions flatten
near the decks, restoring ordinary ground controls. Surfing still obeys solid ramp
undersides, walls/headroom and the saved horizontal speed cap.

The shared timer starts on leaving the start. Checkpoints must be landed on in order.
Falls automatically return to the latest checkpoint; time continues. `-cp`/`-reset`
return manually, `-restart` begins a new run, `-stage 1/2/3` selects practice without
recording a full-course best, `-stats` shows results, `-hud` toggles the timer and
`-auto` toggles automatic recovery. Session records are not saved between map loads.

## Stronger surf response

`[Surfing] AccelerationMultiplier=2` doubles the contact-only air-input acceleration
and wish-direction speed allowance. Valid range is **0.1–4**; **1** restores the
0.19.0 response. Startup and **F8** apply it; updates insert a missing key and preserve
a saved custom value. It consumes the preceding confirmed surf contact once per
physics step. Repeated collision queries do not stack the gain. Ordinary free-air
Bunnyhop uses its existing tuning; gravity, jump height and the 3000-CS-unit stock
speed cap remain independent. No keys means no extra input acceleration.

The map does not overwrite movement preferences. Model surfing requires
`[Surfing] Enabled=true`, `ModelContacts=true` and `[Movement] ObjectMeshCollision=true`.
The stronger response needs the 0.20.0 launcher; the map geometry itself
uses the 0.19.0 model-surf path.

## Original source

`python tools/build_surf_flow.py --output <map.w3m>` builds the map from authored
Python/JASS/MDX/TGA definitions, without retail map/art inputs. Optional
`--script-output <script.j>` writes its generated shared timer script. Source modules
separate route layout, closed model geometry and archive assembly. Chunk origins stay
within native local collision-discovery range. `tools/test-surf-map.ps1` checks shared
seams/deck endpoints, closed solids, native terrain encoding, imports and timer rules.
No game DLL, extracted model/texture or campaign save is distributed.

## Verification

Owned isolated Warcraft III 1.26a completed the final connected route in default classic
mode on OpenGL (33.25 seconds) and Direct3D (33.31 seconds). Both runs reached each
ordered checkpoint and the finish with zero falls, zero checkpoint returns and no
position/teleport fixture commands. The driver used ordinary movement across both
turning decks and confirmed steep surf contacts in every leg. These are scripted
verification times, not competitive benchmarks. OpenGL recording frames were inspected;
Direct3D evidence is native state/physics. Lab files and complete saves were restored
by hash after each owned process exited. Real 1.27a and full native hull/recovery course
completion remain unverified.

Portable checks cover continuous bank/deck seams, closed solids, descending profiles
across the full width (including the flattening tail), model discovery bounds and shared
timer rules. Physics verifies stronger tangent response at 30/60/144 Hz, unchanged ordinary
free-air steering, no gain stacking from repeated face queries, existing speed limits,
walls, cliffs, narrow passages, bridges and swimming. Config tests cover malformed/finite
bounds, startup/F8 parsing and preservation/insertion during repeated updates.
