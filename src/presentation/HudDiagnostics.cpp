#include "GraphicsApi.hpp"
#include "HudDiagnostics.hpp"
#include "../platform/DiagnosticLog.hpp"
#include <gl/GL.h>
#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cstring>

namespace {
void Record(const char* level,const char* format,...) {
    va_list args;va_start(args,format);DiagnosticLog::Write(level,format,args);va_end(args);
}
template<class T>T Proc(const char* name) {
    auto value=wglGetProcAddress(name);auto address=reinterpret_cast<uintptr_t>(value);
    return address>3 && address!=UINTPTR_MAX ? reinterpret_cast<T>(value) : nullptr;
}
}
bool HudDiagnostics::Begin(const char* mode,int width,int height) {
    auto current=wglGetCurrentContext();bool changed=current!=context_ || mode_!=mode;
    context_=current;mode_=mode;
    bool sampled=DiagnosticLog::Due(previous_,changed);
    if(sampled)Record("TRACE","HUD frame mode=%s size=%dx%d context=%p dc=%p",mode,width,height,current,wglGetCurrentDC());
    State("host",sampled);return sampled;
}
void HudDiagnostics::Omitted(const char* reason,bool sampled) {
    if(sampled)Record("TRACE","HUD omitted reason=%s",reason);
}
void HudDiagnostics::Font(const char* role,const char* phase,int requested,int width,int height,unsigned texture,unsigned long error,int glyph,int coverage) {
    Record(!strcmp(phase,"ready") ? "INFO" : "ERROR",
        "HUD font role=%s phase=%s requested=%d atlas=%dx%d texture=%u context=%p winError=%lu glyph=%d coverage=%d",
        role,phase,requested,width,height,texture,wglGetCurrentContext(),error,glyph,coverage);
}
void HudDiagnostics::State(const char* phase,bool sampled) {
    if(!sampled || !wglGetCurrentContext())return;
    int major=1,minor=1;if(auto version=reinterpret_cast<const char*>(glGetString(GL_VERSION)))sscanf_s(version,"%d.%d",&major,&minor);
    auto ext=reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    auto has=[&](const char* name){return ext && strstr(ext,name);};
    bool v12=major>1 || (major==1 && minor>=2),v13=major>1 || (major==1 && minor>=3),v14=major>1 || (major==1 && minor>=4);
    GLint draw=0,read=0,fbDraw=0,fbRead=0,program=0,pipeline=0,equation=0x8006,equationAlpha=0x8006;
    glGetIntegerv(GL_DRAW_BUFFER,&draw);glGetIntegerv(GL_READ_BUFFER,&read);
    // Only query extension state supported by this context; diagnostics must not manufacture GL errors.
    if(major>=3 || has("GL_ARB_framebuffer_object")){glGetIntegerv(0x8CA6,&fbDraw);glGetIntegerv(0x8CAA,&fbRead);}
    else if(has("GL_EXT_framebuffer_object")){glGetIntegerv(0x8CA6,&fbDraw);fbRead=fbDraw;}
    if(major>=2)glGetIntegerv(0x8B8D,&program);
    // Separate shader pipelines can affect pixels even with CURRENT_PROGRAM=0 and no GL errors.
    if(major>4 || (major==4 && minor>=1) || has("GL_ARB_separate_shader_objects"))glGetIntegerv(0x825A,&pipeline);
    if(v14 || has("GL_EXT_blend_minmax") || has("GL_EXT_blend_subtract"))glGetIntegerv(0x8009,&equation);
    equationAlpha=equation;if(major>=2 || has("GL_EXT_blend_equation_separate"))glGetIntegerv(0x883D,&equationAlpha);
    GLfloat coverage=1;GLboolean invert=0;GLint samples=0;bool ms=v13 || has("GL_ARB_multisample");
    if(ms){glGetFloatv(0x80AA,&coverage);glGetBooleanv(0x80AB,&invert);glGetIntegerv(0x80A9,&samples);}
    Record("TRACE","HUD state phase=%s drawBuffer=%04X readBuffer=%04X framebuffer=%d/%d program=%d pipeline=%d blendEquation=%04X/%04X polygonSmooth=%d samples=%d alphaCoverage=%d alphaOne=%d coverage=%d:%.3f:%d fragmentProgram=%d vertexProgram=%d discard=%d",
        phase,draw,read,fbDraw,fbRead,program,pipeline,equation,equationAlpha,glIsEnabled(GL_POLYGON_SMOOTH),samples,
        ms ? glIsEnabled(0x809E) : 0,ms ? glIsEnabled(0x809F) : 0,ms ? glIsEnabled(0x80A0) : 0,coverage,invert,
        has("GL_ARB_fragment_program") ? glIsEnabled(0x8804) : 0,has("GL_ARB_vertex_program") ? glIsEnabled(0x8620) : 0,major>=3 ? glIsEnabled(0x8C89) : 0);
    GLint viewport[4],scissor[4],polygon[2],source=0,destination=0,alpha=0;GLboolean mask[4],depthMask=0;GLfloat alphaRef=0;
    glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_SCISSOR_BOX,scissor);glGetIntegerv(GL_POLYGON_MODE,polygon);
    glGetIntegerv(GL_BLEND_SRC,&source);glGetIntegerv(GL_BLEND_DST,&destination);glGetIntegerv(GL_ALPHA_TEST_FUNC,&alpha);
    glGetFloatv(GL_ALPHA_TEST_REF,&alphaRef);glGetBooleanv(GL_COLOR_WRITEMASK,mask);glGetBooleanv(GL_DEPTH_WRITEMASK,&depthMask);
    Record("TRACE","HUD raster phase=%s viewport=%d,%d,%d,%d colorMask=%d%d%d%d depth=%d:%d stencil=%d scissor=%d:%d,%d,%d,%d cull=%d alpha=%d:%04X:%.3f blend=%d:%04X,%04X polygon=%04X,%04X stipple=%d logic=%d",
        phase,viewport[0],viewport[1],viewport[2],viewport[3],mask[0],mask[1],mask[2],mask[3],glIsEnabled(GL_DEPTH_TEST),depthMask,
        glIsEnabled(GL_STENCIL_TEST),glIsEnabled(GL_SCISSOR_TEST),scissor[0],scissor[1],scissor[2],scissor[3],glIsEnabled(GL_CULL_FACE),
        glIsEnabled(GL_ALPHA_TEST),alpha,alphaRef,glIsEnabled(GL_BLEND),source,destination,polygon[0],polygon[1],glIsEnabled(GL_POLYGON_STIPPLE),glIsEnabled(GL_COLOR_LOGIC_OP));
    GLfloat p[16],m[16];glGetFloatv(GL_PROJECTION_MATRIX,p);glGetFloatv(GL_MODELVIEW_MATRIX,m);
    Record("TRACE","HUD matrices phase=%s projection=%.3f,%.3f,%.3f,%.3f:%.3f,%.3f,%.3f,%.3f modelview=%.3f,%.3f,%.3f,%.3f:%.3f,%.3f,%.3f,%.3f",
        phase,p[0],p[5],p[10],p[15],p[12],p[13],p[14],p[11],m[0],m[5],m[10],m[15],m[12],m[13],m[14],m[11]);
    using Active=void (APIENTRY*)(GLenum);auto active=Proc<Active>("glActiveTexture");if(!active)active=Proc<Active>("glActiveTextureARB");
    GLint unit=0x84C0,units=1;if(active){glGetIntegerv(0x84E0,&unit);glGetIntegerv(0x84E2,&units);}
    bool cube=v13 || has("GL_ARB_texture_cube_map");
    for(int i=0;i<std::clamp(units,1,32);++i) {
        if(active)active(0x84C0+i);
        bool one=glIsEnabled(GL_TEXTURE_1D)!=0,two=glIsEnabled(GL_TEXTURE_2D)!=0,three=v12 && glIsEnabled(0x806F),cubed=cube && glIsEnabled(0x8513);
        if(i && unit!=0x84C0+i && !one && !two && !three && !cubed)continue;
        GLint texture=0,environment=0;GLfloat matrix[16];glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);
        glGetTexEnviv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,&environment);glGetFloatv(GL_TEXTURE_MATRIX,matrix);
        Record("TRACE","HUD texture phase=%s unit=%d active=%d targets=%d%d%d%d binding2D=%d environment=%04X matrix=%.3f,%.3f,%.3f,%.3f texgen=%d%d%d%d",
            phase,i,unit==0x84C0+i,one,two,three,cubed,texture,environment,matrix[0],matrix[5],matrix[12],matrix[13],
            glIsEnabled(GL_TEXTURE_GEN_S),glIsEnabled(GL_TEXTURE_GEN_T),glIsEnabled(GL_TEXTURE_GEN_R),glIsEnabled(GL_TEXTURE_GEN_Q));
    }
    // Sampling another texture stage is observational: restore the caller's active unit exactly.
    if(active)active(unit);
}
