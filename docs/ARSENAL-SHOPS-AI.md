# Arsenal, shops and army assistant

The new paid firearms are Desert Eagle (650 gold), Glock 18 (400), MP5 (1500),
P90 (2350), M249 (5750), M3 (1700), and XM1014 (3000). Open **B**: 1 pistols,
2 shotguns, 3 SMGs, 4 rifles, 5 machine guns, 6 primary ammo, 7 pistol ammo,
8 equipment. Number keys and mouse rows select the same catalog entries.
Buying grants a loaded magazine; reserve packs cost extra. Duplicates, insufficient
funds and full packs do not charge gold. Existing slots **1–7** keep their meanings;
select additional owned weapons with the wheel. Tab toggles item-number routing.
F11 refills owned weapons; F12 grants all weapons. The initial kit is unchanged.
Models, sounds and hand-free previews are converted privately from owned CS files.

The new INI sections `[Deagle]`, `[Glock]`, `[MP5]`, `[P90]`, `[M249]`, `[M3]`,
`[XM1014]` expose `Price`/`AmmoPrice` in native gold (0–1000000), `AmmoPack`
(1–1000 rounds), `MaxReserve` (0–100000), `Magazine` (1–1000), `Damage`
(0–1000000, weapon mode) and `HeroMultiplier` (0–1000, hero mode). Shotgun damage
is a shell total, divided across `[Shotguns] Pellets=8` (1–16), with
`SpreadDegrees=5` cone radius (0–15 degrees). Targets intercept pellets independently.

`FireInterval` is seconds between shots (.025–10), `ReloadSeconds` seconds per full-magazine
reload (.1–30), `Range` world units (100–50000), `MoveSpeed` CS units/second
(50–500), `ModelScale` viewmodel projection scale (.25–2), `Kick` vertical impulse
(0–10 degrees, new firearms). F8 reloads settings; tighter capacities clamp surplus
without granting ammo. Existing reloads finish their current deadline; the next
reload uses the new duration. M3/XM1014 calibrate start, insertion and finish speed
against a completely empty magazine. Partial reloads keep the same insertion speed
and finish sooner, with one insertion per missing/reserved shell. They hold the current clip's terminal pose
between updates, without idle flashes. Each completed insertion transfers **one**
round from reserve into the magazine immediately, so the HUD updates shell by shell.
Press **LMB** to interrupt and fire a loaded round when the weapon's firing cooldown
is ready. An unfinished insertion consumes no ammo; completed shells are retained.
With an empty magazine, the reload continues until a shell is available to fire.
Holding LMB from before an automatic reload does not repeatedly cancel it.
Model timing remains available when the optional audio device fails to initialize.
Shell-loading sound is supplied when the private model lacks an event (including
the retail XM1014); authored M3 events are retained without double playback.
Reload samples are required during private conversion. `[Audio] CSVolumePercent`
continues to control shots and reloads together, including mute.

`[Shotguns] ShellByShellReload=true` and `InterruptReloadOnFire=true` accept
true/false or 1/0. F8 applies these policies to the **next** reload, preserving
the running reload's accounting. Set the first to false for magazine-at-completion
transfer, or the second to false to disable interruption by firing. Updates preserve
explicit custom preferences. Existing `ReloadSeconds` still sets full-magazine timing.

All firearms now use `[Combat] MinimumFirearmRange=30000` world units (0–50000).
Positive values extend each gun to at least that distance and twice
`[Rendering] DrawDistance`, covering the visible camera volume. This applies to
old saved short `Range` values too. Set it to **0** to restore individual weapon
ranges. F8 applies the shared and individual ranges immediately; upgrades preserve
custom values and insert missing settings once. Spread/recoil, target visibility,
terrain and intervening bodies still determine hits. Knife/sword reach and C4 splash
retain their physical rules. Empty firearm rays skip unnecessary long terrain walks.

## Glock mouse actions

**LMB** fires one round with the ordinary single-shot animation. **RMB** fires one
three-round burst per deliberate press, with one round, shot sound and damage
contact per bullet. A short magazine limits the burst to the rounds available.
The burst animation starts once. Holding either button cannot repeat its action.
**Y** still assigns RMB to squad commands in F6; then RMB cannot start a Glock burst.
Reloading, switching weapons/views, inventory/shops, menus, spells, losing attack
control or focus cancel remaining queued bullets.

