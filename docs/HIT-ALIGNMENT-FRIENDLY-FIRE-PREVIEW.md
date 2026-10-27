# Shot alignment and friendly-fire preview

Local follow-up to 0.13.0 and the [headshot/campaign preview](HEADSHOTS-CAMPAIGN-PREVIEW.md). Includes that preview's changes. This is not a published release.

## Precise contacts

Precise firearm and energy rays now invert the unit sprite's complete rendered basis, position and separate visual scale. The previous transform reconstructed yaw from the gameplay facing and used gameplay XY with a rendered Z; it could disagree with interpolated rotation, tilt and independently scaled axes. The existing native sprite getters already used by world obstacles supply the full transform. Ray distance remains in world units. Melee, spell and TPS volume queries keep their existing movement/volume contracts.

Headshots still require an accepted nearest triangle with sufficient named-head influence, and confirmed HP loss before blood/text. Separate named heads remain separate surfaces; no broad volume bridges them, no larger body box steals a neighboring unit's hit, and gates/terrain still occlude. Invalid render transforms or unsupported animation clocks retain conservative body damage without inventing a head position. The existing HeadshotBoneWeight/EnergyHeadshots/HeadshotBlood settings remain editable with F8.

Fired rounds log `shot candidate` with target/type, render/pose validity, sequence/time, local ray and hit/head classification for candidates crossed by the broad bounds. `shot miss` records the world ray and allied-contact policy. Reticle prediction and visual-only Gluon tracking do not emit these diagnostics. These records distinguish unsupported poses, true surface misses, ally protection and weapon aim/recoil in further reports.

## Friendly fire

Direct allied contacts require `[Combat] TargetAllies=true` and nonzero `[Damage] FriendlyFirePercent` (50 by default). A fresh install still starts with ally protection. Previously Q changed only the in-memory preference; F8 or restarting loaded false again. Q now writes only TargetAllies to the existing INI and reports `FRIENDLY FIRE ON (50%)`, off, damage 0%, or an explicit save failure. F8/restart reload the saved preference, and updates preserve custom values. This does not enable ally targeting on behalf of the user or reset their percentage. C4/native spells retain their separate policies and native immunity/defenses remain authoritative.

## Verification and limits

Authored native-memory fixtures exercise the production sprite getters, nonuniform tilted basis, rendered XY/Z differing from gameplay position, separate visual scale, pose clocks, misses and invalid-transform fallback. Independent two-head surfaces verify both heads and the empty gap. Scene tracing covers neighboring units, allies, hidden/cargo/effect units, gates and terrain. The real INI write/read path verifies Q persistence, percentage preservation and save failure; damage fixtures retain friendly coefficients and native attack tests retain scoped attack/miss behavior.

Private reads of the owner's Ogre/OgreMagi/OgreLord models confirm both named head chains are present and parsed; their model files remain private. Synthetic fixtures do not prove every animation agrees with native skinning. No live campaign input/restart or real 1.27a headshot verification is claimed; the native pose adapter remains limited to verified 1.26a, without native animation blending/look-at reproduction.

<!-- launcher-summary
- Fixed precise shot alignment using the complete rendered unit basis, position and visual scale.
- Fixed Q allied-targeting choices being lost on F8 or restart by saving the preference.
- Improved friendly-fire feedback with the effective damage percentage and save failures.
- Added fired-round pose, candidate and miss diagnostics for unresolved model-specific contacts.
- Preserved the previous headshot, blood, energy-weapon and campaign-economy fixes.
-->
