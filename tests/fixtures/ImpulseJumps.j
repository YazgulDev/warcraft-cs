// Authored offline weapon-impulse arena; the owner's ordinary game resources supply its footman.
globals
    unit runner=null
endglobals
function ImpulseChat takes nothing returns nothing
    local string command=GetEventPlayerChatString()
    if command == "-immune" then
        call SetUnitInvulnerable(runner,true)
    elseif command == "-vulnerable" then
        call SetUnitInvulnerable(runner,false)
    elseif command == "-restore" then
        call SetUnitState(runner,UNIT_STATE_LIFE,GetUnitState(runner,UNIT_STATE_MAX_LIFE))
    // Evaluate on the game thread: host-side sampled coordinates become stale during fast flight.
    elseif command == "-nudge" then
        call SetUnitX(runner,GetUnitX(runner)+80.)
    elseif command == "-warp" then
        call SetUnitX(runner,GetUnitX(runner)+2000.)
    endif
endfunction
function main takes nothing returns nothing
    local trigger chat=CreateTrigger()
    call SetCameraBounds(-7808.,-7808.,7808.,7808.,-7808.,7808.,7808.,-7808.)
    call SetDayNightModels("Environment\\DNC\\DNCLordaeron\\DNCLordaeronTerrain\\DNCLordaeronTerrain.mdl","Environment\\DNC\\DNCLordaeron\\DNCLordaeronUnit\\DNCLordaeronUnit.mdl")
    call FogEnable(false)
    call FogMaskEnable(false)
    call SetFloatGameState(GAME_STATE_TIME_OF_DAY,12.)
    call SetTimeOfDayScale(0.)
    set runner=CreateUnit(Player(0),'hfoo',0.,0.,0.)
    call SetUnitAcquireRange(runner,0.)
    call ClearSelection()
    call SelectUnit(runner,true)
    call PanCameraTo(0.,0.)
    call TriggerRegisterPlayerChatEvent(chat,Player(0),"-",false)
    call TriggerAddAction(chat,function ImpulseChat)
    set chat=null
endfunction
function config takes nothing returns nothing
    call SetMapName("Weapon impulse regression")
    call SetMapDescription("Original isolated flat arena for RPG and Tau movement")
    call SetPlayers(1)
    call SetTeams(1)
    call DefineStartLocation(0,0.,0.)
    call SetPlayerStartLocation(Player(0),0)
    call SetPlayerController(Player(0),MAP_CONTROL_USER)
    call SetPlayerRacePreference(Player(0),RACE_PREF_HUMAN)
    call SetPlayerRaceSelectable(Player(0),false)
endfunction
