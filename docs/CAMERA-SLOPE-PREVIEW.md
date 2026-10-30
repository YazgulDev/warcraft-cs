# Slope camera correction

Included in [release 0.15.0](RELEASE-0.15.0.md). The earlier local verification is retained below.

The verified Warcraft III 1.26a camera helper adds its own averaged terrain height
after the FPS controller has already supplied the physical eye position. On uneven
ground this introduces a second, independently changing height into the view.
The FPS camera now removes that native contribution during its own position writes
and deferred render updates. Walking, collision, support height and jumping are unchanged.

The correction requires verified native camera signatures and a successfully installed
hook. RTS, cinematics yielded by the controller and unrelated cameras retain native
terrain handling. Unverified layouts, including experimental 1.27a, retain the previous
public camera API fallback. There is no new setting or configuration migration.

## Verification

An isolated Warcraft III 1.26a OpenGL game ran six deterministic cases on a disposable
map: flat ground, a level eye crossing a slope, a ground-following eye on the slope,
standing still, physical walking and physical jumping. The level-eye case previously
drifted by up to 1.0854 world units; the corrected run had zero drift. Walking and jumping
followed the submitted physical eye within 0.005 world units. Native camera getters expose
the preceding submission, so moving-case assertions compare against that submission
rather than treating the existing frame delay as a height defect.

The scene is compiled only with `tools/build.ps1 -TestMovement`. Ordinary launcher
modules contain neither scene commands nor request-file handling. Synthetic regressions
also cover pitched/yawed camera edges, repeated native render clamps, RTS restoration,
unrelated cameras and rejected signatures. Movement and collision suites pass unchanged.
The user's campaign and saves were not used. No Warcraft 1.27a gameplay verification
or claim that all sources of frame stutter are eliminated is made.

## Installing

Save and close Warcraft, install/update using this preview launcher, then restart the game.
F8 cannot replace a loaded native module. Existing settings and movement mode are retained.
The startup journal reports `FPS native terrain camera normalization installed=1` when
the verified correction is active; see [diagnostic logging](DIAGNOSTICS.md).

<!-- launcher-summary
- Fixed extra native terrain height disturbing the FPS camera on slopes in verified Warcraft III 1.26a.
- Preserved movement, collision, jumping, native RTS cameras and the fallback on unverified hosts.
- Added isolated native slope, walking and jumping regressions without including test scenes in the player module.
-->
