# CS-style carried weapon menu

[Project home](../README.md) · [Documentation](README.md) · [Controls](CONTROLS.md)

Armed F6 keeps every supported weapon you own. Selecting another weapon does not discard the
others or impose a primary/pistol carry limit. The menu previews a choice before equipping it.

With Tab item-number mode off, the first five weapon bindings open these categories:

| Default key | Category |
| --- | --- |
| 1 | Primary: rifles, sniper rifles, SMGs, machine guns, shotguns and available RPG |
| 2 | Pistols |
| 3 | Melee weapons |
| 4 | Grenades |
| 5 | C4 |

Press the same category again to highlight the next carried weapon in that group. Wheel up/down
browses all carried weapons in displayed group order (1 through 5), top to bottom in each list,
and opens the same menu. Wheel up retraces that order. One detent advances one available weapon;
fast batches retain their direction and every detent, skipping unowned/depleted entries.
Left-click confirms the highlight; that click
does not fire, throw or plant, including when held. Release and click again to use the equipped weapon.
Right-click, 0 or Escape cancels without changing weapons. A second Escape after closing returns
to RTS as usual. A category containing the equipped weapon starts on that weapon.

Only owned weapons appear. Empty firearms remain selectable for reloading; spent C4/grenades are
omitted until replenished. No currency or ammunition is spent by browsing. The hand-free previews
reuse the private buy-menu caches from the owner's CS installation; text works without those images.

Tab still starts in item mode by default: 1–6 use Warcraft inventory items. Switch Tab off for the
weapon category shortcuts; the wheel can browse weapons in either item-number mode. I, buying,
native shops, view changes, focus loss and F8 dismiss pending selection. F7 retains camera-wheel
zoom and native combat; F9 remains clean. Existing weapon bindings 6/7 retain direct C4/sword access
when their keys are not owned by item-number routing. Remapping Weapon1–Weapon5 remaps the categories.

Edit the runtime's `WarcraftCS/WarcraftCS.ini`, then press F8:

| `[WeaponSelection]` key | Default | Meaning |
| --- | --- | --- |
| Enabled | true | true/false. false restores legacy direct weapon 1–7 and immediate wheel switching |
| TimeoutSeconds | 5 | .5–15 seconds after the last browse action; expiry cancels, never equips |

Missing/malformed values use defaults; finite numeric values are clamped. Startup/F8 apply these
preferences, and updates add missing keys without replacing custom values or the Tab default.
