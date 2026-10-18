$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Synthetic trajectories and throw clocks require neither game assets nor interactive input.
$script=Join-Path $build 'test-grenades.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/GrenadeTests.cpp')+'" /Fe:grenade-tests.exe'),
    'if errorlevel 1 exit /b 1','grenade-tests.exe','if errorlevel 1 exit /b 1',
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/NativeGrenadeTests.cpp')+'" "'+(Join-Path $root 'src/platform/NativeGrenades.cpp')+'" "'+(Join-Path $root 'src/input/ShortcutBindings.cpp')+'" /Fe:native-grenade-tests.exe'),
    'if errorlevel 1 exit /b 1','native-grenade-tests.exe','if errorlevel 1 exit /b 1',
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/GrenadeViewTests.cpp')+'" "'+(Join-Path $root 'src/presentation/GrenadeView.cpp')+'" /Fe:grenade-view-tests.exe user32.lib gdi32.lib opengl32.lib'),
    'if errorlevel 1 exit /b 1','grenade-view-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if ($LASTEXITCODE) { throw 'Grenade verification failed.' };Write-Output 'PASS pin/release/launch cancellation, trajectory, thin-wall bounce, fuse, falloff and facing.' }
finally { Pop-Location }
