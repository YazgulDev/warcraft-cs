$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'paths.ps1')
$build=Join-Path $root 'build'
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Production portable flight is checked without native targets or retail assets.
$script=Join-Path $build 'test-rpg.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/RocketTests.cpp')+'" /Fe:rpg-tests.exe'),
    'if errorlevel 1 exit /b 1','rpg-tests.exe','if errorlevel 1 exit /b 1',
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/RocketViewTests.cpp')+'" "'+(Join-Path $root 'src/presentation/RocketView.cpp')+'" "'+(Join-Path $root 'src/presentation/GunMesh.cpp')+'" "'+(Join-Path $root 'src/presentation/MuzzleFlashView.cpp')+'" /Fe:rpg-view-tests.exe user32.lib gdi32.lib opengl32.lib'),
    'if errorlevel 1 exit /b 1','rpg-view-tests.exe','if errorlevel 1 exit /b 1') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'RPG flight tests failed.'}}
finally {Pop-Location}
& (Get-WarcraftCsPython $root) (Join-Path $root 'tests/test_optional_rpg.py')
if($LASTEXITCODE){throw 'Optional RPG conversion tests failed.'}
