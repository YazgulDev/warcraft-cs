#include "MovementObstacles.hpp"
#include "../geometry/ModelBounds.hpp"
#include "SpriteTransform.hpp"
#include <algorithm>
#include <limits>

bool MovementObstacles::Bounds(const std::string& path, uintptr_t object, Bounds3& world) {
    if (path.empty() || !object) return false;
    auto found = models_.find(path);
    if (found == models_.end()) {
        Bounds3 model;
        // Unknown models never grant permission to bypass a blocked pathing cell.
        if (!ModelBounds::Load(path, model)) model = {{0,0,0},{0,0,0}};
        wc3::Log("movement obstacle model=%s min=%.1f,%.1f,%.1f max=%.1f,%.1f,%.1f",path.c_str(),
            model.minimum[0],model.minimum[1],model.minimum[2],model.maximum[0],model.maximum[1],model.maximum[2]);
        found = models_.emplace(path, model).first;
    }
    const auto& bounds = found->second;
    if (bounds.maximum[0] <= bounds.minimum[0]) return false;
    uintptr_t sprite = 0; SIZE_T got = 0;
    if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(object + 0x28), &sprite, sizeof(sprite), &got)) return false;
    ModelTransform transform;
    if (!SpriteTransform::Read(sprite, transform)) return false;
    uintptr_t table=0,scaleGetter=0;
    if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(sprite),&table,sizeof(table),&got) ||
        !ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(table+0x44),&scaleGetter,sizeof(scaleGetter),&got) || !scaleGetter) return false;
    // CSprite's verified virtual +44 returns uniform visual scale in ST0 (simple-sprite implementation
    // 4D8550 reads +94). The +38 matrix contains orientation separately, so both factors are required.
    using Scale=float (__thiscall*)(uintptr_t);
    float scale=reinterpret_cast<Scale>(scaleGetter)(sprite);
    if(!std::isfinite(scale) || scale<=0 || scale>100) return false;
    for(float& value:transform.matrix) value*=scale;
    for (int axis = 0; axis < 3; ++axis) {
        world.minimum[axis] = std::numeric_limits<float>::max();
        world.maximum[axis] = std::numeric_limits<float>::lowest();
    }
    // Eight transformed corners preserve rotation, authored height and non-uniform scale.
    for (int corner = 0; corner < 8; ++corner) for (int row = 0; row < 3; ++row) {
        float value = transform.position[row];
        for (int col = 0; col < 3; ++col)
            value += transform.matrix[row * 3 + col] * ((corner & (1 << col)) ? bounds.maximum[col] : bounds.minimum[col]);
        if (!std::isfinite(value)) return false;
        world.minimum[row] = std::min(world.minimum[row], value);
        world.maximum[row] = std::max(world.maximum[row], value);
    }
    return true;
}
std::vector<Bounds3> MovementObstacles::Snapshot(wc3::Handle actor, float x, float y, float radius) {
    std::vector<Bounds3> result;
    if (!base_) return result;
    // Native definition lookup is relatively expensive: cache model names by type only until map unload.
    auto path = [&](int type, bool destructable) -> const std::string& {
        auto key = std::make_pair(type, destructable);
        auto found = paths_.find(key);
        if (found == paths_.end()) found = paths_.emplace(key, destructable ? wc3::DestructableModelPath(type) : wc3::UnitModelPath(type)).first;
        return found->second;
    };
    auto group = wc3::CreateGroup();
    wc3::GroupEnumUnitsInRange(group, &x, &y, &radius, 0);
    for (auto unit = wc3::FirstOfGroup(group); unit; unit = wc3::FirstOfGroup(group)) {
        wc3::GroupRemoveUnit(group, unit);
        if (unit == actor || wc3::IsUnitHidden(unit) || wc3::Real(wc3::GetUnitState(unit, 0)) <= 0) continue;
        Bounds3 bounds;
        if (Bounds(path(wc3::GetUnitTypeId(unit), false), wc3::UnitObject(unit), bounds)) result.push_back(bounds);
    }
    wc3::DestroyGroup(group);
    for (auto object : wc3::NearbyDestructables(x, y, radius)) {
        if (wc3::Real(wc3::GetDestructableLife(object)) <= 0) continue;
        using Resolve = uintptr_t (__fastcall*)(wc3::Handle, uintptr_t);
        Bounds3 bounds;
        if (Bounds(path(wc3::GetDestructableTypeId(object), true),
            reinterpret_cast<Resolve>(base_ + 0x3BE010)(object, 0), bounds)) {
            float cx=wc3::Real(wc3::GetDestructableX(object)), cy=wc3::Real(wc3::GetDestructableY(object)), deck=0;
            // Walkable bridges supply their native deck separately; their enclosing model box includes empty space.
            if (!wc3::WalkableSurface(cx, cy, deck)) result.push_back(bounds);
        }
    }
    return result;
}
