#include "MuzzleFlashView.hpp"
#include <windows.h>
#include <gl/GL.h>
#include <cmath>

void MuzzleFlashView::Draw(float x,float y,float z,float strength,float scale,unsigned serial) {
    if(strength<=0 || scale<=0)return;
    // The shared viewmodel matrix follows bob/recoil. Keep flash depth read-only and restore inherited GL state.
    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_POLYGON_BIT|GL_CURRENT_BIT|GL_TEXTURE_BIT);
    glDisable(GL_TEXTURE_2D);glDisable(GL_ALPHA_TEST);glDisable(GL_LIGHTING);glDisable(GL_FOG);glDisable(GL_CULL_FACE);
    glDisable(GL_POLYGON_STIPPLE);glDisable(GL_COLOR_LOGIC_OP);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_FALSE);
    float radius=2.8f*scale,rotation=float(serial%97)*.77f;
    glBegin(GL_TRIANGLE_FAN);glColor4f(1,1,.8f,strength);glVertex3f(x,y,z+.05f);
    for(int i=0;i<=12;++i) {
        float angle=rotation+i*6.283185307f/12,reach=radius*(i%2 ? .42f : 1.f);
        glColor4f(1,.32f,.025f,0);glVertex3f(x+std::cos(angle)*reach,y+std::sin(angle)*reach,z+.05f);
    }
    glEnd();
    // A white-hot core keeps the flash readable against bright terrain without opaque rectangular edges.
    glBegin(GL_TRIANGLE_FAN);glColor4f(1,1,.95f,strength);glVertex3f(x,y,z+.06f);
    for(int i=0;i<=8;++i) {
        float angle=i*6.283185307f/8;
        glColor4f(1,.75f,.15f,0);glVertex3f(x+std::cos(angle)*radius*.3f,y+std::sin(angle)*radius*.3f,z+.06f);
    }
    glEnd();glPopAttrib();
}
