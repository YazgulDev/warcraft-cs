#include "NativeMinimap.hpp"
#include "WarcraftApi.hpp"
#ifdef WCS_D3D9
#include "D3DMinimap.hpp"
#include "../presentation/D3DGraphics.hpp"
#endif
#include <algorithm>
#include <cmath>
#include <cstring>

static bool ReadBounds(uintptr_t ui,float* bounds) {
    // An unavailable/custom native layout omits this frame's map instead of faulting Warcraft.
    __try {
        uintptr_t minimap=*reinterpret_cast<uintptr_t*>(ui+0x3C0);
        if (!minimap) return false;
        memcpy(bounds,reinterpret_cast<void*>(minimap+0xB4+0x44),4*sizeof(float));return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {return false;}
}

static bool ReadPixelBounds(uintptr_t ui,int width,int height,RECT& rectangle) {
    // Both renderers use the same rounded evaluated CMinimap bounds, expressed from the top left.
    float bounds[4]={};if(!ui || width<=0 || height<=0 || !ReadBounds(ui,bounds))return false;
    for(float value:bounds)if(!std::isfinite(value))return false;
    if(bounds[0]<0 || bounds[1]<0 || bounds[2]>.801f || bounds[3]>.601f || bounds[2]<=bounds[0] || bounds[3]<=bounds[1])return false;
    rectangle.left=std::clamp(int(std::ceil(bounds[0]*width/.8f)),0,width-1);
    rectangle.right=std::clamp(int(std::floor(bounds[2]*width/.8f)),int(rectangle.left)+1,width);
    rectangle.bottom=height-std::clamp(int(std::ceil(bounds[1]*height/.6f)),0,height-1);
    rectangle.top=height-std::clamp(int(std::floor(bounds[3]*height/.6f)),height-int(rectangle.bottom)+1,height);
    return true;
}

void NativeMinimap::ContextDeleted(HGLRC context) {
    if (context_!=context) return;
    // Never delete old numeric texture IDs in a replacement context.
    context_=nullptr;background_=map_=0;textureSide_=0;capturing_=ready_=false;
}
void NativeMinimap::BeginFrame(uintptr_t ui,int width,int height,bool enabled) {
    ui_=ui;width_=width;height_=height;enabled_=enabled;capturing_=ready_=false;
#ifdef WCS_D3D9
    // Convert the same evaluated native rectangle to D3D's top-left coordinates without GL calls.
    RECT rectangle={};bool valid=enabled && ReadPixelBounds(ui,width,height,rectangle);
    D3DMinimap::Configure(rectangle,width,height,valid);
#endif
}
bool NativeMinimap::Prepare() {
    if (!enabled_ || !ui_ || width_<=0 || height_<=0) return false;
    // 1.26a CMinimap embeds CFrame at +B4; its evaluated rectangle is +44..50.
    // Read each frame because cinematic/custom map layouts can change independently of FPS.
    RECT rectangle={};if(!ReadPixelBounds(ui_,width_,height_,rectangle))return false;
    x_=rectangle.left;y_=height_-rectangle.bottom;
    cropWidth_=rectangle.right-rectangle.left;cropHeight_=rectangle.bottom-rectangle.top;
    HGLRC current=wglGetCurrentContext();if (!current) return false;
    if (context_!=current) {ContextDeleted(context_);context_=current;}
    // Forget lost resources rather than deleting names that the host may have recycled.
    if ((background_ && !glIsTexture(background_)) || (map_ && !glIsTexture(map_))) {
        background_=map_=0;textureSide_=0;
    }
    GLint maximum=0;glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maximum);
    int side=1;while (side<std::max(cropWidth_,cropHeight_)) side*=2;
    if (side>maximum) return false;
    glPushAttrib(GL_TEXTURE_BIT|GL_COLOR_BUFFER_BIT|GL_PIXEL_MODE_BIT);
    // Campaign restoration can draw directly to the front buffer; capture the same target as the world.
    GLint buffer=GL_BACK;glGetIntegerv(GL_DRAW_BUFFER,&buffer);captureBuffer_=GLenum(buffer);glReadBuffer(captureBuffer_);
    if (!background_) glGenTextures(1,&background_);
    if (!map_) glGenTextures(1,&map_);
    if (textureSide_!=side) {
        textureSide_=side;
        for (GLuint texture : {background_,map_}) {
            glBindTexture(GL_TEXTURE_2D,texture);
            // Background restoration is pixel-exact; only the resized display map is filtered.
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,texture==background_ ? GL_NEAREST : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,texture==background_ ? GL_NEAREST : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,side,side,0,GL_RGB,GL_UNSIGNED_BYTE,nullptr);
        }
        wc3::Log("Minimap native crop=%d,%d %dx%d texture=%d",x_,y_,cropWidth_,cropHeight_,side);
    }
    // Save the world beneath the original RTS map before allowing cropped UI submissions.
    glBindTexture(GL_TEXTURE_2D,background_);
    glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,x_,y_,cropWidth_,cropHeight_);
    glPopAttrib();
    // Rectangular levels leave letterbox strips: black replaces the racial console behind them.
    glPushAttrib(GL_SCISSOR_BIT|GL_COLOR_BUFFER_BIT);glEnable(GL_SCISSOR_TEST);
    glScissor(x_,y_,cropWidth_,cropHeight_);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);glPopAttrib();
    capturing_=true;return true;
}
bool NativeMinimap::MapBatch(GLsizei count,GLenum type,const void* indices) const {
    // Console/portrait batches may overlap letterboxing, but their vertices extend outside the map.
    // Inspect the submitted client array in projected pixels; preserve actual map, fog and marker batches.
    if (count<=0 || count>100000 || !indices || !glIsEnabled(GL_VERTEX_ARRAY)) return false;
    GLint size=0,stride=0,vertexType=0,viewport[4]={};void* vertices=nullptr;
    glGetIntegerv(GL_VERTEX_ARRAY_SIZE,&size);glGetIntegerv(GL_VERTEX_ARRAY_STRIDE,&stride);
    glGetIntegerv(GL_VERTEX_ARRAY_TYPE,&vertexType);glGetPointerv(GL_VERTEX_ARRAY_POINTER,&vertices);
    if (!vertices || vertexType!=GL_FLOAT || size<2 || size>4 || stride<0) return false;
    if (!stride) stride=size*sizeof(float);
    float model[16]={},projection[16]={};glGetFloatv(GL_MODELVIEW_MATRIX,model);glGetFloatv(GL_PROJECTION_MATRIX,projection);
    glGetIntegerv(GL_VIEWPORT,viewport);
    float tolerance=std::max(3.f,std::max(cropWidth_,cropHeight_)*.05f);
    __try {
        for (int i=0;i<count;++i) {
            unsigned index=type==GL_UNSIGNED_SHORT ? static_cast<const unsigned short*>(indices)[i] :
                type==GL_UNSIGNED_INT ? static_cast<const unsigned*>(indices)[i] :
                type==GL_UNSIGNED_BYTE ? static_cast<const unsigned char*>(indices)[i] : 0xFFFFFFFF;
            if (index>100000) return false;
            const float* vertex=reinterpret_cast<const float*>(static_cast<const char*>(vertices)+size_t(index)*stride);
            float input[4]={vertex[0],vertex[1],size>=3 ? vertex[2] : 0,size==4 ? vertex[3] : 1};
            float eye[4]={},clip[4]={};
            for (int row=0;row<4;++row) for (int col=0;col<4;++col) eye[row]+=model[col*4+row]*input[col];
            for (int row=0;row<4;++row) for (int col=0;col<4;++col) clip[row]+=projection[col*4+row]*eye[col];
            if (!std::isfinite(clip[3]) || std::abs(clip[3])<.000001f) return false;
            float x=viewport[0]+(clip[0]/clip[3]+1)*viewport[2]*.5f;
            float y=viewport[1]+(clip[1]/clip[3]+1)*viewport[3]*.5f;
            // Native map quads/edge markers overscan their layout rectangle; rasterization still clips exactly.
            if (!std::isfinite(x) || !std::isfinite(y) || x<x_-tolerance || x>x_+cropWidth_+tolerance ||
                y<y_-tolerance || y>y_+cropHeight_+tolerance) return false;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {return false;}
    return true;
}
void NativeMinimap::Submit(GLenum mode,GLsizei count,GLenum type,const void* indices,DrawElements original) {
    if (!capturing_ && !Prepare()) return;
    if (!MapBatch(count,type,indices)) return;
    // Retain native terrain/fog/marker batches, rasterizing only their map pixels.
    // Native UI must not overwrite world depth later sampled by health bars and sky.
    glPushAttrib(GL_SCISSOR_BIT|GL_DEPTH_BUFFER_BIT);glDepthMask(GL_FALSE);
    glEnable(GL_SCISSOR_TEST);glScissor(x_,y_,cropWidth_,cropHeight_);
    original(mode,count,type,indices);glPopAttrib();
}
void NativeMinimap::Quad(GLuint texture,float left,float bottom,float right,float top,bool inset) {
    glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture);glColor4f(1,1,1,1);
    // Half-texel inset avoids sampling unused power-of-two padding along the edges.
    float edge=inset ? .5f : 0;
    float u0=edge/textureSide_,v0=edge/textureSide_;
    float u1=(cropWidth_-edge)/textureSide_,v1=(cropHeight_-edge)/textureSide_;
    glBegin(GL_QUADS);
    glTexCoord2f(u0,v0);glVertex2f(left,bottom);glTexCoord2f(u1,v0);glVertex2f(right,bottom);
    glTexCoord2f(u1,v1);glVertex2f(right,top);glTexCoord2f(u0,v1);glVertex2f(left,top);glEnd();
}
void NativeMinimap::FinishFrame() {
    if (!capturing_ || context_!=wglGetCurrentContext()) return;
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    // Native UI may have targeted a different buffer from the final HUD; restore its own captured pixels.
    glReadBuffer(captureBuffer_);glDrawBuffer(captureBuffer_);
    glBindTexture(GL_TEXTURE_2D,map_);glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,x_,y_,cropWidth_,cropHeight_);
    // Restore the world before sky/scope/weapons; the FPS HUD composites the map last.
    GLint mode=0,active=0x84C0;glGetIntegerv(GL_MATRIX_MODE,&mode);
    using ActiveTexture=void (APIENTRY*)(GLenum);
    auto activeTexture=reinterpret_cast<ActiveTexture>(wglGetProcAddress("glActiveTextureARB"));
    if (activeTexture) {
        glGetIntegerv(0x84E0,&active);GLint units=1;glGetIntegerv(0x84E2,&units);
        for (int unit=0;unit<units;++unit) {activeTexture(0x84C0+unit);glDisable(GL_TEXTURE_1D);glDisable(GL_TEXTURE_2D);}
        activeTexture(0x84C0);
    }
    glViewport(0,0,width_,height_);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glDisable(GL_ALPHA_TEST);
    glDisable(GL_SCISSOR_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_CULL_FACE);glDisable(GL_FOG);glDisable(GL_LIGHTING);
    glDisable(GL_POLYGON_STIPPLE);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glDisable(GL_COLOR_LOGIC_OP);
    for (int plane=0;plane<6;++plane) glDisable(GL_CLIP_PLANE0+plane);
    glDisable(GL_TEXTURE_GEN_S);glDisable(GL_TEXTURE_GEN_T);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
    glMatrixMode(GL_TEXTURE);glPushMatrix();glLoadIdentity();
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width_,0,height_,-1,1);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    Quad(background_,float(x_),float(y_),float(x_+cropWidth_),float(y_+cropHeight_),false);
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_TEXTURE);glPopMatrix();
    glMatrixMode(mode);glPopAttrib();if (activeTexture) activeTexture(active);
    capturing_=false;ready_=true;
}
void NativeMinimap::Draw(const MinimapLayout& layout) {
#ifdef WCS_D3D9
    if(D3DGraphics::Active()) {D3DMinimap::Draw(layout);return;}
#endif
    if (!ready_ || context_!=wglGetCurrentContext() || layout.side<=0) return;
    // Overlay supplies a top-left pixel projection; retain its state for subsequent HUD text.
    glPushAttrib(GL_TEXTURE_BIT|GL_CURRENT_BIT);
    Quad(map_,float(layout.x),float(layout.y+layout.side),float(layout.x+layout.side),float(layout.y));
    glPopAttrib();
}
