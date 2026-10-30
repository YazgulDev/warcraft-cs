#include "../platform/GameBuild.hpp"
#include "UnitHitboxes.hpp"
#include "../geometry/ModelBounds.hpp"
#include "../geometry/RayEllipsoid.hpp"
#include "../platform/SpriteTransform.hpp"
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
    float localOrigin[3],localDirection[3],scale=1;
    LocalRay(unit,origin,direction,localOrigin,localDirection,scale);
    for (int i = 0; i < 3; ++i) {
        bounds.minimum[i] = bounds.minimum[i] * scale - 6 - padding;
        bounds.maximum[i] = bounds.maximum[i] * scale + 6 + padding;
    }
    return IntersectBounds(bounds, localOrigin, localDirection, limit, entry);
}

uintptr_t UnitHitboxes::LocalRay(wc3::Handle unit,const float* origin,const float* direction,float* localOrigin,float* localDirection,float& scale) const {
    using Resolve = uintptr_t(__fastcall*)(wc3::Handle, uintptr_t);
    uintptr_t object = base_ && wc3::Rva(0x3BDCB0) ? reinterpret_cast<Resolve>(base_ + wc3::Rva(0x3BDCB0))(unit, 0) : 0, sprite = Pointer(object ? object + 0x28 : 0);
    scale=1;float modelZ = 0;
    Read(sprite + 0xE8, scale);
    if (!std::isfinite(scale) || scale <= 0 || scale > 100) scale = 1;
    float x = wc3::Real(wc3::GetUnitX(unit)), y = wc3::Real(wc3::GetUnitY(unit));
    float z = wc3::Ground(x, y) + wc3::Real(wc3::GetUnitFlyHeight(unit));
    // Use rendered sprite height, which includes flight/bobbing offsets missing from ground-only boxes.
    if (sprite && Read(sprite + 0xC8, modelZ) && std::isfinite(modelZ) && std::abs(modelZ - z) < 1000) z = modelZ;
    float angle = wc3::Real(wc3::GetUnitFacing(unit)) * 0.01745329252f;
    float c = std::cos(angle), s = std::sin(angle), dx = origin[0] - x, dy = origin[1] - y;
    localOrigin[0]=dx*c+dy*s;localOrigin[1]=-dx*s+dy*c;localOrigin[2]=origin[2]-z;
    localDirection[0]=direction[0]*c+direction[1]*s;localDirection[1]=-direction[0]*s+direction[1]*c;localDirection[2]=direction[2];
    return Pointer(sprite ? sprite+0x20 : 0);
}
bool UnitHitboxes::Pose(uintptr_t model,const AnimatedHitModel& mesh,int& sequence,float& time) const {
    if(!mesh.Animated()){sequence=0;time=float(mesh.Sequences()[0].start);return true;}
    if(!base_ || !model || wc3::ActiveBuild()!=wc3::GameBuildKind::Classic126a)return false;
    // Owned 1.26a SetAnimationTime -> 50D560 -> 512500 selects [controller+8]+sequence*16;
    // 511D90 stores the absolute MDX track time in its first DWORD. Refuse an unverified layout.
    const unsigned char clockSignature[]={0x8B,0x41,0x44,0x83,0x49,0x54,0x20};
    if(std::memcmp(reinterpret_cast<void*>(base_+0x512500),clockSignature,sizeof(clockSignature)))return false;
    uintptr_t animation=Pointer(model+0x98),clocks=Pointer(animation ? animation+8 : 0);unsigned count=0,clock=0;unsigned char active=0;
    if(!animation || !clocks || !Read(animation+0x0C,count) || count!=mesh.Sequences().size() ||
        !Read(animation+0x58,active) || active>=count || !Read(clocks+size_t(active)*16,clock))return false;
    const auto& range=mesh.Sequences()[active];if(clock<range.start || clock>range.end)return false;
    sequence=active;time=float(clock);return true;
}
bool UnitHitboxes::IntersectShot(wc3::Handle unit,const float* origin,const float* direction,float limit,float& entry,bool& head,float headWeight,bool diagnostic,unsigned* headRoot) {
    if(headRoot)*headRoot=0xffffffff;
    head=false;int type=wc3::GetUnitTypeId(unit);if(!type)return false;
    auto found=models_.find(type);bool building=wc3::IsUnitType(unit,2)!=FALSE;
    if(found==models_.end())found=models_.emplace(type,Load(type,building)).first;
    float localOrigin[3],localDirection[3],scale=1;
    auto model=LocalRay(unit,origin,direction,localOrigin,localDirection,scale);
    // Precise shots must follow the rendered basis/position, including tilt and interpolated facing.
    // Keep the existing spell/melee/TPS volume transform unchanged.
    using Resolve=uintptr_t (__fastcall*)(wc3::Handle,uintptr_t);
    uintptr_t object=base_ && wc3::Rva(0x3BDCB0) ? reinterpret_cast<Resolve>(base_+wc3::Rva(0x3BDCB0))(unit,0) : 0;
    uintptr_t sprite=Pointer(object ? object+0x28 : 0);
    ModelTransform transform;
    bool rendered=sprite && SpriteTransform::ReadScaled(sprite,transform) && transform.LocalRay(origin,direction,localOrigin,localDirection);
    if(!rendered)for(int i=0;i<3;++i){localOrigin[i]/=scale;localDirection[i]/=scale;}
    auto mesh=shotModels_.find(type);
    if(mesh==shotModels_.end()) {
        AnimatedHitModel shape;std::vector<unsigned char> bytes;auto path=wc3::UnitModelPath(type);
        if(!path.empty() && ModelBounds::ReadModel(path,bytes))shape.Load(bytes);
        wc3::Log("shot mesh type=%08X model=%s triangles=%zu headVertices=%zu",type,path.c_str(),shape.TriangleCount(),shape.HeadVertices());
        mesh=shotModels_.emplace(type,std::move(shape)).first;
    }
    int sequence=0;float time=0;
    bool posed=mesh->second.Ready() && Pose(model,mesh->second,sequence,time);
    // Actual fired rounds diagnose nearby candidate surfaces, without logging every reticle/frame query.
    auto report=[&](bool hit) {
        float broadEntry=0;
        const auto& broad=mesh->second.Ready() ? mesh->second.BroadBounds() : found->second;
        if(diagnostic && IntersectBounds(broad,localOrigin,localDirection,limit,broadEntry))
            wc3::Log("shot candidate target=%08X type=%08X rendered=%d posed=%d sequence=%d time=%.1f hit=%d head=%d entry=%.1f local=%.2f,%.2f,%.2f ray=%.4f,%.4f,%.4f",
                unit,type,rendered,posed,sequence,time,hit,head,hit ? entry : -1.f,
                localOrigin[0],localOrigin[1],localOrigin[2],localDirection[0],localDirection[1],localDirection[2]);
        return hit;
    };
    if(rendered && posed) {
        AnimatedHitModel::Contact contact;
        if(!mesh->second.Intersect(localOrigin,localDirection,limit,sequence,time,contact,headWeight))return report(false);
        entry=contact.distance;head=contact.head && !building;
        if(headRoot && head)*headRoot=contact.headRoot;
        return report(true);
    }
    // Missing/unsupported model or pose keeps bounded damage with no invented head location.
    // Buildings retain their authored volumes; unknown creatures use an ellipsoid instead of empty box corners.
    return report(building ? IntersectBounds(found->second,localOrigin,localDirection,limit,entry) :
        IntersectEllipsoid(found->second,localOrigin,localDirection,limit,entry));
}
