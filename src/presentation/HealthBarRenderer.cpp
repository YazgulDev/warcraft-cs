#include "GraphicsApi.hpp"
#include "HealthBarRenderer.hpp"
#include <windows.h>
#include <gl/GL.h>

unsigned HealthBarRenderer::Draw(const std::vector<UnitHealthBar>& bars,const WorldLabelView& view,int width,int height,float nearPlane,float farPlane) {
    if(bars.empty() || !view.active) return 0;
    GLint mode;glGetIntegerv(GL_MATRIX_MODE,&mode);glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    glDisable(GL_TEXTURE_2D);glDisable(GL_LIGHTING);glDisable(GL_FOG);glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_BLEND);
    glDisable(GL_POLYGON_STIPPLE);glDisable(GL_COLOR_LOGIC_OP);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    for(int plane=0;plane<6;++plane) glDisable(GL_CLIP_PLANE0+plane);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);glDepthRange(0,1);
    unsigned submitted=0;float scale=std::clamp(height/1080.f,.65f,2.f),halfWidth=48*scale,halfHeight=5*scale;
    for(const auto& bar:bars) {
        HealthBarPlacement at;if(!HealthBarView::Project(bar,view,float(width),float(height),nearPlane,farPlane,at)) continue;
        // Constant screen size keeps distant bars readable; Z still belongs to their world anchor.
        float left=at.x-halfWidth,top=at.y-halfHeight,z=1-2*at.depth;
        auto quad=[&](float x,float y,float w,float h) {
            glBegin(GL_QUADS);glVertex3f(x,y,z);glVertex3f(x+w,y,z);glVertex3f(x+w,y+h,z);glVertex3f(x,y+h,z);glEnd();
        };
        glColor3f(.03f,.03f,.03f);quad(left-scale,top-scale,halfWidth*2+scale*2,halfHeight*2+scale*2);
        glColor3f(.18f,.18f,.18f);quad(left,top,halfWidth*2,halfHeight*2);
        if(bar.ally) glColor3f(.2f,.9f,.25f);else glColor3f(.95f,.2f,.15f);
        quad(left,top,halfWidth*2*at.fraction,halfHeight*2);++submitted;
    }
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(mode);glPopAttrib();
    return submitted;
}
