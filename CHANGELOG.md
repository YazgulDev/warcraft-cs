# Changelog

## Unreleased

- Classify mixed head/neck faces at the actual contact using fractional named-head skin weights, retaining physical occlusion and unverified-model exclusions.
- Enable direct Tau/Gluon headshot bonuses by default and emit blood from verified head contacts even when bonus/text is disabled; retain actual-HP-loss requirements and visual-only query exclusion.
- Start a missing campaign base near available workers, prioritize actual worker race over captured foreign halls and retry bounded placement sites within a decision.
- Add undead haunted-mine construction and configurable idle-ghoul lumber gathering; activate night elf mines through the hall's native entangle ability.
- Add configurable army status feedback for native map/resource restrictions, missing economy/workers/resources and progress, keeping manual/squad/combat exclusions and optional escort.
- Add parser/update, mixed-surface, energy/blood and campaign-start/native-economy regression coverage; make the affected native runners catch signed aborts. [Local fix preview](docs/HEADSHOTS-CAMPAIGN-PREVIEW.md).

## 0.13.0 — 2026-10-26

- Add selectable dollar/gold payment in every B category, both wallet balances and whole-price gold fallback when dollars are insufficient. Dollars remain the default and kill rewards stay $200.
- Allow shopping during reload without canceling or freezing animation, audio, deadline or shell insertion; ammo purchases replenish reserve and actual weapon deployment cancels the old reload.
- Trace verified 1.26a posed unit surfaces so neighboring broad bounds do not steal shots; add named-head damage bonuses (default x2) with native defenses and existing melee/spell/TPS behavior retained.
- Add confirmed headshot blood and rising world-space Headshot text; add bounded falling/spinning brass and shotgun cases for accepted conventional rounds, with editable visual settings.
- Show localized item/rune names under the F6/F9 reticle with physical occlusion, Unicode support and configurable range/tolerance.
- Default movement to classic while retaining optional hull/recovery, platform protection and Home in recovery; preserve saved explicit mode choices.
- Default ArmyAI.FollowActor to false: U retains native economy and defense without rallying ordinary troops to the hero. Reserve rally requires an actual hall; F8 can restore optional escort.
- Default RPG to 1800 fixed peak damage and radius 600, M3/XM1014 whole-shell native attack multipliers to x6/x4.8, and HE peak damage to 600 with radius 350.
- Default flashbangs to full two-second stuns for all exposed creatures, including allies and the actor; add enemy-only and distance-falloff opt-ins while retaining safe pause ownership, cover and overlap handling.
- Add stock-only balance migrations at setup/startup/F8, preserving custom/current-version choices and immutable live projectile snapshots.
- Extend posed-model, native shot/item/blast, payment, visual WGL, configuration/update and movement regressions; reject signed native abort statuses in the grenade runner.
- Update modular source/build documentation, complete launcher summaries and release instructions requiring direct downloads first.
- Complete download links, settings, upgrade instructions, verification and 1.27a limitations: [Release 0.13.0](docs/RELEASE-0.13.0.md).

## 0.12.0 — 2026-10-25

- Add experimental exact-host 1.27a/D3D9 support and dual-host Player bundles with adaptive local Miles forwarders.
- Add optional Half-Life Crowbar, Tau Cannon and Gluon Gun, a dedicated weapon category, corrected chrome/animation/muzzle alignment, charge feedback and continuous beam effects.
- Default Tau damage to 100/1000 charged and Gluon to 70 per tick; migrate previous stock damage once while preserving custom values.
- Silence native FPS contact/instant-art attack audio by default while preserving native passives, other units and TPS/RTS presentation.
- Default purchases to a separate dollar wallet: $800 starting funds, $200 per eligible mod weapon kill and a $16000 cap. Add configurable 0.5 percent rare firearm drops and E pickup.
- Keep RPG peak damage 100 and increase blast radius by 150 to 525; migrate the former stock radius once.
- Default Movement.Mode to recovery, retaining optional classic and hull modes. Improve local cliff/ceiling/round hull/static prop contacts and narrow passages; add platform support, invalid-position protection and validated Home recovery.
- Fix FPS camera/body separation at 1.26a camera edges and repair already out-of-world actors in every mode while preserving movement tuning.
- Split the repository homepage into focused documentation, retain source-only publication/notices and extend native-independent, graphics, installation, config and package regressions.
- Full settings, upgrade steps, complete verification and 1.27a/campaign limitations: [Release 0.12.0](docs/RELEASE-0.12.0.md).

## 0.11.0 — 2026-10-20

- Added Deagle, Glock, MP5, P90, M249, M3 and XM1014 purchases.
- Added native shops on E and optional army economy and support AI on U.
- Added a CS-style carried weapon menu with sequential wheel selection.
- Added HE, flash and smoke grenades with original thrown models.
- Added an optional Half-Life RPG with real rockets and laser guidance.
- Added movement of hands and animated muzzle flashes.
- Changed firearm reach to cover the visible world by default.
- Fixed shotgun reload timing and sounds with interruptible per-shell loading.
- Added single-shot and right-click burst actions for the Glock.
- Fixed ledge traversal and camera collapse inside bridge model bounds.
- Fixed inherited OpenGL state hiding the flat HUD.
- Added bounded HUD, font, installation profile and DLL diagnostics.

