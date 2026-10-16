$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environmentScript=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# A hidden WGL window validates glyph coverage without moving the user's mouse or opening Warcraft.
$script=Join-Path $build 'test-hud-font.cmd'
@('@echo off',('call "'+$environmentScript+'" >nul'),
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'src/presentation/HudFont.cpp')+'" "'+(Join-Path $root 'tests/HudFontTests.cpp')+'" /Fe:hud-font-tests.exe /link user32.lib gdi32.lib opengl32.lib'),
'if errorlevel 1 exit /b 1','hud-font-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'HUD text rasterization verification failed.'} }
finally {Pop-Location}
