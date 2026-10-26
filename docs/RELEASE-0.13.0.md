## Download and upgrade

- [**WarcraftCSLauncher_DLL_Included.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.13.0/WarcraftCSLauncher_DLL_Included.exe) — recommended Player launcher with bundled project modules; no local C++ compilation.
- [**WarcraftCSLauncher.exe**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.13.0/WarcraftCSLauncher.exe) — Developer/source installation with consented dependency preparation and local compilation.
- [**WarcraftCS-0.13.0-dist.zip**](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.13.0/WarcraftCS-0.13.0-dist.zip) — standard launcher, sources, update manifest, requirements and checksum; public ZIPs contain no DLL/MIX modules.
- [Sources](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.13.0/WarcraftCS-sources.zip) · [Update manifest](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.13.0/WarcraftCS-update.json) · [Player SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.13.0/WarcraftCSLauncher_DLL_Included.exe.sha256) · [Developer SHA256](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.13.0/WarcraftCSLauncher.exe.sha256).

Save campaign progress and close Warcraft before updating, then restart. Use **Update from GitHub**, or install from the new launcher into the same dedicated mod folder. Setup preserves saves, custom configuration, audio volume, prices and opt-outs, adds missing INI keys and upgrades only prior stock weapon balances. F8 reloads settings after installation; it cannot replace an already loaded DLL. Use your own Warcraft/CS/Half-Life files; no games or converted assets are bundled.

## Warcraft CS 0.13.0 — 26 October 2026

This functional release includes every change since the immutable **v0.12.0** tag: dual-currency buying, accurate first-person contacts and headshots, item labels, blood/casings, shopping during reload, optional army escort and stronger explosives. The default movement mode is now **classic**. Experimental 1.27a support retains its existing limitations.

## Purchases and reloads

**B** shows **DOLLARS** and **GOLD** buttons above every category, plus both balances. Dollars remain the default. When dollars cannot cover the complete price, sufficient gold pays the whole price with `Economy.FallbackToGold=true`. The wallets never combine or both charge. Gold selection uses gold only; dollar kill rewards continue regardless. Failed/duplicate/full purchases spend nothing; quick ammo uses the same payment policy. Native shops retain native gold rules. Startup/F8 restores the saved currency preference. Wallet defaults remain $800 starting funds, $200 per eligible kill and a $16000 cap.

Opening/closing B and navigating or buying ammo preserves reload timing, animation, audio and shotgun insertion progress. Ammo purchases replenish real reserve. Buying and equipping a different weapon still cancels the old reload. Normal reloads transfer from current reserve at completion; shotguns keep the insertion count planned when R was pressed and credit each completed shell once. Shopping cannot convert a menu click into a shot; ordinary buying-zone and incapacitation restrictions remain.

## Precise contacts, headshots and item names

On verified 1.26a models, firearm rays trace posed opaque/cutout MDX triangles, with current skeletal sequence/time, facing, scale and render height. Empty broad-box corners and decorative effect cards no longer steal a shot aimed at a neighboring unit. The nearest accepted surface wins; hidden/cargo units, terrain and gates retain their proper rules. Melee, spells and TPS keep their existing targeting contracts.

Direct firearm contact on supported geometry influenced by a named head/skull bone earns **x2** damage by default, before native defenses/passives and friendly-fire scaling. It does not bypass zero damage or immunity. Explosions, structures, melee and unverified shapes do not invent headshots. Confirmed HP loss emits blood at contact and floating **Headshot** above the target, rising/fading with world depth occlusion. There is no centered HEADSHOT word. The text starts at the contact and does not track a moving target; rapid contacts are briefly coalesced for readability without changing damage.

F6 and clean F9 show the localized ground item/rune name under the reticle, including Unicode. Bodies, gates and terrain occlude it; cursor menus and TPS clear it. Pickup and rune effects retain their behavior.

| Section / key | Default | Range / meaning |
| --- | --- | --- |
| Economy / Currency | dollars | dollars or gold; initial/F8 menu choice |
| Economy / FallbackToGold | true | true/false; whole-price fallback only |
| Combat / PreciseUnitHits | true | true/false; false restores approximate body volumes |
| Combat / Headshots | true | true/false; direct verified head bonuses |
| Combat / HeadshotMultiplier | 2 | 1–10 damage multiplier |
| Interface / ItemNames | true | true/false; first-person ground labels |
| Interface / ItemNameDistance | 1200 | 160–5000 world units, maximum ray length |
| Interface / ItemNameRadius | 32 | 4–128 world units, aim tolerance |

