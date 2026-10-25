#pragma once
#include "../src/geometry/CylinderMesh.hpp"
#include "../src/geometry/RayBounds.hpp"

// Owned synthetic closed boxes compose arches, column bases and ceilings without retail assets.
inline void AddMeshBox(CylinderMesh& mesh,const Bounds3& box) {
    float points[8][3];
    for(unsigned i=0;i<8;++i)for(unsigned axis=0;axis<3;++axis)
        points[i][axis]=(i&(1<<axis)) ? box.maximum[axis] : box.minimum[axis];
    const unsigned faces[][3]={{0,2,3},{0,3,1},{4,5,7},{4,7,6},
        {0,1,5},{0,5,4},{2,6,7},{2,7,3},{0,4,6},{0,6,2},{1,3,7},{1,7,5}};
    for(const auto& face:faces) {
        Triangle3 triangle;
        for(int vertex=0;vertex<3;++vertex)for(int axis=0;axis<3;++axis)
            triangle.vertices[vertex][axis]=points[face[vertex]][axis];
        mesh.triangles.push_back(triangle);
    }
}
