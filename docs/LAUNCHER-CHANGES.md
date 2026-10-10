# Launcher update change lists

Both launcher variants display a compact English list with one change per line. Keep a reviewed UTF-8
`docs/launcher-changes/<version>.txt` for every version being packaged. Each line starts with
`- Added`, `- Fixed`, `- Changed`, `- Updated`, `- Improved`, `- Removed`, `- Enabled`, `- Disabled`
or `- Preserved`. Builds reject missing, empty or invalid current lists. Keep historical lists accurate;
translating an existing summary must not add changes that its release did not ship.
These concise player-facing lists supplement the complete `RELEASE-<version>.md` documents.

The builder adds `ReleaseNotes` to the update manifest and embeds historical lists in both EXEs.
The normal GitHub API path and its rate-limit manifest fallback resolve notes in this order:
valid English manifest list, valid `launcher-summary` metadata, embedded list for the exact version,
legacy body consisting entirely of English change bullets, English unavailable message.
Never reuse another version's list or claim unpublished fixes in a historical list. The 0.6.1 list
describes the published audio update; the crosshair/logging changes belong to 0.7.0.

Full GitHub release documentation is never converted into change bullets. Headings, code blocks,
configuration examples and installation prose cannot override the exact-version fallback. Markdown
bullets and indented continuations in a short summary are normalized to Windows CRLF. Non-Latin
letters are rejected; authors must review the wording as English before packaging or publication.
The dialog preserves already resolved summaries instead of replacing them with its older cache.

For future releases, keep the complete visible GitHub notes and append this hidden metadata,
using only actual shipped changes:

```text
<!-- launcher-summary
- Added CS sound volume control.
- Fixed an issue included in this release.
-->
```

These are format examples, not claims about a release. Never rewrite published tags or shorten
the complete public release notes to fit the launcher. Neither descriptions nor lists are executed. Opening the dialog
does not install anything; existing explicit Update/Not now behavior remains.

Run launcher and actual dual-package checks, including successful API responses with the complete
0.6.1 body, missing bodies, metadata, unknown versions and both launcher variants/modes.
Preview both EXEs with `--preview-update <png>` and
`--preview-update-developer <png>` to inspect the real embedded list without network or activation.
