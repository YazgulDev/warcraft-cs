#pragma once
#include "../platform/WarcraftApi.hpp"
#include <gl/GL.h>
#include <string>

// Render CS art into untouched far-depth pixels; terrain/buildings keep their native depth and textures.
class SkyView {
public:
    void Draw(const std::string& root,const std::string& name,float yaw,float pitch,float fov,float aspect);
    void Reset(bool deleteObject);
    bool Valid() const { return !texture_ || glIsTexture(texture_)==GL_TRUE; }
private:
    bool Load(const std::string& filename);
    GLuint texture_=0;
    std::string name_;
    bool attempted_=false;
};
