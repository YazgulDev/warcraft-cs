// Original single-player time trial. Model placement is generated from the same route data as the overview.
globals
    unit runner=null
    location query=null
    timer clock=null
    texttag hud=null
    real array cpX
    real array cpY
    real array cpZ
    real array cpFacing
    integer checkpoint=0
    integer falls=0
    integer attempts=0
    real started=0.
    real best=0.
    real result=0.
    real grace=0.
    real lastZ=0.
    real stable=0.
    boolean active=false
    boolean finished=false
    boolean practice=false
    boolean automatic=true
    boolean showHud=true
    // Builders select course geometry/rules; timing and checkpoint behavior stay shared.
    string courseTitle="SURF SKYLINE"
    string courseHelp="Синий этап: восток, оранжевый: север, финальный: восток. Разгон W, на грани рампы A/D к её гребню + плавная мышь."
    real courseStartEdge=256.
    real courseStartMinimum=900.
    real courseFall=-1200.
    real coursePadWidth=230.
    real coursePadDepth=400.
    real courseFinishWidth=700.
endglobals

function SkylineAbs takes real value returns real
    if value < 0. then
        return -value
    endif
    return value
endfunction

function SkylineMessage takes string message returns nothing
    call DisplayTimedTextToPlayer(Player(0),0.,0.,8.,message)
endfunction

function SkylineFormat takes real seconds returns string
    local integer ticks=R2I(seconds*100.)
    local integer minutes=ticks/6000
    local integer secs=ticks/100-minutes*60
    local integer hundredths=ticks-(ticks/100)*100
    local string a=I2S(secs)
    local string b=I2S(hundredths)
    if secs < 10 then
        set a="0"+a
    endif
    if hundredths < 10 then
        set b="0"+b
    endif
    return I2S(minutes)+":"+a+"."+b
endfunction

function SkylineGround takes real x, real y returns real
    call MoveLocation(query,x,y)
    return GetLocationZ(query)
endfunction

function SkylineAttached takes nothing returns boolean
    local real dx=GetCameraEyePositionX()-GetUnitX(runner)
    local real dy=GetCameraEyePositionY()-GetUnitY(runner)
    return dx*dx+dy*dy < 16384.
endfunction

function SkylineHeight takes nothing returns real
    // First-person camera Z follows the mod's resolved body. The generous pad/fall tests
    // tolerate crouch-eye offset; RTS uses the native walkable surface instead.
    if SkylineAttached() then
        return GetCameraEyePositionZ()-96.
    endif
    return SkylineGround(GetUnitX(runner),GetUnitY(runner))+GetUnitFlyHeight(runner)
endfunction

function SkylineHelp takes nothing returns nothing
    call SkylineMessage("|cffffcc00"+courseTitle+"|r — 3 этапа, 2 чекпоинта. F6: FPS, P: скорость. Старт по выходу с первой площадки.")
    call SkylineMessage(courseHelp)
    call SkylineMessage("-cp: последний чекпоинт | -restart: новый забег | -stage 1/2/3: тренировка | -stats | -hud | -auto | -help")
    call SkylineMessage("Время при падениях и -cp продолжает идти. Тренировка не обновляет рекорд. Прохождение проверяй в FPS.")
endfunction

function SkylineReturn takes nothing returns nothing
    // All respawns use flat native walkable pads; the mod's script-teleport path rebases physical feet.
    call IssueImmediateOrder(runner,"stop")
    // Classic JASS decimal parsing must not overflow the fractional integer/divisor.
    // Nine fractional digits keep this conversion representable on the 1.26a interpreter.
    call SetUnitX(runner,cpX[checkpoint]+100.*Sin(cpFacing[checkpoint]*0.017453293))
    call SetUnitY(runner,cpY[checkpoint]-100.*Cos(cpFacing[checkpoint]*0.017453293))
    call SetUnitFacing(runner,cpFacing[checkpoint])
    call ClearSelection()
    call SelectUnit(runner,true)
    call PanCameraTo(cpX[checkpoint],cpY[checkpoint])
    set grace=TimerGetElapsed(clock)+0.5
    set lastZ=cpZ[checkpoint]
    set stable=0.
endfunction

function SkylineRestart takes nothing returns nothing
    set checkpoint=0
    set falls=0
    set active=false
    set finished=false
    set practice=false
    set result=0.
    call SkylineReturn()
    call SkylineMessage("Новый забег. Для старта двигайся на восток к первой синей рампе.")
endfunction

