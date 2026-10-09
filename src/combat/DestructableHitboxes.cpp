#include "DestructableHitboxes.hpp"
#include "../geometry/ModelBounds.hpp"
#include "../platform/SpriteTransform.hpp"

namespace {
template<class T> bool Read(uintptr_t at, T& value) {
    SIZE_T got=0;
    return at && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(at),&value,sizeof(value),&got) && got==sizeof(value);
}
}
bool DestructableHitboxes::Intersect(wc3::Handle target,const float* origin,const float* direction,float limit,float& entry) {
    int type=wc3::GetDestructableTypeId(target);
    auto found=models_.find(type);
    if (found==models_.end()) {
        Shape shape;
        Bounds3& bounds=shape.bounds;
        std::string path=wc3::DestructableModelPath(type);
        bool loaded=ModelBounds::Load(path,bounds,&shape.trunk);
        // Missing geometry must not create an invisible broad blocker in front of ordinary enemies.
        if (!loaded) bounds={{0,0,0},{0,0,0}};
        wc3::Log("destructable hitbox type=%08X model=%s loaded=%d min=%.1f,%.1f,%.1f max=%.1f,%.1f,%.1f",type,path.c_str(),loaded,
            bounds.minimum[0],bounds.minimum[1],bounds.minimum[2],bounds.maximum[0],bounds.maximum[1],bounds.maximum[2]);
        wc3::Log("destructable trunk tree=%d triangles=%u",shape.trunk.IsTree(),unsigned(shape.trunk.TriangleCount()));
        found=models_.emplace(type,std::move(shape)).first;
    }
    Bounds3 bounds=found->second.bounds;
    if (bounds.maximum[0]<=bounds.minimum[0]) return false;
    using Resolve=uintptr_t (__fastcall*)(wc3::Handle,uintptr_t);
    uintptr_t object=reinterpret_cast<Resolve>(base_+0x3BE010)(target,0),sprite=0;
    Read(object+0x28,sprite);
    ModelTransform transform;
    float localOrigin[3],localDirection[3];
    // Gates use simple sprites: the unit-only E8/C8 offsets used to discard their authored scale and height.
    if (!SpriteTransform::Read(sprite,transform)||!transform.LocalRay(origin,direction,localOrigin,localDirection)) return false;
    // A tree canopy's empty box cannot steal a shot from an enemy beside its actual trunk.
    if (found->second.trunk.IsTree()) return found->second.trunk.Intersect(localOrigin,localDirection,limit,entry);
    for(int i=0;i<3;++i) { bounds.minimum[i]-=4; bounds.maximum[i]+=4; }
    return IntersectBounds(bounds,localOrigin,localDirection,limit,entry);
}
