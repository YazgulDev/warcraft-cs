$ErrorActionPreference = 'Stop'
$modRoot = Split-Path -Parent $PSScriptRoot
$build = Join-Path $modRoot 'build'
$vs = & 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment = Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Keep geometric failure cases deterministic; the live game separately verifies model data and native damage.
$script = Join-Path $build 'test-hitboxes.cmd'
@('@echo off', ('call "' + $environment + '" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 "' + (Join-Path $modRoot 'tests/RayBoundsTests.cpp') + '" /Fe:hitbox-tests.exe'),
    'if errorlevel 1 exit /b 1', 'hitbox-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script; if ($LASTEXITCODE) { throw 'Hitbox verification failed.' } }
finally { Pop-Location }
