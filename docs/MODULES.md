# Source modules

`launcher/game/GameRenderer` owns validated renderer selection and the 1.26a
compatibility preflight; `GameEdition` independently owns RoC/TFT tokens. The
existing `ClientRequest` carries both preferences through install/update/play.
`setup/direct3d-runtime.ps1` owns pinned-file validation, private installation and
wrapper backups; `tools/fetch-direct3d.ps1` owns upstream acquisition and
`tools/build-direct3d-bridge.ps1` owns the checked static-CRT bridge build. Player
packaging reuses those files and notices without changing native presentation
or gameplay modules. The D3D8-to-9 bridge adapts 1.26a to the existing D3D9 renderer.

`GameBuild` owns exact host identity and address translation for 1.26a/experimental 1.27a.
Unavailable internal adapters retain native behavior instead of installing legacy addresses.
`D3DHooks` owns D3D9 interception. `GraphicsApi` adapts current presentation calls;
`D3DGraphics` owns draw/state/matrix isolation, `D3DTextures` owns texture lifetime,
and `RenderMatrix` owns portable matrix math. Gameplay and layout keep their existing
modules. `D3DNativeUi` owns native draw-slot filtering, hardware cursor ownership
and the portrait target/square camera. `D3DMinimap` owns the private native UI
color/depth canvas, GPU crop and restoration before the mod canvas. Portrait capture
can nest inside that target. `NativeMinimap` supplies the same evaluated bounds
to both backends; `Overlay` retains common HUD placement and existing inventory
adapters. The WGL captures retain their own context resources. See `PORT-1.27.md`.

`HudDiagnostics` samples phase-specific GL state independently of drawing, preserves
queried texture state and the GL error queue, and records labelled font lifecycle
events. The existing platform `DiagnosticLog` still owns serialization and rotation.

`GraphicsDiagnostics` is a scoped presentation observer for every-pass GL error checkpoints and
supported driver debug messages, with bounded duplicate suppression and exact callback-state
restoration. D3D adapters report failed HRESULT operations through the same observer. It consumes
queued errors deliberately, unlike the observational `HudDiagnostics` snapshots. `NativeFaultDiagnostics`
is a separate platform adapter used by existing runtime SEH filters: it records original exception
context/module offsets and a bounded best-effort frame-pointer chain without taking over recovery
or installing a process-wide handler. `tools/test-graphics-diagnostics.ps1` exercises real hidden WGL
driver delivery, callback ownership, opt-out, suppression and an actual recovered Windows exception.

`HudRenderState` owns a scoped raster/texture reset for the flat HUD after the 3D
world/weapon passes. It resets blend equations and MSAA coverage as well as fixed
function masks/texture targets, then restores the caller state before the outer
overlay scope returns to Warcraft. It does not change gameplay or frame timing.

`HudFont` owns the HUD's GDI glyph rasterization and OpenGL alpha atlas in presentation. It uses
a separate memory DC and textured quads, avoiding driver-generated WGL bitmap display lists.
Each font retains its glyph bearings and advances; upload/draw state is restored after use.
`Overlay` owns the hint/status/spell sizes and forgets their GPU names on context replacement.
The hidden-window `test-hud-font.ps1` oracle checks actual text pixels without game assets or input.

`NativeHoverView` owns the verified 1.26a preselection setter and its two UI switches. Live F9 can
temporarily hide native hover circles independently of GL HUD rendering. Map/script setter requests
update the saved preference while hidden; leaving F9 or disabling the option restores the latest pair.
Map replacement discards old UI ownership without writing stale storage. Selection groups and other
view modes retain their native behavior.

`NativeFramePresentation` observes 1.26a's verified frame-batch completion inside the native present
method. The HUD no longer depends on which Windows/OpenGL swap entry point presents the frame.
Upload/readback flushes outside that method and nested callbacks do not compose a second HUD.
The runtime consumes a completed world once per presentation, independent of frame duration,
and resets that pending submission at map boundaries. `NativeMinimap` restores the buffer it captured
even if the HUD target changes. Unknown native signatures retain the previous API swap hook.

The third-person prototype uses `ThirdPersonPose`/`ThirdPersonCamera` in presentation for boom geometry
and native collision clearance. `HeroAttackTimeline` in combat owns delayed, single-contact swings and
recovery. `UnitAttack` in platform adapts those swings and aimed ranged launches to native animation, widget geometry and
unit/widget damage effects; it never issues pursuit orders. Ranged gate launches use
`NativeAttackTarget` identities in `NativeWeaponAttack` through prepare/impact/cleanup, including
callback removal/reuse; missiles remain engine-owned after dispatch. `HeroAttackPose` owns the independent visual clock;
`UnitAttackAnimation` selects authored attack variations, repairs native pose resets at their elapsed
clip time and restores the native playback rate. It temporarily disables native pose blending during
the swing, restoring the previous mode on release without changing the map's blend duration.
Playback reads the model's active clip rather than treating a pending sprite request as Stand.
The native model selector restores the exact authored index synchronously before elapsed-time seeking,
without adding sprite-queue requests or issuing gameplay orders during pose repair.
The controller defers native Stop cleanup while that manual pose is active; autonomous combat
and movement remain suppressed by the existing native guards.
Locomotion resumes after that visual clock ends,
while `HeroAttackTimeline` continues to enforce contact and cooldown. `NativeOrderCancellation` tracks retained targets per actor
without native dependencies. The native weapon adapter distinguishes scoped manual dispatch from automatic
preparation/impact, sharing passive squad predicates with the order guard. The controller coordinates these collaborators
and keeps actor render visibility separate from movement/spell ownership.

