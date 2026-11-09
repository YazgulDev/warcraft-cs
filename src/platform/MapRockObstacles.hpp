#pragma once
#include "../geometry/CylinderMesh.hpp"
#include "../geometry/RayBounds.hpp"
#include <memory>

// Only pathing-enabled rock doodads supplement the native movement obstacle enumeration.
class MapRockObstacles {
public:
    void Append(float x, float y, float radius, std::vector<Bounds3>& bounds,
        std::vector<std::shared_ptr<const CylinderMesh>>& geometry);
    static float ReachableHeight(const Bounds3& rock, bool grounded, float feet, float reachable) {
        // A tall rock face cannot be climbed by repeatedly applying the ground step allowance.
        return grounded && rock.maximum[2] > reachable + .01f ? feet : reachable;
    }
private:
    struct Rock { Bounds3 bounds; std::shared_ptr<const CylinderMesh> mesh; };
    void Load();
    bool loaded_ = false;
    std::vector<Rock> rocks_;
};
