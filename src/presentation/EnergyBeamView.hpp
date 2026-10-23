#pragma once
#include "EnergyBeam.hpp"
#include "WorldEffectPass.hpp"
#include "HudRenderState.hpp"
#include <algorithm>

namespace EnergyBeamView {
inline void Draw(const EnergyBeam& beam,const WorldLabelView& view,float nearPlane,float farPlane) {
    float age=(GetTickCount()-beam.tick)*.001f;
    if(!beam.tick || age>beam.seconds || !view.active || nearPlane<=0 || farPlane<=nearPlane)return;
    HudRenderState raster; // Reuse complete blend/coverage/texture isolation, then restore world depth/projection.
    WorldEffectPass pass(view,nearPlane,farPlane);
    // Original additive geometry uses native scene depth and never writes depth or loads guest sprites.
    glDisable(GL_TEXTURE_2D);glDisable(GL_ALPHA_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glDepthMask(GL_FALSE);
    float direction[3],towards[3],side[3];
    for(int i=0;i<3;++i){direction[i]=beam.end[i]-beam.start[i];towards[i]=(beam.end[i]+beam.start[i])*.5f-view.eye[i];}
    side[0]=direction[1]*towards[2]-direction[2]*towards[1];
    side[1]=direction[2]*towards[0]-direction[0]*towards[2];side[2]=direction[0]*towards[1]-direction[1]*towards[0];
    float length=std::sqrt(side[0]*side[0]+side[1]*side[1]+side[2]*side[2]);
    if(length<.001f){float yaw=view.yaw*.01745329252f;side[0]=std::sin(yaw);side[1]=-std::cos(yaw);side[2]=0;length=1;}
    for(auto& value:side)value*=beam.width/length; // Face the camera without collapsing a horizontal beam into a line.
    float alpha=1-age/beam.seconds;
    for(int layer=0;layer<2;++layer) {
        float scale=layer ? .3f : 1;
        glColor4f(layer ? 1.f : beam.gluon ? .2f : 1.f,layer ? 1.f : beam.gluon ? .4f : .55f,layer || beam.gluon ? 1.f : .1f,alpha);
        glBegin(GL_QUADS);
        for(int i=0;i<4;++i) {
            const auto& point=i<2 ? beam.start : beam.end;float sign=(i==0 || i==3 ? -scale : scale)*(i<2 ? .15f : 1.f);
            // Taper at the nearby muzzle: constant world width otherwise magnifies into an opaque screen-wide wedge.
            glVertex3f(point[0]+side[0]*sign,point[1]+side[1]*sign,point[2]+side[2]*sign);
        }
        glEnd();
    }
}
}
