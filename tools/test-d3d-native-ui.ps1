$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build/d3d-native-ui-tests';New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Verify the actual COM slot filter and native texture capture on a hidden HAL device, with no game assets or input.
$sources=@('tests/D3DNativeUiTests.cpp','src/platform/D3DNativeUi.cpp','src/presentation/D3DGraphics.cpp','src/presentation/D3DTextures.cpp','src/presentation/GraphicsDiagnostics.cpp','src/presentation/HudDiagnostics.cpp','src/platform/DiagnosticLog.cpp') | ForEach-Object {'"'+(Join-Path $root $_)+'"'}
$script=Join-Path $build 'test.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /MT /std:c++17 /EHsc /O2 /W4 /DNOMINMAX '+($sources -join ' ')+' /Fe:ui-tests.exe /link d3d9.lib opengl32.lib user32.lib gdi32.lib'),
    'if errorlevel 1 exit /b 1','ui-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Native D3D UI verification failed.'}}
finally {Pop-Location}
