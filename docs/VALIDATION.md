# Validation of the 0.1.0 source baseline

## 0.7.0 release consolidation (2026-10-10)

All completed feature tips are ancestors of the consolidated release; the old crosshair patch is
equivalent to the retained implementation and the older summary parser is superseded by the tested
English resolver. Merges retain their histories without reverting newer behavior. Source changes
are compared with the immutable v0.6.1 tag. Previous tags and release/feature branches remain intact.

The complete independent gameplay/window/logging wrapper, actual OpenGL reticle and sky tests,
production-XAudio2 silent gain test, launcher/update/reinstall/replacement regressions, Miles provider
checks and config migration in PowerShell 7/Windows PowerShell 5.1 pass. Native evidence below was
recorded with the same consolidated gameplay implementation in disposable maps; release changes
update version, notes and documentation. No full campaign or additional driver-specific claim is made.
Both launchers package the reviewed 0.7.0 English list and identical audited sources; publication
uses complete release notes with separate hidden launcher-summary metadata.

## Native campaign movie input recovery (test branch, 2026-10-10)

Two disposable maps queued the owner's IntroX movie through PlayCinematic/ChangeLevel, then loaded
a flat map containing one owned footman. The previous ordinary Player build reproduced the failure:
Warcraft destroyed its original game HWND for movie playback and created a different HWND afterwards;
F6 no longer reached the mod and FPS stayed inactive. The repaired ordinary build logged destruction
and attachment to the replacement render window. After skipping IntroX, F6 enabled FPS, V displayed
speed, W moved the unit, and screenshots showed the weapon and crosshair. Repeated F6 toggled RTS/FPS;
N suppressed fog and the normal exit restored the map's fog switches. No controller faults occurred.
Both test sessions closed through Warcraft's own exit dialogue. The real campaign, its progress and
the user's installed files were not used; complete movie playback and a full campaign run are not claimed.

Hidden Win32-window tests exercise native F6 forwarding before repair, consumption after repair,
same-HWND procedure resets, repeated observation, an external forwarding subclass, two windows with
different native procedures, late destruction, invalid/foreign HWND rejection and fresh attachment.
The numerical test wrapper includes this adapter check. Ordinary x86 runtime compilation passes.

## Optional fog override (test branch, 2026-10-10)

The actual owned 1.26a native registration table verifies FogEnable/FogMaskEnable `(B)V` and
IsFogEnabled/IsFogMaskEnabled `()B`. A disposable flat map started with both switches enabled.
The ordinary Player build removed fog and the black mask on N, ignored repeat/release packets,
restored fog/mask on F6, reapplied the session choice on re-entry, and restored them on the next N.
F8 applied true and false DisableFogOfWar preferences. Real screenshots and native transition logs
were inspected; the HUD no longer contains Sword or Melee RMB hints. The test game closed normally.

Independent adapter tests cover all four fog/mask baselines, no repeated writes, suppression after
map-script changes, exact restoration, fresh re-entry capture and map-unload snapshot invalidation.
INI validation covers missing/empty/invalid/case-insensitive/0/1 values and true/false reloads;
PS7/Windows PS5 migration retains custom preferences and inserts missing defaults without duplicates.
Only the private runtime/map/config was used. The user's installed game and saves were unchanged.

## Optional speed counter (test branch, 2026-10-10)

Real Warcraft III 1.26a on a disposable single-player map verified the ordinary Player build,
without a compiled movement oracle. The HUD omits WASD/reload/jump/crouch explanations. V shows
250 CS units/s during a normal USP run and hides the counter on the next press; Windows key-repeat
and release packets do not cause another toggle. V in RTS does not change FPS visibility. F8 applies
both true and false INI preferences; screenshots confirm the counter stacks above reload notices
without covering health/ammunition. The test game exited through its own confirmation dialog.

