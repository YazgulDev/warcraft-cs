# Accept either cstrike itself or its Half-Life parent, while keeping generated files outside both games.
. (Join-Path (Split-Path -Parent $PSScriptRoot) 'setup/audio-runtime.ps1')
. (Join-Path (Split-Path -Parent $PSScriptRoot) 'setup/host-build.ps1')
function Get-ClientPaths($Request) {
    $warcraft=(Resolve-Path -LiteralPath $Request.WarcraftDirectory).Path.TrimEnd('\')
    # An unchecked CS requirement must not resolve a missing/previously removed game folder.
    $cstrike=''
    if (!$Request.WarcraftOnly) {
        $cstrike=(Resolve-Path -LiteralPath $Request.CounterStrikeDirectory).Path.TrimEnd('\')
        if (Test-Path -LiteralPath (Join-Path $cstrike 'cstrike/models/v_knife.mdl')) { $cstrike=Join-Path $cstrike 'cstrike' }
    }
    $install=[IO.Path]::GetFullPath($Request.InstallDirectory).TrimEnd('\')
    foreach ($game in @($warcraft,$cstrike)) {
        if (!$game) { continue }
        if ($install -eq $game -or $install.StartsWith($game+'\',[StringComparison]::OrdinalIgnoreCase)) {
            throw 'Choose a client installation folder outside the original Warcraft and CS folders.'
        }
    }
    if ($install -eq [IO.Path]::GetPathRoot($install).TrimEnd('\')) { throw 'Do not install into a drive root.' }
    return @{warcraft=$warcraft;cstrike=$cstrike;install=$install}
}
function Assert-ClientGames([string]$Warcraft,[string]$CounterStrike,[bool]$WarcraftOnly=$false) {
    foreach ($name in @('war3.exe','Game.dll','Mss32.dll','Storm.dll','War3.mpq','War3x.mpq','War3Patch.mpq','War3xlocal.mpq')) {
        if (!(Test-Path -LiteralPath (Join-Path $Warcraft $name) -PathType Leaf)) { throw "Missing Warcraft file: $name" }
    }
    # Keep client preflight and manual setup aligned with native address selection.
    Assert-WarcraftHostBuild $Warcraft
    # Reject missing Miles codecs/providers before the launcher downloads or installs dependencies.
    Assert-WarcraftAudioRuntime $Warcraft
    if ($WarcraftOnly) { return } # Native unit modes use the installed Warcraft audio and art.
    foreach ($weapon in @('ak47','m4a1','usp','awp','knife','c4')) {
        if (!(Test-Path -LiteralPath (Join-Path $CounterStrike "models/v_$weapon.mdl") -PathType Leaf)) { throw "CS folder is required; missing models/v_$weapon.mdl." }
    }
    foreach ($name in @('sound/weapons/knife_slash1.wav','sound/player/pl_step1.wav')) {
        if (!(Test-Path -LiteralPath (Join-Path $CounterStrike $name) -PathType Leaf)) { throw "Missing owned CS sound: $name" }
    }
}
