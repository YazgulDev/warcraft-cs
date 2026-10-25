#pragma once
#include "CombatVisuals.hpp"
#include "WorldLabelView.hpp"
#include "ItemNameView.hpp"

// Owned procedural debris and a cached Unicode critical caption; no game art or native handles are retained.
class CombatVisualView {
public:
    void Draw(const CombatVisuals& effects,const WorldLabelView& view,int width,int height,float nearPlane,float farPlane);
    void Reset(bool deleteObjects){caption_.Reset(deleteObjects);}
    bool Valid() const{return caption_.Valid();}
private:
    ItemNameView caption_;
};