The real INI loader checks missing/empty/malformed values, case-insensitive booleans and 0/1, plus
repeated snapshot loads. Config migration checks fresh/legacy files, true/false custom values,
single Interface/key occurrences and idempotence. Only a private runtime/config/map was used;
the user's installed game and saves were not modified. The default remains ShowSpeed=false.

## Full launcher reinstall (test branch)

Manual Update selects an identical latest GitHub release for repair; startup checks remain quiet
when version and revision match. Both launcher variants describe full private-game reinstall.
Production file-copy tests repair damaged/missing core files, official maps and original audio;
retain progress, custom INI, private assets/maps and the installed proxy until mod setup completes;
and back up edited official content without duplicating unchanged backups on a repeat run.
Original/overlapping/unmarked/active runtimes and junction redirection are rejected before file writes.

The setup-runner integration uses current embedded repair code with synthetic legacy/modern releases
in Player/Developer mode. Game-version metadata is provided by a project-authored test DLL. Only
conversion/module installation is stubbed; production source extraction, Windows PowerShell launch,
request serialization and runtime repair execute. This verifies repair before the downloaded backend,
module/asset replacement and save/config preservation without an actual game or dependency downloads.

## Unreleased bunnyhop test branch (2026-10-09)

Movement, native collision, real INI parsing and migration checks pass. The numerical oracles cover
repeated/manual jumps, acceleration and speed caps, stationary takeoffs, frame-rate behavior, air
strafing, gravity-driven cliff falls, low props, tall/unknown blockers, full bridge footprints,
uphill takeoff/landing momentum, shallow-water support and root/stun/slow restrictions. Configuration
checks cover documented bounds/defaults, malformed/non-finite values, repeated reloads and preservation
of custom movement values during repeated updates in both PowerShell 7 and Windows PowerShell 5.1.
All existing independent gameplay suites also pass.

An explicit movement fixture in a disposable Lost Temple copy on Warcraft III 1.26a verifies five
native checks: five held-Space hops accelerate from 375 to 551 Warcraft units/s; a scaled barricade
stops a walking actor but is cleared by jumping; a native town hall blocks the attempted jump; and
walking off an actual cliff falls gradually from 128 to 17.2 world units and lands. Native model
transforms include the separate visual-scale getter. Terrain uses the verified interpolated height
backend; ordinary GetLocationZ support is preserved for shallow water. Game logs and captures were
inspected. Test requests and destructive fixtures are excluded from ordinary builds.

This is feature-branch verification, without a release/version change. No campaign save or complete
campaign was tested. Irregular/imported geometry and other maps still require playtesting; collisions
use model boxes, not triangle-perfect mesh contact. F8's existing production reload path applies the
movement snapshot; no new live F8 keyboard session is claimed.

## Original source baseline

Windows, native x86 Warcraft III 1.26a / Game.dll 1.26.0.6401, OpenGL, offline private runtime.
No game footage, game content, raw memory dumps or personal campaign saves are included here.

Seven C++ suites independently validate movement, look-angle wrapping, model-bound ray contact,
melee timing/range, friendly/C4/AWP damage policy, INI/ammo calculations and classic recoil.
The recoil oracles check published coefficients, burst timers and the known ran1 seed-one sequence.

Native private stock-map checks confirmed AK first kick:1/0.9/1.5 degrees standing/crouched/moving;
M4:0.65/0.6/1; USP/AWP:2, sustained AK vertical cap5.75 and lateral cap1.75.
O/H recruitment and J release were tested in both modes with real owned footmen and native orders.
No further formation orders followed release, and FPS remained enabled.
One harness wrongly expected exactly two recruits after moving closer to starting workers;
repositioning the fixture confirmed the two-footman oracle without changing ownership filtering.

Earlier native checks covered FPS/RTS attack acquisition, real rune healing and one-time ammo reward,
ordinary equipment/full Backpack handling, H/O/H combat restoration, destructable gate leaves,
hero-model hiding, fullscreen/raw mouse rotation and OpenGL context recreation after Alt-Tab.
Native Backpack rejects auto-use runes even after research; the mod preserves that rule.
Friendly damage coefficient is0.5; C4 base damage2500 is still processed by native Warcraft damage rules.

