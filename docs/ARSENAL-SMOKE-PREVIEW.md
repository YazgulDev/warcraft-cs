# Expanded CS arsenal and dense smoke preview

Preview built on 0.16.0; this is not a new published release.

**B → 3 SMGs** now includes **3 TMP** and **4 UMP45**. **B → 4 Rifles** adds **4 FAMAS**, **5 Galil** and **6 G3SG1**. Existing weapon/menu IDs remain intact. Dollar payment with gold fallback remains the default. Weapons have private owned-CS models, hand-free buy previews, draw/fire/reload animation and audio, physical shot cadence, muzzle attachments and shell casings. F11 refills owned guns and F12 grants the complete available arsenal. The carried PRIMARY list scrolls its visible rows around selection to fit the expanded arsenal.

| Weapon | Magazine / max reserve | Shot interval, s | Reload, s | Weapon damage / native attack multiplier | Price |
| --- | --- | --- | --- | --- | --- |
| TMP | 30 / 120 | 0.07 | 2.125 | 20 / 0.65 | 1250 |
| UMP45 | 25 / 100 | 0.1 | 3.5 | 30 / 0.9 | 1700 |
| FAMAS | 25 / 90 | 0.0825 | 3.3 | 30 / 1 | 2250 |
| Galil | 35 / 90 | 0.0875 | 2.45 | 30 / 1 | 2000 |
| G3SG1 | 20 / 90 | 0.25 | 4.7 | 80 / 2.2 | 5000 |

Hold LMB for automatic fire; G3SG1 is the fast self-loading sniper, with RMB cycling normal view and two scope levels. FAMAS uses continuous automatic fire in this preview. Y may assign RMB to squad orders instead. AWP's optional one-shot finishing rule applies only to AWP. New CS weapons do not depend on optional Half-Life resources.

New `[TMP]`, `[UMP45]`, `[FAMAS]`, `[Galil]`, `[G3SG1]` sections use the same validated controls as existing firearms: Magazine 1–1000; FireInterval .025–10 s; ReloadSeconds .1–30 s; Range 100–50000 world units (Combat.MinimumFirearmRange can raise effective range); MoveSpeed 50–500 CS units/s; ModelScale .25–2; Kick 0–10 degrees; Price/AmmoPrice 0–1000000 in selected currency; AmmoPack 1–1000; MaxReserve 0–100000; Damage 0–1000000; HeroMultiplier 0–1000. F8 applies future fire/reload tuning and clamps capacities without granting ammunition. Active reload deadlines retain the existing snapshot. Install/update converts the new resources from the player's own complete cstrike directory; no retail assets are bundled.

Smoke uses depth-tested volume slices instead of sparse translucent puffs. Long paths through smoke become almost opaque; very close surfaces remain readable. Overlapping clouds compound. The overlay hides health bars, item names and targeting rings behind dense smoke. Bullets and player blind shots retain their physical traces.

| `[SmokeGrenade]` key | Default | Meaning |
| --- | --- | --- |
| Opacity | 0.99 | 0–1 density multiplier, zero disables both visual extinction and AI obstruction. |
| VisibilityDistance | 96 | 16–600 world units of smoke leaving approximately 1% contrast at full density. AI's visibility threshold is 10% contrast, roughly 48 units at stock density. |
| BlocksAI | true | true/false or 1/0. On verified 1.26a, all teams' native basic-attack acquisition, retained windups/contacts and explicit map basic-attack orders obey the smoke line of sight. |

Radius remains 300 (32–1000), DurationSeconds 18 (1–60). Launch snapshots include density, short visibility and AI policy: startup/F8 changes subsequent grenades; active clouds finish with their original values. Smoke neither pauses creatures nor changes their saved sight/acquisition statistics, so exiting/expiry/reset leaves no stat leases. Units can still move. Warcraft minimap fog, area spells, already launched missiles and map-authored damage remain native; this is not a global fog-of-war rewrite. 1.27a retains dense visual smoke but its unverified AI attack internals do not receive the targeting guard.

Updates insert missing weapon and smoke keys while preserving custom settings, including an existing Opacity=0.85.

Verification uses a separate owned Warcraft III 1.26a lab, with modules, settings, caches, graphics bridge and saves restored by hash afterward. OpenGL and Direct3D runs bought each new gun through the real menu, fired held bursts, matched shot serials to spent rounds, completed reserve-funded reloads and cycled both G3SG1 zoom levels. Native smoke checks rejected hostile attack orders, permitted actual blind player damage and restored attack acceptance after expiry. OpenGL screenshots show all new models, reloads and smoke; Direct3D screenshots include TMP/menu/HUD, while Windows Graphics Capture timed out on later frames (the native assertions continued). Portable and hidden graphics regressions cover smoke extinction/depth, all-team hook policy/manual-contact scope, buying, selection, casing eligibility, optional guest bounds, config validation and update preservation. 1.27a was not tested.

<!-- launcher-summary
- Added TMP and UMP45 SMGs, FAMAS and Galil rifles, and a scoped G3SG1 automatic sniper.
- Added private weapon models, animations, reload audio, muzzle flashes, casings and buy previews for the new guns.
- Changed smoke to a dense depth-tested volume with configurable short visibility and all-team AI attack occlusion on Warcraft 1.26a.
- Fixed health bars, item names and target markers revealing targets through dense smoke.
- Fixed carried-weapon browsing to keep long primary lists within the screen.
-->
