# Changelog

## Unreleased

- Windows client EXE with game-folder selection, explicit download/install consent, live setup log and Play.
- Embedded audited source payload; clients do not need Git or a separate source archive.
- Install missing Python and signed Microsoft C++ tools/Windows SDK; reuse existing dependencies.
- Preserve owned game installations through the private-runtime setup pipeline.

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
