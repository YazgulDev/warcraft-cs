$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Exercise the camera adapter and projection with a synthetic native image, without game assets.
$script=Join-Path $build 'test-map-camera-guard.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/MapCameraGuardTests.cpp')+'" "'+(Join-Path $root 'src/platform/MapCameraGuard.cpp')+'" "'+(Join-Path $root 'src/presentation/FirstPersonCamera.cpp')+'" /Fe:map-camera-guard-tests.exe'),
'if errorlevel 1 exit /b 1','map-camera-guard-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if ($LASTEXITCODE) {throw 'Map camera guard verification failed.'} }
finally {Pop-Location}