function SkylineTime takes nothing returns real
    if active then
        return TimerGetElapsed(clock)-started
    endif
    return result
endfunction

function SkylineHud takes nothing returns nothing
    local real ex=GetCameraEyePositionX()
    local real ey=GetCameraEyePositionY()
    local real ez=GetCameraEyePositionZ()
    local real fx=GetCameraTargetPositionX()-ex
    local real fy=GetCameraTargetPositionY()-ey
    local real fz=GetCameraTargetPositionZ()-ez
    local real length=SquareRoot(fx*fx+fy*fy+fz*fz)
    local real horizontal=SquareRoot(fx*fx+fy*fy)
    local real x
    local real y
    local real z
    local string mode="READY"
    if not showHud or not SkylineAttached() or length < 1. or horizontal < 0.1 then
        call SetTextTagVisibility(hud,false)
        return
    endif
    set fx=fx/length
    set fy=fy/length
    set fz=fz/length
    set horizontal=horizontal/length
    // A world text tag follows the actual camera ray; it does not depend on native panel layout.
    // Keep the timer below the mod's multi-line shortcut hints.
    set x=ex+fx*600.-fx*fz/horizontal*20.+fy/horizontal*(-270.)
    set y=ey+fy*600.-fy*fz/horizontal*20.-fx/horizontal*(-270.)
    set z=ez+fz*600.+horizontal*20.
    if active then
        set mode="RUN"
    endif
    if finished then
        set mode="FINISH"
    endif
    if practice then
        set mode="PRACTICE"
    endif
    call SetTextTagText(hud,courseTitle+" | "+mode+" | "+SkylineFormat(SkylineTime())+" | CP "+I2S(checkpoint)+"/2 | FALLS "+I2S(falls),0.023)
    call SetTextTagPos(hud,x,y,z-SkylineGround(x,y))
    call SetTextTagVisibility(hud,true)
endfunction

function SkylineStats takes nothing returns nothing
    call SkylineMessage("Время: "+SkylineFormat(SkylineTime())+" | Чекпоинт "+I2S(checkpoint)+"/2 | Падений: "+I2S(falls)+" | Попыток: "+I2S(attempts))
    if best > 0. then
        call SkylineMessage("Лучшее полное прохождение за эту сессию: "+SkylineFormat(best))
    endif
endfunction

function SkylinePublish takes nothing returns nothing
    local integer state=checkpoint
    local integer ticks=R2I(SkylineTime()*100.)
    local integer count=falls
    // The map owns this actor's custom value. Its bounded status snapshot lets optional
    // observers check timer/checkpoint rules without changing position or gameplay state.
    if active then
        set state=state+4
    endif
    if finished then
        set state=state+8
    endif
    if practice then
        set state=state+16
    endif
    if best > 0. then
        set state=state+32
    endif
    if ticks > 65535 then
        set ticks=65535
    endif
    if count > 127 then
        set count=127
    endif
    call SetUnitUserData(runner,state+count*64+ticks*8192)
endfunction

function SkylineChat takes nothing returns nothing
    local string command=StringCase(GetEventPlayerChatString(),false)
    local integer stage=0
    if command == "-restart" then
        call SkylineRestart()
    elseif command == "-cp" or command == "-reset" then
        if active then
            set falls=falls+1
        endif
        call SkylineReturn()
    elseif command == "-stage 1" or command == "-stage 2" or command == "-stage 3" then
        set stage=S2I(SubString(command,7,8))
        set checkpoint=stage-1
        set practice=true
        set active=true
        set finished=false
        set started=TimerGetElapsed(clock)
        set falls=0
        call SkylineReturn()
        call SkylineMessage("Тренировка этапа "+I2S(stage)+". Для полного забега используй -restart.")
    elseif command == "-auto" then
        set automatic=not automatic
        call SkylineMessage("Автовозврат переключён. Ручной возврат: -cp.")
    elseif command == "-hud" then
        set showHud=not showHud
    elseif command == "-stats" then
        call SkylineStats()
    elseif command == "-help" then
        call SkylineHelp()
    endif
endfunction

