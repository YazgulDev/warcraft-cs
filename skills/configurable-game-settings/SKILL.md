---
name: configurable-game-settings
description: Make new or changed player-facing game parameters configurable when editing gameplay or mods, including weapon damage, feature toggles, sky and audio settings.
---

# Configurable game settings

Every new or changed player-facing tuning parameter must be editable in the game's or mod's
existing configuration as part of the same change. This includes damage, ammunition/counts,
prices, distances, durations, feature toggles, sky options and CS sound volume. Do not leave
these values available only as source constants or require editing code to change them.

- Reuse the project's config format, section names and validated settings loader. If none exists,
  add the smallest suitable config mechanism; avoid creating a competing settings file.
- Ship each key with a documented default, units, valid range/options and when changes take effect.
  Preserve previous behavior by default unless the user requested a different default.
- Validate at the config boundary: handle missing/empty/malformed/non-finite values, clamp bounded
  numbers and reject unsafe options. Runtime behavior must consume the validated settings.
- Apply live reload through the existing reload command when safe. Document parameters that apply
  only on the next map or restart; do not claim live reload where it is not implemented.
- Fresh installs must include the key. Updates must add missing keys without overwriting existing
  custom values, duplicating sections or resetting unrelated settings.
- Verify the real config-to-runtime path, fallback/range handling, reload behavior and preservation
  of a custom value during an update. Put each behavior change in the release's complete notes.

Engine ABI offsets, file-format constants and internal algorithm invariants are not player tuning
parameters. Keep them internal rather than exposing unsafe technical details as game settings.

For Warcraft CS, defaults live in `config/WarcraftCS.ini`, parsing in `src/config/GameplaySettings`,
and migration in `setup/gameplay-config.ps1`. F8 reloads supported settings in FPS. Runtime config
is `Game/WarcraftCS/WarcraftCS.ini` for EXE installs or the prepared private runtime's
`WarcraftCS/WarcraftCS.ini` for source installs. Update README/installation examples together.
