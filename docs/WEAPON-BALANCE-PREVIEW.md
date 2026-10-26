# RPG, shotgun and HE balance preview

Local 0.12.0 preview extending the [reload/army preview](RELOAD-ARMY-PREVIEW.md). Includes its shopping-during-reload fix, optional army escort and all earlier preview features. Stable published packages are unchanged.

## New defaults

| Weapon / setting | Previous stock | New default | Range / units |
| --- | --- | --- | --- |
| RPG Damage | 100 | 1800 | 0–1000000 fixed peak damage |
| RPG Radius | 525 | 600 | 32–3000 world units |
| M3 HeroMultiplier | 3 | 6 | 0–1000 whole-shell native attack multiplier |
| XM1014 HeroMultiplier | 2.4 | 4.8 | 0–1000 whole-shell native attack multiplier |
| HEGrenade Damage | 300 | 600 | 0–1000000 fixed peak damage |

M3/XM1014 use these multipliers in the default `[Damage] Mode=unit` (also the legacy hero alias). A unit with 100 average attack produces a whole-shell base of 600/480, divided among pellets before hit/armor/friendly-fire rules. Fixed `Mode=weapon` damage remains M3 104 / XM1014 80. HE radius remains 350. Flashbang/smoke health damage remains zero; the existing two-second flash area stun remains.

Explosions still respect physical cover, distance falloff, native defenses and friendly-fire policy. F8 changes future shots/throws/launches; rockets and grenades already in flight keep their launch snapshot.

## Upgrade and customization

The installer and actual startup/F8 loader upgrade only previous stock numbers. RPG gains `DamageVersion=2` and `BlastVersion=3`; HE uses `DamageVersion=3`; M3/XM1014 each use `MultiplierVersion=2`. Custom damage/radii/multipliers and explicit zero are preserved. Prior version-2 HE damage 100 and RPG radius 375 remain deliberate choices. After the markers are current, even an intentional return to the old stock number survives subsequent updates. Unversioned HE 100 and RPG radius 375 pass through their historical stock upgrades to 600. Other prices, reload times, pellet counts and ammunition caps retain their settings.

All parameters remain editable in `WarcraftCS/WarcraftCS.ini`. Damage accepts 0–1000000, RPG radius 32–3000 world units, multipliers 0–1000. Missing/empty/malformed/non-finite values use defaults; out-of-range numbers clamp safely. Repeated installer updates add missing keys once and retain unrelated preferences.

## Verification and limits

The real settings parser and contact-damage rules cover defaults, old stock, current-version opt-backs, custom/zero/malformed values and bounds. Repeated installer merges cover the same stock/custom cases and preservation of unrelated audio preferences. Native grenade/RPG fixtures exercise the 600-damage HE and 1800-damage/600-radius RPG launch snapshots and cover/eligibility behavior; RPG flight/conversion and real build/package checks cover the unchanged projectile path.

The active campaign is not driven or restarted. No live campaign or real 1.27a gameplay verification is claimed.

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
