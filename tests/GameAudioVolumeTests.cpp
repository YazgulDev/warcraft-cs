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
        output.emplace_back(text);puts(text);fflush(stdout);
    }
}
static void RequireVolume(const GameAudio& audio,float expected) {
    assert(std::abs(audio.VolumePercent()-expected)<0.001f);
}
static void RequirePitch(const GameAudio& audio,float expected) {
    // Real XAudio2 applies source pitch on its next processing pass; wait for the device rather than a setter echo.
    DWORD deadline=GetTickCount()+250;
    while(std::abs(audio.LoopPitch("volume-test.wav")-expected)>=.001f && GetTickCount()<deadline)Sleep(1);
    assert(std::abs(audio.LoopPitch("volume-test.wav")-expected)<.001f);
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
    // Odd-length PCM followed by unpadded LIST metadata must still reach a real XAudio2 voice.
    {
        const unsigned char odd[]={
            'R','I','F','F',51,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,
            1,0,1,0,0x22,0x56,0,0,0x22,0x56,0,0,1,0,8,0,'d','a','t','a',3,0,0,0,
            128,128,128,'L','I','S','T',4,0,0,0,'I','N','F','O'};
        std::ofstream file(sounds/"odd-pin.wav",std::ios::binary);file.write(reinterpret_cast<const char*>(odd),sizeof(odd));
    }
    for(auto name:{"m3_insertshell.wav","m3_pump.wav","hl_egon_windup2.wav"})std::filesystem::copy_file(sounds/"volume-test.wav",sounds/name);
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
    assert(audio.HasClip("odd-pin.wav") && !audio.HasClip("nuclear_blast.wav"));
    audio.Play("odd-pin.wav",0);assert(SoundCount("odd-pin.wav")==1);
    // Motor loops start/stop once, including repeated render updates and the ordinary stop-all boundary.
    auto loopEvents=[&]{return std::count_if(output.begin(),output.end(),[](const std::string& s){return s.find("CS loop=volume-test.wav")==0;});};
    auto loopBefore=loopEvents();audio.SetLoop("volume-test.wav",true,0);audio.SetLoop("volume-test.wav",true,0);
    assert(loopEvents()==loopBefore+1);audio.SetLoop("volume-test.wav",false);assert(loopEvents()==loopBefore+2);
    // The live motor crosses XAudio's default 2x ceiling without resubmitting the loop or affecting other voices.
    audio.SetLoop("volume-test.wav",true,0);auto queued=loopEvents();
    for(float pitch:{1.f,1.25f,1.75f,2.5f,4.f}){audio.SetLoopPitch("volume-test.wav",pitch);RequirePitch(audio,pitch);}
    assert(loopEvents()==queued);
    audio.SetLoopPitch("volume-test.wav",100);RequirePitch(audio,4);
    audio.SetLoopPitch("volume-test.wav",std::numeric_limits<float>::quiet_NaN());RequirePitch(audio,1);
    audio.SetLoop("volume-test.wav",false);RequirePitch(audio,1);
    // A stop log is insufficient: the real device cursor must stop consuming the former infinite buffer.
    unsigned leaked=0;
    for(int i=0;i<12;++i){
        audio.SetLoop("volume-test.wav",true,0);Sleep(30);
        audio.SetLoop("volume-test.wav",false);Sleep(40);auto stopped=audio.SamplesPlayed("volume-test.wav");
        Sleep(40);if(audio.SamplesPlayed("volume-test.wav")!=stopped)++leaked;
    }
    printf("Audio motor still consuming after release: %u/12\n",leaked);fflush(stdout);assert(!leaked);
    // A brief Gluon tap cancels its long startup cue, preserves another sound and can start again immediately.
    audio.Play("hl_egon_windup2.wav",0);audio.Play("m3_pump.wav",0);Sleep(30);
    assert(audio.SamplesPlayed("hl_egon_windup2.wav")>0);
    audio.StopClip("hl_egon_windup2.wav");audio.StopClip("missing.wav");Sleep(40);
    auto canceled=audio.SamplesPlayed("hl_egon_windup2.wav"),other=audio.SamplesPlayed("m3_pump.wav");Sleep(40);
    assert(audio.SamplesPlayed("hl_egon_windup2.wav")==canceled && audio.SamplesPlayed("m3_pump.wav")>other);
    audio.Play("hl_egon_windup2.wav",0);Sleep(30);auto restarted=audio.SamplesPlayed("hl_egon_windup2.wav");Sleep(30);
    assert(audio.SamplesPlayed("hl_egon_windup2.wav")>restarted);audio.Stop();Sleep(40);
    canceled=audio.SamplesPlayed("hl_egon_windup2.wav");Sleep(40);assert(audio.SamplesPlayed("hl_egon_windup2.wav")==canceled);
    // One-shots of the motor sample cannot borrow its retained loop voice after release.
    for(int i=0;i<16;++i)audio.Play("volume-test.wav",0);
    Sleep(1200);auto finished=audio.SamplesPlayed("volume-test.wav");Sleep(40);assert(audio.SamplesPlayed("volume-test.wav")==finished);
    // Both authored M3 audio and the missing-event XM fallback play exactly one real source voice per insert.
    size_t pumps=SoundCount("m3_pump.wav"),before=SoundCount("m3_insertshell.wav");DWORD now=GetTickCount();
    audio.BeginAnimation(WeaponSlots::M3,"insert",now);audio.Tick(now+1000);
    assert(SoundCount("m3_insertshell.wav")==before+1);
    audio.BeginAnimation(WeaponSlots::XM1014,"insert",now);audio.Tick(now+1000);
    assert(SoundCount("m3_insertshell.wav")==before+2);
    audio.BeginAnimation(WeaponSlots::M3,"after_reload",now);audio.Tick(now+1000);
    assert(SoundCount("m3_pump.wav")==pumps+1);
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
