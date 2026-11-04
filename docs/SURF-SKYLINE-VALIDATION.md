# Surf Skyline verification

The original course was loaded in an isolated owned Warcraft III 1.26a runtime.
OpenGL and Direct3D each completed all three physical stages in default classic
mode, with ordered checkpoints and a recorded full-run result. The scripted driver
used ordinary forward/jump/A/D controls and manually returned to checkpoints
between stages; these are recovery-inclusive runs, not clean competitive times.
OpenGL recorded 23.90 seconds with three counted returns/falls; Direct3D recorded
23.87 seconds with two. Different command scheduling can change these times.

Both native runs also verified that stage-practice finishes cannot create a full
record, an out-of-order finish does not count, checkpoint returns keep the timer
running, and restarting clears the attempt while retaining the session best.
An automatic pit return was observed during course tuning. Imported MDX solids
were decoded through the ordinary model/collision path, including their rotation,
scale, actual top faces and undersides. OpenGL recording was visually inspected;
the Direct3D evidence is native state/physics rather than a recorded video.

All test modules, map replacements, INI, graphics bridge files and the complete
test save tree were backed up and restored with hash checks after the exact test
process exited. Tests did not run inside the user's campaign. Fixture commands are
compiled only in opt-in test modules and are excluded from the ordinary Player
launcher.

Portable movement checks run the production solvers on a synthetic steep triangle
surface at 30, 60 and 144 Hz in classic/hull/recovery. They verify sustained airborne
sliding and block an unreachable underside. Existing terrain, wall, cliff, crest,
bridge, narrow-passage and swimming regressions also pass. Separate obstacle
discovery checks retain actual eligible native-walkable faces, reject ordinary
flat boxes and preserve disabled-model behavior. Config checks cover startup/F8
snapshots, default insertion, opt-outs and preservation of saved custom choices.

The map builder's terrain limits, original file set, solid edge closure, normals,
route bounds/gaps, imports and classic fractional-literal limits have portable
regressions. Generated JASS was also checked against owned common.j declarations.
The outer downloadable ZIP contains only the authored map and instructions.

Real 1.27a gameplay and native hull/recovery course completion have not been
verified. The course requires the new model-surf build; the older terrain-only
practice launcher is insufficient. This is a local preview, with no change to the
published 0.18.0 release or its immutable tag.
