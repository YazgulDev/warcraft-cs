# Unified launcher preview after 0.11.0

This local feature build combines the previously separate 1.27a/D3D9, Half-Life
arsenal and dollar/loot branches. It retains base version 0.11.0 and is labelled
**preview**. It is not a new published release. Use **Install — Player** for the
embedded build; explicit **Update** selects the published GitHub release. Startup
still checks releases, but does not offer the preview's older published base as
an upgrade. A newer stable version can still be offered.

The DLL-included EXE accepts owned **Warcraft III 1.26a (1.26.0.6401)** and
**1.27a (1.27.0.52240)** installations. The shared mod contains both native tables;
native binding also checks 1.27a code signatures. Player setup fills a dedicated
data section in the project-owned sound proxy with the selected original library's
exact named, aliased, decorated and ordinal-only exports. It forwards calls to the
untouched local `WarcraftOriginalMss.dll`. This requires neither a 1.27a retail DLL
at package-build time nor C++ Build Tools at installation. No game library is
redistributed. Malformed inputs fail before either module is installed. Existing
legacy bundles retain their exact host/hash checks.

1.26a launches in OpenGL; 1.27a defaults to D3D9. **1.27a is an experimental partial
port, with no real-game validation yet.** Model/native hooks, minimap/portrait
capture, precise collision and several item/spell helpers have limits documented
in [PORT-1.27.md](PORT-1.27.md). Synthetic host fixtures and real hidden-device D3D9
pixels do not establish full campaign compatibility.

The Half-Life menu is **B → 9 → 1 Crowbar / 2 Tau Cannon / 3 Gluon Gun / 4 RPG**.
Carried weapons use group **6** and the wheel. Turn item-number mode off with Tab
to use weapon number keys. F12 grants available weapons; F11 refills owned ones.
Reinstall with the CS `cstrike` folder so setup converts sibling `valve` loose/PAK
models and sounds. Each weapon has a separate complete-resource seal. Missing
owned content hides only that weapon. **Only Warcraft modes** excludes all CS and
Half-Life weapons. Models, animation, chrome skins, held/world beam alignment,
Tau spin/pitch charge ramps and Crowbar flesh/hard/dirt impacts are included.
See [Half-Life controls and tuning](HALF-LIFE-ARSENAL.md) for prices, ammo and limits.

FPS hits now mute the controlled unit's native weapon contact sound by default while
retaining attack effects. `[Combat] NativeAttackSound=true` restores that sound on F8;
TPS native attacks keep their audio. The verified 1.26a adapter changes only the
synchronous contact and restores the original field afterwards. 1.27a continues to
use its existing direct-damage fallback without unverified attack-component writes.
The opt-out also removes the ordinary instant ranged attack art that can carry its
own arrow sound events, fixing the additional audio path left by the first contact
sound change. The two verified engine call sites are filtered only inside the
current FPS impact; nested attacks by other units and spell/passive art are preserved.
Save and close Warcraft before reinstalling this DLL update; F8 alone cannot load
a replacement module into an already running game.

Gluon's beam tracks aim every game frame, including between damage ticks, stays
bright while channeling and gains animated energy strands and a contact flare.
Setup optionally converts owned Half-Life `xbeam1.spr` and animated `XSpark1.spr`
effect textures; missing art falls back to procedural rendering. No retail art is
bundled. Reinstall to convert the new effects. `[HalfLife] BeamEffects=true`,
`OwnedSprites=true`, `ImpactSize=18` control presentation; flare radius is 0..100
world units and zero disables it. All three support F8; F8 cancels an active channel.

Tau's default damage is now **100 primary / 1000 fully charged**; Gluon deals
**70 per tick**, about 700/second at the default 0.1-second cadence. Native defenses
still apply. `DamageVersion=2` migrates the previous 20/200 and 14 stock values once;
custom damage is preserved, including later edits back to the former values.

The default dollar wallet starts with **$800**, pays **$300** for a hostile creature
killed by mod weapons/HE/C4/RPG, and caps at **$16000**. Native unit/spell/RTS kills
are outside the initial reward adapter. `[Economy] Currency=gold` restores native
gold purchases. The wallet is map-local; changing views or pressing F8 cannot
grant another starting balance. Warcraft map gold and native drops remain separate.

`[WeaponDrops] Enabled=true`, `ChancePercent=0.5` enables rare firearm drops; a
stock chest and nearby weapon name advertise E pickup. Duplicates supply ammo;
Tau/Gluon duplicates supply usable cells directly. Full ammo leaves the chest.
Disabling drops via F8 removes existing chests. Default lifetime is 120 simulation
seconds, range 160 world units, maximum 64 drops. All settings/ranges and save
behavior are in [ECONOMY-AND-LOOT.md](ECONOMY-AND-LOOT.md).

RPG peak damage remains **100**; blast radius becomes **525** (+150). Old stock
375 migrates once with `BlastVersion=2`; custom values survive updates. Existing
rockets keep launch settings. F8 reloads supported tuning. Owned optional assets
remain required for both purchase and free grants.

The integration adds production-path tests for adaptive native forwarders and
Player host installs, energy-cell loot, preview update policy and actual D3D9
Tau/Gluon beam pixels/occlusion. Existing regression and packaging checks cover
config preservation, ownership, native binding, source/resource integrity and
launcher reinstall. Report real-game testing separately from these checks.

