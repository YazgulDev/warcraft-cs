# Warcraft CS 0.7.0

Released 2026-10-10. All changes since the immutable **v0.6.1** tag are included below:
Half-Life-style movement, optional speed/fog controls, campaign-movie and crosshair fixes,
diagnostics, full launcher reinstall and compact English update changes.

## Bunnyhop and airborne traversal

Run up with W, jump, release W, then alternate A while turning the mouse left and D while
turning right. Hold Space to jump again on landing. Air strafes build horizontal speed beyond
the current weapon's running speed; straight hops retain the run-up. Autojump handles takeoff
timing only: the player still supplies the strafes and mouse turns. Releasing Space allows
ground friction to slow the unit. Air acceleration applies on the first takeoff frame and
momentum survives landings. Roots, stuns and slows retain their movement restrictions.

Jump over low objects when the feet clear their transformed collision bounds, land on their
tops, or continue past them. Tall walls, buildings and trees remain solid. Walking or jumping
off a cliff retains the current height initially and falls under gravity until landing.
No fall damage is added. Collision sweeps short segments at high speed, follows native bridge
decks and shallow-water support, and conservatively blocks unknown custom-model/pathing cells.

Edit the private `Game/WarcraftCS/WarcraftCS.ini` (source installations: the prepared runtime's
`WarcraftCS/WarcraftCS.ini`) and press **F8 in FPS**. Movement reload preserves position and
vertical velocity; the next simulation step applies the cap, and the next takeoff uses jump speed.

| `[Movement]` key | Default | Supported value |
| --- | --- | --- |
| BunnyHop | true | true/false; false clips excess speed to weapon running speed on takeoff |
| AutoJump | true | true/false; false requires a fresh Space press for each jump |
| JumpBoostPercent | 0 | 0–100 percent optional moving-takeoff bonus; Shift/Ctrl suppress it |
| MaxBunnySpeed | 1000 | 250–2000 CS/GoldSrc units/s horizontal cap |
| AirAcceleration | 10 | 0–100 wish-direction multiplier; zero disables air-strafe gain |
| JumpSpeed | 268.328 | 1–800 CS units/s upwards |
| Gravity | 800 | 100–3000 CS units/s² downwards |
| StepHeight | 27 | 0–64 Warcraft units; larger drops fall |

One CS unit corresponds to 1.5 Warcraft units. Default jump height is about 67.5 Warcraft units;
the default horizontal cap is 1500 Warcraft units/s, with a supported maximum of 3000.
Numeric values are validated and clamped; missing/empty/malformed/non-finite numbers use defaults.
This is a Warcraft adaptation with an editable cap, rather than an exact GoldSrc engine port;
air-strafe gain depends on input timing and simulation step. Irregular props and narrow passages
can be conservatively blocked by model boxes.

