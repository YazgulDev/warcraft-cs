#include "ShortcutBindings.hpp"
#include <algorithm>
#include <cctype>

namespace {
const char* names[]={"ToggleFPS","Forward","Backward","Left","Right","Jump","Crouch","Walk","Reload","Buy","BuyAmmo","Pickup",
    "SquadFight","SquadFollow","SquadRelease","Speed","Fog","Minimap","HealthBars","Refill","AllWeapons","ReloadSettings",
    "Spell1","Spell2","Spell3","Spell4","SelfCast","Weapon1","Weapon2","Weapon3","Weapon4","Weapon5","Weapon6","Weapon7","ToggleView"};
const int defaults[]={VK_F6,'W','S','A','D',VK_SPACE,VK_CONTROL,VK_SHIFT,'R','B',VK_OEM_PERIOD,'E',
    'H','O','J','P','N','M','K',VK_F7,VK_F9,VK_F8,'Z','X','C','V',VK_MENU,'1','2','3','4','5','6','7','T'};
static_assert(sizeof(defaults)/sizeof(*defaults)==ShortcutBindings::Count);
}
ShortcutBindings::ShortcutBindings() {std::copy(std::begin(defaults),std::end(defaults),keys_.begin());}
const char* ShortcutBindings::Name(ShortcutAction action) {return names[size_t(action)];}
int ShortcutBindings::Parse(std::string value) {
    // Ignore surrounding whitespace and case, never interpret INI text as an OS key expression.
    auto first=value.find_first_not_of(" \t\r\n"),last=value.find_last_not_of(" \t\r\n");
    if(first==std::string::npos) return -1;
    value=value.substr(first,last-first+1);
    for(auto& c:value)c=char(std::toupper(static_cast<unsigned char>(c)));
    if(value=="NONE") return 0;
    if(value.size()==1 && std::isalnum(static_cast<unsigned char>(value[0]))) return value[0];
    if(value=="." || value=="PERIOD")return VK_OEM_PERIOD;
    if(value=="SPACE")return VK_SPACE;
    if(value=="CTRL" || value=="CONTROL")return VK_CONTROL;
    if(value=="SHIFT")return VK_SHIFT;
    if(value=="ALT")return VK_MENU;
    if(value=="TAB")return VK_TAB;
    for(int i=1;i<=12;++i)if(value=="F"+std::to_string(i))return VK_F1+i-1;
    return -1;
}
bool ShortcutBindings::Valid() const {
    // Validate the entire proposed layout together so swaps work and fallback cannot create duplicates.
    bool used[256]={};
    for(size_t i=0;i<Count;++i) {
        int key=keys_[i];if(!key)continue;
        if(key<0 || key>=256 || key==VK_F10 || key==VK_ESCAPE ||
            (key==VK_F6 && i!=size_t(ShortcutAction::ToggleFPS)) || used[key])return false;
        used[key]=true;
    }
    return Key(ShortcutAction::ToggleFPS)!=0;
}
std::string ShortcutBindings::Label(int key) {
    if(!key)return "NONE";
    if((key>='A' && key<='Z') || (key>='0' && key<='9'))return std::string(1,char(key));
    if(key>=VK_F1 && key<=VK_F12)return "F"+std::to_string(key-VK_F1+1);
    if(key==VK_OEM_PERIOD)return ".";
    if(key==VK_SPACE)return "SPACE";
    if(key==VK_CONTROL)return "CTRL";
    if(key==VK_SHIFT)return "SHIFT";
    if(key==VK_MENU)return "ALT";
    return key==VK_TAB ? "TAB" : "?";
}
