$ErrorActionPreference='Stop'
$modRoot=Split-Path -Parent $PSScriptRoot
$build=Join-Path $modRoot 'build'
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# The combined fixture retains many full settings snapshots; give this test enough stack for those independent cases.
# Production reload owns a single snapshot and does not need a larger game-thread stack.
$script=Join-Path $build 'test-gameplay-settings.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $modRoot 'tests/GameplaySettingsTests.cpp')+'" "'+(Join-Path $modRoot 'src/config/GameplaySettings.cpp')+'" "'+(Join-Path $modRoot 'src/movement/MovementPhysics.cpp')+'" "'+(Join-Path $modRoot 'src/input/ShortcutBindings.cpp')+'" /Fe:gameplay-settings-tests.exe /link /STACK:4194304'),
    'if errorlevel 1 exit /b 1','gameplay-settings-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if ($LASTEXITCODE) { throw 'Gameplay settings verification failed.' };Write-Output 'Config, damage and rune ammunition invariants passed.' }
finally { Pop-Location }