`HeroMotionState` in presentation tracks physical takeoff/air/landing and ground locomotion transitions.
`ModelMotionProfile` parses owned MDX sequences and UnitUI gait speeds without native/platform dependencies.
`UnitLocomotion` selects those per-unit sequences/rates and yields to casts/stuns/attacks; model/type changes reset metadata.
It compares the live native sequence before reusing a cached pose, repairing AI overwrites without resetting a correct gait.
`UnitAttack::Preview` returns a read-only `UnitAimTarget` using the same geometry as attack contact;
`TargetMarkerView` projects that snapshot without native handle reads or gameplay writes.
The snapshot supplies terrain-conforming circle vertices and enemy classification; the renderer keeps
world depth for the ring and screen-space brackets, without altering native unit selection.
`Combat.TargetAllies` is shared by direct-contact prediction and damage. The Q mailbox toggles it
on the game thread and persists it through `GameplaySettings::SaveAllyTargeting`; protected allied units/buildings are filtered before nearest-contact selection
in FPS, TPS melee and both TPS ranged traces. Movement collisions remain native. Area bombs and native spells keep
their own policies. Startup/F8 reloads the saved preference and config upgrades preserve custom shortcuts.
`ActorJumpView` captures the native model's skinning inputs between world passes and applies physical
body elevation through that same skinning function for world drawing. It restores the model/cache
at the presentation boundary before the next tick. It does not write unit fly height, grant
flying movement or change pathing. Map unload discards stale model pointers.
For manual attacks it also refreshes the grounded body's native skinning after the controller
repairs elapsed clip time; advancing an animation clock alone cannot update already cached bones.

Sources are grouped by responsibility. Includes name the owning module explicitly; each public type
has its own header and implementation. There is no global header search path masking dependencies.

| Module | Responsibility |
| --- | --- |
| `src/runtime` | DLL startup, hook installation, frame composition and fault boundaries |
| `src/platform` | Verified Warcraft/Windows adapters and native camera/AI/render/world integration |
| `src/input` | Relative mouse input, look angles, weapon/camera wheel accumulation and window-message dispatcher |
| `src/application` | FPS lifecycle and coordination of gameplay systems |
| `src/config` | Validated configuration snapshots |
| `src/movement` | Independent movement physics and validated movement tuning contract |
| `src/input/WeaponSelection.hpp` | Portable pending selection, catalog-derived groups, owned-only cycling and expiry |
| `src/combat` | Weapon/damage/recoil/melee rules, hitboxes, C4 and status effects |
| `src/economy` | Buy catalog, navigation, inventory transactions and shop access |
| `src/inventory` | Item/rune pickup and ammunition recovery |
| `src/squad` | Recruitment, focus orders, follow/fight/release and unreserved army AI policies |
| `src/geometry` | Model bounds, transformations, ray tests and tree meshes |
| `src/audio` | CS sample loading/mixing and its configurable master gain, separate from Warcraft's audio |
| `src/presentation` | FPS camera/HUD/menu/weapon/scope/sky rendering and world-label visibility |

`src/Plugin.cpp` remains the tiny public DLL entry point because published launcher archive validators
require that path. It delegates to `PluginRuntime::Attach`, with no gameplay or input implementation.
`src/runtime/MilesLoader.cpp` builds as a separate sound-forwarding DLL.

Launcher sources use `launcher/app`, `game`, `installation`, `updates` and `packaging`; C# builds discover
sources recursively. Launcher PowerShell entry points keep their published paths. Native builds list
production sources explicitly; verification scenes remain opt-in under `tests`.

`UpdateSelection` distinguishes passive release checks from explicit latest-release reinstalls.
`setup/warcraft-runtime.ps1` owns the common game-file refresh and content-backup policy; setup owns
asset conversion and mod installation. `SetupRunner` invokes the embedded compatibility adapter
before older downloaded installers that lack the new refresh module. Neither path deletes save/config
folders or terminates Warcraft. Reinstall tests use synthetic resources and a version-metadata DLL;
they do not access an actual game installation or run its executable.

