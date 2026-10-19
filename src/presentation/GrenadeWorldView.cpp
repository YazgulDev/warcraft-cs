#include "GrenadeWorldView.hpp"
#include "WorldEffectPass.hpp"
#include "../combat/WeaponSlots.hpp"
#include "../platform/WarcraftApi.hpp"
#include <cmath>

void GrenadeWorldView::Reset(bool deleteTextures) {for(auto& model:models_)model.Reset(deleteTextures);loaded_.fill(false);attempted_.fill(false);}
bool GrenadeWorldView::Valid() const {for(int i=0;i<3;++i)if(loaded_[i] && !models_[i].TexturesValid())return false;return true;}
unsigned GrenadeWorldView::Draw(const std::string& root,const std::vector<GrenadeVisual>& items,const WorldLabelView& view,int width,int height,float nearPlane,float farPlane) {
    if(!view.active || width<=0 || height<=0 || nearPlane<=0 || farPlane<=nearPlane)return 0;
    bool flight=false;for(const auto& item:items)flight|=!item.smoke && WeaponSlots::Grenade(item.slot);if(!flight)return 0;
    WorldEffectPass pass(view,nearPlane,farPlane);unsigned submitted=0;
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
    return submitted;
}
