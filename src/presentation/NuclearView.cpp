#include "GraphicsApi.hpp"
#include "NuclearView.hpp"
#include "WorldEffectPass.hpp"
#include <algorithm>
#include <cmath>

void NuclearView::World(const NativeNuclearBomb& bomb,const WorldLabelView& view,float nearPlane,float farPlane){
    float age=bomb.Age();if(age<0 || age>1 || !view.active)return;WorldEffectPass pass(view,nearPlane,farPlane);
    glDisable(GL_TEXTURE_2D);glDepthMask(GL_FALSE);glDisable(GL_CULL_FACE);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    auto center=bomb.Center();glTranslatef(center[0],center[1],center[2]);
    // The shock front stops at the local configured radius; native effects supply explosion/flames/smoke.
    float radius=std::min(bomb.Settings().radius,bomb.Settings().visualRadius)*age,alpha=(1-age)*.7f;
    glColor4f(1,.6f,.2f,alpha);glBegin(GL_TRIANGLE_STRIP);
    for(int i=0;i<=96;++i){float a=i*6.2831853f/96;for(float r:{std::max(0.f,radius-24),radius})glVertex3f(std::cos(a)*r,std::sin(a)*r,8);}glEnd();
}
