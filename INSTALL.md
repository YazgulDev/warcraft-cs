# Install and play Warcraft CS

You need your own installed Warcraft III **1.26a x86** (Game.dll **1.26.0.6401**)
and Counter-Strike 1.6. Reforged and other Warcraft patches are unsupported.
This offline, single-player prototype does not download games or redistribute their content.
Full campaign/custom-map compatibility has not been verified.
See [REQUIREMENTS.md](REQUIREMENTS.md) for the complete system, owned-file and dependency checklist.

## Windows EXE: recommended for clients

Download `WarcraftCSLauncher.exe` from [GitHub Releases](https://github.com/YazgulDev/warcraft-cs/releases/latest).
Use Windows 10/11 x64. The EXE embeds project source and uses Windows' .NET Framework/PowerShell;
you do not need Git, Python or Visual Studio installed in advance.
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
(the checkbox starts unchecked), then click **Install / Update**.
No download or installation starts before agreement.

Setup reuses compatible installed dependencies. Missing Python is installed privately;
Microsoft C++ Build Tools/Windows SDK may require several GB, administrator approval and a restart.
NumPy and checksum-verified MinHook are prepared locally. The original games remain unchanged.

Wait until the launcher reports **Ready**. Errors appear in the log, also saved as
`<installation folder>\install.log`. If Microsoft asks for a Windows restart, restart,
open the EXE again, choose the same folders, agree and retry **Install / Update**.

### 3. Play

Click **Play**. Close any other Warcraft window first; the launcher will not restart your active match.
Warcraft opens in fullscreen. Choose **Single Player → Custom Game** or **Campaign**,
start a map, select your own living unit and press **F6**.
Maps do not need editing. Enable FPS again after changing maps.

- WASD/mouse: move and aim; Space: jump; Ctrl: crouch; Shift: walk.
- Left mouse: shoot or melee; right mouse: strong melee attack or AWP zoom.
- 1–7: choose weapons; mouse wheel: previous/next weapon; R: reload; F7: refill all ammunition.
- E: pick up items/runes; H/O: recruit your units; J: release them.
- F6: return to RTS; F10: pause menu; F8: reload configuration.

See [README controls and settings](README.md#controls) for details.
Once installed, **Play** does not require download consent or install dependencies.

## Settings, saves, updates and removal

The EXE creates these files inside your selected installation folder:

- `Game`: private Warcraft runtime. Settings: `Game\WarcraftCS\WarcraftCS.ini`.
- `Game\save`: progress saved in this client copy. Original saves are not imported automatically.
- `sources`: embedded project source, private build dependencies and locally converted assets.
- `install.log`: setup diagnostics; `downloads`/`dependencies`: installer cache/private Python when needed.

To update, save your progress, close the private Warcraft window and run **Install / Update**
with the same destination. The setup only updates its marked private runtime and preserves saves.
Back up `Game\save` before updates or removal. If you want existing progress, copy your own original
`save` files into the private runtime while both games are closed, keeping a backup.

If an older client reports "Unable to initialize base sound services", close Warcraft and
run **Install / Update** with a launcher containing the Miles setup fix, using the same destination.
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
git switch release/0.4.0
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
