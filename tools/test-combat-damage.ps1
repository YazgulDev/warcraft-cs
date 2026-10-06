$ErrorActionPreference = 'Stop'
$modRoot = Split-Path -Parent $PSScriptRoot
$build = Join-Path $modRoot 'build'
$vs = & 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment = Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Keep friendly-fire damage rules deterministic; the live game separately verifies model data and native damage.
$script = Join-Path $build 'test-combat-damage.cmd'
@('@echo off', ('call "' + $environment + '" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 "' + (Join-Path $modRoot 'tests/CombatDamageTests.cpp') + '" /Fe:combat-damage-tests.exe'),
    'if errorlevel 1 exit /b 1', 'combat-damage-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script; if ($LASTEXITCODE) { throw 'Combat damage verification failed.' } }
finally { Pop-Location }
