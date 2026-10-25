#pragma once
#include "GraphicsApi.hpp"
#include <string>

// One cached Unicode caption per graphics context; the existing ASCII HUD atlases remain unchanged.
class ItemNameView {
public:
    void Draw(const std::string& name,int width,int height,int fontSize);
    // Reuse the Unicode atlas for a centered world caption; nonnegative window depth enables native occlusion.
    void DrawAt(const std::string& name,int width,int height,int fontSize,float centerX,float top,float alpha,float depth=-1);
    void Reset(bool deleteObjects);
    bool Valid() const {return !texture_ || glIsTexture(texture_)==GL_TRUE;}
private:
    bool Build(const std::string& name,int fontSize,int maximumWidth);
    std::string name_;
    GLuint texture_=0;
    int fontSize_=0,maximumWidth_=0,width_=0,height_=0,textureWidth_=0,textureHeight_=0;
};
