$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'setup/gameplay-config.ps1')
$folder=Join-Path $root ('.local/config-merge-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $folder -Force | Out-Null
$template=Join-Path $root 'config/WarcraftCS.ini'
$config=Join-Path $folder 'legacy.ini'
@('[Runes]','AmmoPercent=37','[AK47]','Price=0','Damage=81','[Damage]','FriendlyFirePercent=25') | Set-Content -LiteralPath $config -Encoding ascii
# Updates append missing keys inside existing sections while retaining all custom values.
Update-GameplayConfig $template $config
$text=Get-Content -LiteralPath $config -Raw
foreach ($required in @('AmmoPercent=37','Price=0','Damage=81','FriendlyFirePercent=25','AWPOneShot=false','BombCount=20','Access=anywhere','AmmoPrice=80','CSVolumePercent=100','Detailed=true','IntervalMs=1000','MaxFileMB=8','ArchiveCount=3','JumpBoostPercent=0','AutoJump=true')) {
    if (!$text.Contains($required)) { throw "Missing/patched preference: $required" }
}
if ([regex]::Matches($text,'(?m)^\[AK47\]').Count -ne 1) { throw 'Duplicate section can hide new INI keys.' }
if ([regex]::Matches($text,'(?m)^\[Audio\]').Count -ne 1) { throw 'Audio migration duplicated or omitted its section.' }
$before=(Get-FileHash -LiteralPath $config).Hash
Update-GameplayConfig $template $config
if ((Get-FileHash -LiteralPath $config).Hash -ne $before) { throw 'Repeated config update changed an already merged file.' }
# Existing CS-volume preferences survive both updates and repeated setup; missing sibling settings still migrate.
$customAudio=Join-Path $folder 'custom-audio.ini'
@('[Audio]','CSVolumePercent=35','[Runes]','AmmoPercent=11') | Set-Content -LiteralPath $customAudio -Encoding ascii
Update-GameplayConfig $template $customAudio
$audioText=Get-Content -LiteralPath $customAudio -Raw
if (!$audioText.Contains('CSVolumePercent=35') -or !$audioText.Contains('AmmoPercent=11') -or !$audioText.Contains('BombCount=20')) {
    throw 'Audio update overwrote preferences or omitted newly migrated defaults.'
}
if ([regex]::Matches($audioText,'(?m)^\[Audio\]').Count -ne 1 -or [regex]::Matches($audioText,'(?m)^CSVolumePercent=').Count -ne 1) {
    throw 'Custom audio setting was duplicated.'
}
$audioBefore=(Get-FileHash -LiteralPath $customAudio).Hash
Update-GameplayConfig $template $customAudio
if ((Get-FileHash -LiteralPath $customAudio).Hash -ne $audioBefore) { throw 'Repeated audio migration changed the config.' }
$empty=Join-Path $folder 'empty.ini'
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
if (!$text.Contains('DisableFogOfWar=false')) { throw 'Legacy config lacks the default fog override key.' }
# Movement upgrades retain personal jump tuning and add missing siblings exactly once.
$customMovement=Join-Path $folder 'custom-movement.ini'
@('[Movement]','JumpBoostPercent=15','AutoJump=false','[Damage]','FriendlyFirePercent=0') | Set-Content -LiteralPath $customMovement -Encoding ascii
Update-GameplayConfig $template $customMovement
$movementText=Get-Content -LiteralPath $customMovement -Raw
foreach ($required in @('JumpBoostPercent=15','AutoJump=false','FriendlyFirePercent=0','MaxBunnySpeed=1000','Gravity=800','BunnyHop=true')) {
    if (!$movementText.Contains($required)) { throw "Movement migration replaced or omitted: $required" }
}
if ([regex]::Matches($movementText,'(?m)^\[Movement\]').Count -ne 1) { throw 'Movement section was duplicated.' }
$movementBefore=(Get-FileHash -LiteralPath $customMovement).Hash
Update-GameplayConfig $template $customMovement
if ((Get-FileHash -LiteralPath $customMovement).Hash -ne $movementBefore) { throw 'Movement migration is not idempotent.' }
[IO.File]::WriteAllText($empty,'');Update-GameplayConfig $template $empty
if (!(Get-Content -LiteralPath $empty -Raw).Contains('BombCount=20')) { throw 'Empty config repair failed.' }
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