`InputDispatcher` translates input into game-thread command mailboxes and returns an optional result.
`CameraWheel` retains fractional wheel notches and clamps the third-person session boom distance.
The controller routes TPS zoom separately from FPS weapon scrolling and supplies the same zoom snapshot
to both camera positioning and reticle rays. Native collision remains in `ThirdPersonCamera`.
The window procedure forwards unconsumed messages and owns render-resource focus recovery. Native
adapters isolate engine layouts; `WorldLabelVisibility` consumes a camera snapshot without Warcraft,
Windows or OpenGL dependencies. Weapon data lives in `combat/Weapon.hpp` independently of the controller.

`GameWindowInput` owns the current render-window subclass and a forwarding procedure per live HWND.
The runtime observes that binding on each swap, including after a native movie destroys/replaces
the game window. A known native-procedure reset is repaired; an external subclass already forwarding
into the mod is preserved to avoid a recursive chain. Final destruction forwards native cleanup before
discarding the record. The adapter logs attachment, recovery failures and destruction. Its hidden-window
regression uses actual Win32 message forwarding without game files or a visible test window.

`MovementPhysics` integrates momentum, jumping and gravity without engine calls. `WarcraftCollision`
sweeps the player footprint against terrain, native bridge decks and solid model volumes supplied by
`MovementObstacles`. The obstacle adapter owns model/type caches and clears them on map changes.
`UnitCollisionProfile` reads live Warcraft collision radius/mask, converts native cell radii to world units
and separates non-colliding map helpers from visual bodies. Movement consumes this profile independently
of weapon target filtering; unknown profiles retain conservative geometry. Grounded support follows each sweep substep.
Reachable terrain/solid tops under the swept footprint supply landing support, including partial lip
contact. Native cliff classification probes both terrain-cell rings so its lower-bank pathing cell
does not become an unknown blocker after landing. Bridge deck clearance retains its full-footprint checks.
Position commit uses exact XY within the existing two-footprint contact neighborhood on either side
of known solids and beside verified cliff transitions, even when the center cell is clear.
The sweep owns approach/slide/retreat there, so native nearest-cell placement never
publishes a transient sprite relocation. Open-space unknown blockers retain native placement checks.
`GameplaySettings` loads movement tuning; the controller applies that snapshot on startup and F8.

`RenderSettings` is part of that validated snapshot. `NativeFrameRate` verifies the supported native
render-demand instruction, owns its one-byte in-memory override and paces presentation with the
Windows performance counter and waitable timer. MaxFPS=0 restores the original operand. The runtime
consumes settings on each swap; Warcraft simulation clocks are untouched. The controller uses steady
elapsed time for movement, while existing weapon/event deadlines keep their established clock.
`NativeCameraFarClip` owns the active FPS channel's ceiling and restores it through native generation
keys, preserving other cameras and foreign edits. `WarcraftApi` validates and resolves that 1.26a
channel; no shared constant or game file is patched. `FirstPersonCamera` writes the configured far
clip and `FpsProjection` matches the live native field when adjusting the world's near plane,
leaving portrait and native cinematic projection independent, including a clamped fallback.

`MapCameraGuard` also removes the verified 1.26a camera helper's averaged terrain Z only
for the controlled live FPS camera, during owned setters and deferred render evaluations.
`FirstPersonCamera` then supplies the physical eye height directly. Signature rejection
retains the previous public-API compensation; RTS and unrelated cameras keep native terrain.
The opt-in `CameraTerrainScene` exercises real slope, walking and jumping submissions in
movement-test builds; it is excluded from normal clients. Physical movement is independent.

Tree movement bounds reuse `TreeTrunkMesh`'s standing ground-connected components, the same decoded
geometry used for destructable shots. Canopy cards and death-only stumps do not expand the player
blocker. The platform adapter applies the existing native sprite transform and visual scale to those
local trunk bounds; unreadable tree geometry supplies no broad box and retains native pathing checks.
`NativeMinimap` filters projected native map batches and crops them into a context-owned texture, restores the world
under the original map location, and composites the result after the FPS scope/weapon pass. It changes
neither frame anchors nor map bounds/fog rules. `MinimapLayout` shares the new rectangle with `HudHints`
so compact shortcut rows wrap before it. M follows the existing input mailbox; configuration supplies
startup/F8 visibility and bounded size. Leaving FPS uses the unchanged native RTS map.
Non-map console/portrait batches are rejected; rectangular level letterboxing stays black.

The optional speed counter uses the same boundaries: `InputDispatcher` queues a V press,
`ShooterController` owns session visibility and exposes horizontal speed in CS units, and `Overlay`
draws the counter above status notices. `GameplaySettings` supplies the startup/F8 preference;
the display switch never mutates physics or writes the player's configuration.

`NativeFogOfWar` owns the independent native fog/mask snapshot and temporary offline FPS override.
The controller queues N through the input mailbox, applies the session/config preference and restores
visibility on FPS exit or suspension. Map unload forgets the old snapshot rather than writing it
into another map. The adapter's state transitions are tested with synthetic native switches.

