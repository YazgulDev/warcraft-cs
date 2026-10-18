#include "GrenadeWorldView.hpp"
#include "../combat/WeaponSlots.hpp"
#include "../platform/WarcraftApi.hpp"
#include <cmath>

void GrenadeWorldView::Reset(bool deleteTextures) {for(auto& model:models_)model.Reset(deleteTextures);loaded_.fill(false);attempted_.fill(false);}
bool GrenadeWorldView::Valid() const {for(int i=0;i<3;++i)if(loaded_[i] && !models_[i].TexturesValid())return false;return true;}
unsigned GrenadeWorldView::Draw(const std::string& root,const std::vector<GrenadeVisual>& items,const WorldLabelView& view,int width,int height,float nearPlane,float farPlane) {
    if(!view.active || width<=0 || height<=0 || nearPlane<=0 || farPlane<=nearPlane)return 0;
    bool flight=false;for(const auto& item:items)flight|=!item.smoke && WeaponSlots::Grenade(item.slot);if(!flight)return 0;
    GLint mode;glGetIntegerv(GL_MATRIX_MODE,&mode);glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();
    float top=std::tan(view.verticalFov*.01745329252f*.5f)*nearPlane;
    glFrustum(-top*view.aspect,top*view.aspect,-top,top,nearPlane,farPlane);
    float cy=std::cos(view.yaw*.01745329252f),sy=std::sin(view.yaw*.01745329252f),cp=std::cos(view.pitch*.01745329252f),sp=std::sin(view.pitch*.01745329252f);
    // Rows are camera right/up/back. Unlike held weapons, world props retain their original xyz axes.
    float matrix[]={sy,-cy*sp,-cy*cp,0,-cy,-sy*sp,-sy*cp,0,0,cp,-sp,0,
        -sy*view.eye[0]+cy*view.eye[1],cy*sp*view.eye[0]+sy*sp*view.eye[1]-cp*view.eye[2],cy*cp*view.eye[0]+sy*cp*view.eye[1]+sp*view.eye[2],1};
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadMatrixf(matrix);
    glDisable(GL_LIGHTING);glDisable(GL_FOG);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);glDisable(GL_SCISSOR_TEST);glDisable(GL_STENCIL_TEST);
    glDisable(GL_POLYGON_STIPPLE);glDisable(GL_COLOR_LOGIC_OP);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);glColorMask(1,1,1,1);
    for(int i=0;i<6;++i)glDisable(GL_CLIP_PLANE0+i);
    glEnable(GL_ALPHA_TEST);glAlphaFunc(GL_GREATER,.1f);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_TRUE);glDepthRange(0,1);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);unsigned submitted=0;
    for(const auto& item:items) {
        if(item.smoke || !WeaponSlots::Grenade(item.slot))continue;int index=item.slot-WeaponSlots::HE;
        if(!attempted_[index]) {
            attempted_[index]=true;auto path=root+"/assets/world/"+WeaponSlots::Caches[item.slot]+".wcg";
            loaded_[index]=models_[index].Load(path);
            wc3::Log("grenade world model slot=%d loaded=%d path=%s",item.slot,loaded_[index],path.c_str());
        }
        if(!loaded_[index])continue;
        glPushMatrix();glTranslatef(item.position[0],item.position[1],item.position[2]);glRotatef(item.yaw,0,0,1);glRotatef(item.spin,0,1,0);
        glScalef(item.modelScale,item.modelScale,item.modelScale);models_[index].DrawWorld();glPopMatrix();++submitted;
    }
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glPopAttrib();glMatrixMode(mode);return submitted;
}
