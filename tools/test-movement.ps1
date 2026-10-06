$ErrorActionPreference = 'Stop'
$modRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $modRoot 'build'
New-Item -ItemType Directory -Path $buildDirectory -Force | Out-Null
$compilerRoot = & 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environmentScript = Join-Path $compilerRoot 'VC/Auxiliary/Build/vcvars32.bat'
$script = Join-Path $buildDirectory 'test-movement.cmd'
# Physics tests run without Warcraft, keeping deterministic motion checks independent of UI input.
@('@echo off', ('call "' + $environmentScript + '" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 "' + (Join-Path $modRoot 'src/MovementPhysics.cpp') + '" "' + (Join-Path $modRoot 'tests/MovementPhysicsTests.cpp') + '" /Fe:movement-tests.exe'),
    'if errorlevel 1 exit /b 1', 'movement-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $buildDirectory
try { & $env:COMSPEC /d /c $script; if ($LASTEXITCODE) { throw 'Movement verification failed.' } }
finally { Pop-Location }