Movement collision now uses a round player hull with standing/crouching head clearance and locally
sampled terrain normals. Repeated jumps cannot create takeoffs on a blocked cliff face; falling onto
one slides downhill. Flat ledge lips, native ramps, low props, retreat from existing overlap and tangent
wall sliding are covered by the regression suite. Clear lanes beside a known solid do not inherit its
overhanging native path cells. There is no distant-terrace height gate.
`[Movement] MaxWalkSlope=46` (15–75 degrees) and `HullRadius=24` (8–64 world units, a cap retaining smaller
native footprints) reload on F8 and preserve custom values on updates. Existing jump/gravity settings
are unchanged. Install and restart for the native code update; see [movement details and geometry limits](MOVEMENT.md).

Static non-tree props with a supported stationary Stand pose now use their actual transformed surfaces.
Arch openings, partial low-plinth landings and crouched headroom no longer inherit the complete model
box. Standing height remains 108, crouched height 66 and the default radius cap 24 world units.
`[Movement] ObjectMeshCollision=true` (true/false, F8 reload) enables this detail; false restores coarse
volumes. Setup preserves the preference. Trees, moving units, buildings, unsupported animated/material
formats and unreadable custom data retain their previous conservative checks; native decks stay authoritative.
Known actor radii remain valid with native pathing disabled. Almost-flat blocked cliff rims no longer
become false walls; continuous steep faces still cannot supply repeat takeoffs. Jump speed, gravity,
acceleration and air strafing are unchanged. Synthetic regressions and an owned static wall decode
passed; native campaign traversal with this updated DLL still needs verification after installation.

Platform support now covers the complete foot, and verified live platform meshes remain available
when a native deck query is missed. Sloping mesh support keeps the lower reachable part rather than
discarding the full triangle. `Movement.RecoverInvalidPosition=true` repairs proven buried states
and lost support for an established platform rider; ordinary falls, underpasses and destroyed-platform
falls remain possible. Home (`Shortcuts.RecoverPosition=HOME`, NONE disables) requests a revalidated
visited checkpoint within `Movement.RecoveryDistance=512` horizontal Warcraft units (64–2048).
Cargo, roots/stuns, casting and cursor menus reject manual rescue; native relocations clear history.
All three preferences reload on F8 and survive updates. Local physics, geometry and native-adapter
fixtures pass; the corrected DLL still needs verification in the reported Ancient One campaign fight.
Save and close Warcraft, install this preview and restart before trying the fix.

The default movement mode is now `[Movement] Mode=classic`, restoring the movement behavior from
the 25 October 2026 06:28 Moscow build (commit 5578392), before the requested 17:08 cutoff.
`Mode=recovery` preserves the complete later platform support, clipped mesh support and automatic/Home
recovery implementation. Classic restores the earlier native-only bridge policy, center deck probe
and whole-triangle height test and discards recovery requests/history. Jump, gravity, acceleration,
air strafing and custom tuning remain shared and unchanged. Mode values are case-insensitive;
missing/empty/unknown names use classic. Save the installed INI and press F8 to switch without
resetting current motion. Updates add a missing classic default and preserve explicit choices.
The Home/recovery sections above describe the optional recovery mode. Install and restart once
for the new native mode switch. Regression fixtures exercise both modes; native campaign behavior
after switching has not been driven in game.

<!-- launcher-summary
- Added experimental Warcraft III 1.27a and Direct3D 9 support alongside 1.26a.
- Added Player setup for both hosts using exact local Miles export forwarders without C++ Build Tools.
- Added optional Half-Life Crowbar, Tau Cannon, Gluon Gun and RPG in the B / 9 Half-Life category.
- Fixed Half-Life chrome skins, complete weapon animations, beam muzzle alignment, Tau charge feedback and varied Crowbar impacts.
- Fixed delayed Gluon beam aiming and added animated energy strands, impact glow and optional owned Half-Life effect textures.
- Changed default Tau damage to 100 / 1000 charged and Gluon damage to 70 per tick, preserving custom settings.
- Disabled native hero weapon contact sounds during FPS attacks by default while retaining native attack effects.
- Fixed residual ranged attack audio from native instant-hit art during FPS weapon contacts, preserving spell and passive effects.
- Changed default purchases to dollars with 800 starting money, 300 per mod weapon kill and a 16000 cap.
- Added configurable rare weapon drops with a 0.5 percent default chance and E pickup.
- Changed default RPG blast radius to 525 while retaining 100 peak damage.
- Preserved custom settings and prevented a local preview from offering its published base as a newer build.
- Improved local cliff collision, downhill sliding, round player hulls, narrow passages and crouching head clearance.
- Improved static prop collision so fitting arch openings stay passable and low parts of tall models can be jumped onto; preserved movement physics and conservative fallback.
- Fixed platform-edge landings, retained live platform surfaces during native query dropout and preserved reachable sloping mesh support.
- Added configurable invalid-position protection and Home recovery to revalidated nearby visited ground, preserving ordinary jumps, falls and underpasses.
- Changed default movement to the build before the 25 October 17:08 cutoff as classic mode; kept the latest recovery mode selectable on F8 without resetting motion.
-->
