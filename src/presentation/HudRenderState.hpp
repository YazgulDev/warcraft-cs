#pragma once
#include <windows.h>
#include "GraphicsApi.hpp"
#include "FixedFunctionState.hpp"
#include <cstdio>
#include <cstdint>
#include <cstring>

// Own the 2D pass's raster/texture state independently of native world and held-model rendering.
class HudRenderState {
public:
    HudRenderState() {
        glGetIntegerv(GL_MATRIX_MODE,&matrixMode_);glPushAttrib(GL_ALL_ATTRIB_BITS);
        int major=1,minor=1;
        if(auto version=reinterpret_cast<const char*>(glGetString(GL_VERSION)))sscanf_s(version,"%d.%d",&major,&minor);
        bool v12=major>1 || (major==1 && minor>=2),v13=major>1 || (major==1 && minor>=3);
        auto extensions=reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        bool cube=v13 || (extensions && std::strstr(extensions,"GL_ARB_texture_cube_map"));
        active_=Proc<Active>("glActiveTexture");if(!active_)active_=Proc<Active>("glActiveTextureARB");
        GLint units=1;
        if(active_){glGetIntegerv(0x84E0,&activeUnit_);glGetIntegerv(0x84E2,&units);}
        // A higher-priority 3D/cube target or a second texture stage can mask every HUD quad.
        for(int unit=0;unit<units;++unit) {
            if(active_)active_(0x84C0+unit);
            glDisable(GL_TEXTURE_1D);glDisable(GL_TEXTURE_2D);
            if(v12)glDisable(0x806F);if(cube)glDisable(0x8513);
            glDisable(GL_TEXTURE_GEN_S);glDisable(GL_TEXTURE_GEN_T);glDisable(GL_TEXTURE_GEN_R);glDisable(GL_TEXTURE_GEN_Q);
        }
        if(active_)active_(0x84C0);
        glMatrixMode(GL_TEXTURE);glPushMatrix();glLoadIdentity();glMatrixMode(matrixMode_);
        glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);
        glDisable(GL_CULL_FACE);glDisable(GL_LIGHTING);glDisable(GL_FOG);
        glDisable(GL_POLYGON_STIPPLE);glDisable(GL_POLYGON_SMOOTH);glDisable(GL_COLOR_LOGIC_OP);
        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
        for(int plane=0;plane<6;++plane)glDisable(GL_CLIP_PLANE0+plane);
        glEnable(GL_ALPHA_TEST);glAlphaFunc(GL_GREATER,0);
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        // Blend factors alone do not undo subtract/min/max equations retained by another render pass.
        auto separate=Proc<Separate>("glBlendEquationSeparate");
        if(!separate)separate=Proc<Separate>("glBlendEquationSeparateEXT");
        if(separate)separate(0x8006,0x8006);
        else {auto equation=Proc<Equation>("glBlendEquation");if(!equation)equation=Proc<Equation>("glBlendEquationEXT");if(equation)equation(0x8006);}
        if(v13 || (extensions && std::strstr(extensions,"GL_ARB_multisample"))) {
            // World MSAA coverage/alpha masks must not discard the opaque interface.
            glDisable(0x809E);glDisable(0x809F);glDisable(0x80A0);
        }
        glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    }
    ~HudRenderState() {
        if(active_)active_(0x84C0);
        glMatrixMode(GL_TEXTURE);glPopMatrix();glMatrixMode(matrixMode_);
        glPopAttrib();if(active_)active_(activeUnit_);
    }
    HudRenderState(const HudRenderState&)=delete;
    HudRenderState& operator=(const HudRenderState&)=delete;
private:
    // Keep standalone text/reticle passes independent of programmable host/third-party rendering.
    FixedFunctionState fixedFunction_;
    using Active=void (APIENTRY*)(GLenum);
    using Equation=void (APIENTRY*)(GLenum);
    using Separate=void (APIENTRY*)(GLenum,GLenum);
    template<class T>static T Proc(const char* name) {
        auto value=wglGetProcAddress(name);auto address=reinterpret_cast<uintptr_t>(value);
        return address>3 && address!=UINTPTR_MAX ? reinterpret_cast<T>(value) : nullptr;
    }
    Active active_=nullptr;
    GLint matrixMode_=GL_MODELVIEW,activeUnit_=0x84C0;
};
