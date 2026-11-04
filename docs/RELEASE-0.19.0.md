# Download and upgrade

- [**WarcraftCSLauncher_DLL_Included.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.19.0/WarcraftCSLauncher_DLL_Included.exe) — Player launcher with bundled project modules and private graphics components; no local C++ compilation.
- [**WarcraftCSLauncher.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.19.0/WarcraftCSLauncher.exe) — Developer/source installation with consented dependency preparation and local compilation.
- [**WarcraftCS-0.19.0-dist.zip**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.19.0/WarcraftCS-0.19.0-dist.zip) — standard launcher, sources, update manifest, requirements and checksum. Public source/distribution ZIPs contain no DLL/MIX modules.
- [**Surf Skyline map + Russian instructions**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.19.0/WarcraftCS-Surf-Skyline.zip) · [direct .w3m](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.19.0/WarcraftCS-Surf-Skyline.w3m) — three-stage time trial with floating solid ramps, gaps and checkpoints.
- [**Surf Practice map + instructions**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.19.0/WarcraftCS-Surf-Practice.zip) · [direct .w3m](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.19.0/WarcraftCS-Surf-Practice.w3m) — three introductory terrain slopes.
- [Requirements](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.19.0/REQUIREMENTS.md) · [Sources](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.19.0/WarcraftCS-sources.zip) · [Update manifest](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.19.0/WarcraftCS-update.json) · [Player SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.19.0/WarcraftCSLauncher_DLL_Included.exe.sha256) · [Developer SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.19.0/WarcraftCSLauncher.exe.sha256).

Save progress and close Warcraft before updating. Use **Update from GitHub**, or install the new launcher into your existing dedicated mod folder, then restart Warcraft. Setup preserves saves, custom controls, prices, renderer, sky and movement choices. New keys are inserted and former stock balance/movement values migrate once; explicit custom values and current-version opt-backs remain saved. Starting Crowbar requires your existing complete owned Half-Life resources. Native skies need no CS assets.

Maps are separate optional downloads. Copy the chosen `.w3m` to `<mod installation>/Game/Maps/Download`, then select **Single Player → Custom Game**. Surf Skyline needs this new model-surf build; the older terrain-only preview launcher is insufficient. Original game files and converted Warcraft/CS/Half-Life assets are not distributed.

## Warcraft CS 0.19.0 — 4 November 2026

All changes since immutable **v0.18.0**: active in-map portals, starting Crowbar, heavy-weapon balance and native exploded deaths; F2 field of view and Warcraft sky selection; faster configurable Bunnyhop, crest/contact fixes, terrain/model surfing and two original downloadable maps. Movement remains **classic** by default. Swimming, the previous arsenal/economy and nuclear RPG remain available.

## Portals and starting Crowbar

Approach an active native waygate during FPS, third-person or clean unit-view control to travel to its **map-authored destination**. Inactive, hidden and dead gates do not activate. The physical floor, camera, velocity and recovery history rebase to the actual native destination. A pause-aware cooldown plus leaving the arrival gate prevents immediate reverse crossings. Scripted region teleports keep their own map triggers; observed native relocations also rebase the controlled view. Campaign-map exits or missing destinations are not synthesized.

All `[Portals]` keys apply at startup/F8 and survive updates:

| Key | Default | Range / units |
| --- | --- | --- |
| Enabled | true | false disables automatic waygate interaction/relocation handling |
| Radius | 160 | 32–512 Warcraft units from the gate center |
| HeightTolerance | 160 | 16–512 Warcraft units; prevents pulling across another floor |
| CooldownSeconds | 1 | 0.25–10 simulation seconds, plus leave-gate re-entry protection |

`[Loadout] Crowbar=true` adds an enabled, fully installed Half-Life Crowbar to the initial kit on startup/map load. It does not select it instead of the normal USP or grant another kit on F8/view switches. Set false to keep it purchase-only. Missing/incomplete resource seals and **Only Warcraft** profiles do not unlock the weapon. See [portals and heavy weapons](PORTALS-HEAVY-WEAPONS.md).

## Heavy-weapon damage and exploded deaths

| Weapon | New stock damage | Previous stock damage |
| --- | --- | --- |
| AWP | 230 fixed / 6× native attack | 115 / 3× |
| G3SG1 | 160 fixed / 4.4× native attack | 80 / 2.2× |
| HE grenade | 1200 peak fixed blast | 600 |
| Tau Cannon | 200 primary / up to 2000 charged | 100 / 1000 |