- Preserve saved prices, bindings and feature opt-outs; migrate only the old stock grenade
  trajectory pair once. U yields to existing custom bindings. Optional RPG resources are sealed
  and rechecked on startup/F8; missing content cannot be bought, granted or selected.
- Keep render/projectile/diagnostic ownership in cohesive modules, extend private conversion
  and the weapon authoring skill, retain upstream notices and include RPG in the full test wrapper.
- Full controls, new settings/defaults/bounds, upgrade steps, verification and limitations:
  [Release 0.11.0](docs/RELEASE-0.11.0.md). All changes are measured from immutable v0.10.0.


## 0.10.0 — 2026-10-18

- Restore native F7 ranged projectiles against destructable gates/crates, including Kael attacks
  on campaign gates. Resolve/revalidate typed widget identities through native preparation, launch
  and cleanup; native missile flight/damage and quest immunity remain authoritative. Existing
  NativeRangedProjectiles/EnableAttackEffects preferences and F8 opt-outs retain instant-hit behavior.

- Prevent transient sideways actor/model snaps near known walls and colliding units. Commit the
  swept-valid position directly while approaching, sliding along or leaving either side of a solid,
  instead of invoking native nearest-cell relocation and rolling it back. Retain terrain, body-volume
  and unknown-blocker validation, bridge/cliff support and existing movement configuration.

- Fix RMB target orders being swallowed by a protected ally between the camera and the enemy,
  including F7. Filter allegiance before nearest-target selection using the existing Q/TargetAllies
  preference; preserve ally spell/item selection, neutral targets and destructable obstruction.
  Distinguish an unavailable/rejected native attack from an empty-aim cancellation in HUD feedback.

- Preserve active H-mode combat ahead of yielding and leash movement with
  `[Squad] PreserveCombatOrders=true` (false restores legacy behavior; startup/F8). Idle followers
  still yield and follow; explicit RMB attacks retain priority while approaching or fighting.
  Move the F6 RMB-orders toggle to Y. Migrate the former semicolon default during updates and
  startup/F8, preserve custom bindings, and disable the new binding if a saved action already owns Y.

- Fix common-target orders on destructable gates/crates, including the gate type used in Human03.
  Track native widget identity/life separately from units so orders, cancellation, death and handle reuse
  work correctly. Allow neutral-owned units/buildings in direct FPS/TPS attacks with allied targeting
  disabled; actual player allies remain protected. Neutral-passive scenery uses native finite damage
  even with AWP finishing enabled, preserving quest immunity. Existing controls and config keys apply.

- Add RMB common-target attack orders in F7/F9 for all available owned mobile units, including units
  outside H/O recruitment radius/cap. Empty aim cancels, restoring the previous follow/combat policy
  and native behavior. Native targeting/pathfinding rules remain authoritative. Inventory cancel and
  buy-menu input keep priority. Add `[Squad] ReticleAttack=true`, `ReticleAttackInFPS=false` (true
  starts armed F6 with orders) and `ReticleAttackDistance=3000` (100..15000 units), startup/F8
  with preserved saved values. In F6, **Y** toggles RMB orders/weapon action and clears the target on
  switch-off. Add rebindable `[Shortcuts] SquadOrders=Y` and live mode hints; preserve existing
  custom bindings. Clean F9 remains without HUD; the controlled actor is excluded.

- Keep third-person zoom stable near buildings ahead/behind the actor. Add
  `[ThirdPerson] CameraBuildingCollision=false` (true restores building push-in; startup/F8).
  Terrain/destructable obstruction remains enabled; disabled building collision permits camera
  passage through building models. Start inventory numbers enabled with
  `[Inventory] NumbersByDefault=true` (false opts out; startup/F8/map reset). Tab still toggles
  items/weapons; gesture cleanup and view switches preserve its session choice. Preserve saved values.

- Add the F6/F7 animated portrait, HP/mana/map-resolved hero XP and inventory above spells.
  Tab toggles 1..6 item activation; I opens mouse use, target selection, drag-out dropping and
  slot rearrangement. Native Warcraft rules govern effects, charges, cooldowns and droppable items.
  Add rebindable inventory shortcuts, `[Interface] InventoryControls=true` and
  `[Inventory] DropDistance=80` (16..128 units, startup/F8), preserving existing custom controls.
  F9 stays clean and third-person hints omit CS ammo/weapon commands.

- Keep point-targeted spells aimed at terrain independently of nearby units/destructables. In auto
  mode, use the controlled actor's native command flags to send point-only orders for abilities such
  as Blink and Flame Strike; a rejected or missing ground point never falls back to a widget target.
  Alt/self casts use the actor's ground position. Explicit slot target overrides remain available.

## 0.9.1 — 2026-10-16

