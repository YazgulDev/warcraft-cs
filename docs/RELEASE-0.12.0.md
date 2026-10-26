# Warcraft CS 0.12.0 — 25 October 2026

This release adds the optional Half-Life arsenal, a dollar wallet and rare weapon loot,
experimental Warcraft III 1.27a/D3D9 support, and selectable movement collision with
platform recovery. It includes every change since the immutable **v0.11.0** tag.
The default movement mode is **recovery**; confirmed hostile creature kills with mod
weapons pay **$200**. Existing explicit settings survive installation and F8 reload.

## Downloads and installation

- **WarcraftCSLauncher_DLL_Included.exe**: ready project-owned x86 mod modules; choose
  **Install — Player**. Supports owned 1.26a and experimental 1.27a installations without
  C++ Build Tools. No Warcraft/Counter-Strike/Half-Life files or converted assets are bundled.
- **WarcraftCSLauncher.exe**: source launcher; Developer setup compiles locally and may
  download missing Microsoft Build Tools/Windows SDK and conversion dependencies.
- **WarcraftCS-0.12.0-dist.zip** and **WarcraftCS-sources.zip**: source-only distribution;
  public ZIPs contain no prebuilt DLL/MIX modules or DLL-included launcher.
- **WarcraftCS-update.json** and SHA256 sidecars bind the two launcher variants to the
  same audited sources. Legacy updater clients retain the source-build path.

Use Windows 10/11 x64 and your own complete Warcraft III installation. The game/mod
runtime is x86. **1.26a (Game.dll 1.26.0.6401) uses OpenGL**; experimental **1.27a
(1.27.0.52240) defaults to D3D9**. RoC/TFT remain selectable. Reforged/other patches
and multiplayer are unsupported. A functioning host graphics driver remains required.

Save campaign progress and close Warcraft before installing this DLL update, then
restart. F8 reloads supported INI tuning; it cannot replace a loaded DLL. Select the
same dedicated mod installation folder to retain configuration. Choose your owned
Counter-Strike `cstrike` folder, with sibling Half-Life `valve` resources, and reinstall
so the private weapon/effect caches are regenerated. **Only Warcraft modes** skips CS
and Half-Life weapons; bundled Player setup then also skips Python/NumPy.

Starting money and dollar balances are map-local and are not stored in Warcraft saves.
Old explicit `Mode=classic`/`hull`/`recovery` and `KillReward` values remain in place:
set `[Movement] Mode=recovery` and `[Economy] KillReward=200`, save and press F8 if
upgrading a customized preview and you want the new defaults.

## Experimental Warcraft III 1.27a and dual-host Player setup

- Select exact native bindings by registered JASS name/signature and independent
  Game.dll code signatures. Missing internal adapters disable themselves instead of
  executing a 1.26a address in 1.27a. Unsupported versions are rejected.
- Add D3D9 interception and a shared rendering adapter for current weapon, HUD,
  grenade, rocket and beam presentation; preserve host graphics state and resources.
  1.27a uses presentation ticks because its native world callback remains unresolved.
- Player packages contain both native tables and an adaptive project-owned Miles
  proxy. Setup derives exact named, decorated, aliased and ordinal-only forwarders
  from the owner's local sound library; the untouched original library is used locally.
  Installation needs no 1.27a retail DLL at package-build time or C++ tools for Player.
  Malformed inputs fail before replacing modules; legacy exact-host/hash checks remain.
- Source launch supports `-Renderer Auto|OpenGL|D3D9`; Auto selects the host default.
  `[Damage] UnavailableAttackFallback=weapon` uses configured weapon damage when the
  unverified 1.27a native attack-stat reader is unavailable; `zero` disables that
  fallback. Missing/invalid values use weapon; startup/F8 applies it. A working
  adapter reporting a real zero attack still deals zero damage.

