$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Synthetic call sites and real hidden WGL pixels verify both frame routes without Warcraft files.
$script=Join-Path $build 'test-native-frame-presentation.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/NativeFramePresentationTests.cpp')+'" "'+(Join-Path $root 'src/platform/NativeFramePresentation.cpp')+'" user32.lib gdi32.lib opengl32.lib /Fe:native-frame-presentation-tests.exe'),
'if errorlevel 1 exit /b 1','native-frame-presentation-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if ($LASTEXITCODE) {throw 'Native frame presentation verification failed.'}}
finally {Pop-Location}