The normal installed baseline loaded a preserved night-elf campaign with Maiev650HP,
correct physical camera position, J/F8 and no private-fixture/controller-fault output.
All campaign missions, map triggers and every custom map have not been verified.
Fresh source-archive setup was also checked outside the original development layout:
it created a separate owned Warcraft runtime, installed NumPy in a private venv,
verified the pinned MinHook checksum, built MinHook and the mod with MSVC x86,
generated Miles forwarding code locally and converted the owner's six CS models.
No prebuilt dependency from the private research checkouts was required.

The resulting normal build started a real Lost Temple map with all native hooks
reporting successful creation/enabling, raw mouse input and FPS rendering active.
All seven viewmodels loaded, including the procedural sword fallback; J and F8
worked without a controller fault. This confirms standalone setup and a native
smoke check; it does not establish complete campaign/custom-map compatibility.

The complete staged tree passed the source allowlist/credential/binary/path audit.
Adversarial audit checks rejected fake credentials, renamed binaries, game assets,
private paths and decompiler headers. An early audit pattern matched its own pattern
text; anchoring the header check fixed that false positive before publication.

## 0.2.0 original sword and English README

The release converter always generates the project's original straight silver blade,
gold guard and leather grip. An existing private Grudge model is not selected or read.
External sword parameters/imports were removed from both setup and conversion.
An owned-CS conversion produced the original procedural sword cache successfully;
its rig/animation data and audio timeline come from the owner's knife as before.
The sword geometry itself is unchanged from the previously verified original sword.
Runtime C++ and combat behavior are unchanged in this update.

PowerShell/Python syntax checks and source-tree audits were run for the changed tooling.
The complete English README includes requirements, setup, controls, settings, limitations
and legal references. Linked relative documents remain in the release tree.
No fresh automated input was sent to the user's active game for this documentation/tooling update.

## 0.3.0 standalone Windows client launcher

The WinForms EXE was built with the Windows .NET Framework compiler. Its embedded ZIP
contains the audited project source, without game assets, downloaded dependencies or build output.
An offscreen preview verified the complete folder-selection, consent and progress layout.
UI tests confirm that valid folders alone cannot enable Install: explicit agreement is required,
and revoking agreement disables it again. Runner/extractor and PowerShell no-consent tests
reject installation before creating files. Path/JSON tests include archive traversal rejection
and protection against installing into the original game folders or a drive root.

The real pinned Python installer passed SHA256 and PSF signature verification, and the real
Microsoft bootstrapper passed Microsoft signature verification. Negative checksum/publisher
checks rejected untrusted downloads. Existing Python and Visual Studio/SDK were reused for
the complete installation tests; fresh vendor installation on a clean Windows machine has
not been tested.

A complete C# runner installation extracted its embedded source into a separate client folder,
created an owned Warcraft copy, installed NumPy in a private venv, verified/downloaded MinHook,
converted the owner's CS models and built the normal 372736-byte native mod. The selected
installation path included Cyrillic characters, spaces, an ampersand and an apostrophe.
The completion marker and Play configuration point inside that client folder.
This test exposed and verified fixes for inherited PowerShell Core module paths, Windows
PowerShell UTF-8 JSON decoding and Unicode paths in the native build command file.

The user's active Warcraft session was left running and received no automated input.
Play was checked through its installed configuration; a fresh game launch was not performed
for this installer feature. Native gameplay behavior is unchanged by the launcher.
All seven independent C++ logic suites and launcher regression checks passed again
when preparing 0.3.0. The release EXE reports assembly version 0.3.0.0 and includes
the English installation guide and source-only release tree.

## Miles provider setup repair

