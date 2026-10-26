# Classic movement, blood criticals and cartridge cases preview

Local 0.12.0 preview extending the dual-currency/headshot feature branch. Published v0.12.0 packages and historical release notes are unchanged. The earlier features remain available: dollar/gold menu buttons, whole-price gold fallback, more accurate 1.26a model hits and localized item/rune labels. [Earlier implementation details](DUAL-CURRENCY-HEADSHOTS-PREVIEW.md).

## Movement default

`[Movement] Mode=classic` is now the default in the INI, settings structure and startup/F8 parser, including missing/empty/invalid mode values. Explicit `recovery` and `hull` remain selectable. F8 changes the strategy without resetting ordinary movement tuning or current motor state. Home recovery is active only in `recovery`. Installer updates retain explicit saved mode choices; select classic in an existing INI to opt into this default.

## Headshot feedback

The centered HEADSHOT word is removed. Confirmed head damage instead emits a short red blood spray at the accepted contact and the Cyrillic word **КРИТ** above the target's head, rising and fading in world space. The ordinary damage marker remains. Native scenery depth occludes both blood and text. Cosmetic effects never change damage, armor, passives, targeting or unit movement. No actual HP reduction means no blood or critical label. Multiple pellets/continuous ticks on the same target are coalesced briefly for readability; their damage is unaffected.

Critical labels start above the head contact at hit time and float from that position rather than following a moving target. Headshot eligibility retains its existing verified-model/1.26a limits; unverified 1.27a hit shapes cannot invent a critical location. Blood uses original procedural soft particles and the label uses the existing Unicode caption rasterizer, with no redistributed art or native text-tag modification.

## Cartridge cases

Each accepted conventional firearm round emits one spent casing from a configurable physical eye-relative ejection point. Glock bursts emit one per round; a shotgun emits one red spent shell per shot, irrespective of pellet count. Empty clicks, reloads, rejected shots, melee, grenades, RPG, Tau and Gluon produce no casings. Brass cases tumble, fall under gravity, bounce and settle, then expire. Shells use a bounded visual pool; there are no native units, items, sounds or pickup triggers for debris.

Casings render in world depth before the held weapon's separate depth pass. Their simple decorative floor is the terrain/walkable support sampled at the shot location; they do not collide with walls or participate in gameplay physics. The ejection origin is configurable in world units and is not a new authored model attachment. There is no delayed pump-animation ejection for the M3. Effects freeze with control suspension/pause and clear on map/control teardown or F8. F7/F9 never create new casings; existing world debris may remain visible in F7 until expiry.

## Editable settings

All keys below are in `[CombatVisuals]`, apply on startup/F8, and are preserved by installer updates. F8 clears current visual particles/text/cases before applying the new snapshot. Booleans accept true/false or 1/0; invalid values use defaults. Number ranges clamp; malformed/nonfinite values use defaults. Values are purely visual.

| Keys | Defaults | Range / units |
| --- | --- | --- |
| HeadshotBlood, CriticalText, ShellCasings | true | Independently enable blood, floating critical text and cases |
| BloodSeconds, BloodScale, BloodCount | 0.5, 1, 16 | .1-3 s; .25-4 size factor; 1-64 particles per burst |
| BloodSpeed, BloodGravity | 180, 400 | 0-500 world units/s; 0-2000 world units/s squared |
| CriticalSeconds, CriticalRise | 1, 40 | .2-4 s; 0-200 world units/s |
| CriticalScale, CriticalOffset | 1, 24 | .5-2 font factor; 0-128 world units above head contact |
| ShellSeconds, ShellSpeed, ShellGravity | 3, 140, 650 | .25-10 s; 20-500 world units/s; 50-2000 world units/s squared |
| ShellScale, ShellBounce, ShellMaximum | 1, 0.35, 64 | .25-4 size factor; 0-.9 rebound; 1-128 active cases |
| ShellRight, ShellUp, ShellForward | 10, -12, 20 | Each -64 to 64 world units relative to physical eye/look; negative Up is below eye |

## Verification

Portable tests exercise critical throttling, bounded pools, weapon families, burst counts, case arcs/bounces/rest, lifetime, pause-sized zero steps, frame rates and toggles. Real hidden WGL tests inspect actual world particles, Cyrillic text and brass pixels, depth occlusion, inherited raster state and context replacement. Existing movement/Warcraft collision, settings parsing/migration, Unicode item/menu, viewmodel, Glock/shotgun, headshot/contact and damage regressions cover affected contracts. Native modules and launcher packaging are built separately from private game assets.

The user's active campaign is not driven/restarted; no new live campaign smoke test or real 1.27a gameplay verification is claimed.

<!-- launcher-summary
- Added selectable dollar and gold purchases with automatic gold fallback when dollars are insufficient.
- Added configurable headshot damage, blood spray and floating critical text on verified 1.26a head contacts.
- Fixed nearby units intercepting shots through empty model bounds with precise posed surface tracing.
- Added localized first-person item and rune names under the reticle with obstacle occlusion.
- Changed the default movement physics to classic while retaining optional recovery and hull modes.
- Added falling, spinning cartridge cases for conventional firearms, including burst rounds and shotgun shells.
- Preserved native defenses, separate wallets and custom settings on update.
-->
