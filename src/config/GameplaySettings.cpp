#include "GameplaySettings.hpp"
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
    // Third-person camera and Arthas swing tuning reload as part of the existing validated snapshot.
    auto thirdEnabled=text("ThirdPerson","Enabled","true");
    if(!_stricmp(thirdEnabled.c_str(),"false") || thirdEnabled=="0")result.thirdPerson.enabled=false;
    result.thirdPerson.distance=std::max(80.f,number("ThirdPerson","CameraDistance",500,1000));
    result.thirdPerson.height=number("ThirdPerson","CameraHeight",35,200);
    // Pitch uses a signed range unlike ordinary nonnegative tuning.
    auto pitchText=text("ThirdPerson","InitialPitch","-12");char* pitchEnd=nullptr;
    float pitch=std::strtof(pitchText.c_str(),&pitchEnd);
    if(pitchEnd!=pitchText.c_str() && !*pitchEnd && std::isfinite(pitch))result.thirdPerson.initialPitch=std::clamp(pitch,-60.f,30.f);
    result.thirdPerson.clearance=number("ThirdPerson","CameraClearance",16,64);
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
    // Live collision may be opted out independently of visual weapon hitboxes and terrain/wall collision.
    result.movement.useNativeUnitCollision = boolean("UseNativeUnitCollision", true);
    result.runePickupRadius=number("Runes","PickupRadius",160,400);
    result.runeAmmoAllWeapons=_stricmp(text("Runes","AmmoWeapons","all").c_str(),"current")!=0;
    auto damageMode=text("Damage","Mode","unit");
    result.heroDamage=_stricmp(damageMode.c_str(),"weapon")!=0;
    if(_stricmp(damageMode.c_str(),"unit") && _stricmp(damageMode.c_str(),"hero") && _stricmp(damageMode.c_str(),"weapon"))
        wc3::Log("Invalid Damage.Mode; using unit");
    // Missing or malformed settings must not silently enable instant kills; explicit preferences still reload on F8.
    auto awpOneShot=text("Damage","AWPOneShot","false");
    result.awpOneShot=!_stricmp(awpOneShot.c_str(),"true") || awpOneShot=="1";
    if (!awpOneShot.empty() && _stricmp(awpOneShot.c_str(),"true") && _stricmp(awpOneShot.c_str(),"false") &&
        awpOneShot!="0" && awpOneShot!="1") wc3::Log("Invalid Damage.AWPOneShot; using false");
    // Old configs retain 50%; zero disables allied damage and full damage is capped at 100%.
    result.friendlyFirePercent=number("Damage","FriendlyFirePercent",50,100);
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
    result.squadClearance=std::max(60.f,number("Squad","PlayerClearance",100,300));
    result.squadYieldAhead=std::max(result.squadClearance,number("Squad","YieldAheadDistance",350,1000));
    result.squadOrderIntervalMs=unsigned(std::max(50.f,number("Squad","OrderIntervalMs",150,1000)));
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
    const char* sections[]={"AK47","M4A1","USP","AWP","Knife","C4","Sword"};
    for (int slot=0;slot<WeaponSlots::Count;++slot) {
        result.damage[slot]=number(sections[slot],"Damage",result.damage[slot],1000000);
        result.heroMultiplier[slot]=number(sections[slot],"HeroMultiplier",result.heroMultiplier[slot],1000);
        result.weaponPrice[slot]=int(number(sections[slot],"Price",float(result.weaponPrice[slot]),1000000));
        result.ammoPrice[slot]=int(number(sections[slot],"AmmoPrice",float(result.ammoPrice[slot]),1000000));
        result.ammoPack[slot]=int(std::max(1.0f,number(sections[slot],"AmmoPack",float(std::max(1,result.ammoPack[slot])),1000)));
        if(slot<4)result.maxReserve[slot]=int(number(sections[slot],"MaxReserve",float(result.maxReserve[slot]),100000));
    }
    result.knifeSecondaryDamage=number("Knife","SecondaryDamage",65,1000000);
    result.swordSecondaryDamage=number("Sword","SecondaryDamage",240,1000000);
    return result;
}
float GameplaySettings::ContactDamage(int slot,bool secondary,float heroAttack) const {
    if (slot<0 || slot>=WeaponSlots::Count) return 0;
    // Native average attack works for heroes and creeps, including their active bonuses; heavy melee keeps its ratio.
    float heavy=secondary ? (slot==WeaponSlots::Knife ? 65.0f/40 : slot==WeaponSlots::Sword ? 2.0f : 1.0f) : 1.0f;
    if (heroDamage && slot!=WeaponSlots::C4) return std::max(0.0f,heroAttack)*heroMultiplier[slot]*heavy;
    return secondary && slot==WeaponSlots::Knife ? knifeSecondaryDamage :
        secondary && slot==WeaponSlots::Sword ? swordSecondaryDamage : damage[slot];
}
