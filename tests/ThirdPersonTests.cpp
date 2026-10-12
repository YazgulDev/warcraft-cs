#include "../src/presentation/ThirdPersonPose.hpp"
#include "../src/combat/HeroAttackTimeline.hpp"
#include "../src/presentation/HeroMotionState.hpp"
#include "../src/config/GameplaySettings.hpp"
#include <cassert>
#include <cmath>
#include <fstream>
namespace wc3 {void Log(const char*,...) {}}
int main() {
    // Camera direction, pivot and boom stay collinear through complete turns and shortened collision distances.
    auto pose=ThirdPersonPose::Calculate(100,200,80,0,0,35,350);
    assert(pose.eye[0]==-250 && pose.eye[1]==200 && pose.eye[2]==115);
    auto rotated=ThirdPersonPose::Calculate(100,200,80,450,-30,35,350);
    assert(std::abs(rotated.eye[0]-100)<.001 && rotated.eye[1]<200 && rotated.eye[2]>115);
    rotated.Place(0);assert(rotated.eye==rotated.pivot);
    // Delayed single contact, IAS scaling, interruption and recovery prevent double hits and mode-switch exploits.
    HeroAttackTimeline swing;
    assert(swing.Begin(10,.56f,2.2f,2));assert(!swing.Begin(10,.56f,2.2f,2));
    assert(!swing.Contact(10.2));assert(swing.Contact(10.3));assert(!swing.Contact(10.4));
    assert(!swing.Begin(10.9,.56f,2.2f,2));assert(swing.Begin(11.2,.56f,2.2f,2));
    swing.Cancel();assert(!swing.Contact(20));assert(!swing.Begin(11.3,.56f,2.2f,2));
    swing.Reset();assert(swing.Begin(0,5,1,1));assert(!swing.Contact(.9));assert(swing.Contact(1));
    // Jump contact transitions persist through air steering; landing recovers to run or idle without replay.
    HeroMotionState motion;ThirdPersonSettings motionSettings;using Pose=HeroMotionState::Pose;
    assert(motion.Update(1,true,0,motionSettings)==Pose::Stand);
    assert(motion.Update(2,true,270,motionSettings)==Pose::Run);
    assert(motion.Update(3,false,270,motionSettings)==Pose::Takeoff);
    assert(motion.Update(3.05,false,0,motionSettings)==Pose::Takeoff);
    assert(motion.Update(3.2,false,100,motionSettings)==Pose::Air);
    assert(motion.Update(3.5,true,0,motionSettings)==Pose::Land);
    assert(motion.Update(3.6,true,270,motionSettings)==Pose::Land);
    assert(motion.Update(3.7,true,270,motionSettings)==Pose::Run);
    motion.Reset();motionSettings.landingDuration=0;
    motion.Update(5,false,0,motionSettings);assert(motion.Update(6,true,0,motionSettings)==Pose::Stand);
    // Real INI fallback, finite bounds, timing coupling and live reload use the same production parser.
    char path[MAX_PATH];GetFullPathNameA("third-person-settings-test.ini",MAX_PATH,path,nullptr);
    {std::ofstream file(path);file<<"[ThirdPerson]\nCameraDistance=nan\nInitialPitch=bad\nAttackInterval=.4\nContactDelay=2\nAttackRange=-10\nAttackArc=500\nRunCycleDistance=0\nTakeoffDuration=nan\nLandingDuration=5\nMovementAnimations=bad\n";}
    auto settings=GameplaySettings::Load(path);
    assert(settings.thirdPerson.enabled && settings.thirdPerson.distance==500 && settings.thirdPerson.initialPitch==-12);
    assert(settings.thirdPerson.attackRange==30 && settings.thirdPerson.attackArc==120);
    assert(settings.thirdPerson.contactDelay==settings.thirdPerson.attackInterval);
    assert(settings.thirdPerson.runCycleDistance==40 && settings.thirdPerson.takeoffDuration==.10f);
    assert(settings.thirdPerson.landingDuration==.5f && settings.thirdPerson.movementAnimations);
    WritePrivateProfileStringA("ThirdPerson","CameraDistance","500",path);
    WritePrivateProfileStringA("ThirdPerson","InitialPitch","-100",path);
    WritePrivateProfileStringA("ThirdPerson","Enabled","false",path);
    WritePrivateProfileStringA("ThirdPerson","MovementAnimations","false",path);
    WritePrivateProfileStringA("ThirdPerson","RunCycleDistance","250",path);
    WritePrivateProfileStringA("Shortcuts","ToggleView","TAB",path);
    auto reloaded=GameplaySettings::Load(path);
    assert(!reloaded.thirdPerson.enabled && reloaded.thirdPerson.distance==500 && reloaded.thirdPerson.initialPitch==-60);
    assert(reloaded.shortcuts.Key(ShortcutAction::ToggleView)==VK_TAB);
    assert(!reloaded.thirdPerson.movementAnimations && reloaded.thirdPerson.runCycleDistance==250);
    DeleteFileA(path);
}
