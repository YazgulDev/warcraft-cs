# Windows client launcher

`WarcraftCSLauncher.exe` is a standalone launcher/installer for Windows 10/11 x64.
It uses the .NET Framework and Windows PowerShell included with supported Windows installs.
The standard EXE embeds audited project source without native DLLs and compiles locally after consent.
`WarcraftCSLauncher_DLL_Included.exe` is a separate download with ready x86 modules/notices and optional Player setup.
Clients do not need Git or Python beforehand. Source setup requires C++ tools/SDK, which can be prepared after agreement.

## Install and play

1. Browse to your Warcraft III **1.26a** installation (`Game.dll` 1.26.0.6401).
2. Browse to your Counter-Strike 1.6 `cstrike` folder or its Half-Life parent. CS is required
   for local hands, models, animation and audio; the launcher does not download either game.
3. Choose the client installation folder. Default: `%LOCALAPPDATA%/WarcraftCS`.
   It must be outside both original game folders. Allow enough space for another Warcraft copy.
4. Read the dependency summary/linked terms and tick the agreement box, initially unchecked.
5. In the standard EXE, press **Install** to compile locally with C++ tools/SDK.
   The DLL-included EXE offers **Install — Player** without Build Tools/SDK and **Install — Developer** for local compilation.
   Install uses the version embedded in that EXE; progress appears in the log.
6. Select **The Frozen Throne** or **Reign of Chaos** under **Game to launch**, then press **Play**.
   Select your own living unit in a single-player map/campaign and press F6.

### Folder examples

| Field | Example | Files to look for |
| --- | --- | --- |
| Warcraft III 1.26a folder | `E:\Warcraft III` | `war3.exe`, `Game.dll` (1.26.0.6401), `Mss32.dll`, `Storm.dll`, `War3.mpq`, `War3x.mpq`, `War3Patch.mpq`, `War3xlocal.mpq` |
| Counter-Strike 1.6 folder | `C:\SteamGames\steamapps\common\Half-Life\cstrike` | `models/v_ak47.mdl`, the other weapon models, `sound/weapons`, `sound/player` |
| Install Warcraft CS here | `D:\WarcraftCS` | A dedicated writable folder outside Warcraft, Half-Life and `cstrike`; setup creates `Game` and `sources` inside it. |

Steam can use another drive or library name. In Steam, open Counter-Strike's
**Manage → Browse local files**, then select its `cstrike` folder (or the containing `Half-Life` folder).
Selecting `C:\SteamGames` or `steamapps` alone is insufficient. Do not select a map,
save folder, individual EXE or the `models` subfolder.
`%LOCALAPPDATA%` denotes your Windows user account's local application-data directory;
the launcher fills in its full path automatically. Keep this default if unsure.

Warcraft's game root must retain `redist/miles` (including `Mssfast.m3d`, `Mp3dec.asi`,
`Reverb3.flt`). Setup copies these owned codecs/providers on first install and on updates,
repairing clients whose older installer omitted the folder. No audio codec is embedded or downloaded.

