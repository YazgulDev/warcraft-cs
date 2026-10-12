#include "../src/presentation/ThirdPersonPose.hpp"
#include "../src/combat/HeroAttackTimeline.hpp"
#include "../src/presentation/HeroMotionState.hpp"
#include "../src/presentation/ModelMotionProfile.hpp"
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
    assert(motion.Update(2.1,true,140,motionSettings,true)==Pose::Walk);
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
    // Exact model indices/durations win over guessed animation names; truncated/missing models remain safe.
    std::vector<uint8_t> model(12+4*132,0);std::memcpy(model.data(),"MDLXSEQS",8);
    uint32_t chunk=4*132;std::memcpy(model.data()+8,&chunk,4);
    const char* names[]={"Stand","Walk","Walk Fast","Jump"};
    for(int i=0;i<4;++i){auto at=12+i*132;std::memcpy(model.data()+at,names[i],std::strlen(names[i]));uint32_t end=1000;std::memcpy(model.data()+at+84,&end,4);}
    ModelMotionProfile profile;
    profile.Load(model,"C;X1;Y1;K\"unitUIID\"\nC;X2;K\"file\"\nC;X3;K\"walk\"\nC;X4;K\"run\"\nC;X1;Y2;K\"hfoo\"\nC;X2;K\"test\"\nC;X3;K100\nC;X4;K300\n","test.mdx",0x68666F6F);
    assert(profile.walk.index==1 && profile.run.index==2 && profile.takeoff.index==3 && profile.air.index==3);
    assert(profile.walkSpeed==100 && profile.runSpeed==300 && profile.run.seconds==1);
    model.resize(20);profile.Load(model,"","custom",0);assert(profile.walk.index==-1 && profile.runSpeed==0);
    // An old refill default must not reset a custom layout when the new dedicated entry key is absent.
    {std::ofstream file(path);file<<"[Shortcuts]\nRefill=F7\nSpeed=Y\n";}
    auto legacy=GameplaySettings::Load(path);
    assert(legacy.shortcuts.Key(ShortcutAction::ToggleThirdPerson)==VK_F7 && legacy.shortcuts.Key(ShortcutAction::Refill)==VK_F11 && legacy.shortcuts.Key(ShortcutAction::Speed)=='Y');
    WritePrivateProfileStringA("ThirdPerson","AutoAnimationSpeed","false",path);
    WritePrivateProfileStringA("ThirdPerson","AnimationSpeedMultiplier","999",path);
    auto manual=GameplaySettings::Load(path);assert(!manual.thirdPerson.autoAnimationSpeed && manual.thirdPerson.animationSpeedMultiplier==3);
    DeleteFileA(path);
}
