#pragma once
#include "RayBounds.hpp"
#include "RayTriangle.hpp"
#include <string>
#include <vector>

// Tree models retain only tall ground-connected components, excluding foliage and death-only stumps.
class TreeTrunkMesh {
public:
    void Load(const std::string& path, const std::vector<unsigned char>& bytes, const Bounds3& standing);
    bool IsTree() const { return tree_; }
    size_t TriangleCount() const { return triangles_.size(); }
    bool Intersect(const float* origin, const float* direction, float limit, float& entry) const;
private:
    bool tree_ = false;
    std::vector<Triangle3> triangles_;
};
