#pragma once
#include "WarcraftApi.hpp"
#include "ModelTransform.hpp"

// Warcraft's simple destructable sprites and animated unit sprites have different member layouts.
namespace SpriteTransform { bool Read(uintptr_t sprite, ModelTransform& transform); }
