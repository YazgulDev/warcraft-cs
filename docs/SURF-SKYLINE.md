# Surf Skyline

An original single-player surf time trial for Warcraft CS. [Download map and instructions](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.19.0/WarcraftCS-Surf-Skyline.zip). Three stages contain 18
solid A-frame rail sections, two turns, two air gaps and two ordered checkpoint pads.
The long gold finish deck gives room to brake. Models, checker textures, overview,
placement and map logic are authored project content, not an imported CS map.

Install the 0.19.0 (or newer) Player launcher, then copy
`WarcraftCS-Surf-Skyline.w3m` into the modded game's `Maps/Download` folder.
Choose **Single Player → Custom Game → Warcraft CS - Surf Skyline**.
Press **F6** for FPS and **P** for speed. Default classic physics is supported;
hull/recovery remain selectable. The older terrain-only surfing build cannot ride
these model faces. `[Surfing] Enabled=true`, `ModelContacts=true` and
`[Movement] ObjectMeshCollision=true` are required; no special speed override is used.

Run east from the starting green pad with W and jump toward a blue ramp's side.
Once on the steep face, use A/D toward its ridge and smooth mouse turns; holding W
throughout is not necessary. Stay on the side rather than walking on the ridge.
Checkpoint 1 turns north along orange ramps; checkpoint 2 turns east toward gaps.
Use the arrows/labels and minimap to orient yourself after a return. A jump requires
walkable support, so Space on a steep face does not create another ground jump.

The timer starts on leaving the starting platform. Land on both green checkpoint
pads in order, then on the gold finish. Flying past a pad above it does not count.
Falling into the pit automatically returns you to the last checkpoint. Time keeps
running through falls and manual checkpoint returns. The fastest full completion
is remembered for the current map session; it is not a persistent or online record.

| FPS chat command (Enter to open/submit) | Action |
| --- | --- |
| `-cp` or `-reset` | Return to the current checkpoint; time continues |
| `-restart` | Start a new full attempt; retain the session best |
| `-stage 1`, `-stage 2`, `-stage 3` | Practise an individual stage; cannot update the full-course record |
| `-stats` | Show time, checkpoint, falls, attempts and session best |
| `-hud` | Toggle the following world-space timer display |
| `-auto` | Toggle automatic pit recovery; enabled by default |
| `-help` | Show Russian instructions |

The course does not modify saved movement tuning. Automatic recovery and the timer
display are map-session controls. There are no enemies or defeat triggers; the
footman and the course objects are invulnerable. This is an offline Warcraft course
inspired by CS surfing, not a byte-for-byte GoldSrc movement implementation.

## Rebuild and checks

`python tools/build_surf_course.py --output <map.w3m>` needs only Python's standard
library and the owned JASS source. `--script-output <script.j>` writes the generated
script for an optional external checker using the owner's common.j declarations.
The builder does not read retail maps, MPQs or extracted assets. Stock unit/terrain
identifiers are references loaded from the player's own game. Map entries use
uncompressed single-unit storage compatible with classic Storm; the downloadable
outer ZIP supplies compression.

`tools/test-surf-map.ps1` checks both original maps, terrain height bounds, closed
solid models, real steep normals, route bounds/gaps, import references, reproducible
archives and the classic JASS fractional-literal limit. Collision regressions cover
model sliding at 30/60/144 Hz in classic/hull/recovery, including solid underside
blocking. Native verification details and limits are recorded in
[validation notes](SURF-SKYLINE-VALIDATION.md).
