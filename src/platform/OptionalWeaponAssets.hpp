#pragma once
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>

// The converter seals complete private RPG resources; missing/changed files never unlock a weapon.
namespace OptionalWeaponAssets {
template<size_t N> inline bool Sealed(const std::filesystem::path& root,const char* marker,const char* signature,const char* const (&names)[N]) {
    auto assets=root/"assets";std::ifstream seal(assets/marker,std::ios::binary);
    char magic[4]{};seal.read(magic,4);if(!seal || std::memcmp(magic,signature,4))return false;
    for(auto name:names) {
        uint64_t length=0,expected=0;seal.read(reinterpret_cast<char*>(&length),8);seal.read(reinterpret_cast<char*>(&expected),8);
        if(!seal || !length || length>64*1024*1024)return false;
        std::ifstream file(assets/name,std::ios::binary);uint64_t hash=14695981039346656037ull,read=0;char bytes[4096];
        while(file) {file.read(bytes,sizeof(bytes));auto count=file.gcount();read+=count;
            for(std::streamsize i=0;i<count;++i){hash^=static_cast<unsigned char>(bytes[i]);hash*=1099511628211ull;}}
        if(read!=length || hash!=expected)return false;
    }
    return seal.peek()==std::char_traits<char>::eof();
}
inline bool Rpg(const std::filesystem::path& root) {
    const char* names[]={"rpg.wcg","rpg.wca","buy/rpg.wcg","world/rpgrocket.wcg",
        "sounds/hl_rpg_fire.wav","sounds/hl_rpg_motor.wav","sounds/hl_rpg_explode.wav"};
    return Sealed(root,"rpg.ready","WRP1",names);
}
inline bool HalfLife(const std::filesystem::path& root,int index) {
    // Separate seals include all required sounds; a stale model alone never unlocks the weapon.
    const char* crowbar[]={"crowbar.wcg","crowbar.wca","buy/crowbar.wcg","sounds/hl_cbar_miss1.wav","sounds/hl_cbar_hit1.wav","sounds/hl_cbar_hitbod1.wav"};
    const char* tau[]={"gauss.wcg","gauss.wca","buy/gauss.wcg","sounds/hl_gauss2.wav","sounds/hl_gauss_charge.wav"};
    const char* gluon[]={"egon.wcg","egon.wca","buy/egon.wcg","sounds/hl_egon_windup2.wav","sounds/hl_egon_run3.wav","sounds/hl_egon_off1.wav"};
    return index==0 ? Sealed(root,"crowbar.ready","WHL1",crowbar) : index==1 ? Sealed(root,"gauss.ready","WHL1",tau) :
        index==2 && Sealed(root,"egon.ready","WHL1",gluon);
}
}
