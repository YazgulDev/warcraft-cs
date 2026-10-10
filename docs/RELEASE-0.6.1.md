# Warcraft CS 0.6.1

All changes since the immutable **v0.6.0** tag: configurable CS sound volume, reusable
game-settings rules, complete-release-note requirements, validation and documentation updates.

## CS sound volume

After installing/updating to 0.6.1, edit your private `Game/WarcraftCS/WarcraftCS.ini`
(source installs: the prepared runtime's `WarcraftCS/WarcraftCS.ini`):

```ini
[Audio]
CSVolumePercent=50
```

- **0–100 percent**, default **100**. 0 mutes CS; 100 retains the previous volume.
  Fractional percentages work. This scales the existing mix, preserving the relative levels
  of shots, reload/animation sounds, footsteps, melee, C4 planting/beeps/explosions and other CS voices.
- **F8 in FPS** applies changes immediately to ongoing and future sounds. Startup uses the
  same validated settings path. No WAV editing or restart is required for volume changes.
- Warcraft music, ambience and native effects retain their own audio controls; the change
  adjusts only the CS XAudio2 master voice, without restarting source voices or animation timers.
- Missing/empty/malformed/non-finite values use 100. Finite out-of-range values clamp to 0–100.
  The mixer also guards invalid direct inputs and remains safe if audio initialization is unavailable.
  Diagnostics report the applied native master gain.
- Setup/update adds the key to older configs and preserves an existing custom value and unrelated
  preferences. Repeated migration is idempotent and adds no duplicate Audio sections or keys.

## Reusable skills and project rules

- Add **configurable-game-settings** to the repository and install it in the owner's personal
  Codex skills with automatic discovery enabled. New/changed player-facing game tuning, including
  damage, counts, toggles, sky and sound, must have editable config keys, documented defaults/ranges,
  boundary validation, supported live reload and preservation during updates. Internal engine/format
  constants remain implementation details. Warcraft CS config/loader/migration locations are documented.
- Update repository and installed **feature-release-workflow**: Release Notes must include **every
  change in the release**, not only the last task. Inspect commits and the full diff from the previous
  published tag, covering additions, settings/defaults, fixes, source/packaging/skills/docs changes,
  upgrade instructions and honest validation/limitations. Historical notes/tags remain unchanged.
- Accept a release version explicitly supplied by the user; otherwise ask before creating it.
- Adopt these requirements in AGENTS and contributor guidance. Keep the established feature →
  current release → master → new release workflow and preserve previous branches/tags.

## Tests, documentation and packages

- Extend real INI tests with mute, fractional/range/non-finite inputs and repeated edits/reloads.
- Extend migration checks for default insertion and preservation of custom audio and other settings,
  duplicate prevention and byte-identical repeated setup in PowerShell 7 and Windows PowerShell 5.1.
- Add `tools/test-cs-audio.ps1` and an actual production-XAudio2 test with a project-authored silent
  PCM clip. Native master readback verifies 50, 0, 12.5 and 100 percent while voices are queued,
  plus mute/restore, bounds and the uninitialized path. It requires a working Windows audio device.
- Update README/main-page features and config instructions, installation example, troubleshooting,
  audio-module responsibility, validation documentation, changelog and current version/branch links.
- Rebuild both EXEs, identical audited source snapshots, checksums, source-only distribution ZIP,
  requirements and update manifest for 0.6.1. No new game sound files or proprietary assets are included.

| Download | Installation |
| --- | --- |
| `WarcraftCSLauncher.exe` | Source-only; Install compiles locally with Microsoft C++ Build Tools / Windows SDK. |
| `WarcraftCSLauncher_DLL_Included.exe` | Ready project-built modules; Install — Player avoids Build Tools / SDK, Developer builds locally. |
| `WarcraftCS-0.6.1-dist.zip` | Source-only launcher/checksum, source archive, requirements and updater manifest. |
| `WarcraftCS-sources.zip` | Exact audited source snapshot embedded in both EXEs, including the new skill and audio tests. |

## Updating and limitations

Save progress, close Warcraft and use **Update** with the existing installation folder.
Saves, INI values, launcher variant, supported Player/Developer mode and private sword preferences
are preserved; dependency downloads still require agreement. After updating, edit the Audio key
and press F8 in FPS. Older clients add this key during setup without replacing an existing preference.

Requirements remain Windows x64, owned Warcraft III **1.26a x86 / Game.dll 1.26.0.6401**,
owned CS 1.6 and OpenGL. Offline single player only; Reforged/multiplayer remain unsupported.
All gameplay regressions, native audio/settings checks, native builds, launcher/update/package checks
and source/publication audits are part of release validation. Actual audio testing uses synthetic
silence outside Warcraft; no new full campaign, live Warcraft F8-input session or clean-machine
dependency installation is claimed.

Unsigned module detections remain possible: DLL loading/native hooks can resemble suspicious
behavior to heuristic antivirus scanners. This is a plausible explanation for the recorded 0.5.0
detection, not a vendor-confirmed cause or a guarantee of a false positive. See
[antivirus troubleshooting](../TROUBLESHOOTING.md#antivirus); this release does not claim a new clean antivirus verdict.
