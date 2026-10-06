# Windows client launcher

`WarcraftCSLauncher.exe` is a standalone launcher/installer for Windows 10/11 x64.
It uses the .NET Framework and Windows PowerShell included with supported Windows installs.
It embeds the project's source and notices; clients do not need Git, a compiler or Python beforehand.

## Install and play

1. Browse to your Warcraft III **1.26a** installation (`Game.dll` 1.26.0.6401).
2. Browse to your Counter-Strike 1.6 `cstrike` folder or its Half-Life parent. CS is required
   for local hands, models, animation and audio; the launcher does not download either game.
3. Choose the client installation folder. Default: `%LOCALAPPDATA%/WarcraftCS`.
   It must be outside both original game folders. Allow enough space for another Warcraft copy.
4. Read the dependency summary/linked terms and tick the agreement box, initially unchecked.
5. Press **Install / Update**. Progress and failures appear in the log.
6. Press **Play**. Select your own living unit in a single-player map/campaign and press F6.

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

Warcraft opens in fullscreen. Choose **Single Player → Custom Game** or **Campaign**,
start a map, select your own living unit and press **F6**. Repeat after changing maps.
Use WASD/mouse to move/aim, left mouse to shoot, R to reload, 1–7 to select weapons,
and F6 to return to RTS. See the [complete controls](../README.md#controls).

After a Windows restart requested by Microsoft setup, open this EXE again, select the
same folders, accept the agreement and retry **Install / Update**. Successful installation
enables **Play**; consent is not required just to play an already prepared installation.

Downloads/installation cannot start without consent: the UI, setup runner, payload extractor
and PowerShell entry point all enforce the agreement. Play never installs dependencies.
Existing game saves are not imported automatically; preserve your progress before updates.

## Dependencies

- Python: reuse a compatible local/registered interpreter with pip/venv; otherwise download
  the pinned PSF Python 3.12.10 x64 installer into the client cache and install privately without PATH changes.
  SHA256 and the PSF Authenticode signature are checked before execution.
- Microsoft Visual Studio 2022 C++ Build Tools/Windows SDK: reuse installed tools; otherwise obtain
  Microsoft's signed bootstrapper and request its C++ workload with recommended SDK components.
  This can consume several GB. Windows may request administrator approval. If a restart is required,
  restart Windows and click Install again. The launcher does not bypass UAC or force a reboot.
- NumPy: installed into the source payload's private Python venv through PyPI.
- MinHook: immutable v1.3.4 revision with SHA256 verification, downloaded into the private cache.

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

Developers need Git, Python and the Windows .NET Framework C# compiler. Stage the reviewed source first:

```powershell
python tools/audit_sources.py --staged
.\tools\build-launcher.ps1
.\tools\test-launcher.ps1
```

Output: `dist/WarcraftCSLauncher.exe` (ignored by Git). The build embeds exactly the audited staged tree.
Only launcher source/scripts/docs are committed; the own-code EXE can be shared separately.
The EXE is not Authenticode-signed by Yazgul; dependency installers are verified using their vendor signatures.
