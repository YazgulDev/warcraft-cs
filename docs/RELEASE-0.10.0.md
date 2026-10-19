# Warcraft CS 0.10.0

Released 2026-10-18. These notes cover **all changes since the immutable v0.9.1 tag**,
including every completed feature branch merged for this release. The HUD text renderer fixed
in 0.9.1 remains in place. This release adds the unit HUD, interactive inventory and squad
commands, together with spell, camera, collision and gate-attack fixes.

## Download and upgrade

- [WarcraftCSLauncher_DLL_Included.exe](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.10.0/WarcraftCSLauncher_DLL_Included.exe): ready project modules; choose **Install — Player** to avoid C++ Build Tools/SDK downloads.
- [WarcraftCSLauncher.exe](https://github.com/YazgulDev/warcraft-cs/releases/download/v0.10.0/WarcraftCSLauncher.exe): source installation with local compilation and dependency preparation after consent.
- `WarcraftCS-0.10.0-dist.zip`: standard launcher, sources, update manifest, requirements and checksum. Public ZIPs remain free of DLL/MIX modules; the DLL-included EXE is a separate download.

Existing clients can choose **Update from GitHub**, then close and restart Warcraft so the
new module loads. A running game keeps its already loaded module. Fresh clients can use
**Install** in either launcher. Existing prices, damage choices, inventory preferences,
custom controls and the Warcraft-only installation profile are preserved; missing new keys
are inserted into the existing INI sections.

Two deliberate defaults are worth checking after updating: **1..6 initially use items**, and
third-person camera building collision is initially disabled. Tab changes items/weapons;
`NumbersByDefault=false` retains a weapon-first startup. `CameraBuildingCollision=true`
restores the previous camera push-in near buildings. See the complete settings table below.
Intermediate test installs using the former semicolon squad toggle migrate it to **Y**.

Supported host: offline **Warcraft III 1.26a x86**, `Game.dll` **1.26.0.6401**.
The Warcraft-only profile supports F7/F9 without CS assets. The full weapon profile also
requires owned Counter-Strike 1.6 assets. Reforged, other patches and online play are unsupported.
No original game libraries, maps, saves, models, portraits, item icons or sounds are bundled.
See [installation](https://github.com/YazgulDev/warcraft-cs/blob/v0.10.0/INSTALL.md) and
[requirements](https://github.com/YazgulDev/warcraft-cs/blob/v0.10.0/REQUIREMENTS.md).

## F6/F7 unit HUD and inventory

- Add an animated native unit portrait in the lower left, with live health and mana values/bars.
  Heroes also show their current level and map-resolved experience progress; ordinary units do
  not receive a fabricated XP bar. Capped heroes do not receive an invented next threshold.
- Show six native inventory slots above the spell row, using current-map item icons and charges.
  The portrait and inventory use the owner's installed Warcraft assets and update on the game
  thread. Portrait rendering isolates its native framebuffer/depth state from the world/HUD.
- Keep the **F9** unit view clean: no new HUD, inventory panel or weapon interface there.
  Third-person hints omit CS-only free-ammunition/free-weapons commands.
- **Tab** toggles physical **1..6** between item activation and weapon selection. Items are the
  initial choice by default. The session choice survives F6/F7 switches and inventory cleanup;
  startup, map reset and F8 restore the saved `NumbersByDefault` preference.
- **I** opens/closes mouse inventory control. Click an item to use it, choose its required native
  target, drag outside the inventory to drop it beside the actor, or drag between slots to
  rearrange items. Escape/RMB cancel an inventory gesture instead of firing or issuing a squad order.
- Preserve Warcraft charges, cooldowns, mana, target restrictions, channels and non-droppable-item
  rules. Slot/item identity is rechecked when input reaches the game thread, avoiding stale-item use.
  A requested operation may be rejected by the native item rules; it does not fabricate an effect.

## Common-target squad orders

- In **F7/F9**, **RMB** orders all available owned mobile units to attack the target under the
  reticle; aim at the screen center in clean F9. This is independent of H/O recruitment radius
  and cap. The controlled actor, buildings, other owners, dead, paused, hidden, loaded and
  otherwise unavailable units are excluded. Native attack eligibility and pathfinding still apply.
- In armed **F6**, **Y** toggles RMB between squad orders and the weapon action. The default
  starts with weapon action, retaining AWP scope and knife secondary attack. Switching orders
  off clears the common target; a held RMB cannot become an accidental scope/secondary attack.
  The HUD identifies RMB's current role. The chosen mode lasts for the map; startup/map reset/F8
  restore `ReticleAttackInFPS`.
- Click empty space to release the common target and restore the previous H/O policy; other
  commanded units return to native behavior. Normal acquisition may choose nearby enemies again.
  Target death/invisibility, ownership or handle changes, H/O/J strategy changes, RTS/map exit
  and feature opt-out release the temporary command safely.
- Explicit target orders outrank following, yielding, spell AI and the combat leash while units
  approach or fight. Active natural **H** combat now also outranks corridor/leash movement by
  default. Idle units still follow/yield; O retains its deliberate follow-without-attacks policy.
  Automatic combat spells retain their existing native rules.
- Protected allies between camera and enemy no longer swallow the command: attack allegiance
  is filtered before nearest-hit selection. Q opt-in restores selecting player allies. Ally-targeted
  spells and items retain their own selection policy. If a target is selected but no native attack
  is accepted, the HUD reports that no available units can attack it rather than an empty-space cancel.
- Inventory cancellation and buy-menu input keep priority over world orders.

## Spells, neutral structures and gates

- Point-only abilities such as **Blink** and **Flame Strike** keep their terrain target even when
  an ally is nearby. Auto targeting uses the actor's native command flags; a missing/rejected
  ground point never falls back to a creature target. Alt/self casts use the actor's ground point.
  Explicit spell-slot target overrides remain available. Learned skills, costs and cooldowns remain native.
- Neutral-owned units/buildings are targetable by direct FPS/TPS attacks with allied targeting
  disabled. Actual player allies remain protected until Q/`TargetAllies` opts in.
- Common-target commands support destructable gates/crates, including the LTg3 gate type used
  in campaign maps. Track typed widget identity/life independently of units, including target
  death, removal, cancellation and handle reuse.
- **F7 ranged attacks now launch the native missile at destructable gates**, including Kael's
  gate attacks. Native preparation, flight, impact, armor and quest immunity remain authoritative.
  Revalidate typed target identity after attack callbacks; do not add instant duplicate damage
  after a handled launch. Neutral structures retain their native missile path as well.
- Existing `NativeRangedProjectiles=false` or `EnableAttackEffects=false` selects the direct-hit
  fallback on F8. Melee/firearm destructable hits retain direct damage. Neutral-passive scenery
  also retains finite native damage with optional AWP finishing enabled; quest protection is not removed.

## Stable camera and side contact

- Default third-person zoom no longer collapses into the actor near broad building model bounds,
  including buildings ahead or behind. The selected wheel distance stays stable. Building collision
  can be opted back in; terrain and destructable obstruction still shorten the boom. Disabling
  building camera collision allows the camera to pass through building models.
- Prevent transient sideways actor/model snaps near known walls and colliding units. During
  approach, parallel sliding and retreat on either side of a solid, commit the position already
  validated by the swept collision solver instead of invoking native nearest-cell relocation
  and then rolling it back. Terrain, body volumes, bridge/cliff support and open-space unknown
  blocker validation remain in effect. Existing movement tuning is unchanged.

## New settings and changed defaults

All listed gameplay keys use the existing `WarcraftCS/WarcraftCS.ini` and apply on startup/F8.
Numbers are world units unless otherwise noted. Supported booleans are true/false or 1/0.
Missing or malformed values fall back to the documented default; bounded finite numbers are clamped.

| Section/key | Default | Meaning / range |
|---|---|---|
| Interface.InventoryControls | true | Enable Tab item-number mode and I mouse inventory in F6/F7 |
| Interface.ShowUnitHud | true | Animated portrait, HP/mana and hero XP in F6/F7 |
| Interface.ShowInventory | true | Show the six inventory slots above spells |
| Interface.UnitHudScalePercent | 100 | Portrait/status panel scale, 60..160 percent |
| Inventory.NumbersByDefault | true | Item numbers on startup/F8/map reset; false starts with weapon numbers |
| Inventory.DropDistance | 80 | Native drag-out drop distance, 16..128 |
| Shortcuts.InventoryNumbers | TAB | Rebind the item/weapon-number toggle; NONE disables the key |
| Shortcuts.InventoryPanel | I | Rebind the mouse inventory panel key; NONE disables the key |
| Shortcuts.SquadOrders | Y | Rebind the armed-F6 RMB role toggle; NONE disables the key |
| ThirdPerson.CameraBuildingCollision | false | true restores camera push-in against building bounds |
| Squad.ReticleAttack | true | Enable common-target RMB orders |
| Squad.ReticleAttackInFPS | false | Initial armed-F6 RMB role; true starts with orders |
| Squad.ReticleAttackDistance | 3000 | Reticle picking range, 100..15000; native attack reach still applies |
| Squad.PreserveCombatOrders | true | Keep H combat ahead of yielding/leash travel; false restores legacy policy |

Updates preserve explicit preferences and add missing keys without duplicate sections. Existing
custom actions on Tab, I or Y win over the corresponding newly added default; the new action
becomes NONE instead of resetting the whole layout. Other custom squad keys/NONE are preserved.
The former intermediate `SquadOrders=SEMICOLON` / `;` default migrates to Y during update or
startup/F8, respecting a saved Y action. The parser still recognizes the semicolon key name.
Existing ranged-projectile/attack-effect, allied-targeting and collision-size settings remain editable.

## Source, packaging and validation

- Add focused portrait/unit-HUD, inventory input/native-item adapters and squad focus/order-mode
  modules. Shared ray picking accepts an action-owned candidate filter; spell/item selection keeps
  its existing defaults. Typed native widget handling is shared by gates and target orders.
- Wire the authored unit-HUD, interactive-inventory and squad regressions into the full independent
  test wrapper. Extend spell targeting, native attack callback lifecycle, camera, configuration
  migration, collision and unit-target tests for the changed behavior. Document module ownership.
- Both launchers embed the same audited source snapshot. Ordinary Player modules exclude opt-in
  test-scene handlers. The native payload contains original project modules/notices, not retail DLLs.
- The complete native-independent regression suite and configuration migration checks pass on
  the merged sources. Release checks validate both EXEs, source/version/native payload hashes,
  native-free public ZIPs, launcher install/update behavior and both variants' complete-release-body
  versus compact-summary handling.
- Separate disposable Warcraft 1.26a scenes exercised animated HUD/inventory controls, Blink and
  Flame Strike target selection, stable camera distances, all-unit RMB attack/cancel and Y routing,
  active-combat priority, neutral/gate attacks, left/right wall/unit contact and native ranged gate
  missiles. Actual screenshots/recording frames were inspected. The final release build receives
  a native gate/missile smoke check; version/source metadata is rebuilt from the same runtime logic.
- In the gate test, native launches leave immediate HP unchanged; later flights apply one 30-point
  hit each (500, 470, 440, 410 before successive launches). Native opt-outs restore direct damage;
  a quest-immune gate stays at 500 HP. Side-contact scenes retain forward sliding and stable idle
  coordinates on both sides without native relocation. Test module/config restoration is byte-exact,
  and save snapshot comparisons show no changes. User campaign progress is not used for these tests.

This remains an early offline prototype. These checks do not establish complete campaign,
custom-map, GPU/driver or every item/ability compatibility. Native map restrictions may reject
orders, casts or item operations. HUD content depends on valid installed/map data; the camera
can pass through building visuals with its new default. Historical details and pre-existing
limits remain in the [0.9.1 notes](https://github.com/YazgulDev/warcraft-cs/blob/v0.10.0/docs/RELEASE-0.9.1.md).

<!-- launcher-summary
- Added animated portraits, health, mana and hero experience to the F6/F7 HUD.
- Added inventory icons, Tab item hotkeys and I mouse use, targeting, dropping and rearranging.
- Added right-click squad target orders in F7/F9 and a Y toggle in armed F6.
- Preserved active squad combat and fixed orders intercepted by protected allies.
- Fixed neutral structure and gate attacks, including native ranged projectiles.
- Fixed Blink and Flame Strike targeting near allies.
- Fixed unstable third-person zoom near buildings and side collision near walls and units.
- Enabled item numbers by default and preserved custom settings during upgrades.
-->
