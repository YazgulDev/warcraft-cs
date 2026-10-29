#include "D3DGraphics.hpp"
#include "D3DGraphicsState.hpp"
#include "GraphicsDiagnostics.hpp"
#include <algorithm>
#include <cstring>

namespace D3DGraphics {
namespace detail {
State& Current() {static thread_local State state;return state;}
static DWORD Blend(GLenum value) {
    switch(value) {
        case GL_ZERO:return D3DBLEND_ZERO;case GL_ONE:return D3DBLEND_ONE;
        case GL_SRC_ALPHA:return D3DBLEND_SRCALPHA;case GL_ONE_MINUS_SRC_ALPHA:return D3DBLEND_INVSRCALPHA;
        case GL_DST_ALPHA:return D3DBLEND_DESTALPHA;case GL_ONE_MINUS_DST_ALPHA:return D3DBLEND_INVDESTALPHA;
        case GL_SRC_COLOR:return D3DBLEND_SRCCOLOR;case GL_ONE_MINUS_SRC_COLOR:return D3DBLEND_INVSRCCOLOR;
        case GL_DST_COLOR:return D3DBLEND_DESTCOLOR;case GL_ONE_MINUS_DST_COLOR:return D3DBLEND_INVDESTCOLOR;
        default:return D3DBLEND_ONE;
    }
}
void Apply() {
    auto& s=Current();auto d=s.device;auto& a=s.attributes;
    auto enabled=[&](GLenum value){return a.enabled.count(value)!=0;};
    // Record the exact rejected state/stage instead of silently losing the following draw.
    auto render=[&](D3DRENDERSTATETYPE key,DWORD value){GraphicsDiagnostics::Result("SetRenderState",d->SetRenderState(key,value),unsigned(key),value);};
    auto stage=[&](DWORD unit,D3DTEXTURESTAGESTATETYPE key,DWORD value){GraphicsDiagnostics::Result("SetTextureStageState",d->SetTextureStageState(unit,key,value),(unit<<16)|unsigned(key),value);};
    auto sampler=[&](DWORD unit,D3DSAMPLERSTATETYPE key,DWORD value){GraphicsDiagnostics::Result("SetSamplerState",d->SetSamplerState(unit,key,value),(unit<<16)|unsigned(key),value);};
    auto binding=[&](DWORD unit,IDirect3DBaseTexture9* texture){GraphicsDiagnostics::Result("SetTexture",d->SetTexture(unit,texture),unit);};
    auto transform=[&](D3DTRANSFORMSTATETYPE key,const D3DMATRIX* matrix){GraphicsDiagnostics::Result("SetTransform",d->SetTransform(key,matrix),unsigned(key));};
    // Every submitted batch starts from owned state, including hostile shader/texture state left by Warcraft.
    GraphicsDiagnostics::Result("SetVertexShader",d->SetVertexShader(nullptr));GraphicsDiagnostics::Result("SetPixelShader",d->SetPixelShader(nullptr));GraphicsDiagnostics::Result("SetVertexDeclaration",d->SetVertexDeclaration(nullptr));
    GraphicsDiagnostics::Result("SetFVF",d->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1|D3DFVF_TEXCOORDSIZE3(0)));
    render(D3DRS_LIGHTING,FALSE);render(D3DRS_FOGENABLE,FALSE);
    render(D3DRS_CULLMODE,D3DCULL_NONE);render(D3DRS_STENCILENABLE,FALSE);
    render(D3DRS_SCISSORTESTENABLE,FALSE);render(D3DRS_CLIPPLANEENABLE,0);
    render(D3DRS_FILLMODE,D3DFILL_SOLID);render(D3DRS_SHADEMODE,D3DSHADE_GOURAUD);
    render(D3DRS_COLORWRITEENABLE,a.colorMask);render(D3DRS_SPECULARENABLE,FALSE);
    render(D3DRS_ZENABLE,enabled(GL_DEPTH_TEST));render(D3DRS_ZWRITEENABLE,a.depthWrite);
    render(D3DRS_ZFUNC,a.depth-GL_NEVER+D3DCMP_NEVER);
    render(D3DRS_ALPHABLENDENABLE,enabled(GL_BLEND));render(D3DRS_BLENDOP,D3DBLENDOP_ADD);
    render(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
    render(D3DRS_SRCBLEND,Blend(a.source));render(D3DRS_DESTBLEND,Blend(a.destination));
    render(D3DRS_ALPHATESTENABLE,enabled(GL_ALPHA_TEST));
    render(D3DRS_ALPHAFUNC,a.alpha-GL_NEVER+D3DCMP_NEVER);
    render(D3DRS_ALPHAREF,DWORD(std::clamp(a.alphaRef,0.f,1.f)*255));
    render(D3DRS_SRGBWRITEENABLE,FALSE);
    auto texture=s.Bound(enabled(0x8513) ? 0x8513 : GL_TEXTURE_2D);
    bool textured=(enabled(GL_TEXTURE_2D)||enabled(0x8513)) && texture && texture->object;
    binding(0,textured ? texture->object : nullptr);
    for(unsigned unit=1;unit<8;++unit) {binding(unit,nullptr);stage(unit,D3DTSS_COLOROP,D3DTOP_DISABLE);}
    stage(0,D3DTSS_COLOROP,textured ? D3DTOP_MODULATE : D3DTOP_SELECTARG2);
    stage(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);stage(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
    stage(0,D3DTSS_ALPHAOP,textured ? D3DTOP_MODULATE : D3DTOP_SELECTARG2);
    stage(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);stage(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);
    stage(0,D3DTSS_TEXCOORDINDEX,0);stage(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
    sampler(0,D3DSAMP_SRGBTEXTURE,FALSE);sampler(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE);
    sampler(0,D3DSAMP_MINFILTER,texture && texture->minFilter==GL_NEAREST ? D3DTEXF_POINT : D3DTEXF_LINEAR);
    sampler(0,D3DSAMP_MAGFILTER,texture && texture->magFilter==GL_NEAREST ? D3DTEXF_POINT : D3DTEXF_LINEAR);
    sampler(0,D3DSAMP_ADDRESSU,texture && texture->wrapS==GL_REPEAT ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP);
    sampler(0,D3DSAMP_ADDRESSV,texture && texture->wrapT==GL_REPEAT ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP);
    auto identity=RenderMatrix::Identity();auto projection=RenderMatrix::DepthForD3D(s.projection.back());
    transform(D3DTS_WORLD,reinterpret_cast<const D3DMATRIX*>(s.model.back().data()));
    transform(D3DTS_VIEW,reinterpret_cast<const D3DMATRIX*>(identity.data()));
    transform(D3DTS_PROJECTION,reinterpret_cast<const D3DMATRIX*>(projection.data()));
    D3DVIEWPORT9 viewport={DWORD(a.viewport[0]),DWORD(s.height-a.viewport[1]-a.viewport[3]),DWORD(a.viewport[2]),DWORD(a.viewport[3]),a.minDepth,a.maxDepth};
    GraphicsDiagnostics::Result("SetViewport",d->SetViewport(&viewport),a.viewport[2],a.viewport[3]);
}
}
using detail::Current;
bool Active() {return Current().active;}
bool BeginFrame(IDirect3DDevice9* device,HDC dc,int width,int height) {
    auto& s=Current();if(s.active || !device || width<=0 || height<=0)return false;
    // Device-loss and frame/state-block failures must remain visible even with TRACE disabled.
    if(!GraphicsDiagnostics::Result("TestCooperativeLevel",device->TestCooperativeLevel()))return false;
    if(s.device!=device) {
        // Texture IDs never cross device ownership; Overlay observes the changed generation and forgets its caches.
        for(auto& entry:s.textures)if(entry.second.object)entry.second.object->Release();
        s.textures.clear();s.device=device;++s.generation;
    }
    if(!GraphicsDiagnostics::Result("CreateStateBlock",device->CreateStateBlock(D3DSBT_ALL,&s.saved)))return false;
    if(!GraphicsDiagnostics::Result("StateBlock.Capture",s.saved->Capture())) {s.saved->Release();s.saved=nullptr;return false;}
    // DrawPrimitiveUP clears stream zero, which some drivers omit from their state-block restore.
    GraphicsDiagnostics::Result("GetStreamSource",device->GetStreamSource(0,&s.stream,&s.streamOffset,&s.streamStride));
    s.scene=GraphicsDiagnostics::Result("BeginScene",device->BeginScene());
    if(!s.scene) {s.saved->Release();s.saved=nullptr;if(s.stream)s.stream->Release();s.stream=nullptr;return false;}
    s.dc=dc;s.width=width;s.height=height;s.attributes={};s.attributes.viewport[2]=width;s.attributes.viewport[3]=height;
    s.model={RenderMatrix::Identity()};s.projection={RenderMatrix::Identity()};s.texture={RenderMatrix::Identity()};
    s.attributeStack.clear();s.mode=GL_MODELVIEW;s.active=true;return true;
}
void EndFrame() {
    auto& s=Current();if(!s.active)return;s.active=false;
    if(s.scene)GraphicsDiagnostics::Result("EndScene",s.device->EndScene());s.scene=false;
    GraphicsDiagnostics::Result("StateBlock.Apply",s.saved->Apply());s.saved->Release();s.saved=nullptr;
    GraphicsDiagnostics::Result("SetStreamSource.restore",s.device->SetStreamSource(0,s.stream,s.streamOffset,s.streamStride));if(s.stream)s.stream->Release();s.stream=nullptr;
}
void DrawTexture(IDirect3DTexture9* texture,int x,int y,int width,int height) {
    auto& s=Current();if(!s.active || !texture)return;
    // A temporary canvas binding reuses its normal state normalization without taking ownership of the native image.
    GLuint name=s.nextTexture++;detail::Texture borrowed;borrowed.object=texture;borrowed.minFilter=borrowed.magFilter=GL_LINEAR;borrowed.wrapS=borrowed.wrapT=GL_CLAMP;
    s.textures.emplace(name,borrowed);PushAttrib(GL_ALL_ATTRIB_BITS);
    Enable(GL_TEXTURE_2D);BindTexture(GL_TEXTURE_2D,name);Disable(GL_DEPTH_TEST);Disable(GL_BLEND);Color4f(1,1,1,1);
    Begin(GL_QUADS);TexCoord2f(0,0);Vertex2i(x,y);TexCoord2f(1,0);Vertex2i(x+width,y);
    TexCoord2f(1,1);Vertex2i(x+width,y+height);TexCoord2f(0,1);Vertex2i(x,y+height);End();
    PopAttrib();s.textures.erase(name);
}
void APIENTRY Begin(GLenum mode) {auto& s=Current();s.primitive=mode;s.vertices.clear();}
void APIENTRY End() {
    auto& s=Current();if(s.vertices.empty())return;detail::Apply();
    D3DPRIMITIVETYPE type=D3DPT_TRIANGLELIST;UINT count=0;std::vector<detail::Vertex> expanded;
    const detail::Vertex* data=s.vertices.data();auto size=s.vertices.size();
    switch(s.primitive) {
        case GL_QUADS:
            for(size_t i=0;i+3<size;i+=4)for(unsigned j:{0u,1u,2u,0u,2u,3u})expanded.push_back(data[i+j]);
            data=expanded.data();count=UINT(expanded.size()/3);break;
        case GL_TRIANGLES:count=UINT(size/3);break;
        case GL_TRIANGLE_STRIP:type=D3DPT_TRIANGLESTRIP;count=size>=3 ? UINT(size-2) : 0;break;
        case GL_TRIANGLE_FAN:case GL_POLYGON:type=D3DPT_TRIANGLEFAN;count=size>=3 ? UINT(size-2) : 0;break;
        case GL_LINES:type=D3DPT_LINELIST;count=UINT(size/2);break;
        case GL_LINE_STRIP:type=D3DPT_LINESTRIP;count=size>=2 ? UINT(size-1) : 0;break;
        case GL_LINE_LOOP:
            expanded=s.vertices;expanded.push_back(data[0]);data=expanded.data();type=D3DPT_LINESTRIP;count=UINT(size);break;
        default:break;
    }
    if(count)GraphicsDiagnostics::Result("DrawPrimitiveUP",s.device->DrawPrimitiveUP(type,count,data,sizeof(detail::Vertex)),unsigned(type),count);
}
void APIENTRY Vertex3f(GLfloat x,GLfloat y,GLfloat z) {auto& s=Current();s.vertices.push_back({x,y,z,s.attributes.color,s.u,s.v,s.w});}
void APIENTRY Vertex2f(GLfloat x,GLfloat y) {Vertex3f(x,y,0);}
void APIENTRY Vertex2i(GLint x,GLint y) {Vertex3f(float(x),float(y),0);}
void APIENTRY TexCoord2f(GLfloat u,GLfloat v) {auto& s=Current();s.u=u;s.v=v;s.w=0;}
void APIENTRY TexCoord3f(GLfloat u,GLfloat v,GLfloat w) {auto& s=Current();s.u=u;s.v=v;s.w=w;}
void APIENTRY Color4f(GLfloat r,GLfloat g,GLfloat b,GLfloat a) {
    auto byte=[](float value){return DWORD(std::clamp(value,0.f,1.f)*255+.5f);};
    Current().attributes.color=(byte(a)<<24)|(byte(r)<<16)|(byte(g)<<8)|byte(b);
}
void APIENTRY Color3f(GLfloat r,GLfloat g,GLfloat b) {Color4f(r,g,b,1);}
void APIENTRY Enable(GLenum capability) {Current().attributes.enabled.insert(capability);}
void APIENTRY Disable(GLenum capability) {Current().attributes.enabled.erase(capability);}
GLboolean APIENTRY IsEnabled(GLenum capability) {return Current().attributes.enabled.count(capability) ? GL_TRUE : GL_FALSE;}
void APIENTRY DepthMask(GLboolean value) {Current().attributes.depthWrite=value!=0;}
void APIENTRY DepthFunc(GLenum value) {Current().attributes.depth=value;}
void APIENTRY DepthRange(GLclampd nearPlane,GLclampd farPlane) {Current().attributes.minDepth=float(nearPlane);Current().attributes.maxDepth=float(farPlane);}
void APIENTRY ClearDepth(GLclampd value) {Current().attributes.clearDepth=float(value);}
void APIENTRY Clear(GLbitfield mask) {
    auto& s=Current();if(mask&GL_DEPTH_BUFFER_BIT) {
        // Clear the whole overlay viewport, independent of native scissor/write-mask leftovers.
        GraphicsDiagnostics::Result("Clear.SetRenderState",s.device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE));
        GraphicsDiagnostics::Result("Clear.depth",s.device->Clear(0,nullptr,D3DCLEAR_ZBUFFER,0,s.attributes.clearDepth,0));
    }
}
void APIENTRY AlphaFunc(GLenum function,GLclampf reference) {Current().attributes.alpha=function;Current().attributes.alphaRef=reference;}
void APIENTRY BlendFunc(GLenum source,GLenum destination) {Current().attributes.source=source;Current().attributes.destination=destination;}
void APIENTRY ColorMask(GLboolean r,GLboolean g,GLboolean b,GLboolean a) {Current().attributes.colorMask=(r ? 1:0)|(g ? 2:0)|(b ? 4:0)|(a ? 8:0);}
void APIENTRY LineWidth(GLfloat value) {Current().attributes.lineWidth=value;}
void APIENTRY PolygonMode(GLenum,GLenum) {} // All project geometry is filled; native raster mode is isolated in Apply().
void APIENTRY Viewport(GLint x,GLint y,GLsizei width,GLsizei height) {auto& v=Current().attributes.viewport;v[0]=x;v[1]=y;v[2]=width;v[3]=height;}
void APIENTRY PushAttrib(GLbitfield) {Current().attributeStack.push_back(Current().attributes);}
void APIENTRY PopAttrib() {auto& s=Current();if(!s.attributeStack.empty()){s.attributes=s.attributeStack.back();s.attributeStack.pop_back();}}
void APIENTRY PushClientAttrib(GLbitfield) {} // Uploads use tightly packed owned buffers and do not inherit GL client arrays.
void APIENTRY PopClientAttrib() {}
void APIENTRY PixelStorei(GLenum,GLint) {}
void APIENTRY PixelTransferi(GLenum,GLint) {}
void APIENTRY PixelTransferf(GLenum,GLfloat) {}
void APIENTRY TexEnvi(GLenum,GLenum,GLint) {} // The shared renderer always modulates diffuse color and texture coverage.
void APIENTRY MatrixMode(GLenum mode) {Current().mode=mode;}
void APIENTRY LoadIdentity() {Current().Matrices().back()=RenderMatrix::Identity();}
void APIENTRY LoadMatrixf(const GLfloat* value) {std::copy(value,value+16,Current().Matrices().back().begin());}
void APIENTRY PushMatrix() {auto& matrices=Current().Matrices();matrices.push_back(matrices.back());}
void APIENTRY PopMatrix() {auto& matrices=Current().Matrices();if(matrices.size()>1)matrices.pop_back();}
static void Multiply(const RenderMatrix::Matrix& matrix) {auto& current=Current().Matrices().back();current=RenderMatrix::Multiply(current,matrix);}
void APIENTRY Translatef(GLfloat x,GLfloat y,GLfloat z) {auto m=RenderMatrix::Identity();m[12]=x;m[13]=y;m[14]=z;Multiply(m);}
void APIENTRY Scalef(GLfloat x,GLfloat y,GLfloat z) {auto m=RenderMatrix::Identity();m[0]=x;m[5]=y;m[10]=z;Multiply(m);}
void APIENTRY Rotatef(GLfloat angle,GLfloat x,GLfloat y,GLfloat z) {
    float length=std::sqrt(x*x+y*y+z*z);if(length<=0)return;x/=length;y/=length;z/=length;
    float c=std::cos(angle*.01745329252f),s=std::sin(angle*.01745329252f),t=1-c;
    Multiply({t*x*x+c,t*x*y+s*z,t*x*z-s*y,0,t*x*y-s*z,t*y*y+c,t*y*z+s*x,0,t*x*z+s*y,t*y*z-s*x,t*z*z+c,0,0,0,0,1});
}
void APIENTRY Ortho(GLdouble l,GLdouble r,GLdouble b,GLdouble t,GLdouble n,GLdouble f) {
    if(r==l || t==b || f==n)return;auto m=RenderMatrix::Identity();
    m[0]=float(2/(r-l));m[5]=float(2/(t-b));m[10]=float(-2/(f-n));m[12]=float(-(r+l)/(r-l));m[13]=float(-(t+b)/(t-b));m[14]=float(-(f+n)/(f-n));Multiply(m);
}
void APIENTRY Frustum(GLdouble l,GLdouble r,GLdouble b,GLdouble t,GLdouble n,GLdouble f) {
    if(r==l || t==b || f==n || n<=0)return;RenderMatrix::Matrix m={};
    m[0]=float(2*n/(r-l));m[5]=float(2*n/(t-b));m[8]=float((r+l)/(r-l));m[9]=float((t+b)/(t-b));
    m[10]=float(-(f+n)/(f-n));m[11]=-1;m[14]=float(-2*f*n/(f-n));Multiply(m);
}
void APIENTRY GetIntegerv(GLenum name,GLint* result) {
    auto& s=Current();switch(name) {
        case GL_MATRIX_MODE:*result=GLint(s.mode);break;
        case GL_VIEWPORT:case GL_SCISSOR_BOX:std::copy(s.attributes.viewport,s.attributes.viewport+4,result);break;
        case GL_MAX_TEXTURE_SIZE:{D3DCAPS9 caps={};s.device->GetDeviceCaps(&caps);*result=GLint(caps.MaxTextureWidth);break;}
        case GL_POLYGON_MODE:result[0]=result[1]=GL_FILL;break;
        case GL_TEXTURE_BINDING_2D:*result=GLint(s.attributes.texture2D);break;
        case GL_BLEND_SRC:*result=GLint(s.attributes.source);break;
        case GL_BLEND_DST:*result=GLint(s.attributes.destination);break;
        case GL_ALPHA_TEST_FUNC:*result=GLint(s.attributes.alpha);break;
        case GL_DRAW_BUFFER:case GL_READ_BUFFER:*result=GL_BACK;break;
        default:*result=0;break;
    }
}
void APIENTRY GetFloatv(GLenum name,GLfloat* result) {
    auto& s=Current();if(name==GL_PROJECTION_MATRIX || name==GL_MODELVIEW_MATRIX || name==GL_TEXTURE_MATRIX) {
        const auto& matrix=name==GL_PROJECTION_MATRIX ? s.projection.back() : name==GL_MODELVIEW_MATRIX ? s.model.back() : s.texture.back();
        std::copy(matrix.begin(),matrix.end(),result);
    } else if(name==GL_CURRENT_COLOR) {
        DWORD color=s.attributes.color;result[0]=float((color>>16)&255)/255;result[1]=float((color>>8)&255)/255;
        result[2]=float(color&255)/255;result[3]=float((color>>24)&255)/255;
    } else *result=name==GL_ALPHA_TEST_REF ? s.attributes.alphaRef : 0;
}
void APIENTRY GetBooleanv(GLenum name,GLboolean* result) {
    if(name==GL_COLOR_WRITEMASK)for(int i=0;i<4;++i)result[i]=(Current().attributes.colorMask&(1<<i)) ? GL_TRUE : GL_FALSE;
    else *result=name==GL_DEPTH_WRITEMASK ? Current().attributes.depthWrite : GL_FALSE;
}
void APIENTRY GetTexEnviv(GLenum,GLenum,GLint* value) {*value=GL_MODULATE;}
GLenum APIENTRY GetError() {return GL_NO_ERROR;}
const GLubyte* APIENTRY GetString(GLenum name) {
    const char* value=name==GL_VERSION ? "1.1 WarcraftCS D3D9 adapter" : name==GL_EXTENSIONS ? "" : "WarcraftCS Direct3D 9";
    return reinterpret_cast<const GLubyte*>(value);
}
HGLRC WINAPI GetCurrentContext() {return reinterpret_cast<HGLRC>(uintptr_t(Current().generation)*16+1);}
HDC WINAPI GetCurrentDC() {return Current().dc;}
PROC WINAPI GetProcAddress(LPCSTR) {return nullptr;} // GL extension pointers must never run inside a D3D frame.
}
