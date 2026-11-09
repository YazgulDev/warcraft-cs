$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Exercise real native discovery/cache routing with synthetic model providers and no game memory.
$script=Join-Path $build 'test-movement-obstacles.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/MovementObstaclesTests.cpp')+'" "'+(Join-Path $root 'src/platform/MovementObstacles.cpp')+'" "'+(Join-Path $root 'src/platform/MapRockObstacles.cpp')+'" "'+(Join-Path $root 'src/geometry/TreeTrunkMesh.cpp')+'" /Fe:movement-obstacles-tests.exe'),
'if errorlevel 1 exit /b 1','movement-obstacles-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if ($LASTEXITCODE) {throw 'Movement obstacle cache verification failed.'} }
finally {Pop-Location}
