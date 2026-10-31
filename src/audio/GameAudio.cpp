#include "GameAudio.hpp"
#include "../platform/WarcraftApi.hpp"
#include "../platform/NativeArchive.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <objbase.h>

bool GameAudio::LoadClip(const std::string& path, Clip& clip) {
    std::ifstream file(path, std::ios::binary);
    std::vector<BYTE> bytes((std::istreambuf_iterator<char>(file)), {});
    return LoadClipBytes(bytes,clip);
}
bool GameAudio::LoadClipBytes(const std::vector<BYTE>& bytes,Clip& clip) {
    // File and owned archive samples share the same bounded PCM decoder and voice lifetime.
    if (bytes.size() < 12 || memcmp(bytes.data(), "RIFF", 4) || memcmp(bytes.data() + 8, "WAVE", 4)) return false;
    WAVEFORMATEX format{};
    // RIFF chunks are padded to even bytes; samples stay owned until every voice finishes.
    for (size_t at = 12; at + 8 <= bytes.size();) {
        uint32_t size; memcpy(&size, bytes.data() + at + 4, 4);
        if (size > bytes.size() - at - 8) return false;
        if (!memcmp(bytes.data() + at, "fmt ", 4) && size >= 16) memcpy(&format, bytes.data() + at + 8, 16);
        if (!memcmp(bytes.data() + at, "data", 4)) clip.data.assign(bytes.begin() + at + 8, bytes.begin() + at + 8 + size);
        // Stock pinpull.wav omits padding before trailing LIST metadata. Complete PCM does not need that metadata.
        if(format.wFormatTag==WAVE_FORMAT_PCM && format.nBlockAlign && !clip.data.empty())break;
        at += 8 + size + (size & 1);
    }
    if (format.wFormatTag != WAVE_FORMAT_PCM || !format.nBlockAlign || clip.data.empty()) return false;
    for (auto& voice : clip.voices) {
        if (FAILED(engine_->CreateSourceVoice(&voice, &format,0,4))) return false; // Tau's charge motor can rise beyond the default 2x voice limit.
        voice->Start();
    }
    return true;
}
void GameAudio::Configure(const std::string& root) {
    LoadTimelines(root); // Pose timing is required even when the optional audio device cannot initialize.
    // Load the system runtime explicitly; audio failure never prevents Warcraft from starting.
    HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(com)) { wc3::Log("CS audio COM initialization failed: %08X", com); return; }
    HMODULE module = LoadLibraryExA("xaudio2_9.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    using Create = HRESULT(WINAPI*)(IXAudio2**, UINT32, XAUDIO2_PROCESSOR);
    auto create = module ? reinterpret_cast<Create>(GetProcAddress(module, "XAudio2Create")) : nullptr;
    HRESULT result = create ? create(&engine_, 0, XAUDIO2_DEFAULT_PROCESSOR) : E_NOINTERFACE;
    if (SUCCEEDED(result)) result = engine_->CreateMasteringVoice(&master_);
    if (FAILED(result)) {
        wc3::Log("CS audio initialization failed: %08X", result);
        if (engine_) engine_->Release(); engine_ = nullptr; CoUninitialize(); return;
    }
    const std::string directory = root + "\\assets\\sounds\\";
    WIN32_FIND_DATAA entry{};
    HANDLE search = FindFirstFileA((directory + "*.wav").c_str(), &entry);
    if (search != INVALID_HANDLE_VALUE) {
        do {
            auto clip = std::make_unique<Clip>();
            if (LoadClip(directory + entry.cFileName, *clip)) clips_.emplace(entry.cFileName, std::move(clip));
            else {
                for (auto voice : clip->voices) if (voice) voice->DestroyVoice();
                wc3::Log("CS audio rejected WAV: %s", entry.cFileName);
            }
        } while (FindNextFileA(search, &entry));
        FindClose(search);
    }
    wc3::Log("CS audio ready: %zu cached samples; Warcraft audio retained", clips_.size());
    CoUninitialize();
}
void GameAudio::LoadTimelines(const std::string& root) {
    const auto& weapons=WeaponSlots::Caches;
    for (int weapon = 0; weapon < WeaponSlots::Count; ++weapon) {
        std::ifstream file(root + "\\assets\\" + weapons[weapon] + ".wca", std::ios::binary);
        char magic[4]; uint32_t count = 0;
        file.read(magic, 4); file.read(reinterpret_cast<char*>(&count), 4);
        if (!file || memcmp(magic, "WCA1", 4) || count > 128) continue;
        for (uint32_t i = 0; i < count; ++i) {
            char name[33]{}; Sequence sequence{}; uint32_t events = 0;
            file.read(name, 32); file.read(reinterpret_cast<char*>(&sequence.duration), 4); file.read(reinterpret_cast<char*>(&events), 4);
            if (!file || events > 128) break;
            for (uint32_t j = 0; j < events; ++j) {
                Event event{}; char sound[65]{};
                file.read(reinterpret_cast<char*>(&event.time), 4); file.read(sound, 64); event.sound = sound;
                sequence.events.push_back(std::move(event));
            }
            if (!file) break;
            // Retail XM1014 insert clips contain no sound events; supply their shell-loading sound explicitly.
            // Fill only absent events, preserving authored M3 events and avoiding doubled shell sounds.
            if(sequence.events.empty() && (weapon==WeaponSlots::M3 || weapon==WeaponSlots::XM1014) && !strcmp(name,"insert"))
                sequence.events.push_back({0,"m3_insertshell.wav"});
            if(sequence.events.empty() && weapon==WeaponSlots::M3 && !strcmp(name,"after_reload"))
                sequence.events.push_back({0,"m3_pump.wav"});
            timelines_[weapon].emplace(name, std::move(sequence));
        }
    }
}
void GameAudio::SetVolumePercent(float percent) {
    // Audio initialization is optional; changing settings must remain safe when its device is unavailable.
    if (!master_) return;
    percent=std::isfinite(percent) ? std::clamp(percent,0.0f,100.0f) : 100.0f;
    // Master gain also changes already playing shots/reloads without restarting voices or animation clocks.
    HRESULT result=master_->SetVolume(percent/100.0f);
    wc3::Log("CS master volume=%.1f percent result=%08X",VolumePercent(),result);
}
float GameAudio::VolumePercent() const {
    // Read the applied native bus gain for diagnostics instead of reporting only the requested setting.
    float gain=0;
    if (master_) master_->GetVolume(&gain);
    return gain*100.0f;
}
bool GameAudio::HasClip(const char* name) const{return name && clips_.find(name)!=clips_.end();}
void GameAudio::Play(const char* name, float volume) {
    auto found = clips_.find(name);
    if (found == clips_.end()) { wc3::Log("CS sound missing: %s", name); return; }
    Clip& clip = *found->second;
    // Reuse an idle voice rather than queuing behind an old shot or interrupting footsteps.
    IXAudio2SourceVoice* voice = nullptr; unsigned occupied = 0;
    for (size_t i=0;i<clip.voices.size()-(clip.looping ? 1 : 0);++i) {
        auto candidate=clip.voices[i];
        XAUDIO2_VOICE_STATE state{}; candidate->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
        if (state.BuffersQueued) ++occupied; else if (!voice) voice = candidate;
    }
    if (!voice) {
        voice = clip.voices[clip.next++ % (clip.voices.size()-(clip.looping ? 1 : 0))];
        voice->Stop(); voice->FlushSourceBuffers(); voice->Start();
    }
    XAUDIO2_BUFFER buffer{}; buffer.AudioBytes = UINT32(clip.data.size()); buffer.pAudioData = clip.data.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM; voice->SetVolume(volume);
    voice->SetFrequencyRatio(1); // A voice released by a loop must not pitch-shift the next ordinary impact/shot.
    HRESULT result = voice->SubmitSourceBuffer(&buffer);
    wc3::Log("CS sound=%s volume=%.2f overlapping=%u queued=%08X", name, volume, occupied, result);
}
bool GameAudio::PlayArchive(const char* path,float volume){
    if(!engine_ || !path || !*path)return false;std::string key=std::string("warcraft:")+path;
    if(clips_.find(key)==clips_.end()){
        auto bytes=NativeArchive::Read(path);auto clip=std::make_unique<Clip>();
        if(!LoadClipBytes(bytes,*clip)){
            for(auto voice:clip->voices)if(voice)voice->DestroyVoice();
            wc3::Log("CS archive sound rejected path=%s bytes=%zu",path,bytes.size());return false;
        }
        clips_.emplace(key,std::move(clip));wc3::Log("CS archive sound loaded path=%s bytes=%zu",path,bytes.size());
    }
    Play(key.c_str(),volume);return true;
}
void GameAudio::SetLoop(const char* name,bool playing,float volume) {
    auto found=clips_.find(name);if(found==clips_.end())return;auto& clip=*found->second;
    if(clip.looping==playing)return;
    auto voice=clip.voices.back();if(!voice)return;
    // The motor owns one reserved voice and cannot queue another loop on every rendered frame.
    voice->Stop();voice->FlushSourceBuffers();clip.looping=false;
    voice->SetFrequencyRatio(1);
    if(playing) {
        XAUDIO2_BUFFER buffer{};buffer.AudioBytes=UINT32(clip.data.size());buffer.pAudioData=clip.data.data();
        buffer.Flags=XAUDIO2_END_OF_STREAM;buffer.LoopCount=XAUDIO2_LOOP_INFINITE;voice->SetVolume(volume);
        clip.looping=SUCCEEDED(voice->SubmitSourceBuffer(&buffer));
    }
    voice->Start();wc3::Log("CS loop=%s playing=%d",name,clip.looping);
}
float GameAudio::LoopPitch(const char* name) const {
    auto found=clips_.find(name);float ratio=1;
    if(found!=clips_.end() && found->second->voices.back())found->second->voices.back()->GetFrequencyRatio(&ratio);
    return ratio;
}
void GameAudio::SetLoopPitch(const char* name,float ratio) {
    auto found=clips_.find(name);if(found==clips_.end() || !found->second->looping)return;
    ratio=std::isfinite(ratio) ? std::clamp(ratio,.25f,4.f) : 1;
    float before=LoopPitch(name);
    // Change the already queued motor voice, preserving its buffer/cursor rather than restarting it each frame.
    HRESULT result=found->second->voices.back()->SetFrequencyRatio(ratio);
    if(std::lround(before*10)!=std::lround(ratio*10))wc3::Log("CS loop pitch=%s requested=%.3f applied=%.3f result=%08X",name,ratio,LoopPitch(name),result);
}
float GameAudio::Duration(int weapon,const char* name) const {
    auto found=timelines_[weapon].find(name);return found==timelines_[weapon].end() ? 0 : found->second.duration;
}
float GameAudio::BeginAnimation(int weapon, const char* name, DWORD start,float rate) {
    auto found = timelines_[weapon].find(name);
    current_ = found == timelines_[weapon].end() ? nullptr : &found->second;
    event_ = 0; start_ = start;rate_=rate;
    Tick(start);
    return current_ ? current_->duration : 0;
}
void GameAudio::Tick(DWORD now) {
    // Both the mesh and sound events use the controller's same animation start clock.
    float elapsed = (now - start_) * 0.001f*rate_;
    while (current_ && event_ < current_->events.size() && current_->events[event_].time <= elapsed) {
        const Event& event = current_->events[event_++]; Play(event.sound.c_str());
        wc3::Log("animation audio expected=%.3f actual=%.3f", event.time, elapsed);
    }
}
void GameAudio::CancelAnimation() { current_ = nullptr; event_ = 0; }
void GameAudio::Stop() {
    // Only our voices stop on mode exit; ambient sounds and Warcraft music keep playing.
    CancelAnimation();
    for (auto& pair : clips_) {pair.second->looping=false;for (auto voice : pair.second->voices) {
        voice->Stop(); voice->FlushSourceBuffers(); voice->Start();
    }}
}
