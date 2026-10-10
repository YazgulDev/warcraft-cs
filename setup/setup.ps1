param(
    [Parameter(Mandatory=$true)][string]$WarcraftDirectory,
    [Parameter(Mandatory=$true)][string]$CounterStrikeDirectory,
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
. (Join-Path $root 'tools/sword-model.ps1')
# Keep the owner's Grudge model and full authored animation set across repeated setup runs.
$SwordModel=Get-WarcraftCsSwordModel $root $SwordModel
$warcraft=(Resolve-Path -LiteralPath $WarcraftDirectory).Path
$cstrike=(Resolve-Path -LiteralPath $CounterStrikeDirectory).Path
if (!$RuntimeDirectory) { $RuntimeDirectory=Join-Path $root '.local/warcraft-cs' }
$runtime=[IO.Path]::GetFullPath($RuntimeDirectory).TrimEnd('\')
# Never install a proxy DLL into the user's original game or create a recursively copied child inside it.
if ($runtime -eq $warcraft.TrimEnd('\') -or $runtime.StartsWith($warcraft.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)) {
    throw 'Choose a separate runtime directory outside the original Warcraft install.'
}
foreach ($name in @('war3.exe','Game.dll','Mss32.dll','Storm.dll','War3.mpq','War3x.mpq','War3Patch.mpq','War3xlocal.mpq')) {
    if (!(Test-Path -LiteralPath (Join-Path $warcraft $name))) { throw "Missing owned Warcraft file: $name" }
}
$version=(Get-Item -LiteralPath (Join-Path $warcraft 'Game.dll')).VersionInfo
if ($version.FileMajorPart -ne 1 -or $version.FileMinorPart -ne 26 -or $version.FileBuildPart -ne 0 -or $version.FilePrivatePart -ne 6401) {
    throw 'Only Warcraft III 1.26a x86 (Game.dll 1.26.0.6401) is supported.'
}
# Detect an incomplete owned install before creating a private copy or downloading dependencies.
Assert-WarcraftAudioRuntime $warcraft
# Reject mismatched Player modules before copying a runtime or installing Python dependencies.
if ($InstallMode -eq 'Player') { Assert-PrebuiltHost $PrebuiltDirectory $warcraft }
foreach ($weapon in @('ak47','m4a1','usp','awp','knife','c4')) {
    if (!(Test-Path -LiteralPath (Join-Path $cstrike "models/v_$weapon.mdl"))) { throw "Missing owned CS model v_$weapon.mdl. Supply the cstrike folder with its loose model files." }
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
$venv=Join-Path $root '.local/venv'
if (!(Test-Path -LiteralPath (Join-Path $venv 'Scripts/python.exe'))) {
    & $PythonExecutable -m venv $venv
    if ($LASTEXITCODE) { throw 'Python venv creation failed; Python 3.10+ with venv is required.' }
}
$python=Join-Path $venv 'Scripts/python.exe'
& $python -m pip install -r (Join-Path $root 'requirements.txt')
if ($LASTEXITCODE) { throw 'Could not install the local Python dependency.' }
# Only source developers need MinHook sources and the compiler; Player uses bundled x86 modules.
if ($InstallMode -eq 'Developer') { & (Join-Path $root 'tools/fetch-dependencies.ps1') }
$assets=Join-Path $runtime 'WarcraftCS/assets'
$arguments=@((Join-Path $root 'tools/export_models.py'),'--cstrike',$cstrike,'--output',$assets)
# Convert owned CS hands/animations with the selected private sword or original generated geometry.
if ($SwordModel) { $arguments+=@('--sword-model',$SwordModel) }
& $python @arguments
if ($LASTEXITCODE) { throw 'Private asset conversion failed.' }
if ($InstallMode -eq 'Developer') {
    & (Join-Path $root 'tools/build.ps1') -OutputDirectory $runtime -MinHookDirectory (Join-Path $root '.local/dependencies/minhook') -PythonExecutable $python
} else {
    Install-PrebuiltRuntime $PrebuiltDirectory $runtime $warcraft
    # Match the ordinary build's first-install policy without replacing the owner's customized INI.
    $config=Join-Path $runtime 'WarcraftCS/WarcraftCS.ini'
    Update-GameplayConfig (Join-Path $root 'config/WarcraftCS.ini') $config
}
# Store machine-specific paths outside source control; launch resolves this owner-only configuration.
@{runtime=$runtime;warcraft=$warcraft;cstrike=$cstrike;sword_model=$SwordModel} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root '.local/setup.json') -Encoding utf8
Write-Output 'Setup complete. Run play.cmd. Game files and converted assets must remain private.'
