# Warcraft III host builds whose Game.dll identity this project ships addresses for.
# These rows mirror wc3::GameBuild::SelectBuild(): a Game.dll the launcher accepts must
# also be one the installed module binds to, otherwise the hooks would silently stand
# down and the game would run unmodified. Support records how much of the runtime is
# known to work on that host, so every caller discloses it instead of over-claiming.
function Get-WarcraftHostBuilds {
    @(
        [pscustomobject]@{Version='1.26.0.6401';Major=1;Minor=26;Build=0;PrivatePart=6401;Support='Complete'}
        [pscustomobject]@{Version='1.27.0.52240';Major=1;Minor=27;Build=0;PrivatePart=52240;Support='Experimental'}
    )
}

function Get-WarcraftHostVersionList {
    # One readable list so no caller re-states the supported builds in its own message.
    return ((Get-WarcraftHostBuilds | ForEach-Object { $_.Version }) -join ' or ')
}

function Find-WarcraftHostBuild($VersionInfo) {
    if (!$VersionInfo) { return $null }
    foreach ($supported in Get-WarcraftHostBuilds) {
        if ($VersionInfo.FileMajorPart -eq $supported.Major -and $VersionInfo.FileMinorPart -eq $supported.Minor -and
            $VersionInfo.FileBuildPart -eq $supported.Build -and $VersionInfo.FilePrivatePart -eq $supported.PrivatePart) {
            return $supported
        }
    }
    return $null
}

function Test-WarcraftHostVersion([string]$Version) {
    # A Player package records its host by name and may only claim a supported one.
    return @((Get-WarcraftHostBuilds | ForEach-Object { $_.Version })) -contains $Version
}

function Get-WarcraftHostBuild([string]$WarcraftDirectory) {
    # Classify a game folder by its Game.dll version resource. Every 1.26a and 1.27a
    # address in the runtime is keyed by this identity, so a repatched build that
    # reuses the same numbers for unrelated functions is refused before any copy.
    $gameDll=Join-Path $WarcraftDirectory 'Game.dll'
    if (!(Test-Path -LiteralPath $gameDll -PathType Leaf)) { throw 'Missing Warcraft file: Game.dll' }
    $version=(Get-Item -LiteralPath $gameDll).VersionInfo
    $supported=Find-WarcraftHostBuild $version
    if (!$supported) {
        $reported=$version.FileMajorPart.ToString()+'.'+$version.FileMinorPart.ToString()+'.'+$version.FileBuildPart.ToString()+'.'+$version.FilePrivatePart.ToString()
        throw ("Unsupported Warcraft III host: Game.dll reports $reported. Supported builds: "+(Get-WarcraftHostVersionList)+'.')
    }
    return $supported
}

function Assert-WarcraftHostBuild([string]$WarcraftDirectory) {
    # Announce the accepted host once per installation decision; setup journals carry the line.
    $supported=Get-WarcraftHostBuild $WarcraftDirectory
    Write-Output ("Warcraft III host: Game.dll $($supported.Version) ($($supported.Support) module support).")
}
