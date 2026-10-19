# Optional Half-Life RPG

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
