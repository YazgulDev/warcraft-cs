# Contribution and release workflow

Read [the workflow skill](skills/feature-release-workflow/SKILL.md) and [AGENTS.md](AGENTS.md).
Current release branch: `release/0.4.0`. Code layout: `src`, `tests`, `tools`, `setup`, `config`, `launcher`.

Start one branch per feature prompt from the current release:

```text
feature/add-<feature-or-features>
```

Merge a feature into the latest release when requested, preserving the feature with a merge commit.
When starting a new release, first obtain its version from the owner, merge the current release into
master, then branch `release/<new-version>` from master. Never delete old release/feature branches
or move an existing published tag. The active release is the initial default branch; master is the
baseline advanced when the owner starts the next release.

Run `tools/test-all.ps1` for relevant math/domain changes, build against your own private game copy,
and test native changes only in a disposable offline map. Do not run fixtures in a campaign save.
Keep behavior comments concise. No third-party source vendoring or game data in Git.

Stage explicit source/document paths. Before committing:

```powershell
python tools/audit_sources.py --staged
git diff --cached --check
```

Check the entire release tree and archive before a push/release. Keep NOTICE current when a dependency
or adaptation changes. Contributor code must be yours to submit under MIT OR Apache-2.0 and retain
upstream notices where applicable. Do not commit credentials, private logs/paths, game files, saves,
extracted/converted content, binaries or downloaded dependencies.
