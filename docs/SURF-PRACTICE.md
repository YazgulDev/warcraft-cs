# Surf Practice

An original offline practice map for the Warcraft CS build with terrain surfing.
No additional launcher update is needed if that build is already installed.

Copy `WarcraftCS-Surf-Practice.w3m` to `Maps/Download` in the modded game, then select
**Single Player → Custom Game → Warcraft CS - Surf Practice**.
The selected invulnerable footman starts on the first green platform.
Press **F6** to enter FPS (the default binding), then **P** for the speed counter.

| Command in chat | Action |
| --- | --- |
| `-surf 1` | 55-degree practice lane |
| `-surf 2` | 60-degree practice lane |
| `-surf 3` | 65-degree practice lane |
| `-reset` | Return to the current lane's start |
| `-auto` | Toggle automatic return after finishing or leaving the lane; enabled initially |
| `-help` | Show Russian control hints |

Enter opens/submits the existing FPS chat. Run east from green ground toward the
striped slope. Try the first descent without jumping; Shift gives a slower entry.
Then experiment with jumping, A/D and mouse turns. The grey landing gives
space to brake. The ground is a heightfield with clear pathing, not a model-mesh ramp.
There are no enemies, victory/defeat triggers or destructible obstacles.

The map does not change the player's movement configuration. `[Surfing] Enabled=true`
is required; the latest surfing launcher enables it by default. Classic/hull/recovery
remain selectable in the mod. Existing controls and custom speed limits still apply.

## Rebuild

`python tools/build_surf_map.py --output <map.w3m>` requires only Python's standard
library and the authored JASS source. Optional `--angles 55 60 65` accepts three finite
48–75 degree angles; `--rise 1600` accepts 512–1800 Warcraft units. They are map-build
parameters, applied by rebuilding the file, not runtime/F8 physics settings. Heights
are checked against the classic terrain representation before writing.

All archive entries, script, placement records and the procedural overview are
generated from owned project source. Stock texture/unit identifiers are references
to resources loaded from the player's Warcraft installation. No retail map, extracted
asset or game DLL is read or bundled. The outer downloadable ZIP provides compression;
map entries use direct single-unit storage for the classic Storm reader.

Format references: [HiveWE map metadata](https://github.com/stijnherfst/HiveWE/wiki/war3map.w3i---Map-Info)
and [War3Net environment serialization](https://github.com/Drake53/War3Net/blob/master/src/War3Net.Build.Core/Serialization/Binary/Environment/MapEnvironment.cs).

Portable geometry/format checks: `python tests/SurfPracticeMapTests.py`.

The final map was checked on owned Warcraft III 1.26a in OpenGL and Direct3D. All
three lanes produced airborne surf contacts and a finish-to-start return; FPS chat
selected each lane and manually reset the actor to the 1700-unit start. The archive
was independently read back file by file, and JASS passed a checker using the owned
common.j declarations. Real 1.27a remains unverified.