- Replace WGL bitmap display-list text with GDI-generated alpha glyph atlases for the F6/F7 HUD,
  including shortcut hints, status, buy-menu text and spell labels. Isolate glyph uploads and drawing
  from inherited OpenGL state, retain resolution scaling and rebuild resources after context changes.

## 0.9.0 — 2026-10-16

### Clean unit view and 5000 draw distance

- Hide Warcraft's native hover/preselection circles on units, buildings and destructables during F9.
  Add `[UnitView] HideHoverCircles=true` (true/false or 1/0, startup/F8); false retains native circles.
  Restore map preferences on mode exit, menus and opt-out, track requests made while hidden, and
  discard unloaded-map state. FPS/TPS target markers and ordinary RTS selection remain available.
- Set fresh/default `[Rendering] DrawDistance=5000` Warcraft units (range 3000–20000, startup/F8).
  Missing/empty/malformed distances use 5000; updates preserve explicit saved distances.

### Attacks past protected allies

- With `[Combat] TargetAllies=false` / Q `ALLIES OFF`, skip allied units and buildings before
  choosing the nearest FPS weapon hit, third-person melee contact or ranged reticle/muzzle target.
  The enemy behind a protected ally can now be marked and hit; the ally receives no direct damage.
- Q `ALLIES ON` restores ordinary allied interception and configured friendly damage. Keep
  terrain/gates/other enemy interception, weapon reach, movement collision, C4 and spell policies.
  The existing startup/F8 setting and its update-preservation rules still apply.

### Warcraft-only launcher profile

- Add a remembered checkbox in both launcher variants to install F7 third person and F9 unarmed,
  UI-free first person without a Counter-Strike folder, model/sound conversion or NumPy.
  Bundled Player setup also skips Python; source builds retain Python/MinHook/C++ requirements.
- Persist `[Installation] WarcraftOnly` (default false), preserve other settings/saves, and reject
  older release updates that cannot honor the selected profile. Uncheck and Install with CS to restore weapons.
- Restrict F6/T and CS supply/buy inputs to installed modes; keep native TPS combat, spells, items,
  squads and Warcraft audio. Hide unavailable CS hints. Add profile/consent/config/backend regressions.

### Attacks through water

- Separate combat surface sampling from liquid-aware movement height. Water alone no longer
  clips FPS firearm/melee rays or third-person melee/ranged prediction and contact before a submerged target.
  Retain solid terrain, cliffs, raised walkable geometry, gates and nearest-body interception.
- Add `[Combat] WaterBlocksAttacks=false`, with F8 live reload and `true`/`false` or `1`/`0`.
  `true` restores the previous liquid blocker; missing/empty/malformed values use `false`.
  Updates preserve custom values. Movement, camera height, water rendering and spell point picking stay unchanged.
- Add water-plane/terrain regressions for the real third-person picker and config preservation/reload,
  plus opt-in native submerged-target and rendered-height probes confined to disposable maps.

### Configurable enemy targeting in unit view

- Default `[UnitView] PreventEnemyAttacks=true` prevents hostile basic-attack acquisition,
  retained wind-up/contacts and map-issued attack orders against the controlled F9 unit.
  Other units, allies and other view modes retain their native targeting behavior.
- Set `false`/`0` and press F8 to restore enemy attacks immediately; `true`/`1` enables protection.
  Missing/empty/malformed values default to true. Updates add the setting and preserve custom choices.
- No saved vulnerability, visibility, ownership or alliance flags are changed. Already airborne
  missiles, spells, area damage and direct scripted damage remain native.
- Verify default protection, live F8 off/on, another unit remaining attackable and normal attacks
  after F9 exit in a disposable native map, alongside config validation and update preservation.

### Unarmed unit view and squad spellcasting

- Add F9 to enter a separate first-person unit view from RTS, FPS or third person. Hide weapons,
  crosshair, HUD, native panels and floating labels. Retain WASD/mouse movement and native unit
  movement speed. Block player attacks, spell requests and purchases. Enemy basic-attack protection is configurable;
  missiles, spells, area effects and scripted damage remain native.
  Press F9 again to return to RTS; F6 and F7 retain their existing mode controls.
- Move the default free all-weapons shortcut from F9 to F12. Migrate the previous F9 binding to
  an unused F12/F5/F4 while preserving custom layouts. Conflicting custom F9 actions leave the
  new binding unassigned until the user selects a free key. Add `[UnitView] Enabled=true` with F8 reload.
- Let H combat followers use supported native attack, healing, buff and summon spells. Preserve
  mana, cooldowns, recognized map charges and ongoing casts instead of interrupting them with
  formation orders. O follow mode stops this module's casts; J releases the squad.
- Add `[Squad] AutomaticSpells`, `SpellSearchRadius`, `AreaSpellRadius`, `HealBelowPercent`,
  `BuffRefreshSeconds` and `SpellIntervalMs`. F8 reloads settings; updates preserve user values.
  Native/map autocast switches remain under Warcraft's control. Arbitrary custom orders, items,
  passives and toggles are not automated.
