$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Exercise the controller's fitted shell timeline and the renderer's terminal-frame policy without retail assets.
$script=Join-Path $build 'test-shotgun-reload.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/ShotgunReloadTests.cpp')+'" /Fe:shotgun-reload-tests.exe'),
    'if errorlevel 1 exit /b 1','shotgun-reload-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if ($LASTEXITCODE) { throw 'Shotgun reload verification failed.' };Write-Output 'PASS shotgun timeline, frame overshoot, missing metadata and terminal reload poses.' }
finally { Pop-Location }
