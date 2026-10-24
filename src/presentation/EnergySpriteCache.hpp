#pragma once
#include "GraphicsApi.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

// Context-owned optional art; a missing cache uses procedural geometry without retrying each frame.
class EnergySpriteCache {
public:
    void Reset(bool deleteObjects) {
        if(deleteObjects)for(auto texture:textures_)if(texture)glDeleteTextures(1,&texture);
        textures_.clear();attempted_=false;
    }
    bool Valid() const {for(auto texture:textures_)if(!glIsTexture(texture))return false;return true;}
    GLuint Frame(const std::string& path,unsigned frame) {
        if(!attempted_){attempted_=true;Load(path);}
        return textures_.empty() ? 0 : textures_[frame%textures_.size()];
    }
private:
    bool attempted_=false;
    std::vector<GLuint> textures_;
    void Load(const std::string& path) {
        std::ifstream file(path,std::ios::binary|std::ios::ate);
        if(!file || file.tellg()<8 || file.tellg()>4*1024*1024)return;
        std::vector<unsigned char> bytes(size_t(file.tellg()));file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
        auto read=[&](size_t at){uint32_t value=0;std::memcpy(&value,bytes.data()+at,4);return value;};
        if(!file || std::memcmp(bytes.data(),"WES1",4))return;
        unsigned count=read(4);if(!count || count>64)return;
        size_t at=8;
        // Validate every frame before allocating GPU objects, so corrupt optional art has an atomic fallback.
        for(unsigned i=0;i<count;++i){if(at+8>bytes.size())return;unsigned w=read(at),h=read(at+4);at+=8;
            if(!w || !h || w>512 || h>512 || at+size_t(w)*h*4>bytes.size())return;at+=size_t(w)*h*4;}
        if(at!=bytes.size())return;
        at=8;glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
        for(unsigned i=0;i<count;++i){unsigned w=read(at),h=read(at+4);at+=8;GLuint texture=0;glGenTextures(1,&texture);
            glBindTexture(GL_TEXTURE_2D,texture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,bytes.data()+at);textures_.push_back(texture);at+=size_t(w)*h*4;}
        glPopClientAttrib();
    }
};
