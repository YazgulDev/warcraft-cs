#include "TargetMarkerView.hpp"
#include "HealthBarView.hpp"
#include <windows.h>
#include <gl/GL.h>

bool TargetMarkerView::Draw(const UnitAimTarget& target,const WorldLabelView& view,int width,int height,float nearPlane,float farPlane) {
    if(!target.handle)return false;
    UnitHealthBar anchor={{target.position[0],target.position[1],target.position[2]},1,1,target.allied};
    auto projection=view;projection.maximumDistance=10000;
    HealthBarPlacement at;bool brackets=HealthBarView::Project(anchor,projection,float(width),float(height),nearPlane,farPlane,at);
    // The contact already passed range and occlusion. Screen brackets mark that actual body/contact, including parallax.
    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_POLYGON_BIT|GL_CURRENT_BIT|GL_DEPTH_BUFFER_BIT);
    glDisable(GL_TEXTURE_2D);glDisable(GL_POLYGON_STIPPLE);glDisable(GL_COLOR_LOGIC_OP);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    // Ground vertices retain world depth, so feet/opaque scenery can cover the ring naturally.
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_FALSE);glDepthRange(0,1);
    if(target.enemy)glColor3f(1,.15f,.1f);else glColor3f(1,.85f,.1f);
    std::array<HealthBarPlacement,32> ring;std::array<bool,32> shown{};
    for(size_t i=0;i<ring.size();++i) {
        UnitHealthBar sample={{target.circle[i][0],target.circle[i][1],target.circle[i][2]},1,1,false};
        shown[i]=HealthBarView::Project(sample,projection,float(width),float(height),nearPlane,farPlane,ring[i]);
    }
    // Screen-space ribbon thickness stays readable while the projected circle follows the map's slope.
    float thickness=std::clamp(height/540.f,1.5f,4.f);glBegin(GL_QUADS);
    for(size_t i=0;i<ring.size();++i) {
        size_t j=(i+1)%ring.size();if(!shown[i] || !shown[j])continue;
        float dx=ring[j].x-ring[i].x,dy=ring[j].y-ring[i].y,length=std::hypot(dx,dy);
        if(length<.001f)continue;float nx=-dy/length*thickness*.5f,ny=dx/length*thickness*.5f;
        glVertex3f(ring[i].x+nx,ring[i].y+ny,1-2*ring[i].depth);
        glVertex3f(ring[j].x+nx,ring[j].y+ny,1-2*ring[j].depth);
        glVertex3f(ring[j].x-nx,ring[j].y-ny,1-2*ring[j].depth);
        glVertex3f(ring[i].x-nx,ring[i].y-ny,1-2*ring[i].depth);
    }
    glEnd();glDisable(GL_DEPTH_TEST);
    if(!brackets){glPopAttrib();return true;}
    if(target.destructable)glColor3f(1,.8f,.2f);else if(target.allied)glColor3f(.2f,1,.3f);else glColor3f(1,.25f,.15f);
    float size=18*std::clamp(height/1080.f,.65f,2.f),stroke=2;
    auto rectangle=[](float x,float y,float w,float h) {
        glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);
    };
    glBegin(GL_QUADS);
    for(int x:{-1,1})for(int y:{-1,1}) {
        float left=at.x+x*size,top=at.y+y*size;
        rectangle(left-(x>0 ? size*.45f : 0),top,size*.45f,stroke);
        rectangle(left,top-(y>0 ? size*.45f : 0),stroke,size*.45f);
    }
    glEnd();glPopAttrib();return true;
}
