#pragma once
#include "WarcraftApi.hpp"
#include "../geometry/ModelTransform.hpp"
#include "../geometry/RayBounds.hpp"
#include "../geometry/CylinderMesh.hpp"
#include <map>
#include <memory>

// Native spatial queries feed conservative 3D obstacles; their model cache belongs to the active map.
class MovementObstacles {
public:
    void Configure(uintptr_t base) { base_ = base; }
    void Reset() { models_.clear(); paths_.clear(); meshes_.clear(); worldMeshes_.clear(); details_.clear(); meshAllowed_.clear(); localTriangles_=worldTriangles_=0; }
    std::vector<Bounds3> Snapshot(wc3::Handle actor, float x, float y, float radius,bool nativeUnitCollision=false,bool detailed=false);
    const CylinderMesh* Geometry(size_t index) const {return index<details_.size() ? details_[index].get() : nullptr;}
private:
    bool Bounds(const std::string& path, bool destructable, uintptr_t object, Bounds3& world,std::shared_ptr<const CylinderMesh>* mesh=nullptr);
    struct WorldMesh {ModelTransform transform;std::shared_ptr<CylinderMesh> source;std::shared_ptr<CylinderMesh> geometry;};
    uintptr_t base_ = 0;
    std::map<std::pair<std::string,bool>, Bounds3> models_;
    std::map<std::pair<int,bool>, std::string> paths_;
    // Combat retains its established coarse snapshots. Movement alone opts into static mesh detail.
    std::map<std::pair<std::string,bool>,bool> meshAllowed_;
    std::map<std::pair<std::string,bool>,std::shared_ptr<CylinderMesh>> meshes_;
    std::map<uintptr_t,WorldMesh> worldMeshes_;
    std::vector<std::shared_ptr<const CylinderMesh>> details_;
    size_t localTriangles_=0,worldTriangles_=0;
};