## Visual settings

Each accepted conventional firearm round ejects one cosmetic case; bursts eject one per round, shotguns one per shell regardless of pellet count. Empty/rejected shots, reloads, melee, explosives, Tau and Gluon produce none. Cases tumble, fall, bounce, settle and expire in a bounded pool. Their decorative floor is the terrain/walkable support sampled at the shot location; they do not collide with walls or affect gameplay. The origin is physical eye-relative rather than a new authored model attachment; M3 ejection is not delayed until the pump animation.

All keys below are in `[CombatVisuals]` and apply on startup/F8. F8 clears old effects before applying new tuning. They freeze during control suspension/pause and clear on map/control teardown. F7/F9 do not create new cases; existing world debris may remain visible in F7 until expiry.

| Keys | Defaults | Range / units |
| --- | --- | --- |
| HeadshotBlood / CriticalText / ShellCasings | true / true / true | Independent visual toggles |
| BloodSeconds / BloodScale / BloodCount | 0.5 / 1 / 16 | .1–3 seconds / .25–4 factor / 1–64 particles |
| BloodSpeed / BloodGravity | 180 / 400 | 0–500 units/s / 0–2000 units/s² |
| CriticalSeconds / CriticalRise | 1 / 40 | .2–4 seconds / 0–200 units/s |
| CriticalScale / CriticalOffset | 1 / 24 | .5–2 factor / 0–128 units above head |
| ShellSeconds / ShellSpeed / ShellGravity | 3 / 140 / 650 | .25–10 seconds / 20–500 units/s / 50–2000 units/s² |
| ShellScale / ShellBounce / ShellMaximum | 1 / .35 / 64 | .25–4 factor / 0–.9 rebound / 1–128 active cases |
| ShellRight / ShellUp / ShellForward | 10 / -12 / 20 | Each -64 to 64 world units relative to physical eye/look |

## Movement and army policy

`[Movement] Mode=classic` is the fresh/missing/invalid-config default, retaining movement before the October collision redesign. Optional `hull` and `recovery` remain selectable; recovery retains platform protection and Home rescue. F8 switches the strategy without resetting ordinary motion/tuning. Existing saved mode choices survive upgrades, so an older explicit recovery choice stays recovery unless edited.

**U** toggles economy and defense without requiring every free soldier to follow the hero. `ArmyAI.FollowActor=false` is the new default; true restores automatic escort. Native construction, training, gathering and threat defense continue. Up to `Defenders` idle fighters may return to the actual hall; with no hall they do not rally to the actor. H/O/J/Y/RMB squad controls retain their rules. F8 applies escort policy at the next active AI decision (`IntervalMs`, default 1500 ms); opting out releases owned approaches subject to existing manual/combat/availability exclusions.

## Explosives and shotgun balance

| Weapon / key | New default | Previous published stock | Valid range |
| --- | --- | --- | --- |
| RPG / Damage | 1800 | 100 | 0–1000000 fixed peak damage |
| RPG / Radius | 600 | 525 | 32–3000 world units |
| M3 / HeroMultiplier | 6 | 3 | 0–1000 whole-shell multiplier |
| XM1014 / HeroMultiplier | 4.8 | 2.4 | 0–1000 whole-shell multiplier |
| HEGrenade / Damage | 600 | 100 | 0–1000000 fixed peak damage |

Shotgun multipliers apply in the default `Damage.Mode=unit` (or legacy hero alias), to the whole shell before splitting among pellets. Fixed `Mode=weapon` shell damage remains M3 104 / XM1014 80. HE radius remains 350. Explosions retain physical cover, distance falloff, native defenses and friendly-fire policy. F8 applies to future shots/throws/launches; live projectiles keep their original snapshot.

Flashbangs deal zero health damage but stun **every exposed living creature** for the full **two seconds** by default, including allies, neutrals and the actor. Cover/radius still gate exposure; buildings, dead, hidden, transported and already map-paused units are excluded from the new pause lease. Native incapacitation blocks the controlled actor's actions while its lease is active. Overlapping flashes extend the lease safely; map unload discards stale handles. Visual blindness keeps its separate viewer distance/facing/cover rules.

