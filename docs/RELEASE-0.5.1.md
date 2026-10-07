# Warcraft CS 0.5.1

This release separates source-only and DLL-included downloads, gives Install and Update
their own buttons, and adds configurable allied damage. Previously completed features
and the latest README changes have been merged into the release.

## Choose your launcher

| Download | What it installs |
| --- | --- |
| `WarcraftCSLauncher.exe` | No prebuilt mod DLLs. One **Install** button builds the mod locally. Missing Microsoft C++ Build Tools / Windows SDK can require several GB. |
| `WarcraftCSLauncher_DLL_Included.exe` | Ready project-built DLLs. **Install — Player** avoids Build Tools / SDK; **Install — Developer** compiles locally. |
| `WarcraftCS-0.5.1-dist.zip` | The source-only launcher, its checksum, the source archive, update manifest and requirements. |
| `WarcraftCS-sources.zip` | The exact audited source snapshot embedded in both launchers. |

Every public ZIP excludes prebuilt mod DLLs. The native-runtime ZIP is a private build
intermediate and is no longer a release asset. Only the separately named DLL-included
EXE distributes the ready modules. Neither launcher includes Warcraft/CS game files,
models, sounds, maps or saves. Both may prepare Python/NumPy with your agreement.

## What's new

- **Install** uses the version embedded in the running EXE. The new, separate **Update**
  button checks the latest stable Warcraft CS release on GitHub and offers its notes
  before installation. Startup checks remain available; **Not now** postpones updates.
- Source-only launchers keep source-only updates. DLL-included launchers keep their
  variant and remembered Player/Developer mode. Each variant has independent SHA256
  verification; dependency downloads require agreement.
- `[Damage] FriendlyFirePercent=50` controls firearms, knife/sword attacks and C4 damage
  to owned/allied units and buildings. Accepted range: **0–100**, including fractions.
  Native armor still applies afterward. **F8** reloads the config; planted C4 keeps its
  planting-time percentage. Zero allied damage avoids native damage events.
- English setup documentation now explains both downloads, folder selection, separate
  Install/Update actions, dependencies and migration from the previous Player launcher.

## Install or update

1. Select your owned **Warcraft III 1.26a x86** root (`Game.dll` version `1.26.0.6401`),
   containing `war3.exe`, `Game.dll`, `Mss32.dll` and Warcraft MPQ archives.
2. Select **Counter-Strike 1.6**'s `cstrike` folder (with `models/v_knife.mdl` and `sound`),
   or its `Half-Life` parent. The Steam library root is not the game folder.
3. Choose a dedicated writable installation folder outside both original games. To
   update an existing client, select the same installation root that contains `Game`,
   `sources` and `client-installed.json`.
4. Read the dependency details, agree to downloads, then choose **Install** or **Update**.
   Close Warcraft before updating; saves, INI settings and private sword selection are preserved.
5. Press **Play**, select your own living unit and press **F6** to enter FPS mode.

**Upgrading the old 0.5.0 Player launcher:** it cannot choose the new DLL-included filename.
Download `WarcraftCSLauncher_DLL_Included.exe` directly, select your existing installation
root and use **Install — Player**. Its old updater refuses a source-build release instead
of silently downloading Build Tools. Choosing the standard source-only EXE intentionally
switches installation/updates to local compilation.

Supported host remains offline Warcraft III 1.26a x86; Reforged and multiplayer are not
supported. Both EXEs remain unsigned. The bundled module triggered antivirus detection
in 0.5.0; separating downloads does not guarantee antivirus acceptance of either EXE or
the locally built DLLs.

Validation: all nine independent C++ suites and launcher/update regressions pass.
Real package tests verify the two assemblies, identical source snapshots, independent
hashes, native-free public ZIPs and runtime source/version binding. No campaign or live
match was opened for this launcher release; fresh dependency installation on a clean
Windows machine and a full campaign remain unverified.

See [INSTALL.md](../INSTALL.md), [REQUIREMENTS.md](../REQUIREMENTS.md) and
[NOTICE](../NOTICE) for full instructions, dependencies, credits and licenses.
