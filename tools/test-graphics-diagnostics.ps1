$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build/graphics-diagnostics-tests'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
$journal=Join-Path $build ('journal-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $journal -Force | Out-Null
# Separate objects and a hidden real context exercise driver delivery without changing the running game.
$sources=@('tests/GraphicsDiagnosticsTests.cpp','src/presentation/GraphicsDiagnostics.cpp','src/presentation/HudDiagnostics.cpp','src/platform/DiagnosticLog.cpp','src/platform/NativeFaultDiagnostics.cpp') | ForEach-Object {'"'+(Join-Path $root $_)+'"'}
$script=Join-Path $build 'test-graphics-diagnostics.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX '+($sources -join ' ')+' /Fe:graphics-diagnostics-tests.exe /link opengl32.lib user32.lib gdi32.lib'),
    'if not "%errorlevel%"=="0" exit /b 1',('graphics-diagnostics-tests.exe "'+$journal+'"')) | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Graphics observer verification failed.'}}
finally {Pop-Location}
