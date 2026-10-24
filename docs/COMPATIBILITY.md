# Compatibility and limitations

[Project home](../README.md) · [Documentation](README.md) · [Requirements](../REQUIREMENTS.md)

## Supported setup

The published **0.11.0** release targets Windows 10/11 x64 and **Warcraft III 1.26a
x86**, with `Game.dll` version **1.26.0.6401**, using OpenGL. Both Reign of Chaos
and The Frozen Throne can be selected in the launcher.

CS weapon mode requires an owned Counter-Strike 1.6 installation. The optional RPG
also requires complete owned Half-Life resources. **Only Warcraft modes** provides
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
  client are not implemented. Purchases use Warcraft gold; CS rounds and team economy
  are not part of this release.

Report a specific map, release version and reproduction steps through
[Discussions](https://github.com/YazgulDev/warcraft-cs/discussions).
See [troubleshooting](../TROUBLESHOOTING.md) for logs and reporting instructions,
and [validation notes](VALIDATION.md) for recorded checks.
