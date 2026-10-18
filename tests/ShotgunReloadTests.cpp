#include "../src/combat/ShotgunReload.hpp"
#include "../src/presentation/WeaponAnimation.hpp"
#include <cassert>
#include <cstring>
#include <limits>
#include <set>

int main() {
    // At low, high and uneven frame rates every required insertion occurs exactly once, in order.
    for(int shells:{1,2,7,8})for(float step:{1.f/144,1.f/30,.071f}) {
        ShotgunReload reload;reload.Configure(shells,8,4.5f,.5f,.4f,.8f);
        std::set<int> seen;int previous=-1;
        for(float t=0;t<reload.Seconds();t+=step) {
            auto pose=reload.At(t);seen.insert(pose.phase);
            assert(pose.phase>=previous && pose.elapsed>=0);
            assert(WeaponAnimation::HoldLastFrame(pose.name));
            if(pose.phase==0)assert(!strcmp(pose.name,"start_reload"));
            else if(pose.phase<=shells)assert(!strcmp(pose.name,"insert"));
            else assert(!strcmp(pose.name,"after_reload"));
            previous=pose.phase;
        }
        assert(seen.size()==size_t(shells+2));
        assert(reload.At(reload.Seconds()).phase==shells+1);
    }
    // Missing-shell count changes duration, never the speed of inserting each shell.
    ShotgunReload one,two,full;
    one.Configure(1,8,4.5f,.5f,.4f,.8f);two.Configure(2,8,4.5f,.5f,.4f,.8f);full.Configure(8,8,4.5f,.5f,.4f,.8f);
    assert(one.Rate()==two.Rate() && two.Rate()==full.Rate());
    assert(one.Seconds()<two.Seconds() && two.Seconds()<full.Seconds());
    assert(std::abs(full.Seconds()-4.5f)<.0001f && two.Seconds()<2.2f);
    // A late frame seeks into the next phase, without resetting its elapsed time or returning to idle.
    ShotgunReload reload;reload.Configure(2,2,2,.5f,.5f,.5f);
    auto late=reload.At(1.2f);assert(late.phase==2 && std::abs(late.elapsed-.2f)<.0001f);
    assert(reload.At(.5f).phase==1 && reload.At(1.5f).phase==3);
    reload.Configure(1,1,3,0,std::numeric_limits<float>::quiet_NaN(),0);
    assert(std::isfinite(reload.Rate()) && reload.At(1.5f).phase==1);
    // Renderer holds terminal reload bones across a delayed update, while idle remains a looping pose.
    for(auto name:{"start_reload","insert","after_reload"})assert(WeaponAnimation::HoldLastFrame(name));
    assert(!WeaponAnimation::HoldLastFrame("shoot1") && !WeaponAnimation::HoldLastFrame("idle"));
    assert(WeaponAnimation::Frame(16,30,10,false)==15);
    assert(WeaponAnimation::Frame(16,30,.25f,false)==7.5f);
    assert(WeaponAnimation::Frame(16,30,1,true)==14);
    assert(WeaponAnimation::Frame(16,30,-1,false)==0);
    // A partial insertion grants nothing; completed shells transfer one real reserve round exactly once.
    ShotgunReload incremental;incremental.Configure(3,8,4.5f,.5f,.4f,.8f);
    int magazine=5,reserve=10;
    assert(incremental.LoadCompleted(.899f,8,magazine,reserve)==0 && magazine==5 && reserve==10);
    assert(incremental.LoadCompleted(.9f,8,magazine,reserve)==1 && magazine==6 && reserve==9);
    assert(incremental.LoadCompleted(1.2f,8,magazine,reserve)==0 && magazine==6);
    // Firing now interrupts the second insertion: only the completed first shell remains spent.
    --magazine;assert(magazine==5 && reserve==9);
    incremental.Configure(3,8,4.5f,.5f,.4f,.8f);
    assert(incremental.LoadCompleted(1.31f,8,magazine,reserve)==2 && magazine==7 && reserve==7);
    assert(incremental.LoadCompleted(1.31f,8,magazine,reserve)==0);
    assert(incremental.LoadCompleted(.9f,8,magazine,reserve)==0);
    assert(incremental.LoadCompleted(1.31f,8,magazine,reserve)==0);
    assert(incremental.LoadCompleted(9,8,magazine,reserve)==1 && magazine==8 && reserve==6);
    assert(incremental.LoadCompleted(9,8,magazine,reserve)==0); // No second magazine at completion.
    // Current capacity and supplies remain authoritative after live edits or external ammunition changes.
    incremental.Configure(3,8,4.5f,.5f,.4f,.8f);magazine=7;reserve=1;
    assert(incremental.LoadCompleted(9,8,magazine,reserve)==1 && magazine==8 && reserve==0);
    incremental.Configure(3,8,4.5f,.5f,.4f,.8f);magazine=4;reserve=0;
    assert(incremental.LoadCompleted(9,6,magazine,reserve)==0 && magazine==4 && reserve==0);
}
