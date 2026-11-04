# Surface swimming

[Project home](../README.md) · [Movement](MOVEMENT.md) · [Settings](CONFIGURATION.md)

Release 0.18.0 automatically swims in deep native water while using direct FPS,
unit-view or third-person control. The body settles partly below the liquid plane and the
physical aiming/camera eye rests 12 Warcraft units above it. WASD paddles at 65% of the current
weapon/unit speed, including native slows and haste. Space does not produce ground jumps or
bunnyhop acceleration while floating. A fresh Space while moving toward a nearby bank launches
one exit jump, so a low canal edge does not trap the swimmer. It cannot climb arbitrarily high
walls or bypass the existing solid sweeps. Shallow water uses the bed and returns to walking; an
accessible bank still obeys ordinary step, solid and cliff collision.

All three movement modes (`classic`, `hull`, `recovery`) share the liquid contact but retain
their own solid sweeps. Walking on a real bridge remains walking, even above deep water.
Water under an overhead deck does not become the deck's walking surface. Recovery does not
misclassify a valid swimmer as a fallen bridge rider. There are no concrete footsteps in water.
Actors with native flight height retain their existing movement instead of being pulled into swimming.

The native liquid query is independently signature-checked on **1.26a x86**. Unsupported or
modified helpers omit swimming; experimental 1.27a has no verified liquid adapter and retains
its existing movement. This is surface swimming; underwater diving and drowning are not added.
Third-person models use authored Swim/Walk Swim/Stand Swim clips when available, otherwise
their own walk/stand clips. The feature does not generate a new swimming skeleton for every unit.

## Configuration

Use `[Swimming]` in `WarcraftCS.ini`; all values reload with **F8**. Setup inserts missing keys
and preserves existing custom values and explicit opt-outs. `Enabled=false` restores the previous
water-support behavior. Dry ground acceleration, jump height and gravity remain unchanged.

| Key | Default | Range / units |
| --- | --- | --- |
| Enabled | true | true / false |
| MinimumDepth | 96 | 32–512 Warcraft units; small exit hysteresis prevents boundary flicker |
| SpeedPercent | 65 | 10–100% of the current weapon/unit speed |
| Acceleration | 6 | 1–30 wish-direction acceleration multiplier |
| Drag | 4 | 0–20 momentum damping per second |
| FloatSeconds | 0.2 | 0.05–2 seconds; buoyancy response time |
| EyeAboveWater | 12 | 0–48 Warcraft units above the liquid surface |
| ExitHeight | 128 | 0–192 Warcraft units; maximum exit apex above water, 0 disables exit jumps |
| ExitReach | 128 | 32–192 Warcraft units; nearby bank detection distance in the movement direction |

The actual water flag/plane, terrain bed and bridge deck are queried separately. A native
walking restriction is relaxed only on real liquid that permits native floating; it cannot
turn an unknown dry blocker, building or tall bank into a passage.
