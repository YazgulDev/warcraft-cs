# Warcraft CS by Yazgul

For feature work, commits, release merges and publication, read and apply
[feature-release-workflow](skills/feature-release-workflow/SKILL.md).

Current release: `release/0.4.0`; VERSION is the project version.
Feature branches use `feature/add-<feature-or-features>` and merge into the latest
release only when requested. A new release requires asking for its version, merging
the previous release into master, then creating the new release branch. Keep old branches.

This is a source-only repository. Exclude `.local`, `build`, dependencies, game assets,
retail/extracted/converted content, binaries, saves, recordings, private mod logs and secrets.
Run `python tools/audit_sources.py --staged` before committing.
Use explicit staging paths. Runtime and domain code are in `src`, tests in `tests`,
private setup/build scripts in `setup` and `tools`. Add concise intent comments for behavior changes.

Supported host: offline Warcraft III 1.26a x86 (Game.dll 1.26.0.6401).
Never test fixtures in the user's campaign. Preserve the latest progress before restarting
an active game. Native opt-in verification scenes are excluded from ordinary builds.