Warcraft opens in fullscreen. Choose **Single Player → Custom Game** or **Campaign**,
start a map, select your own living unit and press **F6**. Repeat after changing maps.
Use WASD/mouse to move/aim, left mouse to shoot, R to reload, 1–7 to select weapons,
and F6 to return to RTS. See the [complete controls](../README.md#controls).

After a Windows restart requested by Microsoft setup, open this EXE again, select the
same folders, accept the agreement and retry **Install**. Successful installation
enables **Play**; consent is not required just to play an already prepared installation.

Downloads/installation cannot start without consent: the UI, setup runner, payload extractor
and PowerShell entry point all enforce the agreement. Play never installs dependencies.
Existing game saves are not imported automatically; preserve your progress before updates.

## Release updates

Startup checks the latest stable release of `YazgulDev/warcraft-cs`; **Check for updates** retries it.
The separate **Update** button performs a fresh GitHub check and offers the latest project release.
Checks always read metadata automatically, regardless of installation consent. When an update is found,
a dialog shows the version and notes. **Update** confirms downloads/installation; **Not now** or closing
the window postpones it. The standard launcher always builds locally, including over a previous Player
installation. The DLL-included launcher preserves its variant and saved Player/Developer mode.
The dialog explains its dependencies and
acceptance of terms. Warcraft must be closed. There is no automatic-install checkbox. Previous folder selections are remembered; legacy automatic-install
preferences do not grant consent or start installation in this launcher.

The updater downloads `WarcraftCS-update.json`, `WarcraftCS-sources.zip` and the matching launcher variant
from that repository's release assets. It validates version/tag, SHA256, archive paths and the source
version, then installs all new project sources in a revision-specific folder. The standard manifest hash
describes `WarcraftCSLauncher.exe`; `DllIncludedLauncherSha256` / `DllIncludedRuntimeSha256` describe the
separate bundled executable and its runtime. Player validates the EXE's
embedded native runtime and installs its two mod modules. Developer rebuilds the modules locally.
Both convert owned game assets. Existing saves, maps and INI settings are kept; a remembered private sword remains selected.
The launcher refuses an update while its Warcraft runtime is running. Save and close the game, then retry.
It never terminates a match to install an update.

After successful setup, a detached helper waits for the old launcher to exit, atomically replaces its EXE,
retains `<launcher>.previous` and restarts it. If the original launch folder is read-only, the verified
cached launcher starts instead; details are in `updates/<revision>/launcher-replacement.log`.

If GitHub API checks return 403/429, the launcher retries through GitHub’s documented latest-asset
manifest URL without a token, retaining version/SHA256 verification. Release notes are linked when
the API cannot provide their text. GitHub/network failures leave offline Play and embedded setup available. Invalid/corrupt packages are not
installed. Setup failures are reported and must be retried; the launcher is replaced only after successful
setup. Package hashes check integrity over HTTPS; the project executable remains unsigned.

Old 0.5.0 Player updaters cannot select the newly named DLL-included EXE. Download it directly and select
the existing installation folder to migrate. Their missing-bundle guard prevents silently installing Build Tools.
Legacy 0.3/0.4 source-build requests retain local compilation when they lack a mode field.

Version comparisons prevent downgrades. Source revisions also detect repaired assets of the same version,
so release/0.6.1 can receive fixes without moving its published v0.6.1 tag. Use the attached sources ZIP for
the updated build; GitHub's automatic tag archives continue to represent the original tag snapshot.

The `dist` folder remains generated/untracked. Release assets include the EXE, checksum, exact embedded
sources ZIP, the separate DLL-included EXE/checksum, readable `REQUIREMENTS.md`, updater manifest and
`WarcraftCS-<version>-dist.zip`. Every public ZIP contains no prebuilt DLLs. The native runtime ZIP stays
in private build output. Upload the manifest last so clients never
start an update against an incomplete package set.

## Dependencies

- Python: reuse a compatible local/registered interpreter with pip/venv; otherwise download
  the pinned PSF Python 3.12.10 x64 installer into the client cache and install privately without PATH changes.
  SHA256 and the PSF Authenticode signature are checked before execution.
- Microsoft Visual Studio 2022 C++ Build Tools/Windows SDK (**Developer only**): reuse installed tools; otherwise obtain
  Microsoft's signed bootstrapper and request its C++ workload with recommended SDK components.
  This can consume several GB. Windows may request administrator approval. If a restart is required,
  restart Windows and click Install again. The launcher does not bypass UAC or force a reboot.
- NumPy: installed into the source payload's private Python venv through PyPI.
- MinHook: statically linked into Player’s bundled module with its BSD notice. Developer obtains immutable v1.3.4 revision with SHA256 verification, downloaded into the private cache.

Vendor references: [Python Windows installer](https://docs.python.org/3.12/using/windows.html),
[Microsoft installer options](https://learn.microsoft.com/en-us/visualstudio/install/use-command-line-parameters-to-install-visual-studio?view=vs-2022).
Licenses and credits are in [NOTICE](../NOTICE).

## Files and troubleshooting

The selected folder contains `Game` (your private Warcraft copy), `sources/<payload-hash>`
(project-owned source), optional `dependencies` and `downloads`, `install.log` and local request/state JSON.
Paths travel through JSON rather than shell interpolation. Source extraction rejects archive traversal.
The original Warcraft/CS folders are read only. Do not redistribute the resulting Game folder or caches.

If installation fails, keep `install.log`, resolve the displayed missing-file/version/network/vendor
message and retry. Close Warcraft in this private runtime before updating it.
Removing the client folder does not uninstall shared Microsoft build tools; those use Windows Apps settings.
Back up any saves from `Game/save` before removing your client copy.

## Build and verify the EXE

Developers need Git, Python, Visual Studio x86 C++ tools/SDK, owned Warcraft III 1.26a and the Windows .NET Framework C# compiler. Stage the reviewed source first:

```powershell
python tools/audit_sources.py --staged
.\tools\build-launcher.ps1 -WarcraftDirectory "E:\Warcraft III"
.\tools\test-launcher.ps1
$manifest=Get-Content dist/WarcraftCS-update.json -Raw | ConvertFrom-Json
.\tools\test-launcher-packages.ps1 -Directory dist
.\tools\test-prebuilt-runtime.ps1 -Package build/launcher/WarcraftCS-runtime.zip -SourceRevision $manifest.Revision -Launcher (Resolve-Path dist/WarcraftCSLauncher_DLL_Included.exe).Path
```

Output: both EXEs, their checksums, sources/requirements/update manifest and a source-only distribution ZIP
(all ignored by Git). Both EXEs embed the same audited staged source tree; only the DLL-included EXE
embeds a native-runtime resource. Git and every public ZIP exclude prebuilt mod modules.
Use `build-launcher.ps1 -SourceOnly` when building just the standard EXE without an owned Warcraft build host.
Only launcher source/scripts/docs are committed; the own-code EXE can be shared separately.
The EXE is not Authenticode-signed by Yazgul; dependency installers are verified using their vendor signatures.