A user's first client launch reported "Unable to initialize base sound services".
The sound proxy/original DLL checksums and standalone device-open tests passed, but
the new private runtime lacked the owned `redist/miles` folder. The older working
development runtime included it. The installer had copied only selected top-level
files and Maps/Campaigns/Movies/AI Scripts, omitting the actual Miles codec/providers.

The warning reproduced in the affected client and an isolated ASCII-path copy,
including a control run with the proxy/mod disabled. Restoring `redist/miles`
from the owner's original installation removed the warning. The affected client
then launched with its normal mod: 43 CS clips initialized, and process-only capture
confirmed Warcraft menu music (RMS about 0.214, versus effectively zero before repair).
No sound-device/registry settings, original game files or saved progress were changed.

The shared audio setup helper runs for both fresh and existing marked runtimes.
Regression checks cover all five provider/codec fixtures, repeat installation,
legacy missing-folder repair, preservation of unrelated runtime files, missing-source
rejection before writes, and rejection of original/unmarked destinations.
Launcher payload/consent tests and Windows PowerShell audio checks passed.
Only owned source is embedded in the rebuilt EXE; the audio components remain local game data.

## 0.4.0 tree targeting, wheel input and requirements

All nine C++ regression suites pass. New synthetic MDX fixtures verify that rays beside
the trunk miss the old canopy box, trunk faces remain solid from either side, short
death stumps are excluded, nearer targets win and scaled rays retain world distance.
The owner's Lordaeron/Cityscape tree models each decode twelve trunk triangles.
Malformed geometry is rejected without falling back to the oversized tree box.

An isolated Lost Temple session on Warcraft III 1.26a verified six native geometry
checks: direct tree trunk hits, adjacent misses and preserved elf gate hits. Separate
wheel notches selected slots 7 -> 1 -> 7 -> 1 -> 2 -> 3 in the real FPS controller.
The screenshot showed the new wheel control hint and functioning weapon rendering.
The fixture was opt-in; ordinary builds exclude it. Existing INI values were unchanged.
No campaign/save was opened. Arbitrary imported tree models and animated model
variants still require map-specific verification.

REQUIREMENTS.md accompanies the source and distribution packages and documents
owned game files, dependencies, manual builds and the release updater contract.

The original 0.3.0 launcher assembly accepted the verified 0.4.0 package and completed
source installation, local dependency setup, owned-asset conversion and the ordinary
native build in a disposable client. Its save sentinel, custom INI values and remembered
private sword were preserved. Launcher regressions also passed release/version checks,
corrupt package rejection, archive/junction protection, running-game refusal and delayed
EXE replacement with backup/restart. The 0.4.0 UI preview displayed the correct version.
Publication lint reported 147 source files, zero failures and zero warnings.

## 0.5.0 launcher Player/Developer installation and update confirmation

The launcher embeds a separate native-runtime ZIP containing two project-built x86 modules,
their SHA256/source-version/Miles-ABI manifest and the project/MinHook/mechanics notices.
DLL import inspection found only inbox Windows libraries; no Visual Studio runtime DLL,
compiler, SDK or proprietary Warcraft library is distributed. Git and the source ZIP remain
source-only. Player needs Python/NumPy to convert the owner's game assets locally.

A real Player installation/update completed with compiler detection and Build Tools installation
replaced by failing test sentinels. Installed DLL bytes matched the embedded package; no MinHook
source was fetched. A fresh installation also succeeded directly from the EXE without an update
cache. Explicit Developer installation fetched pinned MinHook and rebuilt locally using installed
MSVC/SDK. The original updater-enabled 0.3.0 assembly also completed installation of this new
bundle through the verified update cache. Save sentinel bytes, custom INI values and remembered
private sword selection were preserved across the mode changes.

