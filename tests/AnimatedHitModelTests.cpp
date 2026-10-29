#include "MdxHitFixture.hpp"
#include "../src/geometry/AnimatedHitModel.hpp"
#include "../src/geometry/RayEllipsoid.hpp"
#include "../src/presentation/GameText.hpp"
#include <cassert>
#include <cstdio>

int main() {
    _set_error_mode(_OUT_TO_STDERR);
    AnimatedHitModel model;auto bytes=MdxHitFixture::Model();assert(model.Load(bytes));
    assert(model.TriangleCount()==2 && model.HeadVertices()==3);
    // Native corpse presentation consumes authored IDs, while ray skinning keeps its resolved node indices.
    auto heads=model.HeadBones();auto groups=model.BoneGroups();
    assert(heads.size()==1 && heads[0].id==1 && heads[0].root==1 && !groups.empty());
    float ray[]={1,0,0},body[]={-100,0,50},head[]={-100,0,110},empty[]={-100,50,110};
    AnimatedHitModel::Contact hit;assert(model.Intersect(body,ray,200,0,500,hit) && !hit.head && hit.distance==100);
    assert(model.Intersect(head,ray,200,0,0,hit) && hit.head && hit.distance==100);
    assert(model.Intersect(head,ray,200,0,500,hit) && hit.head && hit.distance==110);
    assert(model.Intersect(head,ray,200,0,1000,hit) && hit.head && hit.distance==120);
    float broad=0;assert(IntersectBounds(model.BroadBounds(),empty,ray,200,broad));
    assert(!model.Intersect(empty,ray,200,0,500,hit)); // A neighboring broad box cannot steal the shot.
    assert(!model.Intersect(head,ray,109,0,500,hit));
    assert(!model.Intersect(head,ray,200,-1,0,hit));
    assert(model.Load(MdxHitFixture::Model(true)) && !model.Intersect(head,ray,200,0,500,hit));
    assert(model.Load(MdxHitFixture::Model(false,264)) && model.TriangleCount()==1);
    // One head-influenced corner cannot turn the whole face into either a head or a shoulder.
    assert(model.Load(MdxHitFixture::Model(false,256,true)) && model.HeadVertices()==1);
    float headCorner[]={-100,-8,104},shoulderCorner[]={-100,8,104},middle[]={-100,0,115};
    assert(model.Intersect(headCorner,ray,200,0,500,hit) && hit.head);
    assert(model.Intersect(shoulderCorner,ray,200,0,500,hit) && !hit.head);
    assert(model.Intersect(middle,ray,200,0,500,hit,.2f) && hit.head);
    assert(model.Intersect(middle,ray,200,0,500,hit,.8f) && !hit.head);
    // Independently posed left/right skulls both count, with no head volume bridging the gap.
    assert(model.Load(MdxHitFixture::Model(false,256,false,true)));
    heads=model.HeadBones();assert(heads.size()==2 && heads[0].root!=heads[1].root);
    float rightHead[]={-100,-25,110},leftHead[]={-100,25,110};
    assert(model.Intersect(rightHead,ray,200,0,500,hit) && hit.head && hit.distance==110 && hit.headRoot==1);
    assert(model.Intersect(leftHead,ray,200,0,500,hit) && hit.head && hit.distance==100 && hit.headRoot==2);
    assert(!model.Intersect(head,ray,200,0,500,hit));
    // Native skinning can use a mesh child while leaving its authoring head parent uncomputed.
    auto nested=MdxHitFixture::Model(false,256,false,true);
    for(size_t at=4;at<nested.size();) {
        unsigned length=0;std::memcpy(&length,nested.data()+at+4,4);
        if(!std::memcmp(nested.data()+at,"BONE",4))for(size_t p=at+8;p<at+8+length;) {
            unsigned size=0,id=0;std::memcpy(&size,nested.data()+p,4);std::memcpy(&id,nested.data()+p+84,4);
            if(id==2){std::memset(nested.data()+p+4,0,80);std::memcpy(nested.data()+p+4,"Mesh",4);MdxHitFixture::Set(nested,p+88,1);}
            p+=size+8;
        }
        at+=8+length;
    }
    assert(model.Load(nested));heads=model.HeadBones();
    assert(heads.size()==2 && heads[0].root==1 && heads[0].depth==0 && heads[1].id==2 && heads[1].root==1 && heads[1].depth==1);
    // Truncated/malformed data cannot retain the previous valid model or overrun a chunk.
    for(size_t size:{size_t(0),size_t(3),bytes.size()-1}){auto bad=bytes;bad.resize(size);assert(!model.Load(bad) && !model.Ready());}
    auto bad=bytes;MdxHitFixture::Set(bad,8,0xffffffff);assert(!model.Load(bad));
    MdxPose::Track track;track.interpolation=1;track.keys={{0,{0,0,0,0}},{1000,{20,0,0,0}}};
    assert(track.Sample(500,0,1000,{})[0]==10);
    auto q=MdxPose::Slerp({0,0,0,1},{0,0,1,0},.5f);assert(std::abs(q[2]-.70710678f)<.0001f);
    Bounds3 fallback={{-20,-20,0},{20,20,100}};float corner[]={-100,19,95};
    assert(IntersectBounds(fallback,corner,ray,200,broad) && !IntersectEllipsoid(fallback,corner,ray,200,broad));
    assert(GameText::Label("|cffff0000Rune|r|nPower")=="Rune Power");
    assert(GameText::Label("A||B")=="A|B");
    assert(GameText::Label(u8"|cffff0000\u0420\u0443\u043d\u0430|r")==u8"\u0420\u0443\u043d\u0430");
    std::puts("Posed head/body surfaces, neighbor empty-space rejection, hidden/billboard geometry, range, malformed MDX and localized names passed.");
}
