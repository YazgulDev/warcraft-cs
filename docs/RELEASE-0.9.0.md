# Warcraft CS 0.9.0

Released 2026-10-16. These notes cover the complete change set since the immutable **v0.8.0** tag.
The main additions are native third-person unit control, a clean unarmed first-person view,
automatic squad spellcasting and installation without Counter-Strike assets. This release also
fixes campaign HUD restoration, attack animation interruptions, ramp traversal and submerged hits.

## Download and install

- [WarcraftCSLauncher_DLL_Included.exe](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.9.0/WarcraftCSLauncher_DLL_Included.exe): ready project modules. Choose **Install — Player** to avoid C++ Build Tools and Windows SDK downloads.
- [WarcraftCSLauncher.exe](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.9.0/WarcraftCSLauncher.exe): source installation. It builds the native modules locally and may prepare missing C++ Build Tools/SDK after consent.
- `WarcraftCS-0.9.0-dist.zip` contains the standard source-only launcher, source archive, update manifest, requirements and checksum. **All public ZIPs remain free of DLL/MIX modules.** The DLL-included EXE is distributed separately.
- Both EXEs require download/install agreement before setup. Original games, maps, saves, converted models/sounds and proprietary Warcraft libraries are not distributed.

Select the Warcraft III **1.26a x86** game root containing `war3.exe`, `Game.dll` version
**1.26.0.6401**, the required MPQs and `redist/miles`. Choose a separate writable client destination.
For CS weapons, also select the owned `cstrike` folder (or its Half-Life parent) containing loose models
and sounds. See [installation instructions](https://github.com/YazgulDev/warcraft-cs/blob/v0.9.0/INSTALL.md).

### Warcraft-only installation

Tick **Only Warcraft modes: third person (F7) and first person without UI (F9)** in either launcher.
The CS field becomes optional/disabled. This installs native unit control without CS model/sound
conversion, CS weapons, buying, ammunition rewards or CS sound playback.

- Bundled **Player** setup needs neither Python/NumPy nor C++ Build Tools/SDK in this profile.
- Source/Developer setup still needs Python's standard library, the compiler/SDK and pinned MinHook;
  it skips NumPy and CS conversion.
- The profile is remembered for future updates and recorded as `[Installation] WarcraftOnly=true`
  (default `false`). It retains native TPS attacks, spells, item pickup, squads and Warcraft ambience.
- In this profile **T** cycles between third person and unarmed unit view. **F6** returns to RTS;
  it cannot enter CS weapon FPS. CS supply/buy requests and retained CS model caches are ignored.
- To restore CS weapons, uncheck the option, provide CS and run **Install**, then restart Warcraft.
  Existing private caches can remain on disk. Releases predating this option cannot be installed as
  Warcraft-only; the updater refuses that incompatible source package instead of changing profiles.

## Controls and views

| Key | Full installation | Warcraft-only installation |
| --- | --- | --- |
| F6 | Enter CS weapon FPS from RTS; leave a controlled view for RTS | Return to RTS; CS weapon entry disabled |
| F7 | Enter third person directly; press again in TPS for RTS | Same |
| F9 | Enter unarmed first person directly; press again for RTS | Same |
| T | Switch active FPS/TPS; unarmed view switches to TPS | Switch active TPS/unarmed view |
| Mouse wheel in TPS | Zoom camera closer/farther | Same |
| Left mouse in TPS | Native primary attack, including an empty swing | Same |
| Z / X / C / V, Alt | Existing learned spell slots and self-cast in armed FPS/TPS | TPS spell controls |
| Q | Toggle direct allied targeting in armed FPS/TPS | Toggle allied targeting in TPS |
| F11 | Free ammunition refill for carried equipment, including C4 | Disabled |
| F12 | Free all-weapons/ammunition grant | Disabled |
| H / O / J | Recruit owned combat followers / passive followers / release | Same, including F9 |
| F8 | Reload supported INI settings | Same |

All controls remain configurable under `[Shortcuts]`. F6 is still a recovery alias. The old default
refill **F7** moves to an unused F11/F12/F5/F4; old all-weapons **F9** moves to an unused F12/F5/F4.
Existing custom bindings are preserved. If a custom key conflicts with a new action, that action can
remain `NONE`; assign a free key explicitly. Mode entry no longer grants equipment.

## Native third-person combat and movement

- Control a living owned mobile hero or creature with WASD and mouse look, retaining native spells,
  health, attack effects and status restrictions. Ground, flying, swimming and custom-map units use
  available authored locomotion clips and native movement speed; this is still a prototype.
- Primary melee attacks play native animations, can swing into empty space and apply one delayed
  forward-sweep contact. Ranged units aim with the reticle and launch their native primary missile,
  using current native attack range by default. Native armor/passives and configured allied damage apply.
  CS weapon ammo, damage multipliers and recoil stay separate from native TPS attacks.
- Native target acquisition, retained wind-up and unwanted attack poses are suppressed while direct
  control owns the actor. This fixes close-creep attack cancellation and ranged autoattack loops,
  including the reported Slardar/Sniper cases. Ordinary RTS combat and owned spell casts retain control.
- Manual attack animation ownership lasts through the complete authored clip independently of attack
  cooldown. Preserve alternate clips, native attack-speed scaling and map time scale; repair active
  clip time before skinning instead of repeatedly restarting the first frames. Temporarily suspend
  native pose blending and defer Stop while the manual clip owns the pose, fixing dense melee crowds
  and running-to-attack interruptions. Locomotion resumes after the clip, even during cooldown.
- Native walk/run metadata adjusts playback speed to actual travel. Shift walks, WASD runs, and
  takeoff/air/landing presentation follows physical jump height. Stock Arthas has no authored Jump
  sequence: fallback Hit/Ready poses are reused. Restore native transforms/rates before simulation,
  on view changes and when casts, disabling effects or cinematics own animation.
- TPS camera collision shortens the boom against terrain/buildings and restores the chosen distance
  when clear. Mouse-wheel zoom retains the chosen session distance across view changes, uses the same
  attack/spell camera ray and never changes CS weapons. Discard partial scrolling across focus/menu,
  buy-menu, disabled-state and view boundaries. F8 resets zoom to the configured CameraDistance.
- Reachable contacts receive colored brackets and a ground circle: red for actual enemies, yellow
  for other eligible targets; bracket colors distinguish enemy, ally and destructable. The ring follows
  terrain/bridge decks and respects world depth. Remove the old `TARGET IN RANGE` caption.

### Third-person settings

All keys below are in `[ThirdPerson]`. Values are Warcraft world units unless stated otherwise.
F8 reloads tuning; swing timing affects the next swing, and InitialPitch the next view transition.
Settings do not override native disabling effects. Attack defaults remain fitted to Arthas.

| Key | Default | Supported values / behavior |
| --- | --- | --- |
| Enabled | true | Boolean; false disables TPS entry and leaves active TPS |
| CameraDistance | 500 | 80–1000, bounded by the configured zoom limits |
| CameraZoomStep | 50 | 0–500 per wheel notch; 0 disables zoom |
| CameraMinDistance / CameraMaxDistance | 80 / 1000 | 80–1000 each; max below min collapses to min |
| CameraHeight | 35 | 0–200 above the physical eye |
| CameraClearance | 16 | 0–64 reserved at obstacles |
| InitialPitch | -12 | -60 to +30 degrees |
| AttackRange / AttackArc | 150 / 50 | Melee surface reach 30–400; full sweep angle 1–120 degrees |
| AttackHeight | 64 | 0–200 above the feet |
| MeleeHeightTolerance | 32 | 0–100 above/below the main melee sample, for short creatures |
| UseNativeAttackRange | true | Boolean; use the current primary native ranged reach |
| RangedAttackRange | 600 | 30–5000 manual/fallback ranged reach |
| RangedRangeMultiplier | 1 | 0.1–5; final ranged reach bounded to 30–5000 |
| NativeRangedProjectiles | true | Native missile flight when attack effects are enabled; false uses instant contact |
| AttackInterval | 2.2 | 0.2–5 seconds before repeating a swing |
| ContactDelay | 0.56 | 0–3 seconds, capped by AttackInterval; timing scales with attack speed |
| DamageMultiplier | 1 | 0–10 times current native unit attack |
| MovementAnimations | true | Native locomotion/leap playback |
| AutoAnimationSpeed | true | Use UnitUI gait metadata; old custom strides can retain manual playback |
| AnimationSpeedMultiplier | 1 | 0.1–3; visual playback only |
| RunCycleDistance | 210 | 40–1000 travelled per cycle; manual/fallback stride |
| TakeoffDuration / LandingDuration | 0.10 / 0.16 | 0.02–0.3 / 0–0.5 seconds; zero landing duration disables that phase |
| ShowTargetIndicator | true | Enable contact brackets and circle |
| TargetCircleRadius | 64 | 16–256 world units |

## Unarmed first-person unit view

**F9** provides a separate unit camera with WASD/mouse movement and native unit move speed.
Weapons, hands, crosshair, HUD, minimap, native panels and floating labels are hidden. Player attacks,
spell requests, purchases, ammo grants and item requests are blocked; squad commands remain available.
Warcraft ambience remains audible and CS movement samples are omitted.

| `[UnitView]` key | Default | Behavior |
| --- | --- | --- |
| Enabled | true | Enable F9; disabling with F8 returns active unit view to RTS |
| PreventEnemyAttacks | true | Reject hostile basic-attack acquisition, retained wind-up/contacts and map-issued attack orders against this actor during live F9 |
| HideHoverCircles | true | Hide native hover/preselection circles on units, buildings and destructables during live F9 |

These booleans accept true/false or 1/0; missing/malformed values use true. Startup/F8 applies them.
`PreventEnemyAttacks=false` restores incoming basic attacks. This does **not** grant invulnerability:
already airborne missiles, spells, area effects and direct scripted damage remain native. Other units,
alliances and other view modes are unaffected; no persistent unit protection/visibility flags are set.

Hover suppression borrows Warcraft's native preselection settings, independently of the GL HUD filter.
Leaving F9 or disabling the option restores the map's latest settings, including requests made while
hidden. Map replacement discards stale UI ownership. RTS/FPS/TPS keep their selection/target circles.

## Combat followers cast supported spells

H combat followers can use supported native offensive, healing, buff and summon orders. Retain native
mana, cooldowns, target rules, recognized map charges and cast/channel ownership; formation refreshes
must not interrupt an owned cast. O switches to passive following without this combat spell AI;
J releases followers. Recruit only nearby owned mobile units, not allied/enemy armies or buildings.
Native/map autocast switches remain under Warcraft's control.

New `[Squad]` settings, all applied on F8 and preserved on update:

| Key | Default | Supported values |
| --- | --- | --- |
| AutomaticSpells | true | Boolean; false disables this module's automatic casting |
| SpellSearchRadius | 700 | 50–2000 world units |
| AreaSpellRadius | 250 | 16–1000 world units |
| HealBelowPercent | 85 | 1–100 percent of target life |
| BuffRefreshSeconds | 20 | 1–600 seconds between buff/summon refresh decisions |
| SpellIntervalMs | 1000 | 100–10000 milliseconds between decisions |

The catalog is bounded to supported stock orders. Arbitrary custom spell implementations, items,
passives and toggle spells are not automatically discovered or automated.

## Hit registration, allies, water and ramps

- **Q** toggles direct allied targeting in FPS/TPS and the HUD shows `ALLIES ON/OFF`.
  `[Combat] TargetAllies=false` is the protected startup/F8 default. With it off, skip allied units
  **and buildings before nearest-hit selection**: enemies behind them can be marked/hit, while allies
  receive no direct damage. With it on, the foreground ally intercepts the attack and receives the
  configured `[Damage] FriendlyFirePercent`. Missing/malformed values use false; booleans and 1/0 work.
  Preserve custom bindings and settings. C4 splash and native spells retain their own policies.
- Liquid alone no longer clips FPS firearm/melee rays or TPS melee/ranged targeting before a submerged
  body. `[Combat] WaterBlocksAttacks=false` selects solid terrain and raised walkables for attack traces;
  true restores the former water-plane blocker. Missing/malformed values use false, with F8 reload.
  Keep solid terrain, cliffs, gates, nearest eligible bodies and range limits. Movement/camera water
  heights, water rendering and spell ground picking are unchanged.
- `[Movement] UseNativeUnitCollision=true` uses live creature collision footprints on narrow ramps,
  rather than large weapons/capes or non-colliding map helpers. Buildings/trees retain solid volumes.
  False restores legacy creature model boxes/actor radius. F8 applies the boolean.
- Resolve grounded support between swept movement substeps, fixing uphill/downhill traversal without
  weakening walls/cliffs/bridge/tree collision. This includes the reported Scourge-side DotA ramps.

## Campaign HUD and render defaults

- Compose the HUD at Warcraft's verified frame-batch completion instead of relying on a particular
  Windows/OpenGL swap entry point or dropping valid world frames older than 500 ms. This fixes the
  campaign save/load case where the HUD disappeared until reinstalling.
- Consume each completed world once, reset submissions at map boundaries, and exclude upload/readback
  flushes or nested callbacks. Retain the previous swap hook as the signature fallback.
- Capture/restore the minimap from its own draw buffer, including front-buffer campaign restoration.
- Fresh/default `[Rendering] DrawDistance=5000` replaces the previous 15000 default. Valid values remain
  3000–20000 world units, applied at startup/F8; missing/empty/malformed values use 5000. **Explicit saved
  distances, including 15000, survive normal upgrades.** Edit the INI to adopt 5000 on an older install.
  Draw distance still respects map visibility/atmospheric fog. The existing MaxFPS default remains 144.

## Upgrade guidance

Save and close Warcraft before using **Update** or installing a 0.9.0 EXE into the existing client
destination. Update checks require confirmation; **Install** uses the EXE's embedded version and
**Update** fetches the latest stable GitHub package. Saves, private content and customized settings
are preserved, with missing keys added. Check F7/F9/F11/F12 and any conflict marked NONE after migration.
The runtime INI is `Game/WarcraftCS/WarcraftCS.ini`; F8 reloads supported options in a controlled view.

Ordinary upgrades retain the full CS profile unless the new checkbox is explicitly selected. The
Warcraft-only profile is an installation choice; returning to full CS needs installation/conversion
and a restart. Repeated view switching does not reset the map's loadout or create free supplies.

## Source, validation and limitations

Keep gameplay, presentation and native adapters separate: unit attacks/animation, collision profiles,
camera/target-marker geometry, native frame completion, F9 hover ownership and squad spell policy are
distinct modules. Extend the owned test fixtures/config migrations and the weapon-menu skill for the
new view keys. Update regression harness dependencies for native collision profiles and incoming-attack guards; validate each
real EXE's concise change list against the full release body and manifest fallback. Dependencies and
credits remain documented in [NOTICE](https://github.com/YazgulDev/warcraft-cs/blob/v0.9.0/NOTICE).

Release preparation runs native-independent gameplay/render/adapter regressions, INI/migration checks,
launcher/update/installation tests, both ordinary builds and packaged-EXE/archive checks. Audit the exact
source snapshot for secrets, personal paths, proprietary game content and generated binaries. Test-only
map probes are excluded from ordinary Player modules. The standard launcher and public ZIPs contain
owned sources; only the separately named EXE carries the normal project native modules and notices.

Feature development used disposable Warcraft/DotA scenes for native melee/ranged attacks and effects,
empty swings, movement/animation playback, close crowds, reachable target markers, zoom, ramp traversal,
campaign save/load, squad spellcasting, F9 attack protection, submerged targets and allied foreground
units/buildings. It also checked native hover preference restoration and real 5000/7000 camera fields.
These are bounded checks, not a full campaign/custom-map playthrough. Recent native logic checks used
Direct3D because the isolated test copy could not initialize OpenGL; that is **not** a claim of final
OpenGL UI pixel validation. Separate WGL render regressions cover HUD/marker/minimap state and pixels.
See [validation notes](https://github.com/YazgulDev/warcraft-cs/blob/v0.9.0/docs/VALIDATION.md).

Supported host remains **offline Warcraft III 1.26a x86 (1.26.0.6401)**. Reforged, other patches,
multiplayer, all custom ability orders and full campaign compatibility remain unsupported or unverified.
Some scripted interactions still need RTS control; F6 returns to RTS. Animation fallbacks depend on
available model clips. F9 protection does not block all sources of damage. The loader remains unsigned
and uses native hooks, so antivirus heuristics can still flag it; no clean-detection guarantee is made.
See [troubleshooting](https://github.com/YazgulDev/warcraft-cs/blob/v0.9.0/TROUBLESHOOTING.md#antivirus).

<!-- launcher-summary
- Added native third-person control with melee and ranged unit attacks.
- Added direct F7 entry, camera wheel zoom and reachable-target markers.
- Added model-specific walk, run and jump presentation.
- Fixed interrupted attack animations, close crowds and ranged autoattacks.
- Fixed narrow ramp traversal and grounded movement support.
- Added unarmed F9 unit view with configurable enemy targeting and hidden hover circles.
- Added automatic native spellcasting for combat followers.
- Added Warcraft-only installation without CS assets or Player Python downloads.
- Added Q allied targeting and attacks past protected allies.
- Fixed hits on submerged units while retaining solid obstacles.
- Fixed campaign HUD restoration and minimap buffer recovery.
- Changed default draw distance to 5000 and free supply keys to F11 and F12.
-->
