$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'setup/gameplay-config.ps1')
$folder=Join-Path $root ('.local/config-merge-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $folder -Force | Out-Null
$template=Join-Path $root 'config/WarcraftCS.ini'
# Semicolon custom actions survive; only the former squad default migrates to Y.
foreach($spelling in @('SEMICOLON',';')) {
    $semicolonConfig=Join-Path $folder ('semicolon-'+[guid]::NewGuid().ToString('N')+'.ini')
    @('[Shortcuts]',('Speed='+$spelling)) | Set-Content -LiteralPath $semicolonConfig -Encoding ascii
    Update-GameplayConfig $template $semicolonConfig
    $semicolonText=Get-Content -LiteralPath $semicolonConfig -Raw
    if(!$semicolonText.Contains('Speed='+$spelling) -or !$semicolonText.Contains('SquadOrders=Y')){throw 'Semicolon migration stole a saved shortcut.'}
    foreach($reserved in @($false,$true)) {
        @('[Shortcuts]',('SquadOrders='+$spelling),$(if($reserved){'Speed=Y'}else{'Speed=G'})) | Set-Content -LiteralPath $semicolonConfig -Encoding ascii
        Update-GameplayConfig $template $semicolonConfig
        $migrated=Get-Content -LiteralPath $semicolonConfig -Raw
        $expected=if($reserved){'NONE'}else{'Y'}
        if(!$migrated.Contains('SquadOrders='+$expected) -or !$migrated.Contains($(if($reserved){'Speed=Y'}else{'Speed=G'}))){throw 'Old squad default migration lost a custom key.'}
        $hash=(Get-FileHash $semicolonConfig).Hash;Update-GameplayConfig $template $semicolonConfig
        if((Get-FileHash $semicolonConfig).Hash -ne $hash){throw 'Squad Y migration is not idempotent.'}
    }
}
# Missing defaults yield to custom Y; custom squad remaps/NONE survive updates.
foreach($binding in @('','G','NONE')) {
    $customConfig=Join-Path $folder ('squad-key-'+$binding+'.ini')
    @('[Shortcuts]','Speed=Y',$(if($binding){'SquadOrders='+$binding}else{'; missing SquadOrders'}),'[Squad]','PreserveCombatOrders=false') | Set-Content -LiteralPath $customConfig -Encoding ascii
    Update-GameplayConfig $template $customConfig
    $customText=Get-Content -LiteralPath $customConfig -Raw
    $expected=if($binding){$binding}else{'NONE'}
    if(!$customText.Contains('Speed=Y') -or !$customText.Contains('SquadOrders='+$expected) -or !$customText.Contains('PreserveCombatOrders=false')){throw 'Squad migration lost a saved preference.'}
}
# New reticle defaults fill old configs; explicit range/weapon-mode opt-ins survive repeated updates.
$focusConfig=Join-Path $folder 'squad-reticle.ini'
@('[Squad]','ReticleAttack=false','ReticleAttackInFPS=true','ReticleAttackDistance=800') | Set-Content -LiteralPath $focusConfig -Encoding ascii
Update-GameplayConfig $template $focusConfig
$focusText=Get-Content -LiteralPath $focusConfig -Raw
foreach($required in @('ReticleAttack=false','ReticleAttackInFPS=true','ReticleAttackDistance=800','PreserveCombatOrders=true')) {
    if(!$focusText.Contains($required)){throw "Reticle migration lost preference: $required"}
}
$focusHash=(Get-FileHash -LiteralPath $focusConfig).Hash;Update-GameplayConfig $template $focusConfig
if((Get-FileHash -LiteralPath $focusConfig).Hash -ne $focusHash){throw 'Reticle migration is not idempotent.'}
# Inventory controls preserve custom Tab/I ownership and saved drop/display settings on repeated upgrades.
$inventoryConfig=Join-Path $folder 'interactive-inventory.ini'
@('[Shortcuts]','Speed=TAB','Pickup=I','[Inventory]','DropDistance=96','NumbersByDefault=false','[ThirdPerson]','CameraBuildingCollision=true','[Interface]','InventoryControls=false') | Set-Content -LiteralPath $inventoryConfig -Encoding ascii
Update-GameplayConfig $template $inventoryConfig
$inventoryText=Get-Content -LiteralPath $inventoryConfig -Raw
foreach($required in @('Speed=TAB','Pickup=I','InventoryNumbers=NONE','InventoryPanel=NONE','DropDistance=96','InventoryControls=false','NumbersByDefault=false','CameraBuildingCollision=true')) {
    if(!$inventoryText.Contains($required)){throw "Inventory migration lost preference: $required"}
}
$inventoryHash=(Get-FileHash -LiteralPath $inventoryConfig).Hash;Update-GameplayConfig $template $inventoryConfig
if((Get-FileHash -LiteralPath $inventoryConfig).Hash -ne $inventoryHash){throw 'Inventory migration is not idempotent.'}
# Missing new keys get the requested defaults, without replacing an existing unrelated zoom preference.
$defaultsConfig=Join-Path $folder 'camera-inventory-defaults.ini'
@('[ThirdPerson]','CameraDistance=650','[Inventory]','DropDistance=90') | Set-Content -LiteralPath $defaultsConfig -Encoding ascii
Update-GameplayConfig $template $defaultsConfig
$defaultsText=Get-Content -LiteralPath $defaultsConfig -Raw
foreach($required in @('CameraDistance=650','DropDistance=90','CameraBuildingCollision=false','NumbersByDefault=true','ReticleAttack=true','ReticleAttackInFPS=false','ReticleAttackDistance=3000')) {
    if(!$defaultsText.Contains($required)){throw "New camera/inventory default missing: $required"}
}
# HUD upgrades add missing keys while preserving independent opt-outs and the player's scale.
$hudConfig=Join-Path $folder 'unit-hud.ini'
@('[Interface]','ShowUnitHud=false','ShowInventory=false','UnitHudScalePercent=125') | Set-Content -LiteralPath $hudConfig -Encoding ascii
Update-GameplayConfig $template $hudConfig
$hudText=Get-Content -LiteralPath $hudConfig -Raw
foreach($setting in @('ShowUnitHud=false','ShowInventory=false','UnitHudScalePercent=125','ShowSpellIcons=true')) {
    if(!$hudText.Contains($setting)){throw "Unit HUD migration lost preference: $setting"}
}
$hudHash=(Get-FileHash -LiteralPath $hudConfig).Hash;Update-GameplayConfig $template $hudConfig
if((Get-FileHash -LiteralPath $hudConfig).Hash -ne $hudHash){throw 'Unit HUD migration is not idempotent.'}
# Fresh defaults use 5000, while explicit saved distances and hover opt-outs survive repeated upgrades.
$distance=Join-Path $folder 'distance-hover.ini'
@('[Rendering]','DrawDistance=7000','[UnitView]','HideHoverCircles=false') | Set-Content -LiteralPath $distance -Encoding ascii
Update-GameplayConfig $template $distance
$distanceText=Get-Content -LiteralPath $distance -Raw
if(!$distanceText.Contains('DrawDistance=7000') -or !$distanceText.Contains('HideHoverCircles=false')){throw 'Saved visibility preferences overwritten.'}
if(!(Get-Content -LiteralPath $template -Raw).Contains('DrawDistance=5000')){throw 'Fresh distance default is not 5000.'}
. (Join-Path $root 'setup/installation-profile.ps1')
# Only an explicit setup choice replaces the profile; ordinary F8/default migrations preserve it.
$profile=Join-Path $folder 'profile.ini'
@('[Installation]','; private comment','[Combat]','FriendlyFirePercent=17') | Set-Content -LiteralPath $profile -Encoding utf8
Set-InstallationProfile $profile $true
Update-GameplayConfig $template $profile
$text=Get-Content -LiteralPath $profile -Raw
if(!$text.Contains('WarcraftOnly=true') -or !$text.Contains('FriendlyFirePercent=17') -or ([regex]::Matches($text,'\[Installation\]')).Count -ne 1){throw 'Profile policy lost customization or duplicated its section.'}
$hash=(Get-FileHash -LiteralPath $profile).Hash;Set-InstallationProfile $profile $true
if((Get-FileHash -LiteralPath $profile).Hash -ne $hash){throw 'Profile update is not idempotent.'}
Set-InstallationProfile $profile $false
if(!(Get-Content -LiteralPath $profile -Raw).Contains('WarcraftOnly=false')){throw 'Full install cannot restore weapons.'}
# Water occlusion defaults off; explicit legacy opt-ins survive repeated upgrades.
foreach($choice in @('false','true')) {
    $water=Join-Path $folder ('water-'+$choice+'.ini')
    @('[Combat]',('WaterBlocksAttacks='+$choice)) | Set-Content -LiteralPath $water -Encoding ascii
    Update-GameplayConfig $template $water
    if(!(Get-Content -LiteralPath $water -Raw).Contains('WaterBlocksAttacks='+$choice)){throw 'Water occlusion preference reset.'}
    $before=(Get-FileHash -LiteralPath $water).Hash;Update-GameplayConfig $template $water
    if((Get-FileHash -LiteralPath $water).Hash -ne $before){throw 'Water occlusion update is not idempotent.'}
}
# Relocate only legacy AllWeapons=F9, while preserving custom AI preferences and the remainder of the layout.
$unitConfig=Join-Path $folder 'unit-view.ini'
@('[Shortcuts]','AllWeapons=F9','Reload=Y','[Squad]','AutomaticSpells=false','SpellSearchRadius=420','[UnitView]','Enabled=false') |
    Set-Content -LiteralPath $unitConfig -Encoding ascii
Update-GameplayConfig $template $unitConfig
$unitText=Get-Content -LiteralPath $unitConfig -Raw
foreach($required in @('AllWeapons=F12','ToggleUnitView=F9','Reload=Y','AutomaticSpells=false','SpellSearchRadius=420','Enabled=false','PreventEnemyAttacks=true')) {
    if(!$unitText.Contains($required)){throw "Unit view/AI migration lost preference: $required"}
}
$unitHash=(Get-FileHash -LiteralPath $unitConfig).Hash;Update-GameplayConfig $template $unitConfig
if((Get-FileHash -LiteralPath $unitConfig).Hash -ne $unitHash){throw 'Unit view migration is not idempotent.'}
# Explicit vulnerability preferences survive updates rather than being replaced by the protected default.
$unprotected=Join-Path $folder 'unit-view-unprotected.ini'
@('[UnitView]','PreventEnemyAttacks=false') | Set-Content -LiteralPath $unprotected -Encoding ascii
Update-GameplayConfig $template $unprotected
if(!(Get-Content -LiteralPath $unprotected -Raw).Contains('PreventEnemyAttacks=false')){throw 'Unit-view enemy-attack preference overwritten.'}
# Newly added allied targeting preserves a chosen default and an older custom Q binding.
$allyConfig=Join-Path $folder 'allied-targeting.ini'
@('[Combat]','TargetAllies=true','[Shortcuts]','Reload=Q') | Set-Content -LiteralPath $allyConfig -Encoding ascii
Update-GameplayConfig $template $allyConfig
$allyText=Get-Content -LiteralPath $allyConfig -Raw
foreach($required in @('TargetAllies=true','Reload=Q','ToggleAllyTargeting=NONE')) {
    if(!$allyText.Contains($required)){throw "Allied targeting migration lost preference: $required"}
}
$allyHash=(Get-FileHash -LiteralPath $allyConfig).Hash
Update-GameplayConfig $template $allyConfig
if((Get-FileHash -LiteralPath $allyConfig).Hash -ne $allyHash){throw 'Allied targeting migration is not idempotent.'}
$config=Join-Path $folder 'legacy.ini'
@('[Runes]','AmmoPercent=37','[AK47]','Price=0','Damage=81','[Damage]','FriendlyFirePercent=25') | Set-Content -LiteralPath $config -Encoding ascii
# Updates append missing keys inside existing sections while retaining all custom values.
Update-GameplayConfig $template $config
$text=Get-Content -LiteralPath $config -Raw
foreach ($required in @('AmmoPercent=37','Price=0','Damage=81','FriendlyFirePercent=25','AWPOneShot=false','BombCount=0','Access=anywhere','AmmoPrice=80','CSVolumePercent=100','Detailed=true','IntervalMs=1000','MaxFileMB=8','ArchiveCount=3','JumpBoostPercent=0','AutoJump=true','MaxFPS=144','DrawDistance=5000','HideHoverCircles=true')) {
    if (!$text.Contains($required)) { throw "Missing/patched preference: $required" }
}
if ([regex]::Matches($text,'(?m)^\[AK47\]').Count -ne 1) { throw 'Duplicate section can hide new INI keys.' }
if ([regex]::Matches($text,'(?m)^\[Audio\]').Count -ne 1) { throw 'Audio migration duplicated or omitted its section.' }
$before=(Get-FileHash -LiteralPath $config).Hash
if(!$text.Contains('ShowTargetIndicator=true')) { throw 'Target indicator default missing from migration.' }
$customMarker=Join-Path $folder 'custom-target-marker.ini'
@('[ThirdPerson]','ShowTargetIndicator=false','TargetCircleRadius=96') | Set-Content -LiteralPath $customMarker -Encoding ascii
Update-GameplayConfig $template $customMarker
if(!(Get-Content -LiteralPath $customMarker -Raw).Contains('ShowTargetIndicator=false')) { throw 'Migration reset the hidden target marker preference.' }
if(!(Get-Content -LiteralPath $customMarker -Raw).Contains('TargetCircleRadius=96') -or !$text.Contains('TargetCircleRadius=64')) { throw 'Circle radius migration/default failed.' }
Update-GameplayConfig $template $config
if ((Get-FileHash -LiteralPath $config).Hash -ne $before) { throw 'Repeated config update changed an already merged file.' }
# Reserve/scaling upgrades preserve existing values and do not silently replace a player's damage/economy mode.
$customScaling=Join-Path $folder 'custom-unit-scaling.ini'
@('[AK47]','MaxReserve=250','Price=321','[Damage]','Mode=weapon','[Runes]','AmmoPercent=11',
    '[Loadout]','BombCount=7','UnlimitedBombPurchases=false','[Combat]','ScaleFireRateFromAttackSpeed=true','EnableAttackEffects=false','IgnorePassableInvulnerableObjects=false','[C4]','Price=4321','AmmoPrice=3456','[Movement]','ScaleFromUnitMoveSpeed=true','UnitMoveSpeedReference=270') |
    Set-Content -LiteralPath $customScaling -Encoding ascii
Update-GameplayConfig $template $customScaling
$scalingText=Get-Content -LiteralPath $customScaling -Raw
foreach($setting in @('MaxReserve=250','Price=321','Mode=weapon','AmmoPercent=11','ScaleFireRateFromAttackSpeed=true',
    'ScaleFromUnitMoveSpeed=true','UnitMoveSpeedReference=270','EnableAttackEffects=false','IgnorePassableInvulnerableObjects=false','Price=4321','AmmoPrice=3456','BombCount=7','UnlimitedBombPurchases=false')) {
    if(!$scalingText.Contains($setting)){throw "Scaling migration lost preference: $setting"}
}
$scalingHash=(Get-FileHash -LiteralPath $customScaling).Hash
Update-GameplayConfig $template $customScaling
if((Get-FileHash -LiteralPath $customScaling).Hash -ne $scalingHash){throw 'Scaling migration is not idempotent.'}
foreach($setting in @('MaxReserve=90','ScaleFireRateFromAttackSpeed=false','EnableAttackEffects=true','IgnorePassableInvulnerableObjects=true','ScaleFromUnitMoveSpeed=false','UnitMoveSpeedReference=300','Mode=unit','UnlimitedBombPurchases=true')) {
    if(!$text.Contains($setting)){throw "Missing scaling default: $setting"}
}
# Existing CS-volume preferences survive both updates and repeated setup; missing sibling settings still migrate.
$customAudio=Join-Path $folder 'custom-audio.ini'
@('[Audio]','CSVolumePercent=35','[Runes]','AmmoPercent=11') | Set-Content -LiteralPath $customAudio -Encoding ascii
Update-GameplayConfig $template $customAudio
$audioText=Get-Content -LiteralPath $customAudio -Raw
if (!$audioText.Contains('CSVolumePercent=35') -or !$audioText.Contains('AmmoPercent=11') -or !$audioText.Contains('BombCount=0')) {
    throw 'Audio update overwrote preferences or omitted newly migrated defaults.'
}
if ([regex]::Matches($audioText,'(?m)^\[Audio\]').Count -ne 1 -or [regex]::Matches($audioText,'(?m)^CSVolumePercent=').Count -ne 1) {
    throw 'Custom audio setting was duplicated.'
}
$audioBefore=(Get-FileHash -LiteralPath $customAudio).Hash
Update-GameplayConfig $template $customAudio
if ((Get-FileHash -LiteralPath $customAudio).Hash -ne $audioBefore) { throw 'Repeated audio migration changed the config.' }
$empty=Join-Path $folder 'empty.ini'
# Preserve custom team-bar preferences and add defaults without duplicate sections/keys.
foreach ($preference in @('true','false')) {
    $barConfig=Join-Path $folder ('custom-health-bars-'+$preference+'.ini')
    @('[Interface]',('ShowAllyHealthBars='+$preference),'ShowEnemyHealthBars=true','HealthBarDistance=850','ShowSpeed=true') | Set-Content -LiteralPath $barConfig -Encoding ascii
    Update-GameplayConfig $template $barConfig
    $barText=Get-Content -LiteralPath $barConfig -Raw
    foreach ($key in @('ShowAllyHealthBars','ShowEnemyHealthBars','HealthBarDistance')) {
        if ([regex]::Matches($barText,('(?m)^'+$key+'=')).Count -ne 1) {throw 'Health bar migration duplicated a key.'}
    }
    if (!$barText.Contains('ShowAllyHealthBars='+$preference) -or !$barText.Contains('ShowEnemyHealthBars=true') -or
        !$barText.Contains('HealthBarDistance=850') -or !$barText.Contains('ShowSpeed=true') -or
        [regex]::Matches($barText,'(?m)^\[Interface\]').Count -ne 1) {throw 'Health bar migration replaced preferences.'}
    $barBefore=(Get-FileHash -LiteralPath $barConfig).Hash;Update-GameplayConfig $template $barConfig
    if ((Get-FileHash -LiteralPath $barConfig).Hash -ne $barBefore) {throw 'Health bar migration is not idempotent.'}
}
foreach ($default in @('ShowAllyHealthBars=false','ShowEnemyHealthBars=false','HealthBarDistance=2000')) {
    if (!$text.Contains($default)) {throw ('Legacy config lacks '+$default)}
}
# Optional speed/fog preferences migrate into the existing Interface section without resetting it.
foreach ($preference in @('true','false')) {
    $speedConfig=Join-Path $folder ('custom-speed-'+$preference+'.ini')
    @('[Interface]',('ShowSpeed='+$preference),('DisableFogOfWar='+$preference),'FloatingTextDistance=777') | Set-Content -LiteralPath $speedConfig -Encoding ascii
    Update-GameplayConfig $template $speedConfig
    $speedText=Get-Content -LiteralPath $speedConfig -Raw
    if (!$speedText.Contains('ShowSpeed='+$preference) -or !$speedText.Contains('FloatingTextDistance=777') -or
        !$speedText.Contains('DisableFogOfWar='+$preference) -or [regex]::Matches($speedText,'(?m)^DisableFogOfWar=').Count -ne 1 -or
        [regex]::Matches($speedText,'(?m)^ShowSpeed=').Count -ne 1 -or [regex]::Matches($speedText,'(?m)^\[Interface\]').Count -ne 1) {
        throw 'Speed counter migration reset or duplicated preferences.'
    }
    $speedBefore=(Get-FileHash -LiteralPath $speedConfig).Hash
    Update-GameplayConfig $template $speedConfig
    if ((Get-FileHash -LiteralPath $speedConfig).Hash -ne $speedBefore) { throw 'Speed migration is not idempotent.' }
}
if (!$text.Contains('ShowSpeed=false')) { throw 'Legacy config lacks the default speed counter key.' }
if (!$text.Contains('ShowMinimap=true') -or !$text.Contains('MinimapSizePercent=22')) {throw 'Legacy config lacks minimap defaults.'}
# Preserve explicit map visibility/size through migration and an idempotent second update.
$mapConfig=Join-Path $folder 'custom-minimap.ini'
@('[Interface]','ShowMinimap=false','MinimapSizePercent=31','ShowSpeed=true','ShowSpellIcons=false','ShowSpellCooldowns=false','SpellIconSizePercent=11') | Set-Content -LiteralPath $mapConfig -Encoding ascii
Update-GameplayConfig $template $mapConfig
$mapText=Get-Content -LiteralPath $mapConfig -Raw
foreach ($setting in @('ShowMinimap=false','MinimapSizePercent=31','ShowSpeed=true','ShowSpellIcons=false','ShowSpellCooldowns=false','SpellIconSizePercent=11')) {
    if (!$mapText.Contains($setting)) {throw "Minimap migration lost preference: $setting"}
}
if ([regex]::Matches($mapText,'(?m)^\[Interface\]').Count -ne 1) {throw 'Minimap migration duplicated Interface.'}
$mapHash=(Get-FileHash -LiteralPath $mapConfig).Hash
Update-GameplayConfig $template $mapConfig
if ((Get-FileHash -LiteralPath $mapConfig).Hash -ne $mapHash) {throw 'Minimap migration is not idempotent.'}
if (!$text.Contains('DisableFogOfWar=false')) { throw 'Legacy config lacks the default fog override key.' }
# Movement upgrades retain personal jump tuning and add missing siblings exactly once.
$customMovement=Join-Path $folder 'custom-movement.ini'
@('[Movement]','JumpBoostPercent=15','AutoJump=false','UseNativeUnitCollision=false','[Damage]','FriendlyFirePercent=0') | Set-Content -LiteralPath $customMovement -Encoding ascii
Update-GameplayConfig $template $customMovement
$movementText=Get-Content -LiteralPath $customMovement -Raw
foreach ($required in @('JumpBoostPercent=15','AutoJump=false','UseNativeUnitCollision=false','FriendlyFirePercent=0','MaxBunnySpeed=1000','Gravity=800','BunnyHop=true')) {
    if (!$movementText.Contains($required)) { throw "Movement migration replaced or omitted: $required" }
}
if ([regex]::Matches($movementText,'(?m)^\[Movement\]').Count -ne 1) { throw 'Movement section was duplicated.' }
$movementBefore=(Get-FileHash -LiteralPath $customMovement).Hash
Update-GameplayConfig $template $customMovement
if ((Get-FileHash -LiteralPath $customMovement).Hash -ne $movementBefore) { throw 'Movement migration is not idempotent.' }
[IO.File]::WriteAllText($empty,'');Update-GameplayConfig $template $empty
if (!(Get-Content -LiteralPath $empty -Raw).Contains('BombCount=0')) { throw 'Empty config repair failed.' }
$fresh=Join-Path $folder 'fresh.ini';Update-GameplayConfig $template $fresh
if ((Get-FileHash -LiteralPath $fresh).Hash -ne (Get-FileHash -LiteralPath $template).Hash) { throw 'Fresh config differs from defaults.' }
# New defaults must not overwrite an existing player's explicit AWP finishing preference.
foreach ($preference in @('true','false')) {
    $awpConfig=Join-Path $folder ('custom-awp-'+$preference+'.ini')
    @('[Damage]',('AWPOneShot='+$preference)) | Set-Content -LiteralPath $awpConfig -Encoding ascii
    Update-GameplayConfig $template $awpConfig
    $awpText=Get-Content -LiteralPath $awpConfig -Raw
    if ([regex]::Matches($awpText,'(?m)^AWPOneShot=').Count -ne 1 -or !$awpText.Contains('AWPOneShot='+$preference)) {
        throw 'AWP migration overwrote or duplicated an explicit preference.'
    }
    $awpBefore=(Get-FileHash -LiteralPath $awpConfig).Hash
    Update-GameplayConfig $template $awpConfig
    if ((Get-FileHash -LiteralPath $awpConfig).Hash -ne $awpBefore) { throw 'Repeated AWP migration changed the config.' }
}
Write-Output 'PASS fresh/legacy/empty/repeated config updates preserve custom values and expose new settings.'
# Rendering migration adds defaults without changing an explicit old/custom cap or distance.
foreach ($cap in @('0','120','240')) {
    $renderConfig=Join-Path $folder ('custom-render-'+$cap+'.ini')
    @('[Rendering]',('MaxFPS='+$cap),'DrawDistance=7500') | Set-Content -LiteralPath $renderConfig -Encoding ascii
    Update-GameplayConfig $template $renderConfig
    $renderText=Get-Content -LiteralPath $renderConfig -Raw
    if (!$renderText.Contains('MaxFPS='+$cap) -or !$renderText.Contains('DrawDistance=7500') -or
        [regex]::Matches($renderText,'(?m)^\[Rendering\]').Count -ne 1 -or
        [regex]::Matches($renderText,'(?m)^MaxFPS=').Count -ne 1) {throw 'Rendering update lost/duplicated custom preferences.'}
    $renderBefore=(Get-FileHash -LiteralPath $renderConfig).Hash
    Update-GameplayConfig $template $renderConfig
    if ((Get-FileHash -LiteralPath $renderConfig).Hash -ne $renderBefore) {throw 'Rendering migration is not idempotent.'}
}
# Logging preferences survive upgrades while missing sibling keys are inserted exactly once.
$customLogging=Join-Path $folder 'custom-logging.ini'
@('[Logging]','Detailed=false','IntervalMs=2500','MaxFileMB=2') | Set-Content -LiteralPath $customLogging -Encoding ascii
Update-GameplayConfig $template $customLogging
$loggingText=Get-Content -LiteralPath $customLogging -Raw
foreach ($setting in @('Detailed=false','IntervalMs=2500','MaxFileMB=2','ArchiveCount=3')) {
    if (!$loggingText.Contains($setting)) { throw "Logging migration lost preference/default: $setting" }
}
if ([regex]::Matches($loggingText,'(?m)^\[Logging\]').Count -ne 1) { throw 'Logging migration duplicated its section.' }
$loggingBefore=(Get-FileHash -LiteralPath $customLogging).Hash
Update-GameplayConfig $template $customLogging
if ((Get-FileHash -LiteralPath $customLogging).Hash -ne $loggingBefore) { throw 'Repeated logging migration changed the file.' }
# A custom keyboard layout and spell mapping must survive updates, with missing siblings inserted once.
$customShortcuts=Join-Path $folder 'custom-shortcuts.ini'
@('[Shortcuts]','Spell1=Q','Spell2=E','Pickup=Z','Speed=TAB','Minimap=Y','[Interface]','ShowMinimap=false','ShowAllyHealthBars=true','ShowEnemyHealthBars=true','[Spells]','Slot1Ability=AHtb','Slot1Order=852095','Slot1Target=unit','AimDistance=850') | Set-Content -LiteralPath $customShortcuts -Encoding ascii
Update-GameplayConfig $template $customShortcuts
$shortcutsText=Get-Content -LiteralPath $customShortcuts -Raw
foreach($setting in @('Spell1=Q','Spell2=E','Pickup=Z','Speed=TAB','Minimap=Y','ShowMinimap=false','ShowAllyHealthBars=true','ShowEnemyHealthBars=true','Spell3=C','Spell4=V','Slot1Ability=AHtb','Slot1Order=852095','Slot1Target=unit','AimDistance=850','Enabled=true')) {
    if(!$shortcutsText.Contains($setting)) {throw "Shortcut/spell migration lost preference/default: $setting"}
}
foreach($section in @('Shortcuts','Spells')) {
    if([regex]::Matches($shortcutsText,'(?m)^\['+$section+'\]').Count -ne 1) {throw 'Shortcut/spell section was duplicated.'}
}
$shortcutsBefore=(Get-FileHash -LiteralPath $customShortcuts).Hash
Update-GameplayConfig $template $customShortcuts
if((Get-FileHash -LiteralPath $customShortcuts).Hash -ne $shortcutsBefore) {throw 'Shortcut/spell migration is not idempotent.'}
# Third-person updates retain chosen camera/damage values and a custom view key, adding missing defaults once.
$third=Join-Path $folder 'custom-third-person.ini'
@('[ThirdPerson]','CameraDistance=480','CameraZoomStep=75','CameraMinDistance=200','CameraMaxDistance=800','DamageMultiplier=1.5','RunCycleDistance=260','MovementAnimations=false','RangedAttackRange=777','NativeRangedProjectiles=false','[Shortcuts]','ToggleView=TAB') | Set-Content -LiteralPath $third -Encoding ascii
Update-GameplayConfig $template $third
$thirdText=Get-Content -LiteralPath $third -Raw
foreach($key in @('CameraZoomStep=75','CameraMinDistance=200','CameraMaxDistance=800')) {
    if(!$thirdText.Contains($key)){throw "Camera zoom preference was reset: $key"}
}
foreach($key in @('CameraZoomStep=50','CameraMinDistance=80','CameraMaxDistance=1000')) {
    if(!$text.Contains($key)){throw "Camera zoom default missing: $key"}
}
foreach($setting in @('CameraDistance=480','DamageMultiplier=1.5','ToggleView=TAB','AttackRange=150','ContactDelay=0.56','RunCycleDistance=260','MovementAnimations=false','TakeoffDuration=0.10','LandingDuration=0.16','AutoAnimationSpeed=false','RangedAttackRange=777','NativeRangedProjectiles=false','UseNativeAttackRange=true','RangedRangeMultiplier=1','MeleeHeightTolerance=32')) {
    if(!$thirdText.Contains($setting)){throw "Third-person migration lost preference/default: $setting"}
}
if([regex]::Matches($thirdText,'(?m)^\[ThirdPerson\]').Count -ne 1){throw 'Duplicate third-person section.'}
$thirdBefore=(Get-FileHash -LiteralPath $third).Hash
Update-GameplayConfig $template $third
if((Get-FileHash -LiteralPath $third).Hash -ne $thirdBefore){throw 'Third-person migration is not idempotent.'}
# Preserve custom squad preferences and insert the new rear/yield settings once in the existing section.
$customSquad=Join-Path $folder 'custom-squad.ini'
@('[Squad]','FollowDistance=240','RearFormation=false','PlayerClearance=160','OrderIntervalMs=250') | Set-Content -LiteralPath $customSquad -Encoding ascii
Update-GameplayConfig $template $customSquad
$squadText=Get-Content -LiteralPath $customSquad -Raw
foreach($setting in @('FollowDistance=240','RearFormation=false','PlayerClearance=160','OrderIntervalMs=250','YieldToPlayer=true','YieldAheadDistance=350')) {
    if(!$squadText.Contains($setting)){throw "Squad migration lost preference/default: $setting"}
}
if([regex]::Matches($squadText,'(?m)^\[Squad\]').Count -ne 1){throw 'Duplicate squad section.'}
$squadBefore=(Get-FileHash -LiteralPath $customSquad).Hash
Update-GameplayConfig $template $customSquad
if((Get-FileHash -LiteralPath $customSquad).Hash -ne $squadBefore){throw 'Squad migration is not idempotent.'}
# Existing defaults relocate F7 only once, retaining custom layout choices and avoiding an occupied F11.
$legacyEntry=Join-Path $folder 'legacy-tps-entry.ini'
@('[Shortcuts]','Refill=F7','Speed=F11','Pickup=Y') | Set-Content -LiteralPath $legacyEntry -Encoding ascii
Update-GameplayConfig $template $legacyEntry
$legacyText=Get-Content -LiteralPath $legacyEntry -Raw
foreach($setting in @('Refill=F12','AllWeapons=F5','ToggleUnitView=F9','ToggleThirdPerson=F7','Speed=F11','Pickup=Y','AutoAnimationSpeed=true','AnimationSpeedMultiplier=1')) {
    if(!$legacyText.Contains($setting)){throw "Legacy TPS entry migration lost preference/default: $setting"}
}
$legacyHash=(Get-FileHash -LiteralPath $legacyEntry).Hash
Update-GameplayConfig $template $legacyEntry
if((Get-FileHash -LiteralPath $legacyEntry).Hash -ne $legacyHash){throw 'TPS entry migration is not idempotent.'}
# Both old defaults must migrate together without reserving F12 twice.
$legacyBoth=Join-Path $folder 'legacy-both-views.ini'
@('[Shortcuts]','Refill=F7','Speed=F11','AllWeapons=F9') | Set-Content -LiteralPath $legacyBoth -Encoding ascii
Update-GameplayConfig $template $legacyBoth
$bothText=Get-Content -LiteralPath $legacyBoth -Raw
foreach($setting in @('Refill=F12','AllWeapons=F5','ToggleThirdPerson=F7','ToggleUnitView=F9','Speed=F11')) {
    if(!$bothText.Contains($setting)){throw "Combined shortcut migration lost preference/default: $setting"}
}
# New arsenal/AI keys merge into custom profiles without moving a saved U action or replacing paid values.
$arsenal=Join-Path $folder 'custom-arsenal.ini'
@('[Shortcuts]','Pickup=U','[Deagle]','Price=777','Magazine=13','[Glock]','SecondaryBurstRounds=2','SecondaryBurstInterval=0.08','[Shotguns]','ShellByShellReload=false','InterruptReloadOnFire=false','[ArmyAI]','Workers=9') | Set-Content -LiteralPath $arsenal -Encoding ascii
Update-GameplayConfig $template $arsenal
$arsenalText=Get-Content -LiteralPath $arsenal -Raw
foreach($setting in @('Pickup=U','ArmyAI=NONE','Price=777','Magazine=13','Workers=9','[M249]','[XM1014]','SpreadDegrees=5','[Shops]','Radius=450','SecondaryBurstRounds=2','SecondaryBurstInterval=0.08','SecondaryCooldown=0.5','ShellByShellReload=false','InterruptReloadOnFire=false')) {
    if(!$arsenalText.Contains($setting)){throw "Arsenal/AI upgrade lost preference/default: $setting"}
}
$arsenalHash=(Get-FileHash -LiteralPath $arsenal).Hash
Update-GameplayConfig $template $arsenal
if((Get-FileHash -LiteralPath $arsenal).Hash -ne $arsenalHash){throw 'Arsenal/AI config migration is not idempotent.'}

# Visual preferences survive upgrades while new siblings are inserted only once.
$viewmodel=Join-Path $folder 'custom-viewmodel.ini'
@('[Viewmodel]','BobEnabled=false','BobAmount=0.7','MuzzleFlash=false','MuzzleFlashMilliseconds=90') | Set-Content -LiteralPath $viewmodel -Encoding ascii
Update-GameplayConfig $template $viewmodel
$visualText=Get-Content -LiteralPath $viewmodel -Raw
foreach($setting in @('BobEnabled=false','BobAmount=0.7','MuzzleFlash=false','MuzzleFlashMilliseconds=90','BobFrequency=1.8','BobResponse=12','MuzzleFlashScale=1')) {
    if(!$visualText.Contains($setting)){throw "Viewmodel migration lost preference/default: $setting"}
}
if([regex]::Matches($visualText,'(?m)^\[Viewmodel\]').Count -ne 1){throw 'Duplicate viewmodel section.'}
$visualHash=(Get-FileHash -LiteralPath $viewmodel).Hash
Update-GameplayConfig $template $viewmodel
if((Get-FileHash -LiteralPath $viewmodel).Hash -ne $visualHash){throw 'Viewmodel migration is not idempotent.'}

# Grenade updates preserve saved effect/carry/price preferences and insert every new sibling once.
$grenades=Join-Path $folder 'custom-grenades.ini'
@('[Grenades]','Enabled=false','FuseSeconds=2.25','WorldModelScale=1.25','WorldSpinDegrees=180','[HEGrenade]','Price=777','Capacity=4','[Flashbang]','UnitStunSeconds=0','[SmokeGrenade]','DurationSeconds=7') | Set-Content -LiteralPath $grenades -Encoding ascii
Update-GameplayConfig $template $grenades
$grenadeText=Get-Content -LiteralPath $grenades -Raw
foreach($setting in @('Enabled=false','FuseSeconds=2.25','WorldModelScale=1.25','WorldSpinDegrees=180','Price=777','Capacity=4','UnitStunSeconds=0','DurationSeconds=7','ThrowSpeed=1125','AmmoPrice=300','BlindSeconds=3','Opacity=0.85')) {
    if(!$grenadeText.Contains($setting)){throw "Grenade migration lost preference/default: $setting"}
}
foreach($section in @('Grenades','HEGrenade','Flashbang','SmokeGrenade')) {
    if([regex]::Matches($grenadeText,'(?m)^\['+$section+'\]').Count -ne 1){throw 'Duplicate grenade section.'}
}
$grenadeHash=(Get-FileHash -LiteralPath $grenades).Hash
Update-GameplayConfig $template $grenades
if((Get-FileHash -LiteralPath $grenades).Hash -ne $grenadeHash){throw 'Grenade migration is not idempotent.'}

# Shared reach upgrades old firearm ranges without rewriting their saved values; an explicit opt-out survives.
foreach($saved in @('', 'MinimumFirearmRange=0', 'MinimumFirearmRange=42000')) {
    $reachConfig=Join-Path $folder 'custom-firearm-range.ini'
    @('[Combat]',$saved,'[M3]','Range=1200') | Set-Content -LiteralPath $reachConfig -Encoding ascii
    Update-GameplayConfig $template $reachConfig
    $reachText=Get-Content -LiteralPath $reachConfig -Raw
    $expected=if($saved){$saved}else{'MinimumFirearmRange=30000'}
    if(!$reachText.Contains($expected) -or !$reachText.Contains('Range=1200') -or
        [regex]::Matches($reachText,'(?m)^MinimumFirearmRange=').Count -ne 1){throw 'Firearm reach migration lost/duplicated a preference.'}
    $reachHash=(Get-FileHash -LiteralPath $reachConfig).Hash
    Update-GameplayConfig $template $reachConfig
    if((Get-FileHash -LiteralPath $reachConfig).Hash -ne $reachHash){throw 'Firearm reach migration is not idempotent.'}
}

# New selector defaults fill older installs; custom opt-outs/timeouts survive repeated upgrades.
foreach($custom in @($false,$true)) {
    $selectorConfig=Join-Path $folder ('weapon-selector-'+$custom+'.ini')
    @('[WeaponSelection]',$(if($custom){'Enabled=false';'TimeoutSeconds=9'}),'[Inventory]','NumbersByDefault=false') | Set-Content -LiteralPath $selectorConfig -Encoding ascii
    Update-GameplayConfig $template $selectorConfig
    $selectorText=Get-Content -LiteralPath $selectorConfig -Raw
    $enabled=if($custom){'Enabled=false'}else{'Enabled=true'}
    $timeout=if($custom){'TimeoutSeconds=9'}else{'TimeoutSeconds=5'}
    if(!$selectorText.Contains($enabled) -or !$selectorText.Contains($timeout) -or !$selectorText.Contains('NumbersByDefault=false')){throw 'Weapon selector migration lost defaults/custom controls.'}
    $selectorHash=(Get-FileHash $selectorConfig).Hash
    Update-GameplayConfig $template $selectorConfig
    if((Get-FileHash $selectorConfig).Hash -ne $selectorHash){throw 'Weapon selector migration is not idempotent.'}
}

# Migrate only the complete old stock grenade pair; custom trajectories and versioned pairs are preserved.
foreach($case in @(@('750','800','','1125','660'),@('750.0','800.0','1','1125','660'),@('1000','800','','1000','800'),@('750','500','','750','500'),@('750','800','2','750','800'))) {
    $trajectoryConfig=Join-Path $folder ('trajectory-'+[guid]::NewGuid().ToString()+'.ini')
    @('[Grenades]',('ThrowSpeed='+$case[0]),('Gravity='+$case[1]),$(if($case[2]){'TrajectoryVersion='+$case[2]}),'InheritedVelocity=0.5','[Flashbang]','Capacity=4') | Set-Content -LiteralPath $trajectoryConfig -Encoding ascii
    Update-GameplayConfig $template $trajectoryConfig
    $trajectoryText=Get-Content -LiteralPath $trajectoryConfig -Raw
    foreach($expected in @(('ThrowSpeed='+$case[3]),('Gravity='+$case[4]),'TrajectoryVersion=2','LightGravityScale=0.9090909','InheritedVelocity=0.5','Capacity=4')) {
        if(!$trajectoryText.Contains($expected)){throw ('Trajectory migration lost '+$expected)}
    }
    $trajectoryHash=(Get-FileHash $trajectoryConfig).Hash;Update-GameplayConfig $template $trajectoryConfig
    if((Get-FileHash $trajectoryConfig).Hash -ne $trajectoryHash){throw 'Trajectory migration must be idempotent.'}
}