- Add config migration/policy regressions and opt-in native spell fixtures. Verify bare unit-view
  rendering, blocked player combat, incoming damage, native Storm Bolt and Priest healing with
  stock autocast disabled, and the O/F8 spellcasting opt-outs in a disposable test map.

### Close-contact attack playback

- Follow the model's active clip independently of the sprite's pending request. Dense crowds
  could keep the request pending and prevent manual attack playback from advancing.
- Restore the authored model clip and its elapsed time synchronously through the verified native
  selector before skinning, so close hit reactions cannot starve the queued request.
- Defer native Stop cleanup while a manual attack owns its pose; native movement, acquisition
  and damage guards continue to block autonomous combat. Keep contact, cooldown and status rules.
- Add an isolated six-unit crowd scene and read-only active/pending clip telemetry for regression checks.

### Full attack playback and allied targeting

- Refresh native skinning after repairing the manual attack clock, including while grounded, so
  rendered bones follow the complete swing. Finish authored clips even when configured recovery
  is shorter; held attacks wait for both the minimum recovery and the visual backswing.
- Suspend native pose blending during manual swings so repeated movement pose resets cannot keep
  the visible attack near its first frame. Restore the previous blending policy when ownership ends.
- Add Q (`[Shortcuts] ToggleAllyTargeting`) to toggle direct allied contacts and target markers
  in FPS/TPS. `[Combat] TargetAllies=false` protects allies by default; startup/F8 restores that
  saved choice. Protected allies are skipped before nearest-hit selection. C4 splash and native spells retain their policies.
- Show the current mode in HUD hints. Preserve custom Q bindings and targeting preferences on updates.

### Third-person attack animation recovery

- Separate manual attack playback from combat cooldown. Resume locomotion when the authored attack
  clip ends, while retaining the existing contact delay and repeat-attack interval.
- Preserve native alternate attack clips and advance their playback time after movement or impact
  cleanup resets the pose. Respect native attack-speed modifiers and map-owned time-scale changes.
- Yield to casts, disabling effects and death. Add portable timing regressions and opt-in native
  clip-clock telemetry; verify moving attacks and recovery on Arthas, Archmage and Footman.

### Campaign HUD recovery

- Compose the HUD after Warcraft's verified native frame-batch completion, before presentation,
  independently of the Windows/OpenGL swap entry point. Exclude uploads, readbacks and nested
  callbacks. Keep the previous swap hook as a fallback for unrecognized renderer signatures.
- Consume completed world submissions instead of dropping valid campaign frames older than 500 ms.
  Reset the pending frame on map unload/replacement so menus cannot inherit a previous map's HUD.
- Capture/restore the minimap from its own draw buffer. Add real WGL front/back-buffer regressions
  and opt-in campaign save/load and native presentation fixtures, excluded from ordinary client modules.

### Third-person unit control

- Add third-person mouse-wheel zoom without interrupting spells or changing CS weapons. Preserve native camera collision and shared reticle rays. `[ThirdPerson] CameraZoomStep=50` (0..500 world units per notch, 0 disables), `CameraMinDistance=80` / `CameraMaxDistance=1000` (80..1000 each) reload on F8, which restores the bounded `CameraDistance`. Preserve custom values during config updates and discard pending wheel input at focus/menu/mode boundaries.
- Recover native sprite pose overwrites while standing/running in direct control; leave casts, stuns and manual attacks in charge of their animations.
- Mark the reachable third-person attack contact with red enemy, green ally or gold destructable brackets and a terrain-conforming ground circle (red enemy/yellow other targets). Remove the TARGET IN RANGE caption. Preview shares melee/ranged contact, native range and occlusion rules with LMB. `[ThirdPerson] ShowTargetIndicator=true` and `TargetCircleRadius=64` (16..256 world units) support F8 reload and config-preserving updates.
- Suppress retained-target native wind-up timers and attack poses in direct control, fixing Sniper's visible autoattack loop while preserving manual ranged missiles and RTS attacks.
- Use live Warcraft actor/creature collision footprints on narrow ramps; non-colliding map helpers no longer supply solid model obstacles. Buildings and trees retain solid volumes. `[Movement] UseNativeUnitCollision=true` reloads on F8; false restores legacy creature boxes/actor radius.
- Resolve grounded support between swept movement substeps, fixing fast uphill traversal without weakening walls or cliff collision. Extend disposable movement and collision-profile regression coverage.

- Fix repeated Stop commands resetting manual attacks next to creeps, including DotA Slardar. Track retained
  targets per actor and reject autonomous native preparation/impacts while preserving scoped manual attacks,
  ordinary RTS combat, spell ownership and follow-only squad policies.
- Add aimed primary ranged attacks with native map missile flight, attack effects, native attack range and
  configurable range/multiplier/projectile fallback. Keep CS ammo and recoil separate from unit combat.
- Expand melee contact vertically for short creep models with configurable MeleeHeightTolerance. Restore
  native animation rate before an attack and keep damage dispatch on the manual contact timeline.
