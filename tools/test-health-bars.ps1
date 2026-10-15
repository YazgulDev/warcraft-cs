$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Production adapters use synthetic units; a hidden WGL window verifies actual depth-tested pixels.
$script=Join-Path $build 'test-health-bars.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/UnitHealthBarsTests.cpp')+'" "'+(Join-Path $root 'src/platform/UnitHealthBars.cpp')+'" /Fe:unit-health-bars-tests.exe'),
'if errorlevel 1 exit /b 1','unit-health-bars-tests.exe','if errorlevel 1 exit /b 1',
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/HealthBarViewTests.cpp')+'" "'+(Join-Path $root 'src/presentation/HealthBarRenderer.cpp')+'" "'+(Join-Path $root 'src/presentation/TargetMarkerView.cpp')+'" /Fe:health-bar-view-tests.exe /link user32.lib gdi32.lib opengl32.lib'),
'if errorlevel 1 exit /b 1','health-bar-view-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Health bar regression failed.'} }
finally {Pop-Location}
