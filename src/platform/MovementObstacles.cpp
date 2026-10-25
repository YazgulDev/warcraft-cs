#include "GameBuild.hpp"
#include "MovementObstacles.hpp"
#include "../geometry/ModelBounds.hpp"
#include "../geometry/TreeTrunkMesh.hpp"
#include "SpriteTransform.hpp"
#include "UnitCollisionProfile.hpp"
#include <algorithm>
#include <limits>
#include <cstring>

bool MovementObstacles::Bounds(const std::string& path, bool destructable, uintptr_t object, Bounds3& world,std::shared_ptr<const CylinderMesh>* mesh) {
    if (path.empty() || !object) return false;
    auto key=std::make_pair(path,destructable);
    auto found = models_.find(key);
    if (found == models_.end()) {
        Bounds3 model;
        // Unknown models never grant permission to bypass a blocked pathing cell.
        TreeTrunkMesh trunk;
        // Unit models may also contain EatTree animations; only destructables use standing-trunk rules.
        bool loaded=ModelBounds::Load(path, model, destructable ? &trunk : nullptr);
        meshAllowed_[key]=loaded && destructable && !trunk.IsTree();
        // A standing tree's visual extents include empty canopy space. Decode its trunk before applying
        // the existing native sprite transform; unreadable trees retain native pathing as the fallback.
        if (!loaded || (trunk.IsTree() && !trunk.Bounds(model))) model = {{0,0,0},{0,0,0}};
        wc3::Log("movement obstacle model=%s min=%.1f,%.1f,%.1f max=%.1f,%.1f,%.1f tree=%d trunkTriangles=%u",path.c_str(),
            model.minimum[0],model.minimum[1],model.minimum[2],model.maximum[0],model.maximum[1],model.maximum[2],
            trunk.IsTree(),unsigned(trunk.TriangleCount()));
        found = models_.emplace(key, model).first;
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
    if(mesh && meshAllowed_[key]) {
        auto local=meshes_.find(key);
        if(local==meshes_.end()) {
            // Bound triangle memory as well as object count in the 32-bit host. Eviction
            // reloads the same model on demand and never changes its collision policy.
            if(localTriangles_>500000){meshes_.clear();worldMeshes_.clear();localTriangles_=worldTriangles_=0;}
            auto geometry=std::make_shared<CylinderMesh>();
            if(!ModelBounds::LoadCollision(path,*geometry))geometry.reset();
            if(geometry)localTriangles_+=geometry->triangles.size();
            wc3::Log("movement static mesh model=%s triangles=%u",path.c_str(),geometry ? unsigned(geometry->triangles.size()) : 0);
            local=meshes_.emplace(key,std::move(geometry)).first;
        }
        if(local->second) {
            // Cache transformed vertices until this object's pose/scale changes. Bound retained instances
            // during long maps; eviction changes only work performed, never collision geometry.
            if(worldMeshes_.size()>1024 || worldTriangles_>500000){worldMeshes_.clear();worldTriangles_=0;}
            auto& cached=worldMeshes_[object];
            if(cached.source!=local->second || !cached.geometry ||
                std::memcmp(cached.transform.matrix,transform.matrix,sizeof(transform.matrix)) ||
                std::memcmp(cached.transform.position,transform.position,sizeof(transform.position))) {
                if(cached.geometry)worldTriangles_-=cached.geometry->triangles.size();
                cached={transform,local->second,std::make_shared<CylinderMesh>(local->second->Transformed(transform))};
                worldTriangles_+=cached.geometry->triangles.size();
            }
            *mesh=cached.geometry;
        }
    }
    return true;
}
std::vector<Bounds3> MovementObstacles::Snapshot(wc3::Handle actor, float x, float y, float radius,bool nativeUnitCollision,bool detailed) {
    std::vector<Bounds3> result;
    details_.clear();
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
        if (unit == actor || wc3::IsUnitHidden(unit) || wc3::IsUnitLoaded(unit) || wc3::Real(wc3::GetUnitState(unit, 0)) <= 0) continue;
        auto collision=nativeUnitCollision ? UnitCollisionProfile::Read(unit,base_) : UnitCollisionProfile{};
        if(collision.known && !collision.colliding)continue;
        Bounds3 bounds;
        if (Bounds(path(wc3::GetUnitTypeId(unit), false), false, wc3::UnitObject(unit), bounds)) {
            // Weapons/capes do not make a narrow native ramp impassable. Buildings retain their solid model volume.
            if(collision.known && collision.colliding && !wc3::IsUnitType(unit,2)) {
                float cx=wc3::Real(wc3::GetUnitX(unit)),cy=wc3::Real(wc3::GetUnitY(unit));
                bounds.minimum[0]=cx-collision.radius;bounds.maximum[0]=cx+collision.radius;
                bounds.minimum[1]=cy-collision.radius;bounds.maximum[1]=cy+collision.radius;
            }
            result.push_back(bounds);
            details_.push_back(nullptr);
        }
    }
    wc3::DestroyGroup(group);
    for (auto object : wc3::NearbyDestructables(x, y, radius)) {
        if (wc3::Real(wc3::GetDestructableLife(object)) <= 0) continue;
        using Resolve = uintptr_t (__fastcall*)(wc3::Handle, uintptr_t);
        Bounds3 bounds;
        std::shared_ptr<const CylinderMesh> mesh;
        if (Bounds(path(wc3::GetDestructableTypeId(object), true), true,
            reinterpret_cast<Resolve>(base_ + wc3::Rva(0x3BE010))(object, 0), bounds,detailed ? &mesh : nullptr)) {
            float cx=wc3::Real(wc3::GetDestructableX(object)), cy=wc3::Real(wc3::GetDestructableY(object)), deck=0;
            // Keep verified surfaces for movement even when the native deck is present: a missed
            // deck query must not delete a real platform. Never substitute an enclosing bridge box.
            if (!wc3::WalkableSurface(cx, cy, deck) || (detailed && mesh)) {result.push_back(bounds);details_.push_back(std::move(mesh));}
        }
    }
    return result;
}
