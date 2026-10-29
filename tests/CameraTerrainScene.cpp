#include "CameraTerrainScene.hpp"
#include "../src/presentation/FirstPersonCamera.hpp"
#include "../src/platform/WarcraftApi.hpp"
#include "../src/platform/WarcraftCollision.hpp"
#include "../src/config/GameplaySettings.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>

namespace {
FirstPersonCamera camera;
std::string mode;
float x=0,y=0,yaw=0,wantedZ=0,preZ=0,oldBaseline=0,previousActual=0,previousWanted=0;
float maximumError=0,maximumDeltaError=0,maximumSubmittedError=0;
unsigned frame=0,duration=0;
bool writing=false;
MovementPhysics movement;
WarcraftCollision collision;
wc3::Handle walker=0;
}
bool CameraTerrainScene::BlocksMapCamera(){return !mode.empty() && !writing;}
void CameraTerrainScene::BeforeWorld(uintptr_t base,const char* root) {
    auto request=std::string(root)+"\\test-camera-terrain.request";
    if(GetFileAttributesA(request.c_str())!=INVALID_FILE_ATTRIBUTES) {
        std::ifstream file(request);file>>mode>>x>>y>>yaw>>duration;file.close();DeleteFileA(request.c_str());
        if(mode!="level" && mode!="ground" && mode!="still" && mode!="walk" && mode!="hop"){mode.clear();return;}
        duration=std::clamp(duration,60u,1200u);frame=0;maximumError=maximumDeltaError=maximumSubmittedError=0;
        if(walker && wc3::GetUnitTypeId(walker))wc3::RemoveUnit(walker);walker=0;
        if(mode=="walk" || mode=="hop") {
            // The ordinary collision/physics adapters walk the same private slope; no artificial support is substituted.
            float facing=yaw;walker=wc3::CreateUnit(wc3::GetLocalPlayer(),0x68666F6F,&x,&y,&facing);
            wc3::PauseUnit(walker,TRUE);wc3::SetUnitX(walker,&x);wc3::SetUnitY(walker,&y);
            auto settings=GameplaySettings::Load(std::string(root)+"\\WarcraftCS.ini");movement.Configure(settings.movement);
            movement.Reset(wc3::Ground(x,y));collision.Configure(base);collision.Reset();
        }
        wc3::Log("camera terrain oracle start mode=%s x=%.1f y=%.1f yaw=%.1f frames=%u",mode.c_str(),x,y,yaw,duration);
    }
    if(mode.empty())return;
    // A level eye over changing terrain isolates native camera compensation from collision and stance.
    if(walker) {
        MoveInput input;input.forward=1;input.yaw=yaw;input.jump=mode=="hop";
        movement.Step(input,1.f/75,250);collision.Move(walker,movement,1.f/75);
        x=wc3::Real(wc3::GetUnitX(walker));y=wc3::Real(wc3::GetUnitY(walker));wantedZ=movement.EyeZ();
    } else {
        if(mode!="still") {x+=std::cos(yaw*.01745329252f)*5.0f;y+=std::sin(yaw*.01745329252f)*5.0f;}
        wantedZ=mode=="ground" ? wc3::Ground(x,y)+96 : 700;
    }
    oldBaseline=wc3::Real(wc3::GetCameraTargetPositionZ())-wc3::Real(wc3::GetCameraField(6));
    writing=true;camera.Update(x,y,wantedZ,yaw,0,85,5000);
    writing=false;preZ=wc3::Real(wc3::GetCameraEyePositionZ());
}
void CameraTerrainScene::AfterWorld() {
    if(mode.empty())return;
    float actual=wc3::Real(wc3::GetCameraEyePositionZ());
    float baseline=wc3::Real(wc3::GetCameraTargetPositionZ())-wc3::Real(wc3::GetCameraField(6));
    float error=actual-wantedZ,deltaError=frame ? (actual-previousActual)-(wantedZ-previousWanted) : 0;
    if(frame>30){
        maximumError=std::max(maximumError,std::abs(error));maximumDeltaError=std::max(maximumDeltaError,std::abs(deltaError));
        // Native getter vectors expose the preceding submitted frame, also for X/Y and jumps.
        maximumSubmittedError=std::max(maximumSubmittedError,std::abs(actual-previousWanted));
    }
    wc3::Log("camera terrain oracle frame=%u mode=%s x=%.2f y=%.2f wanted=%.3f before=%.3f actual=%.3f error=%.3f deltaError=%.3f baseline=%.3f->%.3f",
        frame,mode.c_str(),x,y,wantedZ,preZ,actual,error,deltaError,oldBaseline,baseline);
    previousActual=actual;previousWanted=wantedZ;
    if(++frame>=duration) {
        wc3::Log("camera terrain oracle result mode=%s maxError=%.4f maxDeltaError=%.4f submittedEyeError=%.4f %s",mode.c_str(),maximumError,maximumDeltaError,maximumSubmittedError,
            maximumSubmittedError<.05f ? "PASS" : "FAIL");mode.clear();
    }
}
