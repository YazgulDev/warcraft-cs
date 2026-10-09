$ErrorActionPreference='Stop'
$modRoot=Split-Path -Parent $PSScriptRoot
$build=Join-Path $modRoot 'build'
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Validate editable damage settings and bounded/fractional rune ammo rewards independently of native gameplay.
$script=Join-Path $build 'test-gameplay-settings.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $modRoot 'tests/GameplaySettingsTests.cpp')+'" "'+(Join-Path $modRoot 'src/config/GameplaySettings.cpp')+'" "'+(Join-Path $modRoot 'src/movement/MovementPhysics.cpp')+'" /Fe:gameplay-settings-tests.exe'),
    'if errorlevel 1 exit /b 1','gameplay-settings-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if ($LASTEXITCODE) { throw 'Gameplay settings verification failed.' };Write-Output 'Config, damage and rune ammunition invariants passed.' }
finally { Pop-Location }