Apply SOLID at real boundaries: cohesive collaborators, explicit dependencies, narrow contracts and
composition. Adapter isolates native hooks; dispatcher/mailbox separates input from simulation timing.
The application controller still contains legacy combat and diagnostic paths; this change does not
claim to finish all future decomposition.

`ReticleView` owns hip-fire and scope aiming marks. It uses filled rectangles while retaining the
existing center, recoil gap and thickness, and restores inherited polygon/color state after drawing.
`ReticleDiagnostics` observes its draw preparation/completion and the runtime/overlay skip paths;
it shares the bounded logger's interval and master switch without owning geometry or files.
`tools/test-reticle-view.ps1` verifies real pixels in a hidden native WGL window, including zero
line stipple, color masks, polygon stipple and wireframe state. Run it separately on a Windows desktop;
the numerical suites do not require an OpenGL window. This regression does not establish the cause
of an individual GPU-driver report without reproducing that user's installation.

`DiagnosticLog` is a Windows adapter for synchronized UTF-8 records, bounded files and session archives.
`LoggingSettings` joins the existing validated INI snapshot and F8 reload path. Input/runtime/presentation
emit events or sampled summaries without owning files. Launcher `diagnostics` owns discovery, bounded
shared reads, persistent launcher records and full export; `LogViewerForm` is its UI adapter.

After structural changes, run native tests, launcher tests, the native build and both launcher package
checks. Hook/render changes also require a disposable native map. Never run fixtures in user saves.

