#include "GameAudio.hpp"
#include "../platform/WarcraftApi.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <objbase.h>

bool GameAudio::LoadClip(const std::string& path, Clip& clip) {
    std::ifstream file(path, std::ios::binary);
    std::vector<BYTE> bytes((std::istreambuf_iterator<char>(file)), {});
    if (bytes.size() < 12 || memcmp(bytes.data(), "RIFF", 4) || memcmp(bytes.data() + 8, "WAVE", 4)) return false;
    WAVEFORMATEX format{};
    // RIFF chunks are padded to even bytes; samples stay owned until every voice finishes.
    for (size_t at = 12; at + 8 <= bytes.size();) {
        uint32_t size; memcpy(&size, bytes.data() + at + 4, 4);
        if (size > bytes.size() - at - 8) return false;
        if (!memcmp(bytes.data() + at, "fmt ", 4) && size >= 16) memcpy(&format, bytes.data() + at + 8, 16);
        if (!memcmp(bytes.data() + at, "data", 4)) clip.data.assign(bytes.begin() + at + 8, bytes.begin() + at + 8 + size);
        at += 8 + size + (size & 1);
    }
    if (format.wFormatTag != WAVE_FORMAT_PCM || !format.nBlockAlign || clip.data.empty()) return false;
    for (auto& voice : clip.voices) {
        if (FAILED(engine_->CreateSourceVoice(&voice, &format))) return false;
        voice->Start();
    }
    return true;
}
void GameAudio::Configure(const std::string& root) {
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
    static const char* weapons[] = {"ak47", "m4a1", "usp", "awp", "knife", "c4", "sword"};
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
            timelines_[weapon].emplace(name, std::move(sequence));
        }
    }
    wc3::Log("CS audio ready: %zu cached samples; Warcraft audio retained", clips_.size());
    CoUninitialize();
}
void GameAudio::Play(const char* name, float volume) {
    auto found = clips_.find(name);
    if (found == clips_.end()) { wc3::Log("CS sound missing: %s", name); return; }
    Clip& clip = *found->second;
    // Reuse an idle voice rather than queuing behind an old shot or interrupting footsteps.
    IXAudio2SourceVoice* voice = nullptr; unsigned occupied = 0;
    for (auto candidate : clip.voices) {
        XAUDIO2_VOICE_STATE state{}; candidate->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
        if (state.BuffersQueued) ++occupied; else if (!voice) voice = candidate;
    }
    if (!voice) {
        voice = clip.voices[clip.next++ % clip.voices.size()];
        voice->Stop(); voice->FlushSourceBuffers(); voice->Start();
    }
    XAUDIO2_BUFFER buffer{}; buffer.AudioBytes = UINT32(clip.data.size()); buffer.pAudioData = clip.data.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM; voice->SetVolume(volume);
    HRESULT result = voice->SubmitSourceBuffer(&buffer);
    wc3::Log("CS sound=%s volume=%.2f overlapping=%u queued=%08X", name, volume, occupied, result);
}
float GameAudio::BeginAnimation(int weapon, const char* name, DWORD start) {
    auto found = timelines_[weapon].find(name);
    current_ = found == timelines_[weapon].end() ? nullptr : &found->second;
    event_ = 0; start_ = start;
    Tick(start);
    return current_ ? current_->duration : 0;
}
void GameAudio::Tick(DWORD now) {
    // Both the mesh and sound events use the controller's same animation start clock.
    float elapsed = (now - start_) * 0.001f;
    while (current_ && event_ < current_->events.size() && current_->events[event_].time <= elapsed) {
        const Event& event = current_->events[event_++]; Play(event.sound.c_str());
        wc3::Log("animation audio expected=%.3f actual=%.3f", event.time, elapsed);
    }
}
void GameAudio::CancelAnimation() { current_ = nullptr; event_ = 0; }
void GameAudio::Stop() {
    // Only our voices stop on mode exit; ambient sounds and Warcraft music keep playing.
    CancelAnimation();
    for (auto& pair : clips_) for (auto voice : pair.second->voices) {
        voice->Stop(); voice->FlushSourceBuffers(); voice->Start();
    }
}
