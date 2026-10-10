# Troubleshooting

Start with the exact error in the launcher and the relevant log below. Check that you are using
the [latest published launcher](https://github.com/YazgulDev/warcraft-cs/releases/latest) and read
[Installation](INSTALL.md) and [Requirements](REQUIREMENTS.md). Save your progress before closing
Warcraft or retrying an installation/update. Help is provided when time allows.

## Logs and first checks

On the diagnostic test branch, click **Read logs** in either launcher to inspect and export the selected
installation's logs, including retained game sessions. The viewer works while the game/setup is running
and before installation. See [Diagnostics](docs/DIAGNOSTICS.md) for retention and F8 logging settings.

For a launcher installation, paths are relative to the **client installation folder** selected in
the launcher (default `%LOCALAPPDATA%\WarcraftCS`):

| What failed | What to collect |
| --- | --- |
| Install or Update | The exact message in the launcher and `install.log`, if created. Early validation/download failures may appear only in the launcher. |
| Game, F6, weapons or audio | `Game/WarcraftCS/WarcraftCS.log` after reproducing the problem. Copy it before launching again: each game session replaces this log. |
| Launcher replacement/restart | `launcher-replacement.log` in the update's `updates/<revision>/` folder, if present. |
| Source setup | The PowerShell error/output and `WarcraftCS/WarcraftCS.log` inside the prepared runtime, if the game was started. |

Source installations keep the runtime in `.local/warcraft-cs` by default, or the `RuntimeDirectory`
you supplied. They do not produce the launcher's `install.log`. There is currently no `-Doctor`
command or `setup.log` in Warcraft CS.

Verify these requirements first:

- Windows x64, owned Warcraft III **1.26a x86**, `Game.dll` file version **1.26.0.6401**.
- Your own CS 1.6 `cstrike` folder with its loose `models` and `sound` files.
- A separate client/runtime folder outside the original game folders.
- Offline single player and a living unit owned by your player for FPS mode.

## Installation and updates

| Symptom | What to do |
| --- | --- |
| Unsupported Game.dll | Select the installed 1.26a x86 game. Other patches and Reforged are unsupported. Check `Game.dll` Properties → Details. |
| Missing owned Warcraft file or Miles provider | Use a complete working installation of your own Warcraft. Retry setup to copy its audio providers into the private runtime. Do not download individual DLLs from third-party sites. |
| Missing `v_*.mdl`, model or CS sound | Select the actual `cstrike` folder containing the required loose model/sound files, then retry setup. Neither launcher downloads game content. |
| Compiler, MSVC or Windows SDK missing | Standard/Developer mode needs Microsoft C++ Build Tools, x86 MSVC and a Windows SDK. Check the C++ workload in Visual Studio Installer. Use the DLL-included launcher in Player mode to install without Build Tools/SDK. |
| Python or NumPy missing; Python opens the Store | Use Python 3.10+ with venv support or let the launcher prepare its local dependency after consent. Source setup accepts `-PythonExecutable`. Check Windows App execution aliases if `python` opens the Store. |
| Running scripts is disabled | For a source installation, use `setup.cmd` / `play.cmd`, or the PowerShell invocation documented in [Installation](INSTALL.md). |
| MinHook or another download fails | Check the connection and the exact URL/error in the output. Retry when the connection is available. Keep checksum verification enabled. |
| SHA256 mismatch / corrupt package | Download the official asset again and compare its companion checksum. Report the asset name, release and exact error if it persists. Do not bypass the integrity check. |
| Runtime already exists without marker | Choose a new, separate runtime directory. Setup will not overwrite an unknown installation; do not create the marker manually. |
| Close Warcraft / cannot verify the running folder | Save the match and close Warcraft normally, then retry Update. Setup protects files used by a running game. |
| Update is cancelled or installation incomplete | Read the first error, correct it and retry with the same client folder after closing Warcraft. Attach the available log if it fails again. |
| Launcher did not restart after Update | Read `launcher-replacement.log`, if present, and report the exact error plus the release/launcher variant. |

## In game

| Symptom | What to do |
| --- | --- |
| F6 does nothing | Load an offline map/campaign, select a living owned unit and check the runtime log. Confirm the game was launched from the prepared private copy. |
| Weapon does no damage | Check health, stun/disarm, ammunition and `[Damage] Mode`. A unit without a normal attack deals zero weapon damage in `hero` mode. |
| E cannot pick up an item | A free native inventory slot is required. Stun, hiding or transport blocks pickups. A creep's Warcraft Backpack cannot activate runes/tomes. |
| H/O changes workers' orders | Recruitment includes nearby owned movable workers. J releases the squad; reduce `RecruitRadius` if needed. |
| Buy menu/purchase is rejected | Check gold, carried equipment and `[Buy] Access`/`Radius`. Friendly/shop zones are enforced for B and purchases; full ammo and duplicate guns are rejected. |
| Custom map camera/control behaves strangely | Return to RTS with F6. Map scripts can conflict with the mod. Report the map name/version and reproduction steps; do not upload the map itself. |
| Previous saves are missing | The prepared game is a separate copy. Back up saves before transferring your own progress; see [Installation](INSTALL.md). Do not reinstall over the original game. |

## Graphics and sound

| Symptom | What to do |
| --- | --- |
| Black window, rendering or driver problem | The mod uses OpenGL. Try a windowed launch (`play.cmd -Windowed` for source installs) and check your GPU driver. Include GPU/driver version and whether Alt-Tab triggered it. |
| Crosshair missing in FPS | Attach an FPS screenshot showing whether the weapon and HP/ammunition HUD remain visible, plus the runtime log, mod version, launcher/Player or Developer mode, GPU and driver version. The crosshair is drawn by code and has no separate image to reinstall. A successfully acquired overlay context does not prove the crosshair's pixels were visible. Test builds with renderer diagnostics also log `Overlay GL vendor=... renderer=... version=... lineStipple=...`. |
| CS audio is too loud or muted | Set `[Audio] CSVolumePercent` in the private `WarcraftCS.ini` to 0–100; 100 is the original mix and 0 mutes CS. Press F8 in FPS to apply it. This key requires 0.6.1 or newer. |
| Warcraft music/native effects have the wrong volume | Adjust Warcraft's own sound settings. The CS volume parameter affects only the CS mixer. |
| CS sky is missing | Check `[Sky] Enabled` and the selected texture names, then rerun setup with your own CS textures if caches are missing. `[Sky] WarcraftEnabled` controls the native Warcraft sky. F8 reloads settings. |

<a id="antivirus"></a>

## Antivirus

Defender reported `Trojan:Win32/Wacatac.B!ml` for the embedded `WarcraftCS.mix` in 0.5.0.
The mod loads DLLs and hooks native game functions; unsigned builds with this behavior can trigger
heuristic detection. This is a plausible explanation, not a vendor-confirmed cause or proof that a
detection is harmless. A local build can also be blocked.

Compare the official release asset's SHA256 with its checksum file. If blocked, submit the detected
file to your antivirus vendor for review and include the exact detection/version in your report.
Do not disable protection globally. Microsoft discusses possible heuristic false positives in
[its explanation of false detections](https://www.microsoft.com/en-us/security/blog/2018/08/16/partnering-with-the-industry-to-minimize-false-positives/).

## Ask for help or report a test

Open a question in [Discussions — Q&A](https://github.com/YazgulDev/warcraft-cs/discussions/categories/q-a).
Include:

- Release version and launcher variant; Player or Developer installation mode.
- Windows version, GPU and driver version.
- Warcraft `Game.dll` version; Reign of Chaos or The Frozen Throne; map/campaign name and version.
- Steps to reproduce, expected result, actual result and the exact error.
- Relevant log/output, if available, and what you already tried. If there is no log, explain where it stopped.

Positive Windows test reports are welcome in **General**; propose features in **Ideas**.
Questions with enough detail are easier to investigate, but replies and fixes are not guaranteed.

Before uploading logs/screenshots, remove personal paths, usernames and any private data.
Do not attach game DLLs, maps, saves, models, audio, converted caches or the complete private runtime.
