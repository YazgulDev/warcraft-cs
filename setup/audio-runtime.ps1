# Miles loads its providers/codecs from redist/miles; Mss32.dll alone cannot initialize Warcraft audio.
function Assert-WarcraftAudioRuntime([string]$WarcraftDirectory) {
    foreach ($name in @('Mssfast.m3d','Mp3dec.asi','Reverb3.flt')) {
        if (!(Test-Path -LiteralPath (Join-Path $WarcraftDirectory "redist/miles/$name") -PathType Leaf)) {
            throw "Missing owned Warcraft audio file: redist/miles/$name. Select a complete Warcraft III installation."
        }
    }
}

function Copy-WarcraftAudioRuntime([string]$WarcraftDirectory,[string]$RuntimeDirectory) {
    Assert-WarcraftAudioRuntime $WarcraftDirectory
    $original=[IO.Path]::GetFullPath($WarcraftDirectory).TrimEnd('\')
    $runtime=[IO.Path]::GetFullPath($RuntimeDirectory).TrimEnd('\')
    if ($runtime -eq $original -or $runtime.StartsWith($original+'\',[StringComparison]::OrdinalIgnoreCase) -or
        !(Test-Path -LiteralPath (Join-Path $runtime '.warcraft-cs-private-runtime') -PathType Leaf)) {
        throw 'Audio setup requires a marked private runtime outside the original Warcraft installation.'
    }
    # Repair previously installed runtimes too; preserve saves/settings and copy only owned audio components.
    $destination=Join-Path $runtime 'redist/miles'
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $original 'redist/miles') -File -Force |
        Where-Object { $_.Extension -in @('.m3d','.asi','.flt','.dll') } |
        ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $destination $_.Name) -Force }
}
