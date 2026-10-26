# Headshot caption and stronger grenades preview

Local 0.12.0 preview extending the [classic movement/blood/casings build](CLASSIC-BLOOD-CASINGS-PREVIEW.md). Includes the earlier dollar/gold purchases, accurate 1.26a hits, item names, classic movement default, blood and spent cases. Published v0.12.0 packages and historical release notes are unchanged.

## Floating caption

The floating word above a confirmed head contact now reads **Headshot**, replacing the previous Cyrillic critical caption. Its world anchor, blood spray, fading/rise, cover occlusion and `[CombatVisuals] CriticalText` toggle remain unchanged. There is no centered headshot word on the reticle. Existing verified-model/1.26a eligibility limits still apply.

## HE damage

HE peak damage rises from **100 to 300**. `[HEGrenade] Damage` remains editable from 0 to 1000000; `Radius=350`, linear distance falloff, physical cover, native armor/immunity and friendly-fire scaling retain their behavior. Flashbangs and smoke continue dealing zero health damage. RPG and C4 damage are unchanged.

`[HEGrenade] DamageVersion=2` records the stock upgrade. The installer and startup/F8 loader upgrade only an old unversioned/version-1 `Damage=100` to 300. Custom values (including zero) are preserved. An intentional `Damage=100` with version 2 remains 100 on later updates. New installs and missing values default to 300. F8 affects future throws; launched grenades retain their damage/settings snapshot.

## Flash area stun

Each exposed living creature in the flash radius now receives a **full two-second stun** by default. This includes enemies, allies, neutral creatures and the thrower/controlled actor, regardless of look direction or friendly-fire percentage. Physical cover and radius still gate exposure; buildings, dead, hidden and transported units do not qualify. Existing map pauses are never acquired or released by this adapter. A flash deals no health damage.

The player's visual blindness retains its separate distance/facing/cover rules. The controlled unit's ordinary paused/incapacitated status blocks movement, attacks and casting during its stun. The flash's simulation timer still runs, allowing the lease to expire. Overlapping flashes extend the same lease without duplicate pauses or early release. Removing a thrower after release does not cancel the default area stun. Map unload discards leases without dereferencing stale handles.

| Flashbang key | Default | Meaning / range |
| --- | --- | --- |
| Radius | 1000 | 32-3000 world units; cover blocks exposed paths |
| BlindSeconds | 3 | 0-10 seconds of maximum viewer blindness before exposure scaling |
| UnitStunSeconds | 2 | 0-10 seconds; zero disables creature stuns |
| StunAllUnits | true | true/false or 1/0; false restores enemy-only stun excluding the viewer |
| StunDistanceFalloff | false | true/false or 1/0; true scales stun time by distance, false uses the full configured time |

Settings reload with **F8 for subsequent throws** and are preserved by installation updates; already-flying grenades and existing stun deadlines retain their snapshot. Missing/malformed booleans use the documented defaults.

## Verification

Production native-adapter fixtures cover the stronger HE blast and launch snapshot; flash stun on the controlled actor, enemies, allies and neutrals; dead/hidden/cargo/building exclusion; pre-paused ownership; full/optional falling duration; repeated simulation time; overlap extension; cover; zero duration; legacy targeting and removed throwers. Real config parsing and repeated installer merges cover stock upgrade, versioned/custom damage and saved flash opt-outs. The existing real hidden WGL combat-effects oracle verifies the floating caption and world depth/state. Native modules and both launcher payloads are checked separately.

The active campaign is not driven or restarted. No live campaign smoke test or real 1.27a gameplay verification is claimed.

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
-->
