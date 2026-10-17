$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build';New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Real command/formation ownership is tested with authored unit identities/orders, without retail data or input.
$sources=@('tests/SquadFocusFireTests.cpp','src/squad/SquadFocusFire.cpp','src/squad/SquadController.cpp','src/input/ShortcutBindings.cpp') | ForEach-Object {'"'+(Join-Path $root $_)+'"'}
$script=Join-Path $build 'test-squad-reticle.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX '+($sources -join ' ')+' /Fe:squad-reticle-tests.exe'),
    'if not "%errorlevel%"=="0" exit /b 1','squad-reticle-tests.exe','if not "%errorlevel%"=="0" exit /b 1') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Squad reticle regression failed.'};Write-Output 'PASS all-owned targeting, passive override, cancel/restore, external orders, stale handles and teardown.'}
finally {Pop-Location}
