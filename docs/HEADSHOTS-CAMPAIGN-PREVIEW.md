# Headshot and campaign-economy fix preview

Local follow-up to published 0.13.0. Its balances, dollar/gold purchases, reload shopping, movement modes and optional escort remain. This preview is not a new published release.

## Head contacts and blood

Mixed head/neck triangles no longer use a majority vote over their vertices. The accepted ray point interpolates fractional named-head skin influence instead, so a contact near a head corner can count while the shoulder side stays a body hit. `Combat.HeadshotBoneWeight=.35` is editable from .05 to 1 and reloads with F8 for subsequent contacts. The nearest physical contact still wins; body/gate/terrain occlusion and verified-model/pose requirements are retained. No generic upper-body head region is invented for unsupported models or hosts.

Direct Tau/Gluon contacts now share conventional firearms' named-head bonuses by default (`Combat.EnergyHeadshots=true`). False disables energy bonuses while retaining physical head blood. `Headshots=false` still disables all head damage bonuses and their Headshot caption. Confirmed HP loss on an eligible anatomical head contact emits blood from that exact contact independently of the bonus/caption. Existing `CombatVisuals.HeadshotBlood` remains authoritative. Melee, explosives, misses, invulnerability and visual-only continuous-beam retargeting do not create fabricated head blood. Existing short per-target visual throttling/pool limits remain.

## Campaign base startup

For the reported last Frozen Throne undead mission with workers but no base, construction centers on available builders instead of a possibly distant hero. Worker race outranks captured foreign halls. Up to `ArmyAI.PlacementAttempts=12` (1–32) native building sites are tried per decision; accepted construction still owns its existing lease and normal unit order.

Undead acolytes activate an available gold mine with real haunted-mine construction; up to `ArmyAI.LumberWorkers=2` (0–10) unassigned idle ghouls gather real trees. Zero stops new lumber assignments, without canceling existing manual orders. Ghouls with combat, manual or squad reservations keep those priorities. Night elf halls use their available native entangle ability before wisps receive gold-harvest orders. Only compatible owned active mines satisfy undead/night elf harvesting.

`ArmyAI.ShowStatus=true` reports development, queued construction/training, reached targets or why development cannot proceed: no supported economy, no workers, busy/reserved workers, no nearby resources, or native build/train rejection by resources/map rules. U always confirms on/off; F8 requests a current status report. Status messages follow decisions rather than issuing additional game orders. AI state transitions also log gold/lumber values for diagnosis.

All new army keys reload at the next active decision; defaults validate and installation merges preserve custom values. Native costs, blight, building placement, food, tech and mission restrictions decide success. No worker or resources are conjured, no forbidden technology is enabled, and this is still a basic economy assistant rather than a full advanced-research campaign AI. Hero-only maps cannot develop a base without an actual economy. FollowActor stays false; no escort behavior is reintroduced.

## Verification and limits

Authored mixed-face fixtures cover head/shoulder contacts at the same triangle, animation and configurable thresholds. Damage/blood fixtures cover energy opt-out, bonus-disabled blood at the exact contact, no caption and existing immunity policy. Synthetic native army scenes cover worker-only undead starts, distant heroes, captured foreign halls, multiple rejected sites, haunted mines, ghoul wood, night elf entangle, native resource/map rejection and healthy completed targets. Real loader/repeated installer tests cover saved preferences and defaults. Private reads of owned Footman, Peasant, Archer, Death Knight and Warden models still parse and trace; no retail data enters the source/package.

The final mission itself and the active campaign are not driven or restarted. Native module/launcher builds and graphics/source/runtime checks are separate from synthetic scenarios. No live campaign completion or real 1.27a gameplay verification is claimed; unsupported 1.27a head/pose adapters retain their prior limitations.

<!-- launcher-summary
- Fixed headshot classification on mixed head and neck faces using the actual contact and skin influence.
- Enabled direct Tau and Gluon headshot bonuses by default with a configurable opt-out.
- Fixed verified head blood depending on the optional damage bonus or critical caption.
- Fixed campaign base startup using the distant hero instead of available workers and their race.
- Added undead haunted-mine construction and configurable idle-ghoul lumber gathering.
- Added native night elf mine entangling and bounded construction-site retries.
- Added configurable army progress and blocker feedback while preserving native resources and mission restrictions.
-->
