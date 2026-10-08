$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$envScript=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Compile the actual viewport policy with synthetic world and portrait rectangles.
$script=Join-Path $build 'test-fullscreen-view.cmd'
@('@echo off',('call "'+$envScript+'" >nul'),
('cl /nologo /std:c++17 /EHsc /O2 /W4 "'+(Join-Path $root 'src/FullscreenView.cpp')+'" "'+(Join-Path $root 'tests/FullscreenViewTests.cpp')+'" /Fe:fullscreen-view-tests.exe'),
'if errorlevel 1 exit /b 1','fullscreen-view-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Fullscreen viewport verification failed.'} }
finally {Pop-Location}
