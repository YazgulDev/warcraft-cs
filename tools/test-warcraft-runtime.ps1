param([switch]$SkipJunction)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'setup/warcraft-runtime.ps1')
$fixture=Join-Path $root ('build/runtime-reinstall-tests-'+[guid]::NewGuid().ToString('N'))
$original=Join-Path $fixture 'owned'
function Write-Fixture([string]$Path,[string]$Text) {
    New-Item -ItemType Directory -Path (Split-Path -Parent $Path) -Force | Out-Null
    [IO.File]::WriteAllText($Path,$Text)
}
function Require([bool]$Condition,[string]$Message) { if (!$Condition) { throw $Message } }
function Reject([scriptblock]$Action,[string]$Message) {
    $failed=$false;try { & $Action | Out-Null } catch { $failed=$true };Require $failed $Message
}
# Synthetic process discovery exercises the guard without launching/closing the user's actual Warcraft.
$script:runningPath=''
function Get-Process { param($Name,$ErrorAction);if ($script:runningPath) { [pscustomobject]@{Path=$script:runningPath} } }
$names=@('war3.exe','Game.dll','Mss32.dll','Storm.dll','War3.mpq','War3x.mpq','War3Patch.mpq','War3xlocal.mpq')
foreach ($name in $names) { Write-Fixture (Join-Path $original $name) ('owned '+$name) }
Write-Fixture (Join-Path $original 'WarcraftOriginalMss.dll') 'original sound behind source proxy'
Write-Fixture (Join-Path $original 'unins000.exe') 'excluded uninstaller'
Write-Fixture (Join-Path $original 'save/owner.w3z') 'original progress'
foreach ($name in @('Maps/official.w3x','Maps/missing.w3m','Campaigns/official.w3n','Movies/intro.avi','AI Scripts/standard.ai')) {
    Write-Fixture (Join-Path $original $name) ('owned '+$name)
}
Write-Fixture (Join-Path $original 'Maps/save/old.w3v') 'do not import original progress'
$runtime=Join-Path $fixture 'Client/Game'
Copy-WarcraftRuntime $original $runtime | Out-Null
Require ([IO.File]::ReadAllText((Join-Path $runtime 'WarcraftOriginalMss.dll')) -eq 'original sound behind source proxy') 'Proxy became the original sound library.'
Require (!(Test-Path -LiteralPath (Join-Path $runtime 'save/owner.w3z')) -and !(Test-Path -LiteralPath (Join-Path $runtime 'Maps/save/old.w3v'))) 'Original progress was imported.'
Require (!(Test-Path -LiteralPath (Join-Path $runtime 'unins000.exe'))) 'Uninstaller was copied.'
# Damage/delete files in an installed game, retain its own proxy/config/save/private assets, then reinstall.
Write-Fixture (Join-Path $runtime 'Game.dll') 'damaged game DLL'
Write-Fixture (Join-Path $runtime 'WarcraftOriginalMss.dll') 'damaged original audio'
Write-Fixture (Join-Path $runtime 'Mss32.dll') 'installed project proxy'
Write-Fixture (Join-Path $runtime 'Maps/official.w3x') 'edited personal map'
Remove-Item -LiteralPath (Join-Path $runtime 'Storm.dll')
Remove-Item -LiteralPath (Join-Path $runtime 'Maps/missing.w3m')
$personal=@('save/profile.w3z','save/campaign.w3v','WarcraftCS/WarcraftCS.ini','WarcraftCS/assets/custom.wcg','Maps/private.w3x')
foreach ($name in $personal) { Write-Fixture (Join-Path $runtime $name) ('keep '+$name) }
Copy-WarcraftRuntime $original $runtime | Out-Null
foreach ($name in @('Game.dll','Storm.dll','War3.mpq')) {
    Require ([IO.File]::ReadAllText((Join-Path $runtime $name)) -eq ('owned '+$name)) "Missing/corrupt file was not repaired: $name"
}
Require ([IO.File]::ReadAllText((Join-Path $runtime 'Mss32.dll')) -eq 'installed project proxy') 'Reinstall replaced the mod proxy before modules were ready.'
Require ([IO.File]::ReadAllText((Join-Path $runtime 'Maps/missing.w3m')) -eq 'owned Maps/missing.w3m') 'Missing map was not repaired.'
foreach ($name in $personal) { Require ([IO.File]::ReadAllText((Join-Path $runtime $name)) -eq ('keep '+$name)) "Personal data changed: $name" }
$backups=@(Get-ChildItem -LiteralPath (Join-Path $fixture 'Client/backups') -Recurse -File)
Require ($backups.Count -eq 1 -and [IO.File]::ReadAllText($backups[0].FullName) -eq 'edited personal map') 'Edited official map has no recoverable backup.'
Copy-WarcraftRuntime $original $runtime | Out-Null
Require (@(Get-ChildItem -LiteralPath (Join-Path $fixture 'Client/backups') -Recurse -File).Count -eq 1) 'Repeat reinstall duplicated unchanged content backups.'
Require ([IO.File]::ReadAllText((Join-Path $original 'Game.dll')) -eq 'owned Game.dll') 'Original game was modified.'
foreach ($target in @($original,(Join-Path $original 'child'),$fixture)) { Reject { Copy-WarcraftRuntime $original $target } 'Original/overlapping/unmarked runtime accepted.' }
$unmarked=Join-Path $fixture 'Unmarked';Write-Fixture (Join-Path $unmarked 'Game.dll') 'unrelated'
Reject { Copy-WarcraftRuntime $original $unmarked } 'Unmarked directory accepted.'
Require ([IO.File]::ReadAllText((Join-Path $unmarked 'Game.dll')) -eq 'unrelated') 'Unmarked directory changed.'
$script:runningPath=Join-Path $runtime 'war3.exe';Write-Fixture (Join-Path $runtime 'Game.dll') 'active game'
Reject { Copy-WarcraftRuntime $original $runtime } 'Active runtime accepted.'
Require ([IO.File]::ReadAllText((Join-Path $runtime 'Game.dll')) -eq 'active game') 'Active runtime changed.'
$script:runningPath=''
# A junction inside an otherwise valid private runtime must reject the complete write plan up front.
$linked=Join-Path $fixture 'Linked';Write-Fixture (Join-Path $linked '.warcraft-cs-private-runtime') 'private'
Write-Fixture (Join-Path $linked 'Game.dll') 'untouched DLL'
$outside=Join-Path $fixture 'Protected';Write-Fixture (Join-Path $outside 'official.w3x') 'untouched external map'
if (!$SkipJunction) {
    New-Item -ItemType Junction -Path (Join-Path $linked 'Maps') -Value $outside | Out-Null
    Reject { Copy-WarcraftRuntime $original $linked } 'Junction runtime accepted.'
    Require ([IO.File]::ReadAllText((Join-Path $linked 'Game.dll')) -eq 'untouched DLL' -and
        [IO.File]::ReadAllText((Join-Path $outside 'official.w3x')) -eq 'untouched external map') 'Rejected junction repair wrote files.'
    Write-Output 'PASS runtime junction guard.'
}
Write-Output 'PASS full Warcraft reinstall: missing/corrupt game files, maps, original audio, progress/settings preservation, edited-map backups, repeated repair and active/unmarked guards.'
