param([Parameter(Mandatory=$true)][string]$VersionDll)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$trial=Join-Path $root ('.local/profile-installer-'+[guid]::NewGuid().ToString('N'))
# Run the real client/setup/copy/config path with synthetic owned files. Only dependency acquisition,
# compilation and embedded module verification are replaced; CS conversion must never be reached.
$source=Join-Path $trial 'source';$owned=Join-Path $trial 'Owned Warcraft'
foreach($folder in @('setup','launcher','tools','config')){New-Item -ItemType Directory -Path (Join-Path $source $folder) -Force | Out-Null}
Copy-Item -Path (Join-Path $root 'setup/*.ps1') -Destination (Join-Path $source 'setup')
foreach($name in @('install-client.ps1','client-validation.ps1')){Copy-Item -LiteralPath (Join-Path $root ('launcher/'+$name)) -Destination (Join-Path $source 'launcher')}
Copy-Item -LiteralPath (Join-Path $root 'tools/sword-model.ps1') -Destination (Join-Path $source 'tools')
Copy-Item -LiteralPath (Join-Path $root 'config/WarcraftCS.ini') -Destination (Join-Path $source 'config')
Copy-Item -LiteralPath (Join-Path $root 'VERSION') -Destination $source
New-Item -ItemType Directory -Path (Join-Path $owned 'redist/miles') -Force | Out-Null
foreach($name in @('war3.exe','Mss32.dll','Storm.dll','War3.mpq','War3x.mpq','War3Patch.mpq','War3xlocal.mpq')){Set-Content -LiteralPath (Join-Path $owned $name) -Value 'synthetic owner file'}
foreach($name in @('Mssfast.m3d','Mp3dec.asi','Reverb3.flt')){Set-Content -LiteralPath (Join-Path $owned ('redist/miles/'+$name)) -Value 'synthetic codec'}
Copy-Item -LiteralPath $VersionDll -Destination (Join-Path $owned 'Game.dll')
# Keep ordinary-path protection from the actual prebuilt adapter for runtime copying.
Add-Content -LiteralPath (Join-Path $source 'setup/prebuilt-runtime.ps1') -Value @'
function Expand-LauncherRuntime { }
function Assert-PrebuiltHost { }
function Install-PrebuiltRuntime($Prebuilt,$Runtime,$Warcraft) {
    New-Item -ItemType Directory -Path (Join-Path $Runtime 'WarcraftCS/notices') -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $Runtime 'WarcraftCS.mix') -Value 'synthetic native module'
}
'@
Set-Content -LiteralPath (Join-Path $source 'launcher/client-dependencies.ps1') -Value @'
function Get-ClientPython { if($script:mode -eq 'Player'){throw 'Player downloaded Python'};return 'synthetic-python' }
function Install-ClientBuildTools { }
'@
Set-Content -LiteralPath (Join-Path $source 'tools/export_models.py') -Value "raise RuntimeError('Conversion reached')"
Set-Content -LiteralPath (Join-Path $source 'tools/fetch-dependencies.ps1') -Value '# synthetic dependency adapter'
Set-Content -LiteralPath (Join-Path $source 'tools/build.ps1') -Value @'
param($OutputDirectory,$MinHookDirectory,$PythonExecutable)
if($PythonExecutable -ne 'synthetic-python'){throw 'Developer Python not supplied'}
. (Join-Path (Split-Path -Parent $PSScriptRoot) 'setup/gameplay-config.ps1')
New-Item -ItemType Directory -Path (Join-Path $OutputDirectory 'WarcraftCS') -Force | Out-Null
Update-GameplayConfig (Join-Path (Split-Path -Parent $PSScriptRoot) 'config/WarcraftCS.ini') (Join-Path $OutputDirectory 'WarcraftCS/WarcraftCS.ini')
'@
foreach($mode in @('Player','Developer')) {
    $install=Join-Path $trial $mode
    New-Item -ItemType Directory -Path $install -Force | Out-Null
    $request=Join-Path $install 'request.json'
    @{WarcraftDirectory=$owned;CounterStrikeDirectory='';InstallDirectory=$install;InstallMode=$mode;WarcraftOnly=$true;LauncherExecutable=$VersionDll} |
        ConvertTo-Json | Set-Content -LiteralPath $request -Encoding utf8
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $source 'launcher/install-client.ps1') -RequestFile $request -DownloadConsent
    if($LASTEXITCODE){throw "Unit-only $mode setup failed"}
    # A failed remote setup report must still identify the selected profile and execution mode.
    if(!(Get-Content -LiteralPath (Join-Path $install 'install.log') -Raw).Contains("Installation request: mode=$mode warcraftOnly=True")){throw 'Install profile diagnostic missing'}
    $config=Join-Path $install 'Game/WarcraftCS/WarcraftCS.ini'
    if(!(Get-Content -LiteralPath $config -Raw).Contains('WarcraftOnly=true')){throw 'Runtime profile missing'}
    if(Test-Path -LiteralPath (Join-Path $install 'Game/WarcraftCS/assets')){throw 'CS cache created'}
    if(Test-Path -LiteralPath (Join-Path $source '.local/venv')){throw 'Conversion environment created'}
    if((Get-Content -LiteralPath (Join-Path $install 'client-installed.json') -Raw | ConvertFrom-Json).warcraft_only -ne 'True'){throw 'Installed profile not remembered'}
}
Write-Output 'PASS real Player/Developer client setup with no CS path, no conversion/NumPy and no Player Python.'
