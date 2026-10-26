# Dual currency, headshots and item names preview

Earlier local preview based on 0.12.0. The [current follow-up](CLASSIC-BLOOD-CASINGS-PREVIEW.md) changes the movement default and replaces centered headshot text with target-local effects. The published v0.12.0 download and historical release notes are unchanged.

## Purchases

Open **B** and click **DOLLARS** or **GOLD** at the top of any category. Both balances remain visible; the selected currency has a star. Dollars are the default. The menu choice lasts for the current session; startup or **F8** restores `[Economy] Currency=dollars` (`gold` is also supported).

If the dollar balance cannot cover the whole weapon/ammo price, `[Economy] FallbackToGold=true` pays the whole price from Warcraft gold when sufficient. The wallets are never combined or both charged. Selecting gold uses only gold. Duplicate weapons, full ammunition and rejected purchases spend nothing. Prices apply equally to either currency, and receipts identify the wallet actually charged. The **.** ammo shortcut uses the same policy. Native Warcraft shops retain their own native gold rules.

The independent dollar wallet continues earning while gold is selected. Defaults stay $800 starting funds, $200 per eligible enemy kill and $16000 maximum. The menu currency does not modify map gold earnings or purchases.

## Shots and headshots

On verified 1.26a models, firearm rays trace the authored opaque/cutout MDX triangles in the active skeletal sequence and animation time, transformed by current facing, scale and rendered height. Empty broad-box corners and decorative billboard/light cards no longer intercept a shot aimed at a neighbor. The nearest accepted surface wins; hidden/loaded units and transparent effect helpers cannot shield another unit. Gates and terrain still block shots. Existing melee, spell and TPS volume contracts remain unchanged.

A direct firearm contact on geometry influenced by a named head/skull bone earns the configured headshot multiplier, **2** by default. Confirmed damage displays an amber marker and **HEADSHOT** above the reticle. The bonus applies before native defenses/passives and the existing friendly-fire coefficient. It cannot bypass zero damage or invulnerability. Melee, explosions, structures and unverified fallback shapes receive no headshot bonus. Existing AWP finishing behavior is preserved.

`[Combat] PreciseUnitHits=true`, `Headshots=true`, `HeadshotMultiplier=2` (1-10) are editable; **F8** reloads them. Set `PreciseUnitHits=false` to use the previous broad unit volumes, or `Headshots=false` to disable the bonus/label. These options do not change movement/collision.

## Item and rune names

Armed **F6** and clean **F9** first-person views display the localized current-map name of a visible ground item or rune under the reticle. Inventory slots are not required. Turning away, opening a cursor menu or switching to TPS clears the label; physical bodies, gates and terrain hide names behind them. Unicode Cyrillic names render through a dedicated cached caption texture. Existing item pickup and rune effects are unchanged.

`[Interface] ItemNames=true`, `ItemNameDistance=1200` (160-5000 world units), `ItemNameRadius=32` (4-128 world units) control this label. Distance is the maximum reticle ray length; radius is the spherical aim tolerance. **F8** reloads the settings. Updates insert missing keys while retaining saved values, including gold preference and opt-outs.

## Verification and limits

Production parser/transform tests use project-authored animated meshes and synthetic native memory. Scene tests cover neighboring surface selection, stable ties, allies, hidden/cargo/effect units, gates/terrain and localized rune labels without inventory. Payment tests cover dollar priority, whole-price gold fallback, opt-out, rejected purchases and no mixed charge. Real hidden WGL pixels verify the rendered currency buttons, Cyrillic names, inherited graphics state and context replacement. Config tests cover startup/F8 bounds and idempotent update preservation. Private reads of owned Footman, Peasant, Archer and Death Knight models validate ordinary body and named-head intersections; no game assets enter this package.

Custom models without usable named head geometry receive no headshot bonus. Unsupported models/animation layouts retain approximate ellipsoid body targeting (structures retain volumes). Native animation blends, look-at overrides and texture cutout pixels are not reproduced, so posed triangles are not a pixel-perfect render oracle. Experimental 1.27a retains its existing approximate targeting and lacks verified model/pose/item-enumeration/name adapters; headshots and item labels are unavailable there. Currency purchases work through public natives on both supported hosts. Real 1.27a gameplay remains unverified.

The user's active campaign was not driven or restarted. Actual campaign smoke testing of the new features remains to be done in play.

<!-- launcher-summary
- Added selectable dollar and gold purchases with automatic gold fallback when dollars are insufficient.
- Added configurable headshot damage and feedback using named animated head geometry on verified 1.26a models.
- Fixed nearby units intercepting shots through empty model bounds with precise posed surface tracing.
- Added localized first-person item and rune names under the reticle with obstacle occlusion.
- Preserved existing movement, native defenses, separate wallets and custom settings on update.
-->
