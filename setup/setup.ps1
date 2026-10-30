param(
    [Parameter(Mandatory=$true)][string]$WarcraftDirectory,
    [string]$CounterStrikeDirectory='',
    [switch]$WarcraftOnly,
    [string]$RuntimeDirectory='',
    [string]$PythonExecutable='python',
    [string]$SwordModel='',
    [ValidateSet('Player','Developer')][string]$InstallMode='Developer',
    [string]$PrebuiltDirectory=''
)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'audio-runtime.ps1')
. (Join-Path $PSScriptRoot 'prebuilt-runtime.ps1')
. (Join-Path $PSScriptRoot 'gameplay-config.ps1')
. (Join-Path $PSScriptRoot 'warcraft-runtime.ps1')
. (Join-Path $PSScriptRoot 'direct3d-runtime.ps1')
. (Join-Path $PSScriptRoot 'host-build.ps1')
. (Join-Path $root 'tools/sword-model.ps1')
# Keep the owner's Grudge model and full authored animation set across repeated setup runs.
if (!$WarcraftOnly) { $SwordModel=Get-WarcraftCsSwordModel $root $SwordModel }
$warcraft=(Resolve-Path -LiteralPath $WarcraftDirectory).Path
$cstrike=if ($WarcraftOnly) { '' } else { (Resolve-Path -LiteralPath $CounterStrikeDirectory).Path }
if (!$RuntimeDirectory) { $RuntimeDirectory=Join-Path $root '.local/warcraft-cs' }
$runtime=[IO.Path]::GetFullPath($RuntimeDirectory).TrimEnd('\')
# Never install a proxy DLL into the user's original game or create a recursively copied child inside it.
if ($runtime -eq $warcraft.TrimEnd('\') -or $runtime.StartsWith($warcraft.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)) {
    throw 'Choose a separate runtime directory outside the original Warcraft install.'
}
foreach ($name in @('war3.exe','Game.dll','Mss32.dll','Storm.dll','War3.mpq','War3x.mpq','War3Patch.mpq','War3xlocal.mpq')) {
    if (!(Test-Path -LiteralPath (Join-Path $warcraft $name))) { throw "Missing owned Warcraft file: $name" }
}
# Installation accepts the same exact host identities as the native runtime.
Assert-WarcraftHostBuild $warcraft
# Detect an incomplete owned install before creating a private copy or downloading dependencies.
Assert-WarcraftAudioRuntime $warcraft
# Reject mismatched Player modules before copying a runtime or installing Python dependencies.
if ($InstallMode -eq 'Player') { Assert-PrebuiltHost $PrebuiltDirectory $warcraft }
# Preflight all owned weapon/consumable models before modifying a client.
foreach ($weapon in $(if ($WarcraftOnly) { @() } else { @('ak47','m4a1','usp','awp','knife','c4','deagle','glock18','mp5','p90','m249','m3','xm1014','tmp','ump45','famas','galil','g3sg1','hegrenade','flashbang','smokegrenade') })) {
    if (!(Test-Path -LiteralPath (Join-Path $cstrike "models/v_$weapon.mdl"))) { throw "Missing owned CS model v_$weapon.mdl. Supply the cstrike folder with its loose model files." }
    # Real thrown bodies have separate retail geometry/textures from the hands shown in v_ models.
    if ($weapon -in @('hegrenade','flashbang','smokegrenade') -and !(Test-Path -LiteralPath (Join-Path $cstrike "models/w_$weapon.mdl"))) { throw "Missing owned CS world model w_$weapon.mdl." }
}
# A fresh directory cannot be a running game; existing targets must be idle before their DLLs are rebuilt.
if (Test-Path -LiteralPath $runtime) {
    foreach ($process in @(Get-Process war3 -ErrorAction SilentlyContinue)) {
        if (!$process.Path -or [IO.Path]::GetDirectoryName($process.Path).TrimEnd('\') -eq $runtime) {
            throw 'Close Warcraft in this runtime before updating setup; preserve the match first.'
        }
    }
}
# Install and Update refresh the complete owned runtime, including damaged/missing files on retries.
Copy-WarcraftRuntime $warcraft $runtime
# Keep this outside first-install copying so updates repair the missing providers in older clients.
Copy-WarcraftAudioRuntime $warcraft $runtime
New-Item -ItemType Directory -Path (Join-Path $root '.local') -Force | Out-Null
$python=$PythonExecutable
if (!$WarcraftOnly) {
    $venv=Join-Path $root '.local/venv'
    if (!(Test-Path -LiteralPath (Join-Path $venv 'Scripts/python.exe'))) {
        & $PythonExecutable -m venv $venv
        if ($LASTEXITCODE) { throw 'Python venv creation failed; Python 3.10+ with venv is required.' }
    }
    $python=Join-Path $venv 'Scripts/python.exe'
    & $python -m pip install -r (Join-Path $root 'requirements.txt')
    if ($LASTEXITCODE) { throw 'Could not install the local Python dependency.' }
} # Unit-only builds use Python's standard library; Player needs no Python at all.
# Only source developers need MinHook sources and the compiler; Player uses bundled x86 modules.
if ($InstallMode -eq 'Developer') { & (Join-Path $root 'tools/fetch-dependencies.ps1') }
if (!$WarcraftOnly) {
    $assets=Join-Path $runtime 'WarcraftCS/assets'
    $arguments=@((Join-Path $root 'tools/export_models.py'),'--cstrike',$cstrike,'--output',$assets)
    # Convert owned CS hands/animations with the selected private sword or original generated geometry.
    if ($SwordModel) { $arguments+=@('--sword-model',$SwordModel) }
    & $python @arguments
    if ($LASTEXITCODE) { throw 'Private asset conversion failed.' }
}
if ($InstallMode -eq 'Developer') {
    & (Join-Path $root 'tools/build.ps1') -OutputDirectory $runtime -MinHookDirectory (Join-Path $root '.local/dependencies/minhook') -PythonExecutable $python
} else {
    Install-PrebuiltRuntime $PrebuiltDirectory $runtime $warcraft
    # Match the ordinary build's first-install policy without replacing the owner's customized INI.
    $config=Join-Path $runtime 'WarcraftCS/WarcraftCS.ini'
    Update-GameplayConfig (Join-Path $root 'config/WarcraftCS.ini') $config
}
# Both renderers are ready after installation, so switching later is offline and requires no extra setup.
if((Get-WarcraftHostBuild $runtime).Minor -eq 26) {
    $graphics=$PrebuiltDirectory
    if($InstallMode -eq 'Developer') {
        $graphics=Join-Path $root '.local/dependencies/direct3d'
        & (Join-Path $root 'tools/fetch-direct3d.ps1') -Destination $graphics
    }
    Install-Direct3DRuntime $graphics $runtime
}
# The explicit launcher choice replaces only this profile key, preserving all gameplay customization.
. (Join-Path $PSScriptRoot 'installation-profile.ps1')
Set-InstallationProfile (Join-Path $runtime 'WarcraftCS/WarcraftCS.ini') ([bool]$WarcraftOnly)
# Store machine-specific paths outside source control; launch resolves this owner-only configuration.
@{runtime=$runtime;warcraft=$warcraft;cstrike=$cstrike;sword_model=$SwordModel;warcraft_only=[bool]$WarcraftOnly} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root '.local/setup.json') -Encoding utf8
Write-Output 'Setup complete. Run play.cmd. Game files and converted assets must remain private.'
