$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Hidden native windows reproduce movie subclass resets without touching a Warcraft save.
$script=Join-Path $build 'test-window-input.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
('cl /nologo /std:c++17 /EHsc /O2 /W4 "'+(Join-Path $root 'tests/GameWindowInputTests.cpp')+'" "'+(Join-Path $root 'src/platform/GameWindowInput.cpp')+'" user32.lib /Fe:window-input-tests.exe'),
'if errorlevel 1 exit /b 1','window-input-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if ($LASTEXITCODE) {throw 'Window input recovery verification failed.'} }
finally {Pop-Location}
