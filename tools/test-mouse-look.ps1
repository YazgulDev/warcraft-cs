$ErrorActionPreference = 'Stop'
$modRoot = Split-Path -Parent $PSScriptRoot
$build = Join-Path $modRoot 'build'
$vs = & 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment = Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Native input/rendering is checked in-game; these regressions cover fast turns and wraparound math.
$script = Join-Path $build 'test-mouse-look.cmd'
@('@echo off', ('call "' + $environment + '" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 "' + (Join-Path $modRoot 'tests/LookAnglesTests.cpp') + '" /Fe:mouse-look-tests.exe'),
    'if errorlevel 1 exit /b 1', 'mouse-look-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script; if ($LASTEXITCODE) { throw 'Mouse look verification failed.' } }
finally { Pop-Location }
