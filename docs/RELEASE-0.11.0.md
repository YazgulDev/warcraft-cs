# Warcraft CS 0.11.0

Released 2026-10-20. These notes cover **all changes since immutable v0.10.0**:
the expanded arsenal, carried weapon selector, native shops, army assistant,
CS grenades, optional Half-Life RPG, presentation, collision/camera and HUD fixes.
The existing F6/F7 unit HUD, Tab/I inventory and Y squad orders remain available.

## Download and upgrade

- [WarcraftCSLauncher_DLL_Included.exe](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.11.0/WarcraftCSLauncher_DLL_Included.exe): Player modules, without local C++ compilation.
- [WarcraftCSLauncher.exe](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.11.0/WarcraftCSLauncher.exe): Developer/source install with consented dependency preparation.
- `WarcraftCS-0.11.0-dist.zip`: standard launcher, sources, update manifest, requirements and checksum. Source/public ZIPs contain no DLL/MIX modules; the bundled EXE is a separate asset.

Use **Update from GitHub**, or **Install** from the new launcher into your existing
separate client directory. Save and close Warcraft before updating, then restart it;
a running process retains its old module. Setup preserves saves, private settings,
custom controls/prices and profile choices and inserts missing INI keys. Regenerate
private CS caches to obtain the added models, barrel attachments, grenade world
models and reload sounds; incomplete CS distributions now identify missing required
models during preflight. Either `cstrike` or its Half-Life parent can be selected.

