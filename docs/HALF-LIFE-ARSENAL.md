# Optional Half-Life arsenal

Development feature after 0.11.0. Run setup again with your owned CS 1.6 directory to
convert sibling `valve` content (loose files or `pak*.pak`). Each weapon requires its
own valid model, preview, animation timeline and sounds. Missing/incomplete content
hides only that weapon. Only Warcraft installations exclude these weapons.

## Buying and selection

**B → 9 HALF-LIFE → 1 Crowbar / 2 Tau Cannon / 3 Gluon Gun / 4 RPG**. Defaults cost
200 / 5000 / 6000 / 4000 gold. RPG moves out of Equipment. Rejected/duplicate purchases
never charge gold. All carried Half-Life weapons appear in group **6 HALF-LIFE**, in
this order, and in sequential wheel selection. Click to confirm. Turn Tab item numbers
off to use 1–6 for weapons. Custom bindings and the optional legacy direct-slot mode
remain available. Crowbar is purchased rather than added to the starting knife/sword
kit. F12 grants every available weapon; F11 refills owned ones. Existing IDs stay stable.

## Attacks

- **Crowbar:** hold LMB to swing; RMB also swings when weapon secondary actions own
  that button. Contact is delayed until the swing and uses native contact/friendly-fire
  rules, with distinct body and hard-surface sounds. No ammunition.
- **Tau Cannon:** LMB spends two cells for a semi-automatic beam. Hold RMB to charge,
  release to fire once. Starting charge costs one cell, then one per charge-drain
  interval until full charge. Exhaustion discharges once. Damage interpolates from
  ordinary to charged damage. Switching, menus, stuns, casts, F8 and focus loss cancel
  without firing; spent cells stay spent. Disable F6 squad RMB orders with Y to use
  charged fire. Damage is fixed, independent of hero damage mode.
- **Gluon Gun:** hold LMB for a continuous blue beam. Each accepted tick spends one
  cell. Release, exhaustion or loss of attack ownership stops its sound and beam.
  Native target/wall tests run at each accepted damage tick; damage is fixed.

Tau and Gluon use **separate** cell pools, capacity 100 each. Their ammo packs cost
300 gold and add 20 usable cells directly, capped at capacity. R does nothing; full
pools never charge for ammo. Runes and F11/F12 use the same usable capacity, without
hidden reserves. RPG's controls/physics are unchanged; its new purchase path is B/9/4
and carried group 6 (see [RPG settings](HALF-LIFE-RPG.md)).

These are Warcraft adaptations. Native immunities, attack effects, gates, terrain,
friendly fire and map XP ownership apply. Energy stops at the first valid solid contact.
Tau wall penetration, ricochets, propulsion and overcharge self-damage, Gluon splash,
and GoldSrc underwater rules are not implemented. Audio uses owned samples at normal
pitch. Original beam geometry is project-authored; retail models, animations, barrel
attachments and previews are converted locally rather than redistributed.

## Configuration

Edit `WarcraftCS/WarcraftCS.ini`, then F8 or restart. Updates add missing keys without
replacing custom values. F8 cancels charge/channel and rechecks seals; it never grants
ammo. Existing CS/RPG controls and settings are preserved.

| Section / key | Default | Units and accepted bounds |
|---|---:|---|
| Crowbar / Enabled, TauCannon / Enabled, GluonGun / Enabled | true | true/false; still requires owned files |
| Crowbar / Damage, HeroMultiplier | 10, 1 | fixed damage 0–1000000; hero multiplier 0–1000 |
| Crowbar / Price | 200 | gold 0–1000000 |
| Crowbar / Range | 150 | world units 32–1000 |
| Crowbar / FireInterval, ContactSeconds | 0.5, 0.1 | seconds 0.1–5 / 0–1; authored clip can extend recovery |
| TauCannon / Damage, ChargedDamage | 20, 200 | fixed damage 0–1000000 |
| TauCannon / ChargeSeconds | 4 | seconds 0.1–10 to full power |
| TauCannon / ChargeDrainSeconds | 0.3 | seconds per cell 0.05–2 plus the starting cell |
| TauCannon / PrimaryCost | 2 | cells 1–100 per primary shot |
| GluonGun / Damage | 14 | fixed damage per tick 0–1000000 |
| TauCannon and GluonGun / Magazine | 100 | usable cells 1–1000; reserves always zero |
| TauCannon / Price, GluonGun / Price | 5000, 6000 | gold 0–1000000 |
| TauCannon and GluonGun / AmmoPrice, AmmoPack | 300, 20 | gold 0–1000000, cells 1–1000 |
| TauCannon / FireInterval, GluonGun / FireInterval | 0.2, 0.1 | seconds 0.025–10 |
| TauCannon and GluonGun / Range | 30000 | world units 100–50000, subject to firearm reach floor |
| All three / MoveSpeed, ModelScale | 250, 1 | movement 50–500 units/s, viewmodel scale 0.25–2 |
| TauCannon / Kick, GluonGun / Kick | 2, 0 | degrees 0–10 |
| HalfLife / BeamSeconds, BeamWidth | 0.15, 3 | seconds 0.025–1, world units 0.5–20 |

The portable energy state owns spending/cadence; the controller applies native contact
and audio events. Beam snapshots are read-only and never deal damage from rendering.
Movement bob and accepted-shot muzzle flashes use the common viewmodel path.
