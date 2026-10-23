#pragma once
#include "RenderMatrix.hpp"
#include <windows.h>
#include <gl/GL.h>
#include <d3d9.h>
#include <set>
#include <unordered_map>
#include <vector>

// Private fixed-function state belongs to the D3D adapter; presentation retains its existing draw calls.
namespace D3DGraphics { namespace detail {
struct Vertex {float x,y,z;DWORD color;float u,v,w;};
struct Attributes {
    std::set<GLenum> enabled;
    GLenum depth=GL_LESS,source=GL_ONE,destination=GL_ZERO,alpha=GL_ALWAYS;
    float alphaRef=0,lineWidth=1,clearDepth=1,minDepth=0,maxDepth=1;
    bool depthWrite=true;DWORD colorMask=15,color=0xFFFFFFFF;
    GLuint texture2D=0,textureCube=0;
    GLint viewport[4]={};
};
struct Texture {
    IDirect3DBaseTexture9* object=nullptr;
    int width=0,height=0;GLenum target=GL_TEXTURE_2D;
    GLint minFilter=GL_NEAREST,magFilter=GL_NEAREST,wrapS=GL_REPEAT,wrapT=GL_REPEAT;
};
struct State {
    IDirect3DDevice9* device=nullptr;IDirect3DStateBlock9* saved=nullptr;
    IDirect3DVertexBuffer9* stream=nullptr;UINT streamOffset=0,streamStride=0;
    HDC dc=nullptr;bool active=false,scene=false;int width=0,height=0;
    unsigned generation=0;GLuint nextTexture=1;
    Attributes attributes;
    std::vector<Attributes> attributeStack;
    GLenum mode=GL_MODELVIEW,primitive=GL_TRIANGLES;
    std::vector<RenderMatrix::Matrix> model{RenderMatrix::Identity()},projection{RenderMatrix::Identity()},texture{RenderMatrix::Identity()};
    std::vector<Vertex> vertices;float u=0,v=0,w=0;
    std::unordered_map<GLuint,Texture> textures;
    std::vector<RenderMatrix::Matrix>& Matrices() {return mode==GL_PROJECTION ? projection : mode==GL_TEXTURE ? texture : model;}
    Texture* Bound(GLenum target) {
        GLuint name=target==GL_TEXTURE_2D ? attributes.texture2D : attributes.textureCube;
        auto found=textures.find(name);return found==textures.end() ? nullptr : &found->second;
    }
};
State& Current();
void Apply();
void Upload(GLenum target,GLint level,GLint internal,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void* pixels);
void Delete(GLsizei count,const GLuint* names);
}}
