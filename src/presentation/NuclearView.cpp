#include "GraphicsApi.hpp"
#include "NuclearView.hpp"
#include "WorldEffectPass.hpp"
#include "HudRenderState.hpp"
#include <cmath>
#include <algorithm>

namespace {
constexpr float pi=3.14159265359f;
void Sphere(float x,float y,float z,float rx,float ry,float rz,float red,float green,float blue,float alpha){
    for(int row=0;row<10;++row){float a=-pi*.5f+pi*row/10,b=-pi*.5f+pi*(row+1)/10;
        glBegin(GL_TRIANGLE_STRIP);for(int col=0;col<=28;++col){float t=2*pi*col/28;
            for(float v:{a,b}){float shade=.52f+.48f*std::max(0.f,std::cos(v)*std::cos(t)*.35f+std::sin(v)*.8f);
                glColor4f(red*shade,green*shade,blue*shade,alpha);glVertex3f(x+rx*std::cos(v)*std::cos(t),y+ry*std::cos(v)*std::sin(t),z+rz*std::sin(v));}
        }glEnd();
    }
}
}
void NuclearView::World(const NativeNuclearBomb& bomb,const WorldLabelView& view,float nearPlane,float farPlane){
    if(bomb.Age()<0 || !view.active)return;WorldEffectPass pass(view,nearPlane,farPlane);
    glDisable(GL_TEXTURE_2D);glDisable(GL_LIGHTING);glDisable(GL_CULL_FACE);glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    auto center=bomb.Center();glTranslatef(center[0],center[1],center[2]);
    float age=bomb.Age(),growth=std::min(1.f,age/2),fade=std::clamp((bomb.Settings().effectSeconds-age)/2,0.f,1.f);
    // A rising column and billowing cap share native perspective/depth; terrain hides the far side.
    for(int i=0;i<7;++i){float t=float(i)/6;glColor4f(.25f+.55f*(1-t),.18f+.2f*(1-t),.10f,fade*.85f);
        Sphere(0,0,40+growth*(100+500*t),40+growth*(55+35*t),40+growth*(55+35*t),70+growth*90,.25f+.55f*(1-t),.18f+.2f*(1-t),.10f,fade*.85f);
    }
    for(int i=0;i<12;++i){float a=i*2*pi/12;glColor4f(.4f,.3f,.2f,fade*.9f);
        Sphere(std::cos(a)*growth*230,std::sin(a)*growth*230,100+growth*650,75+growth*130,75+growth*130,60+growth*100,.55f,.43f,.30f,fade*.9f);
    }
    glColor4f(1,.55f,.12f,fade*std::max(0.f,1-age/2));Sphere(0,0,30,80+age*230,80+age*230,30+growth*65,1,.55f,.12f,fade*std::max(0.f,1-age/2));
    glColor4f(1,.65f,.15f,fade*.7f);glBegin(GL_TRIANGLE_STRIP);
    for(int i=0;i<=96;++i){float a=i*2*pi/96;for(float radius:{age*450,age*450+20})glVertex3f(std::cos(a)*radius,std::sin(a)*radius,12);}glEnd();
}
void NuclearView::Held(int width,int height){
    HudRenderState state;GLint mode;glGetIntegerv(GL_MATRIX_MODE,&mode);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();glDisable(GL_TEXTURE_2D);
    float x=width*.66f,y=height*.88f,s=height*.07f;
    // An original radiation-marked bomb silhouette replaces the holstered gun while confirmation is armed.
    glColor4f(.25f,.29f,.20f,1);glBegin(GL_TRIANGLE_FAN);glVertex2f(x,y);
    for(int i=0;i<=40;++i){float a=i*2*pi/40;glVertex2f(x+std::cos(a)*s*.65f,y+std::sin(a)*s*1.5f);}glEnd();
    glColor4f(1,.85f,.12f,1);glBegin(GL_TRIANGLE_FAN);glVertex2f(x,y);for(int i=0;i<=32;++i){float a=i*2*pi/32;glVertex2f(x+std::cos(a)*s*.48f,y+std::sin(a)*s*.48f);}glEnd();
    glColor4f(.05f,.05f,.04f,1);for(int lobe=0;lobe<3;++lobe){glBegin(GL_TRIANGLE_FAN);glVertex2f(x,y);for(int i=0;i<=8;++i){float a=lobe*2*pi/3+i*pi/24;glVertex2f(x+std::cos(a)*s*.43f,y+std::sin(a)*s*.43f);}glEnd();}
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(mode);
}
