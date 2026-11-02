#pragma once
#include <cstring>

// Stock models verified in the owner's classic MPQ; only identifiers, never arbitrary asset paths, are configurable.
namespace WarcraftSkyPresets {
struct Preset {const char* id;const char* label;const char* model;};
inline constexpr Preset All[]={
    {"auto","Auto / map sky",""},
    {"summer","Lordaeron summer","Environment\\Sky\\LordaeronSummerSky\\LordaeronSummerSky.mdl"},
    {"fall","Lordaeron autumn","Environment\\Sky\\LordaeronFallSky\\LordaeronFallSky.mdl"},
    {"winter","Lordaeron winter","Environment\\Sky\\LordaeronWinterSky\\LordaeronWinterSky.mdl"},
    {"felwood","Felwood","Environment\\Sky\\FelwoodSky\\FelwoodSky.mdl"},
    {"dalaran","Dalaran","Environment\\Sky\\DalaranSky\\DalaranSky.mdl"}
};
inline constexpr int Count=int(sizeof(All)/sizeof(All[0]));
inline int Index(const char* id){for(int i=0;i<Count;++i)if(!std::strcmp(All[i].id,id))return i;return 0;}
inline bool Valid(const char* id){return !std::strcmp(All[Index(id)].id,id);}
}
