# Arsenal, shops and army assistant

[Project home](../README.md) · [Documentation](README.md) · [Settings](CONFIGURATION.md)

In the local [dual-currency preview](DUAL-CURRENCY-HEADSHOTS-PREVIEW.md), **B** shows dollar and gold buttons above every category. Click the currency to select it; dollars are the default. When dollars cannot cover the whole price, the purchase uses gold if sufficient (`[Economy] FallbackToGold=true`). The wallets never combine. Selecting gold spends only gold. Both balances stay visible and dollar kill rewards continue in either mode. Native Warcraft shops keep their own gold rules. **F8** restores the saved `Economy.Currency` preference.

The [current balance preview](WEAPON-BALANCE-PREVIEW.md) doubles whole-shell native attack multipliers: M3 **x6**, XM1014 **x4.8**. These apply in `Damage.Mode=unit`/`hero`; the fixed weapon-mode shell damage stays 104/80. HE now defaults to **600** peak damage; flash and smoke still deal zero health damage. F8 applies to subsequent shots/throws, and updates preserve custom tuning.

The paid firearms are Desert Eagle (650), Glock 18 (400), MP5 (1500),
P90 (2350), M249 (5750), M3 (1700), and XM1014 (3000). Open **B**: 1 pistols,
2 shotguns, 3 SMGs, 4 rifles, 5 machine guns, 6 primary ammo, 7 pistol ammo,
8 equipment. Number keys and mouse rows select the same catalog entries.
In the [current preview](RELOAD-ARMY-PREVIEW.md), B and quick ammo remain available while reloading. Opening/closing B or buying ammunition preserves the reload clock, sounds and shell insertions; purchasing and equipping a different weapon cancels the old reload. Shotguns keep the number of insertions planned when R was pressed; newly purchased reserves remain available afterward.

Buying grants a loaded magazine; reserve packs cost extra. Duplicates, insufficient
funds and full packs do not charge either wallet. Inventory slot IDs remain stable. The
[carried weapon menu](WEAPON-SELECTION.md) uses groups **1–5**, repeated category keys or the wheel
to highlight a weapon, then LMB to equip it. Tab toggles item-number routing.
F11 refills owned weapons; F12 grants all weapons. The initial kit is unchanged.
Models, sounds and hand-free previews are converted privately from owned CS files.
An optional [Half-Life RPG](HALF-LIFE-RPG.md) appears only with complete owned resources.

The new INI sections `[Deagle]`, `[Glock]`, `[MP5]`, `[P90]`, `[M249]`, `[M3]`,
`[XM1014]` expose `Price`/`AmmoPrice` in the selected currency (0–1000000), `AmmoPack`
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

## First-person hands and muzzle flashes

F6 hands and held weapons sway with actual movement speed, soften while crouching
and settle after stopping or becoming airborne. The decorative pose does not change
the camera or aim. All eleven firearms show an additive flash at the animated barrel,
including each Glock burst round and scoped AWP shots. Knife, sword and C4 do not
emit gunfire flashes. Regenerate owned CS assets through setup to obtain the original
model's barrel attachments; older WCG2 caches use a geometry-derived fallback.

`[Viewmodel]` settings apply with F8 and preserve saved values during updates:

| Key | Default | Range / meaning |
| --- | --- | --- |
| BobEnabled | true | Enables movement of hands and held weapon |
| BobAmount | 1 | 0–3, movement amplitude multiplier |
| BobFrequency | 1.8 | .5–4 cycles/second at 250 CS units/second |
| BobResponse | 12 | 1–30, smoothing response per second |
| MuzzleFlash | true | Enables firearm muzzle flashes |
| MuzzleFlashMilliseconds | 65 | 10–200 milliseconds from first presentation |
| MuzzleFlashScale | 1 | 0–3, flash size multiplier |

## CS 1.6 grenades

Equipment keys **4 HE, 5 Flashbang, 6 Smoke** append stable slots 15–17. Original
number keys remain unchanged. Each purchase buys one grenade, including quick
ammo on a carried grenade. Defaults: HE 300/capacity 1, flash 200/capacity 2,
smoke 300/capacity 1. Full inventory never charges either wallet. Fresh standard kits have
no grenades; all-weapons grants them and free refill restores carried stacks.
Grenades have no reserve, magazine reload, hitscan bullet or muzzle flash.

In armed F6, hold LMB to pull/hold the pin, then release to throw. Releasing early
waits for the pin animation. Holstering, menus, casting or lost control cancels an
unlaunched throw without spending inventory. The native simulation fuse starts
at launch; flights/effects persist across weapon changes and RTS returns, freeze
with Warcraft pause and reset on map unload. The final grenade returns to USP
(or knife). F7/F9 retain native unit attacks.

