#include "SkyView.hpp"
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace { constexpr GLenum cube=0x8513;constexpr float rad=.01745329252f; }
void SkyView::Reset(bool deleteObject) {
    // Deleted/replaced contexts invalidate the numeric name without transferring its ownership.
    if (deleteObject && texture_) glDeleteTextures(1,&texture_);
    texture_=0;name_.clear();attempted_=false;
}
bool SkyView::Load(const std::string& filename) {
    std::ifstream file(filename,std::ios::binary);char magic[4];unsigned width=0,height=0;
    if (!file.read(magic,4) || memcmp(magic,"WCS1",4) || !file.read(reinterpret_cast<char*>(&width),4) ||
        !file.read(reinterpret_cast<char*>(&height),4) || !width || width!=height || width>2048) return false;
    const size_t size=size_t(width)*height*4;
    file.seekg(0,std::ios::end);if (file.tellg()!=std::streamoff(12+size*6)) return false;
    file.seekg(12);std::vector<unsigned char> pixels(size);
    glGenTextures(1,&texture_);glBindTexture(cube,texture_);
    glTexParameteri(cube,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(cube,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(cube,GL_TEXTURE_WRAP_S,0x812F);glTexParameteri(cube,GL_TEXTURE_WRAP_T,0x812F);glTexParameteri(cube,0x8072,0x812F);
    // Caches follow GoldSrc rt/lf/bk/ft/up/dn; OpenGL cube targets use X/Y/Z with Y as vertical.
    const unsigned targets[]={0,1,5,4,2,3};
    for (unsigned face=0;face<6;++face) {
        if (!file.read(reinterpret_cast<char*>(pixels.data()),std::streamsize(size))) { Reset(true);return false; }
        // Warcraft can retain unpack stride/skips after Alt-Tab; our tightly packed buffers must never inherit them.
        glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
        glPixelStorei(GL_UNPACK_ALIGNMENT,1);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
        glTexImage2D(0x8515+targets[face],0,GL_RGBA,GLsizei(width),GLsizei(height),0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
        glPopClientAttrib();
    }
    wc3::Log("CS sky loaded %s size=%u",filename.c_str(),width);return true;
}
void SkyView::Draw(const std::string& root,const std::string& name,float yaw,float pitch,float fov,float aspect) {
    // Matrix stacks are private; the caller isolates all GL attributes/texture units from Warcraft.
    if (name_!=name) { Reset(true);name_=name; }
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();
    if (!attempted_) { attempted_=true;Load(root+"\\assets\\skies\\"+name+".wcs"); }
    if (texture_) {
        glDisable(GL_TEXTURE_2D);glEnable(cube);glBindTexture(cube,texture_);glColor4f(1,1,1,1);
        glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_FALSE);glDepthRange(1,1);
        float y=yaw*rad,p=std::clamp(pitch,-89.0f,89.0f)*rad;
        float forward[]={std::cos(p)*std::cos(y),std::cos(p)*std::sin(y),std::sin(p)};
        float right[]={std::sin(y),-std::cos(y),0};
        float up[]={-std::sin(p)*std::cos(y),-std::sin(p)*std::sin(y),std::cos(p)};
        // Warcraft uses a horizontal camera FOV; vertical rays follow the window aspect ratio.
        float horizontal=std::tan(fov*rad*.5f),vertical=horizontal/aspect;
        const float corners[][2]={{-1,-1},{1,-1},{1,1},{-1,1}};
        glBegin(GL_QUADS);
        for (auto& corner:corners) {
            float direction[3];for (int i=0;i<3;++i) direction[i]=forward[i]+right[i]*corner[0]*horizontal+up[i]*corner[1]*vertical;
            // Warcraft's Z-up world must become the cube's Y-up frame, keeping every horizon horizontal.
            glTexCoord3f(direction[0],direction[2],-direction[1]);glVertex3f(corner[0],corner[1],1);
        }
        glEnd();glDisable(cube);glDisable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);glDepthRange(0,1);
    }
    glPopMatrix();glMatrixMode(GL_MODELVIEW);glPopMatrix();
}
