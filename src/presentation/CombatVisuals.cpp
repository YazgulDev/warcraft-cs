#include "CombatVisuals.hpp"

void CombatVisuals::Headshot(uint32_t target,const Point& contact,const Point& above,const Point& direction,const CombatVisualSettings& s,bool showCritical,bool fatal) {
    // Shotgun pellets/continuous beams share a brief per-target interval instead of stacking unreadable labels.
    bool coalesced=target==lastTarget_ && sinceCritical_<.08f;
    // A fatal pellet/beam contact must still burst after a preceding nonfatal head pellet in the same frame.
    if(coalesced && !fatal)return;
    lastTarget_=target;sinceCritical_=0;
    // Blood is tied to the anatomical contact; disabling bonus text must not suppress it.
    if(showCritical && s.criticalText && !coalesced){if(criticals_.size()>=32)criticals_.erase(criticals_.begin());criticals_.push_back({above,0,s.criticalSeconds,s.criticalRise,s.criticalScale});}
    unsigned count=unsigned(s.bloodCount*(fatal ? s.fatalBloodMultiplier : 1));
    if(s.headshotBlood)for(unsigned i=0;i<count;++i){
        if(blood_.size()>=256)blood_.erase(blood_.begin());
        Point velocity;for(int axis=0;axis<3;++axis)velocity[axis]=(Random()-.5f)*s.bloodSpeed+direction[axis]*s.bloodSpeed*.3f;
        velocity[2]+=s.bloodSpeed*.2f;blood_.push_back({contact,velocity,0,s.bloodSeconds,(1.5f+Random()*2)*s.bloodScale,s.bloodGravity});
    }
}
void CombatVisuals::Eject(int weapon,const Point& eye,const Point& direction,const Point& inherited,float floor,const CombatVisualSettings& s) {
    if(!s.shellCasings || !UsesCases(weapon))return;
    if(shells_.size()>=s.shellMaximum)shells_.erase(shells_.begin());
    float horizontal=std::hypot(direction[0],direction[1]);if(horizontal<.001f)return;
    Point right={direction[1]/horizontal,-direction[0]/horizontal,0};
    Point up={right[1]*direction[2],-right[0]*direction[2],right[0]*direction[1]-right[1]*direction[0]};
    Shell shell;shell.position=eye;shell.floor=floor;shell.lifetime=s.shellSeconds;shell.gravity=s.shellGravity;shell.bounce=s.shellBounce;
    shell.shotgun=weapon==WeaponSlots::M3 || weapon==WeaponSlots::XM1014;shell.size=s.shellScale*(shell.shotgun ? 1.6f : 1);
    float side=s.shellSpeed*(.8f+Random()*.4f),lift=s.shellSpeed*(.55f+Random()*.45f);
    for(int i=0;i<3;++i){shell.position[i]+=right[i]*s.shellOffset[0]+up[i]*s.shellOffset[1]+direction[i]*s.shellOffset[2];
        shell.velocity[i]=right[i]*side+up[i]*lift+direction[i]*s.shellSpeed*.2f+inherited[i]*.2f;}
    shell.angle=Random()*360;shell.spin=500+Random()*700;shells_.push_back(shell);
}
void CombatVisuals::Step(float dt) {
    if(!std::isfinite(dt) || dt<=0)return;
    sinceCritical_+=dt;
    // Short substeps give debris stable arcs/bounces at ordinary and high frame rates. It has no collision authority.
    unsigned steps=unsigned(std::ceil(std::min(dt,1.f)*120));float h=dt/steps;
    for(unsigned step=0;step<steps;++step){
        for(auto& p:blood_){p.age+=h;for(int i=0;i<3;++i)p.position[i]+=p.velocity[i]*h;p.velocity[2]-=p.gravity*h;}
        for(auto& c:criticals_)c.age+=h;
        for(auto& p:shells_){p.age+=h;if(p.resting)continue;
            for(int i=0;i<3;++i)p.position[i]+=p.velocity[i]*h;p.velocity[2]-=p.gravity*h;p.angle+=p.spin*h;
            if(p.position[2]<p.floor+p.size){p.position[2]=p.floor+p.size;p.velocity[2]=std::abs(p.velocity[2])*p.bounce;
                p.velocity[0]*=.55f;p.velocity[1]*=.55f;p.spin*=.5f;
                if(p.velocity[2]<20){p.velocity={0,0,0};p.resting=true;}}
        }
    }
    auto expired=[](const auto& value){return value.age>=value.lifetime;};
    blood_.erase(std::remove_if(blood_.begin(),blood_.end(),expired),blood_.end());
    criticals_.erase(std::remove_if(criticals_.begin(),criticals_.end(),expired),criticals_.end());
    shells_.erase(std::remove_if(shells_.begin(),shells_.end(),expired),shells_.end());
}