Launch pitch and angle-dependent speed follow CS 1.6 at the project's 1.5 world
scale: a level throw leaves at ten degrees upward and 900 world units/s, reaching
1125 when aimed upward. The release starts 24 world units ahead of the eye,
clipped against nearby geometry. Running/jumping velocity is inherited. HE uses
660 world units/s² gravity; flash/smoke use 600. Flights bounce against terrain,
units and buildings using swept hulls. This calibrates launch reach; Warcraft
collision responses and the existing detonation/effect rules remain adaptations. HE has fixed peak damage, linear 3D falloff, cover checks and the
existing friendly-fire percentage; neutral buildings and destructable gates
retain native quest immunity. Flash blinds the controlled view according to
distance, facing and cover. Published 0.12.0 briefly paused nearby hostile mobile units and excluded the controlled actor. The current local preview stuns every exposed creature, including allies and the controlled actor, for the full UnitStunSeconds by default. Pre-paused map units are not leased, and only pauses acquired by this feature are released. StunAllUnits=false restores the former enemy-only filter; UnitStunSeconds=0 disables creature stuns. Smoke is a visual volume with depth-tested soft puffs,
near-eye density and fade; it does not block bullets or alter native AI sight.
Thrown bodies use the owner's original `w_hegrenade.mdl`, `w_flashbang.mdl` and
`w_smokegrenade.mdl` with their textures, real perspective and world occlusion.
Clouds use original soft puffs. Held models/previews and impact/explosion sounds
also come from the owner's installed CS files. Rerun setup/update to convert the
three world models into private `assets/world` caches.

Updates migrate the former stock ThrowSpeed=750 / Gravity=800 pair to the new
defaults once. Other custom pairs survive. `TrajectoryVersion=2` marks this upgrade;
keep it when deliberately choosing the old pair. F8 also recognizes unversioned
old stock values. F8 applies prices/carry
limits and tuning to subsequent launches; live flights/clouds keep their snapshot.

| Section / keys | Default | Bounds / units |
| --- | --- | --- |
| Grenades: Enabled | true | true/false |
| ThrowSpeed / Gravity | 1125 / 660 | 50–2000 world units/s (speed cap) and units/s² (HE) |
| LightGravityScale | .9090909 | .1–2 multiplier applied to flash/smoke gravity |
| Bounce / InheritedVelocity | .45 / 1 | 0–.9 restitution / 0–2 multiplier |
| FuseSeconds | 1.5 | .2–10 seconds after launch |
| ReleaseDelay / RecoverySeconds | .25 / .8 | 0–1 / .1–3 seconds of throw animation |
| MaxActive | 32 | 1–128 simultaneous flights/clouds |
| WorldModelScale / WorldSpinDegrees | 1 / 360 | .25–4 visual scale / 0–1440 degrees/s in flight; F8 affects new throws |
| Each grenade: Capacity | 1 / 2 / 1 | 1–100 charges |
| Price / AmmoPrice | above / above | 0–1000000 gold; one grenade per purchase |
| MoveSpeed / ModelScale | 250 / .9 | 50–500 CS units/s / .25–2 model FOV scale |
| HEGrenade: Damage / Radius | 100 / 350 | 0–1000000 fixed damage / 32–2000 world units |
| Flashbang: Radius / BlindSeconds / UnitStunSeconds | 1000 / 3 / 2 | 32–3000 units / 0–10 s / 0–10 s |
| SmokeGrenade: Radius / DurationSeconds / Opacity | 300 / 18 / .85 | 32–1000 units / 1–60 s / 0–1 |

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
their orders. In the [current preview](RELOAD-ARMY-PREVIEW.md), idle fighters reserve some units at an actual base and leave the rest in place. Visible threats still call idle units into defense. `[ArmyAI] FollowActor=true` restores automatic hero escort; the default is false. Without a hall, reserve units do not rally to the actor either.

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
| FollowActor | false | Preview: true/false or 1/0; automatic hero escort, applied on the next AI decision after F8 |
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

## Grenade balance in the current local preview

The [Headshot/grenades preview](HEADSHOT-GRENADES-PREVIEW.md) raises HE peak damage to 300 while retaining its 350-unit radius and cover/distance/native-defense rules. The stock 100 upgrades once; custom damage and versioned 100 survive updates. Flashbangs stun all exposed living creatures, including allies and the controlled actor, for two seconds by default. `StunAllUnits=false` and `StunDistanceFalloff=true` restore the optional older filtering/falloff behavior. Already-flying grenades retain their settings; F8 applies to new throws.
