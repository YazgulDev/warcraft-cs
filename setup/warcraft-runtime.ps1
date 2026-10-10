. (Join-Path $PSScriptRoot 'prebuilt-runtime.ps1')

function Copy-WarcraftRuntime([string]$WarcraftDirectory,[string]$RuntimeDirectory) {
    $original=(Resolve-Path -LiteralPath $WarcraftDirectory).Path.TrimEnd('\')
    $runtime=[IO.Path]::GetFullPath($RuntimeDirectory).TrimEnd('\')
    if ($runtime -eq [IO.Path]::GetPathRoot($runtime).TrimEnd('\') -or $runtime -eq $original -or
        $runtime.StartsWith($original+'\',[StringComparison]::OrdinalIgnoreCase) -or
        $original.StartsWith($runtime+'\',[StringComparison]::OrdinalIgnoreCase)) {
        throw 'Runtime repair requires a separate private directory outside the original game.'
    }
    Assert-OrdinaryRuntimePath $runtime
    $marker=Join-Path $runtime '.warcraft-cs-private-runtime'
    Assert-OrdinaryRuntimePath $marker
    if ((Test-Path -LiteralPath $runtime) -and !(Test-Path -LiteralPath $marker -PathType Leaf)) {
        throw "Runtime directory already exists without a setup marker: $runtime"
    }
    # Never refresh game files beneath an active match, including when its process path is inaccessible.
    foreach ($process in @(Get-Process war3 -ErrorAction SilentlyContinue)) {
        if (!$process.Path -or [IO.Path]::GetDirectoryName($process.Path).TrimEnd('\') -eq $runtime) {
            throw 'Close Warcraft in this runtime before reinstalling; save your match first.'
        }
    }
    foreach ($name in @('war3.exe','Game.dll','Mss32.dll','Storm.dll','War3.mpq','War3x.mpq','War3Patch.mpq','War3xlocal.mpq')) {
        if (!(Test-Path -LiteralPath (Join-Path $original $name) -PathType Leaf)) { throw "Missing owned Warcraft file: $name" }
    }
    $plan=@(Get-ChildItem -LiteralPath $original -File | Where-Object {
        $_.Extension -in @('.exe','.dll','.mpq','.manifest') -and $_.Name -notlike 'unins*' -and
        $_.Name -notin @('Mss32.dll','WarcraftOriginalMss.dll')
    } | ForEach-Object { @{source=$_.FullName;relative=$_.Name;backup=$false} })
    # Refresh the original sound library separately; never mistake an installed proxy for the original.
    $sound=Join-Path $original 'WarcraftOriginalMss.dll'
    if (!(Test-Path -LiteralPath $sound -PathType Leaf)) { $sound=Join-Path $original 'Mss32.dll' }
    $plan+=@{source=$sound;relative='WarcraftOriginalMss.dll';backup=$false}
    if (!(Test-Path -LiteralPath (Join-Path $runtime 'Mss32.dll'))) {
        $plan+=@{source=$sound;relative='Mss32.dll';backup=$false}
    }
    foreach ($folder in @('Maps','Campaigns','Movies','AI Scripts')) {
        $content=Join-Path $original $folder
        if (!(Test-Path -LiteralPath $content)) { continue }
        foreach ($file in @(Get-ChildItem -LiteralPath $content -Recurse -File)) {
            $relative=$file.FullName.Substring($original.Length+1)
            # Personal progress is never imported over the private runtime's existing saves.
            if ($file.Extension -in @('.w3z','.w3v') -or $relative -match '(?i)(^|[\\/])(save|saves)([\\/]|$)') { continue }
            $plan+=@{source=$file.FullName;relative=$relative;backup=$true}
        }
    }
    $backupRoot=Join-Path (Split-Path -Parent $runtime) ('backups/runtime-content/'+[guid]::NewGuid().ToString('N'))
    # Validate the entire write plan before touching the target; junctions cannot redirect a repair.
    Assert-OrdinaryRuntimePath $backupRoot
    foreach ($item in $plan) {
        Assert-OrdinaryRuntimePath $item.source
        $target=Join-Path $runtime $item.relative
        Assert-OrdinaryRuntimePath $target
        if (Test-Path -LiteralPath $target -PathType Container) { throw "Expected a game file, found a directory: $target" }
    }
    New-Item -ItemType Directory -Path $runtime -Force | Out-Null
    # A failed copy can be retried only in the marked directory this invocation created/validated.
    if (!(Test-Path -LiteralPath $marker)) {
        Set-Content -LiteralPath $marker -Value 'Private owned Warcraft runtime; do not redistribute.' -Encoding ascii
    }
    Write-Output 'Reinstalling owned Warcraft files; preserving saves, mod settings and private-only content...'
    foreach ($item in $plan) {
        $destination=Join-Path $runtime $item.relative
        # Keep a recoverable copy when refreshing an official map/campaign that was edited locally.
        if ($item.backup -and (Test-Path -LiteralPath $destination -PathType Leaf) -and
            (Get-FileHash -LiteralPath $destination).Hash -ne (Get-FileHash -LiteralPath $item.source).Hash) {
            $backup=Join-Path $backupRoot $item.relative
            New-Item -ItemType Directory -Path (Split-Path -Parent $backup) -Force | Out-Null
            Copy-Item -LiteralPath $destination -Destination $backup
            Write-Output "Previous content backed up: $backup"
        }
        New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
        Copy-Item -LiteralPath $item.source -Destination $destination -Force
        Write-Output "Warcraft file refreshed: $($item.relative)"
    }
    Write-Output "Refreshed $($plan.Count) owned Warcraft files."
}
