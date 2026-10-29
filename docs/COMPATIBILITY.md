# Compatibility and limitations

[Project home](../README.md) · [Documentation](README.md) · [Requirements](../REQUIREMENTS.md)

## Supported setup

Release **0.12.0** targets Windows 10/11 x64 and **Warcraft III 1.26a
x86**, with `Game.dll` version **1.26.0.6401**, using OpenGL. It additionally accepts
experimental **1.27a (1.27.0.52240)** with D3D9 in both Player and Developer setup.
[1.27a remains a partial port without real-game verification](PORT-1.27.md). Both Reign of Chaos
and The Frozen Throne can be selected in the launcher.

The [development Direct3D preview](DIRECT3D-126A-PREVIEW.md) additionally supports
1.26a through an app-local D3D8-to-9 bridge and a saved graphics selector. Its native
minimap/portrait images remain unavailable; other HUD panels and weapons were
verified in the isolated game. Published 0.14.0 assets do not contain this option.

CS weapon mode requires an owned Counter-Strike 1.6 installation. Optional Crowbar,
Tau Cannon, Gluon Gun and RPG also require complete owned Half-Life resources. **Only Warcraft modes** provides
third-person and unarmed first-person control without Counter-Strike assets.

Reforged and other Warcraft patches are not supported by this published release.
Check the notes bundled with a development build before assuming it supports another host.

## Maps and gameplay

- Offline single player only; multiplayer is unsupported.
- A living owned unit is required. Maps do not need editing, but full campaign and
  custom-map compatibility has not been verified.
- Map scripts may replace the camera or interfere with unit control. Two-player
  missions retain their intended player-count requirements.
- Native Warcraft health, armor, mana, cooldowns, targeting and quest immunity remain
  authoritative. Custom spell orders and map-specific effects may need configuration.
- Hitboxes approximate model bounds rather than individual bones.
- CS recoil is adapted; complete GoldSrc bullet spread/accuracy and the original
  client are not implemented. Purchases use a map-local dollar wallet by default;
  `Economy.Currency=gold` selects Warcraft gold. CS rounds and team economy are not implemented.
- Dollar rewards cover only mod weapon/explosive kills, not native spell/RTS/squad kills.
  Balances are not persisted in Warcraft saves. See [economy rules](ECONOMY-AND-LOOT.md).
- Recovery-mode collision is not a complete campaign guarantee. Keep progress saved;
  platform/bridge/custom-map interactions and camera ownership can need further fixes.

Report a specific map, release version and reproduction steps through
[Discussions](https://github.com/YazgulDev/warcraft-cs/discussions).
See [troubleshooting](../TROUBLESHOOTING.md) for logs and reporting instructions,
and [validation notes](VALIDATION.md) for recorded checks.
