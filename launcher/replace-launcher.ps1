param([Parameter(Mandatory=$true)][string]$RequestFile)
$ErrorActionPreference='Stop'
$request=Get-Content -LiteralPath $RequestFile -Raw -Encoding UTF8 | ConvertFrom-Json
$log=Join-Path (Split-Path -Parent $RequestFile) 'launcher-replacement.log'
try {
    # Wait for the old launcher to release its EXE; never kill the launcher or a running game.
    if (Get-Process -Id $request.ParentId -ErrorAction SilentlyContinue) {
        Wait-Process -Id $request.ParentId -Timeout 60 -ErrorAction Stop
    }
    $replacement=[IO.Path]::GetFullPath($request.Replacement)
    $target=[IO.Path]::GetFullPath($request.Target)
    if (!(Test-Path -LiteralPath $target -PathType Leaf) -or $replacement -eq $target) { throw 'Invalid launcher replacement paths.' }
    if ((Get-FileHash -LiteralPath $replacement -Algorithm SHA256).Hash -ne $request.Sha256) { throw 'Replacement checksum mismatch.' }
    # Replace atomically on the target volume and retain the previous launcher for recovery.
    $staged=$target+'.update-'+[guid]::NewGuid().ToString('N')+'.exe'
    Copy-Item -LiteralPath $replacement -Destination $staged
    if ((Get-FileHash -LiteralPath $staged -Algorithm SHA256).Hash -ne $request.Sha256) { throw 'Staged launcher checksum mismatch.' }
    [IO.File]::Replace($staged,$target,$target+'.previous', $true)
    Start-Process -FilePath $target -WorkingDirectory (Split-Path -Parent $target) -WindowStyle Normal | Out-Null
    'Launcher updated and restarted.' | Set-Content -LiteralPath $log
} catch {
    $_.Exception.Message | Set-Content -LiteralPath $log
    # A read-only launch directory can still use the verified downloaded launcher.
    if (Test-Path -LiteralPath $request.Replacement -PathType Leaf) {
        if ((Get-FileHash -LiteralPath $request.Replacement -Algorithm SHA256).Hash -eq $request.Sha256) {
            Start-Process -FilePath $request.Replacement -WindowStyle Normal | Out-Null
        }
    }
    exit 1
}
