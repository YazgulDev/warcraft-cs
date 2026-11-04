// Authored regression only: paired waygates and initialized creatures, without a campaign save.
globals
    unit udg_Actor = null
endglobals
function HeavyDeath takes nothing returns nothing
    call SetUnitUserData(udg_Actor,GetUnitUserData(udg_Actor)+1)
endfunction
function ScriptPortal takes nothing returns nothing
    if GetTriggerUnit()==udg_Actor then
        call SetUnitPosition(udg_Actor,3500,0)
    endif
endfunction
function main takes nothing returns nothing
    local unit first
    local unit second
    local unit target
    local trigger death
    local trigger scripted
    local region area
    call SetCameraBounds(-7808,-7808,7808,7808,-7808,7808,7808,-7808)
    call SetDayNightModels("Environment\\DNC\\DNCLordaeron\\DNCLordaeronTerrain\\DNCLordaeronTerrain.mdl","Environment\\DNC\\DNCLordaeron\\DNCLordaeronUnit\\DNCLordaeronUnit.mdl")
    call InitBlizzard()
    call FogEnable(false)
    call FogMaskEnable(false)
    call SetFloatGameState(GAME_STATE_TIME_OF_DAY,12)
    set udg_Actor=CreateUnit(Player(0),'hfoo',-600,0,0)
    call SetUnitInvulnerable(udg_Actor,true)
    call SelectUnit(udg_Actor,true)
    set first=CreateUnit(Player(15),'nwgt',0,0,0)
    set second=CreateUnit(Player(15),'nwgt',1600,0,180)
    call SetUnitInvulnerable(first,true)
    call SetUnitInvulnerable(second,true)
    call WaygateSetDestination(first,1600,0)
    call WaygateSetDestination(second,0,0)
    call WaygateActivate(first,true)
    call WaygateActivate(second,true)
    set target=CreateUnit(Player(1),'hfoo',2400,0,180)
    call PauseUnit(target,true)
    set target=CreateUnit(Player(1),'ucry',2600,400,180)
    call PauseUnit(target,true)
    set target=CreateUnit(Player(1),'nmyr',2600,-400,180)
    call PauseUnit(target,true)
    set target=CreateUnit(Player(1),'hrif',2800,800,180)
    call PauseUnit(target,true)
    set death=CreateTrigger()
    call TriggerRegisterPlayerUnitEvent(death,Player(1),EVENT_PLAYER_UNIT_DEATH,null)
    call TriggerAddAction(death,function HeavyDeath)
    // A native region transition tests camera/physics rebasing independently of waygate queries.
    set area=CreateRegion()
    call RegionAddRect(area,Rect(-2300,-64,-2100,64))
    set scripted=CreateTrigger()
    call TriggerRegisterEnterRegion(scripted,area,null)
    call TriggerAddAction(scripted,function ScriptPortal)
endfunction
function config takes nothing returns nothing
    call SetMapName("Portal and heavy weapon regression")
    call SetMapDescription("Disposable native waygates and artillery-death fixture")
    call SetPlayers(1)
    call SetTeams(1)
    call DefineStartLocation(0,-600,0)
    call SetPlayerStartLocation(Player(0),0)
    call SetPlayerTeam(Player(0),0)
    call SetPlayerRacePreference(Player(0),RACE_PREF_HUMAN)
    call SetPlayerRaceSelectable(Player(0),false)
    call SetPlayerController(Player(0),MAP_CONTROL_USER)
endfunction
