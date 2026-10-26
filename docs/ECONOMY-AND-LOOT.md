# Dollar economy and rare weapon loot

Available in release 0.12.0. Older 0.11.0 packages do not include this economy.

The default B menu and quick-ammo purchase use a separate CS dollar wallet.
Each player's wallet starts with $800 once per map, earns $200 per confirmed
hostile creature killed with CS/Half-Life firearms/melee, HE, C4 or RPG, and holds at most
$16000. Native Warcraft gold, map resource triggers, loot tables and item slots
are preserved. F6/F7/F9, switching units, free-refill shortcuts and F8 do not
grant starting money again. Balances are map-local; they are not saved into
Warcraft campaign saves or carried across maps/restarts.

The local [dual-currency preview](DUAL-CURRENCY-HEADSHOTS-PREVIEW.md) adds **DOLLARS** and **GOLD** buttons above every B menu category and shows both balances. Dollars remain the default. If dollars cannot cover a whole purchase, sufficient gold pays the whole price (`FallbackToGold=true`). The wallets never combine; gold selection spends only gold. Dollar rewards continue while gold is selected. The menu choice is session-local; startup/F8 restores the saved preference. Quick-ammo follows the same selection/fallback, while native Warcraft shops keep their own rules.

Set `[Economy] Currency=gold` to use the original Warcraft gold purchasing
behavior. Prices in weapon sections apply to the selected currency. Gold mode
does not add CS kill rewards to native resources. Switching back to dollars
resumes the retained dollar wallet; it does not convert gold or reset funds.

Each eligible kill independently has a **0.5%** weapon-drop chance by default:
about one drop per 200 kills on average, without a guaranteed interval. The
pool contains enabled firearms, including optional RPG, Tau and Gluon only when
their owned assets are available. Duplicate Tau/Gluon pickups supply usable cells
directly rather than an invisible reserve pool. Melee weapons, C4 and grenades do not enter this pool.
Allies, neutral non-enemies, buildings, nonlethal contacts and already-dead
targets cannot award money or roll loot. Native unit attacks, spells and
squad/RTS kills are outside this initial CS damage reward adapter.

A stock treasure chest marks a dropped weapon. Walk within 160 world units to
see the weapon's name in the pickup hint, then press E. A new weapon grants a
loaded magazine and selects it in armed FPS. An owned weapon instead grants
one configured ammo pack, capped at its reserve capacity. A full or disabled
weapon leaves the drop intact. Pickup uses the CS inventory even when the unit
has no native Warcraft item slots; it does not fire native item-pickup triggers.
The marker uses Warcraft's own chest model and carries no redistributed art.

Edit the usual `Game/WarcraftCS/WarcraftCS.ini` (EXE install), or
`WarcraftCS/WarcraftCS.ini` inside the prepared source runtime. Press F8 in FPS
to reload. Updates add missing keys and preserve custom values.

| Section / key | Default | Bounds / timing |
| --- | --- | --- |
| Economy / Currency | dollars | dollars or gold; F8 |
| Economy / FallbackToGold (local preview) | true | true/false; whole-price gold fallback on insufficient dollars, F8 |
| Economy / StartingMoney | 800 | Whole dollars, 0..1000000, limited by MaximumMoney; next new map/player wallet |
| Economy / KillReward | 200 | Whole dollars, 0..1000000; subsequent kills after F8 |
| Economy / MaximumMoney | 16000 | Whole dollars, 0..1000000; F8 clamps existing balances, increasing it grants no money |
| WeaponDrops / Enabled | true | true/false; F8 disable removes ground drops |
| WeaponDrops / ChancePercent | 0.5 | 0..100 percent per eligible kill; zero disables future rolls |
| WeaponDrops / LifetimeSeconds | 120 | 1..3600 simulation seconds for new drops; existing expiry retained |
| WeaponDrops / PickupRadius | 160 | 16..400 world units; F8 |
| WeaponDrops / MaximumDrops | 64 | 1..256; F8 removes excess drops, at the cap new drops are skipped |

Drop expiry follows Warcraft simulation time and freezes during pause. Changing
views retains drops; unloading the map clears them. Lowering the dollar cap
can discard money above it; raising the cap never restores that discarded money.

The RPG retains its default peak damage **100** and now has a **525** world-unit
blast radius, increased by 150. Damage still falls with distance, cover and native
immunity, and respects friendly-fire settings. F8 affects future rockets; live
rockets retain their launch snapshot. Install/update and direct F8 migrate the old
stock radius 375 once using `[RPG] BlastVersion=2`. Custom radii/damage survive;
to explicitly choose 375, retain version 2 and set `Radius=375`.

Automated checks exercise production wallet transactions, probability boundaries,
native death filtering and marker lifecycles with synthetic game APIs, actual INI
reload and config migration. A live-map screenshot and manual pickup/buy-menu check
remain necessary before claiming in-game validation on a particular map.
