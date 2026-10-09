# Launcher update change lists

Both launcher variants display a plain list with one change per line. Keep a reviewed UTF-8
`docs/launcher-changes/<version>.txt` for every version being packaged; each line starts with `- `.
Builds reject an empty/missing current list or lines without that prefix. Preserve historical files.
These concise player-facing lists supplement the complete `RELEASE-<version>.md` documents.

The builder adds `ReleaseNotes` to the update manifest and embeds historical lists in both EXEs.
The normal GitHub API path and its rate-limit manifest fallback resolve notes in this order:
manifest list, legacy GitHub body, embedded list for the exact version, explicit unavailable message.
Never reuse another version's list or claim unpublished fixes in a historical list. The 0.6.1 list
describes the published audio update; test-branch crosshair/logging work is not part of that release.

Markdown bullets and wrapped continuations from legacy release bodies are normalized for the
Windows textbox. Neither release descriptions nor embedded lists are executed. Opening the dialog
does not install anything; existing explicit Update/Not now behavior remains.

Run launcher and actual dual-package checks. Preview both EXEs with `--preview-update <png>` and
`--preview-update-developer <png>` to inspect the real embedded list without network or activation.
