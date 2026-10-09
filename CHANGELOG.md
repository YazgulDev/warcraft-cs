# Changelog

## Unreleased — test branches

- Disable AWP instant enemy kills by default (`[Damage] AWPOneShot=false`); ordinary damage remains 115 before native armor.
- Require explicit `true`/`1` for AWP finishing; missing/empty/invalid values use false. F8 reload and custom-value preservation remain supported.
- Draw hip-fire and scope reticles with filled geometry while preserving their shape and inherited OpenGL state.
- Add configurable bounded diagnostics, dedicated reticle draw/skip/error records and Read logs in both launchers.
- Show reviewed per-line update changes in both launchers, including the GitHub API rate-limit path.

- Add accelerating A/D-and-mouse air strafes, held-Space autojump and live movement tuning.
- Use air acceleration immediately on takeoff and retain momentum through landings. Straight jumps preserve the run-up; default `JumpBoostPercent=0` disables the separate automatic takeoff bonus.
- Bound accumulated speed with `MaxBunnySpeed=1000` GoldSrc units/s; disabling BunnyHop clips excess momentum on each takeoff. Preserve custom boosts and caps during upgrades.
- Allow height-aware jumps over low objects and gradual gravity-driven falls from cliffs.
- Preserve tall/unknown blockers, swept collision, bridge surfaces, map bounds and spell movement restrictions.
- Add editable `[Movement]` settings, migration preservation and independent/native movement oracles.
- No release merge, version bump or publication; see [movement notes](docs/MOVEMENT.md).

## 0.6.1 — 2026-10-09

- Add `[Audio] CSVolumePercent` (0–100, default 100) for CS shots, reloads, footsteps, melee and C4.
- Apply the CS-only master gain at startup and on F8, including queued/playing sounds; preserve per-event levels and Warcraft's own audio.
- Validate missing/empty/malformed/non-finite values and clamp finite percentages; expose native gain readback for diagnostics.
- Preserve custom audio values while migrating old configs; cover default insertion, repeat setup and duplicate-section/key prevention.
- Add and install `configurable-game-settings`: new game tuning must ship config keys, validated defaults, reload semantics and update preservation.
- Update installed/repository `feature-release-workflow` to require all release changes in Release Notes, based on the previous immutable published tag; reuse a version explicitly supplied by the user.
- Adopt both rules in AGENTS/contributor guidance; update README, installation/troubleshooting, module ownership, validation notes and current version/branch/download references.
- Add real INI/audio migration checks and a production-XAudio2 test with silent synthetic PCM, live gain changes, mute/restore and defensive bounds.
- Rebuild both launcher variants, audited sources, checksums, updater manifest and source-only distribution for 0.6.1; preserve previous branches/tags and update compatibility.

Complete changes since v0.6.0 and validation scope: [RELEASE-0.6.1](docs/RELEASE-0.6.1.md).

## 0.6.0 — 2026-10-09

- Merge all completed feature work since 0.5.1, retaining earlier release/feature branches and immutable tags.
- Add remembered RoC/TFT selection in both launchers and `play-roc.bat` with forwarded map/window options.
- Set fullscreen DPI awareness and recover weapon/menu/sky graphics after focus and OpenGL context changes.
- Add the CS-style gold buy menu, mouse/number navigation, hand-free private previews, live details and `.` quick ammo.
- Configure weapon/ammunition prices, default reduced weapon prices, starting kits and native shop/building buy zones.
- Grant starting equipment once per map; F7 freely refills carried equipment and F9 freely grants all equipment/ammo.
- Allow multiple planted C4 charges, independent simulation-time fuses/damage snapshots and nearest-fuse HUD.
- Add optional private CS sky conversion, tileset mapping, correctly aligned cube faces and isolated GL upload state.
- Enable stock Warcraft summer/winter FPS skies by default while preserving existing map skies and RTS/cinematic state.
- Preserve native hero portraits, expand the FPS viewport, fix near-ground clipping and traverse native walkable bridges.
- Bring AWP/USP viewmodels closer; filter world labels by camera and configurable distance without changing RTS text.
- Organize native/launcher sources by responsibility; retain published entry points and share the buy catalog.
- Merge missing INI defaults without overwriting player settings; document all features, installation and limitations.
- Update the main README, requirements links and branch references; briefly explain recorded antivirus detections
  and their likely unsigned-loader/hook heuristic cause, without claiming vendor confirmation.

Full release notes, downloads and validation scope: [RELEASE-0.6.0](docs/RELEASE-0.6.0.md).

## 0.5.1 — 2026-10-07