Launcher regressions cover consent for both mode buttons, update prompt confirmation/refusal,
legacy source-only release rejection in Player mode, manifest/package hashes and extraction
guards. Native-payload tests reject corruption, traversal and source/version mismatches.
Offscreen previews verified both installation buttons and Player/Developer update dialogs.
GitHub latest release lookup succeeded; rate-limit fallback manifest parsing preserves canonical
repository URLs and stable-version/checksum validation. No new campaign or interactive game
session was opened for these installer changes. Fresh vendor-tool installation on a clean Windows
machine remains unverified; Developer mode reused installed tools in the integration test.

## Configurable allied damage

`Damage.FriendlyFirePercent` defaults to the previous 50%, accepts fractional values,
and clamps to 0–100. Real INI tests cover missing keys, 0/25/100/12.5, out-of-range
values and malformed/NaN input. Damage oracles cover firearms, heavy melee, C4,
hero-mode attack scaling, enemy damage independence and allied AWP finishing protection.
Zero allied damage skips the native damage call to avoid damage-trigger side effects.
Bullets/melee use the reloaded settings snapshot at contact; C4 retains its planting snapshot.
No campaign or live match was used for these independent combat/settings checks.

## Dual launcher and separate Install/Update regression checks

Both UI variants pass folder and download-consent checks. The source-only form exposes
one Install button; the DLL-included form retains explicit Player/Developer installation.
Both expose a separate Update button. Tests cover variant-specific GitHub API and
rate-limit fallback selection, independent executable/runtime checksums, incomplete
variant manifests, missing assets, unexpected launcher names and invalid assemblies.
Existing extraction, running-game, audio setup, delayed replacement and backup/restart
regressions pass. These checks do not open a campaign or interact with the user's game.

## Configurable CS volume (0.6.1)

Real INI regression checks cover missing/empty/malformed/non-finite values, mute, fractional
percentages, 0-100 clamping and repeated edits/reloads of the same settings file. Setup migration
checks in PowerShell 7 and Windows PowerShell 5.1 add Audio defaults to old/empty configs,
preserve an existing CSVolumePercent=35 and other preferences, avoid duplicate sections/keys
and remain byte-identical on repeated setup.

The native CS audio test uses the production GameAudio and settings loader, actual XAudio2
and a project-authored silent PCM clip. Native master-voice readback verifies 50, 0, 12.5 and
100 percent while source voices are queued; mute/restore, defensive bounds and the uninitialized
audio path pass. No game assets, audible test tones, saves or Warcraft window are used.
The master bus preserves per-event CS gains and does not access Warcraft's Miles engine.
Startup and F8 both call ReloadSettings, which applies the validated snapshot to this bus.
All independent gameplay suites and the normal x86 native build also pass.

The configurable-game-settings and feature-release-workflow skills pass the skill validator,
are available in the repository and installed personal skills, and are adopted by AGENTS.md.
No new complete campaign, live Warcraft F8-input session or clean-machine dependency install
is claimed for this audio/settings change.

## Experimental Half-Life-style air strafing

Independent production-physics trials run for eight seconds at 30/60/144 FPS with alternating
A/D turns, no W after the run-up and no takeoff bonus. Speed rises from 375 to approximately
792/1055/960 Warcraft units/s. Straight held-Space hops preserve 375; disabling air acceleration
removes gain, and disabling BunnyHop removes excess speed on every takeoff. Manual jumping,
stationary jumps, root/stun/slow restrictions, optional custom boosts and live cap replacement pass.

Actual production swept collision in a synthetic native world covers low props, tall/unknown
blockers, bridge destruction/edges, hills, cliff falling and water support. A maximum-speed trial
approaches 3000 Warcraft units/s and stops before a thin wall even when the frame's destination
would be beyond it. Real INI loading feeds physics and supports changing a cap without resetting
the current vertical velocity. Migration preserves older custom bonuses and adds the zero default.

The opt-in native scene includes separate `strafe` and straight `hop` trials. This revision's
real-game verification remains pending because the user's Warcraft process is active; no campaign
or running module is replaced. Measurements above are independent physics results only.
