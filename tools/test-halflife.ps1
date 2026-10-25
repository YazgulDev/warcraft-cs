$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'paths.ps1')
$build=Join-Path $root 'build'
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Independent energy/ownership contracts use synthetic data, without a Warcraft process.
$script=Join-Path $build 'test-halflife.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/HalfLifeEnergyTests.cpp')+'" "'+(Join-Path $root 'src/input/ShortcutBindings.cpp')+'" /Fe:halflife-tests.exe'),
    'if errorlevel 1 exit /b 1','halflife-tests.exe','if not %errorlevel%==0 exit /b 1',
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/EnergyBeamViewTests.cpp')+'" /Fe:energy-beam-tests.exe user32.lib gdi32.lib opengl32.lib'),
    'if errorlevel 1 exit /b 1','energy-beam-tests.exe','if not %errorlevel%==0 exit /b 1',
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/StudioPoseTests.cpp')+'" /Fe:studio-pose-tests.exe'),
    'if errorlevel 1 exit /b 1','studio-pose-tests.exe','if not %errorlevel%==0 exit /b 1',
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/StudioMeshTests.cpp')+'" "'+(Join-Path $root 'src/presentation/GunMesh.cpp')+'" "'+(Join-Path $root 'src/presentation/MuzzleFlashView.cpp')+'" /Fe:studio-mesh-tests.exe user32.lib gdi32.lib opengl32.lib'),
    'if errorlevel 1 exit /b 1','studio-mesh-tests.exe','if not %errorlevel%==0 exit /b 1') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Half-Life energy tests failed.'}}
finally {Pop-Location}
& (Get-WarcraftCsPython $root) (Join-Path $root 'tests/test_optional_halflife.py')
if($LASTEXITCODE){throw 'Optional Half-Life conversion tests failed.'}
& (Get-WarcraftCsPython $root) (Join-Path $root 'tests/test_studio_presentation.py')
if($LASTEXITCODE){throw 'Studio model presentation conversion failed.'}