Details and development-only scene controls:
[Movement settings](https://github.com/YazgulDev/warcraft-cs/blob/v0.7.0/docs/MOVEMENT.md).

## Speed display, fog and controls

- **V** shows/hides horizontal speed in CS units/s, using the same units as MaxBunnySpeed and
  excluding vertical velocity. `[Interface] ShowSpeed=false` is the startup/F8 default.
- **N** disables/restores explored fog and the unexplored black mask in offline FPS.
  `[Interface] DisableFogOfWar=false` is the startup/F8 default. Each map's independent fog/mask
  switches are restored on RTS exit, menus and cinematics; unload discards the old map snapshot.
  The override is reasserted if map scripts enable fog while FPS suppression is active.
- Both preferences accept true/false or 1/0. Key toggles change the session only and do not write
  the INI; F8 reloads the saved preference. Repeated Windows keydown packets do not toggle again.
- Remove permanent WASD/reload/jump/crouch explanations and Sword/Melee RMB HUD hints.
  Existing controls, including slot 7 and right-button melee, remain available.
- Native campaign movies can destroy the game window and create a replacement. The mod now
  reconnects its input handler to the actual render window, restoring F6 and other controls
  after playback. Known native procedure resets are repaired; separate live windows retain
  their own native forwarding procedures and external forwarding subclasses are preserved.

## Crosshair and AWP damage

Hip-fire and scoped reticles use filled geometry while preserving their center, recoil gap,
thickness and inherited OpenGL state. Hidden-window pixel tests cover hostile inherited polygon,
stipple and color-mask states. The individual AMD-driver missing-crosshair report was not
reproduced on that user's machine, so these checks do not establish its exact driver-specific cause.

`[Damage] AWPOneShot=false` is the new default. AWP uses ordinary configured damage (115 base
before native armor). Only explicit true/1 enables enemy instant kills in weapon-damage mode;
missing, empty and invalid values use false. Allied damage retains its configurable coefficient.
F8 applies damage changes. An existing custom AWPOneShot=true is preserved during updates.

## Diagnostics and Read logs

Both launchers add **Read logs**, available before installation and while setup/gameplay is running.
Choose the journal, refresh manually or every two seconds, copy the displayed tail, or export the
complete current file. The viewer displays a bounded 512 KiB tail and reads shared active files.
It discovers game/session archives, installer output, launcher logs and replacement journals.

Runtime records include UTC time, severity, process/thread IDs, version/source fingerprint,
Windows/OpenGL information, hooks, map/window/FPS lifecycle, settings, controls, combat, reload,
items, buying, squads, audio, movement and faults. Takeoff/landing records include horizontal
speed and velocity. Crosshair begin/state/end/skip records include the render context, viewport,
color and stipple/polygon/depth/blend state; sampled GL errors distinguish errors already present
in the host frame from draw/restore errors. Submission records do not prove visible GPU pixels.
Raw character text and mouse-motion packets are not logged. Repeated frame records are sampled.

| `[Logging]` key | Default | Supported value |
| --- | --- | --- |
| Detailed | true | true/false or 1/0; false retains essential events/errors |
| IntervalMs | 1000 | 100–60000 ms between repeated movement/render summaries |
| MaxFileMB | 8 | 1–64 MiB per runtime journal |
| ArchiveCount | 3 | 0–8 previous sessions/segments |

Startup/F8 applies these settings. Defaults retain at most 32 MiB of runtime journals;
launcher journals use 4 MiB each with three archives. Writes are synchronized and oversized
records are marked as truncated. Local export is a player action. See
[Diagnostics](https://github.com/YazgulDev/warcraft-cs/blob/v0.7.0/docs/DIAGNOSTICS.md).

## Launcher updates and installation repair

**Install** uses the version embedded in the EXE. **Update** first fetches GitHub's latest stable
release and reinstalls it even when version/revision already match. Startup checks remain quiet
when no update is available. Both paths recopy owned Warcraft files, repair missing/corrupt game
resources, regenerate privately converted CS assets and reinstall/build the selected mod modules.

Saves, custom INI values, Player/Developer mode, launcher variant, private sword preferences and
private-only maps/content are preserved. Edited original map/campaign filenames are backed up
under `backups/runtime-content/<id>/` before replacement. The current setup runner also repairs
older downloaded packages whose installer would otherwise skip an existing Game folder.
Original, overlapping, unmarked or active game installations and unsafe junction redirection
are rejected. Download/dependency consent and running-game protection remain required.

Both update dialogs show a compact **English** change list, one item per line. Reviewed lists
are embedded by exact version, included in the manifest and supplied separately from full public
GitHub notes. API and rate-limit fallback paths retain the list; full Markdown/configuration
prose no longer replaces it. The builder validates the list and both packaged EXEs carry it.
Historical 0.6.1 summaries describe only that release's actual audio/settings changes.

## Documentation, source and packages

Update the main README, folder/first-launch instructions, requirements, troubleshooting, support
guidance and Q&A issue form; document movement, logging, launcher lists, module ownership and
validation. Publication checks narrowly allow the owned support form and release summaries.
Retain all completed feature and prior release branches; merge through release/0.6.1 and master,
then create release/0.7.0. Published tags and historical full Release Notes remain unchanged.

New platform adapters own window input, diagnostics, native fog and model-based movement obstacles.
The input mailbox/controller/presentation boundaries own V/N controls and display preferences.
Ordinary native builds exclude development scenes and request-based destructive fixtures.
Source-only commits and public ZIPs include no proprietary game files, extracted assets or saves.

| Download | Installation |
| --- | --- |
| WarcraftCSLauncher.exe | Source-only Install builds locally with C++ Build Tools / Windows SDK |
| WarcraftCSLauncher_DLL_Included.exe | Player installs ready project modules; Developer builds locally |
| WarcraftCS-0.7.0-dist.zip | Source-only launcher/checksum, exact sources, requirements and manifest |
| WarcraftCS-sources.zip | Exact audited source snapshot embedded in both EXEs |

## Upgrade and verification scope

Save progress, close Warcraft and select the existing client destination before **Update**.
For an older updater unable to select the DLL-included variant, download that EXE and choose
**Install — Player** with the existing destination. Original game installs are not changed.
Missing config keys are inserted without replacing custom values or adding duplicate sections.
Older test configs with JumpBoostPercent=8 retain it: set it to **0** and press F8 for strafe-only gain.
Set MaxBunnySpeed=2000 for the optional maximum; the shipped default remains 1000.

Checks cover movement/collision, real INI loading, migration in PowerShell 7 and Windows PowerShell 5.1,
native hidden-window forwarding, actual OpenGL reticle/sky pixels and state, bounded concurrent logs,
silent production-XAudio2 gain, launcher consent/update/reinstall/replacement and both package variants.
Both ordinary x86 modules are rebuilt and sources/packages audited before publication.

Disposable native maps verified real air-strafe gain, straight-hop retention, the V speed HUD,
N fog restoration and F8 preferences. A separately configured 2000-cap development scene reached
2000 CS units/s with ideal automated strafes after about 74 simulation seconds; this is not a
manual-input timing guarantee. A real IntroX transition reproduced the old lost-F6 failure;
after skipping the movie, the ordinary repaired build restored FPS, crosshair, movement, V and N.
Full natural movie playback, the complete campaigns, all imported map models, clean-machine tool
installation and the reported AMD-driver environment have not all been verified.

Requirements remain Windows x64, owned Warcraft III **1.26a x86 / Game.dll 1.26.0.6401**, owned
CS 1.6 and OpenGL. Offline single player only; Reforged and multiplayer remain unsupported.
Project code/licenses and MinHook/mechanics notices are included. Unsigned native hooks may
trigger heuristic antivirus detections; no new vendor verdict is claimed. See
[Troubleshooting](https://github.com/YazgulDev/warcraft-cs/blob/v0.7.0/TROUBLESHOOTING.md).

<!-- launcher-summary
- Added Half-Life-style air strafing and accelerating bunnyhops.
- Added jumping over low objects and gravity-driven cliff falls.
- Added configurable movement settings and held-Space autojump.
- Added V to show or hide your speed in CS units per second.
- Added N to disable or restore fog of war in offline FPS.
- Fixed F6 and mod controls after campaign movies.
- Improved hip-fire and sniper crosshair drawing.
- Added detailed gameplay, movement and crosshair diagnostics.
- Added Read logs with refresh, copy and export in both launchers.
- Disabled AWP instant kills by default; normal damage is configurable.
- Fixed Update to reinstall the latest GitHub release and repair game files.
- Preserved saves, custom settings and private content during reinstalls.
- Fixed compact English update changes in both launchers.
- Removed basic movement, reload, jump, crouch and melee HUD hints.
- Updated installation, troubleshooting and support documentation.
-->
