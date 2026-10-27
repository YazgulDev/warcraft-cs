#pragma once
#include "../config/CombatVisualSettings.hpp"
#include "../combat/WeaponSlots.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

// Game-thread pools contain copied world values only; rendering never queries units or changes simulation.
class CombatVisuals {
public:
    using Point=std::array<float,3>;
    struct Blood {Point position,velocity;float age=0,lifetime=.5f,size=3,gravity=400;};
    struct Critical {Point position;float age=0,lifetime=1,rise=40,scale=1;};
    struct Shell {Point position,velocity;float age=0,lifetime=3,size=1,angle=0,spin=0,gravity=650,bounce=.35f,floor=0;bool shotgun=false,resting=false;};
    void Reset(){blood_.clear();criticals_.clear();shells_.clear();lastTarget_=0;sinceCritical_=100;}
    static bool UsesCases(int weapon){return WeaponSlots::Firearm(weapon) && !WeaponSlots::HalfLife(weapon);}
    void Headshot(uint32_t target,const Point& contact,const Point& above,const Point& direction,const CombatVisualSettings& settings,bool showCritical=true);
    void Eject(int weapon,const Point& eye,const Point& direction,const Point& inherited,float floor,const CombatVisualSettings& settings);
    void Step(float dt);
    const std::vector<Blood>& BloodDrops() const{return blood_;}
    const std::vector<Critical>& Criticals() const{return criticals_;}
    const std::vector<Shell>& Shells() const{return shells_;}
private:
    float Random(){seed_=1664525u*seed_+1013904223u;return float(seed_>>8)/16777216.f;}
    std::vector<Blood> blood_;
    std::vector<Critical> criticals_;
    std::vector<Shell> shells_;
    uint32_t seed_=0x574353u,lastTarget_=0;
    float sinceCritical_=100;
};
