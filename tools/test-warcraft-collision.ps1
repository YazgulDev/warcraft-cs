$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$envScript=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Compile the actual movement/collision code with a synthetic native world, requiring no game assets.
$script=Join-Path $build 'test-warcraft-collision.cmd'
@('@echo off',('call "'+$envScript+'" >nul'),
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'src/movement/MovementPhysics.cpp')+'" "'+(Join-Path $root 'src/platform/WarcraftCollision.cpp')+'" "'+(Join-Path $root 'tests/WarcraftCollisionTests.cpp')+'" /Fe:warcraft-collision-tests.exe'),
'if errorlevel 1 exit /b 1','warcraft-collision-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Warcraft collision verification failed.'} }
finally {Pop-Location}
