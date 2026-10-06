param([Parameter(Mandatory=$true)][string]$RequestFile, [switch]$DownloadConsent)
$ErrorActionPreference='Stop'
# This gate runs before extraction, filesystem changes, network requests or dependency installers.
if (!$DownloadConsent) { throw 'Download/install consent is required. Nothing was installed.' }
$OutputEncoding=[Console]::OutputEncoding=[Text.UTF8Encoding]::new($false)
# The EXE writes UTF-8 JSON without a BOM; Windows PowerShell must not read Unicode paths as ANSI.
$request=Get-Content -LiteralPath $RequestFile -Raw -Encoding UTF8 | ConvertFrom-Json
$source=Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'client-dependencies.ps1')
. (Join-Path $PSScriptRoot 'client-validation.ps1')
$paths=Get-ClientPaths $request
$runtime=Join-Path $paths.install 'Game'
Assert-ClientGames $paths.warcraft $paths.cstrike
if (Test-Path -LiteralPath $runtime) {
    foreach ($process in @(Get-Process war3 -ErrorAction SilentlyContinue)) {
        if (!$process.Path -or [IO.Path]::GetDirectoryName($process.Path) -eq $runtime) {
            throw 'Close Warcraft in this client installation before updating it. Save your match first.'
        }
    }
}
New-Item -ItemType Directory -Path $paths.install -Force | Out-Null
$log=Join-Path $paths.install 'install.log'
Start-Transcript -LiteralPath $log -Append | Out-Null
try {
    $ready=Join-Path $paths.install 'client-installed.json'
    $backup=Join-Path $paths.install 'client-installed.previous.json'
    # Source revisions live in separate folders: carry the owner's private sword choice into the new setup.
    $swordModel=''
    # Keep the selection available on retries after setup fails and removes the Play-ready marker.
    $state=if (Test-Path -LiteralPath $ready) { $ready } else { $backup }
    if (Test-Path -LiteralPath $state) {
        $previous=Get-Content -LiteralPath $state -Raw -Encoding UTF8 | ConvertFrom-Json
        $swordModel=$previous.sword_model
        if (!$swordModel -and $previous.source) {
            $oldSetup=Join-Path $previous.source '.local/setup.json'
            $sourceRoot=[IO.Path]::GetFullPath((Join-Path $paths.install 'sources'))+'\'
            if ([IO.Path]::GetFullPath($oldSetup).StartsWith($sourceRoot,[StringComparison]::OrdinalIgnoreCase) -and (Test-Path -LiteralPath $oldSetup)) {
                $swordModel=(Get-Content -LiteralPath $oldSetup -Raw -Encoding UTF8 | ConvertFrom-Json).sword_model
            }
        }
    }
    # A failed update must not leave Play enabled against partially replaced assets or binaries.
    if (Test-Path -LiteralPath $ready) {
        Copy-Item -LiteralPath $ready -Destination $backup -Force
        Remove-Item -LiteralPath $ready -Force
    }
    Write-Output 'Checking local dependencies...'
    $python=Get-ClientPython $paths.install $request.PythonExecutable
    Install-ClientBuildTools $paths.install
    # Reuse the same owned-game setup pipeline as source users; no prebuilt game DLLs are shipped.
    & (Join-Path $source 'setup/setup.ps1') -WarcraftDirectory $paths.warcraft -CounterStrikeDirectory $paths.cstrike -RuntimeDirectory $runtime -PythonExecutable $python -SwordModel $swordModel
    if ($LASTEXITCODE) { throw 'Game setup failed.' }
    @{source=$source;runtime=$runtime;version=(Get-Content (Join-Path $source 'VERSION') -Raw).Trim();revision=(Split-Path -Leaf $source);sword_model=$swordModel} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $paths.install 'client-installed.json') -Encoding utf8
    Write-Output 'Installation complete. You can now press Play.'
} finally { Stop-Transcript | Out-Null }
