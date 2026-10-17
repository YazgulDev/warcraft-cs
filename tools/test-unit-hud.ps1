$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Actual WGL pixels verify animation refresh and world color/depth isolation without game files or focus.
$script=Join-Path $build 'test-unit-hud.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/UnitHudTests.cpp')+'" "'+(Join-Path $root 'tests/NativeUnitHudTests.cpp')+'" "'+(Join-Path $root 'src/platform/NativeUnitHud.cpp')+'" "'+(Join-Path $root 'src/platform/NativePortrait.cpp')+'" user32.lib gdi32.lib opengl32.lib /Fe:unit-hud-tests.exe'),
'if errorlevel 1 exit /b 1','unit-hud-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Unit HUD regression failed.'};Write-Output 'PASS unit HUD layouts, portrait animation refresh, world color/depth isolation and hidden lifecycle.'}
finally {Pop-Location}
