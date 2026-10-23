#include "D3DGraphics.hpp"
#include "D3DGraphicsState.hpp"
#include <algorithm>

namespace D3DGraphics {
namespace detail {
void Delete(GLsizei count,const GLuint* names) {
    auto& s=Current();for(GLsizei i=0;i<count;++i) {
        auto entry=s.textures.find(names[i]);if(entry==s.textures.end())continue;
        if(entry->second.object)entry->second.object->Release();s.textures.erase(entry);
    }
}
void Upload(GLenum target,GLint level,GLint,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void* pixels) {
    auto& s=Current();bool cube=target>=0x8515 && target<=0x851A;
    auto entry=s.Bound(cube ? 0x8513 : target);
    if(!entry || level || border || width<=0 || height<=0 || width>8192 || height>8192 || type!=GL_UNSIGNED_BYTE ||
       (format!=GL_RGBA && format!=GL_RGB && format!=GL_ALPHA))return;
    if(!entry->object || entry->width!=width || entry->height!=height) {
        if(entry->object)entry->object->Release();entry->object=nullptr;
        // Managed textures survive Alt-Tab Reset; resource ownership still changes on a new device.
        if(cube) {
            if(width!=height)return;IDirect3DCubeTexture9* texture=nullptr;
            if(FAILED(s.device->CreateCubeTexture(UINT(width),1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&texture,nullptr)))return;
            entry->object=texture;
        } else {
            IDirect3DTexture9* texture=nullptr;
            if(FAILED(s.device->CreateTexture(UINT(width),UINT(height),1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&texture,nullptr)))return;
            entry->object=texture;
        }
        entry->width=width;entry->height=height;entry->target=cube ? 0x8513 : GL_TEXTURE_2D;
    }
    D3DLOCKED_RECT locked={};auto face=static_cast<D3DCUBEMAP_FACES>(target-0x8515);
    HRESULT result=cube ? static_cast<IDirect3DCubeTexture9*>(entry->object)->LockRect(face,0,&locked,nullptr,0) :
        static_cast<IDirect3DTexture9*>(entry->object)->LockRect(0,&locked,nullptr,0);
    if(FAILED(result))return;
    auto input=static_cast<const unsigned char*>(pixels);int channels=format==GL_ALPHA ? 1 : format==GL_RGB ? 3 : 4;
    for(int y=0;y<height;++y) {
        auto row=reinterpret_cast<DWORD*>(static_cast<unsigned char*>(locked.pBits)+size_t(y)*locked.Pitch);
        for(int x=0;x<width;++x) {
            auto pixel=input ? input+(size_t(y)*width+x)*channels : nullptr;
            unsigned r=255,g=255,b=255,a=0;
            if(pixel) {if(format==GL_ALPHA)a=*pixel;else {r=pixel[0];g=pixel[1];b=pixel[2];a=channels==4 ? pixel[3] : 255;}}
            row[x]=(a<<24)|(r<<16)|(g<<8)|b;
        }
    }
    if(cube)static_cast<IDirect3DCubeTexture9*>(entry->object)->UnlockRect(face,0);
    else static_cast<IDirect3DTexture9*>(entry->object)->UnlockRect(0);
}
}
void APIENTRY GenTextures(GLsizei count,GLuint* names) {auto& s=detail::Current();for(GLsizei i=0;i<count;++i){names[i]=s.nextTexture++;s.textures[names[i]]={};}}
void APIENTRY BindTexture(GLenum target,GLuint name) {auto& a=detail::Current().attributes;(target==GL_TEXTURE_2D ? a.texture2D : a.textureCube)=name;}
void APIENTRY DeleteTextures(GLsizei count,const GLuint* names) {detail::Delete(count,names);}
GLboolean APIENTRY IsTexture(GLuint name) {auto& s=detail::Current();auto found=s.textures.find(name);return found!=s.textures.end() && found->second.object ? GL_TRUE : GL_FALSE;}
void APIENTRY TexParameteri(GLenum target,GLenum name,GLint value) {
    auto texture=detail::Current().Bound(target);if(!texture)return;
    switch(name) {case GL_TEXTURE_MIN_FILTER:texture->minFilter=value;break;case GL_TEXTURE_MAG_FILTER:texture->magFilter=value;break;
        case GL_TEXTURE_WRAP_S:texture->wrapS=value;break;case GL_TEXTURE_WRAP_T:texture->wrapT=value;break;default:break;}
}
void APIENTRY TexImage2D(GLenum target,GLint level,GLint internal,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void* pixels) {
    detail::Upload(target,level,internal,width,height,border,format,type,pixels);
}
void APIENTRY GetTexLevelParameteriv(GLenum target,GLint,GLenum name,GLint* value) {
    auto texture=detail::Current().Bound(target);*value=!texture || !texture->object ? 0 : name==GL_TEXTURE_WIDTH ? texture->width : texture->height;
}
}
