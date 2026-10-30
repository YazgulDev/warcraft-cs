#pragma once
#include "../platform/NativeNuclearBomb.hpp"
#include "WorldLabelView.hpp"

// Original procedural geometry needs no downloaded textures, proprietary models or extra installation.
namespace NuclearView {void World(const NativeNuclearBomb& bomb,const WorldLabelView& view,float nearPlane,float farPlane);void Held(int width,int height);}
