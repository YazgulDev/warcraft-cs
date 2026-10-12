#include "TargetMarkerView.hpp"
#include "HealthBarView.hpp"
#include <windows.h>
#include <gl/GL.h>

bool TargetMarkerView::Draw(const UnitAimTarget& target,const WorldLabelView& view,int width,int height,float nearPlane,float farPlane) {
    if(!target.handle)return false;
    UnitHealthBar anchor={{target.position[0],target.position[1],target.position[2]},1,1,target.allied};
    auto projection=view;projection.maximumDistance=10000;
    HealthBarPlacement at;if(!HealthBarView::Project(anchor,projection,float(width),float(height),nearPlane,farPlane,at))return false;
    // The contact already passed range and occlusion. Screen brackets mark that actual body/contact, including parallax.
    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_POLYGON_BIT|GL_CURRENT_BIT);
    glDisable(GL_DEPTH_TEST);glDisable(GL_TEXTURE_2D);glDisable(GL_POLYGON_STIPPLE);glDisable(GL_COLOR_LOGIC_OP);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
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
