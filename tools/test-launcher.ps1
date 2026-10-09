$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build/launcher'
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$sources=@(Get-ChildItem -LiteralPath (Join-Path $root 'launcher') -Filter '*.cs' -File -Recurse | Where-Object { $_.Name -ne 'Program.cs' } | ForEach-Object { $_.FullName })
$payload=Join-Path $build 'Source.zip'
# Exercise the same version-specific fallback resources compiled into both real launchers.
$notesResources=@(Get-ChildItem -LiteralPath (Join-Path $root 'docs/launcher-changes') -Filter '*.txt' -File | ForEach-Object {
    '/resource:'+ $_.FullName+',WarcraftCS.ReleaseNotes.'+$_.Name
})
if (!(Test-Path -LiteralPath $payload)) { throw 'Build the launcher first to create its audited source payload.' }
$tests=Join-Path $build 'LauncherTests.exe'
& $compiler /nologo /target:exe /platform:x64 "/out:$tests" "/resource:$payload,WarcraftCS.Source.zip" $notesResources /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll /r:System.Web.Extensions.dll /r:System.Windows.Forms.dll /r:System.Drawing.dll (Join-Path $root 'tests/LauncherTests.cs') $sources
if ($LASTEXITCODE) { throw 'Launcher test compilation failed.' }
& $tests (Join-Path $root ('.local/launcher-tests-'+[guid]::NewGuid().ToString('N')))
if ($LASTEXITCODE) { throw 'Launcher regression checks failed.' }
# The embedded setup must retain the codec/provider folder in both new and previously installed clients.
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root 'tools/test-audio-runtime.ps1')
if ($LASTEXITCODE) { throw 'Audio runtime installation checks failed.' }
# The shared Install/Update repair policy must run on Windows PowerShell, like the shipped backend.
# Run junction creation separately when the shell's sandbox allows Windows reparse points.
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root 'tools/test-warcraft-runtime.ps1') -SkipJunction
if ($LASTEXITCODE) { throw 'Warcraft full reinstall checks failed.' }
# Parse every script for Windows PowerShell and enforce a backend consent denial before any side effects.
foreach ($script in @(Get-ChildItem -LiteralPath (Join-Path $root 'launcher') -Filter '*.ps1')) {
    $tokens=$null;$errors=$null
    [Management.Automation.Language.Parser]::ParseFile($script.FullName,[ref]$tokens,[ref]$errors) | Out-Null
    if ($errors) { throw "Script parse errors: $($script.Name)" }
}
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root 'launcher/install-client.ps1') -RequestFile 'nonexistent-request.json' 2>&1 | Out-String | Write-Output
if ($LASTEXITCODE -eq 0) { throw 'Backend accepted installation without consent.' }
Write-Output 'PASS backend refuses setup before reading even the request file without consent.'
# Exercise release selection and package integrity offline, without downloading or rebuilding the player's game.
$updateTests=Join-Path $build 'LauncherUpdateTests.exe'
& $compiler /nologo /target:exe /platform:x64 "/out:$updateTests" "/resource:$payload,WarcraftCS.Source.zip" $notesResources /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll /r:System.Web.Extensions.dll /r:System.Windows.Forms.dll /r:System.Drawing.dll (Join-Path $root 'tests/LauncherUpdateTests.cs') $sources
if ($LASTEXITCODE) { throw 'Updater test compilation failed.' }
& $updateTests (Join-Path $root ('.local/update-tests-'+[guid]::NewGuid().ToString('N')))
if ($LASTEXITCODE) { throw 'Updater regression checks failed.' }
# A project-authored DLL supplies version metadata only; no Warcraft binaries/assets or dependencies are used.
$versionFixture=Join-Path $build 'VersionFixture.cs'
'[assembly: System.Reflection.AssemblyFileVersion("1.26.0.6401")] public sealed class VersionFixture {}' | Set-Content -LiteralPath $versionFixture -Encoding ascii
$versionDll=Join-Path $build 'VersionFixture.dll'
& $compiler /nologo /target:library /platform:x86 "/out:$versionDll" $versionFixture
if ($LASTEXITCODE) { throw 'Version fixture compilation failed.' }
$reinstallTests=Join-Path $build 'LauncherReinstallTests.exe'
& $compiler /nologo /target:exe /platform:x64 "/out:$reinstallTests" "/resource:$payload,WarcraftCS.Source.zip" $notesResources /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll /r:System.Web.Extensions.dll /r:System.Windows.Forms.dll /r:System.Drawing.dll (Join-Path $root 'tests/LauncherReinstallTests.cs') $sources
if ($LASTEXITCODE) { throw 'Reinstall integration compilation failed.' }
& $reinstallTests (Join-Path $root ('.local/reinstall-tests-'+[guid]::NewGuid().ToString('N'))) $versionDll
if ($LASTEXITCODE) { throw 'Reinstall integration checks failed.' }
& (Join-Path $PSScriptRoot 'test-launcher-replacement.ps1')
# The denied request is the expected oracle, not the test script's failure status.
$global:LASTEXITCODE=0
