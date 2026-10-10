# Source modules

Sources are grouped by responsibility. Includes name the owning module explicitly; each public type
has its own header and implementation. There is no global header search path masking dependencies.

| Module | Responsibility |
| --- | --- |
| `src/runtime` | DLL startup, hook installation, frame composition and fault boundaries |
| `src/platform` | Verified Warcraft/Windows adapters and native camera/AI/render/world integration |
| `src/input` | Relative mouse input, look angles, weapon wheel and window-message dispatcher |
| `src/application` | FPS lifecycle and coordination of gameplay systems |
| `src/config` | Validated configuration snapshots |
| `src/movement` | Independent movement physics and validated movement tuning contract |
| `src/combat` | Weapon/damage/recoil/melee rules, hitboxes, C4 and status effects |
| `src/economy` | Buy catalog, navigation, inventory transactions and shop access |
| `src/inventory` | Item/rune pickup and ammunition recovery |
| `src/squad` | Recruitment and follow/fight/release policies |
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
The window procedure forwards unconsumed messages and owns render-resource focus recovery. Native
adapters isolate engine layouts; `WorldLabelVisibility` consumes a camera snapshot without Warcraft,
Windows or OpenGL dependencies. Weapon data lives in `combat/Weapon.hpp` independently of the controller.

`MovementPhysics` integrates momentum, jumping and gravity without engine calls. `WarcraftCollision`
sweeps the player footprint against terrain, native bridge decks and solid model volumes supplied by
`MovementObstacles`. The obstacle adapter owns model/type caches and clears them on map changes.
`GameplaySettings` loads movement tuning; the controller applies that snapshot on startup and F8.

The optional speed counter uses the same boundaries: `InputDispatcher` queues a V press,
`ShooterController` owns session visibility and exposes horizontal speed in CS units, and `Overlay`
draws the counter above status notices. `GameplaySettings` supplies the startup/F8 preference;
the display switch never mutates physics or writes the player's configuration.

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