`[Glock] SecondaryBurstRounds=3` (1–5), `SecondaryBurstInterval=0.1` seconds
(.025–1) and `SecondaryCooldown=0.5` seconds (.1–5, from first shot) are editable.
F8 applies them to the next burst; a running burst retains its spacing. The existing
attack-speed scaling option also scales that spacing. Ordinary LMB uses `FireInterval`.

## Native shops in F6/F7

Look at a friendly/neutral shop within **450 world units** and press **E**. Its
native command card opens with native buyer selection, stock, prices and map
requirements. Buy by clicking its buttons. **E/Escape** restores the same actor
and F6/F7 view. F6/F7/F9 while shopping also close the shop and restore the captured
view. Camera/weapon input yields to the shop; the world keeps running. Death,
ownership loss or leaving range closes the interaction.

`[Shops] Enabled=true`, `Radius=450` (100–1200 world units) reload with F8. The ray
must hit the actual shop before terrain, a unit or a destructable. Without a
qualifying shop, E retains item/rune pickup. Native shop abilities cover racial
and custom shops and neutral merchants/camps. No stock is invented.

## Army AI on U

**U** toggles the assistant for the current control session. **Y** retains the F6
RMB-order toggle. `[Shortcuts] ArmyAI=U` accepts existing key syntax or NONE. A saved
custom action using U keeps it on upgrade; the new AI binding becomes NONE.

Only living, available, **owned** units other than the actor and H/O members are
eligible. Explicit RMB targets take priority. J releases H/O membership, making
those units eligible again. Units fighting or executing other active orders retain
their orders. Idle fighters reserve some units at base and send the rest toward
the actor with native attack-move. Visible base threats call idle units into defense.

Stock workers/halls select a basic human, orc, undead or night elf build plan.
AI requests a missing hall, supply when food is tight, a basic barracks, workers
and basic soldiers. Idle workers gather native gold/lumber. Warcraft orders enforce
real costs, placement, technology, food, campaign rules and triggers. The assistant
does not research upgrades or expand into advanced technology. Undead gold gathering
requires an existing haunted mine; custom resource systems need compatible native
orders. Mixed captured races use one basic economy plan. Maps without suitable
workers/halls still get combat support and defense.

All `[ArmyAI]` settings reload with F8:

| Key | Default | Range / meaning |
| --- | --- | --- |
| Enabled | true | Enables U; automation starts off |
| IntervalMs | 1500 | 500–10000 milliseconds between decisions |
| ProductionRetrySeconds | 180 | 30–3600 active decision seconds before retrying canceled/unobserved training; pauses/background time do not count |
| Workers | 6 | 0–30 desired workers |
| Soldiers | 24 | 0–100 desired basic soldiers |
| Defenders | 3 | 0–100 idle fighters reserved at base |
| SupplyBuildings | 6 | 0–30 maximum basic supply buildings |
| BaseRadius | 900 | 100–3000 world units, threat scan |
| SupportDistance | 350 | 100–1500 world units, arrival/refresh spacing |
| BuildRadius | 450 | 192–1500 world units, construction sites |
| ResourceRadius | 1600 | 300–5000 world units, resource search |
| Worker/Hall/Supply/Barracks/Soldier | empty | Five four-character rawcodes select a custom coherent plan |

Incomplete custom plans fall back to stock detection. Rejected builds retry native
candidate sites without relocating buildings or clearing trees. U off stops new
decisions; construction, training, gathering and issued orders continue natively.
Leaving control/unloading resets AI off. Pauses, cinematics and lost focus suspend
new decisions. During a native shop AI continues while purchases remain manual.

Accepted training reserves one pending unit even when Warcraft reports an empty building
order. Observed population growth releases those reservations; hidden mining workers and
garrisons remain counted. Canceled/unobserved training eventually retries at the configured
decision-time limit. Map callbacks are followed by a fresh ownership/exclusion check before
the next order. Reinforcements stop within SupportDistance while fighting units retain combat.
Changing the retry duration applies to newly accepted training reservations.
