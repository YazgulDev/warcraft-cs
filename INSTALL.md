# Install and play Warcraft CS

You need your own installed Warcraft III **1.26a x86** (Game.dll **1.26.0.6401**)
and Counter-Strike 1.6. Reforged and other Warcraft patches are unsupported.
This offline, single-player prototype does not download games or redistribute their content.
Full campaign/custom-map compatibility has not been verified.
See [REQUIREMENTS.md](REQUIREMENTS.md) for the complete system, owned-file and dependency checklist.

## Windows EXE: recommended for clients

Download `WarcraftCSLauncher.exe` from [GitHub Releases](https://github.com/YazgulDev/warcraft-cs/releases/latest).
Use Windows 10/11 x64. This standard EXE and all public ZIPs contain source without prebuilt DLLs;
installation compiles locally and may download missing Build Tools / Windows SDK (several GB).
For ready mod DLLs, download the separate `WarcraftCSLauncher_DLL_Included.exe` and use Player setup.
Both use Windows' .NET Framework/PowerShell. You do not need Git or Python installed in advance.
The launcher is unsigned; downloaded Python/Microsoft installers have verified vendor signatures.

### 1. Choose your game and installation folders

Open the launcher and use **Browse...** next to each field:

| Launcher field | Choose this | Example |
| --- | --- | --- |
| Warcraft III 1.26a folder | Your Warcraft game root, containing `war3.exe`, `Game.dll`, `Mss32.dll`, `Storm.dll` and the four MPQ archives listed below. | `E:\Warcraft III` |
| Counter-Strike 1.6 folder (cstrike or Half-Life) | The `cstrike` folder containing the loose weapon models and sounds. Its `Half-Life` parent also works. | `C:\SteamGames\steamapps\common\Half-Life\cstrike` |
| Install Warcraft CS here | A dedicated writable folder outside both game installations, with space for another Warcraft copy and dependencies. | Keep the default `%LOCALAPPDATA%\WarcraftCS`, or choose `D:\WarcraftCS`. |

These paths are examples; use wherever your games are actually installed.
For CS installed by Steam, browse its local files and locate `cstrike`.
Do not select the Steam library root, `steamapps`, `models`, a Warcraft `Maps`/`save` folder,
or an individual `.exe` file. The last field is the destination for the mod, not an existing game.
The launcher automatically expands the default local application-data path for your Windows account.

Expected Warcraft archives: `War3.mpq`, `War3x.mpq`, `War3Patch.mpq`, `War3xlocal.mpq`.
The game root must also include `redist/miles` with `Mssfast.m3d`, `Mp3dec.asi` and `Reverb3.flt`.
These owned audio components are copied locally; `Mss32.dll` alone is insufficient for Warcraft sound.
Check the version in `Game.dll` file properties. CS must have
`models/v_ak47.mdl`, `v_m4a1.mdl`, `v_usp.mdl`, `v_awp.mdl`, `v_knife.mdl`, `v_c4.mdl`,
plus `sound/weapons` and `sound/player`. CS is required for your local hands, animations and audio.

### 2. Agree and install

Read the download summary and linked terms. Tick **I agree to download and install...**
(the checkbox starts unchecked), then choose the action available in your launcher:

| Button | For whom | What is installed |
| --- | --- | --- |
| **Install** in the standard EXE | Source installation | Missing Microsoft Build Tools/SDK and pinned MinHook source, then a local DLL build. Existing compatible tools are reused. |
| **Install — Player** in the DLL-included EXE | Players | Bundled project DLLs; no Build Tools or Windows SDK. Python/NumPy may be prepared for owned asset conversion. |
| **Install — Developer** in the DLL-included EXE | Source installation | The same local compilation as the standard EXE. |

Install uses the EXE's embedded version. The separate **Update** button checks GitHub and installs its latest stable project release after confirmation.

No download or installation starts before agreement.

Setup reuses compatible installed dependencies. Both modes prepare missing Python privately;
Developer mode alone prepares Microsoft C++ Build Tools/Windows SDK and checksum-verified MinHook source;
Microsoft setup may require several GB, administrator approval and a restart. Both modes prepare NumPy
for local asset conversion. The original games remain unchanged.

Wait until the launcher reports **Ready**. Errors appear in the log, also saved as
`<installation folder>\install.log`. If Microsoft asks for a Windows restart, restart,
open the EXE again, choose the same folders, agree and retry **Install**.

### 3. Play

Choose **The Frozen Throne** or **Reign of Chaos** in **Game to launch**, then click **Play**.
Both launchers offer this selector and remember it after a successful launch; existing preferences default
to Frozen Throne. Close any other Warcraft window first; the launcher will not restart your active match.
Warcraft opens in fullscreen. Choose **Single Player → Custom Game** or **Campaign**,
start a map, select your own living unit and press **F6**.
Maps do not need editing. Enable FPS again after changing maps.

- WASD/mouse: move and aim; Space: jump; Ctrl: crouch; Shift: walk.
- Left mouse: shoot or melee; right mouse: strong melee attack or AWP zoom.
- 1–7: choose weapons; mouse wheel: previous/next weapon; R: reload; F7: refill all ammunition.
- E: pick up items/runes; H/O: recruit your units; J: release them.
- F6: return to RTS; F10: pause menu; F8: reload configuration.

See [README controls and settings](README.md#controls) for details.
The default starting kit is knife, sword and 20 C4 charges. Press B to buy firearms with Warcraft
gold and `.` to buy current-weapon ammunition. Edit `[Loadout]`, `[Buy]`, weapon prices and `[Sky]`
in `Game/WarcraftCS/WarcraftCS.ini`; F8 reloads settings (starting-kit changes apply to the next map).
Once installed, **Play** does not require download consent or install dependencies.

Use the launcher for either edition so fullscreen rendering uses consistent DPI scaling after Alt-Tab.
For a source checkout, `tools/launch.ps1 -Edition ReignOfChaos` opens the original RoC campaign menus;
`-Edition FrozenThrone` opens TFT. Add `-Windowed` for an optional windowed launch. The supported game
version remains 1.26a, and two-player custom missions still require their intended player count.

## Settings, saves, updates and removal

The EXE creates these files inside your selected installation folder:

- `Game`: private Warcraft runtime. Settings: `Game\WarcraftCS\WarcraftCS.ini`.
- `Game\save`: progress saved in this client copy. Original saves are not imported automatically.
- `sources`: embedded project source, private build dependencies and locally converted assets.
- `install.log`: setup diagnostics; `downloads`/`dependencies`: installer cache/private Python when needed.

To update, save your progress, close the private Warcraft window and click **Update**
with the same destination. The setup only updates its marked private runtime and preserves saves. At startup, a new-release dialog
shows its version/notes and offers **Update** or **Not now**. Confirmation installs in your existing
mode supported by your current launcher. Standard EXEs keep source-only updates and compile locally;
DLL-included EXEs keep their variant and saved Player/Developer mode. Selecting an installation button changes the mode.
Old 0.5.0 Player launchers cannot choose the separately named DLL variant: download the new DLL-included EXE
directly and select the same installation folder. This preserves your progress and avoids an implicit compiler installation.
Back up `Game\save` before updates or removal. If you want existing progress, copy your own original
`save` files into the private runtime while both games are closed, keeping a backup.

If an older client reports "Unable to initialize base sound services", close Warcraft and
run **Install** or **Update** with a launcher containing the Miles setup fix, using the same destination.
Setup restores missing `Game/redist/miles` files from your selected complete Warcraft installation.
It does not download or distribute proprietary audio codecs.

To remove the client, close Warcraft, back up saves and delete the selected installation folder
using Windows. Original game installations remain. Shared Microsoft build tools are removed
separately through Windows Apps settings. Never share the generated `Game` folder or game-asset caches.

## Manual source setup

For developers: install Git for Windows, Python 3.10–3.14 with venv/pip, and Visual Studio 2022
or Build Tools with **Desktop development with C++**, MSVC x86/x64 and Windows SDK.
The manual script requires `cstrike` itself rather than its Half-Life parent.

```powershell
git clone https://github.com/YazgulDev/warcraft-cs.git
cd warcraft-cs
git switch release/0.5.1
.\setup.cmd -WarcraftDirectory "E:\Warcraft III" -CounterStrikeDirectory "C:\SteamGames\steamapps\common\Half-Life\cstrike"
.\play.cmd
```

By default this creates `.local/warcraft-cs`, a private Python venv and machine-specific
`.local/setup.json`. It converts owned CS materials, generates the project's sword and builds
the native mod/proxy locally. No game or dependency binary is committed to this repository.

If Python is not on PATH:

```powershell
.\setup\setup.ps1 -WarcraftDirectory "E:\Warcraft III" -CounterStrikeDirectory "C:\SteamGames\steamapps\common\Half-Life\cstrike" -PythonExecutable "C:\Python312\python.exe"
```

An external sword model is not distributed. If you have permission to use your own copy,
source setup accepts `-SwordModel "C:\PrivateModels\v_grudge_sword.mdl"` and remembers the
private path for later updates. Alternatively, place it in the checkout's
`.local/models/v_grudge_sword.mdl`. Without a selected private model, setup generates the
original sword using your CS knife hands/animations. External model permissions are separate
from the project's code license; keep the model outside Git.

Optional launch arguments:

```powershell
.\play.cmd -Windowed
.\play.cmd -Map "E:\Warcraft III\Maps\(4)LostTemple.w3m"
```

Close Warcraft before rebuilding: `./tools/build.ps1`. Run logic tests: `./tools/test-all.ps1`.
Launcher build/test instructions: [Client launcher](docs/CLIENT-LAUNCHER.md).
Licenses and borrowed work: [NOTICE](NOTICE).
