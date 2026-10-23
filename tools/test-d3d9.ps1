$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build/d3d9-tests'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Real hidden-device pixels catch matrix, texture, alpha-atlas and restoration errors without game assets or input.
$sources=@('tests/D3DGraphicsTests.cpp','src/presentation/D3DGraphics.cpp','src/presentation/D3DTextures.cpp','src/presentation/HudFont.cpp','src/presentation/HudDiagnostics.cpp','src/platform/DiagnosticLog.cpp') | ForEach-Object {'"'+(Join-Path $root $_)+'"'}
$script=Join-Path $build 'test-d3d9.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX /DWCS_D3D9 '+($sources -join ' ')+' /Fe:d3d9-tests.exe /link d3d9.lib opengl32.lib user32.lib gdi32.lib'),
    'if errorlevel 1 exit /b 1','d3d9-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'D3D9 rendering verification failed.'}}
finally {Pop-Location}
