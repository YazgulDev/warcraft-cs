$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Verify visual-only motion and flash clocks without native gameplay or retail assets.
$script=Join-Path $build 'test-viewmodel.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/ViewmodelMotionTests.cpp')+'" /Fe:viewmodel-tests.exe'),
    'if errorlevel 1 exit /b 1','viewmodel-tests.exe','if errorlevel 1 exit /b 1',
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/MuzzleFlashTests.cpp')+'" "'+(Join-Path $root 'src/presentation/GunMesh.cpp')+'" "'+(Join-Path $root 'src/presentation/MuzzleFlashView.cpp')+'" /Fe:muzzle-flash-tests.exe user32.lib gdi32.lib opengl32.lib'),
    'if errorlevel 1 exit /b 1','muzzle-flash-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if ($LASTEXITCODE) { throw 'Viewmodel motion verification failed.' };Write-Output 'PASS movement bob, stop/air/duck transitions, frame rates and one-shot muzzle presentation.' }
finally { Pop-Location }

. (Join-Path $PSScriptRoot "paths.ps1")
& (Get-WarcraftCsPython $root) (Join-Path $root "tests/test_export_muzzles.py")
if ($LASTEXITCODE) { throw "Muzzle export verification failed." }
