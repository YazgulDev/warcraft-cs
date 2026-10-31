$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build';New-Item -ItemType Directory -Force $build | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
$script=Join-Path $build 'test-settings-menu.cmd'
$sources=@('tests/SettingsMenuTests.cpp','src/config/SettingsMenu.cpp','src/config/GameplaySettings.cpp','src/platform/SettingsStore.cpp','src/input/ShortcutBindings.cpp') | ForEach-Object {'"'+(Join-Path $root $_)+'"'}
@('@echo off',('call "'+$environment+'" >nul'),('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX '+($sources -join ' ')+' /Fe:settings-menu-tests.exe'),'if not "%errorlevel%"=="0" exit /b 1','settings-menu-tests.exe') | Set-Content $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Settings menu regression failed.'}}
finally {Pop-Location}

$viewSources=@('tests/SettingsMenuViewTests.cpp','src/config/SettingsMenu.cpp','src/input/ShortcutBindings.cpp','src/presentation/SettingsMenuView.cpp','src/presentation/HudFont.cpp','src/presentation/HudDiagnostics.cpp','src/platform/DiagnosticLog.cpp') | ForEach-Object {'"'+(Join-Path $root $_)+'"'}
@('@echo off',('call "'+$environment+'" >nul'),('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX '+($viewSources -join ' ')+' /Fe:settings-menu-view-tests.exe /link user32.lib gdi32.lib opengl32.lib'),'if not "%errorlevel%"=="0" exit /b 1','settings-menu-view-tests.exe') | Set-Content $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Settings-menu graphics regression failed.'}}
finally {Pop-Location}
