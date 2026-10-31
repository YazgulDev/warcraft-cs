#pragma once
#include "WarcraftApi.hpp"
#include "../combat/GrenadeProjectile.hpp"
#include <vector>

class NativeEffectService {
public:
    void Configure(uintptr_t base);
    bool Spawn(const char* model,const GrenadePoint& position,float scale,float expires,bool fade=false);
    void Tick(float age);
    void Reset(){items_.clear();} // Map unload owns effect destruction; never dereference recycled handles.
    size_t Count() const{return items_.size();}
private:
    struct Item{wc3::Handle effect;float expires;bool fade;};
    uintptr_t Sprite(wc3::Handle effect) const;
    bool Transform(wc3::Handle effect,const GrenadePoint& position,float scale);
    uintptr_t base_=0;bool verified_=false;std::vector<Item> items_;
};
