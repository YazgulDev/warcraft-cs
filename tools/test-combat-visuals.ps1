$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Portable visual physics uses copied values, never Warcraft or controller input.
# A negative native abort status is still a failure; stop before a later executable can hide it.
$script=Join-Path $build 'test-combat-visuals.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/CombatVisualsTests.cpp')+'" "'+(Join-Path $root 'src/presentation/CombatVisuals.cpp')+'" /Fe:combat-visuals-tests.exe'),
     'if not "%errorlevel%"=="0" exit /b 1','combat-visuals-tests.exe','if not "%errorlevel%"=="0" exit /b 1',
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/CombatVisualRenderTests.cpp')+'" "'+(Join-Path $root 'src/presentation/CombatVisuals.cpp')+'" "'+(Join-Path $root 'src/presentation/CombatVisualView.cpp')+'" "'+(Join-Path $root 'src/presentation/ItemNameView.cpp')+'" /Fe:combat-visual-render-tests.exe /link opengl32.lib gdi32.lib user32.lib'),
    'if not "%errorlevel%"=="0" exit /b 1','combat-visual-render-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Combat visual physics failed.'}}
finally {Pop-Location}
