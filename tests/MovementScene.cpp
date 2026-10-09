#include "../src/platform/WarcraftCollision.hpp"
#include "../src/config/GameplaySettings.hpp"
#include "../src/presentation/FirstPersonCamera.hpp"
#include "MovementScene.hpp"
#include <fstream>
#include <sstream>

namespace MovementScene {
void Tick(uintptr_t base, const char* root) {
    static wc3::Handle unit=0, prop=0, wall=0;
    static MovementPhysics movement;
    static WarcraftCollision collision;
    static FirstPersonCamera camera;
    static std::string mode;
    static float startX=0, startY=0, startZ=0, yaw=0, peakSpeed=0;
    static int frame=0, duration=600, hops=0;
    static bool fell=false;
    std::string request=std::string(root)+"\\test-movement.request";
    if (GetFileAttributesA(request.c_str()) != INVALID_FILE_ATTRIBUTES) {
        std::ifstream file(request); std::string command; float x=0,y=0; file>>command>>x>>y>>yaw>>duration;
        file.close(); DeleteFileA(request.c_str());
        collision.Configure(base);
        if (command=="scan" || command=="flat") {
            // Native terrain/pathing samples locate an actual downward cliff, without inventing a test floor.
            auto bounds=wc3::GetWorldBounds(); int count=0;
            for (float px=wc3::Real(wc3::GetRectMinX(bounds))+512;px<wc3::Real(wc3::GetRectMaxX(bounds))-512 && count<12;px+=128)
                for (float py=wc3::Real(wc3::GetRectMinY(bounds))+512;py<wc3::Real(wc3::GetRectMaxY(bounds))-512 && count<12;py+=128) {
                    float nx=px+256, z=wc3::TerrainGround(px,py), nz=wc3::TerrainGround(nx,py);
                    if (command=="flat") {
                        // A full corridor avoids confusing a native wall or hill with loss of hop momentum.
                        bool clear=true;float minimum=z,maximum=z;
                        for(int offset=0;offset<=2000 && clear;offset+=32) {
                            float sx=px+offset,sy=py,h=wc3::TerrainGround(sx,sy);
                            minimum=std::min(minimum,h);maximum=std::max(maximum,h);
                            clear=sx<wc3::Real(wc3::GetRectMaxX(bounds))-128 && maximum-minimum<20 && !wc3::IsTerrainPathable(&sx,&sy,1);
                        }
                        if(clear) {wc3::Log("movement oracle flat candidate x=%.0f y=%.0f variation=%.1f",px,py,maximum-minimum);++count;}
                    } else if (z-nz>=100 && wc3::GetTerrainCliffLevel(&px,&py)>wc3::GetTerrainCliffLevel(&nx,&py) && !wc3::IsTerrainPathable(&px,&py,1) && !wc3::IsTerrainPathable(&nx,&py,1)) {
                        wc3::Log("movement oracle cliff candidate x=%.0f y=%.0f high=%.1f low=%.1f",px,py,z,nz); ++count;
                    }
                }
            wc3::RemoveRect(bounds); return;
        }
        if (command!="hop" && command!="prop" && command!="walkprop" && command!="wall" && command!="cliff") return;
        if (unit && wc3::GetUnitTypeId(unit)) wc3::RemoveUnit(unit);
        if (prop && wc3::GetDestructableTypeId(prop)) wc3::RemoveDestructable(prop);
        if (wall && wc3::GetUnitTypeId(wall)) wc3::RemoveUnit(wall);
        wall=0;
        prop=0;
        // Explicit requests operate only in a throwaway map: clear a narrow trial corridor, never campaign saves.
        for (auto object:wc3::NearbyDestructables(x+800,y,1600)) wc3::RemoveDestructable(object);
        float facing=yaw;
        unit=wc3::CreateUnit(wc3::GetLocalPlayer(),0x68666F6F,&x,&y,&facing);
        wc3::PauseUnit(unit,TRUE);
        startX=wc3::Real(wc3::GetUnitX(unit));startY=wc3::Real(wc3::GetUnitY(unit));
        startZ=wc3::Ground(startX,startY);
        movement.Configure(GameplaySettings::Load(std::string(root)+"\\WarcraftCS.ini").movement);
        movement.Reset(startZ);collision.Reset();mode=command;frame=hops=0;peakSpeed=0;fell=false;
        duration=std::clamp(duration,60,1200);
        if(mode=="prop" || mode=="walkprop") {
            // A scaled native barricade provides real model geometry and a Warcraft pathing footprint.
            float px=std::ceil((startX+200)/64)*64,py=std::round(startY/64)*64,scale=.75f,angle=0;
            prop=wc3::CreateDestructable(0x4C546261,&px,&py,&angle,&scale,0);
            // Place on Warcraft's destructable grid without moving the actor into an unwalkable border cell.
            wc3::Log("movement oracle prop handle=%08X x=%.1f y=%.1f",prop,px,py);
            MovementObstacles geometry;geometry.Configure(base);float deck=0;
            wc3::Log("movement oracle prop terrain=%.1f ground=%.1f walkable=%d",wc3::TerrainGround(px,py),wc3::Ground(px,py),wc3::WalkableSurface(px,py,deck));
            for(const auto& box:geometry.Snapshot(unit,px,py,64))
                wc3::Log("movement oracle prop bounds min=%.1f,%.1f,%.1f max=%.1f,%.1f,%.1f",box.minimum[0],box.minimum[1],box.minimum[2],box.maximum[0],box.maximum[1],box.maximum[2]);
        }
        if(mode=="wall") {
            // A paused native town hall verifies unit-model discovery and a solid too tall for the default jump.
            float px=startX+240,py=startY,angle=0;
            wall=wc3::CreateUnit(wc3::Player(12),0x68746F77,&px,&py,&angle);wc3::PauseUnit(wall,TRUE);
            wc3::SetUnitX(wall,&px);wc3::SetUnitY(wall,&py);
        }
        wc3::Log("movement oracle start mode=%s x=%.1f y=%.1f floor=%.1f frames=%d",mode.c_str(),startX,startY,startZ,duration);
    }
    if(mode.empty() || !unit) return;
    MoveInput input;input.forward=1;input.yaw=yaw;
    input.jump=mode=="hop" ? frame>=60 : (mode=="prop" || mode=="wall") && frame>=20 && frame<65;
    bool grounded=movement.Grounded();movement.Step(input,1.f/60,250);
    if(grounded && !movement.Grounded()) ++hops;
    collision.Move(unit,movement,1.f/60);
    float x=wc3::Real(wc3::GetUnitX(unit)), y=wc3::Real(wc3::GetUnitY(unit));
    peakSpeed=std::max(peakSpeed,movement.Speed());
    if (!movement.Grounded() && movement.FeetZ()<startZ-1) fell=true;
    camera.Update(x,y,movement.EyeZ(),yaw,-10,85);
    if (++frame%30==0) wc3::Log("movement oracle progress mode=%s frame=%d x=%.1f y=%.1f feet=%.1f floor=%.1f speed=%.1f grounded=%d hops=%d",
        mode.c_str(),frame,x,y,movement.FeetZ(),wc3::Ground(x,y),movement.Speed(),movement.Grounded(),hops);
    if((mode=="prop" || mode=="walkprop") && frame==30) {
        // Creation may precede sprite initialization: inspect geometry again after native world rendering.
        MovementObstacles geometry;geometry.Configure(base);
        for(const auto& box:geometry.Snapshot(unit,x,y,600))
            wc3::Log("movement oracle live prop bounds min=%.1f,%.1f,%.1f max=%.1f,%.1f,%.1f",box.minimum[0],box.minimum[1],box.minimum[2],box.maximum[0],box.maximum[1],box.maximum[2]);
        wc3::Log("movement oracle actualProp=%.1f,%.1f",wc3::Real(wc3::GetDestructableX(prop)),wc3::Real(wc3::GetDestructableY(prop)));
        for(int point=0;point<9;++point) {
            float a=(point-1)*.78539816339f,px=x+(point?std::cos(a)*24:0),py=y+(point?std::sin(a)*24:0);
            wc3::Log("movement oracle path sample x=%.1f y=%.1f blocked=%d terrain=%.1f",px,py,wc3::IsTerrainPathable(&px,&py,1),wc3::TerrainGround(px,py));
        }
    }
    if(frame>=duration) {
        bool passed=mode=="hop" ? hops>=3 && peakSpeed>450 : mode=="cliff" ? fell && movement.Grounded() && movement.FeetZ()<startZ-80 : mode=="wall" ? x<wc3::Real(wc3::GetUnitX(wall))-24 : mode=="walkprop" ? x<wc3::Real(wc3::GetDestructableX(prop))-24 : x>wc3::Real(wc3::GetDestructableX(prop))+100;
        wc3::Log("movement oracle result mode=%s %s distance=%.1f peakSpeed=%.1f hops=%d fell=%d feet=%.1f",mode.c_str(),passed?"PASS":"FAIL",std::hypot(x-startX,y-startY),peakSpeed,hops,fell,movement.FeetZ());
        mode.clear();
    }
}
}
