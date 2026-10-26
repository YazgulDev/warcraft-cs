#pragma once
#include "RayBounds.hpp"
#include <string>
#include <vector>

// Both unit and destructable hitboxes use the currently mounted map's authored model volumes.
class TreeTrunkMesh;
class CylinderMesh;
namespace ModelBounds {
bool Load(std::string path, Bounds3& bounds, TreeTrunkMesh* tree = nullptr);
bool LoadCollision(std::string path,CylinderMesh& mesh);
// Combat reads its own animated geometry from the same mounted-map precedence as collision.
bool ReadModel(std::string path,std::vector<unsigned char>& bytes);
}
