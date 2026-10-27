#include "../src/presentation/HudRenderState.hpp"
#include "../src/presentation/HudFont.hpp"
#include <vector>
#include <cassert>
#include <cstdio>

namespace {
constexpr int width=320,height=240;
template<class T>T Proc(const char* name){auto value=reinterpret_cast<T>(wglGetProcAddress(name));assert(value);return value;}
using Create=GLuint (APIENTRY*)(GLenum,GLsizei,const char* const*);
using Gen=void (APIENTRY*)(GLsizei,GLuint*);
using Bind=void (APIENTRY*)(GLuint);
using Stages=void (APIENTRY*)(GLuint,GLbitfield,GLuint);
using Query=void (APIENTRY*)(GLuint,GLenum,GLint*);
void Clear(){glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);}
std::vector<unsigned char> Pixels(){std::vector<unsigned char> bytes(width*height*4);glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,bytes.data());return bytes;}
int Ink(const std::vector<unsigned char>& pixels){int count=0;for(size_t i=0;i<pixels.size();i+=4)if(pixels[i]>20 || pixels[i+1]>20 || pixels[i+2]>20)++count;return count;}
void Weapon(){
    // Project-owned triangle stands in for the held mesh's immediate-mode fixed-function batch.
    glDisable(GL_TEXTURE_2D);glColor4f(.7f,.5f,.2f,1);
    glBegin(GL_TRIANGLES);glVertex2f(210,220);glVertex2f(300,220);glVertex2f(275,140);glEnd();
}
void Hud(HudFont& font){
    glDisable(GL_TEXTURE_2D);glColor4f(.2f,1,.3f,1);
    glBegin(GL_QUADS);glVertex2f(150,118);glVertex2f(170,118);glVertex2f(170,122);glVertex2f(150,122);glEnd();
    glColor4f(1,1,1,1);font.Draw(10,35,"HP 100 | 12 / 100 | $800");
}
}
int main(){
    _set_error_mode(_OUT_TO_STDERR);
    HWND window=CreateWindowA("STATIC","Fixed function state",WS_POPUP,0,0,width,height,nullptr,nullptr,nullptr,nullptr);assert(window);
    HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR format={};format.nSize=sizeof(format);format.nVersion=1;
    format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=24;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&format),&format));
    for(int cycle=0;cycle<3;++cycle){
        HGLRC context=wglCreateContext(dc);assert(context && wglMakeCurrent(dc,context));
        glViewport(0,0,width,height);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,width,height,0,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
        HudFont font;assert(font.Ensure(18));Clear();Weapon();{HudRenderState state;Hud(font);}auto expected=Pixels();assert(Ink(expected)>3000);
        auto create=Proc<Create>("glCreateShaderProgramv");auto gen=Proc<Gen>("glGenProgramPipelines");
        auto pipelineBind=Proc<Bind>("glBindProgramPipeline"),use=Proc<Bind>("glUseProgram");
        auto stages=Proc<Stages>("glUseProgramStages");auto query=Proc<Query>("glGetProgramiv");
        const char* shader="#version 330 compatibility\nvoid main(){gl_FragColor=vec4(0,0,0,1);}";
        GLuint program=create(0x8B30,1,&shader),pipeline=0;GLint linked=0;query(program,0x8B82,&linked);assert(linked);
        gen(1,&pipeline);stages(pipeline,0x2,program);pipelineBind(pipeline);use(0);
        // Reproduce the report's zero-error, CURRENT_PROGRAM=0, invisible HUD/weapon symptom.
        GLint value=-1;glGetIntegerv(0x8B8D,&value);assert(value==0);glGetIntegerv(0x825A,&value);assert(GLuint(value)==pipeline);
        Clear();Weapon();Hud(font);assert(Ink(Pixels())==0 && glGetError()==GL_NO_ERROR);
        Clear();{FixedFunctionState state;Weapon();{HudRenderState hud;Hud(font);}}assert(Pixels()==expected);
        glGetIntegerv(0x825A,&value);assert(GLuint(value)==pipeline);glGetIntegerv(0x8B8D,&value);assert(value==0);
        // Standalone HUD scopes also isolate a conventional GLSL program and restore it exactly.
        pipelineBind(0);use(program);Clear();{HudRenderState state;Weapon();Hud(font);}assert(Pixels()==expected);
        glGetIntegerv(0x8B8D,&value);assert(GLuint(value)==program);use(0);
        // Discard can suppress clears too; the owned pass must establish its state before clearing/drawing.
        glEnable(0x8C89);{FixedFunctionState state;Clear();Weapon();{HudRenderState hud;Hud(font);}}assert(Pixels()==expected && glIsEnabled(0x8C89));glDisable(0x8C89);
        assert(glGetError()==GL_NO_ERROR);font.Reset(true);
        std::printf("PASS context %d: invisible pipeline reproduction, exact weapon/reticle/font pixels, program/pipeline/discard restoration (%s).\n",cycle,glGetString(GL_RENDERER));
        wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);
    }
    ReleaseDC(window,dc);DestroyWindow(window);
}
