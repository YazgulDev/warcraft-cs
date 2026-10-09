---
name: warcraft-cs-weapon-menu
description: Add or change weapons in Warcraft CS by Yazgul, keeping the CS-style buy catalog, configurable prices, private previews and inventory controls synchronized. Apply only to this project.
---

# Weapon and buy-menu integration

When adding a weapon, make it purchasable and visible in the buy menu in the same change. Do not leave combat-only weapons inaccessible to ordinary buyers.

- Preserve existing slot IDs in `src/combat/WeaponSlots.hpp`; extend the weapon, price, ammo and damage tables together. Keep F7 free refill and F9 free all-weapons grant working for every supported weapon.
- Register one entry per slot in `src/economy/BuyCatalog.hpp`, with its supported category, number key, visible name and cache stem. Navigation and rendering use this catalog; do not introduce separate weapon lists in `BuyMenu` or `BuyMenuView`.
- Put the purchase price in the weapon's `config/WarcraftCS.ini` section and its matching `GameplaySettings` fallback. For weapons with ammunition also add `AmmoPrice` and `AmmoPack`. Do not change unrelated player settings.
- Include the weapon in private setup conversion and generate its hand-free buy preview through `tools/export_buy_previews.py`. Reuse the player's own installed assets or owned procedural geometry; never commit extracted art, textures, models or preview caches.
- Apply ownership to number keys, wheel switching, rune rewards and ammo purchases. Reject duplicate permanent weapons, unaffordable purchases and full ammunition without charging gold; consumables remain repeatable.
- Update `tests/BuyMenuTests.cpp` so the complete catalog has unique slot registrations and each weapon is reachable by its category/key. Run `tools/test-buy-menu.ps1`, affected gameplay tests and a native build. For changed menu rendering, inspect an in-game screenshot and test mouse hitboxes against the visible rows.
- Document the weapon, controls and config. Follow the repository's feature/release workflow; this skill grants no publication or restart authorization.