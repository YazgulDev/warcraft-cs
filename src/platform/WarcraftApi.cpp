#include "WarcraftApi.hpp"
#include "DiagnosticLog.hpp"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <share.h>

namespace wc3 {
#define NATIVE(result, name, arguments, offset) result (__cdecl* name) arguments = nullptr;
#include "NativeOffsets.inc"
#undef NATIVE
static uintptr_t nativeBase = 0;
static bool cameraLimitsVerified=false;
using SurfaceHeight = float (__fastcall*)(int, BOOL*, float, float, BOOL);
static SurfaceHeight surfaceHeight = nullptr;
using TerrainRoot = uintptr_t (__cdecl*)();
using TerrainHeight = float (__thiscall*)(uintptr_t, const float*, int);
static TerrainRoot terrainRoot = nullptr;
static TerrainHeight terrainHeight = nullptr;
void OpenLog(const char* directory) {
    // Preserve the public native API while the dedicated adapter owns session/file handling.
    DiagnosticLog::Open(directory);
}
void Log(const char* format, ...) {
    va_list args; va_start(args, format); DiagnosticLog::Write("INFO", format, args); va_end(args);
}
void Trace(const char* format, ...) {
    // High-frequency details can be disabled independently of essential events and faults.
    va_list args; va_start(args, format); DiagnosticLog::Write("TRACE", format, args); va_end(args);
}
void LogError(const char* format, ...) {
    va_list args; va_start(args, format); DiagnosticLog::Write("ERROR", format, args); va_end(args);
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
    // The camera getter, generation resolver and scalar clamp must match this owned 1.26a layout.
    const unsigned char cameraGetter[]={0x8B,0x81,0x54,0x02,0x00,0x00,0xC3};
    const unsigned char resolver[]={0x56,0x8B,0x35};
    const unsigned char scalarLimits[]={0x8D,0x96,0x80,0x00,0x00,0x00};
    cameraLimitsVerified=!memcmp(reinterpret_cast<void*>(base+0x2F5ED0),cameraGetter,sizeof(cameraGetter)) &&
        !memcmp(reinterpret_cast<void*>(base+0x3FA30),resolver,sizeof(resolver)) &&
        *reinterpret_cast<uint32_t*>(base+0x3FA33)==base+0xAB7788 &&
        !memcmp(reinterpret_cast<void*>(base+0x4771A6),scalarLimits,sizeof(scalarLimits));
    Log("Native camera far-limit layout verified=%d",cameraLimitsVerified);
#define NATIVE(result, name, arguments, offset) name = reinterpret_cast<decltype(name)>(base + offset);
#include "NativeOffsets.inc"
#undef NATIVE
    Log("Bound Warcraft III 1.26a native API");
    return true;
}
float* FarClipMaximum(uint32_t index,uint32_t serial) {
    if(!cameraLimitsVerified) return nullptr;
    // The native resolver rejects stale generations before the channel layout is accessed.
    using Resolve=uintptr_t (__fastcall*)(uint32_t,uint32_t);
    __try {
        uintptr_t node=reinterpret_cast<Resolve>(nativeBase+0x3FA30)(index,serial);
        if(!node) return nullptr;
        auto limits=reinterpret_cast<float*>(node+0x80);
        if(limits[0]!=100 || !std::isfinite(limits[1]) || limits[1]<100 || limits[1]>20000) return nullptr;
        return limits+1;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return nullptr;}
}
CameraClipChannel FarClipChannel() {
    if(!cameraLimitsVerified) return {};
    using GetUI=uintptr_t (__fastcall*)(int,int);
    __try {
        uintptr_t ui=reinterpret_cast<GetUI>(nativeBase+0x300710)(0,0);
        uintptr_t camera=ui ? *reinterpret_cast<uintptr_t*>(ui+0x254) : 0;
        if(!camera) return {};
        // CAMERA_FIELD_FARZ is the scalar wrapper at +114; +8/+C identify its native channel.
        uint32_t index=*reinterpret_cast<uint32_t*>(camera+0x11C),serial=*reinterpret_cast<uint32_t*>(camera+0x120);
        return {index,serial,FarClipMaximum(index,serial)};
    } __except(EXCEPTION_EXECUTE_HANDLER) {return {};}
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
void Animate(Handle unit,const char* sequence) {
    // Owned 1.26a registration at 3D619B maps SetUnitAnimation (Hunit;S)V to 3C61A0.
    // Its string conversion is synchronous, using the same bounded wrapper as AddSpecialEffect.
    using Animation=void (__cdecl*)(Handle,Handle);
    const unsigned char signature[]={0x8B,0x44,0x24,0x08,0x8B,0x4C,0x24,0x04};
    if(!unit || !nativeBase || std::memcmp(reinterpret_cast<void*>(nativeBase+0x3C61A0),signature,sizeof(signature)))return;
    uintptr_t node[8]={},wrapper[3]={};node[7]=reinterpret_cast<uintptr_t>(sequence);wrapper[2]=reinterpret_cast<uintptr_t>(node);
    reinterpret_cast<Animation>(nativeBase+0x3C61A0)(unit,reinterpret_cast<Handle>(wrapper));
}
int AnimationSequence(Handle unit) {
    // Owned 1.26a sprite getter reads the signed sequence at +2C; its -2 pending sentinel maps to Stand.
    const unsigned char getter[]={0x0F,0xB7,0x41,0x2C,0x66,0x3D,0xFE,0xFF};
    auto object=UnitObject(unit);uintptr_t body=object ? *reinterpret_cast<uintptr_t*>(object+0x28) : 0;
    if(!body || !nativeBase || std::memcmp(reinterpret_cast<void*>(nativeBase+0x4D2FD0),getter,sizeof(getter)))return -1;
    return reinterpret_cast<int (__thiscall*)(uintptr_t)>(nativeBase+0x4D2FD0)(body);
}
float AnimationRate(Handle unit) {
    // Verified SetUnitTimeScale stores the native baseline on CUnit+300 (26DD00).
    auto object=UnitObject(unit);float rate=object ? *reinterpret_cast<float*>(object+0x300) : 1;
    return std::isfinite(rate) && rate>=0 && rate<=10 ? rate : 1;
}
int ActiveAnimationSequence(Handle unit) {
    // Verified 511F10 selects the model controller's active byte at +58, bounded by its sequence count at +0C.
    // A sprite request of -2 is pending, not proof that the model switched to Stand.
    const unsigned char select[]={0x83,0xEC,0x0C,0x8B,0xC2,0x8A,0x54,0x24,0x10};
    if(!unit || !nativeBase || std::memcmp(reinterpret_cast<void*>(nativeBase+0x511F10),select,sizeof(select)))return -1;
    auto object=UnitObject(unit);auto sprite=object ? *reinterpret_cast<uintptr_t*>(object+0x28) : 0;
    auto model=sprite ? *reinterpret_cast<uintptr_t*>(sprite+0x20) : 0;
    auto animation=model ? *reinterpret_cast<uintptr_t*>(model+0x98) : 0;
    if(!animation)return -1;
    unsigned count=*reinterpret_cast<unsigned*>(animation+0x0C);
    unsigned active=*reinterpret_cast<unsigned char*>(animation+0x58);
    return count>0 && count<=256 && active<count ? int(active) : -1;
}
void AnimateIndex(Handle unit,int sequence) {
    // Verified SetUnitAnimationByIndex registration selects the model's exact authored gait/variation.
    const unsigned char signature[]={0x8B,0x4C,0x24,0x04,0xE8};
    if(!unit || sequence<0 || !nativeBase || std::memcmp(reinterpret_cast<void*>(nativeBase+0x3C61F0),signature,sizeof(signature)))return;
    reinterpret_cast<void (__cdecl*)(Handle,int)>(nativeBase+0x3C61F0)(unit,sequence);
}
void AnimationRate(Handle unit,float rate) {
    if(!unit || !nativeBase || !std::isfinite(rate))return;
    const unsigned char native[]={0x8B,0x4C,0x24,0x04,0xE8};
    const unsigned char sprite[]={0x8B,0x01,0xD9,0x44,0x24,0x04};
    if(std::memcmp(reinterpret_cast<void*>(nativeBase+0x3C6300),native,sizeof(native)) ||
        std::memcmp(reinterpret_cast<void*>(nativeBase+0x4D31B0),sprite,sizeof(sprite)))return;
    using Native=void (__cdecl*)(Handle,float*);
    reinterpret_cast<Native>(nativeBase+0x3C6300)(unit,&rate);
    // A movement component defers the JASS setter; update its sprite too, without changing move speed.
    auto object=UnitObject(unit);uintptr_t body=object ? *reinterpret_cast<uintptr_t*>(object+0x28) : 0;
    if(body)reinterpret_cast<void (__thiscall*)(uintptr_t,float)>(nativeBase+0x4D31B0)(body,rate);
}
void AnimationSeek(Handle unit,int sequence,float seconds) {
    // Owned CSprite::SetAnimationTime (4D3010) forwards current sequence + relative milliseconds to this model setter.
    // Use that verified model path without its sprite fallback selecting Stand; native skinning is dirtied normally.
    const unsigned char seek[]={0x55,0x8B,0xE9,0x8B,0x8D,0x98,0x00,0x00,0x00};
    if(!unit || sequence<0 || !nativeBase || !std::isfinite(seconds) || seconds<0 || seconds>60 ||
        std::memcmp(reinterpret_cast<void*>(nativeBase+0x50D560),seek,sizeof(seek)))return;
    auto object=UnitObject(unit);uintptr_t sprite=object ? *reinterpret_cast<uintptr_t*>(object+0x28) : 0;
    uintptr_t model=sprite ? *reinterpret_cast<uintptr_t*>(sprite+0x20) : 0;
    if(!model)return;
    int active=ActiveAnimationSequence(unit);if(active<0)return;
    // Verified CSprite's 4D4093 / 4D9B46 paths select this exact model index before seeking its time.
    // Crowd hit reactions can starve the sprite queue, so select the rendered clip synchronously.
    if(active!=sequence) {
        const unsigned char select[]={0x8B,0x89,0x98,0x00,0x00,0x00,0x85,0xC9};
        if(std::memcmp(reinterpret_cast<void*>(nativeBase+0x50C2E0),select,sizeof(select)))return;
        auto animation=*reinterpret_cast<uintptr_t*>(model+0x98);
        if(!animation || unsigned(sequence)>=*reinterpret_cast<unsigned*>(animation+0x0C))return;
        using Select=BOOL (__fastcall*)(uintptr_t,int,int);
        if(!reinterpret_cast<Select>(nativeBase+0x50C2E0)(model,sequence,8))return;
    }
    // This native setter dirties skinning; it does not enqueue a combat/movement order or write animation memory.
    reinterpret_cast<void (__fastcall*)(uintptr_t,int,int)>(nativeBase+0x50D560)(model,sequence,int(seconds*1000));
}
bool AnimationBlending(Handle unit) {
    // Verified model getter forwards to 513970's native blending-enabled flag; no blend duration is changed.
    const unsigned char getter[]={0x8B,0x89,0x98,0x00,0x00,0x00};
    auto object=UnitObject(unit);auto sprite=object ? *reinterpret_cast<uintptr_t*>(object+0x28) : 0;
    auto model=sprite ? *reinterpret_cast<uintptr_t*>(sprite+0x20) : 0;
    return model && nativeBase && !std::memcmp(reinterpret_cast<void*>(nativeBase+0x501D80),getter,sizeof(getter)) &&
        reinterpret_cast<int (__thiscall*)(uintptr_t)>(nativeBase+0x501D80)(model)!=0;
}
void AnimationBlending(Handle unit,bool enabled) {
    // SetUnitBlendTime's verified sprite enable/disable path (2AB790 -> 4D30B0), without altering stored duration.
    const unsigned char setter[]={0x8B,0x49,0x20,0x85,0xC9};
    if(!unit || !nativeBase || std::memcmp(reinterpret_cast<void*>(nativeBase+0x4D30B0),setter,sizeof(setter)))return;
    auto object=UnitObject(unit);auto sprite=object ? *reinterpret_cast<uintptr_t*>(object+0x28) : 0;
    if(sprite)reinterpret_cast<void (__fastcall*)(uintptr_t,int)>(nativeBase+0x4D30B0)(sprite,enabled);
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
