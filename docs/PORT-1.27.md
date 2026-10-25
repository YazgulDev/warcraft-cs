# Experimental Warcraft III 1.27a support

Release **0.12.0** adds experimental **Game.dll 1.27.0.52240, x86** support alongside
the existing **1.26.0.6401** host. Reforged and other builds are rejected. Older
published 0.11.0 downloads do not contain this port.

## Installation

Use Player setup in the 0.12.0 DLL-included launcher, or Developer/source setup, with your own complete 1.27a installation and the
existing `setup.cmd -WarcraftDirectory ... -CounterStrikeDirectory ...` command.
Choose a separate installation folder. Warcraft-only setup remains available.
The original game is never patched on disk.

Source Play and launchers built from this source select **D3D9 for 1.27a** and
retain **OpenGL for 1.26a**. Source launch accepts
`tools/launch.ps1 -Renderer Auto|OpenGL|D3D9`; Auto is the default, applied on the
next launch. OpenGL on 1.27a is a diagnostic path, not a verified combination.

New unified Player packages include the shared 1.26a/1.27a module and an adaptive
Miles proxy. Setup derives exact export data from the selected owner's original
sound library, retaining names, aliases and ordinals without shipping that library
or requiring C++ Build Tools. The original game stays untouched. Legacy published
0.11.0 packages retain their exact host/hash restriction. See
[complete release details](RELEASE-0.12.0.md); real 1.27a gameplay remains unverified.

## Design

`GameBuild` selects exact native tables and checks three independent 1.27a code
signatures before binding. Missing internal translations return zero; unavailable
adapters do not install 1.26a hooks. Public JASS entries were matched by registered
name and checked against the recorded parameter/return types.

`D3DHooks` adapts early D3D9 factory/device/presentation interception from the
reference fork. The existing 1.26a world/frame hooks retain their role; 1.27a uses
API presentation ticks because its world callback remains unresolved.

`GraphicsApi` forwards legacy OpenGL unchanged and adapts the current weapon/HUD/
projectile presentation to `D3DGraphics` and `D3DTextures` during D3D9 frames.
The adapter converts clip depth, triangulates quads, owns managed textures and
restores host graphics state. Layout and gameplay stay in their current modules.

## Current limitations

This is an **experimental partial port**, not feature parity with 1.26a:

- Native actor mesh filtering, jump skinning, attack-effect replacement,
  movement/combat guards, hover-ring interception and floating-label filtering
  remain disabled without verified 1.27a signatures. Native automatic attack and
  movement can interact differently with direct control.
- Native attack-stat reads are unavailable. In unit damage mode,
  `[Damage] UnavailableAttackFallback=weapon` uses each weapon's configured
  Damage/secondary damage; `zero` disables this fallback. Missing/empty/invalid
  values use `weapon`. Startup/F8 reloads it and updates preserve custom values.
  A real zero-attack unit on a working adapter still deals zero.
- Native unit/destructable model discovery, nearby item/destructable enumeration,
  exact terrain/walkable bridge helpers and camera ceiling overrides are
  unavailable. Hitboxes use existing approximate defaults and terrain uses public
  location-height natives. Item/gate interaction and bridge/tree collision do not
  claim 1.26a behavior.
- Native XP-threshold and inventory-art helpers are unavailable. Public health,
  mana, level, XP and item charges remain available. Internal item activation
  forms are unavailable. Spell helpers retain their own capability guards.
  Dynamic order-name conversion also remains disabled until its helper ABI is verified.
- Native minimap and portrait capture have no D3D9 feed and are omitted there.
  Their OpenGL capture paths remain intact.

## Verification

The owner has no 1.27a installation. **No real 1.27a launch, campaign, fullscreen
or Alt-Tab verification is claimed.** Compilation and synthetic metadata checks
cannot establish real game compatibility.

The existing regression suites and launcher/install checks passed, including real
OpenGL/D3D9 hidden-device pixels. A separate 1.26a test copy reached native binding
and hook initialization but did not produce a verified loaded-map frame; this is
not counted as a successful in-game smoke test.

`tools/test-d3d9.ps1` checks actual pixels on a real hidden D3D9 device: quads,
textured colors, GDI alpha-atlas text, world-depth occlusion, resource persistence
and host-state restoration. It uses no game assets or desktop input.
`tools/test-host-builds.ps1` uses project-authored version-only PE fixtures to
check exact installer versions, address translation, unsupported builds and safe
rejection of a forged 1.27a version lacking code signatures. Both are included
in `tools/test-all.ps1`.

To regenerate native entries, privately run `tools/resolve_native_offsets.py`
against the owner's Game.dll, then `tools/generate_native_offsets.py` against its
registration JSON. Keep JSON and all game files outside source commits.

## Credits

Address research, table generation and D3D9 interception adapt **niceqwer55555**'s
work in [TTT-ux-max/warcraft-cs](https://github.com/TTT-ux-max/warcraft-cs), pinned
to `571fdec95a3059935545ab4c5a1b916fe5ea4866` (including TTT-ux-max's README edit).
The fork retains the project's MIT OR Apache-2.0 licensing and notices. The shared
presentation adapter is project-authored. No Warcraft DLL, proprietary code, art
or binary-derived dump is included.
