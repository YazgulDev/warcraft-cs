#include "NativeEffectService.hpp"
#include "GameBuild.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>
namespace {
template<class T>bool Read(uintptr_t address,T& value){SIZE_T got=0;return address && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),&value,sizeof(value),&got) && got==sizeof(value);}
}
void NativeEffectService::Configure(uintptr_t base){
    base_=base;verified_=false;if(!base || wc3::ActiveBuild()!=wc3::GameBuildKind::Classic126a)return;
    const unsigned char resolve[]={0x6A,0xFF,0x68},mini[]={0xD9,0x44,0x24,0x04,0xD9,0x99,0x94,0,0,0},uber[]={0xD9,0xEE,0x81,0xC1,0xCC,0,0,0};
    verified_=!std::memcmp(reinterpret_cast<void*>(base+0x3C0710),resolve,sizeof(resolve)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0x4D8450),mini,sizeof(mini)) && !std::memcmp(reinterpret_cast<void*>(base+0x4D8B30),uber,sizeof(uber));
    wc3::Log("Nuclear native effect transform verified=%d",verified_);
}
uintptr_t NativeEffectService::Sprite(wc3::Handle effect) const{
    if(!verified_ || !effect)return 0;using Resolve=uintptr_t (__fastcall*)(wc3::Handle,uintptr_t);
    auto object=reinterpret_cast<Resolve>(base_+0x3C0710)(effect,0);uintptr_t table=0,sprite=0,spriteTable=0;
    // AddSpecialEffect creates CEffectImagePos; accept only its verified sprite classes and virtual ABI.
    if(!Read(object,table) || table!=base_+0x8786CC || !Read(object+0x28,sprite) || !Read(sprite,spriteTable))return 0;
    // Native effects allocate the handle-owning CSpriteUber leaf; it inherits the verified transform ABI.
    if(spriteTable!=base_+0x954CD4 && spriteTable!=base_+0x954D4C && spriteTable!=base_+0x954E6C)return 0;
    uintptr_t setScale=0,getScale=0;
    if(!Read(spriteTable+0x24,setScale) || !Read(spriteTable+0x44,getScale))return 0;
    return (setScale==base_+0x4D8450 && getScale==base_+0x4D8550) ||
        (setScale==base_+0x4D8B30 && getScale==base_+0x4D8C40) ? sprite : 0;
}
bool NativeEffectService::Transform(wc3::Handle effect,const GrenadePoint& position,float scale){
    auto sprite=Sprite(effect);if(!sprite)return false;auto table=*reinterpret_cast<uintptr_t*>(sprite);
    using Position=void (__thiscall*)(uintptr_t,const float*);using Scale=void (__thiscall*)(uintptr_t,float);using GetScale=float (__thiscall*)(uintptr_t);
    reinterpret_cast<Position>(*reinterpret_cast<uintptr_t*>(table+0x1C))(sprite,position.data());
    reinterpret_cast<Scale>(*reinterpret_cast<uintptr_t*>(table+0x24))(sprite,scale);
    float actual=reinterpret_cast<GetScale>(*reinterpret_cast<uintptr_t*>(table+0x44))(sprite);
    wc3::Log("Nuclear effect scale requested=%.2f actual=%.2f sprite=%08X",scale,actual,unsigned(sprite));return std::isfinite(actual) && std::abs(actual-scale)<.01f;
}
bool NativeEffectService::Spawn(const char* model,const GrenadePoint& position,float scale,float expires,bool fade){
    if(items_.size()>=64)return false;auto effect=wc3::Effect(model,position[0],position[1]);if(!effect){wc3::LogError("Nuclear effect allocation failed model=%s",model);return false;}
    bool transformed=Transform(effect,position,scale);items_.push_back({effect,expires,fade});
    wc3::Log("Nuclear effect created model=%s scale=%.2f transformed=%d expires=%.2f",model,scale,transformed,expires);return transformed;
}
void NativeEffectService::Tick(float age){
    for(auto it=items_.begin();it!=items_.end();){
        if(age>=it->expires){wc3::DestroyEffect(it->effect);it=items_.erase(it);continue;}
        // Fade only our smoke sprites before releasing their native emitters; no map-owned art is changed.
        if(it->fade && it->expires-age<2){auto sprite=Sprite(it->effect);if(sprite){auto table=*reinterpret_cast<uintptr_t*>(sprite);using Color=void (__thiscall*)(uintptr_t,unsigned);
            unsigned alpha=unsigned(std::clamp((it->expires-age)/2,0.f,1.f)*255);reinterpret_cast<Color>(*reinterpret_cast<uintptr_t*>(table+0x20))(sprite,(alpha<<24)|0xFFFFFF);}}
        ++it;
    }
}