- Move the melee adapter to UnitAttack; add native guard/lifecycle, retained-target and configuration tests.

- Add configurable F7 direct third-person entry from RTS/FPS and F7 return to RTS. Move free ammo refill to
  F11; migrate the old F7 default without resetting custom layouts. Keep T view switching and F6 recovery.
- Extend movement presentation to mobile units: choose authored model indices for walk/run/swim/fly and leap
  fallback poses; Shift walks, WASD runs. Match native UnitUI gait speeds, with configurable automatic
  playback, multiplier and manual stride fallback.

- Add speed-matched native run/idle playback and takeoff/air/landing poses for Arthas; render the visible
  body at its physical jump height and restore its model transform before the next tick. Stock Arthas has no
  authored Jump sequence, so the prototype reuses its native Hit/Ready poses. Preserve casting/stun/attack
  ownership and restore playback rate on T/F6/cinematics/focus loss. Add configurable stride/phase durations.

- Add T (configurable ToggleView) to switch active FPS/TPS, retaining F6 RTS recovery, inventory and position.
- Show the native melee actor with WASD/mouse control, a reticle and existing Z/X/C/V native spell casting.
- Add native hero attack animations, empty swings and delayed single-contact forward melee sweeps; preserve
  native attack damage/armor/passives, live attack-speed modifiers and the configured friendly-fire percentage.
- Separate hero attacks from CS weapon ammunition, recoil, scope and damage multipliers.
- Add collision-shortened camera geometry and [ThirdPerson] camera/swing settings, validated F8 reload and
  preservation of custom settings on updates. Default attack timing remains tuned for Arthas.
- Add attack timeline/camera/config regressions and explicit disposable native Arthas test commands.
- Document controls, timing semantics, module ownership and prototype limitations in docs/THIRD-PERSON.md.

### Release preparation

- Update regression harness dependencies for live collision profiles and incoming-attack guards.
- Verify both real launcher variants resolve the full release body and manifest fallback to the
  reviewed compact English summary; keep source ZIPs free of native modules.
- Publish complete notes since v0.8.0, settings/migration instructions and updated source installation references.

## 0.8.0 — 2026-10-12

- Fix bullets and melee contacts being intercepted by nonphysical effect units such as Broodmother
  webs. `[Combat] IgnorePassableInvulnerableObjects=true` skips Locust units and invulnerable
  noncolliding dummies; heroes, structures and physical immune bodies still block shots.
  F8 reloads the switch; updates preserve custom values. Log skipped ray contacts.

- Fix confirmed firearm hits being rejected beyond the hero's native attack range when passives
  are enabled. Scope the native range check to the verified FPS contact; retain ordinary RTS range,
  weapon/terrain ray limits, native passive eligibility and evasion handling. Log contact distance.

- Remove C4 from the default starting inventory (`[Loadout] BombCount=0`). A positive count or
  explicit all-weapons mode opts into ownership. F7 grants/refills C4 and F9 still grants every weapon.
  Allow unlimited repeat C4 purchases by default (`UnlimitedBombPurchases=true`); MaxBombs retains
  its free-refill/rune amount and optionally caps purchases when the switch is false. Refills preserve
  surplus purchased bombs. Existing custom starting counts survive updates.

- Add configurable native firearm/melee attack effects: preparation, impact and cleanup preserve
  learned passives, item orbs, native chances and map attack callbacks without issuing pursuit orders.
  `[Combat] EnableAttackEffects=true` reloads with F8; false keeps direct damage and native armor class.
- Set both C4 weapon and ammo purchase defaults to 3000 gold per charge; preserve custom prices on updates.
- Log active AWP finishing and attack-effect settings, plus per-contact `finishing`/`effects`, to identify
  preserved `AWPOneShot=true` configurations independently of ordinary unit-scaled damage.

- Default firearm/melee damage to the controlled unit's native attack; retain weapon mode and legacy hero alias.
- Double weapon prices; retain ammo pack prices. Increase default rune ammo rewards to 40%.
- Display current unit-scaled damage, firearm cadence and magazine/reserve limits in buy previews.
- Add per-firearm MaxReserve (0..100000), shared by purchases, free refills and runes, with surplus clamping on F8.
- Add optional native attack-speed firearm cadence (disabled by default), and optional native movement-speed scaling with a configurable reference.
- Reject unavailable native spell requests before dispatch without interrupting active channels; show DotA Shrapnel map-owned charges and reject empty-charge requests.

- Fixed Avatar/morph crashes caused by masking a retained native attack target; the walking motor now blocks pursuit safely.

- Fix idle FPS movement through the separate native walking motor, preserving speed modifiers,
  scripted placement, explicit spell approach, normal units and RTS.
- Keep recruited followers behind the player and route obstructing allies sideways before rejoining;
  add configurable rear/yield preferences, clearance, forward corridor and response interval.
- Resolve object-editor research positions and learned runtime ability replacements; exclude stats
  on another research row and recognize relative passive icon paths.
