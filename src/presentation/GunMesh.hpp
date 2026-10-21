#pragma once
#include <windows.h>
#include <gl/GL.h>
#include <string>
#include <vector>
#include <array>

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
    // Beam origins and muzzle flashes sample the same interpolated attachment in held-model GL coordinates.
    bool MuzzlePoint(const char* animation,float seconds,unsigned serial,std::array<float,3>& point,const char* idle=nullptr) const;
private:
    std::vector<float> SamplePose(const char* animation,float seconds,const char* idle) const;
    bool MuzzlePoint(const std::vector<float>& bones,unsigned serial,std::array<float,3>& point) const;
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
