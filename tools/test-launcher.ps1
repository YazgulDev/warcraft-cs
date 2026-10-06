$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build/launcher'
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$sources=@('ClientRequest.cs','SourcePayload.cs','SetupRunner.cs','LauncherForm.cs') | ForEach-Object { Join-Path $root "launcher/$_" }
$payload=Join-Path $build 'Source.zip'
if (!(Test-Path -LiteralPath $payload)) { throw 'Build the launcher first to create its audited source payload.' }
$tests=Join-Path $build 'LauncherTests.exe'
& $compiler /nologo /target:exe /platform:x64 "/out:$tests" "/resource:$payload,WarcraftCS.Source.zip" /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll /r:System.Web.Extensions.dll /r:System.Windows.Forms.dll /r:System.Drawing.dll (Join-Path $root 'tests/LauncherTests.cs') $sources
if ($LASTEXITCODE) { throw 'Launcher test compilation failed.' }
& $tests (Join-Path $root ('.local/launcher-tests-'+[guid]::NewGuid().ToString('N')))
if ($LASTEXITCODE) { throw 'Launcher regression checks failed.' }
# Parse every script for Windows PowerShell and enforce a backend consent denial before any side effects.
foreach ($script in @(Get-ChildItem -LiteralPath (Join-Path $root 'launcher') -Filter '*.ps1')) {
    $tokens=$null;$errors=$null
    [Management.Automation.Language.Parser]::ParseFile($script.FullName,[ref]$tokens,[ref]$errors) | Out-Null
    if ($errors) { throw "Script parse errors: $($script.Name)" }
}
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root 'launcher/install-client.ps1') -RequestFile 'nonexistent-request.json' 2>&1 | Out-String | Write-Output
if ($LASTEXITCODE -eq 0) { throw 'Backend accepted installation without consent.' }
Write-Output 'PASS backend refuses setup before reading even the request file without consent.'
# The denied request is the expected oracle, not the test script's failure status.
$global:LASTEXITCODE=0
