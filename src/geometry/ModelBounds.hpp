#pragma once
#include "RayBounds.hpp"
#include <string>

// Both unit and destructable hitboxes use the currently mounted map's authored model volumes.
class TreeTrunkMesh;
class CylinderMesh;
namespace ModelBounds {
bool Load(std::string path, Bounds3& bounds, TreeTrunkMesh* tree = nullptr);
bool LoadCollision(std::string path,CylinderMesh& mesh);
}
