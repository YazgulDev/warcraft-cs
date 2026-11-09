#pragma once
#include "GunMesh.hpp"
#include "WorldLabelView.hpp"
#include "../combat/GrenadeVisual.hpp"
#include <array>

// Private CS and Half-Life world models share native world projection, depth and context ownership.
class GrenadeWorldView {
public:
    unsigned Draw(const std::string& root,const std::vector<GrenadeVisual>& items,const WorldLabelView& view,int width,int height,float nearPlane,float farPlane);
    void Reset(bool deleteTextures);
    bool Valid() const;
private:
    std::array<GunMesh,5> models_;
    std::array<bool,5> loaded_{},attempted_{};
};
