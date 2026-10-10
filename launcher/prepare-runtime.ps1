param([Parameter(Mandatory=$true)][string]$RequestFile,[switch]$DownloadConsent)
$ErrorActionPreference='Stop'
if (!$DownloadConsent) { throw 'Download/install consent is required. Nothing was installed.' }
$OutputEncoding=[Console]::OutputEncoding=[Text.UTF8Encoding]::new($false)
$request=Get-Content -LiteralPath $RequestFile -Raw -Encoding UTF8 | ConvertFrom-Json
. (Join-Path $PSScriptRoot 'client-validation.ps1')
. (Join-Path (Split-Path -Parent $PSScriptRoot) 'setup/warcraft-runtime.ps1')
$paths=Get-ClientPaths $request
Assert-ClientGames $paths.warcraft $paths.cstrike
# Old published setup skips existing Game folders. Repair them with this launcher's current policy
# before handing over to that release's verified asset/module installer.
$ready=Join-Path $paths.install 'client-installed.json'
Assert-OrdinaryRuntimePath $ready
Assert-OrdinaryRuntimePath (Join-Path $paths.install 'client-installed.previous.json')
# Keep the compatibility repair in the same installation journal as the selected release's setup.
$log=Join-Path $paths.install 'install.log'
Assert-OrdinaryRuntimePath $log
Start-Transcript -LiteralPath $log -Append | Out-Null
try {
    if (Test-Path -LiteralPath $ready) {
        Copy-Item -LiteralPath $ready -Destination (Join-Path $paths.install 'client-installed.previous.json') -Force
        Remove-Item -LiteralPath $ready
    }
    Copy-WarcraftRuntime $paths.warcraft (Join-Path $paths.install 'Game')
} finally { Stop-Transcript | Out-Null }
