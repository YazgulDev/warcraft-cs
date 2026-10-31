#include "NativeNuclearBomb.hpp"
#include "NativeNuclearDestruction.hpp"
#include <algorithm>
#include <cmath>

void NativeNuclearBomb::Reset(){++generation_;timer_=actor_=owner_=0;actorType_=0;actorObject_=0;age_=-1;flying_=exploded_=flames_=smoke_=false;effects_.Reset();}
bool NativeNuclearBomb::Launch(wc3::Handle actor,const GrenadePoint& eye,const GrenadePoint& direction,const GrenadePoint& motion,const GameplaySettings& settings,NativeRockets& rockets){
    if(timer_ || !settings.nuclear.enabled || !wc3::SinglePlayer() || !wc3::GetUnitTypeId(actor))return false;
    auto clock=wc3::CreateTimer();if(!clock)return false;float duration=360000;wc3::TimerStart(clock,&duration,FALSE,0);
    timer_=clock;actor_=actor;actorType_=wc3::GetUnitTypeId(actor);actorObject_=wc3::UnitObject(actor);owner_=wc3::GetOwningPlayer(actor);
    settings_=settings.nuclear;age_=-1;flying_=true;exploded_=flames_=smoke_=false;unsigned generation=generation_;
    // The same rocket integration, terrain sweep, obstruction tests, motor/trail and world model serve both payloads.
    if(!rockets.Launch(actor,eye,direction,motion,settings,[this,generation](const GrenadePoint& point){if(generation_==generation)Impact(point);})){wc3::DestroyTimer(clock);timer_=0;flying_=false;return false;}
    wc3::Log("Nuclear RPG launched actor=%08X radius=%.0f",actor,settings_.radius);return true;
}
void NativeNuclearBomb::Impact(const GrenadePoint& center){
    if(!timer_ || !flying_)return;center_=center;impactTime_=wc3::Real(wc3::TimerGetElapsed(timer_));age_=0;flying_=false;
    wc3::Log("Nuclear RPG impact at=%.1f,%.1f,%.1f radius=%.0f",center[0],center[1],center[2],settings_.radius);
}
void NativeNuclearBomb::Explosion(){
    effects_.Spawn("Objects\\Spawnmodels\\Other\\NeutralBuildingExplosion\\NeutralBuildingExplosion.mdl",center_,settings_.explosionScale,8);
}
void NativeNuclearBomb::Flames(){
    effects_.Spawn("Abilities\\Spells\\Human\\FlameStrike\\FlameStrike1.mdl",center_,settings_.flameScale,settings_.SmokeStart());
    effects_.Spawn("Abilities\\Spells\\Human\\FlameStrike\\FlameStrikeTarget.mdl",center_,settings_.flameScale,settings_.SmokeStart());
    float reach=std::min(settings_.radius,settings_.visualRadius)*.5f;
    for(int i=0;i<6;++i){float a=i*6.2831853f/6;auto point=center_;point[0]+=std::cos(a)*reach;point[1]+=std::sin(a)*reach;point[2]=wc3::Ground(point[0],point[1]);
        effects_.Spawn("Abilities\\Spells\\Human\\FlameStrike\\FlameStrikeTarget.mdl",point,settings_.flameScale*.55f,settings_.SmokeStart());}
    wc3::Log("Nuclear effect phase=flames age=%.2f",age_);
}
void NativeNuclearBomb::Smoke(){
    float reach=std::min(settings_.radius,settings_.visualRadius)*.4f;
    // A sunflower disk fills the cloud evenly as density changes, avoiding a sparse fixed ring of puffs.
    int count=settings_.smokeEmitters;
    for(int i=0;i<count;++i){auto point=center_;if(i){float a=i*2.39996323f,radius=reach*std::sqrt(float(i)/std::max(1,count-1));
        point[0]+=std::cos(a)*radius;point[1]+=std::sin(a)*radius;point[2]=wc3::Ground(point[0],point[1]);}
        effects_.Spawn("Doodads\\LordaeronSummer\\Props\\SmokeSmudge\\SmokeSmudge0.mdl",point,settings_.smokeScale*(i ? .8f : 1),settings_.FinishedAt(),true);}

    wc3::Log("Nuclear effect phase=smoke age=%.2f emitters=%d scale=%.1f seconds=%.1f",age_,count,settings_.smokeScale,settings_.smokeSeconds);
}
void NativeNuclearBomb::Tick(GameAudio& audio,const KillReward::Sink& rewards){
    if(!timer_ || flying_)return;age_=std::max(0.f,wc3::Real(wc3::TimerGetElapsed(timer_))-impactTime_);unsigned generation=generation_;
    if(!exploded_){exploded_=true;
        // The private composite already includes pitch, timing and a finite rumble fade; the CS master still applies.
        if(settings_.explosionVolumePercent>0 && settings_.explosionSound!="off"){
            if(settings_.explosionSound=="layered" && audio.HasClip("nuclear_blast.wav")){
                audio.Play("nuclear_blast.wav",settings_.explosionVolumePercent*.01f);
                wc3::Log("Nuclear audio preset=layered private CS/Half-Life composite");
            }else{
                const char* path=settings_.explosionSound=="city" ? "Sound\\Buildings\\Death\\CityBuildingDeath1.wav" :
                    settings_.explosionSound=="artillery" ? "Sound\\Units\\Death\\ArtilleryCorpseExplodeDeath1.wav" : "Units\\Creeps\\GoblinSapper\\SapperBlowsUp.wav";
                if(settings_.explosionSound=="layered")wc3::Log("Nuclear layered audio unavailable; using owned Warcraft fallback");
                if(!audio.PlayArchive(path,settings_.explosionVolumePercent*.01f))audio.Play("c4_explode1.wav",settings_.explosionVolumePercent*.01f);
            }
        }
        Explosion();
        if(generation_!=generation)return;
        NativeNuclearDestruction::Apply(actor_,actorType_,actorObject_,owner_,center_,settings_,[&]{return generation_==generation;},rewards);
        if(generation_!=generation)return;
    }
    if(!flames_ && age_>=settings_.flameDelay){flames_=true;Flames();}
    if(!smoke_ && age_>=settings_.SmokeStart()){smoke_=true;Smoke();}
    effects_.Tick(age_);
    if(age_>=settings_.FinishedAt()){wc3::DestroyTimer(timer_);timer_=0;age_=-1;wc3::Log("Nuclear effect sequence finished");}
}
float NativeNuclearBomb::Flash() const{return age_>=0 && settings_.flashSeconds>0 ? .85f*std::clamp(1-age_/settings_.flashSeconds,0.f,1.f) : 0;}
float NativeNuclearBomb::Shake() const{return age_>=0 && age_<1.5f ? std::sin(age_*71)*settings_.shakeDegrees*(1-age_/1.5f) : 0;}
float NativeNuclearBomb::Exposure(const GrenadePoint& eye) const{
    if(age_<0 || settings_.screenRadius<=0)return 0;float distance=0;for(int i=0;i<3;++i)distance+=(eye[i]-center_[i])*(eye[i]-center_[i]);
    return std::clamp(1-std::sqrt(distance)/settings_.screenRadius,0.f,1.f);
}
