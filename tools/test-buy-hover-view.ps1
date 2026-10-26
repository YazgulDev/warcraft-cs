$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Hidden WGL renders the actual menu and localized captions without controlling Warcraft or taking focus.
$script=Join-Path $build 'test-buy-hover-view.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/BuyHoverViewTests.cpp')+'" "'+(Join-Path $root 'src/presentation/BuyMenuView.cpp')+'" "'+(Join-Path $root 'src/presentation/HudFont.cpp')+'" "'+(Join-Path $root 'src/presentation/ItemNameView.cpp')+'" "'+(Join-Path $root 'src/presentation/HudDiagnostics.cpp')+'" "'+(Join-Path $root 'src/platform/DiagnosticLog.cpp')+'" "'+(Join-Path $root 'src/config/GameplaySettings.cpp')+'" "'+(Join-Path $root 'src/input/ShortcutBindings.cpp')+'" /Fe:buy-hover-view-tests.exe /link user32.lib gdi32.lib opengl32.lib'),
    'if errorlevel 1 exit /b 1','buy-hover-view-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Buy/hover rendering verification failed.'}}
finally {Pop-Location}
