#include "WarcraftApi.hpp"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <share.h>

namespace wc3 {
#define NATIVE(result, name, arguments, offset) result (__cdecl* name) arguments = nullptr;
#include "NativeOffsets.inc"
#undef NATIVE
static FILE* logFile = nullptr;
static uintptr_t nativeBase = 0;
using SurfaceHeight = float (__fastcall*)(int, BOOL*, float, float, BOOL);
static SurfaceHeight surfaceHeight = nullptr;
using TerrainRoot = uintptr_t (__cdecl*)();
using TerrainHeight = float (__thiscall*)(uintptr_t, const float*, int);
static TerrainRoot terrainRoot = nullptr;
static TerrainHeight terrainHeight = nullptr;
void OpenLog(const char* directory) {
    char path[MAX_PATH];
    sprintf_s(path, "%s\\WarcraftCS.log", directory);
    // Diagnostics must remain readable while the game is running.
    logFile = _fsopen(path, "w", _SH_DENYNO);
}
void Log(const char* format, ...) {
    if (!logFile) return;
    fprintf(logFile, "[%lu] ", GetTickCount());
    va_list args; va_start(args, format); vfprintf(logFile, format, args); va_end(args);
    fputc('\n', logFile); fflush(logFile);
}
float Real(Bits bits) { float value; memcpy(&value, &bits, 4); return value; }
bool Bind(HMODULE game) {
    // Refuse other builds rather than executing unverified function addresses.
    char path[MAX_PATH]; GetModuleFileNameA(game, path, MAX_PATH);
    DWORD ignored = 0, size = GetFileVersionInfoSizeA(path, &ignored);
    if (!size) return false;
    auto bytes = new unsigned char[size];
    VS_FIXEDFILEINFO* info = nullptr; UINT length = 0;
    bool correct = GetFileVersionInfoA(path, 0, size, bytes) &&
        VerQueryValueA(bytes, "\\", reinterpret_cast<void**>(&info), &length) &&
        info && info->dwFileVersionMS == MAKELONG(26, 1) && info->dwFileVersionLS == MAKELONG(6401, 0);
    delete[] bytes;
    if (!correct) { Log("Unsupported Game.dll: expected 1.26.0.6401"); return false; }
    auto base = reinterpret_cast<uintptr_t>(game);
    // Verified 1.26a GetLocationZ helper: ECX=terrain mode, EDX=raised-surface output,
    // stack=x/y/include-walkables, ST0=height. Refuse a patched helper instead of guessing its ABI.
    const unsigned char surfaceSignature[] = {0x83,0xEC,0x0C,0xD9,0x44,0x24,0x10,0x53,0xD9,0x5C,0x24,0x04};
    if (memcmp(reinterpret_cast<void*>(base+0x126F0),surfaceSignature,sizeof(surfaceSignature)) != 0) {
        Log("Unsupported native walkable-surface helper"); return false;
    }
    surfaceHeight = reinterpret_cast<SurfaceHeight>(base+0x126F0);
    // 126F0 calls 1F5A0 for the terrain object, then 763F00 with position/mode and an ST0 result.
    const unsigned char terrainSignature[]={0x8B,0x44,0x24,0x08,0x83,0xF8,0xFD};
    if(memcmp(reinterpret_cast<void*>(base+0x763F00),terrainSignature,sizeof(terrainSignature))) {
        Log("Unsupported native terrain-height helper");return false;
    }
    terrainRoot=reinterpret_cast<TerrainRoot>(base+0x1F5A0);
    terrainHeight=reinterpret_cast<TerrainHeight>(base+0x763F00);
    nativeBase = base;
#define NATIVE(result, name, arguments, offset) name = reinterpret_cast<decltype(name)>(base + offset);
#include "NativeOffsets.inc"
#undef NATIVE
    Log("Bound Warcraft III 1.26a native API");
    return true;
}
float Ground(float x, float y) {
    Handle location = Location(&x, &y);
    float z = Real(GetLocationZ(location));
    RemoveLocation(location);
    return z;
}
bool WalkableSurface(float x, float y, float& height) {
    if (!surfaceHeight || !std::isfinite(x) || !std::isfinite(y)) return false;
    BOOL raised = FALSE;
    float z = surfaceHeight(-1,&raised,x,y,TRUE);
    if (!raised || !std::isfinite(z)) return false;
    height = z; return true;
}
float TerrainGround(float x, float y) {
    // Query the same interpolated terrain as mode -1, before 126F0 adds walkable object heights.
    // Its FALSE flag cannot exclude objects with mode -1; mode -3 uses a different raw height grid.
    if (!terrainRoot || !terrainHeight) return Ground(x,y);
    float position[]={x,y,0};
    return terrainHeight(terrainRoot(),position,-1);
}
std::string UnitModelPath(int type) {
    auto read = [](uintptr_t address, void* value, size_t size) {
        SIZE_T got = 0;
        return address && ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address), value, size, &got) && got == size;
    };
    auto pointer = [&](uintptr_t address) { uintptr_t value = 0; read(address, &value, 4); return value; };
    // 1.26a's active-map definition list: rawcode at node +8, world path at definition +0x30.
    uintptr_t head = pointer(nativeBase + 0xAB58F0), node = pointer(head + 8) + 0x10, first = node;
    for (int i = 0; head && node && i < 8192; ++i) {
        int id = 0; if (!read(node + 8, &id, 4)) break;
        if (id == type) {
            uintptr_t string = pointer(node - 0xC + 0x30); std::string path;
            for (int j = 0; string && j < 512; ++j) {
                char c = 0; if (!read(string + j, &c, 1) || !c) break; path += c;
            }
            return path.substr(0, path.find(','));
        }
        node = pointer(node); if (node == first) break;
    }
    return {};
}
namespace {
struct DestructableEnumeration { uintptr_t environment; std::vector<Handle> result; };
BOOL __fastcall CollectDestructable(uintptr_t object, DestructableEnumeration* context) {
    using ClassTag = int (__thiscall*)(uintptr_t);
    auto table = *reinterpret_cast<uintptr_t*>(object);
    if (reinterpret_cast<ClassTag>(*reinterpret_cast<uintptr_t*>(table + 0x1C))(object) != 0x2B773364) return TRUE;
    // This is the same native widget-to-handle registration as CreateDestructable, using its current map environment.
    using ToHandle = Handle (__thiscall*)(uintptr_t, uintptr_t, BOOL);
    Handle handle = reinterpret_cast<ToHandle>(nativeBase + 0x430C80)(context->environment, object, FALSE);
    if (handle) context->result.push_back(handle);
    return TRUE;
}
}
std::vector<Handle> NearbyDestructables(float x, float y, float radius) {
    using Environment = uintptr_t (__thiscall*)(uintptr_t);
    uintptr_t root = *reinterpret_cast<uintptr_t*>(nativeBase + 0xAB65F4);
    if (!root) return {};
    DestructableEnumeration context{reinterpret_cast<Environment>(nativeBase + 0x3A8060)(root), {}};
    if (!context.environment) return {};
    // EnumDestructablesInRect's synchronous spatial backend accepts a native callback, unlike its JASS-code parameter.
    using Enumerate = void (__fastcall*)(const float*, unsigned, uintptr_t, uintptr_t,
        BOOL (__fastcall*)(uintptr_t, DestructableEnumeration*), DestructableEnumeration*);
    // CRect stores Y before X; its getters prove the native order differs from a usual XY rectangle.
    float rectangle[] = {y-radius, x-radius, y+radius, x+radius};
    reinterpret_cast<Enumerate>(nativeBase + 0x4693D0)(rectangle, 0x04FB0000, 0, 0, CollectDestructable, &context);
    return context.result;
}
namespace {
BOOL __fastcall CollectItem(uintptr_t object,DestructableEnumeration* context) {
    using ClassTag=int (__thiscall*)(uintptr_t);
    auto table=*reinterpret_cast<uintptr_t*>(object);
    if (reinterpret_cast<ClassTag>(*reinterpret_cast<uintptr_t*>(table+0x1C))(object)!=0x6974656D) return TRUE;
    using ToHandle=Handle (__thiscall*)(uintptr_t,uintptr_t,BOOL);
    Handle item=reinterpret_cast<ToHandle>(nativeBase+0x430C80)(context->environment,object,FALSE);
    if (item) context->result.push_back(item);return TRUE;
}
}
std::vector<Handle> NearbyItems(float x,float y,float radius) {
    using Environment=uintptr_t (__thiscall*)(uintptr_t);
    uintptr_t root=*reinterpret_cast<uintptr_t*>(nativeBase+0xAB65F4);if (!root) return {};
    DestructableEnumeration context{reinterpret_cast<Environment>(nativeBase+0x3A8060)(root),{}};
    if (!context.environment) return {};
    // EnumItemsInRect uses this same synchronous spatial backend with item mask 0x04F70000.
    // Pass a native callback, not a C++ address disguised as JASS bytecode; CRect is Y/X ordered.
    using Enumerate=void (__fastcall*)(const float*,unsigned,uintptr_t,uintptr_t,
        BOOL (__fastcall*)(uintptr_t,DestructableEnumeration*),DestructableEnumeration*);
    float rectangle[]={y-radius,x-radius,y+radius,x+radius};
    reinterpret_cast<Enumerate>(nativeBase+0x4693D0)(rectangle,0x04F70000,0,0,CollectItem,&context);
    return context.result;
}
uintptr_t UnitObject(Handle unit) {
    // Resolve current-map handles instead of retaining pointers through removal or campaign transitions.
    using Resolve=uintptr_t (__fastcall*)(Handle,uintptr_t);
    return unit ? reinterpret_cast<Resolve>(nativeBase+0x3BDCB0)(unit,0) : 0;
}
float UnitAttackAverage(Handle unit) {
    uintptr_t object=UnitObject(unit);
    uintptr_t attack=object ? *reinterpret_cast<uintptr_t*>(object+0x1E8) : 0;
    if (!attack) return 0;
    // Verified 1.26a getters: base+bonus+dice and base+bonus+dice*sides, primary attack index zero.
    using Damage=int (__thiscall*)(uintptr_t,int);
    int low=reinterpret_cast<Damage>(nativeBase+0xC6CB0)(attack,0);
    int high=reinterpret_cast<Damage>(nativeBase+0xC6CD0)(attack,0);
    return (float(low)+float(high))*.5f;
}
std::string DestructableModelPath(int type) {
    // Use CreateDestructable's active-definition filename builder, including directory and variation suffixes.
    // +0x268A50 is its replacement texture slot, not the model; single-variation gates use variation zero.
    using Path = void (__fastcall*)(char*, unsigned, int, int, BOOL, BOOL);
    char path[MAX_PATH] = {};
    reinterpret_cast<Path>(nativeBase + 0x268E20)(path,MAX_PATH,type,0,FALSE,FALSE);
    return path;
}
float DestructableFacing(Handle handle) {
    using Resolve = uintptr_t (__fastcall*)(Handle, uintptr_t);
    using Position = uintptr_t (__thiscall*)(uintptr_t);
    using Angle = uintptr_t (__thiscall*)(uintptr_t, float*);
    uintptr_t object = reinterpret_cast<Resolve>(nativeBase + 0x3BE010)(handle, 0);
    if (!object) return 0;
    // Follow GetUnitFacing's transform getter directly: the shared widget transform supplies radians.
    uintptr_t table = *reinterpret_cast<uintptr_t*>(object);
    uintptr_t position = reinterpret_cast<Position>(*reinterpret_cast<uintptr_t*>(table + 0xB8))(object);
    if (!position) return 0;
    table = *reinterpret_cast<uintptr_t*>(position);
    float angle = 0; reinterpret_cast<Angle>(*reinterpret_cast<uintptr_t*>(table + 0x1C))(position, &angle);
    return angle;
}
Handle Effect(const char* model, float x, float y) {
    // Native string getter +0x4C4630 reads only wrapper+8 -> node+0x1C -> UTF-8 bytes.
    // The stack wrapper lives through the synchronous model load; it never enters the JASS string table.
    uintptr_t node[8] = {}; node[7] = reinterpret_cast<uintptr_t>(model);
    uintptr_t wrapper[3] = {}; wrapper[2] = reinterpret_cast<uintptr_t>(node);
    return AddSpecialEffect(reinterpret_cast<Handle>(wrapper), &x, &y);
}
bool HasUnitBuff(Handle unit, int type) {
    // Native buff objects can have level -1: GetUnitAbilityLevel returning zero does not mean absent.
    // Use the same read-only alias lookup and inclusion flags as 1.26a's UnitRemoveAbility, without removing it.
    using Resolve = uintptr_t (__fastcall*)(Handle, uintptr_t);
    using Find = uintptr_t (__thiscall*)(uintptr_t, int, BOOL, BOOL, BOOL, BOOL);
    uintptr_t object = reinterpret_cast<Resolve>(nativeBase + 0x3BDCB0)(unit, 0);
    return object && reinterpret_cast<Find>(nativeBase + 0x787D0)(object, type, FALSE, TRUE, TRUE, TRUE) != 0;
}
bool IsNeutralUnit(Handle unit) {
    Handle owner = GetOwningPlayer(unit);
    // Owner identity is independent of alliance changes, so passive critters and neutral structures qualify too.
    return owner == Player(12) || owner == Player(13) || owner == Player(14) || owner == Player(15);
}
bool SinglePlayer() {
    // Locally polled input changes simulation, so reject maps with multiple human slots.
    int humans = 0;
    for (int slot = 0; slot < 12; ++slot) {
        Handle player = Player(slot);
        if (GetPlayerController(player) == 0 && GetPlayerSlotState(player) == 1) ++humans;
    }
    return humans == 1;
}
Handle PickOwnedUnit() {
    Handle local = GetLocalPlayer(), group = CreateGroup(), selected = 0, fallback = 0;
    GroupEnumUnitsSelected(group, local, 0);
    Handle unit = FirstOfGroup(group);
    if (unit && GetUnitTypeId(unit) && Real(GetUnitState(unit, 0)) > 0.405f &&
        GetOwningPlayer(unit) == local && !IsUnitType(unit, 2) && !IsUnitLoaded(unit) && !IsUnitHidden(unit)) selected = unit;
    DestroyGroup(group);
    if (selected) return selected;
    group = CreateGroup(); GroupEnumUnitsOfPlayer(group, local, 0);
    while ((unit = FirstOfGroup(group)) != 0) {
        GroupRemoveUnit(group, unit);
        // Swallowed/transported or map-hidden units cannot be controlled from the world.
        if (Real(GetUnitState(unit, 0)) <= 0.405f || IsUnitType(unit, 2) || IsUnitLoaded(unit) || IsUnitHidden(unit)) continue;
        if (!fallback) fallback = unit;
        if (IsUnitType(unit, 0)) { fallback = unit; break; }
    }
    DestroyGroup(group);
    return fallback;
}
}
