// Original offline practice map. The builder fills lane data to match its authored terrain.
globals
    unit surfer = null
    integer selectedLane = 1
    real array laneY
    real array laneAngle
    boolean autoReturn = true
    boolean inRun = false
    real runTime = 0.
    integer runs = 0
endglobals

function SurfMessage takes string message returns nothing
    call DisplayTimedTextToPlayer(Player(0),0.,0.,9.,message)
endfunction

function SurfHelp takes nothing returns nothing
    call SurfMessage("|cffffcc00SURF PRACTICE|r | F6: FPS | W: разбег | Space: прыжок | A/D + мышь: управление на склоне.")
    call SurfMessage("Чат: -surf 1 / -surf 2 / -surf 3 | -reset: наверх | -auto: автовозврат | -help: подсказки.")
    call SurfMessage("Двигайся от зелёной площадки к полосатому склону. Стена впереди не нужна: здесь серфят по рельефу.")
endfunction

function SurfReset takes nothing returns nothing
    // Native relocation lets the installed mod rebase its actor/camera through the normal script-teleport path.
    call IssueImmediateOrder(surfer,"stop")
    call SetUnitX(surfer,-2688.)
    call SetUnitY(surfer,laneY[selectedLane])
    call SetUnitFacing(surfer,0.)
    set inRun=false
    set runTime=0.
    call ClearSelection()
    call SelectUnit(surfer,true)
    call PanCameraTo(-2688.,laneY[selectedLane])
    call SurfMessage("Дорожка " + I2S(selectedLane) + " | " + R2S(laneAngle[selectedLane]) + " градусов. Спуск в сторону X+ (на восток).")
endfunction

function SurfChat takes nothing returns nothing
    local string command=StringCase(GetEventPlayerChatString(),false)
    if command == "-surf 1" or command == "-surf 2" or command == "-surf 3" then
        set selectedLane=S2I(SubString(command,6,7))
        call SurfReset()
    elseif command == "-reset" then
        call SurfReset()
    elseif command == "-auto" then
        set autoReturn=not autoReturn
        if autoReturn then
            call SurfMessage("Автовозврат включён: после финиша или выхода за дорожку вернёшься наверх.")
        else
            call SurfMessage("Автовозврат выключен. Для возврата напиши -reset.")
        endif
    elseif command == "-help" then
        call SurfHelp()
    endif
endfunction

function SurfTick takes nothing returns nothing
    local real x=GetUnitX(surfer)
    local real y=GetUnitY(surfer)-laneY[selectedLane]
    if x > -1536. and not inRun then
        set inRun=true
        set runTime=0.
    endif
    if inRun then
        set runTime=runTime+0.1
    endif
    // The landing is deliberately long; a completed run or missed lane resets only when enabled.
    if autoReturn and (x > 1024. or x < -3584. or y > 864. or y < -864.) then
        if x > 1024. then
            set runs=runs+1
            call SurfMessage("|cff70ff80Финиш!|r Спусков: " + I2S(runs) + " | Время: " + R2S(runTime) + " с.")
        else
            call SurfMessage("Возврат на дорожку. Попробуй снова!")
        endif
        call SurfReset()
    endif
endfunction

function SurfLabel takes integer lane returns nothing
    local texttag label=CreateTextTag()
    call SetTextTagText(label,"SURF " + I2S(lane) + " | " + R2S(laneAngle[lane]) + " deg ->",0.035)
    call SetTextTagPos(label,-2304.,laneY[lane],160.)
    call SetTextTagColor(label,255,225,110,255)
    call SetTextTagPermanent(label,true)
    call SetTextTagVisibility(label,true)
    set label=null
endfunction

function main takes nothing returns nothing
    local trigger chat=CreateTrigger()
    local integer lane=1
    call SetCameraBounds(-3712.,-3712.,3712.,3712.,-3712.,3712.,3712.,-3712.)
    call SetDayNightModels("Environment\\DNC\\DNCLordaeron\\DNCLordaeronTerrain\\DNCLordaeronTerrain.mdl","Environment\\DNC\\DNCLordaeron\\DNCLordaeronUnit\\DNCLordaeronUnit.mdl")
    call FogEnable(false)
    call FogMaskEnable(false)
    call SetFloatGameState(GAME_STATE_TIME_OF_DAY,12.)
    call SetTimeOfDayScale(0.)
@LANE_SETUP@
    set surfer=CreateUnit(Player(0),'hfoo',-2688.,laneY[1],0.)
    call SetUnitInvulnerable(surfer,true)
    call SetUnitAcquireRange(surfer,0.)
    call SetUnitMoveSpeed(surfer,300.)
    call SetPlayerName(Player(0),"Surfer")
    call TriggerRegisterPlayerChatEvent(chat,Player(0),"-",false)
    call TriggerAddAction(chat,function SurfChat)
    call TimerStart(CreateTimer(),0.1,true,function SurfTick)
    loop
        exitwhen lane > 3
        call SurfLabel(lane)
        set lane=lane+1
    endloop
    call SurfReset()
    call TimerStart(CreateTimer(),1.,false,function SurfHelp)
    set chat=null
endfunction

function config takes nothing returns nothing
    call SetMapName("Warcraft CS - Surf Practice")
    call SetMapDescription("Три тренировочных склона для Warcraft CS. F6: FPS. -help: помощь.")
    call SetPlayers(1)
    call SetTeams(1)
    call SetGamePlacement(MAP_PLACEMENT_FIXED)
    call DefineStartLocation(0,-2688.,-2304.)
    call SetPlayerStartLocation(Player(0),0)
    call SetPlayerTeam(Player(0),0)
    call SetPlayerController(Player(0),MAP_CONTROL_USER)
    call SetPlayerRacePreference(Player(0),RACE_PREF_HUMAN)
    call SetPlayerRaceSelectable(Player(0),false)
endfunction
