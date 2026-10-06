#pragma once
#include "WeaponSlots.hpp"
#include <windows.h>
#include <xaudio2.h>
#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Preloaded CS voices mix alongside Warcraft's untouched Miles audio engine.
class GameAudio {
public:
    void Configure(const std::string& root);
    void Play(const char* name, float volume = 0.75f);
    float BeginAnimation(int weapon, const char* name, DWORD start);
    void Tick(DWORD now);
    void CancelAnimation();
    void Stop();
private:
    struct Clip {
        std::vector<BYTE> data;
        std::array<IXAudio2SourceVoice*, 8> voices{};
        size_t next = 0;
    };
    struct Event { float time; std::string sound; };
    struct Sequence { float duration; std::vector<Event> events; };
    bool LoadClip(const std::string& path, Clip& clip);
    IXAudio2* engine_ = nullptr;
    IXAudio2MasteringVoice* master_ = nullptr;
    std::map<std::string, std::unique_ptr<Clip>> clips_;
    std::array<std::map<std::string, Sequence>, WeaponSlots::Count> timelines_;
    const Sequence* current_ = nullptr;
    DWORD start_ = 0;
    size_t event_ = 0;
};