- Publish a source-only `WarcraftCSLauncher.exe` in distribution ZIPs and a separate `WarcraftCSLauncher_DLL_Included.exe` for ready mod modules.
- Separate Install (embedded version) and Update (latest stable GitHub project release); source-only launchers expose one Install button.
- Preserve launcher variants during verified updates, with independent executable/native-payload hashes and explicit dependency consent.
- Add `[Damage] FriendlyFirePercent` (0–100, default 50) for firearms, melee and C4 against owned/allied units and buildings.
- Apply F8 reloads to subsequent contacts and snapshot C4's percentage at planting; zero allied damage emits no native damage event.

Validation: all nine C++ suites and launcher/update regressions pass. Real package checks verify
separate executable hashes, identical source snapshots, native-free public ZIPs and native payload integrity.

## 0.5.0 — 2026-10-06

- Show a release-version/notes confirmation dialog; declining leaves the current installation untouched.
- Add Player and Developer installation buttons, with explicit dependency information.
- Embed verified x86 project modules and their notices for Player setup without Build Tools/SDK.
- Keep original Warcraft libraries local; bind the Player payload to its source snapshot and supported Miles ABI.
- Fall back to GitHub latest-asset manifest downloads on API rate limits.

- Remove the automatic-install checkbox; always check releases at launcher startup and offer discovered updates through an explicit confirmation dialog.
- Restore saved folders without inheriting legacy automatic-install permission.

Validation: launcher/payload regressions and isolated Player/Developer installs pass.
Older updater-enabled launchers accept the new bundle; saves and settings are preserved.

## 0.4.0 — 2026-10-06

- Add a complete requirements file for users, source builds and release updater assets.
- Pick authored tree trunk triangles instead of canopy boxes; preserve ordinary gate/model-bound behavior.
- Add mouse-wheel weapon cycling with wraparound, high-resolution fractions and focus/status reset.
- Fix Warcraft sound initialization by copying the owner's `redist/miles` providers/codecs during setup.
- Repair missing Miles files in existing marked private runtimes; detect incomplete source installs before dependency downloads.
- Include remembered private sword selection and the Miles audio runtime repair.
- Add launcher startup release checks, saved opt-in automatic updates, SHA256 verification and safe EXE replacement/restart.
- Update complete source/build assets while preserving saves, settings and remembered private sword paths; refuse updates during a running match.
- Publish an exact source payload, updater manifest and distribution ZIP as release assets; retain immutable tags and source-only Git history.

Validation: all nine C++ suites and a disposable native tree/gate/wheel session pass.
Requirements are included in both the source snapshot and distribution ZIP.

## 0.3.0 — 2026-10-06

- Windows client EXE with game-folder selection, explicit download/install consent, live setup log and Play.
- Embedded audited source payload; clients do not need Git or a separate source archive.
- Install missing Python and signed Microsoft C++ tools/Windows SDK; reuse existing dependencies.
- Preserve owned game installations through the private-runtime setup pipeline.
- English installation guide with exact folder examples, first launch, updates, saves and removal.
- Support Unicode/space/ampersand/apostrophe installation paths in the background setup and native build.
- Merge release/0.2.0 into master before starting release/0.3.0; retain previous branches and tags.

Validation: launcher consent/path/source-payload tests and all seven C++ logic suites pass;
complete isolated client setup passed using existing Python/MSVC/SDK. Fresh vendor installation
on a clean Windows machine and complete campaign compatibility remain unverified.

## 0.2.0 — 2026-10-06

- Use the project's original straight silver sword with a gold guard and leather grip.
- Generate sword geometry from project-owned source; import hands/animations only from the owner's CS knife.
- Remove external sword-model selection/import from this release; no downloaded sword asset is distributed.
- Translate the complete README to English and update repository/installation links.
- Start release/0.2.0 after merging the 0.1.0 baseline into master; retain all old branches and tags.

## 0.1.0 — 2026-10-06

Initial source-only release of Warcraft CS by Yazgul.

- Global offline FPS mode for Warcraft III 1.26a x86: native units, maps and campaign rules.
- Seven weapon slots, melee secondary attacks, C4, AWP scope, model/sound import from owned CS files.
- Classic CS punch calculations, movement, relative mouse input, fullscreen HUD and Alt-Tab resource recovery.
- Native status restrictions, model-bound targeting, destructable gates, friendly damage coefficient.
- Real item/rune pickup, configurable ammunition/damage, owned-unit combat/passive squads and J release.
- Portable Windows setup/build/launch; separate private runtime and pinned external MinHook build.
- Seven independent C++ regression suites, source-only staged/release audit, credits/license notices.
- Feature/release/master skill and retained release/0.1.0 plus publication feature branch.

Known limits: offline only; supported host version only; approximate model-bound hitboxes;
full CS spread/client behavior and all custom-map/campaign compatibility are not implemented/verified.
Game content, custom sword assets and compiled binaries are intentionally excluded.
