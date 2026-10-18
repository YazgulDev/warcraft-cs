#include "../src/audio/GameAudio.hpp"
#include "../src/config/GameplaySettings.hpp"
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <objbase.h>

static std::vector<std::string> output;
static size_t SoundCount(const char* sound) {
    std::string prefix=std::string("CS sound=")+sound+" volume=";
    return std::count_if(output.begin(),output.end(),[&](const std::string& message){return message.find(prefix)==0;});
}

namespace wc3 {
    void Log(const char* format,...) {
        char text[2048];va_list arguments;va_start(arguments,format);vsnprintf(text,sizeof(text),format,arguments);va_end(arguments);
        output.emplace_back(text);puts(text);
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
    for(auto name:{"m3_insertshell.wav","m3_pump.wav"})std::filesystem::copy_file(sounds/"volume-test.wav",sounds/name);
    // An incompatible COM apartment forces optional audio setup to fail; model timing must still load.
    {
        std::ofstream timeline(root/"assets"/"m3.wca",std::ios::binary);
        uint32_t sequences=2,events=1;float duration=.75f;char name[32]="insert";
        timeline.write("WCA1",4);timeline.write(reinterpret_cast<const char*>(&sequences),4);
        timeline.write(name,32);timeline.write(reinterpret_cast<const char*>(&duration),4);
        timeline.write(reinterpret_cast<const char*>(&events),4);
        float when=0;char sound[64]="m3_insertshell.wav";
        timeline.write(reinterpret_cast<const char*>(&when),4);timeline.write(sound,64);
        char finish[32]="after_reload";events=0;timeline.write(finish,32);
        timeline.write(reinterpret_cast<const char*>(&duration),4);timeline.write(reinterpret_cast<const char*>(&events),4);
        std::ofstream xm(root/"assets"/"xm1014.wca",std::ios::binary);sequences=1;
        xm.write("WCA1",4);xm.write(reinterpret_cast<const char*>(&sequences),4);xm.write(name,32);
        xm.write(reinterpret_cast<const char*>(&duration),4);xm.write(reinterpret_cast<const char*>(&events),4);
    }
    assert(SUCCEEDED(CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED)));
    GameAudio withoutDevice;withoutDevice.Configure(root.string());RequireVolume(withoutDevice,0);
    assert(withoutDevice.Duration(WeaponSlots::M3,"insert")==.75f);
    CoUninitialize();
    // Failed/uninitialized audio remains safe; configured tests query the actual XAudio2 master gain.
    audio.SetVolumePercent(50);RequireVolume(audio,0);
    audio.Configure(root.string());RequireVolume(audio,100);
    // Both authored M3 audio and the missing-event XM fallback play exactly one real source voice per insert.
    size_t before=SoundCount("m3_insertshell.wav");DWORD now=GetTickCount();
    audio.BeginAnimation(WeaponSlots::M3,"insert",now);audio.Tick(now+1000);
    assert(SoundCount("m3_insertshell.wav")==before+1);
    audio.BeginAnimation(WeaponSlots::XM1014,"insert",now);audio.Tick(now+1000);
    assert(SoundCount("m3_insertshell.wav")==before+2);
    audio.BeginAnimation(WeaponSlots::M3,"after_reload",now);audio.Tick(now+1000);
    assert(SoundCount("m3_pump.wav")==1);
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