**1.27a is a partial, unverified real-game port.** No 1.27a campaign, loaded-map,
fullscreen or Alt-Tab validation is claimed. Native model/item/destructable discovery,
exact terrain/bridge collision, several inventory/spell helpers, native attack effects,
minimap/portrait capture and the verified 1.26a camera-clamp override are unavailable
or use documented fallbacks. D3D9 hidden-device tests and synthetic host fixtures do
not establish campaign compatibility. OpenGL on 1.27a is a diagnostic path.
See [the complete port limits](https://github.com/YazgulDev/warcraft-cs/blob/v0.12.0/docs/PORT-1.27.md).

## Optional Half-Life Crowbar, Tau Cannon and Gluon Gun

- Add a dedicated **B → 9** Half-Life category: **1 Crowbar, 2 Tau Cannon, 3 Gluon Gun,
  4 RPG**. Carried Half-Life weapons use group **6** and the wheel. Turn item-number
  mode off with Tab for weapon number keys. F12 grants available weapons; F11 refills
  owned weapons. Each optional weapon needs its own complete private resource seal.
- Crowbar uses traced melee contacts with flesh, hard and dirt impacts, attack cadence
  and delayed contact. Tau primary consumes cells; hold secondary to charge, release
  to fire, with gradual spinner speed and source-audio pitch. Gluon channels while held
  with independent tick/cell accounting; purchases/pickups refill usable cells directly.
  There is no invisible reserve pool for either energy weapon.
- Convert owned loose/PAK Half-Life models/sounds/previews locally. WCG3 caches retain
  chrome flags, animated normals, parent hierarchy and loop flags. Interpolate rigid
  rotations before composing bones, keep complete firing/recoil and final action frames,
  and avoid idle flashes at transitions. Existing WCG2 caches remain readable; CS/RPG
  conversion stays compatible. Reconvert for fixed chrome, animation and Crowbar seals.
- Share one animation sample and native/held projection correction for the barrel and
  beam. Fit Gluon's nozzle to its owned nose material when available, otherwise use the
  authored attachment. Improve beam alignment while moving, aiming and charging.
- Refresh the Gluon endpoint every game frame between damage ticks without extra
  damage, cells or sound. Add a bright core, animated strands and depth-tested impact
  flare. Optional owned `xbeam1.spr`/11-frame `XSpark1.spr` caches supply textures;
  missing/corrupt optional art uses procedural effects. Graphics replacement rebuilds
  textures. No retail pixels, audio, maps or SDK implementation are distributed.
- Default Tau damage is **100 primary / 1000 fully charged**; Gluon is **70 per tick**
  at a **0.1-second** cadence. Native defenses still apply. `HalfLife.DamageVersion=2`
  migrates old stock Tau 20/200 and Gluon 14 once; custom values, zero and later edits
  back to former values survive. F8 cancels charge/channel and rechecks availability
  without granting ammunition. Charge/energy ownership stays separate from rendering.

### Half-Life settings

All settings below live in `WarcraftCS/WarcraftCS.ini`, survive setup and apply on F8
(or startup). Start/end pairs keep End at least Start; equal values disable a ramp.
Ranges reject malformed/non-finite inputs using defaults and clamp finite values.

| Section / key | Default | Units and accepted bounds |
|---|---:|---|
| Crowbar / Enabled, TauCannon / Enabled, GluonGun / Enabled | true | true/false; still requires owned files |
| Crowbar / Damage, HeroMultiplier | 10, 1 | fixed damage 0–1000000; hero multiplier 0–1000 |
| Crowbar / Price | 200 | selected currency 0–1000000 |
| Crowbar / Range | 150 | world units 32–1000 |
| Crowbar / FireInterval, ContactSeconds | 0.5, 0.1 | seconds 0.1–5 / 0–1; authored clip can extend recovery |
| Crowbar / SurfaceSounds | true | true/false; false uses simple flesh/hard impacts |
| TauCannon / Damage, ChargedDamage | 100, 1000 | fixed damage 0–1000000 |
| TauCannon / ChargeSeconds | 4 | seconds 0.1–10 to full power |
| TauCannon / SpinStartRate, SpinEndRate | 1, 3 | animation playback multipliers 0.25–8; End is at least Start |
| TauCannon / ChargePitchStart, ChargePitchEnd | 1, 2.5 | source frequency/pitch ratios 0.25–4; End is at least Start |
| TauCannon / ChargeDrainSeconds | 0.3 | seconds per cell 0.05–2 plus the starting cell |
| TauCannon / PrimaryCost | 2 | cells 1–100 per primary shot |
| GluonGun / Damage | 70 | fixed damage per tick 0–1000000 |
| TauCannon and GluonGun / Magazine | 100 | usable cells 1–1000; reserves always zero |
| TauCannon / Price, GluonGun / Price | 5000, 6000 | selected currency 0–1000000 |
| TauCannon and GluonGun / AmmoPrice, AmmoPack | 300, 20 | selected currency 0–1000000, cells 1–1000 |
| TauCannon / FireInterval, GluonGun / FireInterval | 0.2, 0.1 | seconds 0.025–10 |
| TauCannon and GluonGun / Range | 30000 | world units 100–50000, subject to firearm reach floor |
| All three / MoveSpeed, ModelScale | 250, 1 | movement 50–500 units/s, viewmodel scale 0.25–2 |
| TauCannon / Kick, GluonGun / Kick | 2, 0 | degrees 0–10 |
| HalfLife / BeamSeconds, BeamWidth | 0.15, 3 | seconds 0.025–1, world units 0.5–20 |
| HalfLife / BeamEffects, OwnedSprites | true, true | true/false; animated strands/impact, optional private Half-Life textures |
| HalfLife / ImpactSize | 18 | contact flare radius 0–100 world units; zero disables |
| Combat / NativeAttackSound | false | true/false; FPS native weapon audio and associated instant-hit art, preserves passives and TPS audio |

## Silent FPS attacks

`[Combat] NativeAttackSound=false` suppresses the controlled unit's native weapon
contact audio and ordinary instant ranged attack art during FPS weapon contacts.
That art can carry embedded arrow sound events, which survived the first sound mute.
The 1.26a adapter guards two verified call sites only during the synchronous contact,
restores native fields, and preserves passive/spell art, attacks triggered for other
units and TPS/RTS audio. `true` restores native attack presentation on F8. 1.27a keeps
its direct-damage fallback without unverified attack-component writes. This does not
mute the weapon's owned Half-Life/CS firing sound.

## Dollars, rare firearm drops and RPG blast

The default buy menu and quick-ammo purchase use an independent dollar wallet:
**$800 once per player/map, $200 per confirmed hostile creature kill, $16000 cap**.
Kills from CS/Half-Life firearms/melee, HE, C4 and RPG qualify. Allied/non-hostile units,
buildings, nonlethal hits, corpses and native attacks/spells/squad/RTS kills do not.
Changing views/actors, F8 and free-refill keys never grant starting funds again.
`Currency=gold` restores Warcraft gold purchases without adding CS money to native
resources. Returning to dollars resumes that map's wallet; it does not convert gold.
Map gold, quests, native loot and Warcraft item events remain authoritative.

Each eligible kill rolls **0.5%** for an enabled/available firearm by default, including
optional RPG/Tau/Gluon only with owned assets. Melee, C4 and grenades are excluded.
Stock chest markers show a nearby weapon name; **E** picks up a loaded weapon or one
duplicate ammo pack. Tau/Gluon duplicates give usable cells; full ammo leaves loot.
F8 disable removes existing markers; map unload clears them. Chance is independent,
roughly one per 200 kills on average with no guaranteed interval. Simulation time
controls expiration, so pausing the game does not consume its lifetime.

RPG peak damage stays **100** and blast radius becomes **525 world units (+150)**.
`RPG.BlastVersion=2` upgrades the previous stock 375 once, preserving custom blast and
damage values. Flying rockets retain their launch settings. Owned RPG assets remain
required for buying, picking up and free grants.

| Section / key | Default | Bounds / timing |
| --- | --- | --- |
| Economy / Currency | dollars | dollars or gold; F8 |
| Economy / StartingMoney | 800 | Whole dollars, 0..1000000, limited by MaximumMoney; next new map/player wallet |
| Economy / KillReward | 200 | Whole dollars, 0..1000000; subsequent kills after F8 |
| Economy / MaximumMoney | 16000 | Whole dollars, 0..1000000; F8 clamps existing balances, increasing it grants no money |
| WeaponDrops / Enabled | true | true/false; F8 disable removes ground drops |
| WeaponDrops / ChancePercent | 0.5 | 0..100 percent per eligible kill; zero disables future rolls |
| WeaponDrops / LifetimeSeconds | 120 | 1..3600 simulation seconds for new drops; existing expiry retained |
| WeaponDrops / PickupRadius | 160 | 16..400 world units; F8 |
| WeaponDrops / MaximumDrops | 64 | 1..256; F8 removes excess drops, at the cap new drops are skipped |

## Movement, platforms and camera ownership

- **Mode=recovery** is the shipped and missing/empty/invalid default. It uses a round
  player hull, locally sampled terrain normals, standing/crouching head clearance and
  platform recovery. **Mode=hull** retains the intermediate round-hull implementation
  without automatic/Home platform recovery. **Mode=classic** retains the pre-24 October
  terrain/object/jump strategy, including its old native radius, box/axis sliding and
  floor resolver. All three receive the world-boundary correction. Mode names are
  case-insensitive; F8 switching preserves position, stance and momentum and clears
  stale rescue history when required. Explicit modes survive updates.
- Ground contacts use nearby surface slope rather than a distant terrace height gate.
  Blocked cliff faces cannot supply steps or repeat takeoffs; falls slide downhill.
  Preserve flat ledge lips, ramps, low props, exit from existing overlap and wall-tangent
  motion. A short jump cannot enter the side of an unreachable flat ledge. Clear lanes
  beside known models do not inherit their overhanging path cells; unrelated unknown
  blockers retain conservative native pathing. Head checks preserve crouch under ceilings
  and require sufficient clearance for steps.
- Cap the actor radius while retaining smaller known footprints, including when FPS
  temporarily disables native pathing. Use transformed stationary non-tree prop triangles
  for actual arch openings, low plinths and partial landings. Physical height remains
  108 standing / 66 crouched; weapon/cape extents do not enlarge the body. Static Stand
  pose/material validation and unreadable/animated model fallbacks remain conservative.
  Trees, moving units/buildings and native decks keep their existing authoritative paths.
- Native platform support checks the full foot. Verified live platform meshes remain
  available across a missed native deck query; reachable lower parts of sloping triangles
  can support a landing. Repair proven below-terrain states and lost support for an
  established platform rider, while preserving ordinary falls, underpasses and destroyed
  bridge falls. **Home** requests a revalidated nearby visited checkpoint, not an arbitrary
  teleport. Cargo, roots/stuns, casting and cursor menus reject manual recovery; scripted
  relocations/map/actor/view changes clear incompatible history.
- Repair actors already outside the world before idle/floor sampling and after native
  placement in every mode; retain motion along the edge and inward. RTS camera limits
  do not become invisible walking walls. On verified **1.26a**, commit the physical eye
  target after camera-field writes and bypass the repeated position clamp only while the
  live FPS camera owns it. Restore authored limits immediately after every native call;
  preserve mission/minimap bounds, RTS/cinematics and unrelated cameras. Unverified
  1.27a internals do not install this clamp hook.
- Keep shared acceleration, jumping, gravity, bunnyhop and air-strafe tuning unchanged.
  These collision changes remain Warcraft adaptations rather than a complete GoldSrc
  movement engine. Native campaign traversal, including the reported Ancient One platform,
  still needs verification with the released DLL; synthetic tests cannot establish that.

| Section / key | Default | Bounds / application |
| --- | --- | --- |
| Movement / Mode | recovery | classic, hull, recovery; startup/F8; preserve current motion |
| Movement / MaxWalkSlope | 46 | 15–75 degrees; hull/recovery; F8 |
| Movement / HullRadius | 24 | 8–64 world units, caps known native radius; hull/recovery; F8 |
| Movement / ObjectMeshCollision | true | true/false; hull/recovery; false uses coarse volumes; F8 |
| Movement / RecoverInvalidPosition | true | true/false; recovery mode; F8 |
| Movement / RecoveryDistance | 512 | 64–2048 horizontal world units; recovery mode; F8 |
| Shortcuts / RecoverPosition | HOME | remappable key or NONE; recovery mode; F8 |

## Documentation, integration and credits

Replace the long GitHub homepage with a concise overview and separate documentation
pages for controls, configuration, compatibility, economy, Half-Life and movement.
Remove duplicate setup text, stale prices and obsolete contribution branch guidance;
retain historical release notes and third-party notices. Keep native host/graphics,
portable geometry/movement, energy state, economy and presentation responsibilities in
their existing modules. Extend the weapon authoring workflow and private conversion tools.

1.27a native address research/table generation and D3D9 interception adapt niceqwer55555's
work in TTT-ux-max/warcraft-cs, pinned to 571fdec95a3059935545ab4c5a1b916fe5ea4866.
Original project code remains MIT OR Apache-2.0; third-party notices remain in NOTICE.
Development used Codex assistance. This unofficial offline fan project is unaffiliated
with Blizzard Entertainment or Valve. No game libraries/assets, proprietary dumps,
secrets or converted private caches are part of the published source tree or packages.

## Verification and limits

Release checks cover the complete native-independent wrapper, settings/default/F8
and setup migration, real hidden OpenGL/D3D9 pixels and state restoration, adaptive
native forwarders, synthetic exact-host selection, launcher/update/reinstall and both
actual packaged EXE variants. New camera fixtures cover repeated render clamps,
rotated limits, yaw/pitch, RTS/unrelated cameras and signature refusal; geometry and
collision fixtures cover platform support, passages, headroom, edges and all modes.
Source/archive audits and the mod distribution lint check shipped content and hashes.

These checks do not constitute a full campaign/custom-map playthrough. The owner's
running campaign was observed without driving it or replacing its loaded DLL. There
is no real 1.27a gameplay validation. Native effects, imported geometry and custom map
scripts can still require further fixes. Dollar rewards apply only to the documented
mod damage paths; complete Half-Life wall penetration, reflection/recoil jumping and
underwater weapon behavior are not implemented. Save before upgrading and report the
map, host version, release and reproduction steps with the runtime logs.

<!-- launcher-summary
- Added experimental Warcraft III 1.27a and Direct3D 9 support alongside 1.26a.
- Added Player setup for both hosts using exact local Miles export forwarders without C++ Build Tools.
- Added optional Half-Life Crowbar, Tau Cannon, Gluon Gun and RPG in the B / 9 Half-Life category.
- Fixed Half-Life chrome skins, complete weapon animations, beam muzzle alignment, Tau charge feedback and varied Crowbar impacts.
- Fixed delayed Gluon beam aiming and added animated energy strands, impact glow and optional owned Half-Life effect textures.
- Changed default Tau damage to 100 / 1000 charged and Gluon damage to 70 per tick, preserving custom settings.
- Disabled native hero weapon contact sounds during FPS attacks by default while retaining native attack effects.
- Fixed residual ranged attack audio from native instant-hit art during FPS weapon contacts, preserving spell and passive effects.
- Changed default purchases to dollars with 800 starting money, 200 per mod weapon kill and a 16000 cap.
- Added configurable rare weapon drops with a 0.5 percent default chance and E pickup.
- Changed default RPG blast radius to 525 while retaining 100 peak damage.
- Preserved custom settings and source/runtime integrity through Player install, reinstall and release updates.
- Improved local cliff collision, downhill sliding, round player hulls, narrow passages and crouching head clearance.
- Improved static prop collision so fitting arch openings stay passable and low parts of tall models can be jumped onto; preserved movement physics and conservative fallback.
- Fixed platform-edge landings, retained live platform surfaces during native query dropout and preserved reachable sloping mesh support.
- Added configurable invalid-position protection and Home recovery to revalidated nearby visited ground, preserving ordinary jumps, falls and underpasses.
- Changed default movement to recovery with platform protection and Home rescue; preserved classic and hull choices and F8 switching.
- Fixed FPS camera separation at native camera edges and repaired out-of-world actor positions in every movement mode.
-->
