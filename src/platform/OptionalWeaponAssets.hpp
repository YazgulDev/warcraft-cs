#pragma once
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>

// The converter seals complete private RPG resources; missing/changed files never unlock a weapon.
namespace OptionalWeaponAssets {
inline bool Rpg(const std::filesystem::path& root) {
    const char* names[]={"rpg.wcg","rpg.wca","buy/rpg.wcg","world/rpgrocket.wcg",
        "sounds/hl_rpg_fire.wav","sounds/hl_rpg_motor.wav","sounds/hl_rpg_explode.wav"};
    auto assets=root/"assets";std::ifstream seal(assets/"rpg.ready",std::ios::binary);
    char magic[4]{};seal.read(magic,4);if(!seal || std::memcmp(magic,"WRP1",4))return false;
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
}