Damage remains subject to the selected combat mode, native armor/immunity and existing headshot/protection rules. HE retains its radius and distance falloff. Ordinary RPG stays at **1800 peak / radius 1000** with the existing three-dimensional falloff; nuclear settings are independent. These values do not turn AWP into an unconditional one-shot weapon.

Fixed/charged damage accepts 0–1000000; native attack multipliers accept 0–1000. Startup/F8 changes subsequent contacts or launches. Live explosives retain their launch snapshots. Stock-only migrations preserve custom tuning, zero damage and explicit current-marker opt-backs: AWP/G3SG1 `DamageVersion=2` and `MultiplierVersion=2`, HE `DamageVersion=4`, Tau `DamageVersion=3` and `ChargedDamageVersion=2`.

`[Combat] ExplosiveWeaponDeaths=true` makes eligible hostile creatures killed by Gluon, Tau or ordinary RPG use Warcraft's native exploded/artillery death presentation. Native death events, ownership, armor, immunity and model/hero restrictions remain authoritative. Allies and structures retain their ordinary policy. Surviving units regain their original flags; map-owned flags are preserved and stale/replaced victims cannot receive a restoration write. Full-body explosion takes precedence over energy head-removal presentation. Startup/F8 changes future contacts; RPG captures the policy at launch. Unsupported adapters retain ordinary deaths; the exploded-flag adapter is verified only on 1.26a.

## F2 field of view and Warcraft skies

**F2 → Field of view** controls the horizontal world-camera angle in FPS, clean F9 and third person: `[Rendering] FieldOfView=85`, range **60–120 degrees**, one-degree menu steps. Apply & Save changes the native camera and overlay/sky projection together; manual edits apply on F8. The per-weapon held-model scale is independent. Active scope zoom remains unchanged; leaving the scope uses the saved ordinary angle. Cancel discards the draft; Reset These drafts 85. Applying preserves unrelated movement, inventory and session settings.

When **Sky type = Warcraft**, the existing preset row offers **Auto/map, summer, autumn, winter, Felwood and Dalaran** through `[Sky] WarcraftPreset=auto`. Valid identifiers are `auto`, `summer`, `fall`, `winter`, `felwood`, `dalaran`. Auto preserves an authored sky and fills a missing one from the map terrain. Other choices temporarily override the native sky in controlled views. Stock skies load from the owner's Warcraft resources and need no CS conversion.

Warcraft and CS sky choices remain independent while switching menu drafts. Visibility/type changes and RTS exit restore the latest authored map sky, including script replacements made during an override. Cancel and failed saves preserve the previous settings. Startup/F8 and F2 Apply use the same validated preset. The nine-row panel fits above its controls/hints; the CS type still selects installed private CS sky caches. See [in-game settings](INGAME-SETTINGS.md).

## Faster Bunnyhop and crest fixes

The stock moving-takeoff bonus changes **0 → 15%**, air acceleration **10 → 30**, and the default/maximum editable horizontal Bunnyhop speed cap **1000 → 3000 GoldSrc units/s** (**4500 Warcraft units/s**). This is a speed ceiling, not an instantaneous speed grant. Jump height, gravity, swimming and the selected collision strategy retain their own rules. Shift/Ctrl suppress the optional boost. Roots/stuns and native slows remain effective.

| `[Movement]` key | Default | Range / meaning |
| --- | --- | --- |
| JumpBoostPercent | 15 | 0–100 percent for a real moving takeoff; 0 keeps strafe gain only |
| AirAcceleration | 30 | 0–100 wish-direction acceleration multiplier |
| MaxBunnySpeed | 3000 | 250–3000 GoldSrc units/s; world scale 1.5 |
| ClassicRampContacts | true | bounded airborne foot contact on verified walkable terrain in classic mode |

Startup/F8 applies these values without resetting position or velocity; the next movement step applies the speed limit. Independent `JumpBoostVersion=2`, `AirAccelerationVersion=2` and `MaxBunnySpeedVersion=2` migrate only former stock values once. Keep the current marker when deliberately selecting an older/zero value. Saved custom values are preserved.

