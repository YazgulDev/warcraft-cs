$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environmentScript=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Hidden WGL windows validate real pixels; every nonzero status, including signed native aborts, stops the suite.
$script=Join-Path $build 'test-hud-font.cmd'
$journal=Join-Path $build ('hud-journal-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $journal -Force | Out-Null
@('@echo off',('call "'+$environmentScript+'" >nul'),
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'src/presentation/HudFont.cpp')+'" "'+(Join-Path $root 'tests/HudFontTests.cpp')+'" "'+(Join-Path $root 'src/presentation/HudDiagnostics.cpp')+'" "'+(Join-Path $root 'src/platform/DiagnosticLog.cpp')+'" /Fe:hud-font-tests.exe /link user32.lib gdi32.lib opengl32.lib'),
'if not "%errorlevel%"=="0" exit /b 1','hud-font-tests.exe','if not "%errorlevel%"=="0" exit /b 1',
# The complete flat HUD uses the same state scope in the real overlay and this pixel regression.
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'src/presentation/HudFont.cpp')+'" "'+(Join-Path $root 'src/presentation/UnitHudView.cpp')+'" "'+(Join-Path $root 'tests/HudRenderStateTests.cpp')+'" "'+(Join-Path $root 'src/presentation/HudDiagnostics.cpp')+'" "'+(Join-Path $root 'src/platform/DiagnosticLog.cpp')+'" /Fe:hud-state-tests.exe /link user32.lib gdi32.lib opengl32.lib'),
'if not "%errorlevel%"=="0" exit /b 1','hud-state-tests.exe','if not "%errorlevel%"=="0" exit /b 1',
# A bound separate pipeline is invisible to CURRENT_PROGRAM and survives legacy glPushAttrib.
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/FixedFunctionStateTests.cpp')+'" "'+(Join-Path $root 'src/presentation/HudFont.cpp')+'" "'+(Join-Path $root 'src/presentation/HudDiagnostics.cpp')+'" "'+(Join-Path $root 'src/platform/DiagnosticLog.cpp')+'" /Fe:fixed-function-tests.exe /link user32.lib gdi32.lib opengl32.lib'),
'if not "%errorlevel%"=="0" exit /b 1','fixed-function-tests.exe','if not "%errorlevel%"=="0" exit /b 1',
# Verify bounded logging with real GL state without stealing focus or consuming the host error queue.
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/HudDiagnosticsTests.cpp')+'" "'+(Join-Path $root 'src/presentation/HudFont.cpp')+'" "'+(Join-Path $root 'src/presentation/HudDiagnostics.cpp')+'" "'+(Join-Path $root 'src/platform/DiagnosticLog.cpp')+'" /Fe:hud-log-tests.exe /link user32.lib gdi32.lib opengl32.lib'),
'if not "%errorlevel%"=="0" exit /b 1',('hud-log-tests.exe "'+$journal+'"')) | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'HUD text rasterization verification failed.'} }
finally {Pop-Location}
