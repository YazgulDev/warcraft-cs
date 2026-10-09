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
| `src/movement` | Independent movement physics |
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

`InputDispatcher` translates input into game-thread command mailboxes and returns an optional result.
The window procedure forwards unconsumed messages and owns render-resource focus recovery. Native
adapters isolate engine layouts; `WorldLabelVisibility` consumes a camera snapshot without Warcraft,
Windows or OpenGL dependencies. Weapon data lives in `combat/Weapon.hpp` independently of the controller.

Apply SOLID at real boundaries: cohesive collaborators, explicit dependencies, narrow contracts and
composition. Adapter isolates native hooks; dispatcher/mailbox separates input from simulation timing.
The application controller still contains legacy combat and diagnostic paths; this change does not
claim to finish all future decomposition.

`ReticleView` owns hip-fire and scope aiming marks. It uses filled rectangles while retaining the
existing center, recoil gap and thickness, and restores inherited polygon/color state after drawing.
`tools/test-reticle-view.ps1` verifies real pixels in a hidden native WGL window, including zero
line stipple, color masks, polygon stipple and wireframe state. Run it separately on a Windows desktop;
the numerical suites do not require an OpenGL window. This regression does not establish the cause
of an individual GPU-driver report without reproducing that user's installation.

After structural changes, run native tests, launcher tests, the native build and both launcher package
checks. Hook/render changes also require a disposable native map. Never run fixtures in user saves.
