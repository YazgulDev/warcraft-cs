#pragma once
#include "GunMesh.hpp"
#include "WorldLabelView.hpp"
#include "../combat/RocketVisual.hpp"

// Owned rocket body, original procedural smoke/glow and a depth-tested designator share world space.
class RocketView {
public:
    void Draw(const std::string& root,const std::vector<RocketVisual>& items,bool laser,const GrenadePoint& spot,
        const WorldLabelView& view,int width,int height,float nearPlane,float farPlane);
    void Reset(bool deleteTextures);
    bool Valid() const {return (!loaded_ || model_.TexturesValid()) && (!puff_ || glIsTexture(puff_));}
private:
    GunMesh model_;
    bool loaded_=false,attempted_=false;
    GLuint puff_=0;
};
