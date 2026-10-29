#include "NativeHeadGore.hpp"
#include "NativeArchive.hpp"
#include "GameBuild.hpp"
#include "../geometry/AnimatedHitModel.hpp"
#include <map>

namespace NativeHeadGore {
namespace {
using Skin=void (__stdcall*)(uintptr_t,uintptr_t);
Skin originalSkin=nullptr;
uintptr_t gameBase=0;
struct Rig {
    std::vector<AnimatedHitModel::HeadBone> heads;
    std::vector<std::vector<unsigned>> groups;
};
struct Corpse {Contact contact;bool logged=false;};
std::map<int,Rig> rigs;
std::map<uintptr_t,Corpse> corpses;
template<class T>bool Read(uintptr_t address,T& value) {
    SIZE_T got=0;return address && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),&value,sizeof(value),&got) && got==sizeof(value);
}
uintptr_t Pointer(uintptr_t address) {uintptr_t value=0;Read(address,value);return value;}
bool Alive(wc3::Handle unit) {return wc3::Real(wc3::GetUnitState(unit,0))>.405f;}
bool BonePoint(uintptr_t matrices,const AnimatedHitModel::HeadBone& head,std::array<float,3>& point) {
    std::array<float,12> matrix={};if(!Read(matrices+head.root*sizeof(matrix),matrix))return false;
    // Native palette matrices are row-vector 3x4 transforms in the current render space.
    for(int axis=0;axis<3;++axis) {
        point[axis]=matrix[9+axis];for(int row=0;row<3;++row)point[axis]+=head.pivot[row]*matrix[row*3+axis];
        if(!std::isfinite(point[axis]))return false;
    }
    return true;
}
unsigned HeadRoot(const Rig& rig,const std::vector<unsigned>& group,float weight) {
    std::map<unsigned,unsigned> votes;
    for(unsigned bone:group)for(const auto& head:rig.heads)if(head.id==bone){++votes[head.root];break;}
    // A shared group spanning two skulls cannot be collapsed without deforming the other head.
    if(votes.size()!=1)return 0xffffffff;
    const auto& vote=*votes.begin();return float(vote.second)/group.size()>=weight ? vote.first : 0xffffffff;
}
void Collapse(uintptr_t model,uintptr_t matrices,Corpse& corpse,const Rig& rig) {
    unsigned count=0;if(!Read(model+0x0C,count) || count>4096)return;
    uintptr_t entries=Pointer(model+0x10),output=Pointer(gameBase+0xACBC68);
    if(!entries || !output || !matrices)return;
    std::array<float,3> anchor={};bool anchored=false;
    // The accepted triangle carries the struck head's identity, independent of camera and death pose.
    for(const auto& head:rig.heads)if(head.id==corpse.contact.root)
        {anchored=BonePoint(matrices,head,anchor);break;}
    if(!anchored)return;
    std::vector<uintptr_t> palettes;
    for(unsigned i=0;i<count;++i) {
        uintptr_t entry=Pointer(entries+i*4),geometry=Pointer(entry ? entry+0x0C : 0);
        unsigned groups=0,index=0,disabled=0,geoset=0;unsigned char visible=0;
        if(!entry || !geometry || !Read(entry+0x10,disabled) || disabled || !Read(geometry+0x11C,geoset) || geoset>4096 ||
            !Read(Pointer(model+0x20)+geoset*16+3,visible) || !visible ||
            !Read(geometry+0xF0,groups) || groups>256 || !Read(entry+8,index) || index==0xffffffff)continue;
        // The native builder skips disabled/invisible geosets. Their old palette indices can now
        // belong to body parts or another unit; only this invocation's visible output is ours.
        uintptr_t sizes=Pointer(geometry+0xF4),bones=Pointer(geometry+0x100);unsigned offset=0;
        for(unsigned group=0;group<groups;++group) {
            unsigned length=0;if(!Read(sizes+group*4,length) || !length || length>32)return;
            std::vector<unsigned> ids;for(unsigned j=0;j<length;++j){unsigned id=0;if(!Read(bones+(offset+j)*4,id) || id>=4096)return;ids.push_back(id);}offset+=length;
            std::sort(ids.begin(),ids.end());
            // Refuse a palette that does not occur in this map's authored MDX; never infer a head by height alone.
            if(std::find(rig.groups.begin(),rig.groups.end(),ids)==rig.groups.end())continue;
            if(HeadRoot(rig,ids,corpse.contact.weight)!=corpse.contact.root)continue;
            if(index>0x100000 || group>0x100000-index)return;
            uintptr_t at=output+(index+group)*48;std::array<float,12> previous={};if(!Read(at,previous))return;
            palettes.push_back(at);
        }
    }
    // Native per-instance palettes are regenerated before this hook. Collapse only selected head groups;
    // shared MDX data, body groups, the second head and all other units remain untouched.
    std::array<float,12> collapsed={};for(int axis=0;axis<3;++axis)collapsed[9+axis]=anchor[axis];
    unsigned written=0;for(uintptr_t at:palettes){SIZE_T got=0;if(WriteProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(at),collapsed.data(),sizeof(collapsed),&got) && got==sizeof(collapsed))++written;}
    if(!corpse.logged) {
        corpse.logged=true;
        wc3::Log("Headshot head removal target=%08X root=%u palettes=%u anchor=%.1f,%.1f,%.1f contact=%.1f,%.1f,%.1f",
            corpse.contact.target,corpse.contact.root,written,anchor[0],anchor[1],anchor[2],
            corpse.contact.point[0],corpse.contact.point[1],corpse.contact.point[2]);
    }
}
void __stdcall SkinCorpse(uintptr_t model,uintptr_t matrices) {
    originalSkin(model,matrices);
#ifdef WCS_GAMEPLAY_TEST
    // Read-only native oracle distinguishes a progressing portrait skeleton from a stale captured image.
    auto animation=Pointer(model+0x98);unsigned clips=0,clock=0;unsigned char active=0;
    if(Read(animation+0x0C,clips) && clips==4 && Read(animation+0x58,active) && active<clips &&
        Read(Pointer(animation+8)+active*16,clock) && clock>=260000 && clock<=273333) {
        static DWORD last=0;DWORD now=GetTickCount();
        if(now-last>=1000) {
            last=now;std::array<unsigned char,48*16> bytes={};unsigned hash=2166136261u;
            if(Read(matrices,bytes)){for(auto value:bytes)hash=(hash^value)*16777619u;wc3::Log("fixture portrait skin model=%08X clock=%u hash=%08X",model,clock,hash);}
        }
    }
#endif
    auto found=corpses.find(model);if(found==corpses.end())return;
    auto& contact=found->second.contact;
    if(wc3::UnitObject(contact.target)!=contact.object || Alive(contact.target) ||
        Pointer(Pointer(contact.object+0x28)+0x20)!=model) {corpses.erase(found);return;}
    auto rig=rigs.find(contact.type);if(rig!=rigs.end())Collapse(model,matrices,found->second,rig->second);
}
}
void Install(uintptr_t base,Hook hook) {
    gameBase=base;
    if(!base || wc3::ActiveBuild()!=wc3::GameBuildKind::Classic126a)return;
    // Owned 1.26a skin palette builder: two stack arguments/ret 8. The output getter resolves
    // a native palette index as index*48 + [Game+ACBC68]; validate that layout before installing.
    const unsigned char skin[]={0x8B,0x44,0x24,0x04,0x53,0x8B,0x58,0x0C};
    const unsigned char getter[]={0x83,0xF9,0xFF,0x75,0x03,0x33,0xC0,0xC3,0x8D,0x04,0x49,0xC1,0xE0,0x04,0x03,0x05};
    bool verified=!std::memcmp(reinterpret_cast<void*>(base+0x50C8C0),skin,sizeof(skin)) &&
        !std::memcmp(reinterpret_cast<void*>(base+0x509420),getter,sizeof(getter)) && Pointer(base+0x509430)==base+0xACBC68;
    bool installed=verified && hook(reinterpret_cast<void*>(base+0x50C8C0),reinterpret_cast<void*>(SkinCorpse),reinterpret_cast<void**>(&originalSkin));
    wc3::Log("Headshot native head palette verified=%d installed=%d",verified,installed);
}
Contact Prepare(wc3::Handle target,const std::array<float,3>& point,float weight,bool enabled,unsigned root) {
    Contact contact;if(!enabled || !originalSkin || !target || !Alive(target) || wc3::IsUnitType(target,2))return contact;
    int type=wc3::GetUnitTypeId(target);auto rig=rigs.find(type);
    if(rig==rigs.end()) {
        AnimatedHitModel model;auto bytes=NativeArchive::Read(wc3::UnitModelPath(type));Rig data;
        if(model.Load(bytes)){data.heads=model.HeadBones();data.groups=model.BoneGroups();}
        rig=rigs.emplace(type,std::move(data)).first;
    }
    if(std::none_of(rig->second.heads.begin(),rig->second.heads.end(),[&](const AnimatedHitModel::HeadBone& head){return head.id==root && head.root==root;}))return contact;
    uintptr_t object=wc3::UnitObject(target),sprite=Pointer(object ? object+0x28 : 0),model=Pointer(sprite ? sprite+0x20 : 0);
    if(model)contact={target,object,model,type,point,weight,root};return contact;
}
bool Commit(const Contact& contact) {
    if(!contact.target || wc3::UnitObject(contact.target)!=contact.object || Alive(contact.target) ||
        Pointer(Pointer(contact.object+0x28)+0x20)!=contact.model)return false;
    // Corpse tracking is map-local and bounded. Native removal/respawn invalidates the stored typed identity.
    if(corpses.size()>=256)corpses.erase(corpses.begin());
    corpses[contact.model]={contact};return true;
}
void Reset() {corpses.clear();rigs.clear();}
}
