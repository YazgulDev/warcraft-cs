#include "GameplaySettings.hpp"
#include "../platform/OptionalWeaponAssets.hpp"
#include "../platform/WarcraftApi.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cctype>

GameplaySettings GameplaySettings::Load(const std::string& filename) {
    GameplaySettings result;
    auto text = [&](const char* section,const char* key,const char* fallback) {
        char value[128]; GetPrivateProfileStringA(section,key,fallback,value,sizeof(value),filename.c_str());
        return std::string(value);
    };
    auto number = [&](const char* section,const char* key,float fallback,float maximum) {
        std::string value=text(section,key,""); if (value.empty()) return fallback;
        char* end=nullptr; float parsed=std::strtof(value.c_str(),&end);
        // Reject malformed/NaN values rather than letting invalid config poison ammo, native damage or distances.
        if (end==value.c_str() || *end || !std::isfinite(parsed)) { wc3::Log("Invalid config %s.%s; using default",section,key);return fallback; }
        return std::clamp(parsed,0.0f,maximum);
    };
    // New session controls reload with F8; malformed toggles preserve their documented defaults.
    auto enabled=[&](const char* section,const char* key,bool fallback=true) {
        auto value=text(section,key,fallback ? "true" : "false");
        if(!_stricmp(value.c_str(),"true") || value=="1")return true;
        if(!_stricmp(value.c_str(),"false") || value=="0")return false;
        return fallback; // Malformed values retain the documented default, including opt-in native audio.
    };
    // Visual-only settings share the real startup/F8 snapshot and bounded migration path.
    result.viewmodel.bob=enabled("Viewmodel","BobEnabled");
    result.viewmodel.bobAmount=number("Viewmodel","BobAmount",1,3);
    result.viewmodel.bobFrequency=std::max(.5f,number("Viewmodel","BobFrequency",1.8f,4));
    result.viewmodel.bobResponse=std::max(1.f,number("Viewmodel","BobResponse",12,30));
    result.viewmodel.muzzleFlash=enabled("Viewmodel","MuzzleFlash");
    result.viewmodel.flashMilliseconds=unsigned(std::max(10.f,number("Viewmodel","MuzzleFlashMilliseconds",65,200)));
    result.viewmodel.flashScale=number("Viewmodel","MuzzleFlashScale",1,3);
    // Throw snapshots isolate live projectiles/clouds from later F8 tuning changes.
    auto& grenades=result.grenades;
    grenades.enabled=enabled("Grenades","Enabled");
    grenades.throwSpeed=std::max(50.f,number("Grenades","ThrowSpeed",1125,2000));
    grenades.gravity=std::max(50.f,number("Grenades","Gravity",660,2000));
    // Upgrade only the complete former stock pair, including old configs reloaded directly with F8.
    auto trajectory=text("Grenades","TrajectoryVersion","1");
    if((trajectory=="1" || trajectory.empty()) && grenades.throwSpeed==750 && grenades.gravity==800) {
        grenades.throwSpeed=1125;grenades.gravity=660;
    }
    grenades.lightGravityScale=std::max(.1f,number("Grenades","LightGravityScale",10.f/11,2));
    grenades.bounce=number("Grenades","Bounce",.45f,.9f);
    grenades.inheritedVelocity=number("Grenades","InheritedVelocity",1,2);
    grenades.fuse=std::max(.2f,number("Grenades","FuseSeconds",1.5f,10));
    grenades.releaseDelay=number("Grenades","ReleaseDelay",.25f,1);
    grenades.recovery=std::max(.1f,number("Grenades","RecoverySeconds",.8f,3));
    grenades.heRadius=std::max(32.f,number("HEGrenade","Radius",350,2000));
    grenades.flashRadius=std::max(32.f,number("Flashbang","Radius",1000,3000));
    grenades.flashSeconds=number("Flashbang","BlindSeconds",3,10);
    grenades.flashUnitStunSeconds=number("Flashbang","UnitStunSeconds",2,10);
    grenades.smokeRadius=std::max(32.f,number("SmokeGrenade","Radius",300,1000));
    grenades.smokeSeconds=std::max(1.f,number("SmokeGrenade","DurationSeconds",18,60));
    grenades.smokeOpacity=number("SmokeGrenade","Opacity",.85f,1);
    grenades.maxActive=unsigned(std::max(1.f,number("Grenades","MaxActive",32,128)));
    grenades.worldModelScale=std::max(.25f,number("Grenades","WorldModelScale",1,4));
    grenades.worldSpin=number("Grenades","WorldSpinDegrees",360,1440);
    result.shopInteraction=enabled("Shops","Enabled");
    result.shopInteractionRadius=std::max(100.f,number("Shops","Radius",450,1200));
    result.shotgunPellets=int(std::max(1.f,number("Shotguns","Pellets",8,16)));
    result.shotgunSpreadDegrees=number("Shotguns","SpreadDegrees",5,15);
    // Reload policy is snapshotted when R starts, so F8 cannot award a second magazine mid-reload.
    result.shotgunShellReload=enabled("Shotguns","ShellByShellReload");
    result.shotgunInterruptReload=enabled("Shotguns","InterruptReloadOnFire");
    // Secondary Glock bursts debit real rounds; timing is validated before it reaches the shot scheduler.
    result.glockBurstRounds=int(std::max(1.f,number("Glock","SecondaryBurstRounds",3,5)));
    result.glockBurstInterval=std::max(.025f,number("Glock","SecondaryBurstInterval",.1f,1));
    result.glockSecondaryCooldown=std::max(.1f,number("Glock","SecondaryCooldown",.5f,5));
    // The shared reach upgrades saved short gun ranges without rewriting the player's per-weapon choices.
    result.minimumFirearmRange=number("Combat","MinimumFirearmRange",30000,50000);
    auto& ai=result.armyAI;ai.enabled=enabled("ArmyAI","Enabled");
    ai.intervalMs=unsigned(std::max(500.f,number("ArmyAI","IntervalMs",1500,10000)));
    ai.productionRetrySeconds=unsigned(std::max(30.f,number("ArmyAI","ProductionRetrySeconds",180,3600)));
    ai.workers=int(number("ArmyAI","Workers",6,30));ai.soldiers=int(number("ArmyAI","Soldiers",24,100));
    ai.defenders=int(number("ArmyAI","Defenders",3,100));ai.supplyBuildings=int(number("ArmyAI","SupplyBuildings",6,30));
    ai.baseRadius=std::max(100.f,number("ArmyAI","BaseRadius",900,3000));
    ai.supportDistance=std::max(100.f,number("ArmyAI","SupportDistance",350,1500));
    ai.buildRadius=std::max(192.f,number("ArmyAI","BuildRadius",450,1500));
    ai.resourceRadius=std::max(300.f,number("ArmyAI","ResourceRadius",1600,5000));
    auto rawcode=[&](const char* key) {auto value=text("ArmyAI",key,"");return value.size()==4 && value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")==std::string::npos ? value : std::string();};
    ai.worker=rawcode("Worker");ai.hall=rawcode("Hall");ai.supply=rawcode("Supply");ai.barracks=rawcode("Barracks");ai.soldier=rawcode("Soldier");
    // Missing/malformed profile values preserve the full mod's backwards-compatible default.
    auto profile=text("Installation","WarcraftOnly","false");
    result.warcraftOnly=!_stricmp(profile.c_str(),"true") || profile=="1";
    // The unarmed camera is independently configurable; malformed values preserve enabled entry.
    auto unitView=text("UnitView","Enabled","true");
    result.unitViewEnabled=_stricmp(unitView.c_str(),"false") && unitView!="0";
    // Malformed/missing opt-outs keep the requested clean F9 view; F8 restores immediately when disabled.
    auto hover=text("UnitView","HideHoverCircles","true");
    result.unitViewHideHoverCircles=_stricmp(hover.c_str(),"false") && hover!="0";
    if(_stricmp(hover.c_str(),"true") && _stricmp(hover.c_str(),"false") && hover!="1" && hover!="0")
        wc3::Log("Invalid UnitView.HideHoverCircles; using true");
    // The requested safe unit-view default reloads with F8; explicit false restores native targeting.
    auto preventAttacks=text("UnitView","PreventEnemyAttacks","true");
    result.unitViewPreventEnemyAttacks=_stricmp(preventAttacks.c_str(),"false") && preventAttacks!="0";
    if(_stricmp(preventAttacks.c_str(),"true") && _stricmp(preventAttacks.c_str(),"false") && preventAttacks!="1" && preventAttacks!="0")
        wc3::Log("Invalid UnitView.PreventEnemyAttacks; using true");
    // Third-person camera and Arthas swing tuning reload as part of the existing validated snapshot.
    auto thirdEnabled=text("ThirdPerson","Enabled","true");
    if(!_stricmp(thirdEnabled.c_str(),"false") || thirdEnabled=="0")result.thirdPerson.enabled=false;
    // Bound the wheel and configured starting distance together; reversed limits collapse safely.
    result.thirdPerson.minimumDistance=std::max(80.f,number("ThirdPerson","CameraMinDistance",80,1000));
    result.thirdPerson.maximumDistance=std::max(result.thirdPerson.minimumDistance,number("ThirdPerson","CameraMaxDistance",1000,1000));
    result.thirdPerson.zoomStep=number("ThirdPerson","CameraZoomStep",50,500);
    result.thirdPerson.distance=std::clamp(number("ThirdPerson","CameraDistance",500,1000),result.thirdPerson.minimumDistance,result.thirdPerson.maximumDistance);
    result.thirdPerson.height=number("ThirdPerson","CameraHeight",35,200);
    // Pitch uses a signed range unlike ordinary nonnegative tuning.
    auto pitchText=text("ThirdPerson","InitialPitch","-12");char* pitchEnd=nullptr;
    float pitch=std::strtof(pitchText.c_str(),&pitchEnd);
    if(pitchEnd!=pitchText.c_str() && !*pitchEnd && std::isfinite(pitch))result.thirdPerson.initialPitch=std::clamp(pitch,-60.f,30.f);
    result.thirdPerson.clearance=number("ThirdPerson","CameraClearance",16,64);
    // Opt in only on an explicit true: broad building boxes otherwise cause unwanted automatic zoom.
    auto buildingCollision=text("ThirdPerson","CameraBuildingCollision","false");
    result.thirdPerson.buildingCollision=!_stricmp(buildingCollision.c_str(),"true") || buildingCollision=="1";
    result.thirdPerson.attackRange=std::max(30.f,number("ThirdPerson","AttackRange",150,400));
    result.thirdPerson.attackArc=std::max(1.f,number("ThirdPerson","AttackArc",50,120));
    result.thirdPerson.attackHeight=number("ThirdPerson","AttackHeight",64,200);
    // Ranged targeting remains bounded even with malformed map stats/config; melee sweeps include short creep bodies.
    auto nativeRange=text("ThirdPerson","UseNativeAttackRange","true"),projectiles=text("ThirdPerson","NativeRangedProjectiles","true");
    result.thirdPerson.useNativeAttackRange=_stricmp(nativeRange.c_str(),"false") && nativeRange!="0";
    result.thirdPerson.nativeRangedProjectiles=_stricmp(projectiles.c_str(),"false") && projectiles!="0";
    result.thirdPerson.rangedAttackRange=std::max(30.f,number("ThirdPerson","RangedAttackRange",600,5000));
    result.thirdPerson.rangedRangeMultiplier=std::max(.1f,number("ThirdPerson","RangedRangeMultiplier",1,5));
    result.thirdPerson.meleeHeightTolerance=number("ThirdPerson","MeleeHeightTolerance",32,100);
    result.thirdPerson.attackInterval=std::max(.2f,number("ThirdPerson","AttackInterval",2.2f,5));
    result.thirdPerson.contactDelay=std::min(result.thirdPerson.attackInterval,number("ThirdPerson","ContactDelay",.56f,3));
    result.thirdPerson.damageMultiplier=number("ThirdPerson","DamageMultiplier",1,10);
    // The marker can be hidden without changing contact range, attack targeting or map state.
    auto indicator=text("ThirdPerson","ShowTargetIndicator","true");
    result.thirdPerson.showTargetIndicator=_stricmp(indicator.c_str(),"false") && indicator!="0";
    // Radius uses Warcraft world units and reloads with the other third-person settings.
    result.thirdPerson.targetCircleRadius=std::max(16.f,number("ThirdPerson","TargetCircleRadius",64,256));
    // Native pose playback follows physical motion; unknown toggles keep the requested animation defaults.
    auto motion=text("ThirdPerson","MovementAnimations","true");
    if(!_stricmp(motion.c_str(),"false") || motion=="0")result.thirdPerson.movementAnimations=false;
    result.thirdPerson.runCycleDistance=std::max(40.f,number("ThirdPerson","RunCycleDistance",210,1000));
    result.thirdPerson.takeoffDuration=std::max(.02f,number("ThirdPerson","TakeoffDuration",.10f,.3f));
    result.thirdPerson.landingDuration=number("ThirdPerson","LandingDuration",.16f,.5f);
    // Automatic gait speed is visual only; a custom multiplier never changes native move/attack statistics.
    // Existing manual strides retain their meaning until the player explicitly enables automatic metadata.
    auto autoMotion=text("ThirdPerson","AutoAnimationSpeed",result.thirdPerson.runCycleDistance==210 ? "true" : "false");
    result.thirdPerson.autoAnimationSpeed=_stricmp(autoMotion.c_str(),"false") && autoMotion!="0";
    result.thirdPerson.animationSpeedMultiplier=std::max(.1f,number("ThirdPerson","AnimationSpeedMultiplier",1,3));
    // Unknown values retain the documented default; explicit false/0 bypasses attack-effect dispatch.
    auto attackEffects=text("Combat","EnableAttackEffects","true");
    if(!_stricmp(attackEffects.c_str(),"false") || attackEffects=="0")result.enableAttackEffects=false;
    else if(_stricmp(attackEffects.c_str(),"true") && attackEffects!="1")wc3::Log("Invalid Combat.EnableAttackEffects; using true");
    // Restore legacy object interception only on an explicit opt-out; malformed preferences keep the fix enabled.
    auto transparent=text("Combat","IgnorePassableInvulnerableObjects","true");
    if(!_stricmp(transparent.c_str(),"false") || transparent=="0")result.ignorePassableInvulnerableObjects=false;
    else if(_stricmp(transparent.c_str(),"true") && transparent!="1")wc3::Log("Invalid Combat.IgnorePassableInvulnerableObjects; using true");
    // Only explicit true/1 restores the old water-surface blocker; bad/legacy values keep submerged targets hittable.
    auto waterBlocks=text("Combat","WaterBlocksAttacks","false");
    result.waterBlocksAttacks=!_stricmp(waterBlocks.c_str(),"true") || waterBlocks=="1";
    if(_stricmp(waterBlocks.c_str(),"true") && _stricmp(waterBlocks.c_str(),"false") && waterBlocks!="1" && waterBlocks!="0")
        wc3::Log("Invalid Combat.WaterBlocksAttacks; using false");
    // Bound diagnostic frequency/storage at the existing INI boundary, including malformed legacy values.
    auto detailed = text("Logging", "Detailed", "true");
    if (!_stricmp(detailed.c_str(), "false") || detailed == "0") result.logging.detailed = false;
    else if (_stricmp(detailed.c_str(), "true") && detailed != "1") wc3::Log("Invalid Logging.Detailed; using true");
    result.logging.intervalMs = unsigned(std::max(100.f, number("Logging", "IntervalMs", 1000, 60000)));
    result.logging.maxFileMB = unsigned(std::max(1.f, number("Logging", "MaxFileMB", 8, 64)));
    result.logging.archiveCount = unsigned(number("Logging", "ArchiveCount", 3, 8));
    // Zero opts back into Warcraft's original limiter; positive targets stay within a bounded range.
    float maxFPS=number("Rendering","MaxFPS",144,360);
    result.render.maxFPS=maxFPS==0 ? 0 : int(std::max(30.f,maxFPS));
    // Missing or malformed distances use the same default as a fresh settings snapshot.
    result.render.drawDistance=std::max(3000.f,number("Rendering","DrawDistance",result.render.drawDistance,20000));
    result.runeAmmoPercent=number("Runes","AmmoPercent",result.runeAmmoPercent,100);
    // Missing settings enable the requested hopping; malformed switches preserve documented defaults.
    auto boolean = [&](const char* key, bool fallback) {
        auto value = text("Movement", key, fallback ? "true" : "false");
        if (!_stricmp(value.c_str(), "true")) return true;
        if (!_stricmp(value.c_str(), "false")) return false;
        return fallback;
    };
    result.movement.bunnyHop = boolean("BunnyHop", true);
    result.movement.scaleFromUnitMoveSpeed=boolean("ScaleFromUnitMoveSpeed",false);
    result.movement.unitMoveSpeedReference=std::max(1.f,number("Movement","UnitMoveSpeedReference",300,1000));
    // Fire-rate scaling is opt-in and independent of weapon damage selection.
    auto fireRate=text("Combat","ScaleFireRateFromAttackSpeed","false");
    result.scaleFireRateFromAttackSpeed=!_stricmp(fireRate.c_str(),"true") || fireRate=="1";
    result.movement.autoJump = boolean("AutoJump", true);
    result.movement.jumpBoostPercent = number("Movement", "JumpBoostPercent", 0, 100);
    result.movement.maxBunnySpeed = std::max(250.0f, number("Movement", "MaxBunnySpeed", 1000, 2000));
    result.movement.airAcceleration = number("Movement", "AirAcceleration", 10, 100);
    result.movement.jumpSpeed = std::max(1.0f, number("Movement", "JumpSpeed", 268.328f, 800));
    result.movement.gravity = std::max(100.0f, number("Movement", "Gravity", 800, 3000));
    result.movement.stepHeight = number("Movement", "StepHeight", 27, 64);
    // Keep geometric tuning validated at the same F8 boundary as jump/gravity settings.
    result.movement.maxWalkSlope = std::max(15.f,number("Movement","MaxWalkSlope",46,75));
    result.movement.hullRadius = std::max(8.f,number("Movement","HullRadius",24,64));
    // Live collision may be opted out independently of visual weapon hitboxes and terrain/wall collision.
    result.movement.useNativeUnitCollision = boolean("UseNativeUnitCollision", true);
    // Mesh detail is independent of actor footprints and can be reverted on F8 for custom maps.
    result.movement.objectMeshCollision = boolean("ObjectMeshCollision", true);
    result.runePickupRadius=number("Runes","PickupRadius",160,400);
    result.runeAmmoAllWeapons=_stricmp(text("Runes","AmmoWeapons","all").c_str(),"current")!=0;
    auto damageMode=text("Damage","Mode","unit");
    result.heroDamage=_stricmp(damageMode.c_str(),"weapon")!=0;
    // Experimental hosts may lack the native attack-stat adapter; retain configurable fixed damage as fallback.
    auto unavailableAttack=text("Damage","UnavailableAttackFallback","weapon");
    result.unavailableAttackUsesWeaponDamage=_stricmp(unavailableAttack.c_str(),"zero")!=0;
    if(_stricmp(unavailableAttack.c_str(),"weapon") && _stricmp(unavailableAttack.c_str(),"zero"))
        wc3::Log("Invalid Damage.UnavailableAttackFallback; using weapon");
    if(_stricmp(damageMode.c_str(),"unit") && _stricmp(damageMode.c_str(),"hero") && _stricmp(damageMode.c_str(),"weapon"))
        wc3::Log("Invalid Damage.Mode; using unit");
    // Missing or malformed settings must not silently enable instant kills; explicit preferences still reload on F8.
    auto awpOneShot=text("Damage","AWPOneShot","false");
    result.awpOneShot=!_stricmp(awpOneShot.c_str(),"true") || awpOneShot=="1";
    if (!awpOneShot.empty() && _stricmp(awpOneShot.c_str(),"true") && _stricmp(awpOneShot.c_str(),"false") &&
        awpOneShot!="0" && awpOneShot!="1") wc3::Log("Invalid Damage.AWPOneShot; using false");
    // Old configs retain 50%; zero disables allied damage and full damage is capped at 100%.
    result.friendlyFirePercent=number("Damage","FriendlyFirePercent",50,100);
    // Protection is the safe startup default; malformed values cannot silently enable allied contacts.
    auto allies=text("Combat","TargetAllies","false");
    result.targetAllies=!_stricmp(allies.c_str(),"true") || allies=="1";
    if(_stricmp(allies.c_str(),"true") && _stricmp(allies.c_str(),"false") && allies!="0" && allies!="1")
        wc3::Log("Invalid Combat.TargetAllies; using false");
    result.floatingTextDistance=number("Interface","FloatingTextDistance",1200,5000);
    // Team bars require an explicit opt-in; invalid/legacy preferences keep the FPS view clear.
    auto healthSwitch=[&](const char* key) {
        auto value=text("Interface",key,"false");
        if(!_stricmp(value.c_str(),"true") || value=="1") return true;
        if(!value.empty() && _stricmp(value.c_str(),"false") && value!="0") wc3::Log("Invalid Interface.%s; using false",key);
        return false;
    };
    result.healthBars.allies=healthSwitch("ShowAllyHealthBars");
    result.healthBars.enemies=healthSwitch("ShowEnemyHealthBars");
    result.healthBars.distance=number("Interface","HealthBarDistance",2000,15000);
    // Apply an entire keyboard layout together; a conflicting layout retains usable default controls.
    bool layoutValid=true;
    for(size_t i=0;i<ShortcutBindings::Count;++i) {
        auto action=ShortcutAction(i);auto fallback=ShortcutBindings::Label(result.shortcuts.Key(action));
        int key=ShortcutBindings::Parse(text("Shortcuts",ShortcutBindings::Name(action),fallback.c_str()));
        if(key<0)layoutValid=false;else result.shortcuts.Set(action,key);
    }
    // Legacy installs used F7 for refill. Relocate that old default without discarding unrelated custom keys.
    if(text("Shortcuts","ToggleThirdPerson","").empty() && result.shortcuts.Key(ShortcutAction::Refill)==VK_F7) {
        int replacement=0;
        for(int candidate:{VK_F11,VK_F12,VK_F5,VK_F4}) {
            bool used=false;for(size_t i=0;i<ShortcutBindings::Count;++i)used|=result.shortcuts.Key(ShortcutAction(i))==candidate;
            if(!used){replacement=candidate;break;}
        }
        result.shortcuts.Set(ShortcutAction::Refill,replacement);
    }
    // An older custom Q binding wins; adding a new action must not reset the player's existing layout.
    if(text("Shortcuts","ToggleAllyTargeting","").empty())for(size_t i=0;i<size_t(ShortcutAction::ToggleAllyTargeting);++i)
        if(result.shortcuts.Key(ShortcutAction(i))=='Q')result.shortcuts.Set(ShortcutAction::ToggleAllyTargeting,0);
    // Free F9's former all-weapons default without resetting unrelated custom layouts.
    if(text("Shortcuts","ToggleUnitView","").empty()) {
        if(result.shortcuts.Key(ShortcutAction::AllWeapons)==VK_F9) {
            int replacement=0;
            for(int candidate:{VK_F12,VK_F5,VK_F4}) {
                bool used=false;for(size_t i=0;i<size_t(ShortcutAction::ToggleUnitView);++i)
                    if(i!=size_t(ShortcutAction::AllWeapons))used|=result.shortcuts.Key(ShortcutAction(i))==candidate;
                if(!used){replacement=candidate;break;}
            }
            result.shortcuts.Set(ShortcutAction::AllWeapons,replacement);
        }
        for(size_t i=0;i<size_t(ShortcutAction::ToggleUnitView);++i)
            if(result.shortcuts.Key(ShortcutAction(i))==VK_F9)result.shortcuts.Set(ShortcutAction::ToggleUnitView,0);
    }
    // Partial legacy layouts may explicitly reserve F12 for refill while omitting AllWeapons.
    if(text("Shortcuts","AllWeapons","").empty()) {
        for(int candidate:{VK_F12,VK_F5,VK_F4,0}) {
            bool used=false;for(size_t i=0;i<ShortcutBindings::Count;++i)
                if(i!=size_t(ShortcutAction::AllWeapons) && candidate)used|=result.shortcuts.Key(ShortcutAction(i))==candidate;
            if(!used){result.shortcuts.Set(ShortcutAction::AllWeapons,candidate);break;}
        }
    }
    // Upgrade the former squad default without stealing Y from another saved action.
    bool oldSquadKey=result.shortcuts.Key(ShortcutAction::SquadOrders)==VK_OEM_1;
    if(oldSquadKey)result.shortcuts.Set(ShortcutAction::SquadOrders,'Y');
    // New Tab/I/Y defaults yield to an older customized action instead of resetting the whole layout.
    for(auto action:{ShortcutAction::InventoryNumbers,ShortcutAction::InventoryPanel,ShortcutAction::SquadOrders,ShortcutAction::ArmyAI}) {
        if(!text("Shortcuts",ShortcutBindings::Name(action),"").empty() && !(action==ShortcutAction::SquadOrders && oldSquadKey))continue;
        for(size_t i=0;i<size_t(action);++i)if(result.shortcuts.Key(ShortcutAction(i))==result.shortcuts.Key(action)) {
            result.shortcuts.Set(action,0);break;
        }
    }
    if(!layoutValid || !result.shortcuts.Valid()) {result.shortcuts=ShortcutBindings();wc3::Log("Invalid/conflicting Shortcuts; using default keyboard layout");}
    // Rawcodes and order IDs describe existing abilities; they never grant skills or bypass native costs.
    auto spellsEnabled=text("Spells","Enabled","true");
    result.spells.enabled=!_stricmp(spellsEnabled.c_str(),"true") || spellsEnabled=="1";
    if(_stricmp(spellsEnabled.c_str(),"true") && _stricmp(spellsEnabled.c_str(),"false") && spellsEnabled!="1" && spellsEnabled!="0") {
        result.spells.enabled=true;wc3::Log("Invalid Spells.Enabled; using true");
    }
    result.spells.aimDistance=std::max(100.f,number("Spells","AimDistance",3000,15000));
    for(int slot=0;slot<4;++slot) {
        std::string prefix="Slot"+std::to_string(slot+1);
        auto raw=text("Spells",(prefix+"Ability").c_str(),"");
        if(raw.size()==4 && raw.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")==std::string::npos)result.spells.ability[slot]=raw;
        else if(!raw.empty())wc3::Log("Invalid Spells.%sAbility; using native hero slot",prefix.c_str());
        // An order ID is discrete: reject overflow/fractions instead of clamping to another native command.
        auto orderText=text("Spells",(prefix+"Order").c_str(),"0");char* orderEnd=nullptr;
        double order=std::strtod(orderText.c_str(),&orderEnd);
        if(orderEnd!=orderText.c_str() && !*orderEnd && std::isfinite(order) && order==std::floor(order) &&
            (order==0 || (order>=852000 && order<=853999)))result.spells.order[slot]=int(order);
        else wc3::Log("Invalid Spells.%sOrder; using native order",prefix.c_str());
        auto mode=text("Spells",(prefix+"Target").c_str(),"auto");
        for(auto& c:mode)c=char(std::tolower(static_cast<unsigned char>(c)));
        if(mode=="auto" || mode=="unit" || mode=="point" || mode=="immediate" || mode=="self")result.spells.target[slot]=mode;
        else wc3::Log("Invalid Spells.%sTarget; using auto",prefix.c_str());
    }
    // Legacy/invalid configs show the native map; explicit false/0 hides it on startup and F8.
    auto showMinimap=text("Interface","ShowMinimap","true");
    if (!_stricmp(showMinimap.c_str(),"false") || showMinimap=="0") result.showMinimap=false;
    else if (_stricmp(showMinimap.c_str(),"true") && showMinimap!="1") wc3::Log("Invalid Interface.ShowMinimap; using true");
    result.minimapSizePercent=std::max(10.f,number("Interface","MinimapSizePercent",22,40));
    // Optional spell HUD settings reload with F8; malformed preferences retain its requested default visibility.
    auto enabledHud=[&](const char* key) {
        auto value=text("Interface",key,"true");
        if(!_stricmp(value.c_str(),"false") || value=="0")return false;
        if(_stricmp(value.c_str(),"true") && value!="1")wc3::Log("Invalid Interface.%s; using true",key);
        return true;
    };
    result.showSpellIcons=enabledHud("ShowSpellIcons");result.showSpellCooldowns=enabledHud("ShowSpellCooldowns");
    // These HUD panels reload with F8; the separate clean F9 view always omits them.
    result.showUnitHud=enabledHud("ShowUnitHud");result.showInventory=enabledHud("ShowInventory");
    result.inventoryControls=enabledHud("InventoryControls");
    // Missing/malformed menu preferences keep CS-style preview; F8 replaces the complete snapshot.
    result.weaponSelectionMenu=enabled("WeaponSelection","Enabled");
    result.weaponSelectionTimeout=std::max(.5f,number("WeaponSelection","TimeoutSeconds",5,15));
    result.inventoryDropDistance=std::max(16.f,number("Inventory","DropDistance",80,128));
    // Startup/F8 default to item numbers; an explicit false/0 preserves a weapon-first preference.
    auto itemNumbers=text("Inventory","NumbersByDefault","true");
    result.inventoryNumbersDefault=_stricmp(itemNumbers.c_str(),"false") && itemNumbers!="0";
    result.unitHudScalePercent=std::max(60.f,number("Interface","UnitHudScalePercent",100,160));
    result.spellIconSizePercent=std::max(4.f,number("Interface","SpellIconSizePercent",7,14));
    // Keep the optional counter hidden for old/invalid configs; accept explicit boolean preferences.
    auto showSpeed=text("Interface","ShowSpeed","false");
    result.showSpeed=!_stricmp(showSpeed.c_str(),"true") || showSpeed=="1";
    if (!showSpeed.empty() && _stricmp(showSpeed.c_str(),"true") && _stricmp(showSpeed.c_str(),"false") &&
        showSpeed!="0" && showSpeed!="1") wc3::Log("Invalid Interface.ShowSpeed; using false");
    // Only an explicit opt-in reveals unexplored areas; legacy/malformed settings retain map fog.
    auto disableFog=text("Interface","DisableFogOfWar","false");
    result.disableFogOfWar=!_stricmp(disableFog.c_str(),"true") || disableFog=="1";
    if (!disableFog.empty() && _stricmp(disableFog.c_str(),"true") && _stricmp(disableFog.c_str(),"false") &&
        disableFog!="0" && disableFog!="1") wc3::Log("Invalid Interface.DisableFogOfWar; using false");
    // Legacy files keep their CS volume; malformed values cannot enter the native audio mixer.
    result.csVolumePercent=number("Audio","CSVolumePercent",100,100);
    // Keep formation spacing positive and leash beyond it so units can finish nearby fights.
    result.squadRadius=number("Squad","RecruitRadius",600,2000);
    result.squadFollowDistance=std::max(80.0f,number("Squad","FollowDistance",180,500));
    result.squadLeash=std::max(result.squadFollowDistance+200,number("Squad","CombatLeash",900,3000));
    result.squadMaxUnits=int(std::max(1.0f,number("Squad","MaxUnits",24,64)));
    // Rear spacing and path yielding reload with F8 without releasing or teleporting the squad.
    auto squadSwitch=[&](const char* key) {
        auto value=text("Squad",key,"true");
        if(!_stricmp(value.c_str(),"false") || value=="0")return false;
        if(_stricmp(value.c_str(),"true") && value!="1")wc3::Log("Invalid Squad.%s; using true",key);
        return true;
    };
    result.squadRearFormation=squadSwitch("RearFormation");
    result.squadYield=squadSwitch("YieldToPlayer");
    result.squadPreserveCombatOrders=squadSwitch("PreserveCombatOrders");
    result.squadAutomaticSpells=squadSwitch("AutomaticSpells");
    result.squadReticleAttack=squadSwitch("ReticleAttack");
    // Startup/F8 restore the initial F6 mode; the configured Y shortcut toggles it for this map session.
    auto fpsOrders=text("Squad","ReticleAttackInFPS","false");
    result.squadReticleAttackInFPS=!_stricmp(fpsOrders.c_str(),"true") || fpsOrders=="1";
    result.squadReticleDistance=std::max(100.f,number("Squad","ReticleAttackDistance",3000,15000));
    result.squadSpellRadius=std::max(50.f,number("Squad","SpellSearchRadius",700,2000));
    result.squadAreaSpellRadius=std::max(16.f,number("Squad","AreaSpellRadius",250,1000));
    result.squadHealBelowPercent=std::max(1.f,number("Squad","HealBelowPercent",85,100));
    result.squadBuffRefreshSeconds=std::max(1.f,number("Squad","BuffRefreshSeconds",20,600));
    result.squadSpellIntervalMs=unsigned(std::max(100.f,number("Squad","SpellIntervalMs",1000,10000)));
    result.squadClearance=std::max(60.f,number("Squad","PlayerClearance",100,300));
    result.squadYieldAhead=std::max(result.squadClearance,number("Squad","YieldAheadDistance",350,1000));
    result.squadOrderIntervalMs=unsigned(std::max(50.f,number("Squad","OrderIntervalMs",150,1000)));
    // Dollars are isolated from campaign resources; malformed settings retain the documented defaults.
    auto currency=text("Economy","Currency","dollars");
    if(!_stricmp(currency.c_str(),"gold"))result.economy.currency=EconomySettings::Currency::Gold;
    else if(_stricmp(currency.c_str(),"dollars"))wc3::Log("Invalid Economy.Currency; using dollars");
    result.economy.maximumMoney=int(number("Economy","MaximumMoney",16000,1000000));
    result.economy.startingMoney=std::min(result.economy.maximumMoney,int(number("Economy","StartingMoney",800,1000000)));
    result.economy.killReward=int(number("Economy","KillReward",300,1000000));
    result.weaponDrops.enabled=enabled("WeaponDrops","Enabled");
    result.weaponDrops.chancePercent=number("WeaponDrops","ChancePercent",.5f,100);
    result.weaponDrops.lifetimeSeconds=std::max(1.f,number("WeaponDrops","LifetimeSeconds",120,3600));
    result.weaponDrops.pickupRadius=std::max(16.f,number("WeaponDrops","PickupRadius",160,400));
    result.weaponDrops.maximumDrops=unsigned(std::max(1.f,number("WeaponDrops","MaximumDrops",64,256)));
    // Restricted modes govern both opening B and every transaction, including the quick-ammo key.
    auto access=text("Buy","Access","anywhere");
    if (!_stricmp(access.c_str(),"friendly")) result.buyAccess=BuyAccess::FriendlyBuildings;
    else if (!_stricmp(access.c_str(),"shops")) result.buyAccess=BuyAccess::Shops;
    else if (_stricmp(access.c_str(),"anywhere")) { result.buyAccess=BuyAccess::Shops;wc3::Log("Invalid Buy.Access; using shops"); }
    result.buyRadius=number("Buy","Radius",600,3000);
    result.shopTypes=text("Buy","ShopTypes","");
    result.startAllWeapons=!_stricmp(text("Loadout","Mode","melee").c_str(),"all");
    result.maxBombs=int(number("Loadout","MaxBombs",100,1000));
    // Missing/invalid starting counts retain the bomb-free default; explicit counts remain configurable.
    result.startBombs=std::min(result.maxBombs,int(number("Loadout","BombCount",0,1000)));
    // Unlimited purchases are the default; F8 can restore MaxBombs as a paid-purchase cap.
    auto unlimitedBombs=text("Loadout","UnlimitedBombPurchases","true");
    if(!_stricmp(unlimitedBombs.c_str(),"false") || unlimitedBombs=="0")result.unlimitedBombPurchases=false;
    else if(_stricmp(unlimitedBombs.c_str(),"true") && unlimitedBombs!="1")wc3::Log("Invalid Loadout.UnlimitedBombPurchases; using true");
    result.csSky=_stricmp(text("Sky","Enabled","false").c_str(),"false")!=0;
    // Default to Warcraft's sky even for older configs missing this key; explicit CS sky still takes precedence.
    result.nativeSky=_stricmp(text("Sky","WarcraftEnabled","true").c_str(),"true")==0;
    // Only filename stems from private CS caches are accepted; INI strings never become arbitrary paths.
    auto sky=[&](const char* key,const char* fallback) {
        auto value=text("Sky",key,fallback);
        if (value.empty() || value.size()>48 || value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")!=std::string::npos)
            return std::string(fallback);
        return value;
    };
    result.defaultSky=sky("Default","Des");
    const char* tilesets="ALFWBYXVQJNDCIGOZ";
    for (const char* code=tilesets;*code;++code) {
        char key[]={*code,0};
        const char* fallback=strchr("NWIC",*code) ? "snow" : strchr("OD",*code) ? "DrkG" : strchr("AZJ",*code) ? "forest" : "Des";
        result.tilesetSky[static_cast<unsigned char>(*code)]=sky(key,fallback);
    }
    // Optional RPG defaults use the same bounded F8 snapshot, while availability comes only from owned caches.
    auto& rpg=result.rpg;rpg.enabled=enabled("RPG","Enabled");rpg.guidance=enabled("RPG","GuidanceByDefault");
    rpg.available=!result.warcraftOnly && OptionalWeaponAssets::Rpg(std::filesystem::path(filename).parent_path());
    rpg.radius=std::max(32.f,number("RPG","Radius",525,3000));
    // Upgrade only the former stock blast, including an old config loaded directly with F8.
    auto blastVersion=text("RPG","BlastVersion","1");
    if((blastVersion=="1" || blastVersion.empty()) && rpg.radius==375)rpg.radius=525;
    rpg.launchSpeed=std::max(50.f,number("RPG","LaunchSpeed",375,6000));
    rpg.maximumSpeed=std::max(100.f,number("RPG","MaximumSpeed",3000,10000));
    rpg.acceleration=number("RPG","Acceleration",6000,20000);
    rpg.ignition=std::max(.01f,number("RPG","IgnitionSeconds",.4f,3));
    rpg.boostSeconds=std::max(.1f,number("RPG","BoostSeconds",1,5));
    rpg.gravity=number("RPG","LaunchGravity",600,2000);
    rpg.lifetime=std::max(1.f,number("RPG","LifetimeSeconds",12,30));
    rpg.inheritedVelocity=number("RPG","InheritedVelocity",1,2);
    rpg.modelScale=std::max(.25f,number("RPG","WorldModelScale",1.5f,4));
    rpg.trailSeconds=number("RPG","TrailSeconds",1,3);rpg.trailSize=number("RPG","TrailSize",10,40);
    rpg.maxActive=unsigned(std::max(1.f,number("RPG","MaxActive",16,64)));
    // F8 validates each owned Half-Life weapon independently and replaces one complete tuning snapshot.
    auto& hl=result.halfLife;
    for(int i=0;i<3;++i) {
        hl.enabled[i]=enabled(WeaponSlots::Sections[WeaponSlots::Crowbar+i],"Enabled");
        hl.available[i]=!result.warcraftOnly && OptionalWeaponAssets::HalfLife(std::filesystem::path(filename).parent_path(),i);
    }
    hl.tauChargeSeconds=std::max(.1f,number("TauCannon","ChargeSeconds",4,10));
    // The same bounded charge clock drives a continuous spinner phase and live loop pitch; equal rates disable the ramp.
    hl.tauSpinStartRate=std::max(.25f,number("TauCannon","SpinStartRate",1,8));
    hl.tauSpinEndRate=std::max(hl.tauSpinStartRate,number("TauCannon","SpinEndRate",3,8));
    hl.tauPitchStart=std::max(.25f,number("TauCannon","ChargePitchStart",1,4));
    hl.tauPitchEnd=std::max(hl.tauPitchStart,number("TauCannon","ChargePitchEnd",2.5f,4));
    hl.tauDrainSeconds=std::max(.05f,number("TauCannon","ChargeDrainSeconds",.3f,2));
    hl.tauChargedDamage=number("TauCannon","ChargedDamage",1000,1000000);
    hl.tauPrimaryCost=int(std::max(1.f,number("TauCannon","PrimaryCost",2,100)));
    hl.crowbarContactSeconds=number("Crowbar","ContactSeconds",.1f,1);
    hl.crowbarSurfaceSounds=enabled("Crowbar","SurfaceSounds");
    hl.beamSeconds=std::max(.025f,number("HalfLife","BeamSeconds",.15f,1));
    hl.beamWidth=std::max(.5f,number("HalfLife","BeamWidth",3,20));
    // Optional art and impact intensity reload independently of beam cadence and cell consumption.
    hl.beamEffects=enabled("HalfLife","BeamEffects");hl.ownedSprites=enabled("HalfLife","OwnedSprites");
    hl.impactSize=number("HalfLife","ImpactSize",18,100);
    result.nativeAttackSound=enabled("Combat","NativeAttackSound",false);
    const auto& sections=WeaponSlots::Sections;
    for (int slot=0;slot<WeaponSlots::Count;++slot) {
        auto& weapon=result.weapons[slot];
        // Reloading can tighten capacities and change cadence without granting ammunition.
        if(WeaponSlots::Firearm(slot)) {
            weapon.magazine=int(std::max(1.f,number(sections[slot],"Magazine",float(weapon.magazine),1000)));
            weapon.interval=std::max(.025f,number(sections[slot],"FireInterval",weapon.interval,10));
            weapon.reload=std::max(.1f,number(sections[slot],"ReloadSeconds",weapon.reload,30));
            weapon.range=std::max(100.f,number(sections[slot],"Range",weapon.range,50000));
            result.weaponKick[slot]=number(sections[slot],"Kick",result.weaponKick[slot],10);
        }
        if(WeaponSlots::Grenade(slot))weapon.magazine=int(std::max(1.f,number(sections[slot],"Capacity",float(weapon.magazine),100)));
        result.weaponMoveSpeed[slot]=std::max(50.f,number(sections[slot],"MoveSpeed",result.weaponMoveSpeed[slot],500));
        result.weaponModelScale[slot]=std::max(.25f,number(sections[slot],"ModelScale",result.weaponModelScale[slot],2));
        result.damage[slot]=number(sections[slot],"Damage",result.damage[slot],1000000);
        result.heroMultiplier[slot]=number(sections[slot],"HeroMultiplier",result.heroMultiplier[slot],1000);
        result.weaponPrice[slot]=int(number(sections[slot],"Price",float(result.weaponPrice[slot]),1000000));
        result.ammoPrice[slot]=int(number(sections[slot],"AmmoPrice",float(result.ammoPrice[slot]),1000000));
        result.ammoPack[slot]=int(std::max(1.0f,number(sections[slot],"AmmoPack",float(std::max(1,result.ammoPack[slot])),1000)));
        if(WeaponSlots::Firearm(slot))result.maxReserve[slot]=int(number(sections[slot],"MaxReserve",float(result.maxReserve[slot]),100000));
        if(WeaponSlots::Energy(slot))result.maxReserve[slot]=0; // Cell capacity lives in Magazine; no hidden reload reserve.
        if(slot==WeaponSlots::Crowbar) {
            weapon.range=std::max(32.f,number(sections[slot],"Range",150,1000));
            weapon.interval=std::max(.1f,number(sections[slot],"FireInterval",.5f,5));
        }
    }
    result.knifeSecondaryDamage=number("Knife","SecondaryDamage",65,1000000);
    result.swordSecondaryDamage=number("Sword","SecondaryDamage",240,1000000);
    return result;
}
float GameplaySettings::ContactDamage(int slot,bool secondary,float heroAttack) const {
    if (slot<0 || slot>=WeaponSlots::Count) return 0;
    // Native average attack works for heroes and creeps, including their active bonuses; heavy melee keeps its ratio.
    float heavy=secondary ? (slot==WeaponSlots::Knife ? 65.0f/40 : slot==WeaponSlots::Sword ? 2.0f : 1.0f) : 1.0f;
    if (heroDamage && slot!=WeaponSlots::C4 && slot!=WeaponSlots::RPG && !WeaponSlots::Energy(slot) && !WeaponSlots::Grenade(slot)) {
        // Zero is a real unarmed unit; only the negative unavailable sentinel permits configured fallback damage.
        if(heroAttack>=0 || !unavailableAttackUsesWeaponDamage)return std::max(0.0f,heroAttack)*heroMultiplier[slot]*heavy;
    }
    return secondary && slot==WeaponSlots::Knife ? knifeSecondaryDamage :
        secondary && slot==WeaponSlots::Sword ? swordSecondaryDamage : damage[slot];
}
