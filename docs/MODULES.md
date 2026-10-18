# Source modules

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
on the game thread; protected allied units/buildings are filtered before nearest-contact selection
in FPS, TPS melee and both TPS ranged traces. Movement collisions remain native. Area bombs and native spells keep
their own policies. Startup/F8 reloads the saved default and config upgrades preserve custom shortcuts.
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
Position commit uses exact XY within the existing two-footprint contact neighborhood on either side
of known solids. The sweep owns approach/slide/retreat there, so native nearest-cell placement never
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
of terrain/destructable boom collision and combat hitboxes.

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
availability before enumerating any army. Construction leases and travel identities prevent
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
`BurstFire` in combat owns bounded Glock secondary scheduling, cooldown, cancellation
and late-frame spacing. The controller debits actual ammo and dispatches individual
contacts, while the presentation starts the burst clip only once.

### Grenades

`GrenadeThrow` owns pin/release/launch/recovery transitions; `GrenadeProjectile` calculates portable swept-hull flight, falloff and exposure. `NativeGrenades` adapts native pause-aware timers, collision snapshots, damage, gates and timed flash disorientation leases. `GrenadeVisual` is a read-only snapshot, and `GrenadeView` draws flights/clouds against native world depth before held-weapon depth clearing, then flash blindness after the HUD. No renderer modifies native targets. Map unload forgets native handles. Tests use synthetic clocks/targets and actual hidden-WGL pixels, without retail assets.
