# Buying during reload and optional army escort preview

Local 0.12.0 preview extending the [Headshot and grenade preview](HEADSHOT-GRENADES-PREVIEW.md). Includes all earlier preview features: dollar/gold payment, precise 1.26a hits, item names, classic movement default, blood, cases, stronger HE and all-target flash stuns. Published stable packages and historical notes are unchanged.

## Reload and shop

Open/close **B**, navigate categories and buy ammunition while reloading. The reload retains its absolute deadline, animation and sounds; M3/XM1014 keep inserting shells while the menu owns input. Successful ammunition purchases return to play and add reserve. Failed purchases leave the reload intact. Fire input stays suppressed while shopping and cannot turn a menu click into a shot.

Buying and automatically equipping a different weapon still cancels the previous weapon's reload. Shop access, prices, wallets, full-ammo validation and native Warcraft shop rules retain their normal restrictions. No purchase bypasses incapacitation or buying zones.

A normal magazine reload uses the current reserve when its deadline completes. A shotgun retains the insertion count planned when R was pressed; buying reserve during that reload does not lengthen or restart its fitted animation. Each insertion spends one actual reserve round once. Newly purchased excess reserve remains for the next reload.

## U army policy

**U** toggles economy/base assistance without requiring every free fighter to follow the hero. Workers, construction and training retain their native resource/placement/tech rules. Visible threats still trigger defense. Up to `Defenders` idle fighters may return to the actual hall; without a hall they do not rally to the actor. Other free fighters remain in place. Explicit H/O/J/Y/RMB squad controls keep their existing rules.

`[ArmyAI] FollowActor=false` is the new default. **true** restores automatic escort, with reserves at the base and other available soldiers approaching the controlled actor. Accepts true/false or 1/0; missing/empty/malformed values default to false. **F8** applies on the next active AI decision, bounded by `IntervalMs`. An opt-out releases outstanding assistant approach orders subject to the same availability/manual/native-combat exclusions; it does not cancel queued construction, training or harvesting. Updates insert missing keys once and preserve an explicit preference.

## Verification and limits

Synthetic native-order fixtures exercise default base-only reserves, no-hall behavior, optional escort, live opt-out, manual/combat/squad exclusions, defense and economy. Purchase/shell timeline tests exercise reserve buying during a reload and exactly-once shell credits. Real settings-loader and installer merge tests check validated defaults, true/false choices and repeated update preservation. Both native hosts and launcher packages are built and checked separately.

The user's active campaign is not driven or restarted. No live campaign smoke test or real 1.27a gameplay verification is claimed.

<!-- launcher-summary
- Added selectable dollar and gold purchases with automatic gold fallback when dollars are insufficient.
- Added configurable headshot damage, blood spray and floating Headshot text on verified 1.26a head contacts.
- Fixed nearby units intercepting shots through empty model bounds with precise posed surface tracing.
- Added localized first-person item and rune names under the reticle with obstacle occlusion.
- Changed the default movement physics to classic while retaining optional recovery and hull modes.
- Added falling, spinning cartridge cases for conventional firearms, including burst rounds and shotgun shells.
- Preserved native defenses, separate wallets and custom settings on update.
- Changed the default HE grenade peak damage from 100 to 300 with one-time stock configuration migration.
- Changed flashbangs to stun every exposed creature for two seconds, including allies and the controlled actor.
- Added configurable enemy-only flash targeting and optional distance-based stun duration.
- Fixed buy-menu toggles interrupting reloads and menu navigation freezing reload progress.
- Changed army AI to manage economy and defense without automatically rallying troops to the hero.
- Added an optional ArmyAI FollowActor setting to restore automatic hero escort.
-->
