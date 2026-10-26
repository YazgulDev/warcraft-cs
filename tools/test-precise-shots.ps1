$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Project-authored meshes and synthetic native memory exercise the real parser and transform without a game.
$script=Join-Path $build 'test-precise-shots.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/AnimatedHitModelTests.cpp')+'" /Fe:animated-hit-tests.exe'),
    'if errorlevel 1 exit /b 1','animated-hit-tests.exe','if errorlevel 1 exit /b 1',
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/UnitShotHitboxTests.cpp')+'" "'+(Join-Path $root 'src/combat/UnitHitboxes.cpp')+'" /Fe:unit-shot-tests.exe'),
     'if errorlevel 1 exit /b 1','unit-shot-tests.exe','if errorlevel 1 exit /b 1',
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/WeaponTraceHoverTests.cpp')+'" "'+(Join-Path $root 'src/platform/WeaponTrace.cpp')+'" "'+(Join-Path $root 'src/platform/ItemHover.cpp')+'" "'+(Join-Path $root 'src/input/ShortcutBindings.cpp')+'" /Fe:trace-hover-tests.exe'),
    'if errorlevel 1 exit /b 1','trace-hover-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Precise shot verification failed.'}}
finally {Pop-Location}