function SkylineTick takes nothing returns nothing
    local real now=TimerGetElapsed(clock)
    local real x=GetUnitX(runner)
    local real y=GetUnitY(runner)
    local real z=SkylineHeight()
    local integer next=checkpoint+1
    local real landingWidth=coursePadWidth
    if next == 3 then
        // Match the longer final braking deck while keeping its edges outside the finish gate.
        set landingWidth=courseFinishWidth
    endif
    if SkylineAbs(z-lastZ) < 5. then
        set stable=stable+0.05
    else
        set stable=0.
    endif
    set lastZ=z
    if now > grace then
        if not active and not finished and checkpoint == 0 and x > cpX[0]+courseStartEdge and z > courseStartMinimum then
            set started=now
            set active=true
            set attempts=attempts+1
            call SkylineMessage("|cff70ff80СТАРТ|r Пройди оба чекпоинта и финиш.")
        endif
        // Checkpoints must be reached in order and near their actual top after a stable landing.
        if active and SkylineAbs(x-cpX[next]) < landingWidth and SkylineAbs(y-cpY[next]) < coursePadDepth and SkylineAbs(z-cpZ[next]) < 64. and stable >= 0.1 then
            if next == 3 then
                set result=now-started
                set active=false
                set finished=true
                if not practice and (best == 0. or result < best) then
                    set best=result
                    call SkylineMessage("|cffffcc00НОВЫЙ РЕКОРД|r "+SkylineFormat(result))
                else
                    call SkylineMessage("|cff70ff80ФИНИШ|r "+SkylineFormat(result))
                endif
                call SkylineStats()
            else
                set checkpoint=next
                call SkylineMessage("|cff70ff80ЧЕКПОИНТ "+I2S(checkpoint)+"/2|r Время: "+SkylineFormat(now-started))
            endif
        endif
        if automatic and z < courseFall then
            if active then
                set falls=falls+1
            endif
            call SkylineReturn()
        endif
    endif
    call SkylinePublish()
    call SkylineHud()
endfunction

function SkylineSolid takes integer raw, real x, real y, real z, real facing returns nothing
    local destructable object=CreateDestructableZ(raw,x,y,z,facing,1.,0)
    call SetDestructableInvulnerable(object,true)
    set object=null
endfunction

function SkylineLabel takes integer index, string caption returns nothing
    local texttag label=CreateTextTag()
    call SetTextTagText(label,caption,0.04)
    call SetTextTagPos(label,cpX[index],cpY[index],180.)
    call SetTextTagColor(label,110,255,175,255)
    call SetTextTagPermanent(label,true)
    call SetTextTagVisibility(label,true)
    set label=null
endfunction

function main takes nothing returns nothing
    local trigger chat=CreateTrigger()
    call SetCameraBounds(-7808.,-7808.,7808.,7808.,-7808.,7808.,7808.,-7808.)
    call SetDayNightModels("Environment\\DNC\\DNCLordaeron\\DNCLordaeronTerrain\\DNCLordaeronTerrain.mdl","Environment\\DNC\\DNCLordaeron\\DNCLordaeronUnit\\DNCLordaeronUnit.mdl")
    call FogEnable(false)
    call FogMaskEnable(false)
    call SetFloatGameState(GAME_STATE_TIME_OF_DAY,12.)
    call SetTimeOfDayScale(0.)
    set query=Location(0.,0.)
    set clock=CreateTimer()
    call TimerStart(clock,360000.,false,null)
@COURSE_DATA@
    set runner=CreateUnit(Player(0),'hfoo',cpX[0],cpY[0]-100.,0.)
    call SetUnitInvulnerable(runner,true)
    call SetUnitAcquireRange(runner,0.)
    call SetUnitMoveSpeed(runner,300.)
    set hud=CreateTextTag()
    call SetTextTagPermanent(hud,true)
    call SetTextTagColor(hud,255,240,150,255)
@COURSE_LABELS@
    call TriggerRegisterPlayerChatEvent(chat,Player(0),"-",false)
    call TriggerAddAction(chat,function SkylineChat)
    call SkylineRestart()
    call TimerStart(CreateTimer(),0.05,true,function SkylineTick)
    call TimerStart(CreateTimer(),1.,false,function SkylineHelp)
    set chat=null
endfunction

function config takes nothing returns nothing
    call SetMapName("@MAP_NAME@")
    call SetMapDescription("@MAP_DESCRIPTION@")
    call SetPlayers(1)
    call SetTeams(1)
    call SetGamePlacement(MAP_PLACEMENT_FIXED)
    call DefineStartLocation(0,@START_X@,@START_Y@)
    call SetPlayerStartLocation(Player(0),0)
    call SetPlayerTeam(Player(0),0)
    call SetPlayerController(Player(0),MAP_CONTROL_USER)
    call SetPlayerRacePreference(Player(0),RACE_PREF_HUMAN)
    call SetPlayerRaceSelectable(Player(0),false)
endfunction
