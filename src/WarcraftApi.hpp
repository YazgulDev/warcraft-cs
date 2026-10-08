#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

namespace wc3 {
using Handle = uint32_t;
using Bits = uint32_t;
float Real(Bits bits);
bool Bind(HMODULE game);
void Log(const char* format, ...);
void OpenLog(const char* directory);
float Ground(float x, float y);
// Query real walkable geometry above terrain; removed/dead bridges cease to supply this surface.
bool WalkableSurface(float x, float y, float& height);
Handle PickOwnedUnit();
bool SinglePlayer();
// Neutral owners include hostile, passive, victim and extra slots in classic Warcraft.
bool IsNeutralUnit(Handle unit);
// Cosmetic effects use Warcraft models without introducing pathing/collision units into the map.
std::string UnitModelPath(int type);
// Enumerate native destructable widgets without passing a C++ function as JASS bytecode.
std::vector<Handle> NearbyDestructables(float x, float y, float radius);
std::vector<Handle> NearbyItems(float x,float y,float radius);
// Current native min/max attack includes dice, hero attributes, items and buffs; FPS uses their mean.
float UnitAttackAverage(Handle unit);
extern int (__cdecl* GetUnitCurrentOrder)(Handle);
extern Handle (__cdecl* CreateItem)(int,float*,float*);
extern void (__cdecl* RemoveItem)(Handle);
extern int (__cdecl* GetItemTypeId)(Handle);
extern int (__cdecl* GetItemType)(Handle);
extern Bits (__cdecl* GetItemX)(Handle);
extern Bits (__cdecl* GetItemY)(Handle);
extern BOOL (__cdecl* IsItemOwned)(Handle);
extern BOOL (__cdecl* IsItemVisible)(Handle);
extern BOOL (__cdecl* UnitAddItem)(Handle,Handle);
// Native inventories and orders work for ordinary units as well as heroes.
extern int (__cdecl* UnitInventorySize)(Handle);
extern Handle (__cdecl* UnitItemInSlot)(Handle,int);
extern BOOL (__cdecl* IssuePointOrderById)(Handle,int,float*,float*);
extern BOOL (__cdecl* IssueTargetOrderById)(Handle,int,Handle);
uintptr_t UnitObject(Handle unit);
std::string DestructableModelPath(int type);
float DestructableFacing(Handle object);
Handle Effect(const char* model, float x, float y);
extern Handle (__cdecl* AddSpecialEffect)(Handle, float*, float*);
extern void (__cdecl* DestroyEffect)(Handle);

// JASS reals return their IEEE bits in EAX and receive real parameters by pointer.
extern Handle (__cdecl* GetLocalPlayer)();
extern Handle (__cdecl* Player)(int);
extern Handle (__cdecl* CreateGroup)();
extern Handle (__cdecl* CreateUnit)(Handle, int, float*, float*, float*);
extern void (__cdecl* PauseUnit)(Handle, BOOL);
extern void (__cdecl* KillUnit)(Handle);
// Explicit verification removes its own units; bomb clocks follow native Warcraft pause behavior.
extern void (__cdecl* RemoveUnit)(Handle);
extern Handle (__cdecl* CreateTimer)();
extern void (__cdecl* TimerStart)(Handle, float*, BOOL, Handle);
extern Bits (__cdecl* TimerGetElapsed)(Handle);
extern void (__cdecl* DestroyTimer)(Handle);
extern void (__cdecl* DestroyGroup)(Handle);
extern void (__cdecl* GroupEnumUnitsSelected)(Handle, Handle, Handle);
extern void (__cdecl* GroupEnumUnitsOfPlayer)(Handle, Handle, Handle);
extern void (__cdecl* GroupEnumUnitsInRange)(Handle, float*, float*, float*, Handle);
extern Handle (__cdecl* FirstOfGroup)(Handle);
extern void (__cdecl* GroupRemoveUnit)(Handle, Handle);
extern Handle (__cdecl* GetOwningPlayer)(Handle);
// Economy operates on native Warcraft gold, preserving the campaign's resource triggers/UI.
extern int (__cdecl* GetPlayerState)(Handle,int);
extern void (__cdecl* SetPlayerState)(Handle,int,int);
extern int (__cdecl* GetUnitAbilityLevel)(Handle,int);
extern int (__cdecl* GetPlayerController)(Handle);
extern int (__cdecl* GetPlayerSlotState)(Handle);
extern int (__cdecl* GetUnitTypeId)(Handle);
extern Bits (__cdecl* GetUnitX)(Handle);
extern Bits (__cdecl* GetUnitY)(Handle);
extern Bits (__cdecl* GetUnitFacing)(Handle);
extern Bits (__cdecl* GetUnitState)(Handle, int);
extern Bits (__cdecl* GetUnitFlyHeight)(Handle);
extern Bits (__cdecl* GetUnitMoveSpeed)(Handle);
extern Bits (__cdecl* GetUnitDefaultMoveSpeed)(Handle);
extern BOOL (__cdecl* IsUnitType)(Handle, int);
extern BOOL (__cdecl* IsUnitPaused)(Handle);
// Containment is distinct from stun/disarm; Devour vision identifies a swallowed victim.
extern BOOL (__cdecl* IsUnitLoaded)(Handle);
extern BOOL (__cdecl* IsUnitHidden)(Handle);
bool HasUnitBuff(Handle unit, int type);
extern BOOL (__cdecl* IsUnitEnemy)(Handle, Handle);
extern BOOL (__cdecl* IsUnitAlly)(Handle, Handle);
extern BOOL (__cdecl* IsUnitVisible)(Handle, Handle);
extern BOOL (__cdecl* IsTerrainPathable)(float*, float*, int);
extern void (__cdecl* SetUnitPosition)(Handle, float*, float*);
// Fixture scale checks exercise the same scaled model bounds as custom map units.
extern void (__cdecl* SetUnitScale)(Handle, float*, float*, float*);
extern void (__cdecl* SetUnitFacing)(Handle, float*);
extern void (__cdecl* SetUnitVertexColor)(Handle, int, int, int, int);
extern BOOL (__cdecl* IssueImmediateOrderById)(Handle, int);
extern BOOL (__cdecl* UnitDamageTarget)(Handle, Handle, float*, BOOL, BOOL, int, int, int);
extern void (__cdecl* SetCameraField)(int, float*, float*);
extern Bits (__cdecl* GetCameraField)(int);
extern Bits (__cdecl* GetCameraEyePositionZ)();
extern Bits (__cdecl* GetCameraEyePositionX)();
extern Bits (__cdecl* GetCameraEyePositionY)();
extern Bits (__cdecl* GetCameraTargetPositionZ)();
extern void (__cdecl* SetCameraPosition)(float*, float*);
extern void (__cdecl* SetCameraBounds)(float*, float*, float*, float*, float*, float*, float*, float*);
extern Bits (__cdecl* GetCameraBoundMinX)();
extern Bits (__cdecl* GetCameraBoundMinY)();
extern Bits (__cdecl* GetCameraBoundMaxX)();
extern Bits (__cdecl* GetCameraBoundMaxY)();
extern Handle (__cdecl* GetWorldBounds)();
extern Bits (__cdecl* GetRectMinX)(Handle);
extern Bits (__cdecl* GetRectMinY)(Handle);
extern Bits (__cdecl* GetRectMaxX)(Handle);
extern Bits (__cdecl* GetRectMaxY)(Handle);
extern void (__cdecl* RemoveRect)(Handle);
extern Handle (__cdecl* CreateDestructable)(int, float*, float*, float*, float*, int);
extern void (__cdecl* RemoveDestructable)(Handle);
extern int (__cdecl* GetDestructableTypeId)(Handle);
extern Bits (__cdecl* GetDestructableX)(Handle);
extern Bits (__cdecl* GetDestructableY)(Handle);
extern Bits (__cdecl* GetDestructableLife)(Handle);
extern BOOL (__cdecl* IsDestructableInvulnerable)(Handle);
extern void (__cdecl* SetCameraTargetController)(Handle, float*, float*, BOOL);
extern void (__cdecl* ResetToGameCamera)(float*);
extern Handle (__cdecl* Location)(float*, float*);
extern Bits (__cdecl* GetLocationZ)(Handle);
extern void (__cdecl* RemoveLocation)(Handle);
extern void (__cdecl* ShowInterface)(BOOL, float*);
}
