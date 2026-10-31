#include "GraphicsApi.hpp"
#include "GrenadeView.hpp"
#include "../combat/WeaponSlots.hpp"
#include "../combat/SmokeVisibility.hpp"
#include <windows.h>
#include <gl/GL.h>
#include <cmath>

namespace {
struct Placement {float x,y,z,radius;};
bool Project(const GrenadePoint& point,float radius,const WorldLabelView& v,int w,int h,float nearPlane,float farPlane,Placement& out) {
    float yaw=v.yaw*.01745329252f,pitch=v.pitch*.01745329252f;
    float x=point[0]-v.eye[0],y=point[1]-v.eye[1],z=point[2]-v.eye[2];
    float along=x*std::cos(yaw)+y*std::sin(yaw),right=x*std::sin(yaw)-y*std::cos(yaw);
    float depth=along*std::cos(pitch)+z*std::sin(pitch),up=z*std::cos(pitch)-along*std::sin(pitch);
    if(depth<=nearPlane || depth>=farPlane)return false;
    float tangent=std::tan(v.verticalFov*.01745329252f*.5f);
    out={w*.5f*(1+right/(depth*tangent*v.aspect)),h*.5f*(1-up/(depth*tangent)),
        1-2*(farPlane/(farPlane-nearPlane)-farPlane*nearPlane/((farPlane-nearPlane)*depth)),h*.5f*radius/(depth*tangent)};
    return out.x+out.radius>=0 && out.x-out.radius<=w && out.y+out.radius>=0 && out.y-out.radius<=h;
}
void Disc(const Placement& p,float red,float green,float blue,float alpha,bool soft) {
    glBegin(GL_TRIANGLE_FAN);glColor4f(red,green,blue,alpha);glVertex3f(p.x,p.y,p.z);
    for(int i=0;i<=20;++i){float angle=i*6.283185307f/20;glColor4f(red,green,blue,soft ? 0 : alpha);glVertex3f(p.x+std::cos(angle)*p.radius,p.y+std::sin(angle)*p.radius,p.z);}
    glEnd();
}
void Begin(int width,int height) {
    glPushAttrib(GL_ALL_ATTRIB_BITS);glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    glDisable(GL_TEXTURE_2D);glDisable(GL_LIGHTING);glDisable(GL_FOG);glDisable(GL_CULL_FACE);glDisable(GL_ALPHA_TEST);
    glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_POLYGON_STIPPLE);glDisable(GL_COLOR_LOGIC_OP);
    for(int i=0;i<6;++i)glDisable(GL_CLIP_PLANE0+i);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);glColorMask(1,1,1,1);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
}
void End(GLint mode) {glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glPopAttrib();glMatrixMode(mode);}
}
void GrenadeView::Draw(const std::vector<GrenadeVisual>& items,const WorldLabelView& view,int width,int height,float nearPlane,float farPlane) {
    if(items.empty() || !view.active || width<=0 || height<=0)return;
    GLint mode;glGetIntegerv(GL_MATRIX_MODE,&mode);Begin(width,height);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthRange(0,1);
    float yaw=view.yaw*.01745329252f,pitch=view.pitch*.01745329252f;
    GrenadePoint forward={std::cos(yaw)*std::cos(pitch),std::sin(yaw)*std::cos(pitch),std::sin(pitch)};
    for(const auto& item:items) {
        if(!item.smoke || item.opacity<=0 || item.radius<=0)continue;
        auto center=SmokeVisibility::Center(item);
        // Camera-facing slices integrate fog only in front of world depth. Nearby geometry stays readable,
        // while even a short path through the cloud extinguishes distant silhouettes in both renderers.
        int slices=std::clamp(int(std::ceil(2*item.radius/16)),8,128);
        float step=2*item.radius/slices;
        float alpha=1-std::exp(-SmokeVisibility::Density(item)*step);
        for(int i=0;i<slices;++i) {
            float offset=-item.radius+(i+.5f)*step;
            GrenadePoint point=center;for(int axis=0;axis<3;++axis)point[axis]+=forward[axis]*offset;
            float radius=std::sqrt(std::max(0.f,item.radius*item.radius-offset*offset));
            Placement at;if(Project(point,radius,view,width,height,nearPlane,farPlane,at))Disc(at,.46f,.47f,.48f,alpha,false);
        }
        // Account for the eye-to-first-slice gap when the near plane is inside smoke, rather than
        // adding a distance-independent full-screen blanket that would also hide one's own feet.
        GrenadePoint eye={view.eye[0],view.eye[1],view.eye[2]},to=eye;
        for(int axis=0;axis<3;++axis)to[axis]+=forward[axis]*(nearPlane+step*.5f);
        float length=SmokeVisibility::Length(item,eye,to);
        float eyeDistance=0;for(int axis=0;axis<3;++axis)eyeDistance+=(eye[axis]-center[axis])*(eye[axis]-center[axis]);
        if(length>0 && eyeDistance<item.radius*item.radius) {
            glDisable(GL_DEPTH_TEST);glColor4f(.46f,.47f,.48f,1-std::exp(-SmokeVisibility::Density(item)*length));
            glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(float(width),0);glVertex2f(float(width),float(height));glVertex2f(0,float(height));glEnd();glEnable(GL_DEPTH_TEST);
        }
    }
    End(mode);
}
void GrenadeView::Flash(float opacity,int width,int height) {
    if(opacity<=0 || width<=0 || height<=0)return;
    GLint mode;glGetIntegerv(GL_MATRIX_MODE,&mode);Begin(width,height);glDisable(GL_DEPTH_TEST);
    glColor4f(1,1,1,std::clamp(opacity,0.f,1.f));glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(float(width),0);glVertex2f(float(width),float(height));glVertex2f(0,float(height));glEnd();End(mode);
}
