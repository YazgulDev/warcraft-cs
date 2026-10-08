#pragma once
#include <windows.h>
#include <gl/GL.h>
#include <string>
#include <vector>

class GunMesh {
public:
    bool Load(const std::string& path);
    void Draw(const char* animation, float seconds);
    void Reset(bool deleteTextures);
    bool TexturesValid() const;
private:
    struct Vertex { float x, y, z, u, v; unsigned bone; };
    struct Mesh { unsigned texture; std::vector<Vertex> vertices; };
    struct Sequence { std::string name; float fps; unsigned frames; std::vector<float> bones; };
    unsigned boneCount_ = 0;
    std::vector<GLuint> textures_;
    std::vector<Mesh> meshes_;
    std::vector<Sequence> sequences_;
};