- Refresh spell art from actor-owned native command buttons, add actual mana/disabled states and
  a radial native cooldown sweep.


- Add bottom-right spell icons, configured shortcut labels and live native cooldowns, including passive/unlearned/unsupported states; read owned current-map Art without packaging game assets.
- Add Interface.ShowSpellIcons, ShowSpellCooldowns and SpellIconSizePercent (4..14% of window height, default 7), live F8 reload and preservation during upgrades.
- Preserve native attack target references during morph/combat callbacks; block automatic pursuit through validated order dispatch and the walking motor.

- Fix automatic enemy pursuit in FPS by rejecting native basic-attack validation and retaliation dispatch before chase starts; preserve spell components, ordinary units and RTS control, with bounded rejection diagnostics.

- Fix custom hero spell mapping when the primary ability is fifth/later in the list: prioritize declared research columns, preserve passive slots and prefer learned replacements in duplicate columns; log all four resolved rawcodes.

- Add first-person native hero spell casting on Z/X/C/V, reticle unit/point targeting and Alt self-targeting; preserve learned/passive slots, native mana/cooldowns and explicit cast/channel order ownership.
- Move the speed toggle from V to P and add validated, conflict-checked keyboard shortcuts with live reload, matching HUD labels and update preservation; retain F6 recovery and native Escape/F10 menus.
- Add spell enable/distance/ability/order/target configuration for custom maps and detailed spell/shortcut diagnostics.
- Add FPS health bars for allies (green) and enemies (red), one K toggle for both teams, saved preferences, F8 reload and configurable distance; filter dead/hidden/transported/fog-hidden units and depth-test against the world.
- Fix disappearing nearby ground above the native 10000-unit camera limit: match the live camera field for near clipping and temporarily lift only the FPS far-channel ceiling to the supported 20000-unit range.
- Restore the native camera ceiling on FPS exit, menus, faults and camera changes using generation keys; log actual world projection near/far values separately from the INI request.
- Recognize the full native cliff rim, including diagonal corners, so landing below a ledge does not trap the player in its walkability mask.
- Permit retreat from an existing solid overlap and its native footprint while still rejecting deeper penetration, new solids and unknown flat blockers.
- Log sampled movement rejection reasons and terrain/cliff coordinates for diagnosing stuck positions.
- Raise the client FPS target to 144 and FPS world-camera far clip to 15000 Warcraft units; add Rendering.MaxFPS (0/native or 30вЂ“360) and DrawDistance (3000вЂ“20000), F8 reload and upgrade preservation.
- Use high-resolution elapsed time for player movement and guarded native render-demand changes with paced presentation, without changing Warcraft's simulation timers.
- Use decoded standing tree trunks for movement collision instead of canopy-sized model extents; reuse the shooting geometry and retain native pathing for unreadable models.
- Log tree/trunk classification and add regressions for a clear canopy corridor, trapped starting positions and solid trunk contact.

- Add a native Warcraft minimap in the top-right corner of FPS, including terrain, fog and native markers; M toggles its session visibility.
- Keep rectangular-map letterboxing black and restore the world beneath the original RTS map pixel for pixel; leaving FPS retains Warcraft's normal minimap layout.
- Add `[Interface] ShowMinimap=true` and `MinimapSizePercent=22` (10..40% of window height), validated defaults, live F8 reload and preservation during config upgrades.
- Compact the top shortcut hints into wrapping groups, wrap them before the minimap on narrow windows, remove duplicate buy/ammo hints and move gold beside health/ammunition.

Complete notes and upgrade instructions: [RELEASE-0.8.0](docs/RELEASE-0.8.0.md).
Both launchers, source packages and verified updater metadata are rebuilt for 0.8.0.

## 0.7.0 вЂ” 2026-10-10

- Restore F6 and other mod controls after native campaign movies replace Warcraft's window; also recover a known native input-procedure reset and log attachment/destruction/recovery.
- Add N to disable/restore fog of war in offline FPS, including the unexplored black mask; restore each map's switches for RTS, menus and cinematics.
- Add `[Interface] DisableFogOfWar=false` (`true`/`false` or `1`/`0`), startup/F8 reload and upgrade preservation; N changes only the session.
- Remove Sword and Melee RMB shortcut hints from the HUD while keeping their controls.
- Remove permanent WASD/reload/jump/crouch HUD hints; retain mod-specific shortcuts and creator credit.
- Add V to show/hide horizontal speed in CS/GoldSrc units/s, matching MaxBunnySpeed and excluding vertical velocity.
- Add `[Interface] ShowSpeed=false` (`true`/`false` or `1`/`0`); startup/F8 restores this preference, V changes only the session, and updates preserve custom values.
- Display compact English update summaries in both launchers; keep full GitHub documentation out of the changes box and prefer reviewed summaries over legacy release prose.
- Support hidden launcher-summary metadata, translate the accurate 0.6.1 fallback list, and check real full-body API responses, dialog resolution and both packaged EXEs.
- Make manual Update download and reinstall GitHub's latest stable release even when version/revision already match; startup checks still avoid repeated prompts.
- Share full Warcraft file refresh between Install and Update, repairing missing/corrupt resources while preserving saves, INI preferences and private-only content. Back up edited original maps/campaign files before replacement.
- Apply the current runtime repair policy when reinstalling older GitHub packages whose setup skips existing Game folders; retain Player/Developer mode, verified packages and running-game protection.
- Disable AWP instant enemy kills by default (`[Damage] AWPOneShot=false`); ordinary damage remains 115 before native armor.
- Require explicit `true`/`1` for AWP finishing; missing/empty/invalid values use false. F8 reload and custom-value preservation remain supported.
- Draw hip-fire and scope reticles with filled geometry while preserving their shape and inherited OpenGL state.
- Add configurable bounded diagnostics, dedicated reticle draw/skip/error records and Read logs in both launchers.
- Show reviewed per-line update changes in both launchers, including the GitHub API rate-limit path.

