#include "../src/presentation/ThirdPersonPose.hpp"
#include "../src/combat/HeroAttackTimeline.hpp"
#include "../src/combat/NativeOrderCancellation.hpp"
#include "../src/presentation/HeroMotionState.hpp"
#include "../src/presentation/HeroAttackPose.hpp"
#include "../src/presentation/ModelMotionProfile.hpp"
#include "../src/config/GameplaySettings.hpp"
#include "../src/squad/SquadSpellPolicy.hpp"
#include "../src/input/CameraWheel.hpp"
#include <cassert>
#include <cmath>
#include <fstream>
namespace wc3 {void Log(const char*,...) {}}
int main() {
    // Eligibility and healing thresholds never grant mana, bypass cooldowns or revive a dead target.
    assert(SquadSpellPolicy::Ready(1,false,false,0) && !SquadSpellPolicy::Ready(0,false,false,0));
    assert(!SquadSpellPolicy::Ready(1,true,false,0) && !SquadSpellPolicy::Ready(1,false,true,0));
    assert(!SquadSpellPolicy::Ready(1,false,false,1));
    assert(SquadSpellPolicy::NeedsHealing(200,500,85) && !SquadSpellPolicy::NeedsHealing(0,500,85));
    assert(!SquadSpellPolicy::NeedsHealing(500,500,85) && !SquadSpellPolicy::NeedsHealing(200,0,85));
    // Stop may leave a sticky target in Dota. Repeated idle observations must not interrupt manual attack playback.
    NativeOrderCancellation orders;
    assert(orders.Observe(1,100,0));assert(!orders.Observe(1,100,851972));assert(!orders.Observe(1,100,0));
    assert(orders.Observe(1,200,0));assert(orders.Observe(1,200,851983));
    assert(!orders.Observe(1,0,0));assert(orders.Observe(1,200,0));assert(orders.Observe(2,200,0));
    assert(!orders.Observe(1,200,0));
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
    // An authored clip ends before DPS recovery. Playback advances instead of restarting after pose overwrites.
    HeroAttackPose visual;visual.Begin(10,.8f,2,.28f,1.1f);
    assert(visual.Active(10.3) && std::abs(visual.Elapsed(10.3)-.6f)<.001f);
    assert(!visual.Active(10.5));assert(visual.Elapsed(11)<.8f);
    visual.Begin(20,.1f,1,.56f,2.2f);assert(visual.Active(20.5) && !visual.Active(20.6));
    visual.Begin(30,10,1,.56f,2.2f);assert(visual.Active(32.3) && !visual.Active(40));
    visual.Begin(40,.8f,0,.56f,2.2f);assert(visual.Elapsed(41)==0 && visual.Active(41));
    visual.Reset();assert(!visual.Active(0));
    assert(!GameplaySettings{}.targetAllies && ShortcutBindings{}.Key(ShortcutAction::ToggleAllyTargeting)=='Q');
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
    assert(settings.unitViewEnabled && settings.unitViewPreventEnemyAttacks && settings.squadAutomaticSpells);
    assert(!settings.waterBlocksAttacks);
    WritePrivateProfileStringA("Combat","WaterBlocksAttacks","1",path);assert(GameplaySettings::Load(path).waterBlocksAttacks);
    WritePrivateProfileStringA("Combat","WaterBlocksAttacks","bad",path);assert(!GameplaySettings::Load(path).waterBlocksAttacks);
    WritePrivateProfileStringA("Combat","WaterBlocksAttacks","false",path);assert(!GameplaySettings::Load(path).waterBlocksAttacks);
    assert(settings.shortcuts.Key(ShortcutAction::ToggleUnitView)==VK_F9 && settings.shortcuts.Key(ShortcutAction::AllWeapons)==VK_F12);
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
    // F8 re-parses AI/view preferences and migrates the legacy F9 binding independently of the keyboard layout.
    WritePrivateProfileStringA("UnitView","Enabled","false",path);
    WritePrivateProfileStringA("UnitView","PreventEnemyAttacks","0",path);
    WritePrivateProfileStringA("Squad","AutomaticSpells","false",path);
    WritePrivateProfileStringA("Squad","SpellSearchRadius","nan",path);
    WritePrivateProfileStringA("Squad","HealBelowPercent","500",path);
    auto aiReload=GameplaySettings::Load(path);
    assert(!aiReload.unitViewEnabled && !aiReload.unitViewPreventEnemyAttacks && !aiReload.squadAutomaticSpells && aiReload.squadSpellRadius==700 && aiReload.squadHealBelowPercent==100);
    // Malformed protection values retain the requested default, and F8 can explicitly re-enable it.
    WritePrivateProfileStringA("UnitView","PreventEnemyAttacks","bad",path);
    assert(GameplaySettings::Load(path).unitViewPreventEnemyAttacks);
    WritePrivateProfileStringA("UnitView","PreventEnemyAttacks","1",path);
    assert(GameplaySettings::Load(path).unitViewPreventEnemyAttacks);
    WritePrivateProfileStringA("Shortcuts","AllWeapons","F9",path);
    auto oldF9=GameplaySettings::Load(path);assert(oldF9.shortcuts.Key(ShortcutAction::ToggleUnitView)==VK_F9 && oldF9.shortcuts.Key(ShortcutAction::AllWeapons)==VK_F12);
    // Introducing the new default cannot reset a partial layout with a custom F12 refill.
    WritePrivateProfileStringA("Shortcuts","AllWeapons",nullptr,path);
    WritePrivateProfileStringA("Shortcuts","Refill","F12",path);
    auto partial=GameplaySettings::Load(path);
    assert(partial.shortcuts.Key(ShortcutAction::Refill)==VK_F12 && partial.shortcuts.Key(ShortcutAction::AllWeapons)==VK_F5);
    assert(!reloaded.thirdPerson.enabled && reloaded.thirdPerson.distance==500 && reloaded.thirdPerson.initialPitch==-60);
    assert(reloaded.shortcuts.Key(ShortcutAction::ToggleView)==VK_TAB);
    assert(!reloaded.thirdPerson.movementAnimations && reloaded.thirdPerson.runCycleDistance==250);
    // Startup, invalid values, F8 parsing and custom shortcut upgrades share the production INI path.
    WritePrivateProfileStringA("Combat","TargetAllies","bad",path);assert(!GameplaySettings::Load(path).targetAllies);
    WritePrivateProfileStringA("Combat","TargetAllies","true",path);assert(GameplaySettings::Load(path).targetAllies);
    WritePrivateProfileStringA("Combat","TargetAllies","0",path);assert(!GameplaySettings::Load(path).targetAllies);
    WritePrivateProfileStringA("Shortcuts","ToggleAllyTargeting","F5",path);
    assert(GameplaySettings::Load(path).shortcuts.Key(ShortcutAction::ToggleAllyTargeting)==VK_F5);
    WritePrivateProfileStringA("Shortcuts","ToggleAllyTargeting",nullptr,path);WritePrivateProfileStringA("Shortcuts","Reload","Q",path);
    auto legacyQ=GameplaySettings::Load(path);assert(legacyQ.shortcuts.Key(ShortcutAction::Reload)=='Q' && !legacyQ.shortcuts.Key(ShortcutAction::ToggleAllyTargeting));
    // Exact model indices/durations win over guessed animation names; truncated/missing models remain safe.
    std::vector<uint8_t> model(12+6*132,0);std::memcpy(model.data(),"MDLXSEQS",8);
    uint32_t chunk=6*132;std::memcpy(model.data()+8,&chunk,4);
    const char* names[]={"Stand","Walk","Walk Fast","Jump","Attack","Attack Alternate"};
    for(int i=0;i<6;++i){auto at=12+i*132;std::memcpy(model.data()+at,names[i],std::strlen(names[i]));uint32_t end=i==5 ? 750 : 1000;std::memcpy(model.data()+at+84,&end,4);}
    ModelMotionProfile profile;
    profile.Load(model,"C;X1;Y1;K\"unitUIID\"\nC;X2;K\"file\"\nC;X3;K\"walk\"\nC;X4;K\"run\"\nC;X1;Y2;K\"hfoo\"\nC;X2;K\"test\"\nC;X3;K100\nC;X4;K300\n","test.mdx",0x68666F6F);
    assert(profile.walk.index==1 && profile.run.index==2 && profile.takeoff.index==3 && profile.air.index==3);
    assert(profile.walkSpeed==100 && profile.runSpeed==300 && profile.run.seconds==1);
    assert(profile.attack.index==4 && std::abs(profile.SequenceAt(5).seconds-.75f)<.001f && profile.SequenceAt(999).index==-1);
    model.resize(20);profile.Load(model,"","custom",0);assert(profile.walk.index==-1 && profile.runSpeed==0);
    // An old refill default must not reset a custom layout when the new dedicated entry key is absent.
    {std::ofstream file(path);file<<"[Shortcuts]\nRefill=F7\nSpeed=Y\n";}
    auto legacy=GameplaySettings::Load(path);
    assert(legacy.shortcuts.Key(ShortcutAction::ToggleThirdPerson)==VK_F7 && legacy.shortcuts.Key(ShortcutAction::Refill)==VK_F11 && legacy.shortcuts.Key(ShortcutAction::Speed)=='Y');
    WritePrivateProfileStringA("ThirdPerson","AutoAnimationSpeed","false",path);
    WritePrivateProfileStringA("ThirdPerson","AnimationSpeedMultiplier","999",path);
    auto manual=GameplaySettings::Load(path);assert(!manual.thirdPerson.autoAnimationSpeed && manual.thirdPerson.animationSpeedMultiplier==3);
    // Ranged/manual reach and the short-creep sweep obey actual INI bounds and explicit opt-outs on reload.
    WritePrivateProfileStringA("ThirdPerson","UseNativeAttackRange","false",path);
    WritePrivateProfileStringA("ThirdPerson","NativeRangedProjectiles","false",path);
    WritePrivateProfileStringA("ThirdPerson","RangedAttackRange","999999",path);
    WritePrivateProfileStringA("ThirdPerson","RangedRangeMultiplier","nan",path);
    WritePrivateProfileStringA("ThirdPerson","MeleeHeightTolerance","-10",path);
    auto ranged=GameplaySettings::Load(path);
    assert(!ranged.thirdPerson.useNativeAttackRange && !ranged.thirdPerson.nativeRangedProjectiles);
    assert(ranged.thirdPerson.rangedAttackRange==5000 && ranged.thirdPerson.rangedRangeMultiplier==1 && ranged.thirdPerson.meleeHeightTolerance==0);
    // The actual config boundary supports hiding/reloading the marker and defaults safely on malformed values.
    assert(ranged.thirdPerson.showTargetIndicator);
    WritePrivateProfileStringA("ThirdPerson","ShowTargetIndicator","false",path);
    assert(!GameplaySettings::Load(path).thirdPerson.showTargetIndicator);
    WritePrivateProfileStringA("ThirdPerson","ShowTargetIndicator","broken",path);
    assert(GameplaySettings::Load(path).thirdPerson.showTargetIndicator);
    // Circle radius is a world-space setting: validate its real INI fallback, bounds and subsequent reload.
    assert(GameplaySettings::Load(path).thirdPerson.targetCircleRadius==64);
    WritePrivateProfileStringA("ThirdPerson","TargetCircleRadius","99999",path);
    assert(GameplaySettings::Load(path).thirdPerson.targetCircleRadius==256);
    WritePrivateProfileStringA("ThirdPerson","TargetCircleRadius","nan",path);
    assert(GameplaySettings::Load(path).thirdPerson.targetCircleRadius==64);
    WritePrivateProfileStringA("ThirdPerson","TargetCircleRadius","0",path);
    assert(GameplaySettings::Load(path).thirdPerson.targetCircleRadius==16);
    WritePrivateProfileStringA("ThirdPerson","TargetCircleRadius","96",path);
    assert(GameplaySettings::Load(path).thirdPerson.targetCircleRadius==96);
    // Zoom consumes real validated config, retains partial notches, clamps and never replays input.
    CameraWheel wheel;auto zoom=GameplaySettings::Load(path).thirdPerson;
    assert(zoom.zoomStep==50 && zoom.minimumDistance==80 && zoom.maximumDistance==1000);
    wheel.Add(120);assert(wheel.Take(500,zoom)==450);assert(wheel.Take(450,zoom)==450);
    wheel.Add(-360);assert(wheel.Take(450,zoom)==600);
    wheel.Add(60);assert(wheel.Take(500,zoom)==500);wheel.Add(60);assert(wheel.Take(500,zoom)==450);
    wheel.Add(60);wheel.Reset();wheel.Add(60);assert(wheel.Take(500,zoom)==500);wheel.Reset();
    wheel.Add(12000);assert(wheel.Take(500,zoom)==80);wheel.Add(-12000);assert(wheel.Take(80,zoom)==1000);
    WritePrivateProfileStringA("ThirdPerson","CameraZoomStep","nan",path);
    assert(GameplaySettings::Load(path).thirdPerson.zoomStep==50);
    WritePrivateProfileStringA("ThirdPerson","CameraZoomStep","99999",path);
    assert(GameplaySettings::Load(path).thirdPerson.zoomStep==500);
    WritePrivateProfileStringA("ThirdPerson","CameraZoomStep","0",path);
    wheel.Add(120);assert(wheel.Take(500,GameplaySettings::Load(path).thirdPerson)==500);
    WritePrivateProfileStringA("ThirdPerson","CameraMinDistance","nan",path);
    WritePrivateProfileStringA("ThirdPerson","CameraMaxDistance","bad",path);
    zoom=GameplaySettings::Load(path).thirdPerson;assert(zoom.minimumDistance==80 && zoom.maximumDistance==1000);
    WritePrivateProfileStringA("ThirdPerson","CameraMinDistance","99999",path);
    WritePrivateProfileStringA("ThirdPerson","CameraMaxDistance","-1",path);
    zoom=GameplaySettings::Load(path).thirdPerson;assert(zoom.minimumDistance==1000 && zoom.maximumDistance==1000 && zoom.distance==1000);
    WritePrivateProfileStringA("ThirdPerson","CameraMinDistance","200",path);
    WritePrivateProfileStringA("ThirdPerson","CameraMaxDistance","800",path);
    WritePrivateProfileStringA("ThirdPerson","CameraZoomStep","75",path);
    zoom=GameplaySettings::Load(path).thirdPerson;wheel.Add(-120);assert(wheel.Take(500,zoom)==575);
    // Installation policy loads independently of camera tuning and rejects malformed opt-ins.
    assert(!GameplaySettings::Load(path).warcraftOnly);
    WritePrivateProfileStringA("Installation","WarcraftOnly","true",path);
    assert(GameplaySettings::Load(path).warcraftOnly);
    WritePrivateProfileStringA("Installation","WarcraftOnly","1",path);
    assert(GameplaySettings::Load(path).warcraftOnly);
    WritePrivateProfileStringA("Installation","WarcraftOnly","broken",path);
    assert(!GameplaySettings::Load(path).warcraftOnly);
    DeleteFileA(path);
}
