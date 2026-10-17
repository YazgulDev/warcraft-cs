#include "HudIconCache.hpp"
#include "../platform/NativeArchive.hpp"
#include "../platform/SpellIconImage.hpp"
#include "../platform/WarcraftApi.hpp"
void HudIconCache::Reset(bool deleteObjects) {
    // Map imports can reuse a path with different pixels; contexts can reuse numeric texture IDs.
    if(deleteObjects)for(const auto& icon:icons_)if(icon.second)glDeleteTextures(1,&icon.second);
    icons_.clear();
}
bool HudIconCache::Valid() const {
    for(const auto& icon:icons_)if(icon.second && !glIsTexture(icon.second))return false;return true;
}
GLuint HudIconCache::Texture(const std::string& path) {
    if(path.empty())return 0;auto found=icons_.find(path);if(found!=icons_.end())return found->second;
    if(icons_.size()>=512)return 0;SpellIconImage image;GLuint texture=0;
    // Failed/missing optional art is cached too, so archive reads cannot stall every render frame.
    if(image.Decode(NativeArchive::Read(path))) {
        glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812F);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812F);
        glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,image.width,image.height,0,GL_RGBA,GL_UNSIGNED_BYTE,image.pixels.data());glPopClientAttrib();
    }
    wc3::Log("Spell icon loaded=%d size=%ux%u path=%s",texture!=0,image.width,image.height,path.c_str());
    icons_[path]=texture;return texture;
}
