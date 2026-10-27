# HUD rendering repair preview: issue 10

Local follow-up to 0.13.0. Retains the [headshot/campaign](HEADSHOTS-CAMPAIGN-PREVIEW.md) and
[shot-alignment/friendly-fire](HIT-ALIGNMENT-FRIENDLY-FIRE-PREVIEW.md) previews; this is not a published release.

## Report and diagnosis

[Issue 10](https://github.com/YazgulDev/warcraft-cs/issues/10) describes flickering/missing HUD and a
weapon disappearing after shooting while audio continues. Its screenshots show a visible gun with
missing flat HUD elements. The attached log identifies Warcraft 1.26a and Radeon RX 580 OpenGL,
successful font atlases, submitted HUD/reticle geometry and no observed GL errors. It does not contain
a shot-time capture of the disappearance, and does not record separate shader-pipeline bindings.
Consequently that log alone cannot establish the reporter's exact trigger.

The renderer had a confirmed isolation defect: `glPushAttrib` does not preserve/disable GLSL program
bindings or separate program pipelines. `CURRENT_PROGRAM=0` does not imply fixed-function rendering
when a separate pipeline remains bound. An inherited fragment program can blacken/discard immediate
weapon triangles and HUD quads without producing a GL error. Hidden WGL tests reproduce that path
with the old drawing contract. This is a proven missing-rendering path, not proof that the reporter's
Radeon had that pipeline bound; the added raw diagnostics allow that distinction to be checked.

## Change

`FixedFunctionState` temporarily unbinds supported GLSL/separate programs and disables supported
ARB vertex/fragment programs and rasterizer discard around mod world effects, weapons and HUD.
It restores each binding/enable explicitly after drawing. Standalone flat HUD scopes use the same
state owner. D3D9 returns through its existing independently managed adapter without WGL changes.
There are no new player settings, driver changes or gameplay/balance changes.

The raw HUD log now includes `pipeline` beside `program` before normalization. Prepared/completed
mod passes show both zero. The state owner preserves existing foreign rendering after the mod pass.

## Verification

Real hidden WGL tests on Intel UHD Graphics create an active separate fragment pipeline while the
current program is zero, reproduce invisible weapon/reticle/text with no GL error, and verify exact
recovery of the reference pixels plus program/pipeline/discard restoration. Conventional GLSL
programs, nested scopes, existing raster/texture/font regressions and three replacement contexts
are covered. The HUD runner now catches signed native aborts as failures. Shared viewmodel/animated
muzzle-flash tests also verify actual WGL pixels and state. Radeon-specific confirmation still
requires the reporter's updated run; no real 1.27a gameplay verification is claimed.

<!-- launcher-summary
- Fixed inherited OpenGL programs hiding fixed-function HUD, weapon and effect geometry.
- Added separate shader-pipeline isolation with exact host-state restoration.
- Improved HUD diagnostics to report separate program pipelines even when the current program is zero.
- Added real WGL pixel regressions for missing weapons, reticles and text across graphics contexts.
- Preserved the previous headshot alignment, saved friendly-fire and campaign-economy fixes.
-->