Classic no longer clears horizontal speed on accepted descending crest landings or bounded rising/high-ramp contacts. It resolves support around swept substeps and uses the incoming walkable plane, actual reachable height and headroom. Rejected candidates restore their prior physics state. `ClassicRampContacts=false` retains the earlier ramp-contact behavior. Genuine cliffs, blocked cells, solid props and ceilings still block movement.

The optional takeoff bonus is now confirmed **after collision accepts a real airborne departure**. An uphill jump proposal that lands again in the same frame earns no bonus. This fixes sudden forward launches caused by repeatedly multiplying speed on same-frame recontacts. See [movement](MOVEMENT.md).

## Terrain and model surfing

Steep eligible faces become airborne sliding contacts; gravity accelerates along the contact plane. A/D and mouse turns steer using the existing air-strafe rules. Space on a steep face does not create another ground jump or award a takeoff bonus. Landing on a flat surface restores normal walking/Bunnyhop. Classic, hull and recovery remain selectable; surfing is not a replacement movement mode.

| `[Surfing]` key | Default | Range / meaning |
| --- | --- | --- |
| Enabled | true | enables terrain/model surfing; false opts out |
| ModelContacts | true | use decoded stationary destructable faces when Movement.ObjectMeshCollision=true |
| MaximumAngle | 80 | 50–85 degrees; eligible faces must exceed Movement.MaxWalkSlope (46° stock) |
| ContactDistance | 8 | 0–32 Warcraft units; a small support-contact tolerance, not a distant snap |

Startup/F8 validates these controls; updates preserve custom values and explicit opt-outs. Terrain uses continuous native-walkable heightfields. Model contacts use actual transformed triangle normals and the foot footprint, including rotated imported ramps. Visual bounds alone do not create surf planes. Unsupported/animated geometry retains its fallback; ordinary classic props/native decks retain their historical policy. Ramp undersides, body/head clearance, unknown blockers, walls and real cliff transitions remain solid. The shared Bunnyhop speed cap still applies.

## Two original surf maps

**Surf Practice** offers three terrain descents at 55°, 60° and 65°, green starts and flat landings. In FPS chat use `-surf 1/2/3` to select a lane, `-reset` to return, `-auto` to toggle the enabled automatic return and `-help` for instructions. It provides an introduction before the solid-model course. See [practice guide](SURF-PRACTICE.md).

**Surf Skyline** is a single-player time trial: **18 floating solid rail sections, three stages, two turns, two air gaps, two ordered checkpoint pads and a long gold finish deck**. The timer starts on leaving the start. Land on both green pads in order, then the finish; flying above them or skipping checkpoints does not count. Falls return to the latest checkpoint. Time keeps running through falls/manual returns. The fastest full completion is retained for the current map session, not persisted between launches or sent online.

| Skyline FPS chat command | Action |
| --- | --- |
| `-cp` or `-reset` | return to current checkpoint; time continues |
| `-restart` | new full attempt; keep session best |
| `-stage 1`, `-stage 2`, `-stage 3` | practise a stage without recording a full-course best |
| `-stats` | time, checkpoints, falls, attempts and best |
| `-hud` | toggle the following timer display |
| `-auto` | toggle automatic pit return, enabled by default |
| `-help` | Russian instructions |

**F6** enters FPS, **P** toggles speed, and **Enter** opens/submits chat. Run/jump from the pad with W, then steer on a ramp's side with A/D toward its ridge and smooth mouse turns. Blue stages travel east; orange travels north. Brake/turn on checkpoint pads. The map does not overwrite saved movement tuning. Skyline requires `Surfing.Enabled=true`, `ModelContacts=true` and `Movement.ObjectMeshCollision=true`; all are stock defaults. There are no enemies/defeat triggers; the runner and course objects are invulnerable. See [Skyline guide](SURF-SKYLINE.md).

Both maps, placement, logic and procedural overviews are generated from original project source. Skyline also contains original closed MDX solids and checker textures. Builders do not read a retail map, game archive or extracted art. Stock unit/terrain identifiers reference the owner's installation. Classic Storm-compatible direct map entries are compressed by the small outer ZIP. Builders, literal/format/geometry tests and explicit authored-JASS source-audit entries ship in the source archive. This is Warcraft surfing inspired by CS, not a byte-for-byte GoldSrc simulation.

## Verification and packaging

