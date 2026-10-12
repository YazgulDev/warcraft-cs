$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build';New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Exercise actual config reload and attack/camera contracts without retail assets or desktop input.
$sources=@('tests/ThirdPersonTests.cpp','src/config/GameplaySettings.cpp','src/input/ShortcutBindings.cpp') | ForEach-Object {'"'+(Join-Path $root $_)+'"'}
$script=Join-Path $build 'test-third-person.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX '+($sources -join ' ')+' /Fe:third-person-tests.exe'),
    'if errorlevel 1 exit /b 1','third-person-tests.exe','if errorlevel 1 exit /b 1') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Third-person regression failed.'};Write-Output 'PASS third-person boom, swing timeline and real config/reload contracts.'}
finally {Pop-Location}
