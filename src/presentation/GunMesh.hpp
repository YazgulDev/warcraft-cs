#pragma once
#include <windows.h>
#include "GraphicsApi.hpp"
#include <string>
#include <vector>

class GunMesh {
public:
    bool Load(const std::string& path);
    void Draw(const char* animation,float seconds,float flash=0,float flashScale=1,unsigned serial=0,bool geometry=true,const char* idle=nullptr);
    // Hand-free private caches fit the complete weapon in the buy menu's preview rectangle.
    void DrawPreview(float x,float y,float width,float height);
    // World props retain GoldSrc axes and their authored idle pose, centered on the physical projectile.
    void DrawWorld();
    void Reset(bool deleteTextures);
    bool TexturesValid() const;
private:
    void Geometry(const std::vector<float>& bones,bool viewmodel);
    struct Vertex { float x, y, z, u, v; unsigned bone; };
    struct Mesh { unsigned texture; std::vector<Vertex> vertices; };
    struct Sequence { std::string name; float fps; unsigned frames; std::vector<float> bones; };
    struct Muzzle {unsigned bone;float x,y,z;};
    std::vector<Muzzle> muzzles_;
    unsigned lastFlashSerial_=0;
    std::string label_;
    unsigned boneCount_ = 0;
    std::vector<GLuint> textures_;
    std::vector<Mesh> meshes_;
    std::vector<Sequence> sequences_;
};
