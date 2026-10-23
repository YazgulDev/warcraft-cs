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

<!-- launcher-summary
- Added experimental Warcraft III 1.27a and Direct3D 9 support alongside 1.26a.
- Added Player setup for both hosts using exact local Miles export forwarders without C++ Build Tools.
- Added optional Half-Life Crowbar, Tau Cannon, Gluon Gun and RPG in the B / 9 Half-Life category.
- Fixed Half-Life chrome skins, complete weapon animations, beam muzzle alignment, Tau charge feedback and varied Crowbar impacts.
- Changed default purchases to dollars with 800 starting money, 300 per mod weapon kill and a 16000 cap.
- Added configurable rare weapon drops with a 0.5 percent default chance and E pickup.
- Changed default RPG blast radius to 525 while retaining 100 peak damage.
- Preserved custom settings and prevented a local preview from offering its published base as a newer build.
-->
