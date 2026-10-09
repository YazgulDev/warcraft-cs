#include "UnitHitboxes.hpp"
#include "../geometry/ModelBounds.hpp"
#include <cstring>
#include <string>
#include <vector>

namespace {
template<class T> bool Read(uintptr_t address, T& value) {
    SIZE_T got = 0;
    return address && ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address),
        &value, sizeof(value), &got) && got == sizeof(value);
}
uintptr_t Pointer(uintptr_t address) { uintptr_t result = 0; Read(address, result); return result; }

}
Bounds3 UnitHitboxes::Load(int type, bool building) {
    Bounds3 result;
    if (building) { result = {{-110,-110,0}, {110,110,220}}; }
    // Read the active map's world model, never its separate portrait definition.
    std::string path = wc3::UnitModelPath(type);
    if (!path.empty() && !ModelBounds::Load(path, result)) path += " [fallback]";
    wc3::Log("hitbox type=%08X model=%s min=%.1f,%.1f,%.1f max=%.1f,%.1f,%.1f",
        type, path.c_str(), result.minimum[0],result.minimum[1],result.minimum[2],
        result.maximum[0],result.maximum[1],result.maximum[2]);
    return result;
}
bool UnitHitboxes::Intersect(wc3::Handle unit, const float* origin, const float* direction,
    float limit, float& entry, float padding) {
    int type = wc3::GetUnitTypeId(unit);
    auto found = models_.find(type);
    if (found == models_.end()) found = models_.emplace(type, Load(type, wc3::IsUnitType(unit, 2) != FALSE)).first;
    Bounds3 bounds = found->second;
    using Resolve = uintptr_t(__fastcall*)(wc3::Handle, uintptr_t);
    uintptr_t object = reinterpret_cast<Resolve>(base_ + 0x3BDCB0)(unit, 0), sprite = Pointer(object + 0x28);
    float scale = 1, modelZ = 0;
    Read(sprite + 0xE8, scale);
    if (!std::isfinite(scale) || scale <= 0 || scale > 100) scale = 1;
    float x = wc3::Real(wc3::GetUnitX(unit)), y = wc3::Real(wc3::GetUnitY(unit));
    float z = wc3::Ground(x, y) + wc3::Real(wc3::GetUnitFlyHeight(unit));
    // Use rendered sprite height, which includes flight/bobbing offsets missing from ground-only boxes.
    if (sprite && Read(sprite + 0xC8, modelZ) && std::isfinite(modelZ) && std::abs(modelZ - z) < 1000) z = modelZ;
    float angle = wc3::Real(wc3::GetUnitFacing(unit)) * 0.01745329252f;
    float c = std::cos(angle), s = std::sin(angle), dx = origin[0] - x, dy = origin[1] - y;
    float localOrigin[] = {dx*c + dy*s, -dx*s + dy*c, origin[2] - z};
    float localDirection[] = {direction[0]*c + direction[1]*s, -direction[0]*s + direction[1]*c, direction[2]};
    for (int i = 0; i < 3; ++i) {
        bounds.minimum[i] = bounds.minimum[i] * scale - 6 - padding;
        bounds.maximum[i] = bounds.maximum[i] * scale + 6 + padding;
    }
    return IntersectBounds(bounds, localOrigin, localDirection, limit, entry);
}
