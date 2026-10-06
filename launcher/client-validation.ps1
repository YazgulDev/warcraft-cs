# Accept either cstrike itself or its Half-Life parent, while keeping generated files outside both games.
. (Join-Path (Split-Path -Parent $PSScriptRoot) 'setup/audio-runtime.ps1')
function Get-ClientPaths($Request) {
    $warcraft=(Resolve-Path -LiteralPath $Request.WarcraftDirectory).Path.TrimEnd('\')
    $cstrike=(Resolve-Path -LiteralPath $Request.CounterStrikeDirectory).Path.TrimEnd('\')
    if (Test-Path -LiteralPath (Join-Path $cstrike 'cstrike/models/v_knife.mdl')) { $cstrike=Join-Path $cstrike 'cstrike' }
    $install=[IO.Path]::GetFullPath($Request.InstallDirectory).TrimEnd('\')
    foreach ($game in @($warcraft,$cstrike)) {
        if ($install -eq $game -or $install.StartsWith($game+'\',[StringComparison]::OrdinalIgnoreCase)) {
            throw 'Choose a client installation folder outside the original Warcraft and CS folders.'
        }
    }
    if ($install -eq [IO.Path]::GetPathRoot($install).TrimEnd('\')) { throw 'Do not install into a drive root.' }
    return @{warcraft=$warcraft;cstrike=$cstrike;install=$install}
}
function Assert-ClientGames([string]$Warcraft,[string]$CounterStrike) {
    foreach ($name in @('war3.exe','Game.dll','Mss32.dll','Storm.dll','War3.mpq','War3x.mpq','War3Patch.mpq','War3xlocal.mpq')) {
        if (!(Test-Path -LiteralPath (Join-Path $Warcraft $name) -PathType Leaf)) { throw "Missing Warcraft file: $name" }
    }
    $version=(Get-Item -LiteralPath (Join-Path $Warcraft 'Game.dll')).VersionInfo
    if ($version.FileMajorPart -ne 1 -or $version.FileMinorPart -ne 26 -or $version.FileBuildPart -ne 0 -or $version.FilePrivatePart -ne 6401) { throw 'Only Warcraft III 1.26a (Game.dll 1.26.0.6401) is supported.' }
    # Reject missing Miles codecs/providers before the launcher downloads or installs dependencies.
    Assert-WarcraftAudioRuntime $Warcraft
    foreach ($weapon in @('ak47','m4a1','usp','awp','knife','c4')) {
        if (!(Test-Path -LiteralPath (Join-Path $CounterStrike "models/v_$weapon.mdl") -PathType Leaf)) { throw "CS folder is required; missing models/v_$weapon.mdl." }
    }
    foreach ($name in @('sound/weapons/knife_slash1.wav','sound/player/pl_step1.wav')) {
        if (!(Test-Path -LiteralPath (Join-Path $CounterStrike $name) -PathType Leaf)) { throw "Missing owned CS sound: $name" }
    }
}