Supported host: offline Warcraft III 1.26a x86, Game.dll **1.26.0.6401**. The full
weapon profile needs owned CS 1.6 files. **Only Warcraft** excludes CS weapons and
conversion: use F7 for native third person or the deliberately clean F9 view.
Uncheck that profile for armed F6. Player requires a matching owned Miles library;
if its fingerprint differs, use the ordinary Developer launcher to generate matching
forwarders locally. No game DLLs, models, sounds, maps or saves are redistributed.
Reforged, other patches and online play remain unsupported. See
[installation](https://github.com/YazgulDev/warcraft-cs/blob/v0.11.0/INSTALL.md) and
[requirements](https://github.com/YazgulDev/warcraft-cs/blob/v0.11.0/REQUIREMENTS.md).

All new gameplay keys use the existing startup/F8 settings snapshot. Existing live
reloads/projectiles retain their documented start/launch policy; reloading settings
does not grant ammunition. Custom values survive upgrades. Tab still starts with
item numbers enabled: switch it off for category number keys; the wheel works in
either item-number state. Y remains the F6 RMB-orders toggle; U is the new AI key.
A saved action already using U wins and the newly inserted AI binding becomes NONE.

## Expanded arsenal, presentation, grenades, shops and AI

The new paid firearms are Desert Eagle (650 gold), Glock 18 (400), MP5 (1500),
P90 (2350), M249 (5750), M3 (1700), and XM1014 (3000). Open **B**: 1 pistols,
2 shotguns, 3 SMGs, 4 rifles, 5 machine guns, 6 primary ammo, 7 pistol ammo,
8 equipment. Number keys and mouse rows select the same catalog entries.
Buying grants a loaded magazine; reserve packs cost extra. Duplicates, insufficient
funds and full packs do not charge gold. Inventory slot IDs remain stable. The
[carried weapon menu](https://github.com/YazgulDev/warcraft-cs/blob/v0.11.0/docs/WEAPON-SELECTION.md) uses groups **1–5**, repeated category keys or the wheel
to highlight a weapon, then LMB to equip it. Tab toggles item-number routing.
F11 refills owned weapons; F12 grants all weapons. The initial kit is unchanged.
Models, sounds and hand-free previews are converted privately from owned CS files.
An optional [Half-Life RPG](https://github.com/YazgulDev/warcraft-cs/blob/v0.11.0/docs/HALF-LIFE-RPG.md) appears only with complete owned resources.

The new INI sections `[Deagle]`, `[Glock]`, `[MP5]`, `[P90]`, `[M249]`, `[M3]`,
`[XM1014]` expose `Price`/`AmmoPrice` in native gold (0–1000000), `AmmoPack`
(1–1000 rounds), `MaxReserve` (0–100000), `Magazine` (1–1000), `Damage`
(0–1000000, weapon mode) and `HeroMultiplier` (0–1000, hero mode). Shotgun damage
is a shell total, divided across `[Shotguns] Pellets=8` (1–16), with
`SpreadDegrees=5` cone radius (0–15 degrees). Targets intercept pellets independently.

`FireInterval` is seconds between shots (.025–10), `ReloadSeconds` seconds per full-magazine
reload (.1–30), `Range` world units (100–50000), `MoveSpeed` CS units/second
(50–500), `ModelScale` viewmodel projection scale (.25–2), `Kick` vertical impulse
(0–10 degrees, new firearms). F8 reloads settings; tighter capacities clamp surplus
without granting ammo. Existing reloads finish their current deadline; the next
reload uses the new duration. M3/XM1014 calibrate start, insertion and finish speed
against a completely empty magazine. Partial reloads keep the same insertion speed
and finish sooner, with one insertion per missing/reserved shell. They hold the current clip's terminal pose
between updates, without idle flashes. Each completed insertion transfers **one**
round from reserve into the magazine immediately, so the HUD updates shell by shell.
Press **LMB** to interrupt and fire a loaded round when the weapon's firing cooldown
is ready. An unfinished insertion consumes no ammo; completed shells are retained.
With an empty magazine, the reload continues until a shell is available to fire.
Holding LMB from before an automatic reload does not repeatedly cancel it.
Model timing remains available when the optional audio device fails to initialize.
Shell-loading sound is supplied when the private model lacks an event (including
the retail XM1014); authored M3 events are retained without double playback.
Reload samples are required during private conversion. `[Audio] CSVolumePercent`
continues to control shots and reloads together, including mute.

`[Shotguns] ShellByShellReload=true` and `InterruptReloadOnFire=true` accept
true/false or 1/0. F8 applies these policies to the **next** reload, preserving
the running reload's accounting. Set the first to false for magazine-at-completion
transfer, or the second to false to disable interruption by firing. Updates preserve
explicit custom preferences. Existing `ReloadSeconds` still sets full-magazine timing.

All firearms now use `[Combat] MinimumFirearmRange=30000` world units (0–50000).
Positive values extend each gun to at least that distance and twice
`[Rendering] DrawDistance`, covering the visible camera volume. This applies to
old saved short `Range` values too. Set it to **0** to restore individual weapon
ranges. F8 applies the shared and individual ranges immediately; upgrades preserve
custom values and insert missing settings once. Spread/recoil, target visibility,
terrain and intervening bodies still determine hits. Knife/sword reach and C4 splash
retain their physical rules. Empty firearm rays skip unnecessary long terrain walks.

## Glock mouse actions

**LMB** fires one round with the ordinary single-shot animation. **RMB** fires one
three-round burst per deliberate press, with one round, shot sound and damage
contact per bullet. A short magazine limits the burst to the rounds available.
The burst animation starts once. Holding either button cannot repeat its action.
**Y** still assigns RMB to squad commands in F6; then RMB cannot start a Glock burst.
Reloading, switching weapons/views, inventory/shops, menus, spells, losing attack
control or focus cancel remaining queued bullets.

`[Glock] SecondaryBurstRounds=3` (1–5), `SecondaryBurstInterval=0.1` seconds
(.025–1) and `SecondaryCooldown=0.5` seconds (.1–5, from first shot) are editable.
F8 applies them to the next burst; a running burst retains its spacing. The existing
attack-speed scaling option also scales that spacing. Ordinary LMB uses `FireInterval`.

## First-person hands and muzzle flashes

F6 hands and held weapons sway with actual movement speed, soften while crouching
and settle after stopping or becoming airborne. The decorative pose does not change
the camera or aim. All eleven firearms show an additive flash at the animated barrel,
including each Glock burst round and scoped AWP shots. Knife, sword and C4 do not
emit gunfire flashes. Regenerate owned CS assets through setup to obtain the original
model's barrel attachments; older WCG2 caches use a geometry-derived fallback.

`[Viewmodel]` settings apply with F8 and preserve saved values during updates:

| Key | Default | Range / meaning |
| --- | --- | --- |
| BobEnabled | true | Enables movement of hands and held weapon |
| BobAmount | 1 | 0–3, movement amplitude multiplier |
| BobFrequency | 1.8 | .5–4 cycles/second at 250 CS units/second |
| BobResponse | 12 | 1–30, smoothing response per second |
| MuzzleFlash | true | Enables firearm muzzle flashes |
| MuzzleFlashMilliseconds | 65 | 10–200 milliseconds from first presentation |
| MuzzleFlashScale | 1 | 0–3, flash size multiplier |

## CS 1.6 grenades

Equipment keys **4 HE, 5 Flashbang, 6 Smoke** append stable slots 15–17. Original
number keys remain unchanged. Each purchase buys one grenade, including quick
ammo on a carried grenade. Defaults: HE 300 gold/capacity 1, flash 200/capacity 2,
smoke 300/capacity 1. Full inventory never charges gold. Fresh standard kits have
no grenades; all-weapons grants them and free refill restores carried stacks.
Grenades have no reserve, magazine reload, hitscan bullet or muzzle flash.

In armed F6, hold LMB to pull/hold the pin, then release to throw. Releasing early
waits for the pin animation. Holstering, menus, casting or lost control cancels an
unlaunched throw without spending inventory. The native simulation fuse starts
at launch; flights/effects persist across weapon changes and RTS returns, freeze
with Warcraft pause and reset on map unload. The final grenade returns to USP
(or knife). F7/F9 retain native unit attacks.

Launch pitch and angle-dependent speed follow CS 1.6 at the project's 1.5 world
scale: a level throw leaves at ten degrees upward and 900 world units/s, reaching
1125 when aimed upward. The release starts 24 world units ahead of the eye,
clipped against nearby geometry. Running/jumping velocity is inherited. HE uses
660 world units/s² gravity; flash/smoke use 600. Flights bounce against terrain,
units and buildings using swept hulls. This calibrates launch reach; Warcraft
collision responses and the existing detonation/effect rules remain adaptations. HE has fixed peak damage, linear 3D falloff, cover checks and the
existing friendly-fire percentage; neutral buildings and destructable gates
retain native quest immunity. Flash blinds the controlled view according to
distance, facing and cover. As a Warcraft adaptation, nearby hostile mobile
units briefly pause; already paused units and the controlled actor are excluded,
and only pauses acquired by this feature are released. Set UnitStunSeconds=0 to
disable that adaptation. Smoke is a visual volume with depth-tested soft puffs,
near-eye density and fade; it does not block bullets or alter native AI sight.
Thrown bodies use the owner's original `w_hegrenade.mdl`, `w_flashbang.mdl` and
`w_smokegrenade.mdl` with their textures, real perspective and world occlusion.
Clouds use original soft puffs. Held models/previews and impact/explosion sounds
also come from the owner's installed CS files. Rerun setup/update to convert the
three world models into private `assets/world` caches.

Updates migrate the former stock ThrowSpeed=750 / Gravity=800 pair to the new
defaults once. Other custom pairs survive. `TrajectoryVersion=2` marks this upgrade;
keep it when deliberately choosing the old pair. F8 also recognizes unversioned
old stock values. F8 applies prices/carry
limits and tuning to subsequent launches; live flights/clouds keep their snapshot.

| Section / keys | Default | Bounds / units |
| --- | --- | --- |
| Grenades: Enabled | true | true/false |
| ThrowSpeed / Gravity | 1125 / 660 | 50–2000 world units/s (speed cap) and units/s² (HE) |
| LightGravityScale | .9090909 | .1–2 multiplier applied to flash/smoke gravity |
| Bounce / InheritedVelocity | .45 / 1 | 0–.9 restitution / 0–2 multiplier |
| FuseSeconds | 1.5 | .2–10 seconds after launch |
| ReleaseDelay / RecoverySeconds | .25 / .8 | 0–1 / .1–3 seconds of throw animation |
| MaxActive | 32 | 1–128 simultaneous flights/clouds |
| WorldModelScale / WorldSpinDegrees | 1 / 360 | .25–4 visual scale / 0–1440 degrees/s in flight; F8 affects new throws |
| Each grenade: Capacity | 1 / 2 / 1 | 1–100 charges |
| Price / AmmoPrice | above / above | 0–1000000 gold; one grenade per purchase |
| MoveSpeed / ModelScale | 250 / .9 | 50–500 CS units/s / .25–2 model FOV scale |
| HEGrenade: Damage / Radius | 100 / 350 | 0–1000000 fixed damage / 32–2000 world units |
| Flashbang: Radius / BlindSeconds / UnitStunSeconds | 1000 / 3 / 2 | 32–3000 units / 0–10 s / 0–10 s |
| SmokeGrenade: Radius / DurationSeconds / Opacity | 300 / 18 / .85 | 32–1000 units / 1–60 s / 0–1 |

## Native shops in F6/F7

Look at a friendly/neutral shop within **450 world units** and press **E**. Its
native command card opens with native buyer selection, stock, prices and map
requirements. Buy by clicking its buttons. **E/Escape** restores the same actor
and F6/F7 view. F6/F7/F9 while shopping also close the shop and restore the captured
view. Camera/weapon input yields to the shop; the world keeps running. Death,
ownership loss or leaving range closes the interaction.

`[Shops] Enabled=true`, `Radius=450` (100–1200 world units) reload with F8. The ray
must hit the actual shop before terrain, a unit or a destructable. Without a
qualifying shop, E retains item/rune pickup. Native shop abilities cover racial
and custom shops and neutral merchants/camps. No stock is invented.

## Army AI on U

**U** toggles the assistant for the current control session. **Y** retains the F6
RMB-order toggle. `[Shortcuts] ArmyAI=U` accepts existing key syntax or NONE. A saved
custom action using U keeps it on upgrade; the new AI binding becomes NONE.

Only living, available, **owned** units other than the actor and H/O members are
eligible. Explicit RMB targets take priority. J releases H/O membership, making
those units eligible again. Units fighting or executing other active orders retain
their orders. Idle fighters reserve some units at base and send the rest toward
the actor with native attack-move. Visible base threats call idle units into defense.

Stock workers/halls select a basic human, orc, undead or night elf build plan.
AI requests a missing hall, supply when food is tight, a basic barracks, workers
and basic soldiers. Idle workers gather native gold/lumber. Warcraft orders enforce
real costs, placement, technology, food, campaign rules and triggers. The assistant
does not research upgrades or expand into advanced technology. Undead gold gathering
requires an existing haunted mine; custom resource systems need compatible native
orders. Mixed captured races use one basic economy plan. Maps without suitable
workers/halls still get combat support and defense.

All `[ArmyAI]` settings reload with F8:

| Key | Default | Range / meaning |
| --- | --- | --- |
| Enabled | true | Enables U; automation starts off |
| IntervalMs | 1500 | 500–10000 milliseconds between decisions |
| ProductionRetrySeconds | 180 | 30–3600 active decision seconds before retrying canceled/unobserved training; pauses/background time do not count |
| Workers | 6 | 0–30 desired workers |
| Soldiers | 24 | 0–100 desired basic soldiers |
| Defenders | 3 | 0–100 idle fighters reserved at base |
| SupplyBuildings | 6 | 0–30 maximum basic supply buildings |
| BaseRadius | 900 | 100–3000 world units, threat scan |
| SupportDistance | 350 | 100–1500 world units, arrival/refresh spacing |
| BuildRadius | 450 | 192–1500 world units, construction sites |
| ResourceRadius | 1600 | 300–5000 world units, resource search |
| Worker/Hall/Supply/Barracks/Soldier | empty | Five four-character rawcodes select a custom coherent plan |

Incomplete custom plans fall back to stock detection. Rejected builds retry native
candidate sites without relocating buildings or clearing trees. U off stops new
decisions; construction, training, gathering and issued orders continue natively.
Leaving control/unloading resets AI off. Pauses, cinematics and lost focus suspend
new decisions. During a native shop AI continues while purchases remain manual.

Accepted training reserves one pending unit even when Warcraft reports an empty building
order. Observed population growth releases those reservations; hidden mining workers and
garrisons remain counted. Canceled/unobserved training eventually retries at the configured
decision-time limit. Map callbacks are followed by a fresh ownership/exclusion check before
the next order. Reinforcements stop within SupportDistance while fighting units retain combat.
Changing the retry duration applies to newly accepted training reservations.

## Carried weapon categories and sequential wheel

Armed F6 keeps every supported weapon you own. Selecting another weapon does not discard the
others or impose a primary/pistol carry limit. The menu previews a choice before equipping it.

With Tab item-number mode off, the first five weapon bindings open these categories:

| Default key | Category |
| --- | --- |
| 1 | Primary: rifles, sniper rifles, SMGs, machine guns, shotguns and available RPG |
| 2 | Pistols |
| 3 | Melee weapons |
| 4 | Grenades |
| 5 | C4 |

Press the same category again to highlight the next carried weapon in that group. Wheel up/down
browses all carried weapons in displayed group order (1 through 5), top to bottom in each list,
and opens the same menu. Wheel up retraces that order. One detent advances one available weapon;
fast batches retain their direction and every detent, skipping unowned/depleted entries.
Left-click confirms the highlight; that click
does not fire, throw or plant, including when held. Release and click again to use the equipped weapon.
Right-click, 0 or Escape cancels without changing weapons. A second Escape after closing returns
to RTS as usual. A category containing the equipped weapon starts on that weapon.

Only owned weapons appear. Empty firearms remain selectable for reloading; spent C4/grenades are
omitted until replenished. No currency or ammunition is spent by browsing. The hand-free previews
reuse the private buy-menu caches from the owner's CS installation; text works without those images.

Tab still starts in item mode by default: 1–6 use Warcraft inventory items. Switch Tab off for the
weapon category shortcuts; the wheel can browse weapons in either item-number mode. I, buying,
native shops, view changes, focus loss and F8 dismiss pending selection. F7 retains camera-wheel
zoom and native combat; F9 remains clean. Existing weapon bindings 6/7 retain direct C4/sword access
when their keys are not owned by item-number routing. Remapping Weapon1–Weapon5 remaps the categories.

Edit the runtime's `WarcraftCS/WarcraftCS.ini`, then press F8:

| `[WeaponSelection]` key | Default | Meaning |
| --- | --- | --- |
| Enabled | true | true/false. false restores legacy direct weapon 1–7 and immediate wheel switching |
| TimeoutSeconds | 5 | .5–15 seconds after the last browse action; expiry cancels, never equips |

Missing/malformed values use defaults; finite numeric values are clamped. Startup/F8 apply these
preferences, and updates add missing keys without replacing custom values or the Tab default.

## Optional Half-Life RPG

The RPG is added only when setup finds the owner's `models/v_rpg.mdl`,
`models/rpgrocket.mdl` and `sound/weapons/rocketfire1.wav`, `rocket1.wav`,
`debris1.wav` in the `valve` directory beside the chosen `cstrike` directory.
Loose files override PAK entries. No Half-Life download, engine or retail asset is
bundled. Missing or unsupported files skip RPG conversion and keep CS setup usable.
Run Install/setup again to discover newly installed content.

Conversion publishes a seal only after the held model, animation clock, hand-free
preview, world rocket and three sounds are complete. Startup/F8 checks their sizes
and fingerprints. Partial, changed or missing resources cannot unlock purchases,
free grants or selection. Unit-only installations always exclude RPG.

In armed F6, buy **B → 8 Equipment → 7 RPG** (4000 gold), then use group **1** or the
wheel and LMB confirmation as for other carried primaries. Buying grants one loaded
rocket; reserve packs cost 500 gold for one, up to five in reserve. Duplicates and
full packs never charge. Free refill restores owned RPG; all-weapons grants it only
when resources are available. Rune ammo follows the same ownership/cap rules.

LMB fires one real projectile with original held/world models and launch/motor
sounds. R reloads the authored two-second animation. Empty/loaded deploy and idle
poses stay distinct. RMB toggles the laser (on by default); when Y assigns RMB to
squad orders, RMB retains those orders. Laser guiding follows the visible aim point
and stops through cover. Disable it to reload while one of your rockets is flying.
Holstering is permitted and releases guidance; launched rockets continue in F7/F9
or RTS, using Warcraft's pause-aware simulation clock and clearing on map unload.
F7/F9 retain native unit attacks, rather than firing CS/Half-Life weapons.

Launch/motor behavior follows Half-Life numerical references at the existing 1.5
Warcraft world scale. This remains a Warcraft adaptation: host collision/cover,
fixed peak damage with linear blast falloff, native quest immunity and the shared
friendly-fire percentage apply. There is no full GoldSrc engine or underwater
rocket simulation. The smoke/glow/airburst rendering is original procedural art;
no retail sprite is distributed. Draw distance bounds the designator trace.

Edit `[RPG]` in `Game/WarcraftCS/WarcraftCS.ini`, then press F8 in FPS. Prices,
availability, laser preference and future-launch tuning reload; existing rockets
keep their immutable launch snapshot. F8 does not grant ammunition. Updates add
missing keys and preserve saved prices, tuning and opt-outs.

| Keys | Default | Valid bounds / units |
| --- | --- | --- |
| Enabled / GuidanceByDefault | true / true | true/false; resources remain required |
| Magazine / MaxReserve | 1 / 5 | 1–1000 loaded / 0–100000 reserve |
| Price / AmmoPrice / AmmoPack | 4000 / 500 / 1 | 0–1000000 gold / 1–1000 rockets |
| Damage / Radius | 100 / 375 | 0–1000000 fixed peak damage / 32–3000 world units |
| FireInterval / ReloadSeconds | 1.5 / 2 | .025–10 / .1–30 seconds |
| Range | 30000 | 100–50000 world units, subject to common MinimumFirearmRange |
| LaunchSpeed / MaximumSpeed | 375 / 3000 | 50–6000 / 100–10000 world units/s |
| Acceleration | 6000 | 0–20000 world units/s² during motor boost |
| IgnitionSeconds / BoostSeconds | .4 / 1 | .01–3 / .1–5 seconds |
| LaunchGravity / InheritedVelocity | 600 / 1 | 0–2000 world units/s² / 0–2 projected velocity multiplier |
| LifetimeSeconds / MaxActive | 12 / 16 | 1–30 seconds / 1–64 rockets |
| WorldModelScale | 1.5 | .25–4 |
| TrailSeconds / TrailSize | 1 / 10 | 0–3 seconds / 0–40 world units; zero disables smoke |
| MoveSpeed / ModelScale / Kick | 250 / .9 / 2 | 50–500 CS units/s / .25–2 / 0–10 degrees |

The primary behavior reference is [Valve's RPG implementation](https://github.com/ValveSoftware/halflife/blob/master/dlls/rpg.cpp).
Only factual numerical behavior is independently adapted; SDK code and game assets
are not copied into this source tree. See NOTICE for the distinct upstream terms.

## Ledges and close-contact camera fixes

Landing support now uses the actor's footprint rather than its center alone: a foot
can catch and traverse a reachable terrain/prop lip without sinking beside it and
getting blocked on the next step. Leaving the trailing edge starts a real fall.
Lower-bank/cliff probes cover both neighboring terrain-cell rings, and swept-valid
positions near a cliff avoid native nearest-cell relocation even with a clear center.
Tall/unknown blockers, bridges, water and ordinary terrain validation remain active.
No new movement keys or settings are introduced.

The third-person camera no longer collapses to zero when its pivot is already
inside a broad destructable model proxy, such as a bridge/ladder containing empty
space. Separate positive-distance gate/tree surfaces and terrain still shorten
the boom. Weapon hitboxes and physical movement keep their own collision rules.
The existing CameraBuildingCollision preference, wheel zoom and F8 distance remain.

## Why the HUD could disappear, and the diagnostic changes

The flat HUD previously inherited part of the world/weapon OpenGL state. Setting
blend factors does not reset a subtract/min/max blend equation, and coverage masks,
extra texture targets/stages, smoothing or other inherited raster state can hide
valid HUD quads. A controlled WGL regression reproduces an entirely black HUD under
a legal subtract equation **without an OpenGL error**. The new scoped HudRenderState
normalizes the flat pass after 3D drawing and restores the host state afterwards.
Text, bars, portrait, inventory, spells and menus share that prepared pass. F9 stays
clean by design. This is separate from 0.9.1's earlier bitmap-font/solid-rectangle fix.

The submitted AMD RX Vega 10 report showed overlay/reticle submissions and zero
sampled GL errors, which establishes submission, not visible pixels. Its old logs
do **not** prove which state caused the specific remote failure; the reporter's
GPU/driver has not been tested here. The supplied install transcript described
Developer preparation and contained no bundled-launcher failure, so that error
cannot be attributed to Miles or another cause from those attachments. A strict
Miles-host mismatch is one supported rejection condition, now logged precisely.
The running report also contained CS weapons despite the described Warcraft-only
choice; active profile/file diagnostics make that discrepancy observable.

New bounded diagnostics record host, after-weapon, prepared and completed stages:
draw/read buffers and framebuffer IDs, blend equations, program flags, MSAA coverage,
color/depth/stencil/scissor/polygon state, matrices and active texture targets.
Font lifecycle events identify hint/status/unit/spell/inventory roles, failure stage,
requested and atlas sizes, Win32 error, failed glyph, coverage and recovery. Readiness,
actor snapshot, active profile and config path are logged at startup/F8; F9 records
its deliberate omission. Diagnostics preserve queried state and pending GL errors,
with no automatic screenshots or framebuffer readback.

Use existing `[Logging] Detailed=true` and IntervalMs (default 1000 ms) for frame
snapshots. Mode/context changes are immediate; repeated identical font failures
are limited to one per second per font. Font failure/recovery events remain with
Detailed=false. Existing MaxFileMB/ArchiveCount rotation remains authoritative.
Launcher logs include requested profile, source/bundled variant and setup stages
before install.log exists. Miles rejection records expected/actual SHA-256 hashes.
For reports collect `Game/WarcraftCS/WarcraftCS.log`, client `install.log` and
`%LOCALAPPDATA%\WarcraftCSLauncher\launcher.log`, including rotated segments when relevant.

## Source, conversion and verification

The weapon-menu skill and presentation reference now cover original barrel
attachments, movement pose, muzzle effects and private model/audio verification for
future CS 1.6 additions. Combat/native effects, projectile physics, world views,
optional asset sealing and HUD diagnostics retain separate module ownership.
NOTICE distinguishes independently adapted numerical Half-Life behavior from Valve
SDK/game-asset licensing; no SDK code or retail media is added to the repository.
The complete regression wrapper now also includes optional RPG tests.

Automated verification covers portable movement/collision, targeting, reload/burst
accounting, ownership/prices/menu order, parser/F8/migration contracts, native blast
adapters and real WGL text/portrait/reticle/effect pixels, GL state preservation and
context replacement. Diagnostic tests retain pending host errors and respect trace
opt-out/cadence; XAudio2 tests verify actual source-voice event/motor routing.
Launcher suites exercise consent, profile/dependency selection, reinstall/update,
source/module/hash binding and both actual EXE variants. Source/archive lint excludes
game assets, secrets, dumps and private paths.

Feature development used separate authorized Warcraft maps for the eleven firearms
against a distant building, native shops/army support, grenades, ledge movement,
camera obstruction and RPG impacts/guidance; subsequent HUD runs verify F6/F7,
clean F9 and Warcraft-only switching with byte-exact private DLL/INI restoration.
Release checks rerun the independent suites and ordinary-module smoke scenarios;
these checks are bounded, not a complete campaign/custom-map playthrough or an AMD
compatibility guarantee. Some recordings are silent: successful audio routing is
not a listening test. The army assistant is basic stock-race economy/support,
not a complete melee AI; flash unit pauses, smoke and rocket collision are Warcraft
adaptations, not a GoldSrc engine. Existing user progress is preserved.

<!-- launcher-summary
- Added Deagle, Glock, MP5, P90, M249, M3 and XM1014 purchases.
- Added native shops on E and optional army economy and support AI on U.
- Added a CS-style carried weapon menu with sequential wheel selection.
- Added HE, flash and smoke grenades with original thrown models.
- Added an optional Half-Life RPG with real rockets and laser guidance.
- Added movement of hands and animated muzzle flashes.
- Changed firearm reach to cover the visible world by default.
- Fixed shotgun reload timing and sounds with interruptible per-shell loading.
- Added single-shot and right-click burst actions for the Glock.
- Fixed ledge traversal and camera collapse inside bridge model bounds.
- Fixed inherited OpenGL state hiding the flat HUD.
- Added bounded HUD, font, installation profile and DLL diagnostics.
-->