`UnitHealthBars` samples living visible allied/enemy units on the game thread and owns per-map model
bounds. It passes handle-free `UnitHealthBar` snapshots to `HealthBarRenderer`; `HealthBarView` owns
portable projection/fill rules. The renderer depth-tests before the weapon clears world depth and
restores GL state. The runtime extracts actual projection scales (Warcraft's FOV field is diagonal).
K queues one shared switch in the controller, which saves both existing team INI keys.

`ShortcutBindings` is the keyboard snapshot shared by input dispatch, held movement and HUD labels;
`GameplaySettings` validates the whole layout before replacing it. `HeroSpellSlots` parses owned
table metadata without engine pointers or bundled game data. `NativeSpellSlots` reads supported
learned ability order getters and the actor-matched command card's verified point-only order flags.
`SpellAim` keeps terrain intersection separate from nearest-widget selection, so a nearby ally
cannot replace the ground aim. `SpellCaster` applies that native point-only policy in auto mode,
without a spell-name/rawcode whitelist, and preserves explicit target overrides.
`SpellCaster` owns only explicitly requested spell orders until native completion or deliberate
player interruption; the controller yields its ordinary order cancellation and firing during that
state. Map reset forgets orders without issuing commands to unloaded handles.

`SpellHudSlot` carries value snapshots of the same four ability slots used by casting.
`NativeAbilityInfo` safely reads native per-level Art and the verified CAgentTimer simulation clock;
`NativeArchive` reads bounded current-map-priority archive data. `SpellIconImage` decodes BLP1 palettes
and delegates JPEG/PNG to Windows WIC. `SpellHudView` owns context/map-specific GL texture caches,
while `SpellHudLayout` bounds the bottom-right row independently of the engine. No game art is packaged.
The controller samples state on the game thread; rendering keeps no native ability pointer.

`NativeMovementGuard` intercepts the actor's native walking motor without writing unit speed/root
counters. The native stats call path, explicit spell approach, other units and RTS remain native.
`SquadFormation` computes rear slots and side escape waypoints independently of Warcraft;
`SquadController` issues bounded native travel orders and retains pathfinding/ownership checks.
`NativeAbilitySlots` samples the live ability chain; `NativeAbilityInfo` reads object-editor research
positions/art even for unlearned skills. `NativeCommandCard` matches actual actor-owned ability
objects before reading current art and mana costs. `SpellCooldownWedge` supplies bounded radial
geometry from native remaining/default cooldown values.

The combined FPS overlay draws health bars against retained world depth before the weapon pass,
then composites the captured native minimap and wraps shortcut hints using the current bindings.
NativeMinimap prevents native map batches from writing world depth; ShortcutBindings owns its M
switch alongside the spell and display controls, including validation and live config reload.

`UnitScaling` owns portable cadence/movement formulas; `UnitStatus` samples Warcraft's live IAS
multiplier without writing attack fields. `GameplaySettings.maxReserve` supplies every controller
inventory/recovery capacity. F8 clamps existing surplus reserve and resets fractional rune credit.
`MapSpellCharges` recognizes DotA Shrapnel's existing actor/skill/marker facts and produces a value
snapshot only. SpellCaster checks the shared native snapshot before replacing an active order.

`NativeWeaponAttack` owns the version-verified native preparation/impact/cleanup adapter. A thread-local
manual-contact scope also gates native wind-up retries and attack poses, preventing retained AI targets
from restarting ranged attack animations. Ordinary RTS units and active fighting followers remain native.
The same contact scope substitutes only the weapon's base roll during synchronous impact; native passive rolls,
critical modifiers and attack callbacks remain engine-owned. It temporarily uses instant impact and
extends only the matching primary impact range getter to the verified ray contact's center distance.
Ordinary RTS, unrelated components, secondary attack ranges and preparation keep native range. It
restores the original component only while the same actor still owns it. A handled miss never causes
a second direct-damage hit. `ShooterController` selects this path after contact/friendly scaling; C4,
destructables, explicit legacy AWP finishing and disabled/unavailable effects use the direct path.

`WeaponTargetFilter` reads native Locust, immunity and live collision facts for confirmed ray
candidates. The controller applies its configurable policy before selecting the nearest unit;
skipped effects never shorten the ray. Unit type IDs and model filenames are not special-cased.
Unknown collision facts remain opaque; heroes, structures and physical immune units retain collision.

`WarcraftApi.AttackSurface` samples the verified solid terrain/walkable-height path independently
of liquid-aware `Ground`. FPS contact and TPS prediction/contact consume the same configurable
water policy; physics, camera height and spell point picking keep their existing height contracts.

`ControlView` is the single source of truth for weapon FPS, third person and unarmed unit FPS.
`ShooterController.SelectView` cancels owned input/combat state before switching, while the input,
movement and presentation adapters apply that mode's policies. Unit view shares native attack
suppression with controlled modes, retains incoming damage and omits both overlay and native UI.
`FpsCombatGuard` also owns the verified native attack-target eligibility adapter for F9's configurable
enemy policy. A live controller predicate scopes it to the protected mode; ordinary actor flags and
native target-reference getters remain intact. `NativeWeaponAttack` shares the contact guard so a
previous wind-up cannot bypass a newly enabled policy. Spell/area damage remains native.

`NativeUnitHud` samples immutable `UnitHudSnapshot` values on the game thread: unit resources,
map-resolved XP thresholds and native inventory art/counts. `UnitHudView` draws those values with
portable `UnitHudLayout` geometry. `HudIconCache` shares bounded map-scoped icon decoding/upload
between spell and inventory views. `NativePortrait` redirects the native small portrait viewport
into its own framebuffer with private depth; world color/depth remain untouched. Runtime hooks
own capture timing and context/map/actor invalidation. F9 opts out before any overlay submission.

`InventoryControls` owns bounded input events and stable-item click/drag/target state.
`InventoryController` coordinates those events with native adapters on the game thread;
`NativeItemCommands` reads verified inventory order target forms and `NativeItemActions` owns
explicit item orders during native approach/channeling. The shared cast predicate grants only
those orders movement ownership. `InventoryHudView` presents slot labels, gestures and a free cursor.
Windows messages never issue item natives; focus, reload and mode/map boundaries discard gestures.
Gesture cleanup preserves the session Tab choice; `ApplySettings` and map reset restore the validated
`NumbersByDefault` preference. `ThirdPersonCamera` keeps building proxy collision opt-in independently
of terrain/destructable boom collision and combat hitboxes. Zero-entry containing proxies, including bridge/ladder volumes,
are excluded from camera obstruction: their enclosing boxes are not separate walls. Combat hitboxes and movement keep their own contracts.

`SquadSpellPolicy` owns portable readiness/healing decisions. `SquadSpellcasting` adapts live
ability slots and native orders to a bounded stock-order catalog, using shared native ability,
cooldown and map-charge readers. Its order ownership protects wind-up/channeling from formation
refresh. `SquadController` selects combat/passive behavior, validates player ownership and cancels
only adapter-owned casts on opt-out. No map ability, mana or autocast switch is rewritten.
`PreserveCombatOrders` keeps a live native attack target ahead of corridor/leash travel while
leaving idle attack-move refresh and native spell decisions available. Combat ownership transitions
are logged once rather than per frame. Explicit common-target approaches already outrank all travel.

`SquadFocusFire` owns temporary all-owned-unit target orders independently of H/O membership and
formation policy. `SquadController` composes it: explicit focus temporarily overrides passive guards,
formation/leash and spell AI for those exact unit identities; cancellation restores the underlying
policy. `FpsCombatGuard::AttackTargetObject` exposes the verified read-only native target identity
so cleanup yields to newer orders and discards transferred/reused handles. Window input queues one
game-thread command; reticle picking and native order dispatch stay outside presentation callbacks.
`SquadOrderMode` owns the F6 Y toggle and predicts queued toggles for short key+RMB sequences.
Window input never changes native combat state; the controller applies mode changes before RMB work,
clears pending toggles at focus/view/menu boundaries and restores the saved default on startup/F8/map reset.
`NativeAttackTarget` owns typed current-map widget identity/life access for common-target orders,
including destructable gates. `AttackTargetPolicy` distinguishes neutral ownership from player allies
for direct contact filtering/damage; native eligibility and quest immunity remain engine-owned.

The shared `SpellAim` ray picker accepts an optional action-owned unit filter before hit distance
is shortened. Squad orders supply their allegiance policy so protected allies cannot hide enemies;
spell/item callers retain unfiltered ally selection. Ground/destructable occlusion stays shared.

`ArmyPlan` owns portable stock-race/custom build plans. `ArmyAI` runs a bounded game-thread
decision loop through Warcraft's normal construction/training/resource orders. The controller
supplies exact squad/focus exclusions, while the adapter rechecks local ownership and actor
availability before enumerating any army. `ArmyAISettings.followActor` independently controls automatic hero escort (default false); base reserves rally only to a real hall when escort is off. Economy and threat defense remain separate. Construction leases and travel identities prevent
duplicate sites and stale-handle commands. `ShopAccess` shares live merchant/proximity validation;
the controller yields native selection, UI and camera during shop interaction and restores the
captured actor/view on close. `WeaponDefaults` supplies one stable-slot tuning snapshot; validated
config, buy catalog, audio/cache tables, wheel and presentation include every appended weapon.

Army commands execute at the completed native frame boundary, after world/UI/overlay submission.
Build orders can create birth models immediately; issuing them from the earlier world hook can
draw those models before native skinning initializes their matrices. The next frame now performs
its normal model update before drawing newly constructed objects. Training reservations also
survive Warcraft's zero-current-order reports and pause/background gaps.

`ViewmodelMotion` owns the visual-only speed/stance bob; `ViewmodelPose` crosses
the controller/presentation boundary without changing camera or aim. `MuzzleFlashPulse`
consumes accepted-shot serials once and starts their lifetime on presentation.
`GunMesh` samples optional WCM1 muzzle anchors with its animation bones, retaining
legacy geometry fallback. `MuzzleFlashView` draws additive original geometry with
read-only depth and scoped GL state. `tools/test-viewmodel.ps1` verifies the portable
clock/motion, synthetic format conversion and actual hidden-WGL pixels/state.

`ShotgunReload` in combat maps one absolute reload clock to fitted start/insertion/finish
phases. The controller coordinates transitions; `WeaponAnimation` in presentation holds
terminal reload bones until the next phase, while ordinary firing/draw actions still settle
to idle. `GameAudio` reads private timeline metadata independently of optional audio setup.
Its full-magazine reference keeps the same insertion speed for partial reloads.
It also credits each completed insertion once, bounded by actual reserve/capacity;
interruption preserves those transfers and discards the unfinished insertion.
The controller snapshots reload policy and uses a fresh, fireable LMB edge to interrupt.
`UpdateReload` advances the shared absolute clock before buy-menu/inventory early returns. Menu toggles preserve a running animation/audio timeline; ammo transactions only modify reserve, while actual weapon deployment still cancels the previous reload.
`BurstFire` in combat owns bounded Glock secondary scheduling, cooldown, cancellation
and late-frame spacing. The controller debits actual ammo and dispatches individual
contacts, while the presentation starts the burst clip only once.

### Grenades

`GrenadeThrow` owns pin/release/launch/recovery transitions; `GrenadeLaunch` maps CS pitch/speed into world coordinates, inherits motion and clips the forward release against geometry; `GrenadeProjectile` calculates portable swept-hull flight, falloff and exposure. `NativeGrenades` adapts native pause-aware timers, collision snapshots, damage, gates and timed flash disorientation leases. `GrenadeVisual` is a read-only snapshot, and `GrenadeView` draws flights/clouds against native world depth before held-weapon depth clearing, then flash blindness after the HUD. No renderer modifies native targets. Map unload forgets native handles. Tests use synthetic clocks/targets and actual hidden-WGL pixels, without retail assets.

`GrenadeWorldView` owns the three private w_ prop caches and context lifecycle. It reconstructs the native camera projection before weapon depth clearing, preserves original xyz axes and textured geometry, and centers authored offsets on projectile positions. GunMesh shares geometry submission between held and world models. GrenadeView now owns clouds/blindness only.


`WeaponSelection` keeps a pending candidate distinct from equipped combat state. It derives groups
from `BuyCatalog`, accepts the controller's owned/charge availability snapshot and never issues attacks.
`WeaponSelectionView` renders its category strip and owned stack, sharing Overlay's private buy-preview
cache. Window input queues confirm/cancel before weapon fire or squad RMB; the controller consumes that
mailbox, commits through the existing deployment path and suppresses the confirmation trigger until release.
Context transitions discard preview/input state; the ordinary aim and movement adapters remain active.

## Optional RPG ownership

`OptionalWeaponAssets` validates the private conversion seal at startup/F8; the
single `GameplaySettings::WeaponEnabled` predicate gates economy, free grants,
carried menus, direct switching and fire. `export_halflife_rpg.py` discovers loose
or PAK resources and publishes the seal last, independently of required CS conversion.
No optional file becomes a required dependency of the CS-only install path.

`RocketProjectile` owns portable swept launch, motor pulses and steering. `NativeRockets`
adapts native clocks, immutable launches, laser raycasts, sounds and read-only visual
snapshots. `NativeBlast` centralizes HE/RPG cover, native damage callbacks and gate
immunity; flash leases remain in `NativeGrenades`. `RocketView` owns original trail,
glow and airburst effects plus the private model, sharing `WorldEffectPass` with
grenade world rendering. Overlay alone owns OpenGL context replacement. The controller
coordinates accepted shots/reload/laser input rather than implementing projectile physics.

Dollar economy and loot: `economy/DollarWallet` owns per-player map balances;
`CurrencyDisplay` formats UI money. `inventory/WeaponDropRules` selects eligible
content, while `platform/KillReward` snapshots confirmed CS damage deaths and
`DroppedWeapons` owns pause-aware native chest effects. `BuyInventory::Pickup`
applies ownership/ammo rules. The application wires explicit death callbacks from
firearms, HE/C4 and RPG; callbacks never overwrite native gold or item inventories.

## Half-Life energy weapons
`HalfLifeEnergy` owns portable charge/channel lifecycle, cadence and cell expenditure. The controller translates its accepted events into the existing native Strike contact and GameAudio paths. `EnergyBeam` is a read-only contact snapshot; `EnergyBeamView` draws project-authored depth-tested geometry before held-weapon depth clearing. The common GunMesh bob/muzzle/animation path remains shared. `OptionalWeaponAssets` verifies independent seals; export_halflife_weapons converts only owned local/PAK data and publishes each seal last. BuyCatalog registers all four Half-Life weapons once for buying and group 6 selection.

The energy beam samples GunMesh's shared interpolated muzzle anchor; ViewmodelAnchor compensates held/native FOV while preserving bob/roll and camera heading/pitch. EnergyBeamView tapers the near end. Portable projection and actual WGL attachment/pixel/state regressions accompany authorized native mouse purchases, damage/cell/charge/channel/mode/resource checks in an isolated map; ordinary builds exclude fixtures.

Crowbar, Tau and Gluon opt into private WCG3 conversion. It preserves material flags,
normal bones, parent indices, local animation poses and sequence loop flags.
`StudioPose` interpolates local rigid transforms before composing the hierarchy;
`StudioSkin` derives chrome coordinates from animated camera-space normals.
`GunMesh` shares the composed pose between geometry, preview and muzzle attachments.
WCG2 remains supported with its original sampling behavior; RPG and CS conversion
retain that format. Synthetic conversion and hidden-WGL regressions cover both
formats, chrome movement, loop seams and complete terminal action frames.

`TauChargeFeedback` integrates the charge speed ramp as an absolute animation phase;
the existing energy state exposes elapsed charge time without changing resource rules.
`GameAudio` changes a reserved live loop voice's pitch rather than restarting buffers.
`CrowbarContact` owns impact sample selection; the controller supplies native creature,
structure/destructable or terrain classification independently of damage immunity.
The owned converter fits Gluon's front opening in its rigid bone, while `ViewmodelAnchor`
compensates native versus held aspect and near clipping. Overlay samples one animation
time for both beam attachment and held geometry. Synthetic aperture/phase/projection
tests and real XAudio2 pitch/loop tests cover these boundaries.

## Unified Player host adaptation
`setup/MilesForwarderBuilder.cs` parses PE export metadata and writes only the
project-owned proxy's reserved `.wcsfwd` section. `miles-forwarders.ps1` supplies
the existing .NET compilation bridge; `prebuilt-runtime.ps1` verifies package,
host and module identities before deriving/copying bytes. Format-2 runtime
packages list both supported hosts. Legacy packages retain exact host/hash checks.
The original Miles DLL supplies all sound behavior through ordinal forwarders.
`LauncherVariant.IsPreview` reads a build resource; update selection suppresses
only the preview's published base, while ordinary stable selection is unchanged.

`CylinderHull` in geometry owns round-footprint overlap and contact normals. `MovementSurface` carries
local height/normal/walkability into `MovementPhysics`, which owns ground, sliding and head/stance
responses. `WarcraftCollision` adapts live terrain, pathing, transformed solid bounds and native decks,
using a per-update sample cache; it never substitutes a remote terrace height for a local contact.

`CylinderMesh` owns portable cylinder/triangle contact and disk/plane height extrema.
`StaticModelMesh` validates classic MDX arrays, stationary Stand tracks, material solidity and geoset
visibility before publishing any mesh. `ModelBounds` reads owned mounted-map bytes; `MovementObstacles`
caches local and transformed static geometry per map, refreshed when the sprite transform changes.
Only movement opts into the detailed snapshot; grenade/rocket cover keeps the established coarse contract.
Unsupported poses retain prior bounds, trees retain trunk filtering and native bridge decks remain authoritative.

`MovementRecovery` retains a bounded portable history of visited positions. `WarcraftCollision`
revalidates support/clearance and native walkability before restoring one, tracks platform rider
provenance and rejects stale placement history. InputDispatcher queues Home on the controller mailbox;
the controller filters cargo, disabled/casting and cursor states before requesting game-thread recovery.

Movement mode is one validated snapshot field. WarcraftCollision selects center/rim deck support,
CylinderMesh selects whole/reachable triangle support, and MovementObstacles selects native-only
or retained platform surfaces. Classic also gates the complete recovery path and discards its history;
the common acceleration, jump and ground/air motor implementation remains shared.

`ClassicWarcraftCollision` preserves the owned pre-24-October-rework adapter and calls the original
`ResolveClassicFloor` landing rules. WarcraftCollision composes this concrete classic strategy with
its later hull/recovery strategy; one validated mode selects the complete behavior on the game thread.
The acceleration/jump integrator stays shared. The classic adapter source is compared with 2babf58
and exercised with the original historical collision regressions in addition to current mode tests.

`PurchasePayment` chooses a single wallet before `BuyInventory` validates a transaction. The controller commits that wallet only on success; `BuyMenuState` copies wallet/selection information for `BuyMenuView`, whose buttons share `BuyMenuLayout` with mouse dispatch.

`AnimatedHitModel` and `MdxPose` decode bounded classic MDX geometry, sample skeletal/geoset visibility tracks and intersect posed triangles. `UnitHitboxes::IntersectShot` owns verified native pose and full render transforms through `SpriteTransform::ReadScaled`, retaining independent basis, visual scale and position. Unverified transforms/poses use conservative body fallback; its existing volume method remains available to melee/spells/TPS. `WeaponTrace` owns nearest contact filtering and terrain/destructable occlusion. Only fired-round queries opt into candidate pose diagnostics. `HeadshotPolicy` applies direct-firearm eligibility and the configurable multiplier before ordinary native damage.

`ItemHover` owns game-thread ground-item enumeration, read-only occlusion and copied localized names. `GameText` removes Warcraft color/control markup. `ItemNameView` owns a single cached Unicode caption texture per graphics context; no native handles or names are dereferenced by the renderer. F8/map replacement clear hover caches. Project-authored fixtures in `test-precise-shots.ps1` and actual hidden WGL pixels in `test-buy-hover-view.ps1` exercise these modules without controlling the user's game.

`CombatVisuals` owns bounded copied-value blood/critical/casing pools. Confirmed game-thread HP loss creates target feedback and accepted conventional rounds create cases; small physics substeps update visual debris without giving it collision authority. `CombatVisualView` draws world-depth procedural droplets/cases and reuses `ItemNameView::DrawAt` for cached Unicode critical captions. Rendering never queries native unit handles. Controller suspension freezes visual time; map teardown/F8 clear the pools. The dedicated hidden WGL suite verifies actual pixels and state without campaign input.

`NativeHeadGore` owns the verified 1.26a per-instance corpse palette adapter. `AnimatedHitModel` carries the struck named-head root through `UnitHitboxes`/`WeaponTrace`; no camera-space pivot is compared to world coordinates. The adapter validates authored groups and mirrors native visible/enabled geoset eligibility before collapsing selected-head palettes. Invisible geosets retain stale indices and must never be written. Shared MDX, body palettes, other skulls, damage and death events remain native. Tracking is map-local/bounded and typed identity/life checks release resurrected or replaced objects. Synthetic memory fixtures cover selection, skipped stale output, body/other-instance isolation and resurrection.

`FixedFunctionState` owns OpenGL program/pipeline bindings and legacy-program/rasterizer-discard
enables through an RAII scope. `Overlay` uses it around all mod world effects, held models and HUD;
`HudRenderState` additionally uses it for standalone 2D passes. Unlike legacy attribute stacks, the
scope explicitly restores modern program bindings. It returns immediately on the D3D9 adapter.
Raw diagnostics precede isolation. Hidden WGL tests reproduce zero-error missing geometry with
`CURRENT_PROGRAM=0` and a separate pipeline, compare actual weapon/reticle/glyph pixels and verify
restoration across replacement contexts.


### Local head/contact and campaign-economy fixes

`MdxPose::Triangle` exposes barycentric contact weights; `AnimatedHitModel` retains fractional named-head skin influence until the exact accepted contact. `UnitHitboxes` consumes validated head weight policy, while `HeadshotPolicy` separates direct contact eligibility from optional damage bonuses. `CombatVisuals` separates blood from critical captions and receives only confirmed head world contacts.

`ArmyPlan::Detect` prioritizes builders over captured foreign halls. The existing game-thread `ArmyAI` coordinates bounded native construction, worker-centered startup, race-compatible mines and undead lumber ghouls. `ArmyAIStatus` contains the copied domain result; presentation's `ArmyAIStatusText` maps it to HUD text. The controller reports transitions without taking over native orders. No new engine offsets, free units, resource setters or public runtime entry points are introduced.
