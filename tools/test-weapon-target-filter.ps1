$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Validate real native-fact classification without any game files or physical input.
$script=Join-Path $build 'test-weapon-target-filter.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/WeaponTargetFilterTests.cpp')+'" "'+(Join-Path $root 'src/platform/WeaponTargetFilter.cpp')+'" "'+(Join-Path $root 'src/platform/UnitCollisionProfile.cpp')+'" /Fe:weapon-target-filter-tests.exe'),
    'if errorlevel 1 exit /b 1','weapon-target-filter-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Weapon target filter verification failed.'};Write-Output 'PASS effect transparency, physical immunity and conservative unknown collision.'}
finally {Pop-Location}
