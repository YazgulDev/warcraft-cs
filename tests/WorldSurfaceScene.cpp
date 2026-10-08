#include "WorldSurfaceScene.hpp"
#include "../src/WarcraftCollision.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
namespace WorldSurfaceScene {
void Tick(const char* root) {
    static wc3::Handle unit=0;static MovementPhysics movement;static WarcraftCollision collision;
    static float startX=0,startY=0,yaw=0,length=0;static int frame=0;static bool running=false;
    std::string request=std::string(root)+"\\test-surface.request";
    if(GetFileAttributesA(request.c_str())!=INVALID_FILE_ATTRIBUTES){
        std::ifstream file(request);std::string command;float x=0,y=0;int axis=0;float span=600;
        file>>command>>x>>y>>axis>>span;file.close();DeleteFileA(request.c_str());
        if(command=="scan"){
            // A native oracle compares terrain pathing with actual walkable geometry along either axis.
            for(int offset=-1024;offset<=1024;offset+=32){
                float px=x+(axis==0 ? float(offset):0),py=y+(axis==1 ? float(offset):0),deck=0;
                bool raised=wc3::WalkableSurface(px,py,deck);
                wc3::Log("surface sample offset=%d x=%.1f y=%.1f terrainBlocked=%d raised=%d deck=%.2f ground=%.2f",offset,px,py,wc3::IsTerrainPathable(&px,&py,1),raised,deck,wc3::Ground(px,py));
            }
        }else if(command=="cross"){
            if(unit && wc3::GetUnitTypeId(unit))wc3::RemoveUnit(unit);
            float facing=axis==1 ? 90.f:0.f;
            unit=wc3::CreateUnit(wc3::GetLocalPlayer(),0x68666F6F,&x,&y,&facing);
            wc3::PauseUnit(unit,TRUE);startX=wc3::Real(wc3::GetUnitX(unit));startY=wc3::Real(wc3::GetUnitY(unit));
            yaw=facing;length=span;frame=0;running=true;movement.Reset(wc3::Ground(startX,startY));
            wc3::Log("surface crossing start x=%.1f y=%.1f floor=%.2f axis=%d length=%.1f",startX,startY,movement.FeetZ(),axis,length);
        }
    }
    if(!running)return;
    MoveInput input;input.forward=1;input.yaw=yaw;
    movement.Step(input,1.f/60,250);collision.Move(unit,movement,1.f/60);
    float x=wc3::Real(wc3::GetUnitX(unit)),y=wc3::Real(wc3::GetUnitY(unit));
    ++frame;
    if(frame%60==0)wc3::Log("surface crossing progress x=%.1f y=%.1f feet=%.2f distance=%.1f",x,y,movement.FeetZ(),std::hypot(x-startX,y-startY));
    if(std::hypot(x-startX,y-startY)>=length || frame>=600){
        wc3::Log("surface crossing %s x=%.1f y=%.1f distance=%.1f",std::hypot(x-startX,y-startY)>=length ? "PASS":"FAIL",x,y,std::hypot(x-startX,y-startY));
        wc3::RemoveUnit(unit);unit=0;running=false;
    }
}
}