| Flashbang key | Default | Range / meaning |
| --- | --- | --- |
| Radius / BlindSeconds | 1000 / 3 | 32–3000 units / 0–10 seconds |
| UnitStunSeconds | 2 | 0–10 seconds; zero disables creature stuns |
| StunAllUnits | true | false restores enemy-only targeting excluding the viewer |
| StunDistanceFalloff | false | true scales stun time by distance |

Smoke remains non-damaging. Boolean settings accept true/false or 1/0. All new settings validate malformed/non-finite input, use documented defaults and clamp numeric bounds; installer updates preserve explicit choices.

## Stock upgrades and developer changes

Installer and startup/F8 upgrade only prior stock numbers: RPG 100 damage and 525 radius (including unversioned 375), M3 x3, XM1014 x2.4, and HE 300 (including unversioned 100). `[RPG] DamageVersion=2` / `BlastVersion=3`, `[M3]` / `[XM1014] MultiplierVersion=2`, and `[HEGrenade] DamageVersion=3` record this balance. Custom values, explicit zero and newer-marker opt-backs survive repeated updates. Prior version-2 HE damage 100 and RPG radius 375 remain deliberate choices.

Portable posed-model/ray/headshot/payment rules stay in their own modules; game-thread tracing/item queries are separate from copied UI/visual state and WGL drawing. Build manifests include the new modules, and regression suites cover authored model animation, native contact/occlusion, Unicode text, graphics state, economy, reloads, stuns and configuration. The grenade runner now treats signed abort/crash exits as failures. Release instructions require direct downloads first and a separate compact launcher change list. All source additions remain project-owned; no retail files or decompiler dumps are redistributed.

## Verification and compatibility limits

Release checks include the native-independent suite, actual hidden WGL/D3D9 fixtures, validated settings and repeated installation merges, native module build, both real launchers, source/runtime hashes and package lint. Installer/updater/reinstall and launcher replacement fixtures use synthetic temporary installations; the user's active campaign is not driven or restarted. No full campaign/custom-map or cross-driver verification is claimed.

Supported host remains offline **Warcraft III 1.26a x86, Game.dll 1.26.0.6401 (OpenGL)**. Experimental **1.27a x86, 1.27.0.52240 (D3D9)** retains its partial source adapter and synthetic host/backend checks; **no real 1.27a gameplay test** is claimed. Precise posed headshots and item names currently require verified 1.26a adapters; 1.27a keeps approximate targeting and lacks these labels/bonuses. Models without usable named heads do not receive a fabricated bonus. Native animation blends/look-at overrides and texture cutout pixels are not reproduced, so tracing is not pixel-perfect. Reforged, other patches and multiplayer remain unsupported.

<!-- launcher-summary
- Added selectable dollar and gold purchases with automatic gold fallback when dollars are insufficient.
- Added configurable headshot damage, blood spray and floating Headshot text on verified 1.26a head contacts.
- Fixed nearby units intercepting shots through empty model bounds with precise posed surface tracing.
- Added localized first-person item and rune names under the reticle with obstacle occlusion.
- Changed the default movement physics to classic while retaining optional recovery and hull modes.
- Added falling, spinning cartridge cases for conventional firearms, including burst rounds and shotgun shells.
- Preserved native defenses, separate wallets and custom settings on update.
- Changed the default HE grenade peak damage from 100 to 600 with one-time stock configuration migration.
- Changed flashbangs to stun every exposed creature for two seconds, including allies and the controlled actor.
- Added configurable enemy-only flash targeting and optional distance-based stun duration.
- Fixed buy-menu toggles interrupting reloads and menu navigation freezing reload progress.
- Changed army AI to manage economy and defense without automatically rallying troops to the hero.
- Added an optional ArmyAI FollowActor setting to restore automatic hero escort.
- Changed RPG defaults to 1800 damage and a 600-unit blast radius.
- Changed shotgun native attack multipliers to M3 x6 and XM1014 x4.8.
- Preserved custom weapon tuning with one-time stock balance upgrades and live F8 reload.
-->
