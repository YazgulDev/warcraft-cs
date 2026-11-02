// Authored offline crest trial; only terrain support and one initialized, selected actor.
function main takes nothing returns nothing
    local unit actor
    call SetCameraBounds(-7808,-7808,7808,7808,-7808,7808,7808,-7808)
    call SetDayNightModels("Environment\\DNC\\DNCLordaeron\\DNCLordaeronTerrain\\DNCLordaeronTerrain.mdl","Environment\\DNC\\DNCLordaeron\\DNCLordaeronUnit\\DNCLordaeronUnit.mdl")
    call InitBlizzard()
    call FogEnable(false)
    call FogMaskEnable(false)
    call SetFloatGameState(GAME_STATE_TIME_OF_DAY,12)
    set actor=CreateUnit(Player(0),'hfoo',-700,0,0)
    call SetUnitInvulnerable(actor,true)
    call ClearSelection()
    call SelectUnit(actor,true)
    call PanCameraTo(-700,0)
    set actor=null
endfunction
function config takes nothing returns nothing
    call SetMapName("Crest momentum regression")
    call SetMapDescription("Private offline continuous hill trial")
    call SetPlayers(1)
    call SetTeams(1)
    call DefineStartLocation(0,-700,0)
    call SetPlayerStartLocation(Player(0),0)
    call SetPlayerController(Player(0),MAP_CONTROL_USER)
    call SetPlayerSlotAvailable(Player(0),MAP_CONTROL_USER)
    call SetPlayerRacePreference(Player(0),RACE_PREF_HUMAN)
    call SetPlayerRaceSelectable(Player(0),false)
endfunction
