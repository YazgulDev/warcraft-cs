#pragma once

// Keyboard actions have stable INI names; mouse aiming and native menus retain their own input paths.
enum class ShortcutAction {
    ToggleFPS,Forward,Backward,Left,Right,Jump,Crouch,Walk,Reload,Buy,BuyAmmo,Pickup,
    SquadFight,SquadFollow,SquadRelease,Speed,Fog,Minimap,HealthBars,Refill,AllWeapons,ReloadSettings,
    Spell1,Spell2,Spell3,Spell4,SelfCast,Weapon1,Weapon2,Weapon3,Weapon4,Weapon5,Weapon6,Weapon7,ToggleView,ToggleThirdPerson,ToggleAllyTargeting,ToggleUnitView,InventoryNumbers,InventoryPanel,Count
};
