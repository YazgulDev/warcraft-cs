# Portrait and head gore preview

Local work after 0.15.0; published 0.15.0 assets remain unchanged.

Direct3D portrait capture now disables inherited scissor before clearing its private color/depth buffer. Previously, a native world-band scissor could exclude the small portrait target: the retained nearer face depth rejected later animation frames. Capture also restores the preceding viewport when native UI resumes with a projection change alone. The hidden HAL regression renders a later, farther-depth face frame under the inherited scissor; it fails on the previous order and passes after the fix.

A fatal hostile direct head contact can remove the struck named head while the body keeps its native death animation and decay. Two-headed models retain the other head. Native damage, kill triggers, experience and resurrection keep their existing behavior. This is a render-only 1.26a adapter for validated classic named-head rigs; unsupported models/hosts keep their normal death. No new game assets are distributed.

The stretching fix resolves the anchor from a bone actually used by the current visible skin. Authoring head parents on naga and Crypt Fiends are not necessarily skinned; their uncomputed matrix slots cannot position a removed head. Blended groups now replace only the struck head's averaged contribution, preserving native neck/body/jaw and other-skull contributions. Gameplay's headshot weight threshold no longer creates a visual cutoff in blended skin.

`[CombatVisuals]` settings apply at startup/F8 and installer updates preserve saved choices:

| Key | Default | Meaning |
| --- | --- | --- |
| BodyBlood | true | Small spray on confirmed hostile nonbuilding body hits; true/false or 1/0. |
| BodyBloodCount | 6 | 1–64 droplets per burst. |
| BodyBloodScale | 0.65 | .25–4 size factor. |
| BodyBloodSeconds | 0.35 | .1–3 seconds. |
| BodyBloodSpeed | 110 | 0–500 world units/second; uses shared BloodGravity. |
| HeadshotDecapitation | true | Remove the struck head on subsequent fatal hostile head contacts; false retains native whole models. |
| FatalBloodMultiplier | 3 | Fatal spray multiplier, clamped to 1–4; shares the existing 256-droplet pool. |
| BloodCount | 48 | 1–64 droplets per ordinary burst. |
| BloodScale | 1.75 | .25–4 size factor. |
| BloodSeconds | 0.8 | .1–3 seconds. |
| BloodSpeed | 220 | 0–500 world units/second. |
| BloodVersion | 2 | Stock-upgrade marker; set 2 to preserve a deliberate return to old values. |

`HeadshotBlood=false` disables headshot blood independently of head removal and the floating Headshot caption. F8 clears existing particles; head-removal preferences affect future deaths. Existing custom blood tuning and explicit opt-outs survive updates. Native corpse decay still occurs at the map's normal time.

Body wounds emit six small, short-lived droplets at the accepted impact after actual HP loss. They create no Headshot caption or head removal; misses, protected allies and buildings do not emit body blood. Body pellets/beams coalesce separately from head contacts. `BodyBlood=false` disables only body blood; disable both blood flags to hide all blood.

## Verification

Real 1.26a OpenGL and Direct3D shots were verified on a footman and a two-headed ogre in the isolated test map. The ogre retained its body and the other head in both renderers. A disposable test-only clock freeze made the first corpse pose inspectable; ordinary modules never freeze corpses. Eight consecutive Arthas portrait samples continued changing after the fix; the earlier Direct3D capture became almost static, while the OpenGL reference animated normally. The native portrait clock and skin data also continued advancing.

Synthetic native-palette tests cover body/other-model isolation, separate heads, resurrection, stale identities, invisible/disabled geosets, uncomputed authoring-parent matrices, blended neck skin and groups shared by two skulls. Shared groups preserve the other skull's native contribution. Relevant configuration, precise-shot, visual-pool and hidden HAL/WGL checks accompany the preview. Real 1.27a head removal is unsupported and no native 1.27a test is claimed.

The stretching follow-up exercised fatal AWP contacts on Crypt Fiends, Naga Myrmidons and Naga Sirens in the private 1.26a OpenGL/Direct3D lab, plus an ogre check. Native logs confirmed anchors from the used mesh bones rather than unused authoring parents; screenshots and synthetic native averaging regressions checked the resulting shape. The stale-parent regression fails before the correction and passes after it, including 1/3 and 1/2 neck weights and validation before any palette write.

The body-blood follow-up passed real 1.26a OpenGL and Direct3D contact checks: an ordinary footman body hit created six droplets and no caption, a head contact created 48 droplets and a caption, F8 BodyBlood=false suppressed the next body spray, and a damaging town-hall contact created none. Hidden WGL checks verify visible body particles; portable pool, settings and repeated installer-update tests cover bounds, independent opt-outs, head/body coalescing and preservation.

<!-- launcher-summary
- Added a small configurable blood spray at confirmed enemy body impacts, independent of headshot blood.
- Added configurable fatal headshots that remove the struck head while preserving the body and native death events on verified 1.26a models.
- Preserved the other head on two-headed creatures and restored complete models on native resurrection.
- Improved headshot blood density, size and lifetime with a configurable fatal spray multiplier.
- Fixed a lethal pellet losing its blood burst immediately after a nonfatal head pellet.
- Preserved custom blood settings, explicit visual opt-outs and existing gameplay defaults during updates.
- Fixed Direct3D portrait animation freezing when the previous frame depth was left uncleared by the native world scissor.
- Fixed stretched headshot meshes on naga and Crypt Fiends by using active skin bones and preserving blended neck and jaw contributions.
-->
