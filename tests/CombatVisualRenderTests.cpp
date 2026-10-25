#include "../src/presentation/CombatVisualView.hpp"
#include "../src/presentation/WorldEffectPass.hpp"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <vector>
namespace {
constexpr int width=800,height=600;
std::vector<unsigned char> Pixels(){std::vector<unsigned char> p(width*height*4);glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,p.data());return p;}
void Box(float x,float y,float z,float sx,float sy,float sz){
    glPushMatrix();glTranslatef(x,y,z);glScalef(sx,sy,sz);
    const float v[8][3]={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
    const int faces[6][4]={{0,1,2,3},{4,5,6,7},{0,1,5,4},{2,3,7,6},{0,3,7,4},{1,2,6,5}};
    glBegin(GL_QUADS);for(const auto& f:faces)for(int i:f)glVertex3fv(v[i]);glEnd();glPopMatrix();
}
void Clear(float depth){glColorMask(1,1,1,1);glDepthMask(GL_TRUE);glClearColor(0,0,0,1);glClearDepth(depth);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);}
void Scene(const WorldLabelView& view){WorldEffectPass pass(view,8,2000);glDisable(GL_TEXTURE_2D);
    glColor3f(.12f,.18f,.25f);Box(180,0,40,12,18,40);glColor3f(.22f,.28f,.34f);Box(160,0,96,8,10,10);}
void Save(const char* name){auto p=Pixels();for(size_t i=0;i<p.size();i+=4)std::swap(p[i],p[i+2]);BITMAPFILEHEADER f={};BITMAPINFOHEADER b={};
    f.bfType=0x4d42;f.bfOffBits=sizeof(f)+sizeof(b);f.bfSize=f.bfOffBits+unsigned(p.size());b.biSize=sizeof(b);b.biWidth=width;b.biHeight=height;b.biPlanes=1;b.biBitCount=32;
    std::ofstream out(name,std::ios::binary);out.write(reinterpret_cast<char*>(&f),sizeof(f));out.write(reinterpret_cast<char*>(&b),sizeof(b));out.write(reinterpret_cast<char*>(p.data()),p.size());}
}
int main(){
    _set_error_mode(_OUT_TO_STDERR);
    HWND window=CreateWindowA("STATIC","Combat effects oracle",WS_POPUP,0,0,width,height,nullptr,nullptr,nullptr,nullptr);assert(window);HDC dc=GetDC(window);
    PIXELFORMATDESCRIPTOR fmt={};fmt.nSize=sizeof(fmt);fmt.nVersion=1;fmt.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;fmt.iPixelType=PFD_TYPE_RGBA;fmt.cColorBits=32;fmt.cDepthBits=24;
    assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&fmt),&fmt));CombatVisualView renderer;
    for(int cycle=0;cycle<2;++cycle){HGLRC context=wglCreateContext(dc);assert(context && wglMakeCurrent(dc,context));glViewport(0,0,width,height);renderer.Reset(false);
        WorldLabelView view;view.active=true;view.eye[2]=96;view.aspect=float(width)/height;
        CombatVisualSettings settings;CombatVisuals effects;effects.Headshot(1,{151,-3,96},{160,0,124},{1,0,0},settings);
        effects.Eject(0,{0,0,96},{1,0,0},{0,0,0},0,settings);effects.Step(.11f);
        Clear(1);Scene(view);renderer.Draw(effects,view,width,height,8,2000);auto expected=Pixels();int red=0,brass=0;
        for(size_t i=0;i<expected.size();i+=4){red+=expected[i]>100 && expected[i+1]<80;brass+=expected[i]>120 && expected[i+1]>80 && expected[i+2]<80;}
        int criticalInk=0;for(int y=200;y<265;++y)for(int x=300;x<500;++x){size_t i=(size_t(height-1-y)*width+x)*4;criticalInk+=expected[i]>100 && expected[i+1]<100;}
        assert(red>40 && brass>5 && criticalInk>10 && renderer.Valid());if(!cycle)Save("combat-effects-preview.bmp");
        Clear(1);Scene(view);glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);glColorMask(0,0,0,0);glMatrixMode(GL_PROJECTION);
        renderer.Draw(effects,view,width,height,8,2000);assert(Pixels()==expected);GLint mode=0;glGetIntegerv(GL_MATRIX_MODE,&mode);assert(mode==GL_PROJECTION);
        GLboolean mask[4];glGetBooleanv(GL_COLOR_WRITEMASK,mask);assert(!mask[0]);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        Clear(0);renderer.Draw(effects,view,width,height,8,2000);auto hidden=Pixels();int lit=0;for(size_t i=0;i<hidden.size();i+=4)lit+=hidden[i] || hidden[i+1] || hidden[i+2];assert(!lit);
        effects.Reset();Clear(1);renderer.Draw(effects,view,width,height,8,2000);assert(Pixels()==hidden);
        renderer.Reset(true);wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);
    }
    ReleaseDC(window,dc);DestroyWindow(window);std::puts("Real WGL: world blood, brass cases, Cyrillic critical text, native-depth occlusion, raster restoration and context replacement passed.");
}
