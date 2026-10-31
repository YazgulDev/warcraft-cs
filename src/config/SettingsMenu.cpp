#include "SettingsMenu.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
constexpr float low[]={.1f,.1f,0,0,3000,0,0,0},high[]={5,5,100,2,20000,1,1,0},step[]={.1f,.1f,5,1,500,1,1,1};
const char* modes[]={"classic","recovery","hull"};
std::string Number(float value){char text[32];std::snprintf(text,sizeof(text),"%.4g",double(value));return text;}
}
void SettingsMenu::Queue(Event event) {if(events_.size()<64)events_.push_back(event);}
bool SettingsMenu::RoutingOpen() const {
    // Even a short F2 tap owns following mouse messages before the game thread opens the panel.
    return open_ || std::any_of(events_.begin(),events_.end(),[](auto e){return e.kind==Kind::Toggle;});
}
void SettingsMenu::Begin(const GameplaySettings& s,char tileset,const SkyList& list) {
    open_=true;selected_=0;error_.clear();tileset_=tileset;skies_=list();csAvailable_=!s.warcraftOnly && !skies_.empty();
    const auto& sky=s.tilesetSky[static_cast<unsigned char>(tileset)];std::string current=sky.empty() ? s.defaultSky : sky;savedSky_=current;
    auto found=std::find(skies_.begin(),skies_.end(),current);
    values_={s.mouse.sensitivity,s.mouse.scopeSensitivity,s.csVolumePercent,s.movement.classicCollision ? 0.f : s.movement.platformRecovery ? 1.f : 2.f,
        s.render.drawDistance,s.skyVisible ? 1.f : 0.f,s.csSky ? 1.f : 0.f,found==skies_.end() ? -1.f : float(found-skies_.begin())};initial_=values_;
}
void SettingsMenu::Adjust(int row,float steps) {
    if(row<0 || row>=Rows || !std::isfinite(steps))return;
    // Optional CS skies never turn a native-only client into an empty resource-dependent view.
    if(row==SkyMode && !csAvailable_){values_[row]=0;return;}
    if(row==SkyPreset && !csAvailable_)return;
    float maximum=row==SkyPreset ? float(skies_.size()-1) : high[row];
    float next=values_[row]+steps*step[row];
    values_[row]=Numeric(row) ? std::clamp(next,low[row],maximum) : next>maximum ? low[row] : next<low[row] ? maximum : next;
    error_.clear();
}
bool SettingsMenu::Update(const GameplaySettings& s,char tileset,const SkyList& list,const Save& save) {
    auto pending=std::move(events_);events_.clear();bool applied=false;
    for(auto e:pending) {
        if(e.kind==Kind::Toggle){if(open_)open_=false;else Begin(s,tileset,list);continue;}
        if(!open_)continue;
        if(e.kind==Kind::Cancel){open_=false;error_.clear();continue;}
        if(e.kind==Kind::Apply){
            auto changes=Changes();
            if(changes.empty()){open_=false;continue;}
            if(save(changes)){applied=true;open_=false;error_.clear();}
            else error_="Could not save INI. Changes were not applied; try again.";
            continue;
        }
        if(e.kind==Kind::Defaults){values_={1,1,100,0,5000,1,0,initial_[SkyPreset]};error_.clear();continue;}
        if(e.kind==Kind::Navigate){selected_=(selected_+(e.value>0 ? 1 : Rows-1))%Rows;continue;}
        if(e.kind==Kind::Select){selected_=(e.row%Rows+Rows)%Rows;continue;}
        int row=e.row<0 ? selected_ : e.row;
        if(row<0 || row>=Rows)continue;
        if(e.kind==Kind::Adjust)Adjust(row,e.value);
        if(e.kind==Kind::Slide && Numeric(row) && std::isfinite(e.value)) {
            float value=low[row]+std::clamp(e.value,0.f,1.f)*(high[row]-low[row]);
            values_[row]=std::clamp(low[row]+std::round((value-low[row])/step[row])*step[row],low[row],high[row]);error_.clear();
        }
    }
    return applied;
}
std::vector<ConfigChange> SettingsMenu::Changes() const {
    std::vector<ConfigChange> changes;
    auto changed=[&](int row){return values_[row]!=initial_[row];};
    auto add=[&](const char* section,const char* key,std::string value){changes.push_back({section,key,std::move(value)});};
    if(changed(Sensitivity))add("Mouse","Sensitivity",Number(values_[Sensitivity]));
    if(changed(ScopeSensitivity))add("Mouse","ScopeSensitivity",Number(values_[ScopeSensitivity]));
    if(changed(Volume))add("Audio","CSVolumePercent",Number(values_[Volume]));
    if(changed(Physics))add("Movement","Mode",modes[int(values_[Physics])]);
    if(changed(ViewDistance))add("Rendering","DrawDistance",Number(values_[ViewDistance]));
    if(changed(SkyVisible))add("Sky","Visible",values_[SkyVisible]>0 ? "true" : "false");
    if(changed(SkyMode)) {add("Sky","Enabled",values_[SkyMode]>0 ? "true" : "false");add("Sky","WarcraftEnabled",values_[SkyMode]>0 ? "false" : "true");}
    if(changed(SkyPreset) && values_[SkyPreset]>=0) {
        // Known terrains own their existing per-tileset override; unknown maps use the saved fallback.
        std::string key=tileset_ && std::strchr("ALFWBYXVQJNDCIGOZ",tileset_) ? std::string(1,tileset_) : "Default";
        changes.push_back({"Sky",key,skies_[size_t(values_[SkyPreset])]});
    }
    return changes;
}
float SettingsMenu::Fraction(int row) const {return Numeric(row) ? (values_[row]-low[row])/(high[row]-low[row]) : 0;}
std::string SettingsMenu::Value(int row) const {
    if(row==Physics)return modes[int(values_[row])];
    if(row==SkyVisible)return values_[row]>0 ? "ON" : "OFF";
    if(row==SkyMode)return values_[row]>0 ? "Counter-Strike" : "Warcraft";
    if(row==SkyPreset)return !csAvailable_ ? "No CS skies installed" : values_[row]<0 ? savedSky_+" (missing)" : skies_[size_t(values_[row])];
    return Number(values_[row])+(row==Volume ? "%" : row==ViewDistance ? " units" : "x");
}
const char* SettingsMenu::Label(int row) {
    const char* labels[]={"Mouse sensitivity","Scoped sensitivity","CS volume","Movement physics","View distance","Sky visible","Sky type","CS sky preset"};return labels[row];
}
const char* SettingsMenu::Hint(int row) {
    const char* hints[]={"Mouse multiplier: 1x preserves your previous aiming speed.","Extra multiplier while scoped. Main sensitivity also applies.","Shots, steps and reloads. Warcraft audio stays unchanged.","classic: original | recovery: rescue | hull: detailed collision","World camera distance. Map fog still limits visibility.","Hide/show sky in controlled views; RTS restores the map sky.","Warcraft map sky or a privately converted Counter-Strike sky.","CS sky for this terrain. Changes appear with CS sky type."};return hints[row];
}