- Add accelerating A/D-and-mouse air strafes, held-Space autojump and live movement tuning.
- Use air acceleration immediately on takeoff and retain momentum through landings. Straight jumps preserve the run-up; default `JumpBoostPercent=0` disables the separate automatic takeoff bonus.
- Bound accumulated speed with `MaxBunnySpeed=1000` GoldSrc units/s; disabling BunnyHop clips excess momentum on each takeoff. Preserve custom boosts and caps during upgrades.
- Allow height-aware jumps over low objects and gradual gravity-driven falls from cliffs.
- Preserve tall/unknown blockers, swept collision, bridge surfaces, map bounds and spell movement restrictions.
- Add editable `[Movement]` settings, migration preservation and independent/native movement oracles.
- Merge all completed feature histories through release/0.6.1 and master, then create release/0.7.0; retain previous branches and immutable tags.
- Publish both launcher variants, source-only packages, verified hashes and complete notes since v0.6.1; see [RELEASE-0.7.0](docs/RELEASE-0.7.0.md).

## 0.6.1 вЂ” 2026-10-09

- Add `[Audio] CSVolumePercent` (0вЂ“100, default 100) for CS shots, reloads, footsteps, melee and C4.
- Apply the CS-only master gain at startup and on F8, including queued/playing sounds; preserve per-event levels and Warcraft's own audio.
- Validate missing/empty/malformed/non-finite values and clamp finite percentages; expose native gain readback for diagnostics.
- Preserve custom audio values while migrating old configs; cover default insertion, repeat setup and duplicate-section/key prevention.
- Add and install `configurable-game-settings`: new game tuning must ship config keys, validated defaults, reload semantics and update preservation.
- Update installed/repository `feature-release-workflow` to require all release changes in Release Notes, based on the previous immutable published tag; reuse a version explicitly supplied by the user.
- Adopt both rules in AGENTS/contributor guidance; update README, installation/troubleshooting, module ownership, validation notes and current version/branch/download references.
- Add real INI/audio migration checks and a production-XAudio2 test with silent synthetic PCM, live gain changes, mute/restore and defensive bounds.
- Rebuild both launcher variants, audited sources, checksums, updater manifest and source-only distribution for 0.6.1; preserve previous branches/tags and update compatibility.

Complete changes since v0.6.0 and validation scope: [RELEASE-0.6.1](docs/RELEASE-0.6.1.md).

## 0.6.0 вЂ” 2026-10-09

- Merge all completed feature work since 0.5.1, retaining earlier release/feature branches and immutable tags.
- Add remembered RoC/TFT selection in both launchers and `play-roc.bat` with forwarded map/window options.
- Set fullscreen DPI awareness and recover weapon/menu/sky graphics after focus and OpenGL context changes.
- Add the CS-style gold buy menu, mouse/number navigation, hand-free private previews, live details and `.` quick ammo.
- Configure weapon/ammunition prices, default reduced weapon prices, starting kits and native shop/building buy zones.
- Grant starting equipment once per map; F7 freely refills carried equipment and F9 freely grants all equipment/ammo.
- Allow multiple planted C4 charges, independent simulation-time fuses/damage snapshots and nearest-fuse HUD.
- Add optional private CS sky conversion, tileset mapping, correctly aligned cube faces and isolated GL upload state.
- Enable stock Warcraft summer/winter FPS skies by default while preserving existing map skies and RTS/cinematic state.
- Preserve native hero portraits, expand the FPS viewport, fix near-ground clipping and traverse native walkable bridges.
- Bring AWP/USP viewmodels closer; filter world labels by camera and configurable distance without changing RTS text.
- Organize native/launcher sources by responsibility; retain published entry points and share the buy catalog.
- Merge missing INI defaults without overwriting player settings; document all features, installation and limitations.
- Update the main README, requirements links and branch references; briefly explain recorded antivirus detections
  and their likely unsigned-loader/hook heuristic cause, without claiming vendor confirmation.

Full release notes, downloads and validation scope: [RELEASE-0.6.0](docs/RELEASE-0.6.0.md).

## 0.5.1 вЂ” 2026-10-07

