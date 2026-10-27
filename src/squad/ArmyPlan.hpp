#pragma once
#include "../config/ArmyAISettings.hpp"
#include <array>
#include <vector>

// Portable race detection keeps native command execution separate from the editable build plan.
struct ArmyPlan {
    int worker=0,hall=0,supply=0,barracks=0,soldier=0;
    static int Code(const std::string& text) {
        if(text.size()!=4)return 0;unsigned value=0;for(unsigned char c:text)value=(value<<8)|c;return int(value);
    }
    static std::array<ArmyPlan,4> Races() {return {{{Code("hpea"),Code("htow"),Code("hhou"),Code("hbar"),Code("hfoo")},
        {Code("opeo"),Code("ogre"),Code("obur"),Code("obar"),Code("ogru")},
        {Code("uaco"),Code("unpl"),Code("uzig"),Code("usep"),Code("ugho")},
        {Code("ewsp"),Code("etol"),Code("emow"),Code("eaom"),Code("earc")}}};}
    static ArmyPlan Detect(const std::vector<int>& types) {
        ArmyPlan selected;int best=0;
        // Campaign captures can mix halls from one race with the actual builders of another.
        for(const auto& race:Races()) {int workers=0,halls=0;
            for(int type:types){workers+=type==race.worker;halls+=race.Hall(type);}
            int score=workers*10000+halls;
            if(score>best){best=score;selected=race;}
        }
        return selected;
    }
    static ArmyPlan Custom(const ArmyAISettings& s) {return {Code(s.worker),Code(s.hall),Code(s.supply),Code(s.barracks),Code(s.soldier)};}
    bool Complete() const {return worker && hall && supply && barracks && soldier;}
    bool Hall(int type) const {
        if(type==hall)return true;
        return (hall==Code("htow") && (type==Code("hkee") || type==Code("hcas"))) ||
            (hall==Code("ogre") && (type==Code("ostr") || type==Code("ofrt"))) ||
            (hall==Code("unpl") && (type==Code("unp1") || type==Code("unp2"))) ||
            (hall==Code("etol") && (type==Code("etoa") || type==Code("etoe")));
    }
};