The complete native-independent regression suite passes, including movement/camera, collision, model geometry, portals/exploded-death scopes, settings/migrations, combat, weapons, explosives, HUD/font pixels, OpenGL/Direct3D state, host profiles and Miles forwarders. New source-only fixtures exercise native registration lookup, authored waygates, crest contacts and the map builders. The classic JASS nine-fraction-digit limit has a regression after an eleven-digit angle constant was found to misplace a rotated checkpoint return.

Owned isolated **1.26a** tests verify native waygates, camera/floor relocation, Crowbar, heavy-weapon exploded deaths, field of view, all five Warcraft sky overrides/restoration, classic crest momentum and terrain surfing in **OpenGL and Direct3D**. OpenGL recordings were inspected; Direct3D claims use native measurements unless an individual validation record says otherwise. Test files and complete saves are restored by hash after the exact test process exits; campaigns are not disposable fixtures.

Surf Skyline completed every physical stage and ordered checkpoint in classic mode on both renderers. Recovery-inclusive scripted runs recorded 23.90s/three returns in OpenGL and 23.87s/two returns in Direct3D; these are verification runs, not clean competitive records. Practice finishes cannot create a full-course best, out-of-order finishes fail, checkpoint returns keep time running, and restarting retains the best. Model sliding is additionally checked at 30/60/144 Hz in all three strategies, with underside blocking and opt-outs. See [Skyline verification](SURF-SKYLINE-VALIDATION.md) and [validation history](VALIDATION.md).

Release checks audit the complete source tree, compare the exact embedded archive with the reviewed release tree, verify both EXEs and the source/version/hash-bound x86 Player runtime, reject malformed packages, and exercise setup/upgrades/reinstallation/launcher replacement. Ordinary Player modules contain no fixture input mailboxes or background-test controls. Source/distribution ZIPs contain no native modules. Original map binaries are separate authored assets; compiled project/compatibility modules remain in the separate Player EXE.

## Compatibility and known limits

Offline single player only. Native validation uses **Warcraft III 1.26a x86, Game.dll 1.26.0.6401**, OpenGL and Direct3D. **1.27a, Game.dll 1.27.0.52240**, remains experimental; there is no new real 1.27a verification. Its swimming/exploded-death and other unverified adapters retain documented fallbacks. Reforged/other patches remain unsupported. Auto still chooses OpenGL on 1.26a and Direct3D on experimental 1.27a.

The AMD RX 580 OpenGL UI failure in [issue #10](https://github.com/YazgulDev/warcraft-cs/issues/10) remains unresolved; Direct3D is available as an alternative. Not every campaign, driver, native model, custom ramp or portal script is covered. Native hull/recovery completion of the full Skyline course is unverified; their model contacts have portable regression coverage. Portals use existing map destinations and do not create campaign transitions. Native map death/campaign rules remain authoritative. The included courses and session records do not add multiplayer or online leaderboards.

<!-- launcher-summary
- Added active in-map waygates with native destinations, cooldown and camera and floor relocation.
- Added an owned-resource Crowbar to the starting kit by default with a saved opt-out.
- Changed stock AWP, G3SG1, HE and Tau damage while preserving custom tuning and explicit opt-backs.
- Added configurable native exploded deaths for hostile Gluon, Tau and ordinary RPG kills on verified Warcraft 1.26a.
- Added horizontal field of view to F2 settings with an 85-degree default and 60 to 120 range.
- Added Warcraft sky choices for automatic map skies, summer, autumn, winter, Felwood and Dalaran.
- Preserved independent Warcraft and CS sky choices and restored authored map skies across visibility and view changes.
- Changed default Bunnyhop takeoff boost to 15 percent, air acceleration to 30 and maximum speed to 3000 GoldSrc units per second.
- Preserved saved custom movement tuning and versioned deliberate legacy choices during updates.
- Fixed classic Bunnyhop speed loss on descending crests and bounded rising walkable-ramp contacts.
- Added a configurable classic ramp-contact opt-out while retaining cliffs, walls and head clearance.
- Fixed sudden Bunnyhop launches from repeated same-frame takeoff and landing bonuses.
- Added configurable gravity-driven terrain and stationary-model surfing in classic, hull and recovery.
- Added precise transformed surf faces, solid ramp undersides and model-contact opt-outs.
- Added the original three-lane Surf Practice map as a separate optional download.
- Added the original Surf Skyline time trial with floating rails, gaps, ordered checkpoints, recovery and session records.
- Updated gameplay guides, original map builders, source audits and native and portable regression coverage.
-->
