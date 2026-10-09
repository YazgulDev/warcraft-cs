---
name: feature-release-workflow
description: Apply Yazgul's feature/release/master branch workflow when adding features, merging into a release, preparing a new release, or committing this project's source-only publication. Does not apply to unrelated repositories.
---

# Feature and release workflow

Use for repositories whose AGENTS.md adopts this skill. Preserve all existing branches.

## Adding features

- Before editing for one or several new features requested together, identify the current release and create one branch `feature/add-<feature-or-features>` from it. Use a concise lowercase hyphenated description, for example `feature/add-recoil-and-squad-release`.
- Reuse the branch if continuing the same feature task. If that name already belongs to other work, choose a unique descriptive suffix without resetting its history.
- Keep unrelated local changes intact. Do not mix them into the feature commit.
- Keep feature work on its branch, run relevant checks, and commit only reviewed project-owned files. Merge into the current release when the user requests the merge; never merge features directly into master.

## Identifying the current release

- Read VERSION and available local/remote `release/<semver>` branches. Fetch origin when available before an externally requested merge.
- Compare versions numerically (including SemVer prerelease ordering), not lexically or by commit date. The latest versioned release branch is the target of an unqualified "merge into release" request.
- Use repository metadata to detect inconsistency; resolve a mismatch instead of silently merging into an older release or assuming a hardcoded version.
- Merge the feature with `--no-ff` so its history remains visible. Resolve and verify conflicts before completing the merge. Never delete the feature or old release branch.

## Starting a new release

1. Use a new version explicitly supplied by the user. Otherwise ask for the version, even when a plausible next version exists; suggest one if useful, but do not treat silence as an answer.
2. Verify the version is valid and newer than the current release and its branch does not already represent another release.
3. Complete the current release checks and merge the current release into `master` with `--no-ff`.
4. Create `release/<new-version>` from the updated master, then update VERSION and the changelog for the new release.
5. Retain every old release and feature branch. Do not move published tags, rewrite shared history, or force-push. A release tag identifies an immutable published snapshot, while its release branch may continue to receive features.

Initial source-only publication may bootstrap an empty master commit, create `release/0.1.0`, and merge the initial publication feature into it. Keep the active release as the repository's default branch until master contains a released baseline.

## Complete Release Notes

- Every release must have Release Notes covering **all changes included in that release**, not just the last task or commit.
- Compare the immutable previous published tag with the new release tree and inspect the included commits/merges. A release branch may have advanced since publication; do not use its moving tip as the baseline.
- Include every added feature/parameter, changed default or behavior, fix, relevant source/packaging/skill/documentation change, upgrade instructions and known limitations. Explain config keys, units/ranges and reload timing when parameters are added.
- Reconcile the notes against the full diff and changelog before publication. Keep version, README/download information, validation claims and the uploaded GitHub release body consistent; do not claim checks that were not performed.
- Publish complete Release Notes directly in the GitHub release body and in the project's established release-document location. A link to another document does not replace the full GitHub notes. Keep historical full notes unchanged.
- Provide a separate short plain-text change list for the launcher in a hidden HTML comment appended to the full GitHub body: `<!-- launcher-summary`, then one `- Added ...` bullet per line, then `-->`. GitHub hides this metadata, while the launcher displays only its bullets. Include only changes actually shipped; examples are not permission to invent features.
- Keep installation steps, validation details and developer explanations in the full visible GitHub notes. Use real line breaks in the launcher summary and verify both the complete GitHub page and the compact launcher dialog before publication. Never shorten the public release body to make the updater concise.

## Source-only commits and publication

- Inspect `git status`, the staged file list and the staged diff. Stage explicit paths instead of indiscriminate `git add .`.
- Exclude secrets/credentials, local configs, logs, saves, extracted or converted game content, proprietary game/SDK files, decompiler dumps, downloaded dependencies, binaries, recordings and build output. Include only owned source, tests, necessary configuration/tooling and requested docs/license notices.
- Identify adaptations/references accurately in NOTICE. A user's code license does not relicense upstream code or game assets. Obtain third-party build dependencies separately and preserve their notices.
- Run `tools/audit_sources.py --staged` when provided, plus relevant build/tests. Verify the release/tag tree and archive before uploading.
- Local commits are reversible. Push/create repositories or publish releases only within explicit user authorization for that GitHub destination; the skill itself does not authorize publication elsewhere. Never request a password/token in chat; use supported login flows and stored credentials.
