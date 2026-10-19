#include "../src/presentation/HudRenderState.hpp"
#include "../src/presentation/HudFont.hpp"
#include "../src/presentation/UnitHudView.hpp"
#include "../src/platform/NativePortrait.hpp"
#include <vector>
#include <cassert>
#include <cstdio>

// Portrait pixels are tested separately; use the production bars, labels and glyph rendering here.
void NativePortrait::Draw(int,int,int) {}
namespace {
constexpr int width=640,height=240;
using Equation=void (APIENTRY*)(GLenum);
using Active=void (APIENTRY*)(GLenum);
void Clear() {glColorMask(1,1,1,1);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);}
std::vector<unsigned char> Pixels() {
    std::vector<unsigned char> result(width*height*4);glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,result.data());return result;
}
int Ink(const std::vector<unsigned char>& pixels) {
    int count=0;for(size_t i=0;i<pixels.size();i+=4)if(pixels[i]>10 || pixels[i+1]>10 || pixels[i+2]>10)++count;return count;
}
void Render(HudFont& font) {
    UnitHudSnapshot unit;unit.actor=1;unit.health=80;unit.maxHealth=100;unit.mana=30;unit.maxMana=50;unit.hero=true;unit.level=1;
    NativePortrait portrait;
    UnitHudView::Draw(unit,UnitHudLayout::Calculate(width,height,100),portrait,[&](float x,float y,const char* s){font.Draw(x,y,s);});
    glColor4f(1,.85f,.3f,1);font.Draw(12,30,"F6 FPS | F7 THIRD PERSON | HP MANA EXP");
    glDisable(GL_TEXTURE_2D);glColor4f(.5f,1,.25f,1);
    glBegin(GL_QUADS);glVertex2f(310,118);glVertex2f(330,118);glVertex2f(330,120);glVertex2f(310,120);glEnd();
}
}
int main() {
    HWND window=CreateWindowA("STATIC","HUD raster regression",WS_POPUP,0,0,width,height,nullptr,nullptr,nullptr,nullptr);assert(window);
    HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR format={};format.nSize=sizeof(format);format.nVersion=1;
    format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=32;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&format),&format));
    for(int cycle=0;cycle<3;++cycle) {
        HGLRC context=wglCreateContext(dc);assert(context && wglMakeCurrent(dc,context));
        auto equation=reinterpret_cast<Equation>(wglGetProcAddress("glBlendEquation"));
        if(!equation)equation=reinterpret_cast<Equation>(wglGetProcAddress("glBlendEquationEXT"));assert(equation);
        auto active=reinterpret_cast<Active>(wglGetProcAddress("glActiveTexture"));assert(active);
        std::printf("HUD raster context %d: %s\n",cycle,glGetString(GL_RENDERER));
        glViewport(0,0,width,height);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,width,height,0,-1,1);
        glMatrixMode(GL_MODELVIEW);glLoadIdentity();HudFont font;assert(font.Ensure(18) && font.Ready());
        Clear();{HudRenderState state;Render(font);}auto expected=Pixels();assert(Ink(expected)>3000);
        // The former pass only sets blend factors: a legal subtract equation makes the entire flat HUD black.
        equation(0x800B);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        Clear();Render(font);assert(Ink(Pixels())==0);
        Clear();{HudRenderState state;Render(font);}assert(Pixels()==expected);
        GLint value=0;glGetIntegerv(0x8009,&value);assert(value==0x800B);
        // Compound world state must neither hide bars/text nor leak into the game's next frame.
        Clear();glEnable(GL_DEPTH_TEST);glDepthFunc(GL_NEVER);glEnable(GL_STENCIL_TEST);glStencilFunc(GL_NEVER,0,255);
        glEnable(GL_SCISSOR_TEST);glScissor(0,0,0,0);glEnable(GL_CULL_FACE);glCullFace(GL_FRONT_AND_BACK);
        glEnable(GL_COLOR_LOGIC_OP);glLogicOp(GL_NOOP);glEnable(GL_POLYGON_SMOOTH);
        glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);glColorMask(0,0,0,0);
        glEnable(0x809E);glEnable(0x809F);glEnable(0x80A0);
        active(0x84C1);glEnable(GL_TEXTURE_2D);glEnable(0x806F);glEnable(0x8513);
        glMatrixMode(GL_TEXTURE);glTranslatef(.25f,.5f,0);glMatrixMode(GL_MODELVIEW);
        {HudRenderState state;Render(font);}assert(Pixels()==expected);
        glGetIntegerv(0x84E0,&value);assert(value==0x84C1);
        assert(glIsEnabled(0x806F) && glIsEnabled(0x8513) && glIsEnabled(GL_DEPTH_TEST) && glIsEnabled(GL_SCISSOR_TEST));
        assert(glIsEnabled(0x809E) && glIsEnabled(0x809F) && glIsEnabled(0x80A0));
        GLboolean mask[4];glGetBooleanv(GL_COLOR_WRITEMASK,mask);assert(!mask[0] && !mask[3]);
        GLfloat matrix[16];glGetFloatv(GL_TEXTURE_MATRIX,matrix);assert(matrix[12]==.25f && matrix[13]==.5f);
        glGetIntegerv(0x8009,&value);assert(value==0x800B);assert(glGetError()==GL_NO_ERROR);
        font.Reset(true);assert(!font.Ready());wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);
    }
    ReleaseDC(window,dc);DestroyWindow(window);
    puts("PASS blank-HUD reproduction, production bars/glyphs/reticle pixels, texture/raster/MSAA/blend isolation and three contexts.");
}
