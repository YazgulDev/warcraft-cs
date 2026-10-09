#include "../src/audio/GameAudio.hpp"
#include "../src/config/GameplaySettings.hpp"
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>

namespace wc3 {
    void Log(const char* format,...) {
        va_list arguments;va_start(arguments,format);vprintf(format,arguments);va_end(arguments);
        putchar('\n');
    }
}
static void RequireVolume(const GameAudio& audio,float expected) {
    assert(std::abs(audio.VolumePercent()-expected)<0.001f);
}
int main(int count,char** arguments) {
    assert(count==2);
    const std::filesystem::path root=arguments[1];
    const auto sounds=root/"assets"/"sounds";
    std::filesystem::create_directories(sounds);
    // A project-authored silent PCM clip exercises real voices without game assets or audible test tones.
    const unsigned char header[]={
        'R','I','F','F',0x24,0xAC,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,
        1,0,1,0,0x22,0x56,0,0,0x44,0xAC,0,0,2,0,16,0,'d','a','t','a',0,0xAC,0,0};
    const std::string silence(44032,'\0');
    {
        std::ofstream file(sounds/"volume-test.wav",std::ios::binary);
        file.write(reinterpret_cast<const char*>(header),sizeof(header));file.write(silence.data(),silence.size());
    }
    GameAudio audio;
    // Failed/uninitialized audio remains safe; configured tests query the actual XAudio2 master gain.
    audio.SetVolumePercent(50);RequireVolume(audio,0);
    audio.Configure(root.string());RequireVolume(audio,100);
    const auto ini=(root/"WarcraftCS.ini").string();
    for (const auto value : {"50","0","12.5","100"}) {
        WritePrivateProfileStringA("Audio","CSVolumePercent",value,ini.c_str());
        audio.Play("volume-test.wav");
        // Use the same validated snapshot-to-mixer path as startup/F8, while voices remain queued.
        const auto settings=GameplaySettings::Load(ini);audio.SetVolumePercent(settings.csVolumePercent);
        RequireVolume(audio,std::stof(value));
    }
    // Defensive mixer bounds also protect callers outside the config loader.
    audio.SetVolumePercent(-10);RequireVolume(audio,0);
    audio.SetVolumePercent(200);RequireVolume(audio,100);
    audio.SetVolumePercent(std::numeric_limits<float>::quiet_NaN());RequireVolume(audio,100);
    audio.Stop();
    puts("PASS real XAudio2 CS master gain, config reload, mute/restore, bounds and silent active voices.");
}
