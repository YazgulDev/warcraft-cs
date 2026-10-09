$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$envScript=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Exercise the production reticle on native WGL pixels with inherited Warcraft-style render state.
$script=Join-Path $build 'test-reticle-view.cmd'
@('@echo off',('call "'+$envScript+'" >nul'),
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'src/presentation/ReticleView.cpp')+'" "'+(Join-Path $root 'tests/ReticleViewTests.cpp')+'" /Fe:reticle-view-tests.exe /link user32.lib gdi32.lib opengl32.lib'),
'if errorlevel 1 exit /b 1','reticle-view-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Reticle rasterization verification failed.'} }
finally {Pop-Location}
