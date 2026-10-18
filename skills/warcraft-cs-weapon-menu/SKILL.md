---
name: warcraft-cs-weapon-menu
description: Add or change CS 1.6 weapons in Warcraft CS by Yazgul, including combat, buying, private models, animation, hand movement, muzzle flashes, audio, configuration and verification. Apply only to this project.
---

# Weapon and buy-menu integration

When adding a weapon, make it purchasable and visible in the buy menu in the same change. Do not leave combat-only weapons inaccessible to ordinary buyers.

- Preserve existing slot IDs in `src/combat/WeaponSlots.hpp`; extend the weapon, price, ammo and damage tables together. Keep configurable `Refill` and `AllWeapons` actions working for every supported weapon (currently F11/F12). F7 and F9 belong to third-person and unarmed unit-view entry; those bindings must not grant inventory.
- Register one entry per slot in `src/economy/BuyCatalog.hpp`, with its supported category, number key, visible name and cache stem. Navigation and rendering use this catalog; do not introduce separate weapon lists in `BuyMenu` or `BuyMenuView`.
- Put the purchase price in the weapon's `config/WarcraftCS.ini` section and its matching `GameplaySettings` fallback. For weapons with ammunition also add `AmmoPrice` and `AmmoPack`. Do not change unrelated player settings.
- Include the weapon in private setup conversion and generate its hand-free buy preview through `tools/export_buy_previews.py`. Reuse the player's own installed assets or owned procedural geometry; never commit extracted art, textures, models or preview caches.
- Integrate first-person presentation in the same change: follow [weapon presentation](references/weapon-presentation.md) for animated muzzle attachments, movement bob, accepted-shot events, audio and reload clips. New firearms must receive the shared bob and muzzle renderer without a parallel per-weapon drawing path.
- Apply ownership to number keys, wheel switching, rune rewards and ammo purchases. Reject duplicate permanent weapons, unaffordable purchases and full ammunition without charging gold; consumables remain repeatable.
- Update `tests/BuyMenuTests.cpp` so the complete catalog has unique slot registrations and each weapon is reachable by its category/key. Run `tools/test-buy-menu.ps1`, `tools/test-viewmodel.ps1`, affected gameplay tests and a native build. In an authorized offline test copy, inspect movement, barrel-aligned flashes, fire/reload audio and animations. For changed menu rendering, inspect an in-game screenshot and test mouse hitboxes against the visible rows.
- Document the weapon, controls and config. Follow the repository's feature/release workflow; this skill grants no publication or restart authorization.