- Publish a source-only `WarcraftCSLauncher.exe` in distribution ZIPs and a separate `WarcraftCSLauncher_DLL_Included.exe` for ready mod modules.
- Separate Install (embedded version) and Update (latest stable GitHub project release); source-only launchers expose one Install button.
- Preserve launcher variants during verified updates, with independent executable/native-payload hashes and explicit dependency consent.
- Add `[Damage] FriendlyFirePercent` (0вЂ“100, default 50) for firearms, melee and C4 against owned/allied units and buildings.
- Apply F8 reloads to subsequent contacts and snapshot C4's percentage at planting; zero allied damage emits no native damage event.

Validation: all nine C++ suites and launcher/update regressions pass. Real package checks verify
separate executable hashes, identical source snapshots, native-free public ZIPs and native payload integrity.

## 0.5.0 вЂ” 2026-10-06

- Show a release-version/notes confirmation dialog; declining leaves the current installation untouched.
- Add Player and Developer installation buttons, with explicit dependency information.
- Embed verified x86 project modules and their notices for Player setup without Build Tools/SDK.
- Keep original Warcraft libraries local; bind the Player payload to its source snapshot and supported Miles ABI.
- Fall back to GitHub latest-asset manifest downloads on API rate limits.

- Remove the automatic-install checkbox; always check releases at launcher startup and offer discovered updates through an explicit confirmation dialog.
- Restore saved folders without inheriting legacy automatic-install permission.

Validation: launcher/payload regressions and isolated Player/Developer installs pass.
Older updater-enabled launchers accept the new bundle; saves and settings are preserved.

## 0.4.0 вЂ” 2026-10-06

- Add a complete requirements file for users, source builds and release updater assets.
- Pick authored tree trunk triangles instead of canopy boxes; preserve ordinary gate/model-bound behavior.
- Add mouse-wheel weapon cycling with wraparound, high-resolution fractions and focus/status reset.
- Fix Warcraft sound initialization by copying the owner's `redist/miles` providers/codecs during setup.
- Repair missing Miles files in existing marked private runtimes; detect incomplete source installs before dependency downloads.
- Include remembered private sword selection and the Miles audio runtime repair.
- Add launcher startup release checks, saved opt-in automatic updates, SHA256 verification and safe EXE replacement/restart.
- Update complete source/build assets while preserving saves, settings and remembered private sword paths; refuse updates during a running match.
- Publish an exact source payload, updater manifest and distribution ZIP as release assets; retain immutable tags and source-only Git history.

Validation: all nine C++ suites and a disposable native tree/gate/wheel session pass.
Requirements are included in both the source snapshot and distribution ZIP.

## 0.3.0 вЂ” 2026-10-06

- Windows client EXE with game-folder selection, explicit download/install consent, live setup log and Play.
- Embedded audited source payload; clients do not need Git or a separate source archive.
- Install missing Python and signed Microsoft C++ tools/Windows SDK; reuse existing dependencies.
- Preserve owned game installations through the private-runtime setup pipeline.
- English installation guide with exact folder examples, first launch, updates, saves and removal.
- Support Unicode/space/ampersand/apostrophe installation paths in the background setup and native build.
- Merge release/0.2.0 into master before starting release/0.3.0; retain previous branches and tags.

Validation: launcher consent/path/source-payload tests and all seven C++ logic suites pass;
complete isolated client setup passed using existing Python/MSVC/SDK. Fresh vendor installation
on a clean Windows machine and complete campaign compatibility remain unverified.

## 0.2.0 вЂ” 2026-10-06

- Use the project's original straight silver sword with a gold guard and leather grip.
- Generate sword geometry from project-owned source; import hands/animations only from the owner's CS knife.
- Remove external sword-model selection/import from this release; no downloaded sword asset is distributed.
- Translate the complete README to English and update repository/installation links.
- Start release/0.2.0 after merging the 0.1.0 baseline into master; retain all old branches and tags.

## 0.1.0 вЂ” 2026-10-06

Initial source-only release of Warcraft CS by Yazgul.

- Global offline FPS mode for Warcraft III 1.26a x86: native units, maps and campaign rules.
- Seven weapon slots, melee secondary attacks, C4, AWP scope, model/sound import from owned CS files.
- Classic CS punch calculations, movement, relative mouse input, fullscreen HUD and Alt-Tab resource recovery.
- Native status restrictions, model-bound targeting, destructable gates, friendly damage coefficient.
- Real item/rune pickup, configurable ammunition/damage, owned-unit combat/passive squads and J release.
- Portable Windows setup/build/launch; separate private runtime and pinned external MinHook build.
- Seven independent C++ regression suites, source-only staged/release audit, credits/license notices.
- Feature/release/master skill and retained release/0.1.0 plus publication feature branch.

Known limits: offline only; supported host version only; approximate model-bound hitboxes;
full CS spread/client behavior and all custom-map/campaign compatibility are not implemented/verified.
Game content, custom sword assets and compiled binaries are intentionally excluded.
